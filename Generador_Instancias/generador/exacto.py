import os
import sys
from concurrent.futures import ProcessPoolExecutor

os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")
import numpy as np
from scipy.optimize import Bounds, LinearConstraint, milp
from scipy.sparse import lil_matrix

from construccion import CARPETA_INSTANCIAS, escribir_csv, leer_csv
from evaluar import Instancia, evaluar, leer_solucion

LIMITE_SEGUNDOS = 300.0
PROCESOS = min(6, os.cpu_count() or 1)
COLUMNAS_REFERENCIAS = ["instancia", "conjunto", "tipo", "caso", "Z_generadora", "Z_exacto", "Z_cota_inferior",
                        "D_exacto", "D_cota", "estado"]


def resolver(carpeta, limite_segundos=LIMITE_SEGUNDOS):
    inst = Instancia(carpeta)
    num_especies, num_fertilizantes, num_etapas = len(inst.simbolos), inst.num_fertilizantes, inst.etapas
    gamma = inst.peso_desviacion
    cmin, cmax, cref = inst.concentracion_minima, inst.concentracion_maxima, inst.concentracion_referencia
    agua, aporte = inst.agua, inst.aporte

    num_dosis = num_fertilizantes * num_etapas

    def indice_dosis(f, s):
        return f * num_etapas + s

    def indice_empleo(f, s):
        return num_dosis + f * num_etapas + s

    def indice_seleccion(f):
        return 2 * num_dosis + f

    def indice_holgura_inferior(e, s):
        return 2 * num_dosis + num_fertilizantes + e * num_etapas + s

    def indice_holgura_superior(e, s):
        return 2 * num_dosis + num_fertilizantes + num_especies * num_etapas + e * num_etapas + s

    num_variables = 2 * num_dosis + num_fertilizantes + 2 * num_especies * num_etapas

    costo_unitario = np.zeros(num_variables)
    for f in range(num_fertilizantes):
        for s in range(num_etapas):
            costo_unitario[indice_dosis(f, s)] = (1 - gamma) * inst.precio[f] / 1000.0 / inst.costo_referencia
    for e in range(num_especies):
        for s in range(num_etapas):
            costo_unitario[indice_holgura_inferior(e, s)] = gamma / cref[e, s]
            costo_unitario[indice_holgura_superior(e, s)] = gamma / cref[e, s]

    num_filas = 2 * num_especies * num_etapas + num_etapas + 3 * num_dosis + 1
    matriz = lil_matrix((num_filas, num_variables))
    minimo_fila, maximo_fila = [], []
    fila = 0
    for s in range(num_etapas):
        for e in range(num_especies):
            for f in range(num_fertilizantes):
                if aporte[e, f] > 0:
                    matriz[fila, indice_dosis(f, s)] = aporte[e, f]
                    matriz[fila + 1, indice_dosis(f, s)] = aporte[e, f]
            matriz[fila, indice_holgura_inferior(e, s)] = 1.0
            minimo_fila.append(cmin[e, s] - agua[e])
            maximo_fila.append(np.inf)
            matriz[fila + 1, indice_holgura_superior(e, s)] = -1.0
            minimo_fila.append(-np.inf)
            maximo_fila.append(cmax[e, s] - agua[e])
            fila += 2
        for f in range(num_fertilizantes):
            matriz[fila, indice_dosis(f, s)] = inst.cationes_por_gramo[f]
        minimo_fila.append(-np.inf)
        maximo_fila.append(inst.limite_cationes - inst.carga_cationes_agua)
        fila += 1

    desviacion_sin_dosis = inst.desviacion_sin_dosis()
    cota_dosis = np.zeros((num_fertilizantes, num_etapas))
    for f in range(num_fertilizantes):
        for s in range(num_etapas):
            cotas = [(cmax[e, s] + desviacion_sin_dosis * cref[e, s] - agua[e]) / aporte[e, f]
                     for e in range(num_especies) if aporte[e, f] > 0]
            cota_dosis[f, s] = min([inst.dosis_maxima[f]] + [max(c, 0.0) for c in cotas])
    for f in range(num_fertilizantes):
        for s in range(num_etapas):
            matriz[fila, indice_dosis(f, s)] = 1.0  # q − ε·z ≥ 0
            matriz[fila, indice_empleo(f, s)] = -inst.dosis_minima[f]
            minimo_fila.append(0.0)
            maximo_fila.append(np.inf)
            fila += 1
            matriz[fila, indice_dosis(f, s)] = 1.0  # q − cota·z ≤ 0
            matriz[fila, indice_empleo(f, s)] = -cota_dosis[f, s]
            minimo_fila.append(-np.inf)
            maximo_fila.append(0.0)
            fila += 1
            matriz[fila, indice_empleo(f, s)] = 1.0  # z − y ≤ 0
            matriz[fila, indice_seleccion(f)] = -1.0
            minimo_fila.append(-np.inf)
            maximo_fila.append(0.0)
            fila += 1
    for f in range(num_fertilizantes):  # Σ y ≤ k
        matriz[fila, indice_seleccion(f)] = 1.0
    minimo_fila.append(-np.inf)
    maximo_fila.append(inst.maximo_fertilizantes)
    fila += 1

    binarias = slice(num_dosis, 2 * num_dosis + num_fertilizantes)
    es_entera = np.zeros(num_variables)
    es_entera[binarias] = 1
    maximo_variable = np.full(num_variables, np.inf)
    maximo_variable[binarias] = 1
    restricciones = LinearConstraint(matriz.tocsr()[:fila], minimo_fila, maximo_fila)
    resultado = milp(costo_unitario, constraints=restricciones, integrality=es_entera,
                     bounds=Bounds(np.zeros(num_variables), maximo_variable),
                     options=dict(time_limit=limite_segundos, disp=False, mip_rel_gap=1e-6))
    salida = dict(estado=int(resultado.status), mensaje=resultado.message)
    if resultado.x is None:
        return salida
    cota_dual = getattr(resultado, "mip_dual_bound", None)
    cota_inferior = float(cota_dual) if cota_dual is not None else np.nan

    binarias_fijas = np.round(resultado.x[binarias])
    minimo_variable = np.zeros(num_variables)
    minimo_variable[binarias] = binarias_fijas
    maximo_pulido = maximo_variable.copy()
    maximo_pulido[binarias] = binarias_fijas
    pulido = milp(costo_unitario, constraints=restricciones, bounds=Bounds(minimo_variable, maximo_pulido),
                  options=dict(time_limit=limite_segundos, disp=False))
    if pulido.x is None:
        raise RuntimeError(f"{carpeta}: el pulido de la solución exacta no es factible")
    dosis_gramos = pulido.x[:num_dosis].reshape(num_fertilizantes, num_etapas).copy()
    dosis_gramos[dosis_gramos < 1e-9] = 0.0
    escribir_csv(os.path.join(carpeta, "exacta.csv"), ["f", "s", "q"],
                 [[f, s, repr(float(dosis_gramos[f, s]))]
                  for f in range(num_fertilizantes) for s in range(num_etapas) if dosis_gramos[f, s] > 0])
    evaluacion = evaluar(inst, dosis_gramos)
    if np.isnan(cota_inferior):
        cota_inferior = float(pulido.fun)
    salida.update(Z=float(pulido.fun), Z_cota_inferior=min(cota_inferior, float(pulido.fun)),
                  D=evaluacion["D"], costo=evaluacion["costo"], seleccionados=evaluacion["seleccionados"])
    return salida


def referencia_de_instancia(fila_manifiesto, carpeta_instancias=CARPETA_INSTANCIAS):
    carpeta = os.path.join(carpeta_instancias, fila_manifiesto["instancia"])
    resultado = resolver(carpeta)
    z_generadora = ""
    if fila_manifiesto["tipo"] == "acierto":
        instancia = Instancia(carpeta)
        z_generadora = evaluar(instancia, leer_solucion(os.path.join(carpeta, "generadora.csv"), instancia))["Z"]
    return [fila_manifiesto["instancia"], fila_manifiesto["conjunto"], fila_manifiesto["tipo"],
            fila_manifiesto["caso"], z_generadora, resultado.get("Z", ""), resultado.get("Z_cota_inferior", ""),
            resultado.get("D", ""), fila_manifiesto["D_cota"], resultado["estado"]]


def resolver_todas(carpeta_instancias=CARPETA_INSTANCIAS):
    manifiesto = leer_csv(os.path.join(carpeta_instancias, "manifiesto.csv"))
    with ProcessPoolExecutor(max_workers=PROCESOS) as ejecutor:
        filas = list(ejecutor.map(referencia_de_instancia, manifiesto, [carpeta_instancias] * len(manifiesto)))
    escribir_csv(os.path.join(carpeta_instancias, "referencias.csv"), COLUMNAS_REFERENCIAS, filas)
    return filas


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    if len(sys.argv) > 1:
        print(resolver(sys.argv[1]))
        return
    filas = resolver_todas()
    no_optimas = [fila[0] for fila in filas if fila[-1] != 0]
    print(f"{len(filas)} instancias resueltas; sin óptimo demostrado: {len(no_optimas)} {no_optimas[:5]}")


if __name__ == "__main__":
    main()
