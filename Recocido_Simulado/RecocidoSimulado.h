#pragma once

#include <cstdint>

#include "nucleo/Aleatorio.h"
#include "nucleo/Configuracion.h"
#include "nucleo/FuncionObjetivo.h"
#include "nucleo/Instancia.h"
#include "nucleo/OperadorReparacion.h"
#include "nucleo/Solucion.h"
#include "recocido/ParametrosRecocido.h"

namespace recocido {

struct ResultadoRecocido {
    nucleo::Solucion final;  // S_a al terminar: la que devuelve la figura 1 de Guerra Ibarra
    nucleo::Solucion mejor;  // la mejor solución aceptada durante la ejecución
    double t0 = 0.0;         // temperatura inicial, ec. (13)
    double alfa = 0.0;       // factor de enfriamiento, ec. (15)
    int temperaturas = 0;    // niveles de temperatura recorridos
    long evaluaciones = 0;
    long aceptadas = 0;       // candidatas aceptadas
    long aceptadasPeores = 0; // de ellas, peores que la actual (por el criterio de Metropolis)
    std::uint64_t semilla = 0;
};

// Recocido simulado de Guerra Ibarra et al. (2026), sobre el núcleo compartido: la misma función
// objetivo y, si la configuración lo pide, el mismo operador de reparación que las arañas.
//
// Del artículo: la solución inicial (ec. 4), el vecino multiplicativo (ec. 6 a 8), la temperatura
// inicial (ec. 13), el enfriamiento geométrico (ec. 14 y 15) y la estructura de la figura 1.
// Resuelto con el texto frente a la figura: la aceptación es la de Metropolis, P = e^(−(Ac − Aa)/t)
// (la figura trae el signo invertido), y cada una de las n candidatas de una temperatura pasa por la
// decisión de aceptación (en la figura, el bucle interno la salta). E_v baja proporcional a t/t0.
// Las variantes de ParametrosRecocido cambian esas cuatro decisiones solo si se piden.
class RecocidoSimulado {
public:
    RecocidoSimulado(const nucleo::Instancia& instancia, const nucleo::Configuracion& configuracion,
                     ParametrosRecocido parametros);

    ResultadoRecocido ejecutar(nucleo::Aleatorio& azar) const;

    // Operadores, expuestos para las pruebas.
    nucleo::Solucion solucionInicial(nucleo::Aleatorio& azar) const;
    // Ec. (6) a (8): cada dosis sube o baja, con igual probabilidad, una fracción ρ ~ U(0, Ev) de sí
    // misma; la que quedara negativa pasa a cero. Una dosis en cero no vuelve a ser positiva, salvo
    // con activar, que además activa una dosis nula en [ε, β]. Con el vecino «uno», solo cambia una
    // dosis, elegida al azar, y con activar se activa si es nula.
    nucleo::Solucion vecina(const nucleo::Solucion& actual, double Ev, nucleo::Aleatorio& azar) const;
    // Lo mismo, escribiendo la candidata en una solución ya dimensionada: evita reservar memoria en cada
    // paso. Solo fija las dosis y las variables derivadas; lo demás lo escribe la evaluación.
    void vecinaEn(const nucleo::Solucion& actual, double Ev, nucleo::Aleatorio& azar,
                  nucleo::Solucion& candidata) const;
    static double temperaturaInicial(double Aa, double Ac, double p);  // |Aa − Ac| / (−ln p)
    static double factorEnfriamiento(double t0, double tf, int I);     // (tf/t0)^(1/I)
    // Cota de la perturbación a la temperatura t: Ev·t/t0 si decrece; Ev si es fija.
    static double perturbacionMaxima(const ParametrosRecocido& P, double t, double t0);
    // Criterio de Metropolis: se acepta si mejora; si no, con probabilidad e^(−(Ac − Aa)/t) > U.
    static bool acepta(double Aa, double Ac, double t, double U);
    static bool esMejora(double Aa, double Ac) { return Aa > Ac; }

    const ParametrosRecocido& parametros() const { return parametros_; }

private:
    // La dosis de posición idx (fertilizante idx / S, etapa idx % S) toma un valor uniforme en [ε, β].
    void activarDosis(int idx, nucleo::Solucion& candidata, nucleo::Aleatorio& azar) const;

    // Sin estado mutable: varias ejecuciones pueden correr a la vez sobre el mismo objeto, cada una
    // con su generador y su propia función objetivo.
    const nucleo::Instancia* instancia_;
    nucleo::Configuracion configuracion_;
    ParametrosRecocido parametros_;
    nucleo::OperadorReparacion reparacion_;
};

}  // namespace recocido
