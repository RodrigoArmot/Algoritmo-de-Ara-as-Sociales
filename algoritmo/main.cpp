#include <cstdio>
#include <exception>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>

#include "Aleatorio.h"
#include "Instancia.h"
#include "SSA.h"

using namespace std;

int main(int argc, char* argv[]) {
    // Sin argumentos usa la carpeta datos/ del proyecto; opcionalmente se pueden indicar las tres rutas.
    if (argc != 1 && argc != 4) {
        fputs("Uso: ssa_basico [<carpeta de la instancia> <parametros_ssa.csv> <salida.csv>]\n", stderr);
        return 2;
    }
    const string carpetaInstancia = argc == 4 ? argv[1] : string(DIR_DATOS) + "/completa";
    const string archivoParametros = argc == 4 ? argv[2] : string(DIR_DATOS) + "/parametros_ssa.csv";
    const string archivoSalida = argc == 4 ? argv[3] : string(DIR_DATOS) + "/resultados.csv";
    try {
        const Instancia instancia = leerInstancia(carpetaInstancia);

        const auto valoresCsv = leerClaveValor(archivoParametros);
        auto valor = [&valoresCsv](const string& clave) {
            const auto it = valoresCsv.find(clave);
            if (it == valoresCsv.end()) throw runtime_error("parametros_ssa.csv: falta la clave '" + clave + "'");
            return it->second;
        };
        ParametrosSSA parametros;
        parametros.poblacion = aEntero(valor("poblacion"), "poblacion");
        parametros.tasaAtenuacion = aReal(valor("ra"), "ra");
        parametros.probCambioMascara = aReal(valor("pc"), "pc");
        parametros.probMascara = aReal(valor("pm"), "pm");
        parametros.iteraciones = aEntero(valor("iteraciones"), "iteraciones");
        const int repeticiones = aEntero(valor("repeticiones"), "repeticiones");
        if (parametros.poblacion < 1 || parametros.iteraciones < 1 || repeticiones < 1)
            throw runtime_error("parametros_ssa.csv: poblacion, iteraciones y repeticiones deben ser positivos");

        ofstream salida(archivoSalida);
        if (!salida) throw runtime_error("No se pudo crear " + archivoSalida);
        salida << setprecision(17);
        salida << "repeticion,semilla,Z,D,K,costo,seleccionados,iteraciones,evaluaciones\n";

        printf("%s: %d fertilizantes x %d etapas, gamma = %.2f; poblacion %d, ra %.3g, pc %.3g, pm %.3g; "
               "%d ejecuciones\n",
               instancia.nombre.c_str(), instancia.numFertilizantes, instancia.numEtapas, instancia.pesoDesviacion,
               parametros.poblacion, parametros.tasaAtenuacion, parametros.probCambioMascara, parametros.probMascara,
               repeticiones);
        printf("%4s %12s %10s %10s %10s %4s %8s %10s\n", "rep", "Z mejor", "D", "costo", "K", "y", "iter", "evals");

        const string caso = instancia.nombre + "/aranas/gamma=" + to_string(instancia.pesoDesviacion);
        for (int repeticion = 1; repeticion <= repeticiones; ++repeticion) {
            Aleatorio azar(derivarSemilla(caso, static_cast<uint64_t>(repeticion)));
            const ResultadoSSA resultado = ejecutarSSA(instancia, parametros, azar);
            const Solucion& mejor = resultado.mejor;
            const int seleccionados = numSeleccionados(instancia, mejor);
            printf("%4d %12.6f %10.4f %10.4f %10.4f %4d %8d %10ld\n", repeticion, mejor.objetivo, mejor.desviacion,
                   mejor.costo, mejor.costoRelativo, seleccionados, resultado.iteraciones, resultado.evaluaciones);
            salida << repeticion << ',' << resultado.semilla << ',' << mejor.objetivo << ',' << mejor.desviacion
                   << ',' << mejor.costoRelativo << ',' << mejor.costo << ',' << seleccionados << ','
                   << resultado.iteraciones << ',' << resultado.evaluaciones << '\n';
        }
        return 0;
    } catch (const exception& error) {
        fprintf(stderr, "Error: %s\n", error.what());
        return 1;
    }
}
