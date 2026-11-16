# SSA_Basico · Manual de uso

Versión básica del Algoritmo de Arañas Sociales adaptado al cálculo de la formulación de la solución nutritiva (R5 y R6). Lee una instancia, ejecuta el algoritmo un número fijo de veces, cada vez con su propia semilla, y guarda una fila por ejecución en un archivo CSV.

## 1. Contenido de la carpeta

```
SSA_Basico/
├── CMakeLists.txt            compilación del programa y de las pruebas
├── LICENCIAS_TERCEROS.txt    licencia MIT del código de referencia de Yu
├── datos/
│   ├── completa/             instancia del problema (5 archivos CSV)
│   └── parametros_ssa.csv    parámetros del algoritmo y número de repeticiones
└── algoritmo/
    ├── Instancia.h/.cpp      clase Instancia: lectura de los CSV, datos derivados y validación
    ├── Solucion.h            clase Solucion: dosis por fertilizante y etapa, y su evaluación
    ├── FuncionObjetivo.h/.cpp     clase FuncionObjetivo (R2)
    ├── OperadorReparacion.h/.cpp  clase OperadorReparacion (R4)
    ├── Arana.h/.cpp          clase Arana: vibración objetivo, máscara y caminata aleatoria
    ├── SSA.h/.cpp            clase AlgoritmoSSA: población inicial e iteración
    ├── Aleatorio.h           generador de números al azar y semilla de cada repetición
    ├── ModuloEjecucion.h/.cpp     clase ModuloEjecucion (R6): repeticiones y CSV de salida
    ├── main.cpp              programa ssa_basico
    └── pruebas.cpp           pruebas unitarias y de integración (Google Test)
```

## 2. Requisitos

- Un compilador de C++17: GCC 11 o posterior, o el MinGW que trae CLion en Windows.
- CMake 3.20 o posterior (CLion ya lo incluye).
- Conexión a internet la primera vez que se configura el proyecto: CMake descarga Google Test v1.17.0. Después ya no hace falta.

## 3. Compilar

### 3.1 En CLion (Windows)

1. *File → Open* y elegir la carpeta `SSA_Basico`. CLion detecta el `CMakeLists.txt` y configura el proyecto.
2. Recomendado: añadir el perfil **Release** en *Settings → Build, Execution, Deployment → CMake*, botón **+**. El perfil Debug funciona, pero es bastante más lento.
3. *Build → Build Project*. Se generan dos ejecutables: `ssa_basico`, el programa, y `pruebas`, las pruebas.

### 3.2 Desde una terminal (Linux)

Desde la carpeta `SSA_Basico`:

```
cmake -S . -B build
cmake --build build
```

Si no se indica el tipo de compilación, se usa Release.

## 4. Ejecutar

### 4.1 Orden

```
ssa_basico <carpeta de la instancia> <parametros_ssa.csv> <salida.csv>
```

Ejemplo, desde la carpeta `SSA_Basico`:

```
build/ssa_basico datos/completa datos/parametros_ssa.csv resultados.csv
```

### 4.2 En CLion

1. Elegir la configuración `ssa_basico` y abrir *Run → Edit Configurations…*.
2. **Program arguments:** `datos/completa datos/parametros_ssa.csv resultados.csv`
3. **Working directory:** la carpeta `SSA_Basico`. Si no se fija, el programa no encuentra la carpeta `datos`.
4. *Run*. El archivo `resultados.csv` queda en la carpeta `SSA_Basico`.

### 4.3 Qué muestra la consola

Una línea con la instancia y los parámetros, y una fila por repetición:

```
completa: 54 fertilizantes x 3 etapas, gamma = 0.50; poblacion 30, ra 1, pc 0.7, pm 0.1; 30 ejecuciones
 rep      Z mejor          D      costo          K    y     iter      evals
   1     3.808422     6.4700    15.4347     1.1468   16     1000      30000
   2     3.632422     5.5668    22.8541     1.6981   16     1000      30000
 ...
```

Treinta repeticiones con los parámetros por defecto tardan unos segundos en Release.

## 5. Archivo de salida

Una fila por repetición, con 17 cifras significativas para que cada valor pueda reproducirse exactamente.

| Columna | Significado |
|---|---|
| `repeticion` | número de la repetición, desde 1 |
| `semilla` | semilla con la que se ejecutó esa repetición |
| `Z` | aptitud de la mejor solución, γ·D + (1 − γ)·K |
| `D` | desviación del requerimiento nutricional |
| `K` | costo relativo, costo/Kref |
| `costo` | costo de la formulación, en USD |
| `seleccionados` | número de fertilizantes empleados (a lo más k) |
| `iteraciones` | iteraciones realizadas |
| `evaluaciones` | evaluaciones de la función objetivo (población × iteraciones) |

## 6. Archivos de datos

Todos son CSV con coma como separador, punto como separador decimal y encabezado en la primera fila.

**Cuidado con Excel en español:** al guardar puede cambiar las comas por punto y coma y los puntos decimales por comas, y entonces el programa no puede leer el archivo. Conviene editarlos con un editor de texto.

### 6.1 `parametros_ssa.csv`: parámetros del algoritmo

| Clave | Valor actual | Significado |
|---|---|---|
| `poblacion` | 30 | número de arañas |
| `ra` | 1.0 | tasa de atenuación |
| `pc` | 0.7 | probabilidad de conservar la máscara |
| `pm` | 0.1 | tasa de máscara |
| `iteraciones` | 1000 | criterio de parada |
| `repeticiones` | 30 | ejecuciones independientes |

Para cambiar un parámetro basta con editar este archivo; no hace falta recompilar.

### 6.2 Carpeta de la instancia (`datos/completa/`)

| Archivo | Columnas | Contenido |
|---|---|---|
| `parametros.csv` | `clave,valor` | `etapas`, `V` (L), `Emax` (dS/m), `k`, `gamma`, `Kref` (USD) |
| `especies.csv` | `especie,peso_atomico,carga_E_mas` | especies químicas; la carga es 0 si la especie no pertenece a E+ |
| `limites.csv` | `especie,etapa,Cmin,Cmax` | una fila por especie y etapa (mg/L); las etapas empiezan en 1 |
| `agua.csv` | `especie,C0` | concentración del agua de partida (mg/L) |
| `fertilizantes.csv` | `nombre,formula,mw,precio,epsilon,delta` y una columna por especie | peso molecular (g/mol), precio (USD/kg), dosis mínima y máxima (g) y número de átomos de cada especie; `formula` es solo informativa |

Las columnas de átomos de `fertilizantes.csv` deben llevar exactamente los símbolos de `especies.csv`.

Para probar otra instancia se copia la carpeta `completa` con otro nombre, se editan sus archivos y se pasa esa carpeta como primer argumento.

## 7. Reproducibilidad

La semilla de cada repetición se calcula a partir del nombre de la carpeta de la instancia, de γ y del número de repetición. En consecuencia:

- la misma instancia con los mismos parámetros da siempre los mismos resultados;
- si se renombra la carpeta o se cambia γ, cambian las semillas y, por tanto, los resultados;
- las semillas coinciden con las del proyecto completo (`Algoritmo de Arañas Sociales`), y las 30 repeticiones de la instancia completa dan los mismos valores en ambos;
- en otro sistema operativo o con otro compilador, las últimas cifras pueden variar por las funciones matemáticas de la biblioteca (logaritmo, potencia, raíz).

## 8. Pruebas

Son 36 pruebas de Google Test. Las de las clases del algoritmo son unitarias (R5); las del módulo de ejecución y del programa, unitarias y de integración (R6). En conjunto ejecutan todas las funciones y todas las líneas del código de `algoritmo/`, y todas sus ramas salvo tres que no pueden darse (el control de `Aleatorio::entero`, el desempate por índice cuando el índice es menor y la captura de una excepción ajena a `std::exception` en `main`).

En CLion, elegir la configuración **All CTest** (o `pruebas`) y pulsar *Run*; la ventana de resultados marca cada prueba en verde o en rojo. Desde una terminal:

```
ctest --test-dir build
```

| Clase | Pruebas | Qué comprueban |
|---|:-:|---|
| Instancia | 9 | lectura de la instancia completa, con BOM, líneas en blanco y barra final en la ruta; rechazo de una carpeta inexistente, de un archivo vacío o mal formado, de una columna, clave o fila faltante, de números y enteros mal escritos, de una especie desconocida, de una etapa fuera de rango, de un límite inferior mayor que el superior, de un fertilizante que no aporta ninguna especie y de un agua que agota la conductividad; matriz de aporte unitario (0,1708 mg/L de N y 0,24429 de Ca por gramo de nitrato de calcio) |
| Solucion | 1 | posición de cada dosis, empleo y número de fertilizantes seleccionados |
| FuncionObjetivo | 2 | dosificación nula (D = D(0), costo cero, K = 0); la intensidad decrece cuando la aptitud crece |
| OperadorReparacion | 6 | cota superior; dominio disjunto de la dosis mínima; escalado de la etapa que excede la conductividad; 200 posiciones al azar con todos los fertilizantes quedan admisibles; retira el fertilizante de menor aporte relativo; desempate por precio y por índice |
| AlgoritmoSSA | 5 | la posición inicial tiene k fertilizantes (o todos, si hay menos) con dosis entre ε y β; rechazo de parámetros no positivos; distancia y promedio de las desviaciones; la misma semilla repite la ejecución, otra semilla da otra, y la mejor solución es admisible |
| Arana | 7 | adopta la vibración más intensa y descarta la de su objetivo; intensidad recibida según la distancia; la máscara no cambia con el contador en cero y se regenera cuando crece; la reflexión devuelve la dosis al intervalo, también en los casos límite; la caminata queda entre cero y la cota de búsqueda |
| Aleatorio | 1 | la misma semilla repite la secuencia; reales, enteros y muestras en su intervalo; semilla de cada repetición |
| ModuloEjecucion | 4 | lee los parámetros del algoritmo y rechaza los inválidos; escribe una fila admisible por repetición, que se repite por separado con la semilla que guarda; tabla por pantalla y rechazo de una salida que no se puede crear |
| Programa | 1 | códigos de salida de `ssa_basico`: 2 sin argumentos, 1 con una instancia inexistente y 0 con una ejecución correcta |

## 9. Mensajes de error frecuentes

| Mensaje | Causa probable |
|---|---|
| `Uso: ssa_basico …` | no se pasaron los tres argumentos |
| `No se pudo abrir datos/…` | el directorio de trabajo no es `SSA_Basico` o la ruta está mal escrita |
| `falta la columna '…'` | el encabezado del CSV no tiene ese nombre exacto |
| `'…' no es un número` | separador decimal con coma, o celda vacía |
| `falta la clave '…'` | falta una fila en `parametros.csv` o en `parametros_ssa.csv` |
| `limites.csv: se espera una fila por especie y etapa` | faltan o sobran filas de límites |
| `límite inferior mayor que el superior en …` | en `limites.csv`, Cmin es mayor que Cmax |
| `… no aporta ninguna especie de la instancia` | un fertilizante no tiene átomos de ninguna especie de `especies.csv` |
| `el agua de partida agota por sí sola el límite de conductividad` | con esa agua, ninguna dosificación es admisible |
