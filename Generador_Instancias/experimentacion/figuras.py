"""Figuras del capítulo 7: calibración de cada algoritmo y brecha relativa al óptimo según el tamaño del
catálogo. Decimales con coma. El valor elegido se marca con un punto de mayor tamaño y borde, de modo que
se distingue también en una impresión en escala de grises.

Uso:  python experimentacion/figuras.py [carpeta_de_salida]
Lee resultados/calibracion/<algoritmo>.json y resultados/comparacion/experimentacion_por_instancia.csv.
Por omisión escribe en resultados/figuras. Requiere Matplotlib.
"""
import csv
import json
import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
from matplotlib.ticker import FuncFormatter

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CARPETA_RESULTADOS = os.path.join(CARPETA_PROYECTO, "resultados")
CON_COMA = FuncFormatter(lambda valor, posicion: ("%g" % valor).replace(".", ","))
ETIQUETA = {"poblacion": "Tamaño de la población", "ra": "Tasa de atenuación $r_a$",
            "pc": "Probabilidad de cambio de máscara $p_c$", "pm": "Tasa de máscara $p_m$",
            "n": "Candidatas por temperatura $n$", "Ev": "Perturbación máxima $E_v$",
            "p": "Probabilidad de aceptación inicial $p$", "tf": "Temperatura final $t_f$"}
ESCALA_LOGARITMICA = {"ra", "tf", "n"}
plt.rcParams["font.size"] = 9
plt.rcParams["font.family"] = ["Arial", "Liberation Sans", "DejaVu Sans"]


def figura_calibracion(algoritmo, carpeta_salida):
    with open(os.path.join(CARPETA_RESULTADOS, "calibracion", f"{algoritmo}.json"), encoding="utf-8") as archivo:
        barridos = json.load(archivo)["pasadas"][-1]["barridos"]
    figura, ejes = plt.subplots(2, 2, figsize=(16 / 2.54, 11 / 2.54))
    for eje, barrido in zip(ejes.flat, barridos):
        valores = [float(valor) for valor in barrido["valores"]]
        brecha = [100 * b for b in barrido["brecha"]]
        eje.plot(valores, brecha, "o-", color="#333333", markersize=3.5, linewidth=1)
        elegido = valores.index(float(barrido["elegido"]))
        eje.plot([valores[elegido]], [brecha[elegido]], "o", color="#c0392b", markeredgecolor="black",
                 markersize=7, zorder=5)
        if barrido["parametro"] in ESCALA_LOGARITMICA:
            eje.set_xscale("log")
        else:
            eje.xaxis.set_major_formatter(CON_COMA)
        if max(brecha) - min(brecha) < 1e-9:  # todos los valores dan la misma brecha
            eje.set_ylim(brecha[0] - 10, brecha[0] + 10)
        eje.yaxis.set_major_formatter(CON_COMA)
        eje.set_xlabel(ETIQUETA[barrido["parametro"]])
        eje.set_ylabel("Brecha relativa media (%)")
        eje.grid(True, linewidth=0.3, alpha=0.6)
    figura.tight_layout()
    ruta = os.path.join(carpeta_salida, f"Figura_Calibracion_{algoritmo}.png")
    figura.savefig(ruta, dpi=300)
    plt.close(figura)
    return ruta


def figura_brecha_por_catalogo(carpeta_salida):
    ruta_datos = os.path.join(CARPETA_RESULTADOS, "comparacion", "experimentacion_por_instancia.csv")
    with open(ruta_datos, newline="", encoding="utf-8") as archivo:
        filas = list(csv.DictReader(archivo))
    tamanos = sorted({int(fila["F"]) for fila in filas})
    figura, eje = plt.subplots(figsize=(16 / 2.54, 8 / 2.54))
    for desplazamiento, columna, color in ((-0.2, "brecha_relativa_A", "#4d4d4d"), (0.2, "brecha_relativa_B", "#d9d9d9")):
        datos = [[100 * float(fila[columna]) for fila in filas if int(fila["F"]) == tamano] for tamano in tamanos]
        eje.boxplot(datos, positions=[i + desplazamiento for i in range(len(tamanos))], widths=0.32,
                    patch_artist=True, boxprops=dict(facecolor=color, linewidth=0.8),
                    medianprops=dict(color="black", linewidth=1.2), whiskerprops=dict(linewidth=0.8),
                    capprops=dict(linewidth=0.8), flierprops=dict(marker="o", markersize=2.5, markeredgewidth=0.6))
    eje.set_yscale("log")
    eje.yaxis.set_major_formatter(CON_COMA)
    eje.set_xticks(range(len(tamanos)))
    eje.set_xticklabels([f"{tamano} fertilizantes" for tamano in tamanos])
    eje.set_ylabel("Brecha relativa al óptimo (%)")
    eje.grid(True, axis="y", linewidth=0.3, alpha=0.6)
    eje.legend(handles=[Patch(facecolor="#4d4d4d", edgecolor="black", label="Algoritmo de Arañas Sociales"),
                        Patch(facecolor="#d9d9d9", edgecolor="black", label="Recocido simulado")],
               frameon=False, fontsize=8, loc="lower right")
    figura.tight_layout()
    ruta = os.path.join(carpeta_salida, "Figura_Brecha_Catalogo.png")
    figura.savefig(ruta, dpi=300)
    plt.close(figura)
    return ruta


def main():
    carpeta_salida = sys.argv[1] if len(sys.argv) > 1 else os.path.join(CARPETA_RESULTADOS, "figuras")
    os.makedirs(carpeta_salida, exist_ok=True)
    for ruta in (figura_calibracion("aranas", carpeta_salida), figura_calibracion("recocido", carpeta_salida),
                 figura_brecha_por_catalogo(carpeta_salida)):
        print(ruta)


if __name__ == "__main__":
    main()
