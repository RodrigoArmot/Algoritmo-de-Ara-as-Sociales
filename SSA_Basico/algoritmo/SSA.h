#pragma once

#include <cstdint>
#include <vector>

#include "Aleatorio.h"
#include "Arana.h"
#include "FuncionObjetivo.h"
#include "Instancia.h"
#include "OperadorReparacion.h"
#include "Solucion.h"

struct ParametrosSSA {
    int poblacion = 30;
    double ra = 1.0;
    double pc = 0.7;
    double pm = 0.1;
    int iteraciones = 1000;
};

struct ResultadoSSA {
    Solucion mejor;
    int iteraciones = 0;
    long evaluaciones = 0;
    std::uint64_t semilla = 0;
};

class AlgoritmoSSA {
public:
    AlgoritmoSSA(const Instancia& instancia, const ParametrosSSA& parametros);

    ResultadoSSA ejecutar(Aleatorio& azar) const;

    Solucion posicionInicial(Aleatorio& azar) const;

    static double distancia(const std::vector<double>& a, const std::vector<double>& b);
    static double promedioDesviaciones(const std::vector<Vibracion>& vibraciones);

private:
    const Instancia& I;
    ParametrosSSA P;
    FuncionObjetivo objetivo;
    OperadorReparacion reparacion;
};
