#include "ModuloEjecucion.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

using namespace std;

ConfiguracionEjecucion ModuloEjecucion::leerConfiguracion(const string& ruta) {
    const auto valoresCsv = leerClaveValor(ruta);
    auto valor = [&valoresCsv](const string& clave) {
        const auto it = valoresCsv.find(clave);
        if (it == valoresCsv.end()) throw runtime_error("parametros_ssa.csv: falta la clave '" + clave + "'");
        return it->second;
    };
    ConfiguracionEjecucion configuracion;
    configuracion.parametros.poblacion = aEntero(valor("poblacion"), "poblacion");
    configuracion.parametros.tasaAtenuacion = aReal(valor("ra"), "ra");
    configuracion.parametros.probCambioMascara = aReal(valor("pc"), "pc");
    configuracion.parametros.probMascara = aReal(valor("pm"), "pm");
    configuracion.parametros.iteraciones = aEntero(valor("iteraciones"), "iteraciones");
    configuracion.repeticiones = aEntero(valor("repeticiones"), "repeticiones");
    if (configuracion.parametros.poblacion < 1 || configuracion.parametros.iteraciones < 1 ||
        configuracion.repeticiones < 1)
        throw runtime_error("parametros_ssa.csv: poblacion, iteraciones y repeticiones deben ser positivos");
    return configuracion;
}

uint64_t ModuloEjecucion::semilla(const Instancia& instancia, int repeticion) {
    const string caso = instancia.nombre + "/aranas/gamma=" + to_string(instancia.pesoDesviacion);
    return derivarSemilla(caso, static_cast<uint64_t>(repeticion));
}

vector<ResultadoSSA> ModuloEjecucion::ejecutar(const Instancia& instancia, const ConfiguracionEjecucion& configuracion,
                                               const string& rutaSalida, FILE* pantalla) {
    ofstream salida(rutaSalida);
    if (!salida) throw runtime_error("No se pudo crear " + rutaSalida);
    salida << setprecision(17);
    salida << "repeticion,semilla,Z,D,K,costo,seleccionados,iteraciones,evaluaciones\n";

    const ParametrosSSA& parametros = configuracion.parametros;
    if (pantalla) {
        fprintf(pantalla,
                "%s: %d fertilizantes x %d etapas, gamma = %.2f; poblacion %d, ra %.3g, pc %.3g, pm %.3g; "
                "%d ejecuciones\n",
                instancia.nombre.c_str(), instancia.numFertilizantes, instancia.numEtapas, instancia.pesoDesviacion,
                parametros.poblacion, parametros.tasaAtenuacion, parametros.probCambioMascara,
                parametros.probMascara, configuracion.repeticiones);
        fprintf(pantalla, "%4s %12s %10s %10s %10s %4s %8s %10s\n", "rep", "Z mejor", "D", "costo", "K", "y",
                "iter", "evals");
    }

    const AlgoritmoSSA algoritmo(instancia, parametros);
    vector<ResultadoSSA> resultados;
    for (int repeticion = 1; repeticion <= configuracion.repeticiones; ++repeticion) {
        Aleatorio azar(semilla(instancia, repeticion));
        resultados.push_back(algoritmo.ejecutar(azar));
        const ResultadoSSA& resultado = resultados.back();
        const Solucion& mejor = resultado.mejor;
        const int seleccionados = mejor.numSeleccionados();
        if (pantalla)
            fprintf(pantalla, "%4d %12.6f %10.4f %10.4f %10.4f %4d %8d %10ld\n", repeticion, mejor.aptitud,
                    mejor.desviacion, mejor.costo, mejor.costoRelativo, seleccionados, resultado.iteraciones,
                    resultado.evaluaciones);
        salida << repeticion << ',' << resultado.semilla << ',' << mejor.aptitud << ',' << mejor.desviacion << ','
               << mejor.costoRelativo << ',' << mejor.costo << ',' << seleccionados << ',' << resultado.iteraciones
               << ',' << resultado.evaluaciones << '\n';
    }
    return resultados;
}
