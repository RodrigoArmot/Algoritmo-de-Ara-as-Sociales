"""Tablas del capítulo 7 y de los Anexos L, N y O, a partir de los resultados guardados.

Lee instancias/, resultados/verificacion.json, resultados/calibracion/ y resultados/comparacion/, y
escribe resultados/tablas_capitulo7.md con todas las tablas en el formato de la tesis: coma decimal,
porcentajes con un decimal y valores p menores que 0,001 como «< 0,001». No ejecuta nada: hay que correr
antes verificar.py, calibrar.py (aranas, recocido y confirmar), comprobar_calibracion.py, comparar.py,
analizar.py y medir_tiempos.py.

Uso: .venv/Scripts/python experimentacion/tablas.py
"""
import json
import os
import sys

CARPETA_PROYECTO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(CARPETA_PROYECTO, "generador"))

from construccion import CARPETA_INSTANCIAS, leer_csv  # noqa: E402

RESULTADOS = os.path.join(CARPETA_PROYECTO, "resultados")
SSA, SA = "Algoritmo de Arañas Sociales", "Recocido simulado"
NOMBRE_PARAMETRO = {
    "poblacion": "Tamaño de la población", "ra": "Tasa de atenuación ⟨r_a⟩",
    "pc": "Probabilidad de cambio de máscara ⟨p_c⟩", "pm": "Tasa de máscara ⟨p_m⟩",
    "Ev": "Perturbación máxima ⟨E_v⟩", "n": "Candidatas por temperatura ⟨n⟩",
    "p": "Probabilidad de aceptación inicial ⟨p⟩", "tf": "Temperatura final ⟨t_f⟩"}
NOMBRE_FACTOR = {"F": "⟨|F|⟩", "k": "⟨k⟩", "eps": "⟨ε⟩ (g)", "tau": "⟨τ⟩", "agua": "Agua", "V": "⟨V⟩ (L)"}
NOMBRE_RESPUESTA = {"brecha_relativa_A": f"Brecha relativa, {SSA}", "brecha_relativa_B": f"Brecha relativa, {SA}",
                    "A12": "Tamaño del efecto ⟨A_12⟩"}
NOMBRE_CASO = {"sinMo": "Sin fuentes de molibdeno", "k4": "⟨k⟩ = 4", "eps1": "⟨ε⟩ = 1 g",
               "emax10": "⟨E^max⟩ = 1,0 dS/m", "sinCatalogo": "Catálogo vacío"}
NOMBRE_AGUA = {"destilada": "destilada", "cihnm": "de pozo (CIHNM)"}
NOMBRE_CONTRASTE = {"experimentacion": "Experimentación: SSA frente a SA", "robustez": "Robustez: SSA frente a SA",
                    "activacion": "Activación de dosis: SA con frente a sin",
                    "experimentacion y robustez": "Experimentación y robustez"}


# --- Formato ---

def numero(valor, decimales=3):
    return f"{valor:.{decimales}f}".replace(".", ",").replace("-", "−")


def porcentaje(valor, decimales=1):
    return f"{numero(100 * valor, decimales)} %"


def valor_p(valor):
    return "< 0,001" if valor < 0.001 else numero(valor, 3)


def p_igual(valor):
    """«p < 0,001» o «p = 0,110», para usar dentro de una oración."""
    return "p < 0,001" if valor < 0.001 else f"p = {numero(valor, 3)}"


def parametro(valor):
    """Valores de parámetros: potencias de diez como ⟨10^(−k)⟩ y el resto con coma decimal."""
    if isinstance(valor, float) and 0 < valor < 1e-3:
        return f"⟨10^(−{round(-__import__('math').log10(valor))})⟩"
    if isinstance(valor, float) and valor.is_integer():
        return str(int(valor))
    return f"{valor:g}".replace(".", ",")


def tabla(titulo, encabezado, filas, nota):
    lineas = [f"**{titulo}**", "", "| " + " | ".join(encabezado) + " |", "|" + "---|" * len(encabezado)]
    lineas += ["| " + " | ".join(str(celda) for celda in fila) + " |" for fila in filas]
    lineas += ["", f"*Nota.* {nota} Elaboración propia.", ""]
    return "\n".join(lineas)


def leer_json(*partes):
    with open(os.path.join(RESULTADOS, *partes), encoding="utf-8") as archivo:
        return json.load(archivo)


def rango(valores, decimales=3):
    if numero(min(valores), decimales) == numero(max(valores), decimales):
        return numero(min(valores), decimales)
    return f"{numero(min(valores), decimales)} a {numero(max(valores), decimales)}"


# --- Anexo L ---

def tablas_anexo_l(manifiesto, referencias, verificacion):
    salida = ["## Anexo L · Instancias de prueba y generador", ""]
    conjuntos = [("cal_", "Calibración", "acierto", "⟨|F|⟩, ⟨k⟩, ⟨ε⟩", 2, 2026),
                 ("fal_", "Calibración", "fallo", "caso de fallo, ⟨|F|⟩", 2, 2029),
                 ("exp_", "Experimentación", "acierto", "⟨|F|⟩, ⟨k⟩, ⟨ε⟩, ⟨τ⟩", 3, 2027),
                 ("rob_", "Robustez", "acierto", "agua, ⟨V⟩", 3, 2028)]
    filas = []
    for prefijo, conjunto, tipo, factores, repeticiones, semilla in conjuntos:
        cantidad = sum(nombre.startswith(prefijo) and (prefijo != "fal_" or fila["conjunto"] == "calibracion")
                       for nombre, fila in manifiesto.items())
        filas.append([conjunto, "Deben acertar" if tipo == "acierto" else "Deben fallar", factores, repeticiones,
                      cantidad, semilla])
    filas.append(["Verificación", "Deben fallar", "catálogo vacío", 1, 1, 2029])
    filas.append(["Verificación", "Entradas inválidas", "un error por archivo", "—",
                  len(leer_csv(os.path.join(CARPETA_INSTANCIAS, "invalidas.csv"))), "—"])
    salida.append(tabla("Tabla L1. Conjuntos de instancias",
                        ["Conjunto", "Tipo", "Factores que cruza", "Repeticiones", "Instancias", "Semilla maestra"],
                        filas, "Los factores que un conjunto no cruza toman su nivel central: ⟨|F|⟩ = 35, ⟨k⟩ = 16, "
                               "⟨ε⟩ = 0,05 g, ⟨τ⟩ = 0,05, agua destilada y ⟨V⟩ = 1000 L."))
    filas = []
    for caso in ("sinMo", "k4", "eps1", "emax10", "sinCatalogo"):
        de_caso = [r for r in referencias.values() if r["caso"] == caso]
        filas.append([NOMBRE_CASO[caso], len(de_caso), rango([float(r["D_cota"]) for r in de_caso]),
                      rango([float(r["D_exacto"]) for r in de_caso]), rango([float(r["Z_exacto"]) for r in de_caso])])
    salida.append(tabla("Tabla L2. Instancias que deben fallar",
                        ["Caso", "Instancias", "Cota de la desviación ⟨D_cota⟩", "Desviación del óptimo ⟨D^*⟩",
                         "Aptitud óptima ⟨Z^*⟩"], filas,
                        "Con ⟨|F|⟩ = 54, el caso sin fuentes de molibdeno queda con 52 fertilizantes. Ninguna "
                        "formulación admisible tiene una desviación menor que su cota."))
    v = verificacion
    filas = [
        ["Reproducibilidad", f"{v['reproducibilidad']['carpetas']} carpetas regeneradas, "
                             f"{v['reproducibilidad']['archivos_distintos']} archivos distintos"],
        ["Formulación generadora", f"{v['generadoras']['correctas']} de {v['generadoras']['total']} admisibles "
                                   f"con ⟨D = 0⟩ y ⟨Z = 0,5⟩"],
        ["Óptimo exacto", f"{v['exactos']['correctos']} de {v['exactos']['total']} admisibles y demostrados; "
                          f"⟨Z^* ≤ 0,5⟩ en las de acierto y ⟨D^* ≥ D_cota⟩ en las {v['exactos']['de_fallo']} de fallo"],
        ["Cotas con ssa_basico", f"{v['ssa_basico']['instancias']} instancias × {v['ssa_basico']['repeticiones']} "
                                 f"ejecuciones; {v['ssa_basico']['ejecuciones_bajo_cota']} por debajo de su cota"],
        ["Entradas inválidas", f"{v['invalidas']['rechazadas_como_se_espera']} de {v['invalidas']['total']} "
                               f"rechazadas con el mensaje esperado"]]
    salida.append(tabla("Tabla L3. Verificación del generador", ["Comprobación", "Resultado"], filas,
                        "La comprobación con ssa_basico usa la configuración de partida."))
    return salida


# --- Calibración: Tabla 19 y Anexo N ---

def tablas_calibracion():
    salida = ["## Calibración · Tabla 19 y Anexo N", ""]
    informes = {algoritmo: leer_json("calibracion", f"{algoritmo}.json") for algoritmo in ("aranas", "recocido")}
    filas = []
    for algoritmo, nombre in (("aranas", SSA), ("recocido", SA)):
        informe = informes[algoritmo]
        for indice, barrido in enumerate(informe["pasadas"][0]["barridos"]):
            p = barrido["parametro"]
            filas.append([nombre if indice == 0 else "", NOMBRE_PARAMETRO[p],
                          f"{parametro(min(barrido['valores']))} a {parametro(max(barrido['valores']))}",
                          parametro(informe["partida"][p]), parametro(informe["calibrada"][p])])
    salida.append(tabla("Tabla 19. Parámetros de control calibrados",
                        ["Algoritmo", "Parámetro", "Valores explorados", "Valor de partida", "Valor calibrado"], filas,
                        "Calibración con las 78 instancias del conjunto de calibración (54 que deben acertar y 24 que "
                        "deben fallar); respuesta: brecha relativa media al óptimo."))
    for letra, (algoritmo, nombre) in zip("12", (("aranas", SSA), ("recocido", SA))):
        informe = informes[algoritmo]
        pasadas = informe["pasadas"]
        filas = []
        for indice, barrido in enumerate(pasadas[0]["barridos"]):
            for j, valor in enumerate(barrido["valores"]):
                celdas = []
                for pasada in pasadas:
                    b = pasada["barridos"][indice]
                    marca = " *" if valor == b["elegido"] else ""
                    celdas.append(f"{porcentaje(b['brecha'][j])} ± {numero(100 * b['error_estandar'][j], 1)}{marca}")
                filas.append([NOMBRE_PARAMETRO[barrido["parametro"]] if j == 0 else "", parametro(valor)] + celdas)
        salida.append(tabla(f"Tabla N{letra}. Calibración del {nombre if algoritmo == 'aranas' else nombre.lower()}",
                            ["Parámetro", "Valor"] + [f"Brecha media, pasada {p['pasada']}" for p in pasadas], filas,
                            "Brecha relativa media al óptimo ± error estándar sobre las 78 instancias, con dos "
                            "ejecuciones por instancia y 30 000 evaluaciones. El asterisco marca el valor elegido en "
                            "cada pasada; cada parámetro se varía con los demás en su valor vigente."))
    confirmacion = leer_json("calibracion", "confirmacion.json")
    filas = [[nombre, porcentaje(c["brecha_partida"]), porcentaje(c["brecha_calibrada"]),
              f"{c['instancias_que_mejoran']} de {c['instancias']}", valor_p(c["wilcoxon_p"]),
              f"{porcentaje(c['acierto_partida'])} → {porcentaje(c['acierto_calibrada'])}"]
             for nombre, c in ((SSA, confirmacion["aranas"]), (SA, confirmacion["recocido"]))]
    salida.append(tabla("Tabla N3. Confirmación de la calibración",
                        ["Algoritmo", "Brecha de partida", "Brecha calibrada", "Instancias en que mejora",
                         "Wilcoxon, p", "Ejecuciones con acierto"], filas,
                        "Cuatro ejecuciones nuevas por instancia. Prueba de rangos con signo de Wilcoxon sobre la brecha "
                        "de cada instancia. El acierto se cuenta en las 54 instancias que deben acertar."))
    comprobaciones = leer_json("calibracion", "comprobaciones.json")
    filas = []
    for algoritmo, nombre in (("aranas", SSA), ("recocido", SA)):
        for fila in comprobaciones[algoritmo]["eleccion_por_mediana"]:
            if fila["coincide"]:
                filas.append([nombre, NOMBRE_PARAMETRO[fila["parametro"]], "Elección por mediana",
                              f"{parametro(fila['elegido_por_mediana'])}, igual que por media"])
            else:
                filas.append([nombre, NOMBRE_PARAMETRO[fila["parametro"]], "Elección por mediana",
                              f"{parametro(fila['elegido_por_mediana'])} en lugar de {parametro(fila['elegido_por_media'])}; "
                              f"diferencia no significativa (Wilcoxon, {p_igual(fila['wilcoxon_p'])})"])
        for fila in comprobaciones[algoritmo]["borde_de_la_rejilla"]:
            filas.append([nombre, NOMBRE_PARAMETRO[fila["parametro"]], f"Más allá del borde {fila['borde']}",
                          f"{parametro(fila['valor'])}: {porcentaje(fila['brecha'])} frente a "
                          f"{porcentaje(fila['brecha_del_elegido'])} con {parametro(fila['elegido'])} "
                          f"(Wilcoxon, {p_igual(fila['wilcoxon_p'])})"])
    salida.append(tabla("Tabla N4. Comprobaciones de la calibración", ["Algoritmo", "Parámetro", "Comprobación",
                                                                       "Resultado"], filas,
                        "Pasada final. La elección por mediana aplica la misma regla de empate que la calibración."))
    return salida


# --- Comparación: Tablas 20 y 21 y Anexo O ---

def con_cuartiles(r, clave, decimales=3, en_porcentaje=False):
    formato = (lambda x: porcentaje(x)) if en_porcentaje else (lambda x: numero(x, decimales))
    return f"{formato(r[clave + '_mediana'])} [{formato(r[clave + '_q1'])} a {formato(r[clave + '_q3'])}]"


def tablas_comparacion(resumen, tiempos):
    salida = ["## Comparación · Tablas 20 y 21 y Anexo O", ""]
    r = resumen["contrastes"]["experimentacion"]["resumen"]
    filas = [
        ["Aptitud ⟨Z⟩", con_cuartiles(r, "mediana_Z_A"), con_cuartiles(r, "mediana_Z_B")],
        ["Desviación ⟨D⟩", con_cuartiles(r, "mediana_D_A"), con_cuartiles(r, "mediana_D_B")],
        ["Costo (USD)", con_cuartiles(r, "mediana_costo_A", 2), con_cuartiles(r, "mediana_costo_B", 2)],
        ["Brecha al óptimo ⟨Z − Z^*⟩", con_cuartiles(r, "brecha_A"), con_cuartiles(r, "brecha_B")],
        ["Brecha relativa al óptimo", con_cuartiles(r, "brecha_relativa_A", en_porcentaje=True),
         con_cuartiles(r, "brecha_relativa_B", en_porcentaje=True)],
        ["Ejecuciones con acierto ⟨Z ≤ 0,5⟩", porcentaje(r["acierto_A"]), porcentaje(r["acierto_B"])],
        ["Ejecuciones que alcanzan el óptimo", porcentaje(r["optimo_A"]), porcentaje(r["optimo_B"])],
        ["Instancias en que es mejor", r["gana_A"], r["gana_B"]],
        ["Tiempo por ejecución (s)", numero(tiempos["aranas_s_por_ejecucion"]),
         numero(tiempos["recocido_s_por_ejecucion"])]]
    salida.append(tabla("Tabla 20. Comparación sobre las 243 instancias de experimentación",
                        ["Indicador", SSA, SA], filas,
                        f"Mediana sobre las instancias de la mediana de las 30 ejecuciones, con el rango "
                        f"intercuartílico entre corchetes. La aptitud óptima mediana es "
                        f"{numero(r['Z_exacto_mediana'])}. Una ejecución alcanza el óptimo si "
                        f"⟨Z − Z^* ≤ 10^(−3)⟩; con ⟨10^(−4)⟩ la proporción es {porcentaje(r['optimo_0.0001_A'])} y "
                        f"{porcentaje(r['optimo_0.0001_B'])}, y con ⟨10^(−2)⟩, {porcentaje(r['optimo_0.01_A'])} y "
                        f"{porcentaje(r['optimo_0.01_B'])}. En {r['empates']} instancias la diferencia no es "
                        f"significativa. Tiempo medido en un solo hilo ({tiempos['procesador']})."))
    filas = []
    for fila in resumen["contrastes"]["experimentacion"]["por_factor"]:
        factor = fila["factor"]
        filas.append([NOMBRE_FACTOR[factor], parametro(fila["nivel"]), numero(fila["brecha_A_mediana"]),
                      numero(fila["brecha_B_mediana"]), porcentaje(fila["brecha_relativa_A_mediana"]),
                      porcentaje(fila["brecha_relativa_B_mediana"]), porcentaje(fila["acierto_A"]),
                      porcentaje(fila["acierto_B"]),
                      f"{fila['gana_A']} / {fila['gana_B']} / {fila['empates']}"])
    salida.append(tabla("Tabla 21. Resultados por nivel de cada factor",
                        ["Factor", "Nivel", "Brecha SSA", "Brecha SA", "Brecha relativa SSA", "Brecha relativa SA",
                         "Acierto SSA", "Acierto SA", "Mejor SSA / mejor SA / sin diferencia"], filas,
                        "Brecha: mediana sobre las instancias de la brecha mediana por instancia. Cada nivel reúne "
                        "81 instancias."))
    filas = []
    for fila in resumen["efecto_de_factores"]:
        if fila["contraste"] != "experimentacion":
            continue
        medianas = " / ".join(
            porcentaje(float(m)) if fila["respuesta"] != "A12" else numero(float(m))
            for m in fila["medianas"].split(" / "))
        filas.append([NOMBRE_FACTOR[fila["factor"]], NOMBRE_RESPUESTA[fila["respuesta"]], medianas,
                      numero(fila["estadistico"], 1), valor_p(fila["p"]), numero(fila["valor_efecto"], 3)])
    salida.append(tabla("Tabla O1. Efecto de cada factor (prueba de Friedman)",
                        ["Factor", "Respuesta", "Mediana por nivel", "⟨χ^2⟩ de Friedman", "p", "⟨W⟩ de Kendall"], filas,
                        "Bloques de 81: cada uno reúne las tres instancias que solo difieren en el factor (mismos "
                        "niveles de los demás factores y misma repetición). ⟨W⟩ de Kendall: 0 sin efecto, 1 efecto "
                        "máximo. Las comparaciones por pares, con la prueba de Wilcoxon y la corrección de Holm, están "
                        "en efecto_de_factores.csv."))
    robustez = leer_csv(os.path.join(RESULTADOS, "comparacion", "robustez_por_instancia.csv"))
    resultado = {"A": "SSA", "B": "SA", "empate": "sin diferencia"}
    filas = [[NOMBRE_AGUA[f["agua"]], numero(float(f["V"]), 0), f["rep"], numero(float(f["Z_exacto"])),
              numero(float(f["mediana_Z_A"])), numero(float(f["mediana_Z_B"])), porcentaje(float(f["acierto_A"])),
              porcentaje(float(f["acierto_B"])), valor_p(float(f["p_holm"])), resultado[f["resultado"]]]
             for f in sorted(robustez, key=lambda f: (f["agua"], float(f["V"]), int(f["rep"])))]
    rr = resumen["contrastes"]["robustez"]["resumen"]
    salida.append(tabla("Tabla O2. Instancias de robustez",
                        ["Agua", "⟨V⟩ (L)", "Repetición", "⟨Z^*⟩", "Mediana ⟨Z⟩ SSA", "Mediana ⟨Z⟩ SA", "Acierto SSA",
                         "Acierto SA", "p (Holm)", "Mejor"], filas,
                        f"Mann-Whitney por instancia con la corrección de Holm. Sobre las 12 instancias, prueba de "
                        f"Wilcoxon: {p_igual(rr['wilcoxon_p'])}."))
    filas = []
    for fila in resumen["efecto_de_factores"]:
        if fila["contraste"] != "robustez":
            continue
        medianas = " / ".join(porcentaje(float(m)) if fila["respuesta"] != "A12" else numero(float(m))
                              for m in fila["medianas"].split(" / "))
        filas.append([NOMBRE_FACTOR[fila["factor"]], NOMBRE_RESPUESTA[fila["respuesta"]], medianas,
                      valor_p(fila["p"]), numero(fila["valor_efecto"], 3)])
    salida.append(tabla("Tabla O3. Efecto del agua y del volumen en robustez (Wilcoxon pareada)",
                        ["Factor", "Respuesta", "Mediana por nivel", "p", "Biserial de rangos"], filas,
                        "Seis pares por factor. Niveles: agua de pozo y destilada; 200 y 1000 L. Con seis pares, el "
                        "menor valor p posible es 0,031."))
    a = resumen["contrastes"]["activacion"]["resumen"]
    filas = [["Aptitud ⟨Z⟩", numero(a["mediana_Z_A_mediana"]), numero(a["mediana_Z_B_mediana"])],
             ["Desviación ⟨D⟩", numero(a["mediana_D_A_mediana"]), numero(a["mediana_D_B_mediana"])],
             ["Costo (USD)", numero(a["mediana_costo_A_mediana"], 2), numero(a["mediana_costo_B_mediana"], 2)],
             ["Ejecuciones con acierto", porcentaje(a["acierto_A"]), porcentaje(a["acierto_B"])],
             ["Instancias en que es mejor", a["gana_A"], a["gana_B"]]]
    salida.append(tabla("Tabla O4. Efecto de la activación de dosis en el recocido",
                        ["Indicador", "Con activación", "Sin activación"], filas,
                        f"243 instancias de experimentación. En {a['empates']} la diferencia no es significativa; "
                        f"Wilcoxon sobre el conjunto: {p_igual(a['wilcoxon_p'])}."))
    pruebas = leer_csv(os.path.join(RESULTADOS, "comparacion", "pruebas_estadisticas.csv"))
    filas = []
    for fila in pruebas:
        if "Friedman" in fila["prueba"] or "Wilcoxon pareada" in fila["prueba"]:
            continue  # ya están en las Tablas O1 y O3
        p = valor_p(float(fila["p"])) if fila["p"] else "—"
        efecto = f"{fila['efecto']} = {numero(float(fila['valor_efecto']), 3)}" if fila["valor_efecto"] else "—"
        filas.append([NOMBRE_CONTRASTE.get(fila["contraste"], fila["contraste"]), fila["prueba"], fila["n"], p, efecto,
                      (fila.get("detalle", "") or "—").replace("gana A", "mejor el primero").replace("gana B", "mejor el segundo")])
    salida.append(tabla("Tabla O5. Pruebas estadísticas", ["Contraste", "Prueba", "n", "p", "Tamaño del efecto",
                                                          "Detalle"], filas,
                        "Biserial de rangos negativa: el primer algoritmo del contraste obtiene menor aptitud. Nivel de "
                        "significancia de 0,05."))
    e = resumen["exacto"]
    filas = [[SSA, numero(tiempos["aranas_s_por_ejecucion"]), f"{tiempos['instancias_algoritmos']} instancias × "
                                                               f"{tiempos['repeticiones']} ejecuciones"],
             [SA, numero(tiempos["recocido_s_por_ejecucion"]), f"{tiempos['instancias_algoritmos']} instancias × "
                                                              f"{tiempos['repeticiones']} ejecuciones"],
             ["Método exacto (HiGHS)", f"{numero(tiempos['exacto_mediana_s'])} (máximo {numero(tiempos['exacto_maximo_s'], 2)})",
              f"{tiempos['instancias_exacto']} instancias; óptimo demostrado en {e['optimos_demostrados']}, con una "
              f"brecha a la cota inferior de a lo más {numero(e['brecha_relativa_a_la_cota_maxima'] * 1e6, 1)} × "
              f"⟨10^(−6)⟩"]]
    salida.append(tabla("Tabla O6. Tiempo de cómputo", ["Método", "Tiempo (s)", "Medición"], filas,
                        f"Mediana del tiempo por ejecución, en un solo hilo, en {tiempos['procesador']}."))
    n = resumen["normalidad"]
    salida += ["**Normalidad (para el texto).** Prueba de Shapiro-Wilk sobre las 30 aptitudes de cada instancia y "
               f"algoritmo: rechaza la normalidad en {n['total']['rechazan']} de {n['total']['muestras']} muestras "
               f"({porcentaje(n['total']['rechazan'] / n['total']['muestras'])}; {n['aranas']['rechazan']} de "
               f"{n['aranas']['muestras']} en el {SSA} y {n['recocido']['rechazan']} de {n['recocido']['muestras']} en "
               f"el recocido), y en {n['total']['rechazan_con_holm']} con la corrección de Holm.", ""]
    return salida


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    manifiesto = {f["instancia"]: f for f in leer_csv(os.path.join(CARPETA_INSTANCIAS, "manifiesto.csv"))}
    referencias = {f["instancia"]: f for f in leer_csv(os.path.join(CARPETA_INSTANCIAS, "referencias.csv"))}
    partes = ["# Tablas del capítulo 7 y de los Anexos L, N y O", "",
              "Generado por experimentacion/tablas.py a partir de los resultados guardados. Las tablas que el "
              "capítulo conserva son la 19, la 20 y la 21; las demás van a los anexos.", ""]
    partes += tablas_anexo_l(manifiesto, referencias, leer_json("verificacion.json"))
    partes += tablas_calibracion()
    partes += tablas_comparacion(leer_json("comparacion", "resumen.json"), leer_json("comparacion", "tiempos.json"))
    ruta = os.path.join(RESULTADOS, "tablas_capitulo7.md")
    with open(ruta, "w", encoding="utf-8", newline="\n") as archivo:
        archivo.write("\n".join(partes))
    print(f"Tablas escritas en {ruta}")


if __name__ == "__main__":
    main()
