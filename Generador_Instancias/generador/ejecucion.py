import os
import subprocess
from concurrent.futures import ThreadPoolExecutor

from construccion import CARPETA_INSTANCIAS, CARPETA_PROYECTO, escribir_csv

PRESUPUESTO_EVALUACIONES = 30000
COLUMNA_APTITUD = {"aranas": "Z", "recocido": "Z_mejor"}
COLUMNA_DESVIACION = {"aranas": "D", "recocido": "D_mejor"}
COLUMNA_COSTO = {"aranas": "costo", "recocido": "costo_mejor"}


def buscar_programa(nombre, proyectos):
    carpeta_superior = os.path.dirname(CARPETA_PROYECTO)
    for proyecto in proyectos:
        for compilacion in ("cmake-build-release", "build", "cmake-build-debug"):
            for archivo in (nombre + ".exe", nombre):
                ruta = os.path.join(carpeta_superior, proyecto, compilacion, archivo)
                if os.path.isfile(ruta):
                    return ruta
    return None


def programas(ruta_ssa_basico=None, ruta_formulacion=None):
    return {"aranas": ruta_ssa_basico or os.environ.get("SSA_BASICO")
            or buscar_programa("ssa_basico", ["SSA_Basico", ""]),
            "recocido": ruta_formulacion or os.environ.get("FORMULACION")
            or buscar_programa("formulacion", ["Algoritmo de Arañas Sociales"])}


def nombre_de_configuracion(configuracion):
    return "_".join(f"{clave}{valor:.6g}" if isinstance(valor, float) else f"{clave}{valor}"
                    for clave, valor in sorted(configuracion.items()))


def escribir_parametros_ssa(carpeta_configuracion, configuracion, repeticiones):
    iteraciones = PRESUPUESTO_EVALUACIONES // configuracion["poblacion"]
    escribir_csv(os.path.join(carpeta_configuracion, "parametros_ssa.csv"), ["clave", "valor"], [
        ("poblacion", configuracion["poblacion"]), ("ra", repr(configuracion["ra"])),
        ("pc", repr(configuracion["pc"])), ("pm", repr(configuracion["pm"])),
        ("iteraciones", iteraciones), ("repeticiones", repeticiones)])


def preparar_carpeta(algoritmo, carpeta_configuracion, configuracion, repeticiones):
    os.makedirs(carpeta_configuracion, exist_ok=True)
    if algoritmo == "aranas":
        escribir_parametros_ssa(carpeta_configuracion, configuracion, repeticiones)


def orden_aranas(programa, carpeta_instancia, configuracion, repeticiones, salida):
    archivo_parametros = os.path.join(os.path.dirname(salida), "parametros_ssa.csv")
    return [programa, carpeta_instancia, archivo_parametros, salida]


def orden_recocido(programa, carpeta_instancia, configuracion, repeticiones, salida):
    candidatas = int(configuracion["n"])
    temperaturas = max(1, round(PRESUPUESTO_EVALUACIONES / candidatas) - 1)  # I + 1 temperaturas de n candidatas
    orden = [programa, "recocido", carpeta_instancia, "--repeticiones", str(repeticiones), "--hilos", "1",
             "--salida", salida, "--p", repr(configuracion["p"]), "--n", str(candidatas), "--I", str(temperaturas),
             "--tf", repr(configuracion["tf"]), "--Ev", repr(configuracion["Ev"]),
             "--vecino", configuracion["vecino"], "--aceptacion", configuracion["aceptacion"],
             "--perturbacion", configuracion["perturbacion"], "--delta", configuracion["delta"]]
    if configuracion.get("activar"):
        orden.append("--activar")
    return orden


ORDENES = {"aranas": orden_aranas, "recocido": orden_recocido}


class Ejecutor:

    def __init__(self, rutas_programas):
        self.programas = rutas_programas

    def orden(self, algoritmo, instancia, configuracion, repeticiones, salida):
        return ORDENES[algoritmo](self.programas[algoritmo], os.path.join(CARPETA_INSTANCIAS, instancia),
                                  configuracion, repeticiones, salida)

    def ejecutar(self, trabajo):
        algoritmo, instancia, configuracion, repeticiones, salida = trabajo
        if os.path.isfile(salida):
            return
        temporal = salida + ".tmp"
        orden = self.orden(algoritmo, instancia, configuracion, repeticiones, temporal)
        proceso = subprocess.run(orden, capture_output=True, cwd=os.path.dirname(salida))
        if proceso.returncode != 0:
            raise RuntimeError(f"{' '.join(orden)}\n{proceso.stderr.decode('utf-8', errors='replace')}")
        os.replace(temporal, salida)

    def ejecutar_todos(self, trabajos):
        with ThreadPoolExecutor(max_workers=os.cpu_count() or 1) as ejecutor:
            list(ejecutor.map(self.ejecutar, trabajos))
