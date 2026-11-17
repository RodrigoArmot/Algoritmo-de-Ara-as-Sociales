#pragma once

namespace recocido {

// Parámetros del recocido simulado de Guerra Ibarra et al. (2026). Los publicados son ll, ul, Ev y
// tf (§4.2). El artículo no da p, I ni n: sus valores son decisión propia (p = 0,8, I = 1000,
// n = 100), declarada en R8.
//
// Las cuatro variantes del final sirven al barrido de reproducción de su Tabla 5
// (Prompt_Barrido_Recocido.md): cubren lo que el artículo deja ambiguo. Sus valores por omisión son
// los de la implementación validada, de modo que sin tocarlas el comportamiento no cambia.
struct ParametrosRecocido {
    enum class Arranque {
        GuerraIbarra,     // ec. (4): cada dosis al azar en [ll, ul]
        PoblacionInicial  // el procedimiento de la población inicial de las arañas, y reparar
    };
    enum class Vecino {
        Todos,  // ec. (6) a (8) sobre todas las dosis a la vez
        Uno     // ec. (6) a (8) sobre una sola dosis, elegida al azar
    };
    enum class Aceptacion {
        Metropolis,  // una peor se acepta con probabilidad e^(−(Ac − Aa)/t)
        SoloMejoras  // solo se acepta una candidata estrictamente mejor
    };
    enum class DeltaInicial {
        UnaPareja,  // Δ de la ec. (13) con una sola candidata, como en la figura 1
        Media       // Δ como media de |Aa − Ac| sobre muestrasDelta candidatas de la solución inicial
    };
    enum class Perturbacion {
        Decreciente,  // E_v = Ev·t/t0: baja linealmente con la temperatura
        Fija          // E_v = Ev en todas las temperaturas
    };

    double ll = 10.0;   // límite inferior de la dosis inicial, g
    double ul = 500.0;  // límite superior de la dosis inicial, g
    double Ev = 0.15;   // cota de la perturbación a la temperatura inicial (ec. 6)
    double p = 0.8;     // probabilidad deseada de aceptar una solución peor al inicio (ec. 13)
    double tf = 1.0;    // temperatura final
    int I = 1000;       // número de estados de energía (ec. 15)
    int n = 100;        // candidatas por temperatura, cada una con su decisión de aceptación
    Arranque arranque = Arranque::GuerraIbarra;

    Vecino vecino = Vecino::Todos;
    Aceptacion aceptacion = Aceptacion::Metropolis;
    DeltaInicial deltaInicial = DeltaInicial::UnaPareja;
    int muestrasDelta = 100;  // solo con DeltaInicial::Media
    Perturbacion perturbacion = Perturbacion::Decreciente;

    // Activación de dosis (R8, capítulo 7). La ec. (8) no saca una dosis de cero, así que con la
    // cardinalidad el recocido no podría incorporar fertilizantes. Con activar, cada candidata activa
    // además una dosis nula elegida al azar, con un valor uniforme en [ε, β]. Sin dosis nulas no se
    // sortea nada adicional, y en el experimento 1 de Guerra Ibarra el resultado no cambia.
    bool activar = false;
};

}  // namespace recocido
