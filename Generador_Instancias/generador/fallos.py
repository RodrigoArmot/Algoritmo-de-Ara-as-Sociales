import csv
import itertools
import math
import os
import shutil

import numpy as np

from construccion import (CARPETA_BASE, CONDUCTIVIDAD_MAXIMA, NIVEL_CENTRAL, InstanciaConstruida, escribir_csv,
                          escribir_instancia, semilla_desde_texto)
from evaluar import Instancia

SEMILLA_MAESTRA_FALLOS = 2029
CASOS_DE_FALLO = ["sinMo", "k4", "eps1", "emax10"]
TAMANOS_CATALOGO = [20, 35, 54]
REPETICIONES = (1, 2)
MAXIMO_FERTILIZANTES_REDUCIDO = 4
DOSIS_MINIMA_ALTA = 1.0
CONDUCTIVIDAD_REDUCIDA = 1.0
COTA_MINIMA_DE_FALLO = 1e-6
COMBINACIONES_MAXIMAS = 2_000_000

ENTRADAS_INVALIDAS = [
    ("inv_limite_invertido", "límite inferior mayor que el superior en Nnit"),
    ("inv_fertilizante_sin_aporte", "Inerte no aporta ninguna especie de la instancia"),
    ("inv_agua_excesiva", "el agua de partida agota por sí sola el límite de conductividad"),
    ("inv_falta_Kref", "falta la clave 'Kref'"),
    ("inv_celda_vacia", "no es un número"),
]


# --- Cotas inferiores de la desviación ---

def partes_de_la_desviacion_del_agua(instancia):
    agua = instancia.agua[:, None]
    defecto = np.maximum(0.0, instancia.concentracion_minima - agua) / instancia.concentracion_referencia
    exceso = np.maximum(0.0, agua - instancia.concentracion_maxima) / instancia.concentracion_referencia
    return defecto, exceso


def cota_por_especies_sin_fuente(instancia):
    defecto, exceso = partes_de_la_desviacion_del_agua(instancia)
    tiene_fuente = (instancia.aporte > 0).any(axis=1)
    return float(exceso.sum() + defecto[~tiene_fuente].sum())


def cota_por_cardinalidad(instancia):
    defecto, exceso = partes_de_la_desviacion_del_agua(instancia)
    defecto_por_especie = defecto.sum(axis=1)
    tiene_fuente = (instancia.aporte > 0).any(axis=1)
    piso = float(exceso.sum() + defecto_por_especie[~tiene_fuente].sum())
    por_cubrir = [e for e in range(len(instancia.simbolos)) if defecto_por_especie[e] > 0 and tiene_fuente[e]]
    maximo = instancia.maximo_fertilizantes
    if maximo >= len(por_cubrir):
        return piso
    patrones = sorted({sum(1 << i for i, e in enumerate(por_cubrir) if instancia.aporte[e, f] > 0)
                       for f in range(instancia.num_fertilizantes)})
    tamano_grupo = min(maximo, len(patrones))
    if math.comb(len(patrones), tamano_grupo) > COMBINACIONES_MAXIMAS:
        return piso
    peso = [defecto_por_especie[e] for e in por_cubrir]
    menor_sin_cubrir = math.inf
    for grupo in itertools.combinations(patrones, tamano_grupo):
        cubiertas = 0
        for patron in grupo:
            cubiertas |= patron
        sin_cubrir = sum(peso[i] for i in range(len(por_cubrir)) if not cubiertas >> i & 1)
        menor_sin_cubrir = min(menor_sin_cubrir, sin_cubrir)
    return piso + float(menor_sin_cubrir)


def cota_por_dosis_minima(instancia):
    defecto, exceso = partes_de_la_desviacion_del_agua(instancia)
    total = 0.0
    for especie in range(len(instancia.simbolos)):
        fuentes = [f for f in range(instancia.num_fertilizantes) if instancia.aporte[especie, f] > 0]
        for etapa in range(instancia.etapas):
            sin_fuente = defecto[especie, etapa] + exceso[especie, etapa]
            con_fuente = [max(0.0, instancia.agua[especie] + instancia.aporte[especie, f] * instancia.dosis_minima[f]
                              - instancia.concentracion_maxima[especie, etapa])
                          / instancia.concentracion_referencia[especie, etapa] for f in fuentes]
            total += min([sin_fuente] + con_fuente)
    return float(total)


def cota_por_conductividad(instancia):
    defecto, exceso = partes_de_la_desviacion_del_agua(instancia)
    cota = float(exceso.sum())
    disponible = instancia.limite_cationes - instancia.carga_cationes_agua
    cationes = [e for e in range(len(instancia.simbolos)) if instancia.cationes_por_mg[e] > 0]
    for etapa in range(instancia.etapas):
        faltante = {e: max(0.0, instancia.concentracion_minima[e, etapa] - instancia.agua[e]) for e in cationes}
        sobrante = sum(instancia.cationes_por_mg[e] * faltante[e] for e in cationes) - disponible
        orden = sorted(cationes, key=lambda e: -instancia.cationes_por_mg[e]
                       * instancia.concentracion_referencia[e, etapa])
        for e in orden:
            if sobrante <= 0:
                break
            liberado = min(sobrante, instancia.cationes_por_mg[e] * faltante[e])
            cota += liberado / (instancia.cationes_por_mg[e] * instancia.concentracion_referencia[e, etapa])
            sobrante -= liberado
    return float(cota)


def cotas_de_desviacion(instancia):
    return dict(sin_fuente=cota_por_especies_sin_fuente(instancia),
                cardinalidad=cota_por_cardinalidad(instancia),
                dosis_minima=cota_por_dosis_minima(instancia),
                conductividad=cota_por_conductividad(instancia))


def cota_de_desviacion(instancia):
    return max(cotas_de_desviacion(instancia).values())


# --- Instancias que deben fallar ---

def generar_instancia_de_fallo(base, carpeta_salida, caso, tamano_catalogo, repeticion):
    c = NIVEL_CENTRAL
    fuentes_molibdeno = base.indices_de_fuentes("Mo")
    if caso == "sinMo":
        tamano_catalogo = min(tamano_catalogo, len(base.fertilizantes) - len(fuentes_molibdeno))
    nombre = f"fal_{caso}_F{tamano_catalogo}_r{repeticion}"
    semilla = semilla_desde_texto(f"{SEMILLA_MAESTRA_FALLOS}|caso={caso}|F={tamano_catalogo}|rep={repeticion}")
    construida = InstanciaConstruida(base, semilla, c["maximo_fertilizantes"], c["dosis_minima"], c["tipo_agua"],
                                     c["volumen"], repeticion, nombre)
    generadora = construida.fertilizantes_generadora
    maximo_fertilizantes = c["maximo_fertilizantes"]
    dosis_minima = c["dosis_minima"]
    conductividad_maxima = CONDUCTIVIDAD_MAXIMA
    if caso == "sinMo":
        conservar = [f for f in generadora if f not in fuentes_molibdeno]
        catalogo = construida.completar_catalogo(tamano_catalogo, conservar, excluir=fuentes_molibdeno)
    elif caso == "sinCatalogo":
        catalogo = []
    else:
        catalogo = construida.completar_catalogo(tamano_catalogo, generadora)
    if caso == "k4":
        maximo_fertilizantes = MAXIMO_FERTILIZANTES_REDUCIDO
    elif caso == "eps1":
        dosis_minima = DOSIS_MINIMA_ALTA
    elif caso == "emax10":
        conductividad_maxima = CONDUCTIVIDAD_REDUCIDA

    carpeta = os.path.join(carpeta_salida, nombre)
    escribir_instancia(carpeta, base, catalogo, construida.concentraciones, c["tolerancia"], construida.agua,
                       c["volumen"], maximo_fertilizantes, dosis_minima, construida.costo_referencia,
                       conductividad_maxima=conductividad_maxima)
    cota = cota_de_desviacion(Instancia(carpeta))
    if cota < COTA_MINIMA_DE_FALLO:
        raise RuntimeError(f"{nombre}: la cota de la desviación es {cota}; el caso no garantiza fallar")
    return dict(instancia=nombre, conjunto="verificacion" if caso == "sinCatalogo" else "calibracion",
                tipo="fallo", caso=caso, F=len(catalogo), k=maximo_fertilizantes, eps=dosis_minima,
                tau=c["tolerancia"], agua=c["tipo_agua"], V=c["volumen"], Emax=conductividad_maxima,
                rep=repeticion, semilla=semilla, etapa_base=construida.etapa_base + 1,
                intentos=construida.intentos, n_generadora="", Kref=construida.costo_referencia,
                generadora="", D_cota=cota)


def generar_instancias_de_fallo(base, carpeta_salida):
    filas = [generar_instancia_de_fallo(base, carpeta_salida, caso, tamano, repeticion)
             for caso in CASOS_DE_FALLO for tamano in TAMANOS_CATALOGO for repeticion in REPETICIONES]
    filas.append(generar_instancia_de_fallo(base, carpeta_salida, "sinCatalogo", NIVEL_CENTRAL["tamano_catalogo"], 1))
    return filas


# --- Entradas inválidas ---

def leer_filas(ruta):
    with open(ruta, newline="", encoding="utf-8") as archivo:
        return list(csv.reader(archivo))


def generar_entrada_invalida(carpeta, nombre):
    os.makedirs(carpeta, exist_ok=True)
    for archivo in ("parametros.csv", "especies.csv", "agua.csv", "limites.csv", "fertilizantes.csv"):
        shutil.copyfile(os.path.join(CARPETA_BASE, archivo), os.path.join(carpeta, archivo))
    if nombre == "inv_limite_invertido":
        ruta = os.path.join(carpeta, "limites.csv")
        filas = leer_filas(ruta)
        filas[1] = [filas[1][0], filas[1][1], "120.0", filas[1][3]]  # Nnit, etapa 1: Cmin 120 > Cmax 100
        escribir_csv(ruta, filas[0], filas[1:])
    elif nombre == "inv_fertilizante_sin_aporte":
        ruta = os.path.join(carpeta, "fertilizantes.csv")
        filas = leer_filas(ruta)
        encabezado = filas[0]
        inerte = dict(nombre="Inerte", formula="-", mw="100.0", precio="1.0", epsilon="0.01", delta="110000.0")
        filas.append([inerte.get(columna, "0") for columna in encabezado])
        escribir_csv(ruta, encabezado, filas[1:])
    elif nombre == "inv_agua_excesiva":
        ruta = os.path.join(carpeta, "agua.csv")
        filas = leer_filas(ruta)
        filas = [[especie, "5000.0" if especie == "Na" else valor] for especie, valor in filas]
        escribir_csv(ruta, filas[0], filas[1:])
    elif nombre == "inv_falta_Kref":
        ruta = os.path.join(carpeta, "parametros.csv")
        filas = [fila for fila in leer_filas(ruta) if fila[0] != "Kref"]
        escribir_csv(ruta, filas[0], filas[1:])
    elif nombre == "inv_celda_vacia":
        ruta = os.path.join(carpeta, "parametros.csv")
        filas = [[clave, "" if clave == "V" else valor] for clave, valor in leer_filas(ruta)]
        escribir_csv(ruta, filas[0], filas[1:])


def generar_entradas_invalidas(carpeta_salida):
    for nombre, _ in ENTRADAS_INVALIDAS:
        generar_entrada_invalida(os.path.join(carpeta_salida, nombre), nombre)
    escribir_csv(os.path.join(carpeta_salida, "invalidas.csv"), ["instancia", "error_esperado"], ENTRADAS_INVALIDAS)
    return ENTRADAS_INVALIDAS
