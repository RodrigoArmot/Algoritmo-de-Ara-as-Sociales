import itertools
import json
import os
import sys
import warnings

import numpy as np
from scipy.stats import friedmanchisquare, mannwhitneyu, rankdata, shapiro, wilcoxon

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(CARPETA_PROYECTO, "generador"))

from construccion import CARPETA_INSTANCIAS, escribir_csv, leer_csv
from ejecucion import COLUMNA_APTITUD, COLUMNA_COSTO, COLUMNA_DESVIACION

CARPETA_RESULTADOS = os.path.join(CARPETA_PROYECTO, "resultados", "comparacion")
ALFA = 0.05
APTITUD_GENERADORA = 0.5
UMBRAL_OPTIMO = 1e-3
UMBRALES_OPTIMO_ALTERNATIVOS = (1e-4, 1e-2)
ALGORITMO_DE_RAMA = {"aranas": "aranas", "recocido": "recocido", "recocido_sin_activar": "recocido"}
CONTRASTES = [  # (nombre, rama A, rama B, conjunto, factores del diseño)
    ("experimentacion", "aranas", "recocido", "experimentacion", ("F", "k", "eps", "tau")),
    ("robustez", "aranas", "recocido", "robustez", ("agua", "V")),
    ("activacion", "recocido", "recocido_sin_activar", "experimentacion", ()),
]
RESPUESTAS_POR_FACTOR = ("brecha_relativa_A", "brecha_relativa_B", "A12")


# --- Lectura ---

def leer_ejecuciones(rama, instancia):
    algoritmo = ALGORITMO_DE_RAMA[rama]
    filas = leer_csv(os.path.join(CARPETA_RESULTADOS, rama, instancia + ".csv"))
    return tuple(np.array([float(fila[columna[algoritmo]]) for fila in filas])
                 for columna in (COLUMNA_APTITUD, COLUMNA_DESVIACION, COLUMNA_COSTO))


# --- Estadísticos ---

def correccion_de_holm(valores_p):
    valores_p = np.asarray(valores_p, dtype=float)
    cantidad = len(valores_p)
    ajustados = np.empty(cantidad)
    maximo = 0.0
    for posicion, indice in enumerate(np.argsort(valores_p)):
        maximo = max(maximo, (cantidad - posicion) * valores_p[indice])
        ajustados[indice] = min(1.0, maximo)
    return ajustados


def efecto_a12(aptitudes_a, aptitudes_b):
    a = np.asarray(aptitudes_a)[:, None]
    b = np.asarray(aptitudes_b)[None, :]
    return float(np.mean(a < b) + 0.5 * np.mean(a == b))


def biserial_de_rangos(valores_a, valores_b):
    diferencias = np.asarray(valores_a) - np.asarray(valores_b)
    diferencias = diferencias[diferencias != 0]
    if len(diferencias) == 0:
        return 0.0
    rangos = rankdata(np.abs(diferencias))
    positivos, negativos = rangos[diferencias > 0].sum(), rangos[diferencias < 0].sum()
    return float((positivos - negativos) / (positivos + negativos))


def wilcoxon_pareado(valores_a, valores_b):
    if np.all(np.asarray(valores_a) == np.asarray(valores_b)):
        return dict(estadistico=0.0, p=1.0, efecto=0.0)
    prueba = wilcoxon(valores_a, valores_b)
    return dict(estadistico=float(prueba.statistic), p=float(prueba.pvalue),
                efecto=biserial_de_rangos(valores_a, valores_b))


def cuartiles(valores):
    q1, mediana, q3 = np.percentile(valores, [25, 50, 75])
    return float(q1), float(mediana), float(q3)


# --- Comparación por instancia ---

def comparar_por_instancia(rama_a, rama_b, instancias, manifiesto, referencias):
    filas = []
    for instancia in instancias:
        z_a, d_a, costo_a = leer_ejecuciones(rama_a, instancia)
        z_b, d_b, costo_b = leer_ejecuciones(rama_b, instancia)
        todas_iguales = np.all(z_a == z_a[0]) and np.all(z_b == z_b[0]) and z_a[0] == z_b[0]
        valor_p = 1.0 if todas_iguales else float(mannwhitneyu(z_a, z_b, alternative="two-sided").pvalue)
        z_exacto = float(referencias[instancia]["Z_exacto"])
        datos = manifiesto[instancia]
        fila = dict(
            instancia=instancia, F=int(datos["F"]), k=int(datos["k"]), eps=float(datos["eps"]),
            tau=float(datos["tau"]), agua=datos["agua"], V=float(datos["V"]), rep=int(datos["rep"]),
            Z_exacto=z_exacto, mediana_Z_A=float(np.median(z_a)), mediana_Z_B=float(np.median(z_b)),
            mejor_Z_A=float(z_a.min()), mejor_Z_B=float(z_b.min()),
            acierto_A=float(np.mean(z_a <= APTITUD_GENERADORA + 1e-12)),
            acierto_B=float(np.mean(z_b <= APTITUD_GENERADORA + 1e-12)),
            brecha_A=float(np.median(z_a)) - z_exacto, brecha_B=float(np.median(z_b)) - z_exacto,
            brecha_relativa_A=(float(np.median(z_a)) - z_exacto) / z_exacto,
            brecha_relativa_B=(float(np.median(z_b)) - z_exacto) / z_exacto,
            optimo_A=float(np.mean(z_a - z_exacto <= UMBRAL_OPTIMO)),
            optimo_B=float(np.mean(z_b - z_exacto <= UMBRAL_OPTIMO)))
        for umbral in UMBRALES_OPTIMO_ALTERNATIVOS:
            fila[f"optimo_{umbral:g}_A"] = float(np.mean(z_a - z_exacto <= umbral))
            fila[f"optimo_{umbral:g}_B"] = float(np.mean(z_b - z_exacto <= umbral))
        fila.update(mediana_D_A=float(np.median(d_a)), mediana_D_B=float(np.median(d_b)),
                    mediana_costo_A=float(np.median(costo_a)), mediana_costo_B=float(np.median(costo_b)),
                    p=valor_p, A12=efecto_a12(z_a, z_b))
        filas.append(fila)
    for fila, ajustado in zip(filas, correccion_de_holm([fila["p"] for fila in filas])):
        fila["p_holm"] = float(ajustado)
        if ajustado < ALFA:
            fila["resultado"] = "A" if fila["mediana_Z_A"] < fila["mediana_Z_B"] else "B"
        else:
            fila["resultado"] = "empate"
    return filas


def resumir(filas):
    def columna(nombre):
        return np.array([fila[nombre] for fila in filas])

    resumen = dict(instancias=len(filas))
    for nombre in ("mediana_Z_A", "mediana_Z_B", "Z_exacto", "brecha_A", "brecha_B", "brecha_relativa_A",
                   "brecha_relativa_B", "mediana_D_A", "mediana_D_B", "mediana_costo_A", "mediana_costo_B", "A12"):
        q1, mediana, q3 = cuartiles(columna(nombre))
        resumen[f"{nombre}_mediana"], resumen[f"{nombre}_q1"], resumen[f"{nombre}_q3"] = mediana, q1, q3
    for nombre in ("acierto_A", "acierto_B", "optimo_A", "optimo_B") + tuple(
            f"optimo_{u:g}_{lado}" for u in UMBRALES_OPTIMO_ALTERNATIVOS for lado in "AB"):
        resumen[nombre] = float(columna(nombre).mean())
    resumen.update(gana_A=sum(fila["resultado"] == "A" for fila in filas),
                   gana_B=sum(fila["resultado"] == "B" for fila in filas),
                   empates=sum(fila["resultado"] == "empate" for fila in filas))
    if len(filas) > 1:
        prueba = wilcoxon_pareado(columna("mediana_Z_A"), columna("mediana_Z_B"))
        resumen.update(wilcoxon_estadistico=prueba["estadistico"], wilcoxon_p=prueba["p"],
                       wilcoxon_biserial=prueba["efecto"])
    return resumen


def resumir_por_factor(filas, factores):
    tabla = []
    for factor in factores:
        for nivel in sorted({fila[factor] for fila in filas}):
            resumen = resumir([fila for fila in filas if fila[factor] == nivel])
            tabla.append(dict(factor=factor, nivel=nivel, **{clave: resumen[clave] for clave in (
                "instancias", "brecha_A_mediana", "brecha_B_mediana", "brecha_relativa_A_mediana",
                "brecha_relativa_A_q1", "brecha_relativa_A_q3", "brecha_relativa_B_mediana",
                "brecha_relativa_B_q1", "brecha_relativa_B_q3", "acierto_A", "acierto_B", "gana_A", "gana_B",
                "empates", "A12_mediana")}))
    return tabla


# --- Efecto de los factores ---

def bloques_de(filas, factor, factores):
    otros = [f for f in factores if f != factor]
    niveles = sorted({fila[factor] for fila in filas})
    bloques = {}
    for fila in filas:
        bloques.setdefault(tuple(fila[f] for f in otros) + (fila["rep"],), {})[fila[factor]] = fila
    completos = [bloque for bloque in bloques.values() if len(bloque) == len(niveles)]
    return niveles, completos


def efecto_de_factores(contraste, filas, factores):
    resultados = []
    for factor in factores:
        niveles, bloques = bloques_de(filas, factor, factores)
        for respuesta in RESPUESTAS_POR_FACTOR:
            por_nivel = [np.array([bloque[nivel][respuesta] for bloque in bloques]) for nivel in niveles]
            base = dict(contraste=contraste, factor=factor, respuesta=respuesta, bloques=len(bloques),
                        niveles=" / ".join(f"{nivel:g}" if isinstance(nivel, float) else str(nivel) for nivel in niveles),
                        medianas=" / ".join(f"{np.median(valores):.6g}" for valores in por_nivel))
            if len(niveles) >= 3:
                prueba = friedmanchisquare(*por_nivel)
                w_kendall = float(prueba.statistic) / (len(bloques) * (len(niveles) - 1))
                pares = list(itertools.combinations(range(len(niveles)), 2))
                pruebas_pares = [wilcoxon_pareado(por_nivel[i], por_nivel[j]) for i, j in pares]
                ajustados = correccion_de_holm([prueba_par["p"] for prueba_par in pruebas_pares])
                resultados.append(dict(base, prueba="Friedman", estadistico=float(prueba.statistic),
                                       p=float(prueba.pvalue), efecto="W de Kendall", valor_efecto=w_kendall,
                                       pares="; ".join(f"{niveles[i]:g}-{niveles[j]:g}: p_holm={q:.3g}"
                                                       for (i, j), q in zip(pares, ajustados))))
            else:
                prueba = wilcoxon_pareado(por_nivel[0], por_nivel[1])
                resultados.append(dict(base, prueba="Wilcoxon pareada", estadistico=prueba["estadistico"],
                                       p=prueba["p"], efecto="biserial de rangos", valor_efecto=prueba["efecto"],
                                       pares=""))
    return resultados


# --- Normalidad ---

def probar_normalidad(instancias_por_rama):
    filas = []
    for rama, instancias in instancias_por_rama.items():
        for instancia in instancias:
            aptitudes = leer_ejecuciones(rama, instancia)[0]
            if np.all(aptitudes == aptitudes[0]):
                filas.append(dict(rama=rama, instancia=instancia, W="", p="", rechaza="constante"))
                continue
            with warnings.catch_warnings():
                warnings.simplefilter("ignore")
                prueba = shapiro(aptitudes)
            filas.append(dict(rama=rama, instancia=instancia, W=float(prueba.statistic), p=float(prueba.pvalue),
                              rechaza="si" if prueba.pvalue < ALFA else "no"))
    evaluables = [fila for fila in filas if fila["rechaza"] != "constante"]
    ajustados = correccion_de_holm([fila["p"] for fila in evaluables])
    for fila, ajustado in zip(evaluables, ajustados):
        fila["p_holm"] = float(ajustado)
        fila["rechaza_holm"] = "si" if ajustado < ALFA else "no"
    resumen = {}
    for rama in list(instancias_por_rama) + ["total"]:
        de_rama = filas if rama == "total" else [fila for fila in filas if fila["rama"] == rama]
        resumen[rama] = dict(muestras=len(de_rama), rechazan=sum(f["rechaza"] == "si" for f in de_rama),
                             rechazan_con_holm=sum(f.get("rechaza_holm") == "si" for f in de_rama),
                             constantes=sum(f["rechaza"] == "constante" for f in de_rama),
                             mediana_W=float(np.median([f["W"] for f in de_rama if f["W"] != ""])))
    return filas, resumen


# --- Método exacto ---

def resumir_exacto(referencias, instancias):
    """Brecha del óptimo de HiGHS a su cota inferior (prueba de optimalidad) en las instancias dadas."""
    brechas = [(float(referencias[i]["Z_exacto"]) - float(referencias[i]["Z_cota_inferior"]))
               / float(referencias[i]["Z_exacto"]) for i in instancias]
    return dict(instancias=len(instancias), optimos_demostrados=sum(referencias[i]["estado"] == "0" for i in instancias),
                brecha_relativa_a_la_cota_maxima=float(max(brechas)),
                brecha_relativa_a_la_cota_mediana=float(np.median(brechas)))


# --- Salida ---

def escribir_filas(nombre, filas):
    columnas = list(filas[0].keys())
    for fila in filas:
        for columna in fila:
            if columna not in columnas:
                columnas.append(columna)
    escribir_csv(os.path.join(CARPETA_RESULTADOS, nombre), columnas,
                 [[fila.get(columna, "") for columna in columnas] for fila in filas])


def tabla_de_pruebas(informe):
    filas = []
    for nombre, contraste in informe["contrastes"].items():
        resumen = contraste["resumen"]
        filas.append(dict(contraste=nombre, comparacion=f"{contraste['A']} frente a {contraste['B']}",
                          prueba="Wilcoxon de rangos con signo (medianas por instancia)",
                          n=resumen["instancias"], estadistico=resumen.get("wilcoxon_estadistico"),
                          p=resumen.get("wilcoxon_p"), efecto="biserial de rangos",
                          valor_efecto=resumen.get("wilcoxon_biserial")))
        filas.append(dict(contraste=nombre, comparacion=f"{contraste['A']} frente a {contraste['B']}",
                          prueba="Mann-Whitney por instancia, Holm, α = 0,05", n=resumen["instancias"],
                          estadistico="", p="", efecto="A12 mediano", valor_efecto=resumen["A12_mediana"],
                          detalle=f"gana A en {resumen['gana_A']}, gana B en {resumen['gana_B']}, "
                                  f"sin diferencia en {resumen['empates']}"))
    for fila in informe["efecto_de_factores"]:
        filas.append(dict(contraste=fila["contraste"], comparacion=f"{fila['factor']} sobre {fila['respuesta']}",
                          prueba=fila["prueba"], n=fila["bloques"], estadistico=fila["estadistico"], p=fila["p"],
                          efecto=fila["efecto"], valor_efecto=fila["valor_efecto"], detalle=fila["pares"]))
    normalidad = informe["normalidad"]["total"]
    filas.append(dict(contraste="experimentacion y robustez", comparacion="normalidad de las 30 aptitudes",
                      prueba="Shapiro-Wilk, α = 0,05", n=normalidad["muestras"], estadistico="", p="",
                      efecto="", valor_efecto="",
                      detalle=f"rechazan {normalidad['rechazan']}; con Holm, {normalidad['rechazan_con_holm']}"))
    return filas


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    manifiesto = {fila["instancia"]: fila for fila in leer_csv(os.path.join(CARPETA_INSTANCIAS, "manifiesto.csv"))}
    referencias = {fila["instancia"]: fila for fila in leer_csv(os.path.join(CARPETA_INSTANCIAS, "referencias.csv"))}
    por_conjunto = {}
    for nombre, datos in manifiesto.items():
        por_conjunto.setdefault(datos["conjunto"], []).append(nombre)

    informe = dict(alfa=ALFA, umbral_optimo=UMBRAL_OPTIMO, contrastes={}, efecto_de_factores=[])
    for nombre, rama_a, rama_b, conjunto, factores in CONTRASTES:
        filas = comparar_por_instancia(rama_a, rama_b, por_conjunto[conjunto], manifiesto, referencias)
        escribir_filas(f"{nombre}_por_instancia.csv", filas)
        por_factor = resumir_por_factor(filas, factores)
        if por_factor:
            escribir_filas(f"{nombre}_por_factor.csv", por_factor)
        informe["contrastes"][nombre] = dict(A=rama_a, B=rama_b, resumen=resumir(filas), por_factor=por_factor)
        if nombre != "activacion":
            informe["efecto_de_factores"] += efecto_de_factores(nombre, filas, factores)
    escribir_filas("efecto_de_factores.csv", informe["efecto_de_factores"])

    exp_y_rob = por_conjunto["experimentacion"] + por_conjunto["robustez"]
    filas_normalidad, informe["normalidad"] = probar_normalidad({"aranas": exp_y_rob, "recocido": exp_y_rob})
    escribir_filas("normalidad_por_muestra.csv", filas_normalidad)
    informe["exacto"] = resumir_exacto(referencias, exp_y_rob)
    escribir_filas("pruebas_estadisticas.csv", tabla_de_pruebas(informe))
    with open(os.path.join(CARPETA_RESULTADOS, "resumen.json"), "w", encoding="utf-8") as archivo:
        json.dump(informe, archivo, indent=1, ensure_ascii=False)

    for nombre, contraste in informe["contrastes"].items():
        r = contraste["resumen"]
        print(f"{nombre} (A = {contraste['A']}, B = {contraste['B']}): Z {r['mediana_Z_A_mediana']:.3f} frente a "
              f"{r['mediana_Z_B_mediana']:.3f}; gana A {r['gana_A']}, B {r['gana_B']}, empates {r['empates']}; "
              f"Wilcoxon p = {r.get('wilcoxon_p', float('nan')):.3g}, biserial {r.get('wilcoxon_biserial', float('nan')):.3f}; "
              f"A12 {r['A12_mediana']:.3f}")
    for fila in informe["efecto_de_factores"]:
        print(f"  {fila['contraste']:15s} {fila['factor']:4s} {fila['respuesta']:18s} {fila['prueba']:16s} "
              f"p = {fila['p']:.3g}  {fila['efecto']} = {fila['valor_efecto']:.3f}  medianas {fila['medianas']}  {fila['pares']}")
    print("normalidad:", json.dumps(informe["normalidad"], ensure_ascii=False))
    print("exacto:", json.dumps(informe["exacto"], ensure_ascii=False))


if __name__ == "__main__":
    main()
