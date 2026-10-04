#include "SSA.h"

#include <cmath>
#include <limits>
#include <vector>

using namespace std;

namespace {

const double MENOS_INFINITO = -numeric_limits<double>::infinity();

struct Vibracion {
    double logIntensidad = MENOS_INFINITO;
    vector<double> origen;
};

struct Arana {
    Solucion posicion;
    Vibracion vibracionObjetivo;
    int iteracionesSinCambio = 0;
    vector<double> movimiento;
    vector<unsigned char> mascara;
};

double logIntensidad(double valorObjetivo) { return log(log(1.0 / (valorObjetivo - (-1.0)) + 1.0)); }

double distancia(const vector<double>& origenA, const vector<double>& origenB) {
    double suma = 0.0;
    for (size_t i = 0; i < origenA.size(); ++i) suma += fabs(origenA[i] - origenB[i]);
    return suma;
}

double promedioDesviaciones(const vector<Vibracion>& vibraciones) {
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

double logIntensidadRecibida(double logIntensidadOrigen, double distanciaAlOrigen, double desviacionPromedio,
                             double tasaAtenuacion) {
    if (distanciaAlOrigen == 0.0) return logIntensidadOrigen;
    const double escala = desviacionPromedio * tasaAtenuacion;
    if (!(escala > 0.0)) return MENOS_INFINITO;
    return logIntensidadOrigen - distanciaAlOrigen / escala;
}

void elegirVibracion(Arana& arana, const vector<Vibracion>& vibraciones, const vector<double>& distancias,
                     double desviacionPromedio, double tasaAtenuacion) {
    int elegida = -1;
    double maxima = arana.vibracionObjetivo.logIntensidad;
    for (size_t i = 0; i < vibraciones.size(); ++i) {
        if (vibraciones[i].origen == arana.vibracionObjetivo.origen) continue;
        const double recibida =
            logIntensidadRecibida(vibraciones[i].logIntensidad, distancias[i], desviacionPromedio, tasaAtenuacion);
        if (recibida > maxima) {
            elegida = static_cast<int>(i);
            maxima = recibida;
        }
    }
    if (elegida == -1) {
        ++arana.iteracionesSinCambio;
        return;
    }
    arana.vibracionObjetivo.logIntensidad = maxima;
    arana.vibracionObjetivo.origen = vibraciones[elegida].origen;
    arana.iteracionesSinCambio = 0;
}

void cambiarMascara(Arana& arana, double probCambioMascara, double probMascara, Aleatorio& azar) {
    if (!(azar.uniforme() > pow(probCambioMascara, arana.iteracionesSinCambio))) return;
    arana.iteracionesSinCambio = 0;
    const double probBit = probMascara * azar.uniforme();
    for (auto& bit : arana.mascara) bit = azar.uniforme() < probBit ? 1 : 0;
}

double reflejar(double nuevoValor, double valorAnterior, double cota, Aleatorio& azar) {
    if (nuevoValor >= 0.0 && nuevoValor <= cota) return nuevoValor;
    if (!(cota > 0.0)) return 0.0;
    const double reflejado = nuevoValor > cota ? (cota - valorAnterior) * azar.uniforme()
                                               : valorAnterior * azar.uniforme();
    return reflejado < 0.0 ? 0.0 : (reflejado > cota ? cota : reflejado);
}

void caminar(Arana& arana, const vector<Vibracion>& vibraciones, const vector<double>& dosisLimite,
             Aleatorio& azar) {
    vector<double>& dosis = arana.posicion.dosis;
    const int cantidadVibraciones = static_cast<int>(vibraciones.size());
    for (size_t i = 0; i < dosis.size(); ++i) {
        arana.movimiento[i] *= azar.uniforme();
        const double puntoAtraccion = arana.mascara[i] ? vibraciones[azar.entero(cantidadVibraciones)].origen[i]
                                                       : arana.vibracionObjetivo.origen[i];
        arana.movimiento[i] += azar.uniforme() * (puntoAtraccion - dosis[i]);
        const double dosisAnterior = dosis[i];
        dosis[i] = reflejar(dosisAnterior + arana.movimiento[i], dosisAnterior, dosisLimite[i], azar);
    }
}

}  // namespace

ResultadoSSA ejecutarSSA(const Instancia& instancia, const ParametrosSSA& parametros, Aleatorio& azar) {
    const size_t numAranas = parametros.poblacion;
    ResultadoSSA resultado;
    resultado.semilla = azar.semilla();

    vector<Arana> aranas(numAranas);
    for (Arana& arana : aranas) {
        arana.posicion = posicionInicial(instancia, azar);
        reparar(instancia, arana.posicion);
        arana.vibracionObjetivo.origen = arana.posicion.dosis;
        arana.movimiento.assign(instancia.dimension(), 0.0);
        arana.mascara.assign(instancia.dimension(), 0);
    }

    vector<Vibracion> vibraciones(numAranas);
    vector<vector<double>> distancias(numAranas, vector<double>(numAranas, 0.0));
    bool hayMejor = false;

    for (int iteracion = 0; iteracion < parametros.iteraciones; ++iteracion) {
        for (size_t indiceArana = 0; indiceArana < numAranas; ++indiceArana) {
            Solucion& solucion = aranas[indiceArana].posicion;
            evaluar(instancia, solucion);
            ++resultado.evaluaciones;
            if (!hayMejor || solucion.objetivo < resultado.mejor.objetivo) {
                resultado.mejor = solucion;
                hayMejor = true;
            }
            vibraciones[indiceArana].logIntensidad = logIntensidad(solucion.objetivo);
            vibraciones[indiceArana].origen = solucion.dosis;
        }

        const double desviacionPromedio = promedioDesviaciones(vibraciones);
        for (size_t i = 0; i < numAranas; ++i)
            for (size_t j = i + 1; j < numAranas; ++j)
                distancias[i][j] = distancias[j][i] = distancia(vibraciones[i].origen, vibraciones[j].origen);

        for (size_t indiceArana = 0; indiceArana < numAranas; ++indiceArana) {
            Arana& arana = aranas[indiceArana];
            elegirVibracion(arana, vibraciones, distancias[indiceArana], desviacionPromedio,
                            parametros.tasaAtenuacion);
            cambiarMascara(arana, parametros.probCambioMascara, parametros.probMascara, azar);
            caminar(arana, vibraciones, instancia.dosisLimite, azar);
            reparar(instancia, arana.posicion);
        }
        ++resultado.iteraciones;
    }
    return resultado;
}
