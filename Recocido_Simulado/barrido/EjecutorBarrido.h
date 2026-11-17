#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "recocido/barrido/ConfiguracionBarrido.h"
#include "nucleo/CargadorInstancia.h"
#include "nucleo/Instancia.h"

namespace barrido {

struct Fila {
    std::string fase;
    ConfiguracionBarrido configuracion;
    double gamma = 0.0;
    int repeticion = 0;
    std::uint64_t semilla = 0;
    double reFinal = 0.0, costoFinal = 0.0;
    double reMejor = 0.0, costoMejor = 0.0;
    long evaluaciones = 0;
    double t0 = 0.0;
    int temperaturas = 0;
    long aceptadas = 0;
};

class EjecutorBarrido {
public:
    explicit EjecutorBarrido(const nucleo::ProblemaCargado& experimento1);

    std::vector<Fila> ejecutar(const ConfiguracionBarrido& configuracion, const std::string& fase,
                               int repeticiones) const;
    Fila ejecutarUna(const ConfiguracionBarrido& configuracion, const std::string& fase, std::size_t gamma,
                     int repeticion) const;

    static std::uint64_t semilla(const std::string& fase, const std::string& identificador, double gamma,
                                 int repeticion);
    static const std::vector<double>& gammas();

private:
    std::vector<nucleo::Instancia> instancias_;
    nucleo::Configuracion configuracion_;
};

std::string textoGamma(double gamma);

}  // namespace barrido
