#pragma once

#include <limits>
#include <vector>

#include "Aleatorio.h"
#include "Solucion.h"

struct Vibracion {
    double logIntensidad = -std::numeric_limits<double>::infinity();
    std::vector<double> origen;
};

class Arana {
public:
    explicit Arana(const Solucion& posicionInicial);

    bool elegirVibracion(const std::vector<Vibracion>& vibraciones, const std::vector<double>& distancias,
                         double sigma, double ra);

    bool cambiarMascara(double pc, double pm, Aleatorio& azar);

    void caminar(const std::vector<Vibracion>& vibraciones, const std::vector<double>& beta, Aleatorio& azar);

    static double logIntensidadRecibida(double logOrigen, double d, double sigma, double ra);

    static double reflejar(double x, double q0, double b, Aleatorio& azar);

    Solucion posicion;
    Vibracion objetivo;
    int c = 0;
    std::vector<double> m;
    std::vector<unsigned char> mascara;
};
