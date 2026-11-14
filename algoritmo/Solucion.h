// Solución candidata (R3): las dosis q_(f,s), en gramos, y su evaluación. El empleo z y la
// selección y no se guardan: se deducen de las dosis, porque un fertilizante está empleado en una
// etapa si su dosis es positiva y seleccionado si lo está en alguna etapa.
#pragma once

#include <vector>

#include "Instancia.h"

class Solucion {
public:
    Solucion() = default;
    explicit Solucion(const Instancia& instancia) : q(instancia.dimension(), 0.0), S_(instancia.S) {}

    double& dosis(int f, int s) { return q[f * S_ + s]; }
    double dosis(int f, int s) const { return q[f * S_ + s]; }

    bool seleccionado(int f) const {
        for (int s = 0; s < S_; ++s)
            if (q[f * S_ + s] > 0.0) return true;
        return false;
    }

    int numSeleccionados() const {
        int n = 0;
        for (int f = 0; S_ > 0 && f < static_cast<int>(q.size()) / S_; ++f) n += seleccionado(f);
        return n;
    }

    std::vector<double> q;  // |F|·|S|, posición f·S + s
    double costo = 0.0;     // USD
    double D = 0.0;         // desviación del requerimiento nutricional
    double K = 0.0;         // costo relativo, costo/Kref
    double Z = 0.0;         // aptitud, γ·D + (1 − γ)·K

private:
    int S_ = 0;
};
