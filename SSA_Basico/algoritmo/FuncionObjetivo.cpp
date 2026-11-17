#include "FuncionObjetivo.h"

#include <algorithm>
#include <vector>

void FuncionObjetivo::evaluar(Solucion& x) const {
    const int E = I.E, F = I.F, S = I.S;
    std::vector<double> C(E);
    double D = 0.0;
    for (int s = 0; s < S; ++s) {
        for (int e = 0; e < E; ++e) C[e] = I.C0[e];
        for (int f = 0; f < F; ++f) {
            const double q = x.dosis(f, s);
            if (q == 0.0) continue;
            for (const Aporte& a : I.aportes[f]) C[a.especie] += a.alfa * q;
        }
        for (int e = 0; e < E; ++e) {
            const int i = e * S + s;
            D += (std::max(0.0, I.Cmin[i] - C[e]) + std::max(0.0, C[e] - I.Cmax[i])) / I.Cref[i];
        }
    }

    double costo = 0.0;
    for (int s = 0; s < S; ++s)
        for (int f = 0; f < F; ++f) costo += x.dosis(f, s) / 1000.0 * I.fertilizantes[f].precio;

    x.D = D;
    x.costo = costo;
    x.K = costo / I.Kref;
    x.Z = I.gamma * x.D + (1.0 - I.gamma) * x.K;
}
