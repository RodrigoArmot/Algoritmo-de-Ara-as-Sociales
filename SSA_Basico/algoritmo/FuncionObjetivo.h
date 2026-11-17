#pragma once

#include <cmath>

#include "Instancia.h"
#include "Solucion.h"

class FuncionObjetivo {
public:
    explicit FuncionObjetivo(const Instancia& instancia) : I(instancia) {}

    void evaluar(Solucion& x) const;

    static double logIntensidad(double Z) { return std::log(std::log(1.0 / (Z - (-1.0)) + 1.0)); }

private:
    const Instancia& I;
};
