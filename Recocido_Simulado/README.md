# Recocido_Simulado

Implementación del recocido simulado de Guerra Ibarra et al. con el que se compara el Algoritmo de Arañas Sociales en el capítulo 7. Es la carpeta `recocido` del proyecto completo de formulación, donde se compila dentro del programa `formulacion` y se ejecuta con `formulacion recocido`.

| Archivo | Contenido |
|---|---|
| `ParametrosRecocido.h` | Parámetros de control (dosis inicial, perturbación, probabilidad inicial de aceptación, temperatura final, estados de energía y candidatas por temperatura) y variantes de vecindad, aceptación, perturbación y activación de dosis |
| `RecocidoSimulado.h`, `RecocidoSimulado.cpp` | Algoritmo: solución inicial, vecino, aceptación y enfriamiento |
| `barrido/` | Barrido de parámetros con que se reprodujo la Tabla 5 de Guerra Ibarra et al. |
| `pruebas/` | Pruebas unitarias con Google Test |
| `CMakeLists.txt` | Bibliotecas `recocido` y `barrido` y sus pruebas |

La carpeta no compila sola: usa la instancia, la función objetivo y el operador de reparación del núcleo del proyecto completo (`nucleo/`), que no se incluye.
