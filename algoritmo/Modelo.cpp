#include "Modelo.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace std;

bool seleccionado(const Instancia& instancia, const Solucion& solucion, int fertilizante) {
    for (int etapa = 0; etapa < instancia.numEtapas; ++etapa)
        if (solucion.dosis[fertilizante * instancia.numEtapas + etapa] > 0.0) return true;
    return false;
}

int numSeleccionados(const Instancia& instancia, const Solucion& solucion) {
    int cantidad = 0;
    for (int fertilizante = 0; fertilizante < instancia.numFertilizantes; ++fertilizante)
        cantidad += seleccionado(instancia, solucion, fertilizante);
    return cantidad;
}

double sumaCationes(const Instancia& instancia, const Solucion& solucion, int etapa) {
    double aportado = 0.0;
    for (int fertilizante = 0; fertilizante < instancia.numFertilizantes; ++fertilizante)
        aportado += instancia.coefCationes[fertilizante] * solucion.dosis[fertilizante * instancia.numEtapas + etapa];
    return instancia.cationesAgua + aportado;
}

double aporteRelativo(const Instancia& instancia, const Solucion& solucion, int fertilizante) {
    double total = 0.0;
    for (int etapa = 0; etapa < instancia.numEtapas; ++etapa)
        for (const Aporte& aporte : instancia.aportes[fertilizante]) {
            const int indice = aporte.especie * instancia.numEtapas + etapa;
            if (instancia.concMin[indice] > 0.0)
                total += aporte.aporteUnitario * solucion.dosis[fertilizante * instancia.numEtapas + etapa] /
                         instancia.concRef[indice];
        }
    return total;
}

void evaluar(const Instancia& instancia, Solucion& solucion) {
    const int numEspecies = instancia.numEspecies;
    const int numFertilizantes = instancia.numFertilizantes;
    const int numEtapas = instancia.numEtapas;
    vector<double> concentracion(numEspecies);
    double desviacion = 0.0;
    for (int etapa = 0; etapa < numEtapas; ++etapa) {
        for (int especie = 0; especie < numEspecies; ++especie) concentracion[especie] = instancia.concAgua[especie];
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
            const double dosis = solucion.dosis[fertilizante * numEtapas + etapa];
            if (dosis == 0.0) continue;
            for (const Aporte& aporte : instancia.aportes[fertilizante])
                concentracion[aporte.especie] += aporte.aporteUnitario * dosis;
        }
        for (int especie = 0; especie < numEspecies; ++especie) {
            const int indice = especie * numEtapas + etapa;
            desviacion += (max(0.0, instancia.concMin[indice] - concentracion[especie]) +
                           max(0.0, concentracion[especie] - instancia.concMax[indice])) /
                          instancia.concRef[indice];
        }
    }

    double costo = 0.0;
    for (int etapa = 0; etapa < numEtapas; ++etapa)
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
            costo += solucion.dosis[fertilizante * numEtapas + etapa] / 1000.0 *
                     instancia.fertilizantes[fertilizante].precio;

    solucion.desviacion = desviacion;
    solucion.costo = costo;
    solucion.costoRelativo = (costo - instancia.costoReferencia) / instancia.costoReferencia;
    solucion.objetivo = instancia.pesoDesviacion * solucion.desviacion +
                        (1.0 - instancia.pesoDesviacion) * solucion.costoRelativo;
}

void reparar(const Instancia& instancia, Solucion& solucion) {
    const int numFertilizantes = instancia.numFertilizantes;
    const int numEtapas = instancia.numEtapas;

    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
        const double dosisMin = instancia.fertilizantes[fertilizante].dosisMin;
        const double dosisMax = instancia.fertilizantes[fertilizante].dosisMax;
        for (int etapa = 0; etapa < numEtapas; ++etapa) {
            double& dosis = solucion.dosis[fertilizante * numEtapas + etapa];
            if (dosis > dosisMax) dosis = dosisMax;
            if (dosis > 0.0 && dosis < dosisMin) dosis = dosis < dosisMin / 2.0 ? 0.0 : dosisMin;
        }
    }

    for (int etapa = 0; etapa < numEtapas; ++etapa) {
        double aportado = 0.0;
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
            aportado += instancia.coefCationes[fertilizante] * solucion.dosis[fertilizante * numEtapas + etapa];
        if (instancia.cationesAgua + aportado <= instancia.limiteCationes) continue;
        const double factorReduccion = (instancia.limiteCationes - instancia.cationesAgua) / aportado;
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
            double& dosis = solucion.dosis[fertilizante * numEtapas + etapa];
            dosis *= factorReduccion;
            if (dosis > 0.0 && dosis < instancia.fertilizantes[fertilizante].dosisMin) dosis = 0.0;
        }
    }

    vector<double> aporteRelativoPorFertilizante(numFertilizantes, 0.0);
    int adquiridos = 0;
    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
        if (seleccionado(instancia, solucion, fertilizante)) {
            aporteRelativoPorFertilizante[fertilizante] = aporteRelativo(instancia, solucion, fertilizante);
            ++adquiridos;
        }
    while (adquiridos > instancia.maxFertilizantes) {
        int aRetirar = -1;
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
            if (!seleccionado(instancia, solucion, fertilizante)) continue;
            if (aRetirar == -1) {
                aRetirar = fertilizante;
                continue;
            }
            const double precioActual = instancia.fertilizantes[fertilizante].precio;
            const double precioRetirar = instancia.fertilizantes[aRetirar].precio;
            const double aporteActual = aporteRelativoPorFertilizante[fertilizante];
            const double aporteRetirar = aporteRelativoPorFertilizante[aRetirar];
            if (aporteActual < aporteRetirar ||
                (aporteActual == aporteRetirar &&
                 (precioActual > precioRetirar || (precioActual == precioRetirar && fertilizante > aRetirar))))
                aRetirar = fertilizante;
        }
        for (int etapa = 0; etapa < numEtapas; ++etapa) solucion.dosis[aRetirar * numEtapas + etapa] = 0.0;
        --adquiridos;
    }
}

Solucion posicionInicial(const Instancia& instancia, Aleatorio& azar) {
    Solucion solucion;
    solucion.dosis.assign(instancia.dimension(), 0.0);
    for (int fertilizante : azar.muestra(instancia.numFertilizantes,
                                         min(instancia.maxFertilizantes, instancia.numFertilizantes))) {
        for (int etapa = 0; etapa < instancia.numEtapas; ++etapa) {
            const double limite = instancia.dosisLimite[fertilizante * instancia.numEtapas + etapa];
            if (!isfinite(limite))
                throw runtime_error(instancia.fertilizantes[fertilizante].nombre +
                                    " no aporta ninguna especie de la instancia");
            solucion.dosis[fertilizante * instancia.numEtapas + etapa] =
                azar.uniforme(instancia.fertilizantes[fertilizante].dosisMin, limite);
        }
    }
    return solucion;
}
