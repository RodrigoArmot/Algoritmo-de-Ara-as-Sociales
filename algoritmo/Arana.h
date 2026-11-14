// Araña del Algoritmo de Arañas Sociales (R3 y R4) y su movimiento.
//
// La elección de la vibración objetivo, el cambio de máscara y la caminata aleatoria se derivan de
// Spider::choose_vibration, Spider::mask_changing y Spider::random_walk de la implementación de
// referencia de James J. Q. Yu, publicada bajo licencia MIT
// (https://github.com/James-Yu/social-spider-algorithm, cpp/SSA.cpp); ver LICENCIAS_TERCEROS.txt.
// El manejo de cotas sigue la ecuación (6) de Yu y Li (2015a), en el intervalo [0, β].
#pragma once

#include <limits>
#include <vector>

#include "Aleatorio.h"
#include "Solucion.h"

// Vibración: logaritmo de su intensidad y posición de la araña que la emitió. La intensidad se
// guarda en logaritmo porque las atenuadas pueden caer por debajo del menor número representable.
struct Vibracion {
    double logIntensidad = -std::numeric_limits<double>::infinity();
    std::vector<double> origen;
};

class Arana {
public:
    // Estado inicial: la vibración objetivo es su propia posición, con intensidad cero; el
    // desplazamiento, la máscara y el contador valen cero.
    explicit Arana(const Solucion& posicionInicial);

    // Adopta la vibración más intensa que recibe si supera a su vibración objetivo, descartando las
    // que proceden de la posición que ya tiene como objetivo. Si no cambia, c aumenta en uno.
    // distancias[i] es la distancia a la posición que emitió vibraciones[i]. Devuelve si cambió.
    bool elegirVibracion(const std::vector<Vibracion>& vibraciones, const std::vector<double>& distancias,
                         double sigma, double ra);

    // Con probabilidad 1 − pc^c regenera la máscara: cada dimensión vale uno con una probabilidad
    // sorteada entre cero y pm. Devuelve si la regeneró.
    bool cambiarMascara(double pc, double pm, Aleatorio& azar);

    // Caminata aleatoria, dimensión a dimensión:  m ← r1·m + r2·(p − q),  q ← q + m,  y se refleja
    // si queda fuera de [0, β]. p es la dosis de la vibración objetivo o, si la máscara vale uno, la
    // de una araña al azar.
    void caminar(const std::vector<Vibracion>& vibraciones, const std::vector<double>& beta, Aleatorio& azar);

    // Intensidad recibida a distancia d (ec. 3), en logaritmo: log I − d/(σ·ra).
    static double logIntensidadRecibida(double logOrigen, double d, double sigma, double ra);
    // Ecuación (6): la dosis que sale de [0, b] queda en una fracción al azar de la distancia entre
    // la dosis anterior q0 y la cota rebasada.
    static double reflejar(double x, double q0, double b, Aleatorio& azar);

    Solucion posicion;
    Vibracion objetivo;
    int c = 0;                           // iteraciones desde que la vibración objetivo cambió
    std::vector<double> m;               // desplazamiento propuesto en la iteración anterior
    std::vector<unsigned char> mascara;  // máscara de dimensiones
};
