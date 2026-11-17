#pragma once

namespace recocido {

struct ParametrosRecocido {
    enum class Arranque {
        GuerraIbarra,
        PoblacionInicial
    };
    enum class Vecino {
        Todos,
        Uno
    };
    enum class Aceptacion {
        Metropolis,
        SoloMejoras
    };
    enum class DeltaInicial {
        UnaPareja,
        Media
    };
    enum class Perturbacion {
        Decreciente,
        Fija
    };

    double ll = 10.0;
    double ul = 500.0;
    double Ev = 0.15;
    double p = 0.8;
    double tf = 1.0;
    int I = 1000;
    int n = 100;
    Arranque arranque = Arranque::GuerraIbarra;

    Vecino vecino = Vecino::Todos;
    Aceptacion aceptacion = Aceptacion::Metropolis;
    DeltaInicial deltaInicial = DeltaInicial::UnaPareja;
    int muestrasDelta = 100;
    Perturbacion perturbacion = Perturbacion::Decreciente;

    bool activar = false;
};

}  // namespace recocido
