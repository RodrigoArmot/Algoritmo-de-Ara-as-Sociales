// Pruebas unitarias (R5) y de integración (R6) con Google Test.
// CARPETA_DATOS la define CMake: es la carpeta datos/ del proyecto.

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Aleatorio.h"
#include "Arana.h"
#include "FuncionObjetivo.h"
#include "Instancia.h"
#include "ModuloEjecucion.h"
#include "OperadorReparacion.h"
#include "SSA.h"
#include "Solucion.h"

namespace fs = std::filesystem;

namespace {

const std::string DATOS = CARPETA_DATOS;
const std::string COMPLETA = DATOS + "/completa";

const Instancia& completa() {
    static const Instancia I = Instancia::leer(COMPLETA);
    return I;
}

int fertilizante(const Instancia& I, const std::string& nombre) {
    for (int f = 0; f < I.F; ++f)
        if (I.fertilizantes[f].nombre == nombre) return f;
    return -1;
}

int especie(const Instancia& I, const std::string& simbolo) {
    for (int e = 0; e < I.E; ++e)
        if (I.especies[e].simbolo == simbolo) return e;
    return -1;
}

double alfa(const Instancia& I, int f, int e) {
    for (const Aporte& a : I.aportes[f])
        if (a.especie == e) return a.alfa;
    return 0.0;
}

// Admisible: a lo más k fertilizantes, cada dosis es cero o está entre ε y δ, y ninguna etapa
// supera el límite de la suma equivalente de cationes.
::testing::AssertionResult admisible(const Instancia& I, const Solucion& x) {
    if (x.numSeleccionados() > I.k) return ::testing::AssertionFailure() << "más de k fertilizantes";
    for (int f = 0; f < I.F; ++f)
        for (int s = 0; s < I.S; ++s) {
            const double q = x.dosis(f, s);
            if (q != 0.0 && (q < I.fertilizantes[f].epsilon || q > I.fertilizantes[f].delta))
                return ::testing::AssertionFailure() << "dosis fuera de dominio: " << I.fertilizantes[f].nombre;
        }
    const OperadorReparacion rep(I);
    for (int s = 0; s < I.S; ++s)
        if (rep.sumaCationes(x, s) > I.L + 1e-9) return ::testing::AssertionFailure() << "conductividad en etapa " << s;
    return ::testing::AssertionSuccess();
}

// Copia la instancia completa en una carpeta temporal para alterar uno de sus archivos.
fs::path copiaTemporal(const std::string& nombre) {
    const fs::path destino = fs::temp_directory_path() / ("ssa_basico_" + nombre);
    fs::remove_all(destino);
    fs::create_directories(destino);
    for (const auto& archivo : fs::directory_iterator(COMPLETA)) fs::copy_file(archivo.path(), destino / archivo.path().filename());
    return destino;
}

void anadirLinea(const fs::path& archivo, const std::string& linea) {
    std::ofstream(archivo, std::ios::app) << linea << '\n';
}

}  // namespace

// ---------------------------------------------------------------------------------- Instancia
TEST(Instancia, LeeLaInstanciaCompleta) {
    const Instancia& I = completa();
    EXPECT_EQ(I.F, 54);
    EXPECT_EQ(I.E, 16);
    EXPECT_EQ(I.S, 3);
    EXPECT_EQ(I.k, 16);
    EXPECT_DOUBLE_EQ(I.gamma, 0.5);
}

TEST(Instancia, RechazaUnaCarpetaInexistente) {
    EXPECT_THROW(Instancia::leer(DATOS + "/no_existe"), std::runtime_error);
}

TEST(Instancia, RechazaUnArchivoMalFormado) {
    const fs::path carpeta = copiaTemporal("mal_formado");
    anadirLinea(carpeta / "fertilizantes.csv", "Incompleto,X,100.0,1.0");
    EXPECT_THROW(Instancia::leer(carpeta.string()), std::runtime_error);
    fs::remove_all(carpeta);
}

TEST(Instancia, RechazaUnLimiteInferiorMayorQueElSuperior) {
    const fs::path carpeta = copiaTemporal("limites");
    std::ifstream entrada(carpeta / "limites.csv");
    std::stringstream texto;
    texto << entrada.rdbuf();
    entrada.close();
    std::string contenido = texto.str();
    const std::string original = "Nnit,1,100.0,100.0";
    ASSERT_NE(contenido.find(original), std::string::npos);
    contenido.replace(contenido.find(original), original.size(), "Nnit,1,200.0,100.0");
    std::ofstream(carpeta / "limites.csv") << contenido;
    EXPECT_THROW(Instancia::leer(carpeta.string()), std::runtime_error);
    fs::remove_all(carpeta);
}

// Ecuación (2): 1 g de nitrato de calcio en 1000 L aporta 0,1708 mg/L de N nítrico y 0,24429 de Ca.
TEST(Instancia, MatrizDeAporteUnitario) {
    const Instancia& I = completa();
    const int f = fertilizante(I, "Calciumnitrate");
    ASSERT_GE(f, 0);
    EXPECT_NEAR(alfa(I, f, especie(I, "Nnit")), 0.1708, 5e-5);
    EXPECT_NEAR(alfa(I, f, especie(I, "Ca")), 0.24429, 5e-6);
}

// ---------------------------------------------------------------------------- FuncionObjetivo
TEST(FuncionObjetivo, DosificacionNula) {
    const Instancia& I = completa();
    Solucion x(I);
    FuncionObjetivo(I).evaluar(x);
    EXPECT_EQ(x.D, I.D0);
    EXPECT_EQ(x.costo, 0.0);
    EXPECT_EQ(x.K, 0.0);
    EXPECT_NEAR(x.Z, I.gamma * I.D0, 1e-12);
}

TEST(FuncionObjetivo, LaIntensidadDecreceConLaAptitud) {
    EXPECT_GT(FuncionObjetivo::logIntensidad(-0.5), FuncionObjetivo::logIntensidad(0.0));
    EXPECT_GT(FuncionObjetivo::logIntensidad(0.0), FuncionObjetivo::logIntensidad(1.0));
    EXPECT_GT(FuncionObjetivo::logIntensidad(1.0), FuncionObjetivo::logIntensidad(10.0));
    EXPECT_TRUE(std::isfinite(FuncionObjetivo::logIntensidad(-0.99)));
}

// ------------------------------------------------------------------------- OperadorReparacion
TEST(OperadorReparacion, DominioDisjuntoDeLaDosisMinima) {
    const Instancia& I = completa();
    const int f = fertilizante(I, "Calciumnitrate");
    const double eps = I.fertilizantes[f].epsilon;
    Solucion x(I);
    x.dosis(f, 0) = 0.4 * eps;  // menos de la mitad: pasa a cero
    x.dosis(f, 1) = 0.6 * eps;  // la mitad o más: sube a ε
    x.dosis(f, 2) = 2.0 * eps;  // ya admisible: no cambia
    OperadorReparacion(I).reparar(x);
    EXPECT_EQ(x.dosis(f, 0), 0.0);
    EXPECT_EQ(x.dosis(f, 1), eps);
    EXPECT_EQ(x.dosis(f, 2), 2.0 * eps);
}

TEST(OperadorReparacion, DejaAdmisiblesPosicionesConTodosLosFertilizantes) {
    const Instancia& I = completa();
    const OperadorReparacion rep(I);
    Aleatorio azar(1);
    for (int prueba = 0; prueba < 200; ++prueba) {
        Solucion x(I);
        for (int i = 0; i < I.dimension(); ++i) x.q[i] = azar.uniforme(0.0, 2.0 * I.beta[i]);
        ASSERT_GT(x.numSeleccionados(), I.k);
        rep.reparar(x);
        ASSERT_TRUE(admisible(I, x)) << "prueba " << prueba;
    }
}

TEST(OperadorReparacion, RetiraElDeMenorAporteRelativo) {
    const Instancia& I = completa();
    const OperadorReparacion rep(I);
    // Un gramo de los k + 1 primeros fertilizantes con aporte relativo distinto de cero.
    Solucion x(I);
    std::vector<int> elegidos;
    for (int f = 0; f < I.F && static_cast<int>(elegidos.size()) <= I.k; ++f) {
        Solucion prueba(I);
        prueba.dosis(f, 0) = 1.0;
        if (rep.aporteRelativo(prueba, f) > 0.0) {
            elegidos.push_back(f);
            x.dosis(f, 0) = 1.0;
        }
    }
    ASSERT_EQ(static_cast<int>(elegidos.size()), I.k + 1);
    ASSERT_LE(rep.sumaCationes(x, 0), I.L);  // la conductividad no interviene
    int menor = elegidos[0];
    for (int f : elegidos)
        if (rep.aporteRelativo(x, f) < rep.aporteRelativo(x, menor)) menor = f;
    for (int f : elegidos) {
        if (f != menor) {
            ASSERT_NE(rep.aporteRelativo(x, f), rep.aporteRelativo(x, menor));  // sin empates
        }
    }
    rep.reparar(x);
    EXPECT_EQ(x.numSeleccionados(), I.k);
    EXPECT_FALSE(x.seleccionado(menor));
}

// ------------------------------------------------------------------------------ AlgoritmoSSA
TEST(AlgoritmoSSA, PosicionInicialConKFertilizantesEntreEpsilonYBeta) {
    const Instancia& I = completa();
    const AlgoritmoSSA ssa(I, ParametrosSSA{});
    Aleatorio azar(7);
    for (int prueba = 0; prueba < 50; ++prueba) {
        const Solucion x = ssa.posicionInicial(azar);
        EXPECT_EQ(x.numSeleccionados(), I.k);
        for (int f = 0; f < I.F; ++f)
            for (int s = 0; s < I.S; ++s) {
                const double q = x.dosis(f, s);
                if (q == 0.0) continue;
                EXPECT_GE(q, I.fertilizantes[f].epsilon);
                EXPECT_LE(q, I.beta[f * I.S + s]);
            }
    }
}

// --------------------------------------------------------------------------------------- Arana
TEST(Arana, AdoptaLaVibracionMasIntensaYDescartaLaDeSuObjetivo) {
    Solucion x;
    x.q = {0.0, 0.0};
    Arana arana(x);
    std::vector<Vibracion> v(3);
    v[0].origen = {0.0, 0.0};  // la suya: procede de su objetivo y se descarta
    v[0].logIntensidad = -1.0;
    v[1].origen = {1.0, 0.0};
    v[1].logIntensidad = -2.0;
    v[2].origen = {5.0, 0.0};
    v[2].logIntensidad = -1.5;
    const std::vector<double> d = {0.0, 1.0, 5.0};
    // Recibidas con σ = 1 y ra = 1: −3 y −6,5. Gana la segunda.
    EXPECT_TRUE(arana.elegirVibracion(v, d, 1.0, 1.0));
    EXPECT_EQ(arana.objetivo.origen, v[1].origen);
    EXPECT_DOUBLE_EQ(arana.objetivo.logIntensidad, -3.0);
    EXPECT_EQ(arana.c, 0);
    // Si ninguna la supera, conserva su objetivo y el contador aumenta.
    arana.objetivo.logIntensidad = 0.0;
    EXPECT_FALSE(arana.elegirVibracion(v, d, 1.0, 1.0));
    EXPECT_EQ(arana.c, 1);
}

TEST(Arana, LaMascaraNoCambiaConElContadorEnCero) {
    Solucion x;
    x.q.assign(10, 0.0);
    Arana arana(x);
    Aleatorio azar(3);
    for (int i = 0; i < 100; ++i) EXPECT_FALSE(arana.cambiarMascara(0.7, 0.1, azar));
}

TEST(Arana, LaReflexionDevuelveLaDosisAlIntervalo) {
    Aleatorio azar(5);
    EXPECT_EQ(Arana::reflejar(3.0, 2.0, 10.0, azar), 3.0);  // dentro: no cambia
    for (int i = 0; i < 100; ++i) {
        const double arriba = Arana::reflejar(15.0, 8.0, 10.0, azar);
        const double abajo = Arana::reflejar(-4.0, 1.0, 10.0, azar);
        EXPECT_GE(arriba, 0.0);
        EXPECT_LE(arriba, 10.0);
        EXPECT_GE(abajo, 0.0);
        EXPECT_LE(abajo, 10.0);
    }
}

TEST(Arana, LaCaminataQuedaEntreCeroYLaCotaDeBusqueda) {
    const Instancia& I = completa();
    const AlgoritmoSSA ssa(I, ParametrosSSA{});
    Aleatorio azar(11);
    std::vector<Vibracion> v(5);
    for (auto& vib : v) vib.origen = ssa.posicionInicial(azar).q;
    Arana arana(ssa.posicionInicial(azar));
    arana.objetivo.origen = v[0].origen;
    arana.mascara.assign(arana.mascara.size(), 1);  // también sigue a arañas al azar
    for (int paso = 0; paso < 20; ++paso) {
        arana.caminar(v, I.beta, azar);
        for (int i = 0; i < I.dimension(); ++i) {
            ASSERT_GE(arana.posicion.q[i], 0.0);
            ASSERT_LE(arana.posicion.q[i], I.beta[i]);
        }
    }
}

// ------------------------------------------------------------------------------ AlgoritmoSSA
TEST(AlgoritmoSSA, ReproducibleConLaMismaSemillaYAdmisible) {
    const Instancia& I = completa();
    ParametrosSSA P;
    P.poblacion = 10;
    P.iteraciones = 50;
    const AlgoritmoSSA ssa(I, P);
    Aleatorio a1(12345), a2(12345), a3(12346);
    const ResultadoSSA r1 = ssa.ejecutar(a1), r2 = ssa.ejecutar(a2), r3 = ssa.ejecutar(a3);
    EXPECT_EQ(r1.mejor.q, r2.mejor.q);
    EXPECT_EQ(r1.mejor.Z, r2.mejor.Z);
    EXPECT_NE(r1.mejor.Z, r3.mejor.Z);
    EXPECT_TRUE(admisible(I, r1.mejor));
    EXPECT_EQ(r1.iteraciones, 50);
    EXPECT_EQ(r1.evaluaciones, 500);
}

// --------------------------------------------------------------- ModuloEjecucion (integración)
TEST(ModuloEjecucion, LeeLosParametrosDelAlgoritmo) {
    const ConfiguracionEjecucion c = ModuloEjecucion::leerConfiguracion(DATOS + "/parametros_ssa.csv");
    EXPECT_EQ(c.parametros.poblacion, 30);
    EXPECT_DOUBLE_EQ(c.parametros.ra, 1.0);
    EXPECT_DOUBLE_EQ(c.parametros.pc, 0.7);
    EXPECT_DOUBLE_EQ(c.parametros.pm, 0.1);
    EXPECT_EQ(c.parametros.iteraciones, 1000);
    EXPECT_EQ(c.repeticiones, 30);
    EXPECT_THROW(ModuloEjecucion::leerConfiguracion(DATOS + "/no_existe.csv"), std::runtime_error);
}

TEST(ModuloEjecucion, EscribeUnaFilaPorRepeticionQueSeReproduceConSuSemilla) {
    const Instancia& I = completa();
    ConfiguracionEjecucion c;
    c.parametros.poblacion = 10;
    c.parametros.iteraciones = 20;
    c.repeticiones = 3;
    const fs::path salida = fs::temp_directory_path() / "ssa_basico_salida.csv";
    const std::vector<ResultadoSSA> resultados = ModuloEjecucion::ejecutar(I, c, salida.string(), nullptr);
    ASSERT_EQ(resultados.size(), 3u);

    std::ifstream archivo(salida);
    std::string linea;
    std::getline(archivo, linea);
    EXPECT_EQ(linea, "repeticion,semilla,Z,D,K,costo,seleccionados,iteraciones,evaluaciones");
    int filas = 0;
    while (std::getline(archivo, linea)) {
        std::vector<std::string> campos;
        std::stringstream ss(linea);
        for (std::string campo; std::getline(ss, campo, ',');) campos.push_back(campo);
        ASSERT_EQ(campos.size(), 9u);
        ++filas;
        EXPECT_EQ(std::stoi(campos[0]), filas);
        EXPECT_LE(std::stoi(campos[6]), I.k);
        EXPECT_EQ(std::stol(campos[8]), 200);
        EXPECT_TRUE(admisible(I, resultados[filas - 1].mejor));
        // La semilla del archivo repite la ejecución por separado, con la misma aptitud.
        Aleatorio azar(std::stoull(campos[1]));
        const ResultadoSSA r = AlgoritmoSSA(I, c.parametros).ejecutar(azar);
        EXPECT_EQ(r.mejor.Z, std::strtod(campos[2].c_str(), nullptr));
    }
    EXPECT_EQ(filas, 3);
    archivo.close();
    fs::remove(salida);
}
