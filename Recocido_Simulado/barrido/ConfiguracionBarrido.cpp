#include "recocido/barrido/ConfiguracionBarrido.h"

#include <stdexcept>

namespace barrido {
namespace {

using V = ParametrosRecocido::Vecino;
using A = ParametrosRecocido::Aceptacion;
using D = ParametrosRecocido::DeltaInicial;
using E = ParametrosRecocido::Perturbacion;

const std::vector<double> kTFinal = {1.0, 1e-2, 1e-3, 1e-5};
const std::vector<double> kP = {0.5, 0.8, 0.95};
const std::vector<int> kI = {100, 1000, 5000};
const std::vector<int> kN = {10, 100, 500};

std::string textoTFinal(double t) {
    if (t == 1.0) return "1";
    if (t == 1e-2) return "1e-2";
    if (t == 1e-3) return "1e-3";
    if (t == 1e-5) return "1e-5";
    throw std::invalid_argument("barrido: temperatura final fuera de los niveles");
}

std::string textoP(double p) {
    if (p == 0.5) return "0.5";
    if (p == 0.8) return "0.8";
    if (p == 0.95) return "0.95";
    throw std::invalid_argument("barrido: p fuera de los niveles");
}

std::string tokenP(double p) {
    if (p == 0.5) return "p050";
    if (p == 0.8) return "p080";
    if (p == 0.95) return "p095";
    throw std::invalid_argument("barrido: p fuera de los niveles");
}

}  // namespace

std::string ConfiguracionBarrido::nivelVecino() const { return vecino == V::Uno ? "uno" : "todos"; }
std::string ConfiguracionBarrido::nivelAceptacion() const {
    return aceptacion == A::SoloMejoras ? "solo_mejoras" : "metropolis";
}
std::string ConfiguracionBarrido::nivelTFinal() const { return textoTFinal(tFinal); }
std::string ConfiguracionBarrido::nivelP() const { return textoP(p); }
std::string ConfiguracionBarrido::nivelDeltaT0() const { return deltaT0 == D::Media ? "media_100" : "una_pareja"; }
std::string ConfiguracionBarrido::nivelEv() const { return Ev == E::Fija ? "fija" : "decreciente"; }

std::string ConfiguracionBarrido::base() const {
    return nivelVecino() + "-" + (aceptacion == A::SoloMejoras ? "solomejoras" : "metropolis") + "-t" +
           textoTFinal(tFinal) + "-" + tokenP(p) + "-" + (deltaT0 == D::Media ? "media100" : "pareja") + "-" +
           (Ev == E::Fija ? "Evfija" : "Evdec");
}

std::string ConfiguracionBarrido::identificador() const {
    // El orden de los tokens es el de la tabla de factores; I y n van antes de E_v.
    const std::string b = base();
    const auto ultimo = b.rfind('-');
    return b.substr(0, ultimo) + "-I" + std::to_string(I) + "-n" + std::to_string(n) + b.substr(ultimo);
}

ParametrosRecocido ConfiguracionBarrido::parametros() const {
    ParametrosRecocido par;  // ll, ul, Ev inicial y arranque: los publicados
    par.vecino = vecino;
    par.aceptacion = aceptacion;
    par.tf = tFinal;
    par.p = p;
    par.deltaInicial = deltaT0;
    par.muestrasDelta = 100;
    par.I = I;
    par.n = n;
    par.perturbacion = Ev;
    return par;
}

long ConfiguracionBarrido::evaluacionesPorEjecucion() const {
    return static_cast<long>(I + 1) * n + (deltaT0 == D::Media ? 100 : 1);
}

const std::vector<std::string>& nombresFactores() {
    static const std::vector<std::string> nombres = {"vecino", "aceptacion", "t_final", "p",
                                                     "delta_t0", "I", "n", "E_v"};
    return nombres;
}

std::vector<std::string> niveles(const ConfiguracionBarrido& c) {
    return {c.nivelVecino(), c.nivelAceptacion(), c.nivelTFinal(), c.nivelP(),
            c.nivelDeltaT0(), std::to_string(c.I), std::to_string(c.n), c.nivelEv()};
}

std::vector<ConfiguracionBarrido> todas() {
    std::vector<ConfiguracionBarrido> lista;
    for (V vecino : {V::Todos, V::Uno})
        for (A aceptacion : {A::Metropolis, A::SoloMejoras})
            for (double tFinal : kTFinal)
                for (double p : kP)
                    for (D delta : {D::UnaPareja, D::Media})
                        for (int I : kI)
                            for (int n : kN)
                                for (E Ev : {E::Decreciente, E::Fija})
                                    lista.push_back({vecino, aceptacion, tFinal, p, delta, I, n, Ev});
    return lista;
}

std::vector<ConfiguracionBarrido> cribado() {
    std::vector<ConfiguracionBarrido> lista;
    for (const auto& c : todas())
        if ((c.I == 100 || c.I == 1000) && (c.n == 10 || c.n == 100)) lista.push_back(c);
    return lista;
}

ConfiguracionBarrido conIn(ConfiguracionBarrido c, int I, int n) {
    c.I = I;
    c.n = n;
    return c;
}

const std::vector<int>& nivelesI() { return kI; }
const std::vector<int>& nivelesN() { return kN; }

}  // namespace barrido
