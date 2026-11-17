import copy
import csv
import hashlib
import os

import numpy as np
from scipy.optimize import nnls

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CARPETA_BASE = os.path.join(CARPETA_PROYECTO, "base")
CARPETA_INSTANCIAS = os.path.join(CARPETA_PROYECTO, "instancias")

AGUA_POZO_CIHNM = {"Nnit": 2.80, "Ca": 125.0, "Mg": 24.0, "Na": 27.6, "K": 3.90, "Cl": 106.5, "S": 41.6, "B": 0.30}
COTA_SUPERIOR_SIN_REQUERIMIENTO = {"Na": 460.0, "Cl": 709.0}
CONDUCTIVIDAD_MAXIMA = 2.5
PESO_DESVIACION = 0.5
ETAPAS = 1
ETAPAS_BASE = 3
DOSIS_MAXIMA_POR_LITRO = 110.0
INTENTOS_MAXIMOS = 1000
NIVEL_CENTRAL = dict(tamano_catalogo=35, maximo_fertilizantes=16, dosis_minima=0.05, tolerancia=0.05,
                     tipo_agua="destilada", volumen=1000.0)


def leer_csv(ruta):
    with open(ruta, newline="", encoding="utf-8") as archivo:
        return list(csv.DictReader(archivo))


def escribir_csv(ruta, encabezado, filas):
    with open(ruta, "w", newline="", encoding="utf-8") as archivo:
        escritor = csv.writer(archivo, lineterminator="\n")
        escritor.writerow(encabezado)
        escritor.writerows(filas)


def semilla_desde_texto(texto):
    return int.from_bytes(hashlib.sha256(texto.encode()).digest()[:8], "big")


def limite_cationes(conductividad_maxima):
    return 9.819 * conductividad_maxima - 1.462


class InstanciaBase:

    def __init__(self, carpeta=CARPETA_BASE):
        self.especies = leer_csv(os.path.join(carpeta, "especies.csv"))
        self.simbolos = [especie["especie"] for especie in self.especies]
        self.peso_atomico = np.array([float(especie["peso_atomico"]) for especie in self.especies])
        self.carga_cation = np.array([float(especie["carga_E_mas"]) for especie in self.especies])
        self.fertilizantes = leer_csv(os.path.join(carpeta, "fertilizantes.csv"))
        self.peso_molecular = np.array([float(fertilizante["mw"]) for fertilizante in self.fertilizantes])
        self.precio = np.array([float(fertilizante["precio"]) for fertilizante in self.fertilizantes])
        self.atomos = np.array([[float(fertilizante[simbolo]) for simbolo in self.simbolos]
                                for fertilizante in self.fertilizantes])
        num_especies = len(self.simbolos)
        self.concentracion_minima = np.zeros((num_especies, ETAPAS_BASE))  # (Cmin) mg/L
        self.concentracion_maxima = np.zeros((num_especies, ETAPAS_BASE))  # (Cmax) mg/L
        for fila in leer_csv(os.path.join(carpeta, "limites.csv")):
            especie = self.simbolos.index(fila["especie"])
            etapa = int(fila["etapa"]) - 1
            self.concentracion_minima[especie, etapa] = float(fila["Cmin"])
            self.concentracion_maxima[especie, etapa] = float(fila["Cmax"])
        self.especies_con_requerimiento = [especie for especie in range(num_especies)
                                           if self.concentracion_minima[especie, 0] > 0]
        self.fuentes = {especie: [f for f in range(len(self.fertilizantes)) if self.atomos[f, especie] > 0]
                        for especie in self.especies_con_requerimiento}

    def aporte_unitario(self, volumen):
        return 1000.0 * (self.peso_atomico[:, None] * self.atomos.T) / (self.peso_molecular[None, :] * volumen)

    def en_una_etapa(self, etapa):
        copia = copy.copy(self)
        copia.concentracion_minima = self.concentracion_minima[:, [etapa]]
        copia.concentracion_maxima = self.concentracion_maxima[:, [etapa]]
        return copia

    def agua_de_partida(self, tipo_agua):
        return np.array([AGUA_POZO_CIHNM.get(simbolo, 0.0) if tipo_agua == "cihnm" else 0.0
                         for simbolo in self.simbolos])

    def indices_de_fuentes(self, simbolo):
        especie = self.simbolos.index(simbolo)
        return [f for f in range(len(self.fertilizantes)) if self.atomos[f, especie] > 0]


def intentar_formulacion_generadora(base_etapa, generador_azar, maximo_fertilizantes, dosis_minima,
                                    agua, volumen):
    num_especies = len(base_etapa.simbolos)
    aporte = base_etapa.aporte_unitario(volumen)
    limite = limite_cationes(CONDUCTIVIDAD_MAXIMA)
    cationes_por_mg = base_etapa.carga_cation / base_etapa.peso_atomico
    carga_cationes_agua = float(np.sum(agua * base_etapa.carga_cation / base_etapa.peso_atomico))
    centro_intervalo = (base_etapa.concentracion_minima + base_etapa.concentracion_maxima) / 2.0
    concentraciones_objetivo = (centro_intervalo * generador_azar.uniform(0.8, 1.2, size=(num_especies, 1))
                                * generador_azar.uniform(0.95, 1.05, size=(num_especies, ETAPAS)))
    cantidad_fertilizantes = int(generador_azar.integers(max(9, maximo_fertilizantes - 3),
                                                         maximo_fertilizantes + 1))

    fertilizantes = []
    for especie in generador_azar.permutation(base_etapa.especies_con_requerimiento):
        if not any(base_etapa.atomos[f, especie] > 0 for f in fertilizantes):
            fertilizantes.append(int(generador_azar.choice(base_etapa.fuentes[especie])))
    if len(fertilizantes) > cantidad_fertilizantes:
        return None
    no_elegidos = [f for f in range(len(base_etapa.fertilizantes)) if f not in fertilizantes]
    fertilizantes += [int(f) for f in generador_azar.choice(
        no_elegidos, size=cantidad_fertilizantes - len(fertilizantes), replace=False)]
    fertilizantes = sorted(set(fertilizantes))

    requeridas = base_etapa.especies_con_requerimiento
    dosis_gramos = np.zeros((len(fertilizantes), ETAPAS))
    for etapa in range(ETAPAS):
        matriz_aporte = aporte[np.ix_(requeridas, fertilizantes)]
        faltante = np.clip(concentraciones_objetivo[requeridas, etapa] - agua[requeridas], 0.0, None)
        peso_fila = 1.0 / concentraciones_objetivo[requeridas, etapa]
        dosis_gramos[:, etapa], _ = nnls(matriz_aporte * peso_fila[:, None], faltante * peso_fila)

    con_dosis = dosis_gramos.sum(axis=1) > 0
    fertilizantes = [f for f, usado in zip(fertilizantes, con_dosis) if usado]
    dosis_gramos = dosis_gramos[con_dosis]

    cationes_por_gramo = (aporte[:, fertilizantes] * cationes_por_mg[:, None]).sum(axis=0)
    for etapa in range(ETAPAS):
        carga_fertilizantes = float(cationes_por_gramo @ dosis_gramos[:, etapa])
        if carga_cationes_agua + carga_fertilizantes > limite:
            dosis_gramos[:, etapa] *= 0.98 * (limite - carga_cationes_agua) / carga_fertilizantes

    dosis_gramos[(dosis_gramos > 0) & (dosis_gramos < dosis_minima)] = dosis_minima
    if np.any(dosis_gramos > DOSIS_MAXIMA_POR_LITRO * volumen):
        return None
    for etapa in range(ETAPAS):
        if carga_cationes_agua + float(cationes_por_gramo @ dosis_gramos[:, etapa]) > limite:
            return None
    concentraciones = agua[:, None] + aporte[:, fertilizantes] @ dosis_gramos
    if np.any(concentraciones[requeridas] <= 0):
        return None
    for simbolo, cota in COTA_SUPERIOR_SIN_REQUERIMIENTO.items():
        if np.any(concentraciones[base_etapa.simbolos.index(simbolo)] > cota):
            return None
    if len(fertilizantes) > maximo_fertilizantes:
        return None
    return fertilizantes, dosis_gramos, concentraciones


class InstanciaConstruida:

    def __init__(self, base, semilla, maximo_fertilizantes, dosis_minima, tipo_agua, volumen, repeticion,
                 nombre):
        self.semilla = semilla
        self.generador_azar = np.random.default_rng(semilla)
        self.etapa_base = (repeticion - 1) % ETAPAS_BASE
        self.base_etapa = base.en_una_etapa(self.etapa_base)
        self.agua = base.agua_de_partida(tipo_agua)
        for intentos in range(1, INTENTOS_MAXIMOS + 1):
            resultado = intentar_formulacion_generadora(self.base_etapa, self.generador_azar,
                                                        maximo_fertilizantes, dosis_minima, self.agua, volumen)
            if resultado is not None:
                break
        else:
            raise RuntimeError(f"{nombre}: sin formulación generadora en {INTENTOS_MAXIMOS} intentos")
        self.intentos = intentos
        self.fertilizantes_generadora, self.dosis_gramos, self.concentraciones = resultado
        self.costo_referencia = float(sum(self.dosis_gramos[i, etapa] / 1000.0 * base.precio[f]
                                          for i, f in enumerate(self.fertilizantes_generadora)
                                          for etapa in range(ETAPAS)))

    def completar_catalogo(self, tamano_catalogo, conservar, excluir=()):
        candidatos = [f for f in range(len(self.base_etapa.fertilizantes))
                      if f not in self.fertilizantes_generadora and f not in excluir]
        faltan = tamano_catalogo - len(conservar)
        extra = ([int(f) for f in self.generador_azar.choice(candidatos, size=faltan, replace=False)]
                 if faltan > 0 else [])
        return sorted(conservar + extra)


def escribir_instancia(carpeta, base, catalogo, concentraciones, tolerancia, agua, volumen,
                       maximo_fertilizantes, dosis_minima, costo_referencia,
                       conductividad_maxima=CONDUCTIVIDAD_MAXIMA, generadora=None):
    os.makedirs(carpeta, exist_ok=True)
    escribir_csv(os.path.join(carpeta, "parametros.csv"), ["clave", "valor"], [
        ("etapas", ETAPAS), ("V", repr(volumen)), ("Emax", conductividad_maxima), ("k", maximo_fertilizantes),
        ("gamma", PESO_DESVIACION), ("Kref", repr(costo_referencia)),
        ("restriccion_dosis_minima", 1), ("restriccion_conductividad", 1),
        ("restriccion_cardinalidad", 1), ("normalizar_costo", 1)])
    escribir_csv(os.path.join(carpeta, "especies.csv"), ["especie", "peso_atomico", "carga_E_mas"],
                 [[e["especie"], e["peso_atomico"], e["carga_E_mas"]] for e in base.especies])
    escribir_csv(os.path.join(carpeta, "agua.csv"), ["especie", "C0"],
                 [[simbolo, repr(float(c))] for simbolo, c in zip(base.simbolos, agua)])
    filas_limites = []
    for especie, simbolo in enumerate(base.simbolos):
        for etapa in range(ETAPAS):
            if simbolo in COTA_SUPERIOR_SIN_REQUERIMIENTO:
                minimo, maximo = 0.0, COTA_SUPERIOR_SIN_REQUERIMIENTO[simbolo]
            else:
                minimo = concentraciones[especie, etapa] * (1 - tolerancia)
                maximo = concentraciones[especie, etapa] * (1 + tolerancia)
            filas_limites.append([simbolo, etapa + 1, repr(float(minimo)), repr(float(maximo))])
    escribir_csv(os.path.join(carpeta, "limites.csv"), ["especie", "etapa", "Cmin", "Cmax"], filas_limites)
    columnas = list(base.fertilizantes[0].keys())
    filas_fertilizantes = []
    for f in catalogo:
        fila = dict(base.fertilizantes[f])
        fila["epsilon"] = repr(dosis_minima)
        fila["delta"] = repr(DOSIS_MAXIMA_POR_LITRO * volumen)
        filas_fertilizantes.append([fila[columna] for columna in columnas])
    escribir_csv(os.path.join(carpeta, "fertilizantes.csv"), columnas, filas_fertilizantes)
    if generadora is not None:
        fertilizantes, dosis_gramos = generadora
        posicion_en_catalogo = {f: i for i, f in enumerate(catalogo)}
        escribir_csv(os.path.join(carpeta, "generadora.csv"), ["f", "s", "q"],
                     [[posicion_en_catalogo[f], etapa, repr(float(dosis_gramos[i, etapa]))]
                      for i, f in enumerate(fertilizantes) for etapa in range(ETAPAS)
                      if dosis_gramos[i, etapa] > 0])
