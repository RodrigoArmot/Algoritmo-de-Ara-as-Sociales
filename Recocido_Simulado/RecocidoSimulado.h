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
    nucleo::Solucion final;
    nucleo::Solucion mejor;
    double t0 = 0.0;
    double alfa = 0.0;
    int temperaturas = 0;
    long evaluaciones = 0;
    long aceptadas = 0;
    long aceptadasPeores = 0;
    std::uint64_t semilla = 0;
};

class RecocidoSimulado {
public:
    RecocidoSimulado(const nucleo::Instancia& instancia, const nucleo::Configuracion& configuracion,
                     ParametrosRecocido parametros);

    ResultadoRecocido ejecutar(nucleo::Aleatorio& azar) const;

    nucleo::Solucion solucionInicial(nucleo::Aleatorio& azar) const;
    nucleo::Solucion vecina(const nucleo::Solucion& actual, double Ev, nucleo::Aleatorio& azar) const;
    void vecinaEn(const nucleo::Solucion& actual, double Ev, nucleo::Aleatorio& azar,
                  nucleo::Solucion& candidata) const;
    static double temperaturaInicial(double Aa, double Ac, double p);
    static double factorEnfriamiento(double t0, double tf, int I);
    static double perturbacionMaxima(const ParametrosRecocido& P, double t, double t0);
    static bool acepta(double Aa, double Ac, double t, double U);
    static bool esMejora(double Aa, double Ac) { return Aa > Ac; }

    const ParametrosRecocido& parametros() const { return parametros_; }

private:
    void activarDosis(int idx, nucleo::Solucion& candidata, nucleo::Aleatorio& azar) const;

    const nucleo::Instancia* instancia_;
    nucleo::Configuracion configuracion_;
    ParametrosRecocido parametros_;
    nucleo::OperadorReparacion reparacion_;
};

}  // namespace recocido
