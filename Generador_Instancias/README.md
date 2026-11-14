# Generador_Instancias · Manual de uso

Genera los datasets sintéticos con los que se calibra y se evalúa el Algoritmo de Arañas Sociales (resultado 7). La idea es saber de antemano dónde el algoritmo debe acertar y dónde debe fallar, y comprobar que así ocurre:

- **Instancias que deben acertar.** Se construyen "hacia adelante". Primero se elige una formulación que ya cumple las condiciones operativas, la generadora. Después se abren los intervalos de concentración alrededor de lo que esa formulación produce. La generadora da D = 0 y Z = 0,5, de modo que existe una solución equilibrada y el óptimo cumple Z\* ≤ 0,5.
- **Instancias que deben fallar.** Se toma una instancia satisfacible y se le cambia una sola cosa para que no tenga buena solución: sin molibdeno, k = 4, ε = 1 g o E^max = 1,0 dS/m. Cada una lleva una cota inferior de la desviación, D_cota. Ninguna formulación admisible baja de ella, así que si el algoritmo baja, hay un error.
- **Entradas inválidas.** Archivos con un error concreto que `ssa_basico` debe rechazar con su mensaje.

Las instancias tienen el formato de `SSA_Basico/datos/completa` y se ejecutan directamente con `ssa_basico`.

## 1. Contenido de la carpeta

```
Generador_Instancias/
├── requirements.txt         versiones exactas de NumPy y SciPy
├── base/                    instancia completa de tres etapas (copia de SSA_Basico/datos/completa)
├── generador/
│   ├── construccion.py      lectura de la base, semillas, formulación generadora y escritura de archivos
│   ├── generar.py           genera todos los conjuntos (programa principal)
│   ├── fallos.py            instancias que deben fallar, sus cotas y las entradas inválidas
│   ├── evaluar.py           función objetivo y admisibilidad, igual que ssa_basico
│   ├── exacto.py            óptimo exacto de cada instancia con HiGHS
│   └── verificar.py         las cinco comprobaciones del resultado
└── instancias/              una carpeta por instancia, manifiesto.csv, referencias.csv e invalidas.csv
```

`instancias/` no se sube al repositorio (`.gitignore`): son unos 2300 archivos que salen del código. En un clon nuevo se crea con los pasos 1 y 2 de la sección 3, en unos 10 segundos, y queda idéntica byte a byte a la original; el paso 3 lo comprueba. `base/` sí se sube, porque es la entrada del generador y no se puede regenerar.

## 2. Abrir en PyCharm

1. *File → Open* y elegir la carpeta `Generador_Instancias`.
2. Intérprete: *Settings → Project → Python Interpreter → Add Interpreter → Virtualenv Environment*, con la carpeta `.venv` del proyecto y Python 3.13. Si la `.venv` no existe, PyCharm la crea. Después, en la terminal de PyCharm:
   ```
   pip install -r requirements.txt
   ```
3. Para ejecutar un programa, clic derecho sobre el archivo en `generador/` y *Run*. Los programas encuentran sus carpetas solos, sin importar el directorio de trabajo.

Desde una terminal, en la carpeta del proyecto: `.venv/Scripts/python generador/<programa>.py` en Windows, o `.venv/bin/python …` en Linux.

## 3. Uso

| Paso | Programa | Qué hace | Tiempo |
|---|---|---|---|
| 1 | `generar.py` | Escribe las 339 carpetas de `instancias/` y los archivos `manifiesto.csv` e `invalidas.csv` | 6 s |
| 2 | `exacto.py` | Resuelve el óptimo exacto de las 334 instancias ejecutables y escribe `exacta.csv` en cada una y `referencias.csv` | 3 s |
| 3 | `verificar.py` | Hace las cinco comprobaciones; necesita `ssa_basico` compilado | 25 s |

`verificar.py` busca `ssa_basico` en `../SSA_Basico/cmake-build-release`, `build` o `cmake-build-debug`, o en esas mismas carpetas al lado del proyecto, como en el repositorio. También se puede indicar con `--binario ruta/ssa_basico.exe`. Conviene compilarlo en Release. Otras opciones: `--repeticiones 30` y `--exacto`, que vuelve a resolver los óptimos antes de comprobarlos.

## 4. Conjuntos

| Prefijo | Conjunto | Tipo | Factores que cruza | Instancias | Semilla maestra |
|---|---|---|---|---|---|
| `cal_` | calibracion | acierto | \|F\|, k, ε; 2 repeticiones | 54 | 2026 |
| `fal_` | calibracion | fallo | caso × \|F\|; 2 repeticiones | 24 | 2029 |
| `exp_` | experimentacion | acierto | \|F\|, k, ε, τ; 3 repeticiones | 243 | 2027 |
| `rob_` | robustez | acierto | agua, V; 3 repeticiones | 12 | 2028 |
| `fal_sinCatalogo` | verificacion | fallo | catálogo vacío | 1 | 2029 |
| `inv_` | — | inválida | un error por archivo | 5 | — |

- **La calibración usa 78 instancias:** las 54 de acierto y las 24 de fallo, todas con `conjunto = calibracion` en el manifiesto.
- Los factores que un conjunto no cruza toman el nivel central: |F| = 35, k = 16, ε = 0,05 g, τ = 0,05, agua destilada y V = 1000 L. Todas usan γ = 0,5 y E^max = 2,5 dS/m, salvo el caso emax10.
- El nombre de cada instancia lleva sus niveles, por ejemplo `exp_F35_k16_e0.05_t0.05_destilada_V1000_r2` o `fal_k4_F20_r1`. En las de acierto, `r1`, `r2` y `r3` toman los intervalos de propagación, crecimiento y producción.

## 5. Casos que deben fallar

| Caso | Cambio | Por qué falla | D_cota |
|---|---|---|---|
| `sinMo` | Se retiran las dos fuentes de molibdeno; con \|F\| = 54 quedan 52 | El molibdeno se queda sin aporte | 1 − τ = 0,95 |
| `k4` | k = 4 | Cuatro fertilizantes no cubren las 14 especies con requerimiento | 4,75 a 5,70 |
| `eps1` | ε = 1 g | Con 1 g, las fuentes de Zn, Cu, Ni y Mo exceden su máximo, y dejarlas en cero las deja sin aporte | 3,80 a 4,75 |
| `emax10` | E^max = 1,0 dS/m | Los cationes no caben en el límite; el quiebre de estas instancias está entre 1,44 y 2,13 dS/m | 0,66 a 1,27 |
| `sinCatalogo` | Ningún fertilizante | Queda solo el agua | D(0) = 13,3 |

D_cota es la mayor de cuatro cotas que valen para cualquier instancia (`fallos.py`):

1. **Especies sin fuente:** quedan con la concentración del agua.
2. **Cardinalidad:** el mejor grupo de k fertilizantes, por búsqueda exhaustiva, deja sin cubrir al menos esa desviación.
3. **Dosis mínima:** cada especie, o queda sin fuente o recibe al menos α·ε.
4. **Conductividad:** relajación del límite de cationes. El déficit se carga primero a las especies que más meq/L liberan por unidad de desviación.

La cota de conductividad sustituye a la fórmula del 23/09 (`Diseno_Experimental_R7.md`). Esa fórmula suponía que la reparación escala todos los cationes por igual, lo que no la hace cota para cualquier formulación.

## 6. Archivos de salida

- **Por instancia:** `parametros.csv`, `especies.csv`, `agua.csv`, `limites.csv` y `fertilizantes.csv`, los que lee `ssa_basico`. Además, `generadora.csv` en las de acierto y `exacta.csv` (columnas f, s, q) en todas las ejecutables. `parametros.csv` lleva también cuatro claves que solo usa el proyecto completo (`restriccion_*` y `normalizar_costo`); `ssa_basico` las ignora.
- **`manifiesto.csv`:** una fila por instancia con conjunto, tipo (acierto o fallo), caso, niveles, semilla, etapa base, intentos, número de fertilizantes de la generadora, Kref, la generadora y D_cota.
- **`referencias.csv`:** Z de la generadora, Z\* exacto, cota inferior de Z\* (la cota dual de HiGHS), D\*, D_cota y estado del resolvedor (0 = óptimo demostrado). La calibración usa Z\* para medir la brecha (Z − Z\*)/Z\*. Nunca puede salir Z por debajo de la cota inferior.
- **`invalidas.csv`:** cada entrada inválida con el texto del error que debe dar `ssa_basico`.

## 7. Qué cuenta como acertar y fallar

- **Acierta:** la mejor solución tiene Z ≤ 0,5, es decir, un equilibrio entre desviación y costo al menos tan bueno como el de la generadora. La medida continua es la brecha a Z\*.
- **Falla como debe:** D ≥ D_cota en cada repetición y Z ≥ cota inferior de Z\*. Todas las instancias de fallo tienen Z\* > 0,5 (la menor, 0,558), así que tampoco pueden acertar.

## 8. Reproducibilidad

- La semilla de cada instancia es el SHA-256 de la semilla maestra de su conjunto y de sus niveles. No se sortea, así que cada instancia se puede regenerar por separado y añadir niveles no cambia las existentes.
- Con NumPy 2.3.5 y SciPy 1.16.3 (`requirements.txt`) la salida es idéntica byte a byte, y `verificar.py` lo comprueba. Con otras versiones, las últimas cifras pueden cambiar (del orden de 10⁻¹³ en términos relativos), aunque los catálogos y las generadoras salen iguales.
- Los archivos se escriben en UTF-8 con fin de línea LF, sea cual sea el sistema.
- Las 309 instancias de acierto son las mismas del proyecto completo (`Algoritmo de Arañas Sociales/herramientas/generador`): mismos catálogos, mismas generadoras y mismos Z\*, con una diferencia máxima de 5·10⁻¹⁶.

## 9. Resultado de la verificación (13/11/2026)

`ssa_basico` en Release, con los parámetros de partida (población 30, r_a = 1, p_c = 0,7, p_m = 0,1, 1000 iteraciones) y 30 repeticiones:

```
1. Reproducibilidad: 339 carpetas regeneradas; archivos distintos: 0
2. Generadoras: 309 de 309 admisibles con D = 0 y Z = 0,5
3. Óptimos exactos: 334 de 334 correctos (Z* ≤ 0,5 en las de acierto; D* ≥ D_cota en las 25 de fallo)
4. ssa_basico en 79 instancias, 30 repeticiones cada una: ninguna ejecución por debajo de su cota
   Deben acertar: 1289 de 1620 ejecuciones con Z ≤ 0,5 (79.6 %); brecha media a Z*: 56.3 %
   Deben fallar, emax10: D mínima 0.9028, margen mínimo sobre D_cota 0.2309; brecha media a Z* 91.7 %
   Deben fallar, eps1: D mínima 3.8000, margen mínimo sobre D_cota 0.0000; brecha media a Z* 2.4 %
   Deben fallar, k4: D mínima 5.7000, margen mínimo sobre D_cota 0.0000; brecha media a Z* 4.2 %
   Deben fallar, sinCatalogo: D mínima 13.3000, margen mínimo sobre D_cota -0.0000; brecha media a Z* 0.0 %
   Deben fallar, sinMo: D mínima 0.9500, margen mínimo sobre D_cota 0.0000; brecha media a Z* 9.8 %
5. Entradas inválidas: 5 de 5 rechazadas con el mensaje esperado
Todo se cumple.
```
