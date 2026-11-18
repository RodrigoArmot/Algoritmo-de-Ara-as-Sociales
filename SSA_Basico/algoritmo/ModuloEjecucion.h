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
    //Desde parametros_ssa.csv
    static ConfiguracionEjecucion leerConfiguracion(const std::string& ruta);

    static std::uint64_t semilla(const Instancia& instancia, int repeticion);

    static std::vector<ResultadoSSA> ejecutar(const Instancia& instancia, const ConfiguracionEjecucion& configuracion,
                                              const std::string& rutaSalida, std::FILE* pantalla);
};
