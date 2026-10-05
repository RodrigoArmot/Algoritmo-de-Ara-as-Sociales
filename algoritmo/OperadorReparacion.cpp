#include "OperadorReparacion.h"

#include <vector>

using namespace std;

double OperadorReparacion::sumaCationes(const Solucion& solucion, int etapa) const {
    double aportado = 0.0;
    for (int fertilizante = 0; fertilizante < instancia_.numFertilizantes; ++fertilizante)
        aportado += instancia_.coefCationes[fertilizante] * solucion.dosis(fertilizante, etapa);
    return instancia_.cationesAgua + aportado;
}

double OperadorReparacion::aporteRelativo(const Solucion& solucion, int fertilizante) const {
    double total = 0.0;
    for (int etapa = 0; etapa < instancia_.numEtapas; ++etapa)
        for (const Aporte& aporte : instancia_.aportes[fertilizante]) {
            const int indice = aporte.especie * instancia_.numEtapas + etapa;
            if (instancia_.concMin[indice] > 0.0)
                total += aporte.aporteUnitario * solucion.dosis(fertilizante, etapa) / instancia_.concRef[indice];
        }
    return total;
}

void OperadorReparacion::reparar(Solucion& solucion) const {
    const int numFertilizantes = instancia_.numFertilizantes;
    const int numEtapas = instancia_.numEtapas;

    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
        const double dosisMin = instancia_.fertilizantes[fertilizante].dosisMin;
        const double dosisMax = instancia_.fertilizantes[fertilizante].dosisMax;
        for (int etapa = 0; etapa < numEtapas; ++etapa) {
            double& dosis = solucion.dosis(fertilizante, etapa);
            if (dosis > dosisMax) dosis = dosisMax;
            if (dosis > 0.0 && dosis < dosisMin) dosis = dosis < dosisMin / 2.0 ? 0.0 : dosisMin;
        }
    }

    for (int etapa = 0; etapa < numEtapas; ++etapa) {
        double aportado = 0.0;
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
            aportado += instancia_.coefCationes[fertilizante] * solucion.dosis(fertilizante, etapa);
        if (instancia_.cationesAgua + aportado <= instancia_.limiteCationes) continue;
        const double factorReduccion = (instancia_.limiteCationes - instancia_.cationesAgua) / aportado;
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
            double& dosis = solucion.dosis(fertilizante, etapa);
            dosis *= factorReduccion;
            if (dosis > 0.0 && dosis < instancia_.fertilizantes[fertilizante].dosisMin) dosis = 0.0;
        }
    }

    vector<double> aporteRelativoPorFertilizante(numFertilizantes, 0.0);
    int adquiridos = 0;
    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
        if (solucion.seleccionado(fertilizante)) {
            aporteRelativoPorFertilizante[fertilizante] = aporteRelativo(solucion, fertilizante);
            ++adquiridos;
        }
    while (adquiridos > instancia_.maxFertilizantes) {
        int aRetirar = -1;
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
            if (!solucion.seleccionado(fertilizante)) continue;
            if (aRetirar == -1) {
                aRetirar = fertilizante;
                continue;
            }
            const double precioActual = instancia_.fertilizantes[fertilizante].precio;
            const double precioRetirar = instancia_.fertilizantes[aRetirar].precio;
            const double aporteActual = aporteRelativoPorFertilizante[fertilizante];
            const double aporteRetirar = aporteRelativoPorFertilizante[aRetirar];
            if (aporteActual < aporteRetirar ||
                (aporteActual == aporteRetirar &&
                 (precioActual > precioRetirar || (precioActual == precioRetirar && fertilizante > aRetirar))))
                aRetirar = fertilizante;
        }
        for (int etapa = 0; etapa < numEtapas; ++etapa) solucion.dosis(aRetirar, etapa) = 0.0;
        --adquiridos;
    }
}
