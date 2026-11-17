// Etapa 2 · recocido simulado de Guerra Ibarra et al. (2026) sobre el núcleo compartido.
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

#include "nucleo/Ejecutor.h"
#include "nucleo/OperadorReparacion.h"
#include "nucleo/PosicionInicial.h"
#include "nucleo/Semilla.h"
#include "nucleo/pruebas/Ayudas.h"
#include "recocido/RecocidoSimulado.h"

using nucleo::Aleatorio;
using nucleo::Solucion;
using recocido::ParametrosRecocido;
using recocido::RecocidoSimulado;

namespace {

using pruebas::SecuenciaFija;

ParametrosRecocido parametrosCortos(int I, int n) {
    ParametrosRecocido p;
    p.I = I;
    p.n = n;
    return p;
}

void esperarIguales(const recocido::ResultadoRecocido& a, const recocido::ResultadoRecocido& b) {
    EXPECT_EQ(a.final.q, b.final.q);
    EXPECT_EQ(a.final.Z, b.final.Z);
    EXPECT_EQ(a.mejor.q, b.mejor.q);
    EXPECT_EQ(a.t0, b.t0);
    EXPECT_EQ(a.evaluaciones, b.evaluaciones);
    EXPECT_EQ(a.aceptadas, b.aceptadas);
}

}  // namespace

TEST(Recocido, TemperaturaInicialEsLaEcuacion13) {
    EXPECT_NEAR(RecocidoSimulado::temperaturaInicial(10.0, 12.0, 0.8), 2.0 / -std::log(0.8), 1e-15);
    EXPECT_EQ(RecocidoSimulado::temperaturaInicial(12.0, 10.0, 0.8), RecocidoSimulado::temperaturaInicial(10.0, 12.0, 0.8));
}

TEST(Recocido, ElFactorDeEnfriamientoLlevaDeT0ATfEnIPasos) {
    const double t0 = 65.5, tf = 1.0;
    const int I = 1000;
    const double alfa = RecocidoSimulado::factorEnfriamiento(t0, tf, I);
    EXPECT_LT(alfa, 1.0);
    double t = t0;
    for (int i = 0; i < I; ++i) t *= alfa;
    EXPECT_NEAR(t, tf, 1e-9);
}

TEST(Recocido, MetropolisAceptaLasMejorasYLasPeoresConProbabilidadDecreciente) {
    EXPECT_TRUE(RecocidoSimulado::acepta(5.0, 4.0, 1e-9, 0.999));  // mejora: siempre
    EXPECT_TRUE(RecocidoSimulado::acepta(5.0, 5.0, 1.0, 0.999));   // igual: P = 1 > U
    const double P = std::exp(-1.0);                               // peor por 1 a t = 1
    EXPECT_TRUE(RecocidoSimulado::acepta(4.0, 5.0, 1.0, P - 1e-9));
    EXPECT_FALSE(RecocidoSimulado::acepta(4.0, 5.0, 1.0, P + 1e-9));
    EXPECT_TRUE(RecocidoSimulado::acepta(4.0, 5.0, 10.0, 0.9));    // más caliente, más aceptación
    EXPECT_FALSE(RecocidoSimulado::acepta(4.0, 5.0, 0.1, 0.001));
}

class RecocidoGI : public ::testing::Test {
protected:
    nucleo::ProblemaCargado p = pruebas::cargar("gi_exp1");
    const nucleo::Instancia& I = p.instancia;
};

TEST_F(RecocidoGI, LaInstanciaEsElExperimento1) {
    EXPECT_EQ(I.numFertilizantes(), 69);
    EXPECT_EQ(I.numEtapas(), 1);
    EXPECT_FALSE(p.configuracion.dosisMinima || p.configuracion.conductividad || p.configuracion.cardinalidad);
    EXPECT_FALSE(p.configuracion.normalizarCosto);
    EXPECT_EQ(I.Cmin(I.indiceEspecie("K"), 0), 250.0);
    EXPECT_EQ(I.Cmin(I.indiceEspecie("N"), 0), 200.0);
    EXPECT_EQ(I.Cmin(I.indiceEspecie("P"), 0), 60.0);
    EXPECT_EQ(I.Cmin(I.indiceEspecie("Cl"), 0), 5.0);
    EXPECT_EQ(I.Cmin(I.indiceEspecie("Na"), 0), 2.0);
    // El N total cuenta todos los átomos de la fórmula, como la ec. (2): también los de la urea y los quelatos.
    EXPECT_EQ(I.fertilizante(I.indiceFertilizante("Urea")).n[static_cast<std::size_t>(I.indiceEspecie("N"))], 2);
    EXPECT_EQ(I.fertilizante(I.indiceFertilizante("Ironchelate")).n[static_cast<std::size_t>(I.indiceEspecie("N"))], 2);
}

TEST_F(RecocidoGI, LaSolucionInicialQuedaEntreLlYUl) {
    const RecocidoSimulado sa(I, p.configuracion, ParametrosRecocido{});
    Aleatorio azar(1);
    const Solucion x = sa.solucionInicial(azar);
    for (double q : x.q.datos()) {
        EXPECT_GE(q, 10.0);
        EXPECT_LT(q, 500.0);
    }
}

TEST_F(RecocidoGI, LaVecinaCambiaCadaDosisComoMuchoEvYNoReactivaLosCeros) {
    const RecocidoSimulado sa(I, p.configuracion, ParametrosRecocido{});
    Aleatorio azar(2);
    Solucion x = sa.solucionInicial(azar);
    for (int f = 0; f < I.numFertilizantes(); f += 3) x.q(f, 0) = 0.0;
    for (int intento = 0; intento < 50; ++intento) {
        const Solucion y = sa.vecina(x, 0.15, azar);
        for (int f = 0; f < I.numFertilizantes(); ++f) {
            const double antes = x.q(f, 0), despues = y.q(f, 0);
            EXPECT_GE(despues, 0.0);
            EXPECT_LE(std::fabs(despues - antes), 0.15 * antes + 1e-12);
            if (antes == 0.0) {
                EXPECT_EQ(despues, 0.0) << "una dosis en cero no vuelve a ser positiva";
            }
        }
    }
}

// Ec. (6) a (8) con una secuencia conocida: por cada dosis, primero ρ = Ev·u y después el signo.
TEST_F(RecocidoGI, LaVecinaConUnaSecuenciaFija) {
    const RecocidoSimulado sa(I, p.configuracion, ParametrosRecocido{});
    Solucion x(I);
    x.q(0, 0) = 100.0;
    x.q(1, 0) = 200.0;
    SecuenciaFija azar({0.5, 0.2, 0.9, 0.7});
    const Solucion y = sa.vecina(x, 0.15, azar);
    EXPECT_DOUBLE_EQ(y.q(0, 0), 100.0 + 100.0 * 0.075);  // ρ = 0,075; u = 0,2 < 0,5: sube
    EXPECT_DOUBLE_EQ(y.q(1, 0), 200.0 - 200.0 * 0.135);  // ρ = 0,135; u = 0,7: baja
    for (int f = 2; f < I.numFertilizantes(); ++f) EXPECT_EQ(y.q(f, 0), 0.0);
}

// La ec. (13) toma Δ de una sola pareja de soluciones, de modo que t0 cambia entre ejecuciones y
// puede quedar por debajo de tf. Con t0 > tf se recorren t0, t0·α, …, t0·α^I = tf; con t0 ≤ tf la
// figura 1 termina tras el primer nivel.
TEST_F(RecocidoGI, RecorreLasTemperaturasDeLaFigura1YCuentaLasEvaluaciones) {
    const ParametrosRecocido par = parametrosCortos(20, 7);
    const RecocidoSimulado sa(I, p.configuracion, par);
    int calientes = 0, frias = 0;
    for (std::uint64_t semilla = 1; semilla <= 60; ++semilla) {
        Aleatorio azar(semilla);
        const auto r = sa.ejecutar(azar);
        if (r.t0 > par.tf) {
            ++calientes;
            EXPECT_EQ(r.temperaturas, par.I + 1);
            EXPECT_EQ(r.evaluaciones, 1 + static_cast<long>(par.I + 1) * par.n);
            EXPECT_NEAR(r.t0 * std::pow(r.alfa, par.I), par.tf, 1e-9 * r.t0);
        } else {
            ++frias;
            EXPECT_EQ(r.temperaturas, 1);
            EXPECT_EQ(r.evaluaciones, 1 + static_cast<long>(par.n));
        }
        EXPECT_LE(r.mejor.Z, r.final.Z);
        EXPECT_LE(r.aceptadasPeores, r.aceptadas);
    }
    EXPECT_GT(calientes, 0);
    EXPECT_GT(frias, 0);
}

TEST_F(RecocidoGI, LaMismaSemillaDaLaMismaEjecucion) {
    const RecocidoSimulado sa(I, p.configuracion, parametrosCortos(30, 10));
    Aleatorio a(42), b(42), c(43);
    const auto ra = sa.ejecutar(a), rb = sa.ejecutar(b), rc = sa.ejecutar(c);
    esperarIguales(ra, rb);
    EXPECT_NE(ra.final.q, rc.final.q);
}

TEST_F(RecocidoGI, ElResultadoNoDependeDelNumeroDeHilos) {
    const RecocidoSimulado sa(I, p.configuracion, parametrosCortos(30, 10));
    auto tarea = [&](int i) {
        Aleatorio azar(nucleo::semilla::derivar("gi_exp1/prueba", static_cast<std::uint64_t>(i + 1)));
        return sa.ejecutar(azar);
    };
    const auto uno = nucleo::ejecutarEnParalelo<recocido::ResultadoRecocido>(8, 1, tarea);
    const auto cuatro = nucleo::ejecutarEnParalelo<recocido::ResultadoRecocido>(8, 4, tarea);
    for (std::size_t i = 0; i < uno.size(); ++i) esperarIguales(uno[i], cuatro[i]);
}

// ------------------------------------------------------------------ problema completo
class RecocidoCompleto : public ::testing::Test {
protected:
    nucleo::ProblemaCargado p = pruebas::cargar("completa");
    const nucleo::Instancia& I = p.instancia;
};

// El arranque sobre el problema completo es el de la población inicial de las arañas.
TEST_F(RecocidoCompleto, ElArranqueUsaKFertilizantesEntreEpsilonYBeta) {
    Aleatorio azar(4);
    for (int intento = 0; intento < 20; ++intento) {
        Solucion x = nucleo::posicionInicial(I, p.configuracion, azar);
        EXPECT_EQ(x.numSeleccionados(), I.k());
        for (int f = 0; f < I.numFertilizantes(); ++f) {
            if (!x.seleccionado(f)) continue;
            for (int s = 0; s < I.numEtapas(); ++s) {
                EXPECT_GE(x.q(f, s), I.fertilizante(f).epsilon);
                EXPECT_LE(x.q(f, s), I.beta(f, s));
            }
        }
        nucleo::OperadorReparacion(I, p.configuracion).reparar(x);
        pruebas::esperarAdmisible(I, p.configuracion, x);
    }
}

TEST_F(RecocidoCompleto, ConLasTresRestriccionesDevuelveSolucionesAdmisibles) {
    ParametrosRecocido par = parametrosCortos(40, 15);
    par.arranque = ParametrosRecocido::Arranque::PoblacionInicial;
    const RecocidoSimulado sa(I, p.configuracion, par);
    for (std::uint64_t semilla : {11u, 12u, 13u}) {
        Aleatorio azar(semilla);
        const auto r = sa.ejecutar(azar);
        pruebas::esperarAdmisible(I, p.configuracion, r.final);
        pruebas::esperarAdmisible(I, p.configuracion, r.mejor);
        EXPECT_LE(r.mejor.Z, r.final.Z);
        EXPECT_TRUE(r.mejor.evaluada);
    }
}

// ------------------------------------------------------------------ activación de dosis
// Configuración comparada en el capítulo 7: todas las dosis, solo mejoras, Ev fija, con activación.
namespace {
ParametrosRecocido comparada(bool activar) {
    ParametrosRecocido par;
    par.arranque = ParametrosRecocido::Arranque::PoblacionInicial;
    par.aceptacion = ParametrosRecocido::Aceptacion::SoloMejoras;
    par.perturbacion = ParametrosRecocido::Perturbacion::Fija;
    par.activar = activar;
    return par;
}
}  // namespace

TEST_F(RecocidoCompleto, LaActivacionEnciendeUnaSolaDosisNulaEntreEpsilonYBeta) {
    const RecocidoSimulado sa(I, p.configuracion, comparada(true));
    Aleatorio azar(21);
    Solucion x = nucleo::posicionInicial(I, p.configuracion, azar);
    nucleo::OperadorReparacion(I, p.configuracion).reparar(x);
    for (int intento = 0; intento < 50; ++intento) {
        const Solucion y = sa.vecina(x, 0.1, azar);
        int activadas = 0;
        for (int f = 0; f < I.numFertilizantes(); ++f) {
            for (int s = 0; s < I.numEtapas(); ++s) {
                const double antes = x.q(f, s), despues = y.q(f, s);
                if (antes > 0.0) {
                    EXPECT_LE(std::fabs(despues - antes), 0.1 * antes + 1e-12);
                } else if (despues > 0.0) {
                    ++activadas;
                    EXPECT_GE(despues, I.fertilizante(f).epsilon);
                    EXPECT_LE(despues, std::max(I.beta(f, s), I.fertilizante(f).epsilon));
                }
            }
        }
        EXPECT_EQ(activadas, 1) << "cada candidata activa exactamente una dosis nula";
    }
}

TEST_F(RecocidoCompleto, ConElVecinoUnoSoloSeActivaLaDosisElegidaSiEsNula) {
    ParametrosRecocido par = comparada(true);
    par.vecino = ParametrosRecocido::Vecino::Uno;
    const RecocidoSimulado sa(I, p.configuracion, par);
    Aleatorio azar(22);
    const Solucion nula(I);  // todas las dosis en cero
    for (int intento = 0; intento < 30; ++intento) {
        const Solucion y = sa.vecina(nula, 0.1, azar);
        int positivas = 0;
        for (int f = 0; f < I.numFertilizantes(); ++f)
            for (int s = 0; s < I.numEtapas(); ++s)
                if (y.q(f, s) > 0.0) {
                    ++positivas;
                    EXPECT_GE(y.q(f, s), I.fertilizante(f).epsilon);
                }
        EXPECT_EQ(positivas, 1);
    }
}

TEST_F(RecocidoCompleto, ConActivacionDevuelveSolucionesAdmisiblesYMejoraLaInicial) {
    ParametrosRecocido par = comparada(true);
    par.I = 40;
    par.n = 15;
    const RecocidoSimulado sa(I, p.configuracion, par);
    for (std::uint64_t semilla : {31u, 32u, 33u}) {
        Aleatorio azar(semilla);
        const auto r = sa.ejecutar(azar);
        pruebas::esperarAdmisible(I, p.configuracion, r.mejor);
        EXPECT_EQ(r.mejor.Z, r.final.Z) << "aceptando solo mejoras, la final es la mejor";
        EXPECT_GT(r.aceptadas, 0);
        EXPECT_EQ(r.aceptadasPeores, 0);
    }
}

// Sin dosis nulas la activación no sortea nada: en el experimento 1 de Guerra Ibarra la ejecución
// es idéntica con y sin ella.
TEST_F(RecocidoGI, SinDosisNulasLaActivacionNoCambiaLaEjecucion) {
    ParametrosRecocido sin = parametrosCortos(30, 20), con = sin;
    sin.aceptacion = con.aceptacion = ParametrosRecocido::Aceptacion::SoloMejoras;
    sin.perturbacion = con.perturbacion = ParametrosRecocido::Perturbacion::Fija;
    con.activar = true;
    const RecocidoSimulado a(I, p.configuracion, sin), b(I, p.configuracion, con);
    for (std::uint64_t semilla : {41u, 42u}) {
        Aleatorio azarA(semilla), azarB(semilla);
        esperarIguales(a.ejecutar(azarA), b.ejecutar(azarB));
    }
}
