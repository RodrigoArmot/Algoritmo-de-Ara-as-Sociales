#pragma once

#include <cmath>

#include "Instancia.h"
#include "Solucion.h"

using namespace std;

class FuncionObjetivo {
public:
    explicit FuncionObjetivo(const Instancia& instancia) : instancia_(instancia) {}

    void evaluar(Solucion& solucion) const;

    static double logIntensidad(double aptitud) { return log(log(1.0 / (aptitud - (-1.0)) + 1.0)); }

private:
    const Instancia& instancia_;
};
