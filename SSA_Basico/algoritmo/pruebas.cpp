#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif

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

void reemplazar(const fs::path& archivo, const std::string& original, const std::string& nuevo) {
    std::ifstream entrada(archivo, std::ios::binary);
    std::stringstream texto;
    texto << entrada.rdbuf();
    entrada.close();
    std::string contenido = texto.str();
    const std::size_t i = contenido.find(original);
    ASSERT_NE(i, std::string::npos) << original;
    contenido.replace(i, original.size(), nuevo);
    std::ofstream(archivo, std::ios::binary) << contenido;
}

void rechazaConCambio(const std::string& nombre, const std::string& archivo, const std::string& original,
                      const std::string& nuevo) {
    const fs::path carpeta = copiaTemporal(nombre);
    reemplazar(carpeta / archivo, original, nuevo);
    EXPECT_THROW(Instancia::leer(carpeta.string()), std::runtime_error) << nombre;
    fs::remove_all(carpeta);
}

std::string filaNitratoDePotasio(const std::string& nombre, double precio, double delta) {
    std::ostringstream fila;
    fila << nombre << ",KNO3,101.1," << precio << ",0.05," << delta << ",1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0";
    return fila.str();
}

fs::path instanciaMinima(const std::string& nombre, int k, const std::vector<std::string>& fertilizantes) {
    const fs::path carpeta = copiaTemporal(nombre);
    std::ofstream(carpeta / "parametros.csv") << "clave,valor\netapas,3\nV,1000.0\nEmax,2.5\nk," << k
                                              << "\ngamma,0.5\nKref,10.0\n";
    std::ofstream f(carpeta / "fertilizantes.csv");
    f << "nombre,formula,mw,precio,epsilon,delta,Nnit,Nam,P,K,Ca,Mg,S,Fe,Mn,Zn,B,Cu,Mo,Cl,Ni,Na\n";
    for (const std::string& fila : fertilizantes) f << fila << '\n';
    return carpeta;
}

int ejecutarPrograma(const std::string& argumentos) {
    std::string orden = "\"" + std::string(PROGRAMA) + "\"" + argumentos;
#ifdef _WIN32
    orden = "\"" + orden + " >NUL 2>NUL\"";
    return std::system(orden.c_str());
#else
    orden += " >/dev/null 2>/dev/null";
    const int estado = std::system(orden.c_str());
    return WIFEXITED(estado) ? WEXITSTATUS(estado) : -1;
#endif
}

}  // namespace

// TEST Instancias
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

TEST(Instancia, MatrizDeAporteUnitario) {
    const Instancia& I = completa();
    const int f = fertilizante(I, "Calciumnitrate");
    ASSERT_GE(f, 0);
    EXPECT_NEAR(alfa(I, f, especie(I, "Nnit")), 0.1708, 5e-5);
    EXPECT_NEAR(alfa(I, f, especie(I, "Ca")), 0.24429, 5e-6);
}

TEST(Instancia, AceptaVariantesDeFormato) {
    const fs::path carpeta = copiaTemporal("formato");
    reemplazar(carpeta / "parametros.csv", "clave,valor", "\xEF\xBB\xBF" "clave , valor");
    anadirLinea(carpeta / "especies.csv", "   ");
    const Instancia I = Instancia::leer(carpeta.string() + "/");
    EXPECT_EQ(I.nombre, "ssa_basico_formato");
    EXPECT_EQ(Instancia::leer(carpeta.string() + "\\").nombre, "ssa_basico_formato");
    EXPECT_EQ(I.F, 54);
    EXPECT_EQ(I.E, 16);
    fs::remove_all(carpeta);
}

TEST(Instancia, RechazaArchivosIncompletos) {
    EXPECT_THROW(Instancia::leer(""), std::runtime_error);
    {
        const fs::path carpeta = copiaTemporal("vacio2");
        std::ofstream(carpeta / "agua.csv", std::ios::trunc);
        EXPECT_THROW(Instancia::leer(carpeta.string()), std::runtime_error);
        fs::remove_all(carpeta);
    }
    rechazaConCambio("columna", "especies.csv", "peso_atomico", "peso");
    rechazaConCambio("clave", "parametros.csv", "gamma,0.5", "");
    rechazaConCambio("filas", "limites.csv", "Na,3,0.0,460.0", "");
}

TEST(Instancia, RechazaValoresInvalidos) {
    rechazaConCambio("real", "parametros.csv", "V,1000.0", "V,mil");
    rechazaConCambio("real2", "parametros.csv", "V,1000.0", "V,1000x");
    rechazaConCambio("real3", "parametros.csv", "V,1000.0", "V,");
    rechazaConCambio("entero", "parametros.csv", "etapas,3", "etapas,3.5");
    rechazaConCambio("entero2", "parametros.csv", "etapas,3", "etapas,tres");
    rechazaConCambio("entero3", "parametros.csv", "etapas,3", "etapas,");
    rechazaConCambio("especie", "agua.csv", "Nnit,0.0", "Xx,0.0");
    rechazaConCambio("etapa", "limites.csv", "Nnit,1,100.0,100.0", "Nnit,4,100.0,100.0");
    rechazaConCambio("etapa0", "limites.csv", "Nnit,1,100.0,100.0", "Nnit,0,100.0,100.0");
}

TEST(Instancia, RechazaInstanciasInconsistentes) {
    {
        const fs::path carpeta = copiaTemporal("inerte");
        anadirLinea(carpeta / "fertilizantes.csv", "Inerte,X,100.0,1.0,0.05,110000.0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
        EXPECT_THROW(Instancia::leer(carpeta.string()), std::runtime_error);
        fs::remove_all(carpeta);
    }
    rechazaConCambio("agua", "agua.csv", "K,0.0", "K,10000.0");
}

// TEST Solucion
TEST(Solucion, DosisEmpleoYSeleccion) {
    const Instancia& I = completa();
    Solucion x(I);
    EXPECT_EQ(static_cast<int>(x.q.size()), I.F * I.S);
    EXPECT_EQ(x.numSeleccionados(), 0);
    x.dosis(2, 1) = 5.0;
    const Solucion& lectura = x;
    EXPECT_EQ(x.q[2 * I.S + 1], 5.0);
    EXPECT_EQ(lectura.dosis(2, 1), 5.0);
    EXPECT_TRUE(x.seleccionado(2));
    EXPECT_FALSE(x.seleccionado(3));
    EXPECT_EQ(x.numSeleccionados(), 1);
    EXPECT_EQ(Solucion().numSeleccionados(), 0);
}

// TEST FuncionObjetivo
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

// TEST OperadorReparacion
TEST(OperadorReparacion, DominioDisjuntoDeLaDosisMinima) {
    const Instancia& I = completa();
    const int f = fertilizante(I, "Calciumnitrate");
    const double eps = I.fertilizantes[f].epsilon;
    Solucion x(I);
    x.dosis(f, 0) = 0.4 * eps;
    x.dosis(f, 1) = 0.6 * eps;
    x.dosis(f, 2) = 2.0 * eps;
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
    ASSERT_LE(rep.sumaCationes(x, 0), I.L);
    int menor = elegidos[0];
    for (int f : elegidos)
        if (rep.aporteRelativo(x, f) < rep.aporteRelativo(x, menor)) menor = f;
    for (int f : elegidos) {
        if (f != menor) {
            ASSERT_NE(rep.aporteRelativo(x, f), rep.aporteRelativo(x, menor));
        }
    }
    rep.reparar(x);
    EXPECT_EQ(x.numSeleccionados(), I.k);
    EXPECT_FALSE(x.seleccionado(menor));
}

TEST(OperadorReparacion, AjustaLaDosisALaCotaSuperior) {
    const fs::path carpeta = instanciaMinima("delta", 1, {filaNitratoDePotasio("Acotado", 5.76, 10.0)});
    const Instancia I = Instancia::leer(carpeta.string());
    Solucion x(I);
    x.dosis(0, 0) = 25.0;
    OperadorReparacion(I).reparar(x);
    EXPECT_EQ(x.dosis(0, 0), 10.0);
    fs::remove_all(carpeta);
}

TEST(OperadorReparacion, EscalaLaEtapaQueExcedeLaConductividad) {
    const Instancia& I = completa();
    const OperadorReparacion rep(I);
    const int ca = fertilizante(I, "Calciumnitrate"), k = fertilizante(I, "Potassiumnitrate");
    Solucion x(I);
    x.dosis(ca, 0) = 20000.0;
    x.dosis(k, 0) = 0.5;
    x.dosis(ca, 1) = 100.0;
    const double lambda = I.L / rep.sumaCationes(x, 0);
    ASSERT_LT(lambda, 1.0);
    ASSERT_LT(0.5 * lambda, I.fertilizantes[k].epsilon);
    rep.reparar(x);
    EXPECT_NEAR(x.dosis(ca, 0), 20000.0 * lambda, 1e-9);
    EXPECT_EQ(x.dosis(k, 0), 0.0);
    EXPECT_EQ(x.dosis(ca, 1), 100.0);
    EXPECT_LE(rep.sumaCationes(x, 0), I.L + 1e-9);
}

TEST(OperadorReparacion, DesempataPorPrecioYPorIndice) {
    const fs::path a = instanciaMinima("precio", 1, {filaNitratoDePotasio("Barato", 5.0, 110000.0),
                                                     filaNitratoDePotasio("Caro", 6.0, 110000.0)});
    const Instancia I = Instancia::leer(a.string());
    Solucion x(I);
    x.dosis(0, 0) = x.dosis(1, 0) = 1.0;
    OperadorReparacion(I).reparar(x);
    EXPECT_TRUE(x.seleccionado(0));
    EXPECT_FALSE(x.seleccionado(1));
    fs::remove_all(a);

    const fs::path c = instanciaMinima("precio2", 1, {filaNitratoDePotasio("Caro", 6.0, 110000.0),
                                                      filaNitratoDePotasio("Barato", 5.0, 110000.0)});
    const Instancia K = Instancia::leer(c.string());
    Solucion z(K);
    z.dosis(0, 0) = z.dosis(1, 0) = 1.0;
    OperadorReparacion(K).reparar(z);
    EXPECT_FALSE(z.seleccionado(0));
    EXPECT_TRUE(z.seleccionado(1));
    fs::remove_all(c);

    const fs::path b = instanciaMinima("indice", 1, {filaNitratoDePotasio("Primero", 5.0, 110000.0),
                                                     filaNitratoDePotasio("Segundo", 5.0, 110000.0)});
    const Instancia J = Instancia::leer(b.string());
    Solucion y(J);
    y.dosis(0, 0) = y.dosis(1, 0) = 1.0;
    OperadorReparacion(J).reparar(y);
    EXPECT_TRUE(y.seleccionado(0));
    EXPECT_FALSE(y.seleccionado(1));
    fs::remove_all(b);
}

// TEST AlgoritmoSSA
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

TEST(AlgoritmoSSA, PosicionInicialConMenosFertilizantesQueK) {
    const fs::path carpeta = instanciaMinima("pocos", 5, {filaNitratoDePotasio("Uno", 5.0, 110000.0),
                                                          filaNitratoDePotasio("Dos", 6.0, 110000.0)});
    const Instancia I = Instancia::leer(carpeta.string());
    Aleatorio azar(9);
    const Solucion x = AlgoritmoSSA(I, ParametrosSSA{}).posicionInicial(azar);
    EXPECT_EQ(x.numSeleccionados(), 2);
    fs::remove_all(carpeta);
}

TEST(AlgoritmoSSA, RechazaParametrosNoPositivos) {
    const Instancia& I = completa();
    ParametrosSSA sinAranas, sinIteraciones;
    sinAranas.poblacion = 0;
    sinIteraciones.iteraciones = 0;
    EXPECT_THROW({ const AlgoritmoSSA a(I, sinAranas); }, std::invalid_argument);
    EXPECT_THROW({ const AlgoritmoSSA a(I, sinIteraciones); }, std::invalid_argument);
}

TEST(AlgoritmoSSA, DistanciaYPromedioDeDesviaciones) {
    EXPECT_DOUBLE_EQ(AlgoritmoSSA::distancia({0.0, 1.0, 2.0}, {3.0, -1.0, 2.0}), 5.0);
    std::vector<Vibracion> v(2);
    v[0].origen = {0.0, 0.0};
    v[1].origen = {2.0, 4.0};
    EXPECT_DOUBLE_EQ(AlgoritmoSSA::promedioDesviaciones(v), 1.5);
}

// TEST Arana
TEST(Arana, AdoptaLaVibracionMasIntensaYDescartaLaDeSuObjetivo) {
    Solucion x;
    x.q = {0.0, 0.0};
    Arana arana(x);
    std::vector<Vibracion> v(3);
    v[0].origen = {0.0, 0.0};
    v[0].logIntensidad = -1.0;
    v[1].origen = {1.0, 0.0};
    v[1].logIntensidad = -2.0;
    v[2].origen = {5.0, 0.0};
    v[2].logIntensidad = -1.5;
    const std::vector<double> d = {0.0, 1.0, 5.0};
    EXPECT_TRUE(arana.elegirVibracion(v, d, 1.0, 1.0));
    EXPECT_EQ(arana.objetivo.origen, v[1].origen);
    EXPECT_DOUBLE_EQ(arana.objetivo.logIntensidad, -3.0);
    EXPECT_EQ(arana.c, 0);
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
    EXPECT_EQ(Arana::reflejar(3.0, 2.0, 10.0, azar), 3.0);
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
    arana.mascara.assign(arana.mascara.size(), 1);
    for (int paso = 0; paso < 20; ++paso) {
        arana.caminar(v, I.beta, azar);
        for (int i = 0; i < I.dimension(); ++i) {
            ASSERT_GE(arana.posicion.q[i], 0.0);
            ASSERT_LE(arana.posicion.q[i], I.beta[i]);
        }
    }
}

TEST(Arana, RegeneraLaMascaraYReiniciaElContador) {
    Solucion x;
    x.q.assign(1000, 0.0);
    Arana arana(x);
    Aleatorio azar(13);
    arana.c = 60;
    EXPECT_TRUE(arana.cambiarMascara(0.7, 1.0, azar));
    EXPECT_EQ(arana.c, 0);
    int unos = 0;
    for (unsigned char bit : arana.mascara) unos += bit;
    EXPECT_GT(unos, 0);
    EXPECT_LT(unos, 1000);
    arana.c = 60;
    EXPECT_TRUE(arana.cambiarMascara(0.7, 0.0, azar));
    for (unsigned char bit : arana.mascara) EXPECT_EQ(bit, 0);
}

TEST(Arana, IntensidadRecibidaSegunLaDistancia) {
    EXPECT_EQ(Arana::logIntensidadRecibida(-1.0, 0.0, 5.0, 1.0), -1.0);  // la propia, sin atenuar
    EXPECT_DOUBLE_EQ(Arana::logIntensidadRecibida(-1.0, 2.0, 4.0, 0.5), -2.0);
    EXPECT_EQ(Arana::logIntensidadRecibida(-1.0, 2.0, 0.0, 1.0), -std::numeric_limits<double>::infinity());
}

TEST(Arana, ReflexionEnLosCasosLimite) {
    Aleatorio azar(17);
    EXPECT_EQ(Arana::reflejar(5.0, 1.0, 0.0, azar), 0.0);  // cota nula
    for (int i = 0; i < 100; ++i) {
        const double arriba = Arana::reflejar(15.0, 12.0, 10.0, azar);
        const double abajo = Arana::reflejar(-1.0, 30.0, 10.0, azar);
        EXPECT_GE(arriba, 0.0);
        EXPECT_LE(arriba, 10.0);
        EXPECT_GE(abajo, 0.0);
        EXPECT_LE(abajo, 10.0);
    }
}

// TEST AlgoritmoSSA
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

// TEST Aleatorio
TEST(Aleatorio, SecuenciasReproduciblesYEnSusIntervalos) {
    Aleatorio a(42), b(42);
    EXPECT_EQ(a.semilla(), 42u);
    for (int i = 0; i < 1000; ++i) {
        const double u = a.uniforme();
        EXPECT_EQ(u, b.uniforme());
        EXPECT_GE(u, 0.0);
        EXPECT_LT(u, 1.0);
        const double v = a.uniforme(2.0, 5.0);
        b.uniforme(2.0, 5.0);
        EXPECT_GE(v, 2.0);
        EXPECT_LT(v, 5.0);
        const int e = a.entero(7);
        b.entero(7);
        EXPECT_GE(e, 0);
        EXPECT_LT(e, 7);
    }
    std::vector<int> m = a.muestra(10, 4);
    ASSERT_EQ(m.size(), 4u);
    std::sort(m.begin(), m.end());
    EXPECT_EQ(std::unique(m.begin(), m.end()), m.end());
    EXPECT_GE(m.front(), 0);
    EXPECT_LT(m.back(), 10);
    EXPECT_EQ(derivarSemilla("caso", 1), derivarSemilla("caso", 1));
    EXPECT_NE(derivarSemilla("caso", 1), derivarSemilla("caso", 2));
    EXPECT_NE(derivarSemilla("caso", 1), derivarSemilla("otro", 1));
}

// TEST ModuloEjecucion
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
        Aleatorio azar(std::stoull(campos[1]));
        const ResultadoSSA r = AlgoritmoSSA(I, c.parametros).ejecutar(azar);
        EXPECT_EQ(r.mejor.Z, std::strtod(campos[2].c_str(), nullptr));
    }
    EXPECT_EQ(filas, 3);
    archivo.close();
    fs::remove(salida);
}

TEST(ModuloEjecucion, RechazaConfiguracionesInvalidas) {
    const fs::path ruta = fs::temp_directory_path() / "ssa_basico_parametros.csv";
    std::ofstream(ruta) << "clave,valor\npoblacion,30\nra,1.0\npc,0.7\niteraciones,10\nrepeticiones,3\n";
    EXPECT_THROW(ModuloEjecucion::leerConfiguracion(ruta.string()), std::runtime_error);
    for (const char* valores : {"poblacion,0\niteraciones,10\nrepeticiones,3", "poblacion,30\niteraciones,0\nrepeticiones,3",
                                       "poblacion,30\niteraciones,10\nrepeticiones,0"}) {
        std::ofstream(ruta) << "clave,valor\nra,1.0\npc,0.7\npm,0.1\n" << valores << '\n';
        EXPECT_THROW(ModuloEjecucion::leerConfiguracion(ruta.string()), std::runtime_error) << valores;
    }
    fs::remove(ruta);
}

TEST(ModuloEjecucion, MuestraLaTablaPorPantallaYRechazaUnaSalidaInvalida) {
    const Instancia& I = completa();
    ConfiguracionEjecucion c;
    c.parametros.poblacion = 5;
    c.parametros.iteraciones = 5;
    c.repeticiones = 2;
    const fs::path pantalla = fs::temp_directory_path() / "ssa_basico_pantalla.txt";
    const fs::path salida = fs::temp_directory_path() / "ssa_basico_salida2.csv";
    std::FILE* archivo = std::fopen(pantalla.string().c_str(), "w");
    ASSERT_NE(archivo, nullptr);
    ModuloEjecucion::ejecutar(I, c, salida.string(), archivo);
    std::fclose(archivo);
    std::ifstream lectura(pantalla);
    std::string linea;
    int lineas = 0;
    std::getline(lectura, linea);
    EXPECT_EQ(linea.rfind("completa: 54 fertilizantes x 3 etapas", 0), 0u);
    for (lineas = 1; std::getline(lectura, linea);) ++lineas;
    EXPECT_EQ(lineas, 4);
    lectura.close();
    fs::remove(pantalla);
    fs::remove(salida);
    const fs::path invalida = fs::temp_directory_path() / "ssa_basico_no_existe" / "salida.csv";
    EXPECT_THROW(ModuloEjecucion::ejecutar(I, c, invalida.string(), nullptr), std::runtime_error);
}

// TEST Programa (integración)
TEST(Programa, CodigosDeSalidaDeSsaBasico) {
    const fs::path parametros = fs::temp_directory_path() / "ssa_basico_programa.csv";
    const fs::path salida = fs::temp_directory_path() / "ssa_basico_programa_salida.csv";
    std::ofstream(parametros) << "clave,valor\npoblacion,5\nra,1.0\npc,0.7\npm,0.1\niteraciones,5\nrepeticiones,2\n";
    const std::string comunes = " \"" + parametros.string() + "\" \"" + salida.string() + "\"";
    EXPECT_EQ(ejecutarPrograma(""), 2);
    EXPECT_EQ(ejecutarPrograma(" \"" + DATOS + "/no_existe\"" + comunes), 1);
    EXPECT_EQ(ejecutarPrograma(" \"" + COMPLETA + "\"" + comunes), 0);
    std::ifstream lectura(salida);
    std::string linea;
    int filas = 0;
    while (std::getline(lectura, linea)) ++filas;
    EXPECT_EQ(filas, 3);
    lectura.close();
    fs::remove(parametros);
    fs::remove(salida);
}
