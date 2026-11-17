// Barrido de reproducción del recocido: variantes del recocido y arnés (Prompt_Barrido_Recocido.md).
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "nucleo/Ejecutor.h"
#include "nucleo/pruebas/Ayudas.h"
#include "recocido/RecocidoSimulado.h"
#include "recocido/barrido/ConfiguracionBarrido.h"
#include "recocido/barrido/Criterio.h"
#include "recocido/barrido/EjecutorBarrido.h"
#include "recocido/barrido/ResumenBarrido.h"

using barrido::ConfiguracionBarrido;
using recocido::ParametrosRecocido;
using recocido::RecocidoSimulado;

// ----------------------------------------------------------------------- variantes del recocido
TEST(VariantesRecocido, LosValoresPorOmisionSonLosDeLaImplementacionValidada) {
    const ParametrosRecocido p;
    EXPECT_EQ(p.vecino, ParametrosRecocido::Vecino::Todos);
    EXPECT_EQ(p.aceptacion, ParametrosRecocido::Aceptacion::Metropolis);
    EXPECT_EQ(p.deltaInicial, ParametrosRecocido::DeltaInicial::UnaPareja);
    EXPECT_EQ(p.perturbacion, ParametrosRecocido::Perturbacion::Decreciente);
    EXPECT_EQ(p.tf, 1.0);
    EXPECT_EQ(p.p, 0.8);
    EXPECT_EQ(p.I, 1000);
    EXPECT_EQ(p.n, 100);
}

class VariantesGI : public ::testing::Test {
protected:
    nucleo::ProblemaCargado p = pruebas::cargar("gi_exp1");
    const nucleo::Instancia& I = p.instancia;
};

TEST_F(VariantesGI, ElVecinoUnoCambiaUnaSolaDosis) {
    ParametrosRecocido par;
    par.vecino = ParametrosRecocido::Vecino::Uno;
    const RecocidoSimulado sa(I, p.configuracion, par);
    nucleo::Aleatorio azar(21);
    const nucleo::Solucion x = sa.solucionInicial(azar);
    for (int intento = 0; intento < 200; ++intento) {
        const nucleo::Solucion y = sa.vecina(x, 0.15, azar);
        int cambiadas = 0;
        for (std::size_t i = 0; i < x.q.datos().size(); ++i) {
            const double antes = x.q.datos()[i], despues = y.q.datos()[i];
            if (antes != despues) ++cambiadas;
            EXPECT_LE(std::fabs(despues - antes), 0.15 * antes + 1e-12);
        }
        EXPECT_LE(cambiadas, 1);
    }
}

TEST_F(VariantesGI, SoloMejorasNuncaAceptaUnaPeorYLaFinalEsLaMejor) {
    ParametrosRecocido par;
    par.aceptacion = ParametrosRecocido::Aceptacion::SoloMejoras;
    par.I = 30;
    par.n = 20;
    const RecocidoSimulado sa(I, p.configuracion, par);
    for (std::uint64_t semilla = 1; semilla <= 5; ++semilla) {
        nucleo::Aleatorio azar(semilla);
        const auto r = sa.ejecutar(azar);
        EXPECT_EQ(r.aceptadasPeores, 0);
        EXPECT_EQ(r.final.Z, r.mejor.Z);
        EXPECT_EQ(r.final.q, r.mejor.q);
    }
}

TEST_F(VariantesGI, LaDeltaPorMediaGastaCienCandidatasYElPlanLasCuenta) {
    for (auto delta : {ParametrosRecocido::DeltaInicial::UnaPareja, ParametrosRecocido::DeltaInicial::Media}) {
        ConfiguracionBarrido c;
        c.deltaT0 = delta;
        c.tFinal = 1e-5;  // t0 > tf: se recorren todos los niveles
        c.I = 20;
        c.n = 7;
        const RecocidoSimulado sa(I, p.configuracion, c.parametros());
        nucleo::Aleatorio azar(5);
        const auto r = sa.ejecutar(azar);
        ASSERT_GT(r.t0, c.tFinal);
        EXPECT_EQ(r.evaluaciones, c.evaluacionesPorEjecucion());
        EXPECT_EQ(r.evaluaciones, 21L * 7 + (delta == ParametrosRecocido::DeltaInicial::Media ? 100 : 1));
    }
}

TEST(VariantesRecocido, LaPerturbacionFijaNoBajaConLaTemperatura) {
    ParametrosRecocido par;
    EXPECT_DOUBLE_EQ(RecocidoSimulado::perturbacionMaxima(par, 5.0, 10.0), 0.075);  // decreciente: Ev·t/t0
    par.perturbacion = ParametrosRecocido::Perturbacion::Fija;
    EXPECT_EQ(RecocidoSimulado::perturbacionMaxima(par, 5.0, 10.0), 0.15);
    EXPECT_EQ(RecocidoSimulado::perturbacionMaxima(par, 1e-5, 10.0), 0.15);
}

// ------------------------------------------------------------------------------ configuraciones
// Los niveles de la tabla del prompt dan 2·2·4·3·2·3·3·2 = 1728 configuraciones. Las 3456 del prompt
// cuentan cada una dos veces: evaluada con la solución final y con la mejor encontrada.
TEST(Barrido, HayMilSetecientasVeintiochoConfiguracionesConIdentificadorUnico) {
    const auto lista = barrido::todas();
    ASSERT_EQ(lista.size(), 1728u);
    std::set<std::string> ids, bases;
    for (const auto& c : lista) {
        ids.insert(c.identificador());
        bases.insert(c.base());
    }
    EXPECT_EQ(ids.size(), 1728u);
    EXPECT_EQ(bases.size(), 192u);  // 1728 / 9 pares (I, n)
    EXPECT_EQ(barrido::cribado().size(), 768u);

    // El ejemplo del prompt.
    ConfiguracionBarrido c;
    c.vecino = ParametrosRecocido::Vecino::Uno;
    c.tFinal = 1e-3;
    c.deltaT0 = ParametrosRecocido::DeltaInicial::Media;
    EXPECT_EQ(c.identificador(), "uno-metropolis-t1e-3-p080-media100-I1000-n100-Evdec");
    EXPECT_TRUE(ids.count(c.identificador()));
    const auto niveles = barrido::niveles(c);
    EXPECT_EQ(niveles, (std::vector<std::string>{"uno", "metropolis", "1e-3", "0.8", "media_100", "1000", "100",
                                                 "decreciente"}));
}

// -------------------------------------------------------------------------------------- criterio
TEST(Barrido, LosRangosPublicadosSonLosDeLaTabla5) {
    const auto r = barrido::rangosPublicados(nucleo::rutaDatos() / "guerra_ibarra_tabla5.csv");
    ASSERT_EQ(r.size(), 3u);
    EXPECT_EQ(r[0].gamma, 0.65);
    EXPECT_EQ(r[0].RE.min, 0.00765);
    EXPECT_EQ(r[0].RE.max, 0.04342);
    EXPECT_EQ(r[0].costo.min, 2.07);
    EXPECT_EQ(r[0].costo.max, 2.57);
    EXPECT_EQ(r[1].RE.min, 0.00992);
    EXPECT_EQ(r[1].RE.max, 0.02599);
    EXPECT_EQ(r[1].costo.max, 3.10);
    EXPECT_EQ(r[2].RE.min, 0.00678);
    EXPECT_EQ(r[2].RE.max, 0.03797);
    EXPECT_EQ(r[2].costo.min, 2.56);
    EXPECT_EQ(r[2].costo.max, 3.21);
}

TEST(Barrido, CriterioDistanciaYCribado) {
    const auto r = barrido::rangosPublicados(nucleo::rutaDatos() / "guerra_ibarra_tabla5.csv");
    std::vector<barrido::Medianas> dentro = {{0.02, 2.3}, {0.02, 2.5}, {0.02, 3.0}};
    EXPECT_TRUE(barrido::cumple(dentro, r));
    EXPECT_EQ(barrido::distancia(dentro, r), 0.0);
    EXPECT_TRUE(barrido::pasaCribado(dentro, r));

    auto fuera = dentro;
    fuera[1].RE = 0.25;  // unas 9,6 veces el máximo de γ = 0,75: menos de una década, todavía pasa el cribado
    EXPECT_FALSE(barrido::cumple(fuera, r));
    EXPECT_NEAR(barrido::distancia(fuera, r), std::log10(0.25 / 0.02599), 1e-12);
    EXPECT_TRUE(barrido::pasaCribado(fuera, r));
    fuera[1].RE = 0.52;  // veinte veces: ya no
    EXPECT_FALSE(barrido::pasaCribado(fuera, r));

    auto caro = dentro;
    caro[2].costo = 32.1;  // el costo no entra en el cribado, sí en el criterio
    EXPECT_TRUE(barrido::pasaCribado(caro, r));
    EXPECT_FALSE(barrido::cumple(caro, r));
    EXPECT_NEAR(barrido::distancia(caro, r), 1.0, 1e-12);

    // El costo cero de la dosificación nula, o mil veces fuera, cuenta como tres décadas.
    EXPECT_EQ(barrido::distanciaLog(0.0, r[0].costo), barrido::kTopeDecadas);
    EXPECT_EQ(barrido::distanciaLog(1e6, r[0].costo), barrido::kTopeDecadas);

    EXPECT_EQ(barrido::mediana({3.0, 1.0, 2.0}), 2.0);
    EXPECT_EQ(barrido::mediana({4.0, 1.0, 2.0, 3.0}), 2.5);
}

// ------------------------------------------------------------------------------------- ejecución
TEST(Barrido, LaSemillaDependeDeFaseConfiguracionGammaYRepeticion) {
    using barrido::EjecutorBarrido;
    const std::string id = ConfiguracionBarrido().identificador();
    const auto s = EjecutorBarrido::semilla("fase1", id, 0.65, 1);
    EXPECT_EQ(s, EjecutorBarrido::semilla("fase1", id, 0.65, 1));
    EXPECT_NE(s, EjecutorBarrido::semilla("fase3", id, 0.65, 1));
    EXPECT_NE(s, EjecutorBarrido::semilla("fase1", id, 0.75, 1));
    EXPECT_NE(s, EjecutorBarrido::semilla("fase1", id, 0.65, 2));
    ConfiguracionBarrido otra;
    otra.n = 10;
    EXPECT_NE(s, EjecutorBarrido::semilla("fase1", otra.identificador(), 0.65, 1));
}

TEST(Barrido, ElResultadoNoDependeDelNumeroDeHilosYSeLeeComoSeEscribe) {
    const auto problema = pruebas::cargar("gi_exp1");
    const barrido::EjecutorBarrido ejecutor(problema);
    std::vector<ConfiguracionBarrido> configuraciones;
    for (const auto& c : barrido::cribado())
        if (c.I == 100 && c.n == 10 && configuraciones.size() < 6) configuraciones.push_back(c);
    auto tarea = [&](int i) {
        const auto& c = configuraciones[static_cast<std::size_t>(i) / 6];
        return ejecutor.ejecutarUna(c, "prueba", static_cast<std::size_t>(i) % 3, static_cast<int>(i % 6) / 3 + 1);
    };
    const int n = static_cast<int>(configuraciones.size()) * 6;
    const auto uno = nucleo::ejecutarEnParalelo<barrido::Fila>(n, 1, tarea);
    const auto varios = nucleo::ejecutarEnParalelo<barrido::Fila>(n, 8, tarea);
    for (int i = 0; i < n; ++i) EXPECT_EQ(barrido::lineaFila(uno[static_cast<std::size_t>(i)]),
                                          barrido::lineaFila(varios[static_cast<std::size_t>(i)]));

    const auto ruta = std::filesystem::temp_directory_path() / "barrido_prueba_filas.csv";
    {
        std::ofstream csv(ruta);
        csv << barrido::encabezadoFilas() << '\n';
        for (const auto& f : uno) csv << barrido::lineaFila(f) << '\n';
    }
    const auto leidas = barrido::leerFilas(ruta);
    std::filesystem::remove(ruta);
    ASSERT_EQ(leidas.size(), uno.size());
    for (std::size_t i = 0; i < uno.size(); ++i) {
        EXPECT_EQ(barrido::lineaFila(leidas[i]), barrido::lineaFila(uno[i]));
        EXPECT_EQ(leidas[i].configuracion.identificador(), uno[i].configuracion.identificador());
        EXPECT_EQ(leidas[i].semilla, uno[i].semilla);
    }
}
