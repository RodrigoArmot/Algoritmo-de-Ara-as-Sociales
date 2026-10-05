#pragma once

#include <cstdint>
#include <vector>

#include "Aleatorio.h"
#include "Arana.h"
#include "FuncionObjetivo.h"
#include "Instancia.h"
#include "OperadorReparacion.h"
#include "Solucion.h"

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

class AlgoritmoSSA {
public:
    AlgoritmoSSA(const Instancia& instancia, const ParametrosSSA& parametros);

    ResultadoSSA ejecutar(Aleatorio& azar) const;

    Solucion posicionInicial(Aleatorio& azar) const;

    static double distancia(const vector<double>& posicionA, const vector<double>& posicionB);
    static double promedioDesviaciones(const vector<Vibracion>& vibraciones);

private:
    const Instancia& instancia_;
    ParametrosSSA parametros_;
    FuncionObjetivo funcionObjetivo_;
    OperadorReparacion operadorReparacion_;
};
