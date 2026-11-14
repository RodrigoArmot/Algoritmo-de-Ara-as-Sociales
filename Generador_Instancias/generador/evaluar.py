"""Evaluación de una formulación con la función objetivo del capítulo 4, como la calcula ssa_basico.

Reproduce en Python lo que hacen Instancia.cpp y FuncionObjetivo.cpp de SSA_Basico:
    concentración  C = C0 + Σ_f α·q
    desviación     D = Σ_(e,s) [máx(0, Cmin − C) + máx(0, C − Cmax)] / Cref
    costo relativo K = costo / Kref
    aptitud        Z = γ·D + (1 − γ)·K
y comprueba que la formulación sea admisible: dosis nula o entre ε y δ, a lo más k fertilizantes
empleados y la suma equivalente de cationes dentro del límite en cada etapa.

Uso: .venv/Scripts/python generador/evaluar.py <carpeta_instancia> <solucion.csv>
La solución es un CSV con columnas f, s, q (posición en el catálogo, etapa desde 0 y gramos).
"""
import os
import sys

import numpy as np

from construccion import leer_csv, limite_cationes

TOLERANCIA_RELATIVA = 1e-12


class Instancia:
    """Una instancia leída de su carpeta, con los datos derivados que usa la función objetivo."""

    def __init__(self, carpeta):
        self.carpeta = carpeta
        self.nombre = os.path.basename(os.path.normpath(carpeta))
        parametros = {fila["clave"]: fila["valor"] for fila in leer_csv(os.path.join(carpeta, "parametros.csv"))}
        self.etapas = int(parametros["etapas"])
        self.volumen = float(parametros["V"])
        self.conductividad_maxima = float(parametros["Emax"])
        self.maximo_fertilizantes = int(parametros["k"])
        self.peso_desviacion = float(parametros["gamma"])
        self.costo_referencia = float(parametros["Kref"])

        especies = leer_csv(os.path.join(carpeta, "especies.csv"))
        self.simbolos = [especie["especie"] for especie in especies]
        self.peso_atomico = np.array([float(especie["peso_atomico"]) for especie in especies])
        self.carga_cation = np.array([float(especie["carga_E_mas"]) for especie in especies])
        agua = {fila["especie"]: float(fila["C0"]) for fila in leer_csv(os.path.join(carpeta, "agua.csv"))}
        self.agua = np.array([agua[simbolo] for simbolo in self.simbolos])

        num_especies = len(self.simbolos)
        self.concentracion_minima = np.zeros((num_especies, self.etapas))
        self.concentracion_maxima = np.zeros((num_especies, self.etapas))
        for fila in leer_csv(os.path.join(carpeta, "limites.csv")):
            especie, etapa = self.simbolos.index(fila["especie"]), int(fila["etapa"]) - 1
            self.concentracion_minima[especie, etapa] = float(fila["Cmin"])
            self.concentracion_maxima[especie, etapa] = float(fila["Cmax"])
        # (Cref) El punto medio del intervalo, o la mitad de la cota máxima si la especie no tiene requerimiento.
        self.concentracion_referencia = np.where(
            self.concentracion_minima > 0, (self.concentracion_minima + self.concentracion_maxima) / 2.0,
            self.concentracion_maxima / 2.0)

        fertilizantes = leer_csv(os.path.join(carpeta, "fertilizantes.csv"))
        self.nombres_fertilizantes = [fertilizante["nombre"] for fertilizante in fertilizantes]
        self.precio = np.array([float(f["precio"]) for f in fertilizantes])  # USD/kg
        self.dosis_minima = np.array([float(f["epsilon"]) for f in fertilizantes])  # (ε) g
        self.dosis_maxima = np.array([float(f["delta"]) for f in fertilizantes])  # (δ) g
        peso_molecular = np.array([float(f["mw"]) for f in fertilizantes])
        atomos = np.array([[float(f[simbolo]) for simbolo in self.simbolos] for f in fertilizantes])
        atomos = atomos.reshape(len(fertilizantes), num_especies)
        # (α) aporte[e, f] en mg/L por gramo, con el orden de operaciones de Instancia.cpp
        self.aporte = 1000.0 * self.peso_atomico[:, None] * atomos.T / (peso_molecular[None, :] * self.volumen)

        cationes_por_mg = np.where(self.carga_cation > 0, self.carga_cation / self.peso_atomico, 0.0)
        self.cationes_por_mg = cationes_por_mg
        self.carga_cationes_agua = float(np.sum(self.agua * cationes_por_mg))  # (u) meq/L
        self.cationes_por_gramo = (self.aporte * cationes_por_mg[:, None]).sum(axis=0)
        self.limite_cationes = limite_cationes(self.conductividad_maxima)  # (L) meq/L

    @property
    def num_fertilizantes(self):
        return len(self.nombres_fertilizantes)

    def desviacion_por_especie(self, concentraciones):
        """Aporte de cada especie y etapa a D, con las concentraciones dadas (forma especies × etapas)."""
        defecto = np.maximum(0.0, self.concentracion_minima - concentraciones)
        exceso = np.maximum(0.0, concentraciones - self.concentracion_maxima)
        return (defecto + exceso) / self.concentracion_referencia

    def desviacion_sin_dosis(self):
        """(D(0)) Desviación con la dosificación nula: la concentración es la del agua de partida."""
        concentraciones = np.repeat(self.agua[:, None], self.etapas, axis=1)
        return float(self.desviacion_por_especie(concentraciones).sum())


def leer_solucion(ruta, instancia):
    """Matriz de dosis en gramos, fertilizantes × etapas, a partir de un CSV f, s, q."""
    dosis_gramos = np.zeros((instancia.num_fertilizantes, instancia.etapas))
    for fila in leer_csv(ruta):
        dosis_gramos[int(fila["f"]), int(fila["s"])] = float(fila["q"])
    return dosis_gramos


def evaluar(instancia, dosis_gramos):
    """Aptitud y admisibilidad de una formulación. Devuelve un diccionario con Z, D, K, costo,
    seleccionados, admisible y, si no lo es, el motivo."""
    concentraciones = instancia.agua[:, None] + instancia.aporte @ dosis_gramos
    desviacion = float(instancia.desviacion_por_especie(concentraciones).sum())
    costo = float(np.sum(dosis_gramos / 1000.0 * instancia.precio[:, None]))
    costo_relativo = costo / instancia.costo_referencia
    aptitud = instancia.peso_desviacion * desviacion + (1.0 - instancia.peso_desviacion) * costo_relativo
    seleccionados = int(np.sum(dosis_gramos.sum(axis=1) > 0))

    motivos = []
    if np.any(dosis_gramos < 0):
        motivos.append("dosis negativa")
    positivas = dosis_gramos > 0
    bajo_minimo = positivas & (dosis_gramos < instancia.dosis_minima[:, None] * (1 - TOLERANCIA_RELATIVA))
    if np.any(bajo_minimo):
        motivos.append("dosis entre 0 y ε")
    if np.any(dosis_gramos > instancia.dosis_maxima[:, None]):
        motivos.append("dosis por encima de δ")
    if seleccionados > instancia.maximo_fertilizantes:
        motivos.append(f"{seleccionados} fertilizantes, más que k = {instancia.maximo_fertilizantes}")
    for etapa in range(instancia.etapas):
        carga = instancia.carga_cationes_agua + float(instancia.cationes_por_gramo @ dosis_gramos[:, etapa])
        if carga > instancia.limite_cationes * (1 + TOLERANCIA_RELATIVA):
            motivos.append(f"conductividad excedida en la etapa {etapa + 1}")
    return dict(Z=aptitud, D=desviacion, K=costo_relativo, costo=costo, seleccionados=seleccionados,
                admisible=not motivos, motivo="; ".join(motivos))


def main():
    instancia = Instancia(sys.argv[1])
    resultado = evaluar(instancia, leer_solucion(sys.argv[2], instancia))
    for clave, valor in resultado.items():
        print(f"{clave}: {valor}")


if __name__ == "__main__":
    main()
