#pragma once

#include <string>
#include <vector>

#include "recocido/ParametrosRecocido.h"

namespace barrido {

using recocido::ParametrosRecocido;

struct ConfiguracionBarrido {
    ParametrosRecocido::Vecino vecino = ParametrosRecocido::Vecino::Todos;
    ParametrosRecocido::Aceptacion aceptacion = ParametrosRecocido::Aceptacion::Metropolis;
    double tFinal = 1.0;
    double p = 0.8;
    ParametrosRecocido::DeltaInicial deltaT0 = ParametrosRecocido::DeltaInicial::UnaPareja;
    int I = 1000;
    int n = 100;
    ParametrosRecocido::Perturbacion Ev = ParametrosRecocido::Perturbacion::Decreciente;

    std::string identificador() const;
    std::string base() const;
    ParametrosRecocido parametros() const;
    long evaluacionesPorEjecucion() const;

    std::string nivelVecino() const;
    std::string nivelAceptacion() const;
    std::string nivelTFinal() const;
    std::string nivelP() const;
    std::string nivelDeltaT0() const;
    std::string nivelEv() const;
};

const std::vector<std::string>& nombresFactores();
std::vector<std::string> niveles(const ConfiguracionBarrido& c);

std::vector<ConfiguracionBarrido> todas();
std::vector<ConfiguracionBarrido> cribado();
ConfiguracionBarrido conIn(ConfiguracionBarrido c, int I, int n);
const std::vector<int>& nivelesI();
const std::vector<int>& nivelesN();

}  // namespace barrido
