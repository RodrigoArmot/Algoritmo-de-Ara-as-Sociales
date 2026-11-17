#pragma once

#include "Instancia.h"
#include "Solucion.h"

class OperadorReparacion {
public:
    explicit OperadorReparacion(const Instancia& instancia) : I(instancia) {}

    void reparar(Solucion& x) const;
    double sumaCationes(const Solucion& x, int s) const;
    double aporteRelativo(const Solucion& x, int f) const;

private:
    const Instancia& I;
};
