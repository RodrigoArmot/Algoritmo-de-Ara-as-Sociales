#pragma once

#include <string>
#include <vector>

#include "recocido/ParametrosRecocido.h"

namespace barrido {

using recocido::ParametrosRecocido;

// Una combinación de los ocho factores que el artículo de Guerra Ibarra et al. (2026) deja sin
// publicar o ambiguos (Prompt_Barrido_Recocido.md). El resto del recocido es el validado.
struct ConfiguracionBarrido {
    ParametrosRecocido::Vecino vecino = ParametrosRecocido::Vecino::Todos;
    ParametrosRecocido::Aceptacion aceptacion = ParametrosRecocido::Aceptacion::Metropolis;
    double tFinal = 1.0;
    double p = 0.8;
    ParametrosRecocido::DeltaInicial deltaT0 = ParametrosRecocido::DeltaInicial::UnaPareja;
    int I = 1000;
    int n = 100;
    ParametrosRecocido::Perturbacion Ev = ParametrosRecocido::Perturbacion::Decreciente;

    // Identificador estable y legible, por ejemplo uno-metropolis-t1e-3-p080-media100-I1000-n100-Evdec.
    std::string identificador() const;
    // Los seis factores sin I ni n: lo que la fase 1 criba y la fase 2 expande a los nueve pares (I, n).
    std::string base() const;
    ParametrosRecocido parametros() const;
    // Evaluaciones de una ejecución cuando t0 > tf: (I + 1)·n + 1, y 99 más si Δ es la media de 100.
    long evaluacionesPorEjecucion() const;

    // El nivel de cada factor con el nombre del prompt, para las columnas de los CSV.
    std::string nivelVecino() const;      // todos · uno
    std::string nivelAceptacion() const;  // metropolis · solo_mejoras
    std::string nivelTFinal() const;      // 1 · 1e-2 · 1e-3 · 1e-5
    std::string nivelP() const;           // 0.5 · 0.8 · 0.95
    std::string nivelDeltaT0() const;     // una_pareja · media_100
    std::string nivelEv() const;          // decreciente · fija
};

// Nombres de los ocho factores, en el orden de las columnas, y el nivel de cada uno en una configuración.
const std::vector<std::string>& nombresFactores();
std::vector<std::string> niveles(const ConfiguracionBarrido& c);

// Las 1728 configuraciones (2·2·4·3·2·3·3·2), en un orden fijo.
std::vector<ConfiguracionBarrido> todas();
// Fase 1: las de I en {100, 1000} y n en {10, 100} (768).
std::vector<ConfiguracionBarrido> cribado();
// Una configuración con otro par (I, n).
ConfiguracionBarrido conIn(ConfiguracionBarrido c, int I, int n);
// Los niveles de I y de n.
const std::vector<int>& nivelesI();
const std::vector<int>& nivelesN();

}  // namespace barrido
