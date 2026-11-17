import argparse
import json
import os
import statistics
import sys

import numpy as np
from scipy.stats import wilcoxon

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from calibrar import (CARPETA_RESULTADOS, EMPATE, REPETICIONES_CALIBRACION, Referencias, carpeta_de,
                      evaluar_configuraciones, medir)
from ejecucion import Ejecutor, programas

MAS_ALLA_DEL_BORDE = {
    ("aranas", "pc", "inferior"): [0.001, 0.005],
    ("aranas", "pc", "superior"): [0.995, 0.999],
    ("aranas", "pm", "inferior"): [0.001, 0.005],
    ("aranas", "pm", "superior"): [0.7, 0.9],
    ("aranas", "ra", "inferior"): [0.001],
    ("aranas", "ra", "superior"): [5000.0],
    ("aranas", "poblacion", "inferior"): [5],
    ("aranas", "poblacion", "superior"): [150],
    ("recocido", "Ev", "inferior"): [0.01, 0.025],
    ("recocido", "Ev", "superior"): [1.5],
}


def eleccion_por_mediana(algoritmo, referencias):
    with open(os.path.join(CARPETA_RESULTADOS, f"{algoritmo}.json"), encoding="utf-8") as archivo:
        informe = json.load(archivo)
    pasada = informe["pasadas"][-1]
    resultado = []
    for barrido in pasada["barridos"]:
        base = dict(pasada["configuracion"])
        medianas, por_instancia = [], []
        for valor in barrido["valores"]:
            configuracion = dict(base, **{barrido["parametro"]: valor})
            brechas = medir(algoritmo, carpeta_de(algoritmo, configuracion), referencias)["brecha_por_instancia"]
            por_instancia.append(brechas)
            medianas.append(statistics.median(brechas))
        mejor = min(medianas)
        empatados = [i for i, m in enumerate(medianas) if m - mejor <= EMPATE]
        indice = next((i for i in empatados if barrido["valores"][i] == barrido["elegido"]), empatados[0])
        por_mediana = barrido["valores"][indice]
        fila = dict(parametro=barrido["parametro"], elegido_por_media=barrido["elegido"],
                    elegido_por_mediana=por_mediana, coincide=por_mediana == barrido["elegido"],
                    valores=barrido["valores"], brecha_mediana=medianas)
        if not fila["coincide"]:
            del_elegido = por_instancia[barrido["valores"].index(barrido["elegido"])]
            prueba = wilcoxon(por_instancia[indice], del_elegido)
            fila.update(wilcoxon_p=float(prueba.pvalue),
                        brecha_media_de_cada_uno=[statistics.mean(del_elegido), statistics.mean(por_instancia[indice])],
                        brecha_mediana_de_cada_uno=[statistics.median(del_elegido), medianas[indice]])
        resultado.append(fila)
    return informe["calibrada"], resultado


def probar_borde(ejecutor, algoritmo, calibrada, referencias):
    from calibrar import VALORES_A_PROBAR
    resultado = []
    for parametro, valores in VALORES_A_PROBAR[algoritmo]:
        elegido = calibrada[parametro]
        lado = "inferior" if elegido == min(valores) else "superior" if elegido == max(valores) else None
        if lado is None or (algoritmo, parametro, lado) not in MAS_ALLA_DEL_BORDE:
            continue
        nuevos = MAS_ALLA_DEL_BORDE[(algoritmo, parametro, lado)]
        configuraciones = [dict(calibrada, **{parametro: v}) for v in nuevos + [elegido]]
        medidas = evaluar_configuraciones(ejecutor, algoritmo, configuraciones, referencias, REPETICIONES_CALIBRACION)
        del_elegido = medidas[-1]
        for valor, medida in zip(nuevos, medidas[:-1]):
            prueba = wilcoxon(medida["brecha_por_instancia"], del_elegido["brecha_por_instancia"])
            resultado.append(dict(parametro=parametro, borde=lado, valor=valor, elegido=elegido,
                                  brecha=medida["brecha"], error_estandar=medida["error_estandar"],
                                  brecha_del_elegido=del_elegido["brecha"],
                                  instancias_que_mejoran=int(sum(a < b for a, b in zip(
                                      medida["brecha_por_instancia"], del_elegido["brecha_por_instancia"]))),
                                  wilcoxon_p=float(prueba.pvalue), tasa_acierto=medida["tasa_acierto"],
                                  ejecuciones_bajo_cota=medida["ejecuciones_bajo_cota"]))
    return resultado


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    argumentos = argparse.ArgumentParser()
    argumentos.add_argument("--ssa-basico", help="ruta al programa ssa_basico compilado")
    argumentos.add_argument("--formulacion", help="ruta al programa formulacion del proyecto completo")
    opciones = argumentos.parse_args()
    ejecutor = Ejecutor(programas(opciones.ssa_basico, opciones.formulacion))
    referencias = Referencias()
    informe = {}
    for algoritmo in ("aranas", "recocido"):
        calibrada, por_mediana = eleccion_por_mediana(algoritmo, referencias)
        borde = probar_borde(ejecutor, algoritmo, calibrada, referencias)
        informe[algoritmo] = dict(calibrada=calibrada, eleccion_por_mediana=por_mediana, borde_de_la_rejilla=borde)
        for fila in por_mediana:
            detalle = "" if fila["coincide"] else (
                f"  <- distinto; brecha media {100 * fila['brecha_media_de_cada_uno'][0]:.1f} % frente a "
                f"{100 * fila['brecha_media_de_cada_uno'][1]:.1f} %, mediana {100 * fila['brecha_mediana_de_cada_uno'][0]:.1f} % "
                f"frente a {100 * fila['brecha_mediana_de_cada_uno'][1]:.1f} %; Wilcoxon p = {fila['wilcoxon_p']:.3g}")
            print(f"{algoritmo} {fila['parametro']}: por media {fila['elegido_por_media']}, "
                  f"por mediana {fila['elegido_por_mediana']}{detalle}")
        for fila in borde:
            print(f"{algoritmo} {fila['parametro']} = {fila['valor']} (borde {fila['borde']}): brecha "
                  f"{100 * fila['brecha']:.1f} % ± {100 * fila['error_estandar']:.1f} frente a "
                  f"{100 * fila['brecha_del_elegido']:.1f} % con {fila['elegido']}; mejora en "
                  f"{fila['instancias_que_mejoran']} de {len(referencias.nombres)}; Wilcoxon p = {fila['wilcoxon_p']:.3g}")
    with open(os.path.join(CARPETA_RESULTADOS, "comprobaciones.json"), "w", encoding="utf-8") as archivo:
        json.dump(informe, archivo, indent=1, ensure_ascii=False)


if __name__ == "__main__":
    main()
