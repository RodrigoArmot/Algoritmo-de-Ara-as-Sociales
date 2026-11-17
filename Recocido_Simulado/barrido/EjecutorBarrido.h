#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "recocido/barrido/ConfiguracionBarrido.h"
#include "nucleo/CargadorInstancia.h"
#include "nucleo/Instancia.h"

namespace barrido {

// Una ejecución del barrido: la configuración, el γ, la repetición y lo que registra el prompt —RE y
// costo de la solución final y de la mejor encontrada, evaluaciones y semilla—, más tres
// diagnósticos del recocido.
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

// Corre el recocido del núcleo sobre el experimento 1 de Guerra Ibarra (gi_exp1) con γ = 0,65, 0,75 y
// 0,85. Sin estado mutable: varias configuraciones pueden correr a la vez sobre el mismo objeto.
class EjecutorBarrido {
public:
    explicit EjecutorBarrido(const nucleo::ProblemaCargado& experimento1);

    // Las repeticiones de los tres γ de una configuración, en orden (γ, repetición).
    std::vector<Fila> ejecutar(const ConfiguracionBarrido& configuracion, const std::string& fase,
                               int repeticiones) const;
    // Una sola ejecución: la del γ de índice `gamma` y la repetición dada (desde 1).
    Fila ejecutarUna(const ConfiguracionBarrido& configuracion, const std::string& fase, std::size_t gamma,
                     int repeticion) const;

    // La semilla depende solo de la fase, la configuración, el γ y la repetición.
    static std::uint64_t semilla(const std::string& fase, const std::string& identificador, double gamma,
                                 int repeticion);
    static const std::vector<double>& gammas();

private:
    std::vector<nucleo::Instancia> instancias_;  // una por γ
    nucleo::Configuracion configuracion_;
};

// El γ como en los CSV: «0.65», «0.75», «0.85».
std::string textoGamma(double gamma);

}  // namespace barrido
