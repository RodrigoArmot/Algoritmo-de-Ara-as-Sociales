// Función objetivo (R2, capítulo 4): concentraciones, holguras, desviación D, costo relativo K y
// aptitud Z = γ·D + (1 − γ)·K.
#pragma once

#include <cmath>

#include "Instancia.h"
#include "Solucion.h"

class FuncionObjetivo {
public:
    explicit FuncionObjetivo(const Instancia& instancia) : I(instancia) {}

    // Escribe en la solución el costo, D, K y Z.
    void evaluar(Solucion& x) const;

    // Logaritmo de la intensidad que emite una posición de aptitud Z (Yu y Li, 2015a, ec. 1):
    // I = log(1/(Z − C) + 1), con C = −1, menor que toda aptitud porque D ≥ 0 y K ≥ 0 implican Z ≥ 0.
    static double logIntensidad(double Z) { return std::log(std::log(1.0 / (Z - (-1.0)) + 1.0)); }

private:
    const Instancia& I;
};
