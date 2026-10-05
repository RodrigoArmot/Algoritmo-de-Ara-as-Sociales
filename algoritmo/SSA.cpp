#include "SSA.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace std;

AlgoritmoSSA::AlgoritmoSSA(const Instancia& instancia, const ParametrosSSA& parametros)
    : instancia_(instancia),
      parametros_(parametros),
      funcionObjetivo_(instancia),
      operadorReparacion_(instancia) {
    if (parametros_.poblacion < 1 || parametros_.iteraciones < 1)
        throw invalid_argument("la población y las iteraciones deben ser positivas");
}

double AlgoritmoSSA::distancia(const vector<double>& posicionA, const vector<double>& posicionB) {
    double suma = 0.0;
    for (size_t i = 0; i < posicionA.size(); ++i) suma += fabs(posicionA[i] - posicionB[i]);
    return suma;
}

double AlgoritmoSSA::promedioDesviaciones(const vector<Vibracion>& vibraciones) {
    const size_t dimension = vibraciones.front().origen.size();
    const double cantidad = static_cast<double>(vibraciones.size());
    double suma = 0.0;
    for (size_t i = 0; i < dimension; ++i) {
        double media = 0.0;
        for (const auto& vibracion : vibraciones) media += vibracion.origen[i];
        media /= cantidad;
        double sumaCuadrados = 0.0;
        for (const auto& vibracion : vibraciones)
            sumaCuadrados += (media - vibracion.origen[i]) * (media - vibracion.origen[i]);
        suma += sqrt(sumaCuadrados / cantidad);
    }
    return suma / static_cast<double>(dimension);
}

Solucion AlgoritmoSSA::posicionInicial(Aleatorio& azar) const {
    Solucion solucion(instancia_);
    for (int fertilizante : azar.muestra(instancia_.numFertilizantes,
                                         min(instancia_.maxFertilizantes, instancia_.numFertilizantes)))
        for (int etapa = 0; etapa < instancia_.numEtapas; ++etapa)
            solucion.dosis(fertilizante, etapa) =
                azar.uniforme(instancia_.fertilizantes[fertilizante].dosisMin,
                              instancia_.dosisLimite[fertilizante * instancia_.numEtapas + etapa]);
    return solucion;
}

ResultadoSSA AlgoritmoSSA::ejecutar(Aleatorio& azar) const {
    const size_t numAranas = parametros_.poblacion;
    ResultadoSSA resultado;
    resultado.semilla = azar.semilla();

    vector<Arana> aranas;
    aranas.reserve(numAranas);
    for (size_t i = 0; i < numAranas; ++i) {
        Solucion solucion = posicionInicial(azar);
        operadorReparacion_.reparar(solucion);
        aranas.emplace_back(solucion);
    }

    vector<Vibracion> vibraciones(numAranas);
    vector<vector<double>> distancias(numAranas, vector<double>(numAranas, 0.0));
    bool hayMejor = false;

    for (int iteracion = 0; iteracion < parametros_.iteraciones; ++iteracion) {
        for (size_t indiceArana = 0; indiceArana < numAranas; ++indiceArana) {
            Solucion& solucion = aranas[indiceArana].posicion;
            funcionObjetivo_.evaluar(solucion);
            ++resultado.evaluaciones;
            if (!hayMejor || solucion.aptitud < resultado.mejor.aptitud) {
                resultado.mejor = solucion;
                hayMejor = true;
            }
            vibraciones[indiceArana].logIntensidad = FuncionObjetivo::logIntensidad(solucion.aptitud);
            vibraciones[indiceArana].origen = solucion.vectorDosis;
        }

        const double desviacionPromedio = promedioDesviaciones(vibraciones);
        for (size_t i = 0; i < numAranas; ++i)
            for (size_t j = i + 1; j < numAranas; ++j)
                distancias[i][j] = distancias[j][i] = distancia(vibraciones[i].origen, vibraciones[j].origen);

        for (size_t indiceArana = 0; indiceArana < numAranas; ++indiceArana) {
            Arana& arana = aranas[indiceArana];
            arana.elegirVibracion(vibraciones, distancias[indiceArana], desviacionPromedio,
                                  parametros_.tasaAtenuacion);
            arana.cambiarMascara(parametros_.probCambioMascara, parametros_.probMascara, azar);
            arana.caminar(vibraciones, instancia_.dosisLimite, azar);
            operadorReparacion_.reparar(arana.posicion);
        }
        ++resultado.iteraciones;
    }
    return resultado;
}
