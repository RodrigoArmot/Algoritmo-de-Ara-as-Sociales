import argparse
import json
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(CARPETA_PROYECTO, "generador"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from construccion import CARPETA_INSTANCIAS, leer_csv
from ejecucion import Ejecutor, preparar_carpeta, programas
from comparar import CARPETA_RESULTADOS, configuracion_calibrada
import exacto

REPETICIONES = 30


def nombre_del_procesador():
    if platform.system() == "Windows":
        proceso = subprocess.run(["powershell", "-NoProfile", "-Command",
                                  "(Get-CimInstance Win32_Processor).Name"], capture_output=True, text=True)
        if proceso.returncode == 0 and proceso.stdout.strip():
            return proceso.stdout.strip()
    return platform.processor()


def medir_algoritmo(ejecutor, algoritmo, configuracion, instancias, carpeta_temporal):
    carpeta = os.path.join(carpeta_temporal, algoritmo)
    preparar_carpeta(algoritmo, carpeta, configuracion, REPETICIONES)
    tiempos = []
    for instancia in instancias:
        salida = os.path.join(carpeta, instancia + ".csv")
        orden = ejecutor.orden(algoritmo, instancia, configuracion, REPETICIONES, salida)
        inicio = time.perf_counter()
        subprocess.run(orden, check=True, capture_output=True, cwd=carpeta)
        tiempos.append((time.perf_counter() - inicio) / REPETICIONES)
    return statistics.median(tiempos)


def medir_exacto(instancias, carpeta_temporal):
    tiempos = []
    for instancia in instancias:
        copia = os.path.join(carpeta_temporal, "exacto", instancia)
        shutil.copytree(os.path.join(CARPETA_INSTANCIAS, instancia), copia)
        inicio = time.perf_counter()
        exacto.resolver(copia)
        tiempos.append(time.perf_counter() - inicio)
    return statistics.median(tiempos), max(tiempos)


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    argumentos = argparse.ArgumentParser()
    argumentos.add_argument("--ssa-basico", help="ruta al programa ssa_basico compilado")
    argumentos.add_argument("--formulacion", help="ruta al programa formulacion del proyecto completo")
    opciones = argumentos.parse_args()
    rutas = programas(opciones.ssa_basico, opciones.formulacion)
    ejecutor = Ejecutor(rutas)
    manifiesto = leer_csv(os.path.join(CARPETA_INSTANCIAS, "manifiesto.csv"))
    muestra = [fila["instancia"] for fila in manifiesto
               if fila["conjunto"] == "experimentacion" and fila["tau"] == "0.05" and fila["rep"] == "1"]
    exp_y_rob = [fila["instancia"] for fila in manifiesto if fila["conjunto"] in ("experimentacion", "robustez")]
    with tempfile.TemporaryDirectory() as carpeta_temporal:
        aranas = medir_algoritmo(ejecutor, "aranas", configuracion_calibrada("aranas"), muestra, carpeta_temporal)
        recocido = medir_algoritmo(ejecutor, "recocido", configuracion_calibrada("recocido"), muestra,
                                   carpeta_temporal)
        exacto_mediana, exacto_maximo = medir_exacto(exp_y_rob, carpeta_temporal)
    informe = dict(procesador=nombre_del_procesador(), sistema=platform.platform(), instancias_algoritmos=len(muestra),
                   repeticiones=REPETICIONES, aranas_s_por_ejecucion=aranas, recocido_s_por_ejecucion=recocido,
                   instancias_exacto=len(exp_y_rob), exacto_mediana_s=exacto_mediana, exacto_maximo_s=exacto_maximo)
    os.makedirs(CARPETA_RESULTADOS, exist_ok=True)
    with open(os.path.join(CARPETA_RESULTADOS, "tiempos.json"), "w", encoding="utf-8") as archivo:
        json.dump(informe, archivo, indent=1, ensure_ascii=False)
    print(json.dumps(informe, indent=1, ensure_ascii=False))


if __name__ == "__main__":
    main()
