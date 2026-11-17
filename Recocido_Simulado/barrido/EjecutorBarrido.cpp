#include "recocido/barrido/EjecutorBarrido.h"

#include <stdexcept>

#include "nucleo/Aleatorio.h"
#include "nucleo/Semilla.h"
#include "recocido/RecocidoSimulado.h"

namespace barrido {

std::string textoGamma(double gamma) {
    if (gamma == 0.65) return "0.65";
    if (gamma == 0.75) return "0.75";
    if (gamma == 0.85) return "0.85";
    throw std::invalid_argument("barrido: γ fuera de los del experimento");
}

const std::vector<double>& EjecutorBarrido::gammas() {
    static const std::vector<double> g = {0.65, 0.75, 0.85};
    return g;
}

EjecutorBarrido::EjecutorBarrido(const nucleo::ProblemaCargado& experimento1)
    : configuracion_(experimento1.configuracion) {
    for (double gamma : gammas()) {
        instancias_.push_back(experimento1.instancia);
        instancias_.back().fijarGamma(gamma);
    }
}

std::uint64_t EjecutorBarrido::semilla(const std::string& fase, const std::string& identificador, double gamma,
                                       int repeticion) {
    return nucleo::semilla::derivar(fase + "/" + identificador + "/gamma=" + textoGamma(gamma),
                                    static_cast<std::uint64_t>(repeticion));
}

Fila EjecutorBarrido::ejecutarUna(const ConfiguracionBarrido& configuracion, const std::string& fase,
                                  std::size_t gamma, int repeticion) const {
    const recocido::RecocidoSimulado sa(instancias_.at(gamma), configuracion_, configuracion.parametros());
    Fila f;
    f.fase = fase;
    f.configuracion = configuracion;
    f.gamma = gammas()[gamma];
    f.repeticion = repeticion;
    f.semilla = semilla(fase, configuracion.identificador(), f.gamma, repeticion);
    nucleo::Aleatorio azar(f.semilla);
    const auto r = sa.ejecutar(azar);
    f.reFinal = r.final.D;
    f.costoFinal = r.final.costo;
    f.reMejor = r.mejor.D;
    f.costoMejor = r.mejor.costo;
    f.evaluaciones = r.evaluaciones;
    f.t0 = r.t0;
    f.temperaturas = r.temperaturas;
    f.aceptadas = r.aceptadas;
    return f;
}

std::vector<Fila> EjecutorBarrido::ejecutar(const ConfiguracionBarrido& configuracion, const std::string& fase,
                                            int repeticiones) const {
    std::vector<Fila> filas;
    for (std::size_t g = 0; g < instancias_.size(); ++g)
        for (int rep = 1; rep <= repeticiones; ++rep) filas.push_back(ejecutarUna(configuracion, fase, g, rep));
    return filas;
}

}  // namespace barrido
