"""Verificación del generador y de las instancias (resultado 7, «Verificación y validación»).

Comprobaciones, cada una con su resultado:
  1. Reproducibilidad: genera todo de nuevo en una carpeta temporal y lo compara byte a byte con instancias/.
  2. Generadoras: en cada instancia que debe acertar, la generadora es admisible con D = 0 y Z = 0,5.
  3. Óptimos exactos: cada exacta.csv es admisible y su Z coincide con referencias.csv. En las instancias
     que deben acertar, Z* ≤ 0,5; en las que deben fallar, D* ≥ D_cota > 0. Si está el exactos.csv del
     proyecto completo, los Z* de exp_ y rob_ coinciden con los anteriores.
  4. ssa_basico sobre el conjunto de calibración y la instancia de catálogo vacío. En ninguna repetición
     puede salir Z por debajo de la cota inferior de Z*, ni D por debajo de D_cota en las de fallo.
     Además informa cuánto acierta el algoritmo: repeticiones con Z ≤ 0,5 y brecha media (Z − Z*)/Z*.
  5. Entradas inválidas: ssa_basico termina con código 1 y el mensaje esperado.

Uso (en PyCharm, ejecutar este archivo; o desde la carpeta del proyecto):
    .venv/Scripts/python generador/verificar.py [--binario ruta/ssa_basico] [--repeticiones 30] [--exacto]
--exacto vuelve a resolver los óptimos antes de comprobarlos. Termina con «Todo se cumple.» y código 0
si las cinco comprobaciones pasan.
"""
import argparse
import filecmp
import os
import statistics
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

from construccion import CARPETA_INSTANCIAS, CARPETA_PROYECTO, escribir_csv, leer_csv
from evaluar import Instancia, evaluar, leer_solucion
import exacto
import generar

TOLERANCIA = 1e-9
APTITUD_GENERADORA = 0.5
ARCHIVOS_DE_REFERENCIA = {"exacta.csv", "referencias.csv"}  # los escribe exacto.py, no el generador
EXACTOS_ANTERIORES = os.path.join(os.path.dirname(CARPETA_PROYECTO), "Algoritmo de Arañas Sociales", "datos",
                                  "instancias", "una_etapa", "exactos.csv")
# Parámetros de partida del algoritmo (Yu y Li, 2015a; población propia), los de SSA_Basico
PARAMETROS_SSA = dict(poblacion=30, ra=1.0, pc=0.7, pm=0.1, iteraciones=1000)


def buscar_ssa_basico():
    """Busca el programa compilado al lado del proyecto: en Tesis/SSA_Basico o en la raíz del repositorio."""
    if os.environ.get("SSA_BASICO"):
        return os.environ["SSA_BASICO"]
    carpeta_superior = os.path.dirname(CARPETA_PROYECTO)
    for proyecto in (os.path.join(carpeta_superior, "SSA_Basico"), carpeta_superior):
        for compilacion in ("cmake-build-release", "build", "cmake-build-debug"):
            for nombre in ("ssa_basico.exe", "ssa_basico"):
                ruta = os.path.join(proyecto, compilacion, nombre)
                if os.path.isfile(ruta):
                    return ruta
    sys.exit("No se encontró ssa_basico compilado; indíquelo con --binario o la variable SSA_BASICO")


def comprobar_reproducibilidad():
    with tempfile.TemporaryDirectory() as carpeta_temporal:
        generar.generar_todo(carpeta_temporal)
        distintos = []
        for raiz, _, archivos in os.walk(carpeta_temporal):
            for archivo in archivos:
                relativo = os.path.relpath(os.path.join(raiz, archivo), carpeta_temporal)
                guardado = os.path.join(CARPETA_INSTANCIAS, relativo)
                if not os.path.isfile(guardado) or not filecmp.cmp(os.path.join(raiz, archivo), guardado,
                                                                  shallow=False):
                    distintos.append(relativo)
        for raiz, _, archivos in os.walk(CARPETA_INSTANCIAS):
            for archivo in archivos:
                relativo = os.path.relpath(os.path.join(raiz, archivo), CARPETA_INSTANCIAS)
                if archivo not in ARCHIVOS_DE_REFERENCIA and not os.path.isfile(
                        os.path.join(carpeta_temporal, relativo)):
                    distintos.append(f"{relativo} (sobra)")
        carpetas = sum(os.path.isdir(os.path.join(carpeta_temporal, d)) for d in os.listdir(carpeta_temporal))
    return carpetas, distintos


def comprobar_generadoras(manifiesto):
    fallas = []
    de_acierto = [fila for fila in manifiesto if fila["tipo"] == "acierto"]
    for fila in de_acierto:
        carpeta = os.path.join(CARPETA_INSTANCIAS, fila["instancia"])
        instancia = Instancia(carpeta)
        resultado = evaluar(instancia, leer_solucion(os.path.join(carpeta, "generadora.csv"), instancia))
        if not (resultado["admisible"] and resultado["D"] == 0.0
                and abs(resultado["Z"] - APTITUD_GENERADORA) <= 1e-12):
            fallas.append(f"{fila['instancia']}: Z = {resultado['Z']}, D = {resultado['D']} {resultado['motivo']}")
    return len(de_acierto), fallas


def comprobar_exactos(referencias):
    fallas = []
    for fila in referencias:
        carpeta = os.path.join(CARPETA_INSTANCIAS, fila["instancia"])
        instancia = Instancia(carpeta)
        resultado = evaluar(instancia, leer_solucion(os.path.join(carpeta, "exacta.csv"), instancia))
        z_exacto = float(fila["Z_exacto"])
        problemas = []
        if fila["estado"] != "0":
            problemas.append(f"estado {fila['estado']}")
        if not resultado["admisible"]:
            problemas.append(resultado["motivo"])
        if abs(resultado["Z"] - z_exacto) > TOLERANCIA:
            problemas.append(f"Z evaluada {resultado['Z']} ≠ Z de HiGHS {z_exacto}")
        if fila["tipo"] == "acierto" and z_exacto > APTITUD_GENERADORA + 1e-12:
            problemas.append(f"Z* = {z_exacto} > 0,5")
        if fila["tipo"] == "fallo":
            d_cota = float(fila["D_cota"])
            if not (d_cota > 0 and float(fila["D_exacto"]) >= d_cota - TOLERANCIA):
                problemas.append(f"D* = {fila['D_exacto']} por debajo de D_cota = {d_cota}")
        if problemas:
            fallas.append(f"{fila['instancia']}: {'; '.join(problemas)}")
    comparadas, diferencia_maxima = 0, 0.0
    if os.path.isfile(EXACTOS_ANTERIORES):
        anteriores = {fila["instancia"]: float(fila["Z"]) for fila in leer_csv(EXACTOS_ANTERIORES)}
        for fila in referencias:
            if fila["instancia"] in anteriores:
                comparadas += 1
                diferencia = abs(float(fila["Z_exacto"]) - anteriores[fila["instancia"]])
                diferencia_maxima = max(diferencia_maxima, diferencia)
                if diferencia > TOLERANCIA:
                    fallas.append(f"{fila['instancia']}: Z* = {fila['Z_exacto']}, antes {anteriores[fila['instancia']]}")
    return fallas, comparadas, diferencia_maxima


def ejecutar_ssa_basico(binario, carpeta_instancia, archivo_parametros, archivo_salida):
    proceso = subprocess.run([binario, carpeta_instancia, archivo_parametros, archivo_salida],
                             capture_output=True)
    return proceso.returncode, proceso.stderr.decode("utf-8", errors="replace").strip()


def comprobar_algoritmo(binario, referencias, repeticiones):
    """Ejecuta ssa_basico en las instancias de calibración y verificación y compara con sus referencias."""
    elegidas = [fila for fila in referencias if fila["conjunto"] in ("calibracion", "verificacion")]
    fallas, informe = [], {"acierto": [], "fallo": []}
    with tempfile.TemporaryDirectory() as carpeta_temporal:
        archivo_parametros = os.path.join(carpeta_temporal, "parametros_ssa.csv")
        escribir_csv(archivo_parametros, ["clave", "valor"],
                     list(PARAMETROS_SSA.items()) + [("repeticiones", repeticiones)])

        def correr(fila):
            salida = os.path.join(carpeta_temporal, fila["instancia"] + ".csv")
            codigo, error = ejecutar_ssa_basico(binario, os.path.join(CARPETA_INSTANCIAS, fila["instancia"]),
                                                archivo_parametros, salida)
            return fila, codigo, error, (leer_csv(salida) if codigo == 0 else [])

        with ThreadPoolExecutor(max_workers=min(16, os.cpu_count() or 1)) as ejecutor:
            corridas = list(ejecutor.map(correr, elegidas))
    for fila, codigo, error, ejecuciones in corridas:
        if codigo != 0:
            fallas.append(f"{fila['instancia']}: ssa_basico terminó con código {codigo}: {error}")
            continue
        z_exacto, z_cota = float(fila["Z_exacto"]), float(fila["Z_cota_inferior"])
        for ejecucion in ejecuciones:
            aptitud, desviacion = float(ejecucion["Z"]), float(ejecucion["D"])
            if aptitud < z_cota - TOLERANCIA:
                fallas.append(f"{fila['instancia']} rep {ejecucion['repeticion']}: Z = {aptitud} < cota de Z* = {z_cota}")
            if fila["tipo"] == "fallo" and desviacion < float(fila["D_cota"]) - TOLERANCIA:
                fallas.append(f"{fila['instancia']} rep {ejecucion['repeticion']}: D = {desviacion} < D_cota = {fila['D_cota']}")
            informe[fila["tipo"]].append(dict(instancia=fila["instancia"], caso=fila["caso"], Z=aptitud, D=desviacion,
                                              brecha=(aptitud - z_exacto) / z_exacto,
                                              margen=desviacion - float(fila["D_cota"] or 0)))
    return len(elegidas), fallas, informe


def comprobar_invalidas(binario):
    fallas = []
    invalidas = leer_csv(os.path.join(CARPETA_INSTANCIAS, "invalidas.csv"))
    with tempfile.TemporaryDirectory() as carpeta_temporal:
        archivo_parametros = os.path.join(carpeta_temporal, "parametros_ssa.csv")
        escribir_csv(archivo_parametros, ["clave", "valor"], list(PARAMETROS_SSA.items()) + [("repeticiones", 1)])
        for fila in invalidas:
            codigo, error = ejecutar_ssa_basico(binario, os.path.join(CARPETA_INSTANCIAS, fila["instancia"]),
                                                archivo_parametros, os.path.join(carpeta_temporal, "salida.csv"))
            if codigo != 1 or fila["error_esperado"] not in error:
                fallas.append(f"{fila['instancia']}: código {codigo}, mensaje «{error}»")
    return len(invalidas), fallas


def mostrar(fallas, limite=10):
    for falla in fallas[:limite]:
        print("    ", falla)
    if len(fallas) > limite:
        print(f"     … y {len(fallas) - limite} más")


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    argumentos = argparse.ArgumentParser()
    argumentos.add_argument("--binario", help="ruta al programa ssa_basico compilado")
    argumentos.add_argument("--repeticiones", type=int, default=30)
    argumentos.add_argument("--exacto", action="store_true", help="volver a resolver los óptimos exactos")
    opciones = argumentos.parse_args()
    binario = opciones.binario or buscar_ssa_basico()
    todo_bien = True

    carpetas, distintos = comprobar_reproducibilidad()
    print(f"1. Reproducibilidad: {carpetas} carpetas regeneradas; archivos distintos: {len(distintos)}")
    mostrar(distintos)
    todo_bien &= not distintos

    manifiesto = leer_csv(os.path.join(CARPETA_INSTANCIAS, "manifiesto.csv"))
    total, fallas = comprobar_generadoras(manifiesto)
    print(f"2. Generadoras: {total - len(fallas)} de {total} admisibles con D = 0 y Z = 0,5")
    mostrar(fallas)
    todo_bien &= not fallas

    if opciones.exacto:
        print("   Resolviendo de nuevo los óptimos exactos…")
        exacto.resolver_todas()
    referencias = leer_csv(os.path.join(CARPETA_INSTANCIAS, "referencias.csv"))
    fallas, comparadas, diferencia_maxima = comprobar_exactos(referencias)
    de_fallo = [fila for fila in referencias if fila["tipo"] == "fallo"]
    print(f"3. Óptimos exactos: {len(referencias) - len(fallas)} de {len(referencias)} correctos "
          f"(Z* ≤ 0,5 en las de acierto; D* ≥ D_cota en las {len(de_fallo)} de fallo)")
    if comparadas:
        print(f"   Frente a los {comparadas} óptimos anteriores de exp_ y rob_: diferencia máxima {diferencia_maxima:.1e}")
    mostrar(fallas)
    todo_bien &= not fallas

    total, fallas, informe = comprobar_algoritmo(binario, referencias, opciones.repeticiones)
    print(f"4. ssa_basico en {total} instancias, {opciones.repeticiones} repeticiones cada una: "
          f"{'ninguna' if not fallas else len(fallas)} ejecución por debajo de su cota")
    mostrar(fallas)
    todo_bien &= not fallas
    aciertos = informe["acierto"]
    if aciertos:
        con_acierto = sum(e["Z"] <= APTITUD_GENERADORA + 1e-12 for e in aciertos)
        print(f"   Deben acertar: {con_acierto} de {len(aciertos)} ejecuciones con Z ≤ 0,5 "
              f"({100 * con_acierto / len(aciertos):.1f} %); brecha media a Z*: "
              f"{100 * statistics.mean(e['brecha'] for e in aciertos):.1f} %")
    for caso in sorted({e["caso"] for e in informe["fallo"]}):
        del_caso = [e for e in informe["fallo"] if e["caso"] == caso]
        print(f"   Deben fallar, {caso}: D mínima {min(e['D'] for e in del_caso):.4f}, margen mínimo sobre "
              f"D_cota {min(e['margen'] for e in del_caso):.4f}; brecha media a Z* "
              f"{100 * statistics.mean(e['brecha'] for e in del_caso):.1f} %")

    total, fallas = comprobar_invalidas(binario)
    print(f"5. Entradas inválidas: {total - len(fallas)} de {total} rechazadas con el mensaje esperado")
    mostrar(fallas)
    todo_bien &= not fallas

    print("Todo se cumple." if todo_bien else "HAY FALLAS.")
    return 0 if todo_bien else 1


if __name__ == "__main__":
    sys.exit(main())
