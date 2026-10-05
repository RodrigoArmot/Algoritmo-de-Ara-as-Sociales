#include "Arana.h"

#include <cmath>

using namespace std;

Arana::Arana(const Solucion& posicionInicial)
    : posicion(posicionInicial),
      movimiento(posicionInicial.vectorDosis.size(), 0.0),
      mascara(posicionInicial.vectorDosis.size(), 0) {
    vibracionObjetivo.origen = posicion.vectorDosis;
}

double Arana::logIntensidadRecibida(double logIntensidadOrigen, double distanciaAlOrigen,
                                    double desviacionPromedio, double tasaAtenuacion) {
    if (distanciaAlOrigen == 0.0) return logIntensidadOrigen;
    const double escala = desviacionPromedio * tasaAtenuacion;
    if (!(escala > 0.0)) return -numeric_limits<double>::infinity();
    return logIntensidadOrigen - distanciaAlOrigen / escala;
}

bool Arana::elegirVibracion(const vector<Vibracion>& vibraciones, const vector<double>& distancias,
                            double desviacionPromedio, double tasaAtenuacion) {
    int elegida = -1;
    double maxima = vibracionObjetivo.logIntensidad;
    for (size_t i = 0; i < vibraciones.size(); ++i) {
        if (vibraciones[i].origen == vibracionObjetivo.origen) continue;
        const double recibida =
            logIntensidadRecibida(vibraciones[i].logIntensidad, distancias[i], desviacionPromedio, tasaAtenuacion);
        if (recibida > maxima) {
            elegida = static_cast<int>(i);
            maxima = recibida;
        }
    }
    if (elegida == -1) {
        ++iteracionesSinCambio;
        return false;
    }
    vibracionObjetivo.logIntensidad = maxima;
    vibracionObjetivo.origen = vibraciones[elegida].origen;
    iteracionesSinCambio = 0;
    return true;
}

bool Arana::cambiarMascara(double probCambioMascara, double probMascara, Aleatorio& azar) {
    if (!(azar.uniforme() > pow(probCambioMascara, iteracionesSinCambio))) return false;
    iteracionesSinCambio = 0;
    const double probBit = probMascara * azar.uniforme();
    for (auto& bit : mascara) bit = azar.uniforme() < probBit ? 1 : 0;
    return true;
}

double Arana::reflejar(double nuevoValor, double valorAnterior, double cota, Aleatorio& azar) {
    if (nuevoValor >= 0.0 && nuevoValor <= cota) return nuevoValor;
    if (!(cota > 0.0)) return 0.0;
    const double reflejado = nuevoValor > cota ? (cota - valorAnterior) * azar.uniforme()
                                               : valorAnterior * azar.uniforme();
    return reflejado < 0.0 ? 0.0 : (reflejado > cota ? cota : reflejado);
}

void Arana::caminar(const vector<Vibracion>& vibraciones, const vector<double>& dosisLimite, Aleatorio& azar) {
    vector<double>& dosis = posicion.vectorDosis;
    const int cantidadVibraciones = static_cast<int>(vibraciones.size());
    for (size_t i = 0; i < dosis.size(); ++i) {
        movimiento[i] *= azar.uniforme();
        const double puntoAtraccion = mascara[i] ? vibraciones[azar.entero(cantidadVibraciones)].origen[i]
                                                 : vibracionObjetivo.origen[i];
        movimiento[i] += azar.uniforme() * (puntoAtraccion - dosis[i]);
        const double dosisAnterior = dosis[i];
        dosis[i] = reflejar(dosisAnterior + movimiento[i], dosisAnterior, dosisLimite[i], azar);
    }
}
