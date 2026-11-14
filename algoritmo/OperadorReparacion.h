// Operador de reparación (R4, capítulo 5). Impone, en este orden y en un solo recorrido:
//   1. la cota superior y el dominio disjunto de la dosis mínima realizable;
//   2. la tolerancia de conductividad, escalando la etapa entera por λ = (L − u)/v;
//   3. la disponibilidad acotada de insumos, retirando el fertilizante de menor aporte relativo
//      hasta quedar en k.
#pragma once

#include "Instancia.h"
#include "Solucion.h"

class OperadorReparacion {
public:
    explicit OperadorReparacion(const Instancia& instancia) : I(instancia) {}

    void reparar(Solucion& x) const;

    // Suma equivalente de cationes de la etapa s, u + v, en meq/L.
    double sumaCationes(const Solucion& x, int s) const;
    // Aporte relativo A_f = Σ_s Σ_{e : Cmin(e,s) > 0} α_(e,f)·q_(f,s)/Cref_(e,s).
    double aporteRelativo(const Solucion& x, int f) const;

private:
    const Instancia& I;
};
