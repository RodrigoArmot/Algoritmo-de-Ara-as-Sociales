#include "Arana.h"

#include <cmath>

Arana::Arana(const Solucion& posicionInicial)
    : posicion(posicionInicial), m(posicionInicial.q.size(), 0.0), mascara(posicionInicial.q.size(), 0) {
    objetivo.origen = posicion.q;
}

double Arana::logIntensidadRecibida(double logOrigen, double d, double sigma, double ra) {
    if (d == 0.0) return logOrigen;
    const double base = sigma * ra;
    if (!(base > 0.0)) return -std::numeric_limits<double>::infinity();
    return logOrigen - d / base;
}

bool Arana::elegirVibracion(const std::vector<Vibracion>& v, const std::vector<double>& d, double sigma,
                            double ra) {
    int elegida = -1;
    double maxima = objetivo.logIntensidad;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (v[i].origen == objetivo.origen) continue;
        const double recibida = logIntensidadRecibida(v[i].logIntensidad, d[i], sigma, ra);
        if (recibida > maxima) {
            elegida = static_cast<int>(i);
            maxima = recibida;
        }
    }
    if (elegida == -1) {
        ++c;
        return false;
    }
    objetivo.logIntensidad = maxima;
    objetivo.origen = v[elegida].origen;
    c = 0;
    return true;
}

bool Arana::cambiarMascara(double pc, double pm, Aleatorio& azar) {
    if (!(azar.uniforme() > std::pow(pc, c))) return false;
    c = 0;
    const double p = pm * azar.uniforme();
    for (auto& bit : mascara) bit = azar.uniforme() < p ? 1 : 0;
    return true;
}

double Arana::reflejar(double x, double q0, double b, Aleatorio& azar) {
    if (x >= 0.0 && x <= b) return x;
    if (!(b > 0.0)) return 0.0;
    const double y = x > b ? (b - q0) * azar.uniforme() : q0 * azar.uniforme();
    return y < 0.0 ? 0.0 : (y > b ? b : y);
}

void Arana::caminar(const std::vector<Vibracion>& v, const std::vector<double>& beta, Aleatorio& azar) {
    std::vector<double>& q = posicion.q;
    const int n = static_cast<int>(v.size());
    for (std::size_t i = 0; i < q.size(); ++i) {
        m[i] *= azar.uniforme();
        const double p = mascara[i] ? v[azar.entero(n)].origen[i] : objetivo.origen[i];
        m[i] += azar.uniforme() * (p - q[i]);
        const double q0 = q[i];
        q[i] = reflejar(q0 + m[i], q0, beta[i], azar);
    }
}
