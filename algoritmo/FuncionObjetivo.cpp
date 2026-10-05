#include "FuncionObjetivo.h"

#include <algorithm>
#include <vector>

using namespace std;

void FuncionObjetivo::evaluar(Solucion& solucion) const {
    const int numEspecies = instancia_.numEspecies;
    const int numFertilizantes = instancia_.numFertilizantes;
    const int numEtapas = instancia_.numEtapas;
    vector<double> concentracion(numEspecies);
    double desviacion = 0.0;
    for (int etapa = 0; etapa < numEtapas; ++etapa) {
        for (int especie = 0; especie < numEspecies; ++especie) concentracion[especie] = instancia_.concAgua[especie];
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
            const double dosis = solucion.dosis(fertilizante, etapa);
            if (dosis == 0.0) continue;
            for (const Aporte& aporte : instancia_.aportes[fertilizante])
                concentracion[aporte.especie] += aporte.aporteUnitario * dosis;
        }
        for (int especie = 0; especie < numEspecies; ++especie) {
            const int indice = especie * numEtapas + etapa;
            desviacion += (max(0.0, instancia_.concMin[indice] - concentracion[especie]) +
                           max(0.0, concentracion[especie] - instancia_.concMax[indice])) /
                          instancia_.concRef[indice];
        }
    }

    double costo = 0.0;
    for (int etapa = 0; etapa < numEtapas; ++etapa)
        for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
            costo += solucion.dosis(fertilizante, etapa) / 1000.0 * instancia_.fertilizantes[fertilizante].precio;

    solucion.desviacion = desviacion;
    solucion.costo = costo;
    solucion.costoRelativo = (costo - instancia_.costoReferencia) / instancia_.costoReferencia;
    solucion.aptitud = instancia_.pesoDesviacion * solucion.desviacion +
                       (1.0 - instancia_.pesoDesviacion) * solucion.costoRelativo;
}
