"""Comparación de los dos algoritmos (resultado 10, capítulo 7).

Corre 30 repeticiones de cada algoritmo, con su configuración calibrada (resultados/calibracion/*.json) y
30 000 evaluaciones de la función objetivo, sobre las 243 instancias de experimentación y las 12 de
robustez. Corre también el recocido sin activación de dosis sobre las de experimentación, para medir el
efecto de la activación.

Uso (en PyCharm, ejecutar este archivo; o desde la carpeta del proyecto):
    .venv/Scripts/python experimentacion/comparar.py [--ssa-basico ruta] [--formulacion ruta]
Escribe resultados/comparacion/<rama>/<instancia>.csv y configuraciones.json; después, analizar.py.
"""
import argparse
import json
import os
import sys

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(CARPETA_PROYECTO, "generador"))

from construccion import CARPETA_INSTANCIAS, leer_csv  # noqa: E402
from ejecucion import Ejecutor, preparar_carpeta, programas  # noqa: E402

CARPETA_RESULTADOS = os.path.join(CARPETA_PROYECTO, "resultados", "comparacion")
CARPETA_CALIBRACION = os.path.join(CARPETA_PROYECTO, "resultados", "calibracion")
REPETICIONES = 30


def configuracion_calibrada(algoritmo):
    ruta = os.path.join(CARPETA_CALIBRACION, f"{algoritmo}.json")
    if not os.path.isfile(ruta):
        sys.exit(f"Falta {ruta}: ejecute antes calibracion/calibrar.py {algoritmo}")
    with open(ruta, encoding="utf-8") as archivo:
        return json.load(archivo)["calibrada"]


def ramas():
    """Cada rama: (algoritmo, configuración, conjuntos de instancias en que corre)."""
    aranas = configuracion_calibrada("aranas")
    recocido = configuracion_calibrada("recocido")
    return {"aranas": ("aranas", aranas, ("experimentacion", "robustez")),
            "recocido": ("recocido", recocido, ("experimentacion", "robustez")),
            "recocido_sin_activar": ("recocido", dict(recocido, activar=False), ("experimentacion",))}


def instancias_de(conjuntos):
    return [fila["instancia"] for fila in leer_csv(os.path.join(CARPETA_INSTANCIAS, "manifiesto.csv"))
            if fila["conjunto"] in conjuntos]


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    argumentos = argparse.ArgumentParser()
    argumentos.add_argument("--ssa-basico", help="ruta al programa ssa_basico compilado")
    argumentos.add_argument("--formulacion", help="ruta al programa formulacion del proyecto completo")
    opciones = argumentos.parse_args()
    rutas = programas(opciones.ssa_basico, opciones.formulacion)
    for algoritmo, ruta in rutas.items():
        if not ruta:
            sys.exit(f"No se encontró el programa de {algoritmo}; indíquelo con --ssa-basico o --formulacion")
    trabajos, usadas = [], {}
    for rama, (algoritmo, configuracion, conjuntos) in ramas().items():
        carpeta = os.path.join(CARPETA_RESULTADOS, rama)
        preparar_carpeta(algoritmo, carpeta, configuracion, REPETICIONES)
        nombres = instancias_de(conjuntos)
        trabajos += [(algoritmo, nombre, configuracion, REPETICIONES, os.path.join(carpeta, nombre + ".csv"))
                     for nombre in nombres]
        usadas[rama] = dict(algoritmo=algoritmo, configuracion=configuracion, instancias=len(nombres),
                            repeticiones=REPETICIONES)
    with open(os.path.join(CARPETA_RESULTADOS, "configuraciones.json"), "w", encoding="utf-8") as archivo:
        json.dump(usadas, archivo, indent=1, ensure_ascii=False)
    Ejecutor(rutas).ejecutar_todos(trabajos)
    print(f"Comparación terminada: {len(trabajos)} archivos de {REPETICIONES} repeticiones en {CARPETA_RESULTADOS}")


if __name__ == "__main__":
    main()
