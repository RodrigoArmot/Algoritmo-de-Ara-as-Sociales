#include "SSA.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

AlgoritmoSSA::AlgoritmoSSA(const Instancia& instancia, const ParametrosSSA& parametros)
    : I(instancia), P(parametros), objetivo(instancia), reparacion(instancia) {
    if (P.poblacion < 1 || P.iteraciones < 1)
        throw std::invalid_argument("la población y las iteraciones deben ser positivas");
}

double AlgoritmoSSA::distancia(const std::vector<double>& a, const std::vector<double>& b) {
    double d = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) d += std::fabs(a[i] - b[i]);
    return d;
}

double AlgoritmoSSA::promedioDesviaciones(const std::vector<Vibracion>& v) {
    const std::size_t dimension = v.front().origen.size();
    const double n = static_cast<double>(v.size());
    double suma = 0.0;
    for (std::size_t i = 0; i < dimension; ++i) {
        double media = 0.0;
        for (const auto& x : v) media += x.origen[i];
        media /= n;
        double cuadrados = 0.0;
        for (const auto& x : v) cuadrados += (media - x.origen[i]) * (media - x.origen[i]);
        suma += std::sqrt(cuadrados / n);
    }
    return suma / static_cast<double>(dimension);
}

Solucion AlgoritmoSSA::posicionInicial(Aleatorio& azar) const {
    Solucion x(I);
    for (int f : azar.muestra(I.F, std::min(I.k, I.F)))
        for (int s = 0; s < I.S; ++s) x.dosis(f, s) = azar.uniforme(I.fertilizantes[f].epsilon, I.beta[f * I.S + s]);
    return x;
}

ResultadoSSA AlgoritmoSSA::ejecutar(Aleatorio& azar) const {
    const std::size_t n = P.poblacion;
    ResultadoSSA r;
    r.semilla = azar.semilla();

    // Población inicial: posición al azar y reparada.
    std::vector<Arana> aranas;
    aranas.reserve(n);
    for (std::size_t a = 0; a < n; ++a) {
        Solucion x = posicionInicial(azar);
        reparacion.reparar(x);
        aranas.emplace_back(x);
    }

    std::vector<Vibracion> vibraciones(n);
    std::vector<std::vector<double>> distancias(n, std::vector<double>(n, 0.0));
    bool hayMejor = false;

    for (int iteracion = 0; iteracion < P.iteraciones; ++iteracion) {
        for (std::size_t a = 0; a < n; ++a) {
            Solucion& x = aranas[a].posicion;
            objetivo.evaluar(x);
            ++r.evaluaciones;
            if (!hayMejor || x.Z < r.mejor.Z) {
                r.mejor = x;
                hayMejor = true;
            }
            vibraciones[a].logIntensidad = FuncionObjetivo::logIntensidad(x.Z);
            vibraciones[a].origen = x.q;
        }

        const double sigma = promedioDesviaciones(vibraciones);
        for (std::size_t a = 0; a < n; ++a)
            for (std::size_t b = a + 1; b < n; ++b)
                distancias[a][b] = distancias[b][a] = distancia(vibraciones[a].origen, vibraciones[b].origen);

        for (std::size_t a = 0; a < n; ++a) {
            Arana& arana = aranas[a];
            arana.elegirVibracion(vibraciones, distancias[a], sigma, P.ra);
            arana.cambiarMascara(P.pc, P.pm, azar);
            arana.caminar(vibraciones, I.beta, azar);
            reparacion.reparar(arana.posicion);
        }
        ++r.iteraciones;
    }
    return r;
}
