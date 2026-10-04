#pragma once

#include <cstdint>

#include "Aleatorio.h"
#include "Instancia.h"
#include "Modelo.h"

using namespace std;

struct ParametrosSSA {
    int poblacion = 30;
    double tasaAtenuacion = 1.0;
    double probCambioMascara = 0.7;
    double probMascara = 0.1;
    int iteraciones = 1000;
};

struct ResultadoSSA {
    Solucion mejor;
    int iteraciones = 0;
    long evaluaciones = 0;
    uint64_t semilla = 0;
};

ResultadoSSA ejecutarSSA(const Instancia& instancia, const ParametrosSSA& parametros, Aleatorio& azar);
