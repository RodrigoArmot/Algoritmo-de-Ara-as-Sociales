#pragma once

#include "Instancia.h"
#include "Solucion.h"

using namespace std;

class OperadorReparacion {
public:
    explicit OperadorReparacion(const Instancia& instancia) : instancia_(instancia) {}

    void reparar(Solucion& solucion) const;

    double sumaCationes(const Solucion& solucion, int etapa) const;
    double aporteRelativo(const Solucion& solucion, int fertilizante) const;

private:
    const Instancia& instancia_;
};
