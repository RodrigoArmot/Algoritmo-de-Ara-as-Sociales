// Programa del módulo de ejecución automatizada (R6).
//
// Uso: ssa_basico <carpeta de la instancia> <parametros_ssa.csv> <salida.csv>
// Ejemplo, desde la carpeta SSA_Basico:
//   build/ssa_basico datos/completa datos/parametros_ssa.csv resultados.csv

#include <cstdio>
#include <exception>

#include "Instancia.h"
#include "ModuloEjecucion.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::fputs("Uso: ssa_basico <carpeta de la instancia> <parametros_ssa.csv> <salida.csv>\n", stderr);
        return 2;
    }
    try {
        const Instancia instancia = Instancia::leer(argv[1]);
        const ConfiguracionEjecucion configuracion = ModuloEjecucion::leerConfiguracion(argv[2]);
        ModuloEjecucion::ejecutar(instancia, configuracion, argv[3], stdout);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Error: %s\n", error.what());
        return 1;
    }
}
