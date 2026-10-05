#include <cstdio>
#include <exception>
#include <string>

#include "Instancia.h"
#include "ModuloEjecucion.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 1 && argc != 4) {
        fputs("Uso: ssa_basico [<carpeta de la instancia> <parametros_ssa.csv> <salida.csv>]\n", stderr);
        return 2;
    }
    const string carpetaInstancia = argc == 4 ? argv[1] : string(DIR_DATOS) + "/completa";
    const string archivoParametros = argc == 4 ? argv[2] : string(DIR_DATOS) + "/parametros_ssa.csv";
    const string archivoSalida = argc == 4 ? argv[3] : string(DIR_DATOS) + "/resultados.csv";
    try {
        const Instancia instancia = Instancia::leer(carpetaInstancia);
        const ConfiguracionEjecucion configuracion = ModuloEjecucion::leerConfiguracion(archivoParametros);
        ModuloEjecucion::ejecutar(instancia, configuracion, archivoSalida, stdout);
        return 0;
    } catch (const exception& error) {
        fprintf(stderr, "Error: %s\n", error.what());
        return 1;
    }
}
