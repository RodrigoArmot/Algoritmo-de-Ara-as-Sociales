#include "ModuloEjecucion.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

ConfiguracionEjecucion ModuloEjecucion::leerConfiguracion(const std::string& ruta) {
    const auto p = leerClaveValor(ruta);
    auto valor = [&p](const std::string& clave) {
        const auto it = p.find(clave);
        if (it == p.end()) throw std::runtime_error("parametros_ssa.csv: falta la clave '" + clave + "'");
        return it->second;
    };
    ConfiguracionEjecucion c;
    c.parametros.poblacion = aEntero(valor("poblacion"), "poblacion");
    c.parametros.ra = aReal(valor("ra"), "ra");
    c.parametros.pc = aReal(valor("pc"), "pc");
    c.parametros.pm = aReal(valor("pm"), "pm");
    c.parametros.iteraciones = aEntero(valor("iteraciones"), "iteraciones");
    c.repeticiones = aEntero(valor("repeticiones"), "repeticiones");
    if (c.parametros.poblacion < 1 || c.parametros.iteraciones < 1 || c.repeticiones < 1)
        throw std::runtime_error("parametros_ssa.csv: poblacion, iteraciones y repeticiones deben ser positivos");
    return c;
}

std::uint64_t ModuloEjecucion::semilla(const Instancia& instancia, int repeticion) {
    const std::string caso = instancia.nombre + "/aranas/gamma=" + std::to_string(instancia.gamma);
    return derivarSemilla(caso, static_cast<std::uint64_t>(repeticion));
}

std::vector<ResultadoSSA> ModuloEjecucion::ejecutar(const Instancia& I, const ConfiguracionEjecucion& c,
                                                    const std::string& rutaSalida, std::FILE* pantalla) {
    std::ofstream salida(rutaSalida);
    if (!salida) throw std::runtime_error("No se pudo crear " + rutaSalida);
    salida << std::setprecision(17);
    salida << "repeticion,semilla,Z,D,K,costo,seleccionados,iteraciones,evaluaciones\n";

    const ParametrosSSA& P = c.parametros;
    if (pantalla) {
        std::fprintf(pantalla,
                     "%s: %d fertilizantes x %d etapas, gamma = %.2f; poblacion %d, ra %.3g, pc %.3g, pm %.3g; "
                     "%d ejecuciones\n",
                     I.nombre.c_str(), I.F, I.S, I.gamma, P.poblacion, P.ra, P.pc, P.pm, c.repeticiones);
        std::fprintf(pantalla, "%4s %12s %10s %10s %10s %4s %8s %10s\n", "rep", "Z mejor", "D", "costo", "K", "y",
                     "iter", "evals");
    }

    const AlgoritmoSSA algoritmo(I, P);
    std::vector<ResultadoSSA> resultados;
    for (int rep = 1; rep <= c.repeticiones; ++rep) {
        Aleatorio azar(semilla(I, rep));
        resultados.push_back(algoritmo.ejecutar(azar));
        const ResultadoSSA& r = resultados.back();
        const Solucion& x = r.mejor;
        const int y = x.numSeleccionados();
        if (pantalla)
            std::fprintf(pantalla, "%4d %12.6f %10.4f %10.4f %10.4f %4d %8d %10ld\n", rep, x.Z, x.D, x.costo, x.K, y,
                         r.iteraciones, r.evaluaciones);
        salida << rep << ',' << r.semilla << ',' << x.Z << ',' << x.D << ',' << x.K << ',' << x.costo << ',' << y
               << ',' << r.iteraciones << ',' << r.evaluaciones << '\n';
    }
    return resultados;
}
