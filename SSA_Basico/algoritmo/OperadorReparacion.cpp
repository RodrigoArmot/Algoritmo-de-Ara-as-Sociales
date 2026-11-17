#include "OperadorReparacion.h"

#include <vector>

double OperadorReparacion::sumaCationes(const Solucion& x, int s) const {
    double v = 0.0;
    for (int f = 0; f < I.F; ++f) v += I.coefCationes[f] * x.dosis(f, s);
    return I.u + v;
}

double OperadorReparacion::aporteRelativo(const Solucion& x, int f) const {
    double A = 0.0;
    for (int s = 0; s < I.S; ++s)
        for (const Aporte& a : I.aportes[f]) {
            const int i = a.especie * I.S + s;
            if (I.Cmin[i] > 0.0) A += a.alfa * x.dosis(f, s) / I.Cref[i];
        }
    return A;
}

void OperadorReparacion::reparar(Solucion& x) const {
    const int F = I.F, S = I.S;

    // 1. Cota superior y dominio disjunto de la dosis mínima realizable.
    for (int f = 0; f < F; ++f) {
        const double eps = I.fertilizantes[f].epsilon, delta = I.fertilizantes[f].delta;
        for (int s = 0; s < S; ++s) {
            double& q = x.dosis(f, s);
            if (q > delta) q = delta;
            if (q > 0.0 && q < eps) q = q < eps / 2.0 ? 0.0 : eps;
        }
    }

    // 2. Tolerancia de conductividad.
    for (int s = 0; s < S; ++s) {
        double v = 0.0;
        for (int f = 0; f < F; ++f) v += I.coefCationes[f] * x.dosis(f, s);
        if (I.u + v <= I.L) continue;
        const double lambda = (I.L - I.u) / v;
        for (int f = 0; f < F; ++f) {
            double& q = x.dosis(f, s);
            q *= lambda;
            if (q > 0.0 && q < I.fertilizantes[f].epsilon) q = 0.0;
        }
    }

    // 3. Disponibilidad acotada de insumos. El aporte relativo se calcula una vez, antes de retirar.
    // En empate se retira el más caro y, si también empatan, el de mayor índice.
    std::vector<double> A(F, 0.0);
    int adquiridos = 0;
    for (int f = 0; f < F; ++f)
        if (x.seleccionado(f)) {
            A[f] = aporteRelativo(x, f);
            ++adquiridos;
        }
    while (adquiridos > I.k) {
        int retirar = -1;
        for (int f = 0; f < F; ++f) {
            if (!x.seleccionado(f)) continue;
            if (retirar == -1) {
                retirar = f;
                continue;
            }
            const double Pf = I.fertilizantes[f].precio, Pr = I.fertilizantes[retirar].precio;
            if (A[f] < A[retirar] || (A[f] == A[retirar] && (Pf > Pr || (Pf == Pr && f > retirar)))) retirar = f;
        }
        for (int s = 0; s < S; ++s) x.dosis(retirar, s) = 0.0;
        --adquiridos;
    }
}
