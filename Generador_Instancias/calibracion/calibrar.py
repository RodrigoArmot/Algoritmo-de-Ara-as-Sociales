"""Calibración de los parámetros de control (resultado 9, capítulo 7): un parámetro a la vez.

Instancias: las 78 del conjunto de calibración del manifiesto, 54 que deben acertar (cal_) y 24 que deben
fallar (fal_).

Respuesta: la brecha media al óptimo exacto, (Z − Z*)/Z*, con Z* de instancias/referencias.csv. La
aptitud media ya no sirve, porque el Z* de las instancias de fallo es varias veces el de las de acierto
y dominaría el promedio; la brecha pone a todas en la misma escala.

Procedimiento, el mismo de la calibración anterior: cada configuración se ejecuta dos veces por instancia
con 30 000 evaluaciones de la función objetivo; se varía un parámetro a la vez desde la configuración
vigente y se elige el valor de menor brecha media; ante un empate se conserva el vigente. Las pasadas se
repiten hasta que una completa no cambia ningún valor. Las semillas dependen solo de la instancia y de
la repetición, así que todas las configuraciones se miden con los mismos números aleatorios.

Programas:
  arañas    ssa_basico (SSA_Basico), con iteraciones = 30 000 / población
  recocido  formulacion recocido (proyecto completo), con I + 1 temperaturas de n candidatas

Uso (en PyCharm, ejecutar con parámetros; o desde la carpeta del proyecto):
    .venv/Scripts/python calibracion/calibrar.py aranas   [--ssa-basico ruta]
    .venv/Scripts/python calibracion/calibrar.py recocido [--formulacion ruta]
    .venv/Scripts/python calibracion/calibrar.py confirmar [--ssa-basico ruta] [--formulacion ruta]
Escribe resultados/calibracion/<algoritmo>.json y un CSV por configuración e instancia. Si se
interrumpe, al volver a ejecutarlo retoma lo que falta.
"""
import argparse
import json
import os
import statistics
import sys

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(CARPETA_PROYECTO, "generador"))

from construccion import CARPETA_INSTANCIAS, leer_csv  # noqa: E402
from ejecucion import (COLUMNA_APTITUD, COLUMNA_DESVIACION, Ejecutor, nombre_de_configuracion,  # noqa: E402
                       preparar_carpeta, programas)

CARPETA_RESULTADOS = os.path.join(CARPETA_PROYECTO, "resultados", "calibracion")
REPETICIONES_CALIBRACION = 2
REPETICIONES_CONFIRMACION = 4
PASADAS_MAXIMAS = 4
EMPATE = 1e-9
APTITUD_GENERADORA = 0.5

CONFIGURACION_DE_PARTIDA = {
    # r_a, p_c y p_m de Yu y Li (2015a); la población de 30 es propia y fija el presupuesto.
    "aranas": dict(poblacion=30, ra=1.0, pc=0.7, pm=0.1),
    # La configuración más cercana a la Tabla 5 de Guerra Ibarra et al. (2026), con activación de dosis.
    "recocido": dict(vecino="todos", aceptacion="mejoras", perturbacion="fija", delta="pareja", activar=True,
                     p=0.95, n=500, Ev=0.15, tf=1e-5),
}
VALORES_A_PROBAR = {
    "aranas": [("poblacion", [10, 20, 30, 40, 50, 75, 100]),
               ("ra", [0.01, 0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0, 1000.0]),
               ("pc", [0.01, 0.05, 0.1, 0.3, 0.5, 0.7, 0.9, 0.99]),
               ("pm", [0.01, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5])],
    "recocido": [("n", [50, 100, 200, 500, 1000, 2000, 5000]),
                 ("Ev", [0.05, 0.1, 0.15, 0.25, 0.4, 0.55, 0.75, 1.0]),
                 ("p", [0.5, 0.7, 0.8, 0.9, 0.95, 0.99]),
                 ("tf", [1e-7, 1e-5, 1e-3, 1e-1, 1.0])],
}
# --- Medida de la respuesta ---

class Referencias:
    """Z* y D_cota de las instancias de calibración, de instancias/referencias.csv."""

    def __init__(self):
        filas = leer_csv(os.path.join(CARPETA_INSTANCIAS, "referencias.csv"))
        self.calibracion = [fila for fila in filas if fila["conjunto"] == "calibracion"]
        self.nombres = [fila["instancia"] for fila in self.calibracion]
        self.por_nombre = {fila["instancia"]: fila for fila in self.calibracion}


def medir(algoritmo, carpeta_configuracion, referencias):
    """Brecha media a Z* (y su error estándar), y para el informe: tasa de acierto en las instancias que
    deben acertar y si alguna de fallo bajó de su cota de desviación."""
    brechas_por_instancia, aciertos, ejecuciones_de_acierto, bajo_cota = [], 0, 0, 0
    for nombre in referencias.nombres:
        fila = referencias.por_nombre[nombre]
        z_exacto = float(fila["Z_exacto"])
        ejecuciones = leer_csv(os.path.join(carpeta_configuracion, nombre + ".csv"))
        aptitudes = [float(e[COLUMNA_APTITUD[algoritmo]]) for e in ejecuciones]
        brechas_por_instancia.append(statistics.mean((z - z_exacto) / z_exacto for z in aptitudes))
        if fila["tipo"] == "acierto":
            ejecuciones_de_acierto += len(aptitudes)
            aciertos += sum(z <= APTITUD_GENERADORA + 1e-12 for z in aptitudes)
        else:
            d_cota = float(fila["D_cota"])
            bajo_cota += sum(float(e[COLUMNA_DESVIACION[algoritmo]]) < d_cota - 1e-9 for e in ejecuciones)
    return dict(brecha=statistics.mean(brechas_por_instancia),
                error_estandar=statistics.stdev(brechas_por_instancia) / len(brechas_por_instancia) ** 0.5,
                tasa_acierto=aciertos / ejecuciones_de_acierto, ejecuciones_bajo_cota=bajo_cota,
                brecha_por_instancia=brechas_por_instancia)


def carpeta_de(algoritmo, configuracion, subcarpeta=""):
    return os.path.join(CARPETA_RESULTADOS, algoritmo, subcarpeta, nombre_de_configuracion(configuracion))


def evaluar_configuraciones(ejecutor, algoritmo, configuraciones, referencias, repeticiones, subcarpeta=""):
    for configuracion in configuraciones:
        preparar_carpeta(algoritmo, carpeta_de(algoritmo, configuracion, subcarpeta), configuracion, repeticiones)
    trabajos = [(algoritmo, nombre, configuracion, repeticiones,
                 os.path.join(carpeta_de(algoritmo, configuracion, subcarpeta), nombre + ".csv"))
                for configuracion in configuraciones for nombre in referencias.nombres]
    ejecutor.ejecutar_todos(trabajos)
    return [medir(algoritmo, carpeta_de(algoritmo, configuracion, subcarpeta), referencias)
            for configuracion in configuraciones]


# --- Calibración y confirmación ---

def calibrar(ejecutor, algoritmo, referencias):
    vigente = dict(CONFIGURACION_DE_PARTIDA[algoritmo])
    informe = dict(algoritmo=algoritmo, instancias=len(referencias.nombres), respuesta="brecha media a Z*",
                   partida=dict(vigente), pasadas=[])
    for pasada in range(1, PASADAS_MAXIMAS + 1):
        cambios, barridos = 0, []
        for parametro, valores in VALORES_A_PROBAR[algoritmo]:
            medidas = evaluar_configuraciones(ejecutor, algoritmo, [dict(vigente, **{parametro: v}) for v in valores],
                                              referencias, REPETICIONES_CALIBRACION)
            mejor = min(m["brecha"] for m in medidas)
            empatados = [i for i, m in enumerate(medidas) if m["brecha"] - mejor <= EMPATE]
            elegido = next((i for i in empatados if valores[i] == vigente[parametro]), empatados[0])
            barridos.append(dict(parametro=parametro, valores=valores, previo=vigente[parametro], elegido=valores[elegido],
                                 brecha=[m["brecha"] for m in medidas], error_estandar=[m["error_estandar"] for m in medidas],
                                 tasa_acierto=[m["tasa_acierto"] for m in medidas],
                                 ejecuciones_bajo_cota=[m["ejecuciones_bajo_cota"] for m in medidas]))
            print(f"pasada {pasada} {parametro}: "
                  + "  ".join(f"{v}:{100 * m['brecha']:.1f}%" for v, m in zip(valores, medidas))
                  + f"  -> {valores[elegido]}", flush=True)
            cambios += valores[elegido] != vigente[parametro]
            vigente[parametro] = valores[elegido]
        informe["pasadas"].append(dict(pasada=pasada, barridos=barridos, configuracion=dict(vigente)))
        if cambios == 0:
            break
    informe["calibrada"] = dict(vigente)
    os.makedirs(CARPETA_RESULTADOS, exist_ok=True)
    with open(os.path.join(CARPETA_RESULTADOS, f"{algoritmo}.json"), "w", encoding="utf-8") as archivo:
        json.dump(informe, archivo, indent=1, ensure_ascii=False)
    print(f"Configuración calibrada de {algoritmo}: {vigente}")
    return vigente


def confirmar(ejecutor, referencias, algoritmos):
    """Partida frente a calibrada con cuatro repeticiones por instancia y la prueba de rangos con signo de
    Wilcoxon sobre la brecha media de cada instancia."""
    from scipy.stats import wilcoxon
    lineas, cifras = [], {}
    for algoritmo in algoritmos:
        with open(os.path.join(CARPETA_RESULTADOS, f"{algoritmo}.json"), encoding="utf-8") as archivo:
            calibrada = json.load(archivo)["calibrada"]
        partida = CONFIGURACION_DE_PARTIDA[algoritmo]
        de_partida, de_calibrada = evaluar_configuraciones(ejecutor, algoritmo, [partida, calibrada], referencias,
                                                           REPETICIONES_CONFIRMACION, subcarpeta="confirmacion")
        mejora = sum(c < p for c, p in zip(de_calibrada["brecha_por_instancia"], de_partida["brecha_por_instancia"]))
        prueba = wilcoxon(de_calibrada["brecha_por_instancia"], de_partida["brecha_por_instancia"])
        cifras[algoritmo] = dict(partida=partida, calibrada=calibrada, repeticiones=REPETICIONES_CONFIRMACION,
                                 instancias=len(referencias.nombres), brecha_partida=de_partida["brecha"],
                                 brecha_calibrada=de_calibrada["brecha"], instancias_que_mejoran=mejora,
                                 wilcoxon_p=float(prueba.pvalue), acierto_partida=de_partida["tasa_acierto"],
                                 acierto_calibrada=de_calibrada["tasa_acierto"],
                                 bajo_cota=de_partida["ejecuciones_bajo_cota"] + de_calibrada["ejecuciones_bajo_cota"])
        lineas.append(f"{algoritmo}: brecha media de partida {100 * de_partida['brecha']:.1f} %, calibrada "
                      f"{100 * de_calibrada['brecha']:.1f} %; mejora en {mejora} de {len(referencias.nombres)} "
                      f"instancias; Wilcoxon p = {prueba.pvalue:.2g}; acierto {100 * de_partida['tasa_acierto']:.1f} % "
                      f"-> {100 * de_calibrada['tasa_acierto']:.1f} %")
    texto = "\n".join(lineas) + "\n"
    print(texto, end="")
    with open(os.path.join(CARPETA_RESULTADOS, "confirmacion.txt"), "w", encoding="utf-8") as archivo:
        archivo.write(texto)
    with open(os.path.join(CARPETA_RESULTADOS, "confirmacion.json"), "w", encoding="utf-8") as archivo:
        json.dump(cifras, archivo, indent=1, ensure_ascii=False)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    argumentos = argparse.ArgumentParser()
    argumentos.add_argument("tarea", choices=["aranas", "recocido", "confirmar"])
    argumentos.add_argument("--ssa-basico", help="ruta al programa ssa_basico compilado")
    argumentos.add_argument("--formulacion", help="ruta al programa formulacion del proyecto completo")
    opciones = argumentos.parse_args()
    rutas = programas(opciones.ssa_basico, opciones.formulacion)
    necesarios = ["aranas", "recocido"] if opciones.tarea == "confirmar" else [opciones.tarea]
    for algoritmo in necesarios:
        if not rutas[algoritmo]:
            sys.exit(f"No se encontró el programa de {algoritmo}; indíquelo con --ssa-basico o --formulacion")
    ejecutor = Ejecutor(rutas)
    referencias = Referencias()
    if opciones.tarea == "confirmar":
        confirmar(ejecutor, referencias, ["aranas", "recocido"])
    else:
        calibrar(ejecutor, opciones.tarea, referencias)


if __name__ == "__main__":
    main()
