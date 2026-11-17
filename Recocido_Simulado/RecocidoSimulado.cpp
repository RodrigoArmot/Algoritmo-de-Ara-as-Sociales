#include "recocido/RecocidoSimulado.h"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

#include "nucleo/PosicionInicial.h"

namespace recocido {
namespace {

// Ec. (6) a (8) sobre una dosis: ρ ~ U(0, Ev), r = q·ρ, q ± r con igual probabilidad, y nunca negativa.
void perturbar(double& q, double Ev, nucleo::Aleatorio& azar) {
    const double rho = azar.uniforme(0.0, Ev);  // ec. (6)
    const double r = q * rho;                   // ec. (7)
    q = azar.uniforme() < 0.5 ? q + r : q - r;  // ec. (8), con igual probabilidad
    if (q < 0.0) q = 0.0;                       // la dosis no puede ser negativa
}

}  // namespace

RecocidoSimulado::RecocidoSimulado(const nucleo::Instancia& instancia, const nucleo::Configuracion& configuracion,
                                   ParametrosRecocido parametros)
    : instancia_(&instancia),
      configuracion_(configuracion),
      parametros_(parametros),
      reparacion_(instancia, configuracion) {
    instancia.validar(configuracion);
    const auto& P = parametros_;
    if (!(P.p > 0.0 && P.p < 1.0)) throw std::invalid_argument("recocido: p debe estar en (0, 1)");
    if (!(P.tf > 0.0)) throw std::invalid_argument("recocido: la temperatura final debe ser positiva");
    if (P.I < 1 || P.n < 1) throw std::invalid_argument("recocido: I y n deben ser al menos 1");
    if (!(P.Ev >= 0.0 && P.Ev <= 1.0)) throw std::invalid_argument("recocido: Ev debe estar en [0, 1]");
    if (!(P.ll >= 0.0 && P.ul >= P.ll)) throw std::invalid_argument("recocido: se necesita 0 ≤ ll ≤ ul");
    if (P.muestrasDelta < 1) throw std::invalid_argument("recocido: se necesita al menos una muestra para Δ");
}

nucleo::Solucion RecocidoSimulado::solucionInicial(nucleo::Aleatorio& azar) const {
    if (parametros_.arranque == ParametrosRecocido::Arranque::PoblacionInicial)
        return nucleo::posicionInicial(*instancia_, configuracion_, azar);
    nucleo::Solucion x(*instancia_);  // ec. (4): q = random(ll, ul) para todos los fertilizantes
    for (double& q : x.q.datos()) q = azar.uniforme(parametros_.ll, parametros_.ul);
    x.derivarBinarias();
    return x;
}

void RecocidoSimulado::vecinaEn(const nucleo::Solucion& actual, double Ev, nucleo::Aleatorio& azar,
                                nucleo::Solucion& candidata) const {
    candidata.q = actual.q;
    auto& dosis = candidata.q.datos();
    if (parametros_.vecino == ParametrosRecocido::Vecino::Uno) {
        const int idx = azar.entero(static_cast<int>(dosis.size()));
        double& q = dosis[static_cast<std::size_t>(idx)];
        if (parametros_.activar && q == 0.0) {
            activarDosis(idx, candidata, azar);
        } else {
            perturbar(q, Ev, azar);
        }
    } else if (parametros_.activar) {
        // Todas las dosis positivas se perturban como en la ec. (8), y una nula, elegida al azar, se activa.
        std::vector<int> ceros;
        for (int i = 0; i < static_cast<int>(dosis.size()); ++i)
            if (dosis[static_cast<std::size_t>(i)] == 0.0) ceros.push_back(i);
        for (double& q : dosis)
            if (q != 0.0) perturbar(q, Ev, azar);
        if (!ceros.empty())
            activarDosis(ceros[static_cast<std::size_t>(azar.entero(static_cast<int>(ceros.size())))], candidata, azar);
    } else {
        for (double& q : dosis) perturbar(q, Ev, azar);
    }
    candidata.derivarBinarias();
    candidata.evaluada = false;
}

void RecocidoSimulado::activarDosis(int idx, nucleo::Solucion& candidata, nucleo::Aleatorio& azar) const {
    const int S = candidata.q.columnas(), f = idx / S, s = idx % S;
    const double eps = instancia_->fertilizante(f).epsilon, b = instancia_->beta(f, s);
    candidata.q(f, s) = azar.uniforme(eps, b > eps ? b : eps);
}

nucleo::Solucion RecocidoSimulado::vecina(const nucleo::Solucion& actual, double Ev, nucleo::Aleatorio& azar) const {
    nucleo::Solucion candidata = actual;
    vecinaEn(actual, Ev, azar, candidata);
    return candidata;
}

double RecocidoSimulado::temperaturaInicial(double Aa, double Ac, double p) {
    return std::fabs(Aa - Ac) / (-std::log(p));
}

double RecocidoSimulado::factorEnfriamiento(double t0, double tf, int I) {
    return std::pow(tf / t0, 1.0 / I);
}

double RecocidoSimulado::perturbacionMaxima(const ParametrosRecocido& P, double t, double t0) {
    if (P.perturbacion == ParametrosRecocido::Perturbacion::Decreciente && t0 > 0.0) return P.Ev * t / t0;
    return P.Ev;
}

bool RecocidoSimulado::acepta(double Aa, double Ac, double t, double U) {
    if (esMejora(Aa, Ac)) return true;
    return std::exp(-(Ac - Aa) / t) > U;
}

ResultadoRecocido RecocidoSimulado::ejecutar(nucleo::Aleatorio& azar) const {
    const ParametrosRecocido& P = parametros_;
    nucleo::FuncionObjetivo objetivo(*instancia_, configuracion_);
    // Sin restricciones la reparación no toca la posición, y las variables derivadas ya las fija la
    // vecina: se omite y el resultado es el mismo.
    const bool conRestricciones =
        configuracion_.dosisMinima || configuracion_.conductividad || configuracion_.cardinalidad;
    auto preparar = [&](nucleo::Solucion& x) {
        if (conRestricciones) reparacion_.reparar(x);
        objetivo.evaluar(x);
    };

    ResultadoRecocido r;
    r.semilla = azar.semilla();

    // Figura 1: solución inicial, una candidata, temperatura inicial y factor de enfriamiento.
    nucleo::Solucion Sa = solucionInicial(azar);
    preparar(Sa);
    nucleo::Solucion Sc(*instancia_);
    vecinaEn(Sa, P.Ev, azar, Sc);
    preparar(Sc);
    if (P.deltaInicial == ParametrosRecocido::DeltaInicial::Media) {
        // Δ como media de |Aa − Ac| sobre muestrasDelta candidatas; la primera decisión sigue siendo
        // sobre la primera de ellas, como con una sola pareja.
        double suma = std::fabs(Sa.Z - Sc.Z);
        nucleo::Solucion otra(*instancia_);
        for (int j = 1; j < P.muestrasDelta; ++j) {
            vecinaEn(Sa, P.Ev, azar, otra);
            preparar(otra);
            suma += std::fabs(Sa.Z - otra.Z);
        }
        r.t0 = suma / P.muestrasDelta / (-std::log(P.p));
    } else {
        r.t0 = temperaturaInicial(Sa.Z, Sc.Z, P.p);
    }
    r.mejor = Sa;

    // Con t0 > tf se recorren los I + 1 niveles t0, t0·α, …, t0·α^I = tf: la figura sigue mientras
    // t ≥ tf. Con t0 ≤ tf la figura termina tras el primer nivel, y aquí también.
    const bool enfriar = r.t0 > P.tf;
    const bool metropolis = P.aceptacion == ParametrosRecocido::Aceptacion::Metropolis;
    r.alfa = enfriar ? factorEnfriamiento(r.t0, P.tf, P.I) : 1.0;
    double t = r.t0;
    bool primeraCandidata = true;  // la primera decisión es sobre la candidata de la temperatura inicial
    for (int nivel = 0;; ++nivel) {
        const double Ev = perturbacionMaxima(P, t, r.t0);
        for (int i = 0; i < P.n; ++i) {
            if (!primeraCandidata) {
                vecinaEn(Sa, Ev, azar, Sc);
                preparar(Sc);
            }
            primeraCandidata = false;
            // El número al azar U solo se sortea si la candidata no mejora, como en la figura.
            if (esMejora(Sa.Z, Sc.Z) || (metropolis && acepta(Sa.Z, Sc.Z, t, azar.uniforme()))) {
                if (Sc.Z > Sa.Z) ++r.aceptadasPeores;
                ++r.aceptadas;
                std::swap(Sa, Sc);
                if (Sa.Z < r.mejor.Z) r.mejor = Sa;
            }
        }
        ++r.temperaturas;
        if (!enfriar || nivel == P.I) break;
        t *= r.alfa;
    }

    r.final = std::move(Sa);
    r.evaluaciones = objetivo.evaluaciones();
    return r;
}

}  // namespace recocido
