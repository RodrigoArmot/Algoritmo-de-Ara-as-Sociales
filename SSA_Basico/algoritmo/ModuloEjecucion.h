// Módulo de ejecución automatizada (R6). Ejecuta el algoritmo sobre una instancia tantas veces como
// se pida, cada vez con su propia semilla, y escribe una fila por ejecución en un archivo CSV.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "Instancia.h"
#include "SSA.h"

struct ConfiguracionEjecucion {
    ParametrosSSA parametros;
    int repeticiones = 30;
};

class ModuloEjecucion {
public:
    // Lee parametros_ssa.csv: poblacion, ra, pc, pm, iteraciones y repeticiones.
    static ConfiguracionEjecucion leerConfiguracion(const std::string& ruta);

    // Semilla de la repetición r (desde 1). Depende del nombre de la instancia, de γ y de r, con el
    // mismo texto que el proyecto completo, para que ambos den los mismos resultados.
    static std::uint64_t semilla(const Instancia& instancia, int repeticion);

    // Ejecuta las repeticiones, escribe el CSV y, si pantalla no es nulo, una tabla por pantalla.
    static std::vector<ResultadoSSA> ejecutar(const Instancia& instancia, const ConfiguracionEjecucion& configuracion,
                                              const std::string& rutaSalida, std::FILE* pantalla);
};
