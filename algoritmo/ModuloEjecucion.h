#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "Instancia.h"
#include "SSA.h"

using namespace std;

struct ConfiguracionEjecucion {
    ParametrosSSA parametros;
    int repeticiones = 30;
};

class ModuloEjecucion {
public:
    static ConfiguracionEjecucion leerConfiguracion(const string& ruta);

    static uint64_t semilla(const Instancia& instancia, int repeticion);

    static vector<ResultadoSSA> ejecutar(const Instancia& instancia, const ConfiguracionEjecucion& configuracion,
                                         const string& rutaSalida, FILE* pantalla);
};
