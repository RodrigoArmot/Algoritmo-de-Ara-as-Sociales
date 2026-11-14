"""Generador de las instancias de prueba (resultado 7, capítulo 7).

Crea, en la carpeta instancias/, todos los conjuntos:

  Instancias que deben acertar, construidas hacia adelante (este archivo):
    cal_  calibración       |F| × k × ε, 2 repeticiones          54   semilla maestra 2026
    exp_  experimentación   |F| × k × ε × τ, 3 repeticiones     243   semilla maestra 2027
    rob_  robustez          agua × V, 3 repeticiones             12   semilla maestra 2028
  Instancias que deben fallar y entradas inválidas (fallos.py):
    fal_  fallo con cota conocida de la desviación               25   semilla maestra 2029
    inv_  entradas que ssa_basico debe rechazar                   5

Cada instancia que debe acertar parte de una formulación que ya cumple las condiciones operativas, la
generadora, y abre los intervalos de concentración alrededor de lo que esa formulación produce. Así es
satisfacible por construcción: la generadora da D = 0, K = 1 y Z = 0,5, y el óptimo cumple Z* ≤ 0,5.

Uso (en PyCharm, ejecutar este archivo; o desde la carpeta del proyecto):
    .venv/Scripts/python generador/generar.py [carpeta_salida]
Por omisión escribe en instancias/. Con la misma versión de NumPy y SciPy (requirements.txt), la salida
es idéntica byte a byte; verificar.py lo comprueba.
"""
import itertools
import os
import sys

from construccion import (CARPETA_INSTANCIAS, CONDUCTIVIDAD_MAXIMA, NIVEL_CENTRAL, InstanciaBase,
                          InstanciaConstruida, escribir_csv, escribir_instancia, semilla_desde_texto)
import fallos

# Factores y niveles (capítulo 7, Tabla 17)
TAMANOS_CATALOGO = [20, 35, 54]  # |F|
MAXIMOS_FERTILIZANTES = [13, 16, 20]  # k
DOSIS_MINIMAS = [0.01, 0.05, 0.25]  # ε, gramos
TOLERANCIAS = [0.02, 0.05, 0.10]  # τ
TIPOS_AGUA = ["destilada", "cihnm"]
VOLUMENES = [200.0, 1000.0]  # V, litros

SEMILLA_MAESTRA = {"calibracion": 2026, "experimentacion": 2027, "robustez": 2028}
PREFIJO = {"calibracion": "cal", "experimentacion": "exp", "robustez": "rob"}

COLUMNAS_MANIFIESTO = ["instancia", "conjunto", "tipo", "caso", "F", "k", "eps", "tau", "agua", "V", "Emax",
                       "rep", "semilla", "etapa_base", "intentos", "n_generadora", "Kref", "generadora",
                       "D_cota"]


def disenos():
    """Las 309 combinaciones de niveles, en el orden en que se generan."""
    c = NIVEL_CENTRAL
    for tamano, maximo, dosis in itertools.product(TAMANOS_CATALOGO, MAXIMOS_FERTILIZANTES, DOSIS_MINIMAS):
        for repeticion in (1, 2):
            yield ("calibracion", tamano, maximo, dosis, c["tolerancia"], c["tipo_agua"], c["volumen"],
                   repeticion)
    for tamano, maximo, dosis, tolerancia in itertools.product(TAMANOS_CATALOGO, MAXIMOS_FERTILIZANTES,
                                                                DOSIS_MINIMAS, TOLERANCIAS):
        for repeticion in (1, 2, 3):
            yield "experimentacion", tamano, maximo, dosis, tolerancia, c["tipo_agua"], c["volumen"], repeticion
    for tipo_agua, volumen in itertools.product(TIPOS_AGUA, VOLUMENES):
        for repeticion in (1, 2, 3):
            yield ("robustez", c["tamano_catalogo"], c["maximo_fertilizantes"], c["dosis_minima"],
                   c["tolerancia"], tipo_agua, volumen, repeticion)


def generar_instancia_de_acierto(base, carpeta_salida, conjunto, tamano_catalogo, maximo_fertilizantes,
                                 dosis_minima, tolerancia, tipo_agua, volumen, repeticion):
    nombre = (f"{PREFIJO[conjunto]}_F{tamano_catalogo}_k{maximo_fertilizantes}_e{dosis_minima}"
              f"_t{tolerancia}_{tipo_agua}_V{int(volumen)}_r{repeticion}")
    # El texto de la semilla conserva las abreviaturas del generador original (F, k, eps, tau, V, rep),
    # para que las instancias sean las mismas.
    semilla = semilla_desde_texto(
        f"{SEMILLA_MAESTRA[conjunto]}|F={tamano_catalogo}|k={maximo_fertilizantes}|eps={dosis_minima}"
        f"|tau={tolerancia}|agua={tipo_agua}|V={volumen}|rep={repeticion}")
    construida = InstanciaConstruida(base, semilla, maximo_fertilizantes, dosis_minima, tipo_agua, volumen,
                                     repeticion, nombre)
    generadora = construida.fertilizantes_generadora
    catalogo = construida.completar_catalogo(tamano_catalogo, conservar=generadora)
    escribir_instancia(os.path.join(carpeta_salida, nombre), base, catalogo, construida.concentraciones,
                       tolerancia, construida.agua, volumen, maximo_fertilizantes, dosis_minima,
                       construida.costo_referencia, generadora=(generadora, construida.dosis_gramos))
    return dict(instancia=nombre, conjunto=conjunto, tipo="acierto", caso="", F=tamano_catalogo,
                k=maximo_fertilizantes, eps=dosis_minima, tau=tolerancia, agua=tipo_agua, V=volumen,
                Emax=CONDUCTIVIDAD_MAXIMA, rep=repeticion, semilla=semilla, etapa_base=construida.etapa_base + 1,
                intentos=construida.intentos, n_generadora=len(generadora), Kref=construida.costo_referencia,
                generadora=";".join(base.fertilizantes[f]["nombre"] for f in generadora), D_cota="")


def generar_todo(carpeta_salida=CARPETA_INSTANCIAS):
    base = InstanciaBase()
    os.makedirs(carpeta_salida, exist_ok=True)
    filas = [generar_instancia_de_acierto(base, carpeta_salida, *diseno) for diseno in disenos()]
    filas += fallos.generar_instancias_de_fallo(base, carpeta_salida)
    escribir_csv(os.path.join(carpeta_salida, "manifiesto.csv"), COLUMNAS_MANIFIESTO,
                 [[fila[columna] for columna in COLUMNAS_MANIFIESTO] for fila in filas])
    invalidas = fallos.generar_entradas_invalidas(carpeta_salida)
    return filas, invalidas


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    carpeta_salida = sys.argv[1] if len(sys.argv) > 1 else CARPETA_INSTANCIAS
    filas, invalidas = generar_todo(carpeta_salida)
    por_tipo = {}
    for fila in filas:
        por_tipo[fila["tipo"]] = por_tipo.get(fila["tipo"], 0) + 1
    print(f"{por_tipo.get('acierto', 0)} instancias que deben acertar, {por_tipo.get('fallo', 0)} que deben "
          f"fallar y {len(invalidas)} entradas inválidas en {carpeta_salida}")


if __name__ == "__main__":
    main()
