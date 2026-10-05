#pragma once

#include <vector>

#include "Instancia.h"

using namespace std;

class Solucion {
public:
    Solucion() = default;
    explicit Solucion(const Instancia& instancia)
        : vectorDosis(instancia.dimension(), 0.0), numEtapas_(instancia.numEtapas) {}

    double& dosis(int fertilizante, int etapa) { return vectorDosis[fertilizante * numEtapas_ + etapa]; }
    double dosis(int fertilizante, int etapa) const { return vectorDosis[fertilizante * numEtapas_ + etapa]; }

    bool seleccionado(int fertilizante) const {
        for (int etapa = 0; etapa < numEtapas_; ++etapa)
            if (vectorDosis[fertilizante * numEtapas_ + etapa] > 0.0) return true;
        return false;
    }

    int numSeleccionados() const {
        int cantidad = 0;
        for (int fertilizante = 0; numEtapas_ > 0 && fertilizante < static_cast<int>(vectorDosis.size()) / numEtapas_;
             ++fertilizante)
            cantidad += seleccionado(fertilizante);
        return cantidad;
    }

    vector<double> vectorDosis;
    double costo = 0.0;
    double desviacion = 0.0;
    double costoRelativo = 0.0;
    double aptitud = 0.0;

private:
    int numEtapas_ = 0;
};
