#pragma once

#include <limits>
#include <vector>

#include "Aleatorio.h"
#include "Solucion.h"

using namespace std;

struct Vibracion {
    double logIntensidad = -numeric_limits<double>::infinity();
    vector<double> origen;
};

class Arana {
public:

    explicit Arana(const Solucion& posicionInicial);

    bool elegirVibracion(const vector<Vibracion>& vibraciones, const vector<double>& distancias,
                         double desviacionPromedio, double tasaAtenuacion);

    bool cambiarMascara(double probCambioMascara, double probMascara, Aleatorio& azar);

    void caminar(const vector<Vibracion>& vibraciones, const vector<double>& dosisLimite, Aleatorio& azar);

    static double logIntensidadRecibida(double logIntensidadOrigen, double distanciaAlOrigen,
                                        double desviacionPromedio, double tasaAtenuacion);

    static double reflejar(double nuevoValor, double valorAnterior, double cota, Aleatorio& azar);

    Solucion posicion;
    Vibracion vibracionObjetivo;
    int iteracionesSinCambio = 0;
    vector<double> movimiento;
    vector<unsigned char> mascara;
};
