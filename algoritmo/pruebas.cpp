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

using namespace std;

namespace fs = filesystem;

namespace {

const string DATOS = CARPETA_DATOS;
const string COMPLETA = DATOS + "/completa";

const Instancia& completa() {
    static const Instancia instancia = Instancia::leer(COMPLETA);
    return instancia;
}

int indiceFertilizante(const Instancia& instancia, const string& nombre) {
    for (int fertilizante = 0; fertilizante < instancia.numFertilizantes; ++fertilizante)
        if (instancia.fertilizantes[fertilizante].nombre == nombre) return fertilizante;
    return -1;
}

int indiceEspecie(const Instancia& instancia, const string& simbolo) {
    for (int especie = 0; especie < instancia.numEspecies; ++especie)
        if (instancia.especies[especie].simbolo == simbolo) return especie;
    return -1;
}

double aporteUnitario(const Instancia& instancia, int fertilizante, int especie) {
    for (const Aporte& aporte : instancia.aportes[fertilizante])
        if (aporte.especie == especie) return aporte.aporteUnitario;
    return 0.0;
}

::testing::AssertionResult admisible(const Instancia& instancia, const Solucion& solucion) {
    if (solucion.numSeleccionados() > instancia.maxFertilizantes)
        return ::testing::AssertionFailure() << "más de k fertilizantes";
    for (int fertilizante = 0; fertilizante < instancia.numFertilizantes; ++fertilizante)
        for (int etapa = 0; etapa < instancia.numEtapas; ++etapa) {
            const double dosis = solucion.dosis(fertilizante, etapa);
            if (dosis != 0.0 && (dosis < instancia.fertilizantes[fertilizante].dosisMin ||
                                 dosis > instancia.fertilizantes[fertilizante].dosisMax))
                return ::testing::AssertionFailure()
                       << "dosis fuera de dominio: " << instancia.fertilizantes[fertilizante].nombre;
        }
    const OperadorReparacion reparacion(instancia);
    for (int etapa = 0; etapa < instancia.numEtapas; ++etapa)
        if (reparacion.sumaCationes(solucion, etapa) > instancia.limiteCationes + 1e-9)
            return ::testing::AssertionFailure() << "conductividad en etapa " << etapa;
    return ::testing::AssertionSuccess();
}

fs::path copiaTemporal(const string& nombre) {
    const fs::path destino = fs::temp_directory_path() / ("ssa_basico_" + nombre);
    fs::remove_all(destino);
    fs::create_directories(destino);
    for (const auto& archivo : fs::directory_iterator(COMPLETA))
        fs::copy_file(archivo.path(), destino / archivo.path().filename());
    return destino;
}

void anadirLinea(const fs::path& archivo, const string& linea) {
    ofstream(archivo, ios::app) << linea << '\n';
}

}  // namespace

TEST(Instancia, LeeLaInstanciaCompleta) {
    const Instancia& instancia = completa();
    EXPECT_EQ(instancia.numFertilizantes, 54);
    EXPECT_EQ(instancia.numEspecies, 16);
    EXPECT_EQ(instancia.numEtapas, 3);
    EXPECT_EQ(instancia.maxFertilizantes, 16);
    EXPECT_DOUBLE_EQ(instancia.pesoDesviacion, 0.5);
}

TEST(Instancia, RechazaUnaCarpetaInexistente) {
    EXPECT_THROW(Instancia::leer(DATOS + "/no_existe"), runtime_error);
}

TEST(Instancia, RechazaUnArchivoMalFormado) {
    const fs::path carpeta = copiaTemporal("mal_formado");
    anadirLinea(carpeta / "fertilizantes.csv", "Incompleto,X,100.0,1.0");
    EXPECT_THROW(Instancia::leer(carpeta.string()), runtime_error);
    fs::remove_all(carpeta);
}

TEST(Instancia, RechazaUnLimiteInferiorMayorQueElSuperior) {
    const fs::path carpeta = copiaTemporal("limites");
    ifstream entrada(carpeta / "limites.csv");
    stringstream texto;
    texto << entrada.rdbuf();
    entrada.close();
    string contenido = texto.str();
    const string original = "Nnit,1,100.0,100.0";
    ASSERT_NE(contenido.find(original), string::npos);
    contenido.replace(contenido.find(original), original.size(), "Nnit,1,200.0,100.0");
    ofstream(carpeta / "limites.csv") << contenido;
    EXPECT_THROW(Instancia::leer(carpeta.string()), runtime_error);
    fs::remove_all(carpeta);
}

TEST(Instancia, MatrizDeAporteUnitario) {
    const Instancia& instancia = completa();
    const int fertilizante = indiceFertilizante(instancia, "Calciumnitrate");
    ASSERT_GE(fertilizante, 0);
    EXPECT_NEAR(aporteUnitario(instancia, fertilizante, indiceEspecie(instancia, "Nnit")), 0.1708, 5e-5);
    EXPECT_NEAR(aporteUnitario(instancia, fertilizante, indiceEspecie(instancia, "Ca")), 0.24429, 5e-6);
}

TEST(FuncionObjetivo, DosificacionNula) {
    const Instancia& instancia = completa();
    Solucion solucion(instancia);
    FuncionObjetivo(instancia).evaluar(solucion);
    EXPECT_EQ(solucion.desviacion, instancia.desviacionInicial);
    EXPECT_EQ(solucion.costo, 0.0);
    EXPECT_EQ(solucion.costoRelativo, -1.0);
    EXPECT_NEAR(solucion.aptitud,
                instancia.pesoDesviacion * instancia.desviacionInicial - (1.0 - instancia.pesoDesviacion), 1e-12);
}

TEST(FuncionObjetivo, LaIntensidadDecreceConLaAptitud) {
    EXPECT_GT(FuncionObjetivo::logIntensidad(-0.5), FuncionObjetivo::logIntensidad(0.0));
    EXPECT_GT(FuncionObjetivo::logIntensidad(0.0), FuncionObjetivo::logIntensidad(1.0));
    EXPECT_GT(FuncionObjetivo::logIntensidad(1.0), FuncionObjetivo::logIntensidad(10.0));
    EXPECT_TRUE(isfinite(FuncionObjetivo::logIntensidad(-0.99)));
}

TEST(OperadorReparacion, DominioDisjuntoDeLaDosisMinima) {
    const Instancia& instancia = completa();
    const int fertilizante = indiceFertilizante(instancia, "Calciumnitrate");
    const double dosisMin = instancia.fertilizantes[fertilizante].dosisMin;
    Solucion solucion(instancia);
    solucion.dosis(fertilizante, 0) = 0.4 * dosisMin;  // menos de la mitad: pasa a cero
    solucion.dosis(fertilizante, 1) = 0.6 * dosisMin;  // la mitad o más: sube a ε
    solucion.dosis(fertilizante, 2) = 2.0 * dosisMin;  // ya admisible: no cambia
    OperadorReparacion(instancia).reparar(solucion);
    EXPECT_EQ(solucion.dosis(fertilizante, 0), 0.0);
    EXPECT_EQ(solucion.dosis(fertilizante, 1), dosisMin);
    EXPECT_EQ(solucion.dosis(fertilizante, 2), 2.0 * dosisMin);
}

TEST(OperadorReparacion, DejaAdmisiblesPosicionesConTodosLosFertilizantes) {
    const Instancia& instancia = completa();
    const OperadorReparacion reparacion(instancia);
    Aleatorio azar(1);
    for (int prueba = 0; prueba < 200; ++prueba) {
        Solucion solucion(instancia);
        for (int i = 0; i < instancia.dimension(); ++i)
            solucion.vectorDosis[i] = azar.uniforme(0.0, 2.0 * instancia.dosisLimite[i]);
        ASSERT_GT(solucion.numSeleccionados(), instancia.maxFertilizantes);
        reparacion.reparar(solucion);
        ASSERT_TRUE(admisible(instancia, solucion)) << "prueba " << prueba;
    }
}

TEST(OperadorReparacion, RetiraElDeMenorAporteRelativo) {
    const Instancia& instancia = completa();
    const OperadorReparacion reparacion(instancia);
    Solucion solucion(instancia);
    vector<int> elegidos;
    for (int fertilizante = 0;
         fertilizante < instancia.numFertilizantes && static_cast<int>(elegidos.size()) <= instancia.maxFertilizantes;
         ++fertilizante) {
        Solucion prueba(instancia);
        prueba.dosis(fertilizante, 0) = 1.0;
        if (reparacion.aporteRelativo(prueba, fertilizante) > 0.0) {
            elegidos.push_back(fertilizante);
            solucion.dosis(fertilizante, 0) = 1.0;
        }
    }
    ASSERT_EQ(static_cast<int>(elegidos.size()), instancia.maxFertilizantes + 1);
    ASSERT_LE(reparacion.sumaCationes(solucion, 0), instancia.limiteCationes);  // la conductividad no interviene
    int menor = elegidos[0];
    for (int fertilizante : elegidos)
        if (reparacion.aporteRelativo(solucion, fertilizante) < reparacion.aporteRelativo(solucion, menor))
            menor = fertilizante;
    for (int fertilizante : elegidos) {
        if (fertilizante != menor) {
            ASSERT_NE(reparacion.aporteRelativo(solucion, fertilizante),
                      reparacion.aporteRelativo(solucion, menor));  // sin empates
        }
    }
    reparacion.reparar(solucion);
    EXPECT_EQ(solucion.numSeleccionados(), instancia.maxFertilizantes);
    EXPECT_FALSE(solucion.seleccionado(menor));
}

TEST(AlgoritmoSSA, PosicionInicialConKFertilizantesEntreEpsilonYBeta) {
    const Instancia& instancia = completa();
    const AlgoritmoSSA ssa(instancia, ParametrosSSA{});
    Aleatorio azar(7);
    for (int prueba = 0; prueba < 50; ++prueba) {
        const Solucion solucion = ssa.posicionInicial(azar);
        EXPECT_EQ(solucion.numSeleccionados(), instancia.maxFertilizantes);
        for (int fertilizante = 0; fertilizante < instancia.numFertilizantes; ++fertilizante)
            for (int etapa = 0; etapa < instancia.numEtapas; ++etapa) {
                const double dosis = solucion.dosis(fertilizante, etapa);
                if (dosis == 0.0) continue;
                EXPECT_GE(dosis, instancia.fertilizantes[fertilizante].dosisMin);
                EXPECT_LE(dosis, instancia.dosisLimite[fertilizante * instancia.numEtapas + etapa]);
            }
    }
}

TEST(Arana, AdoptaLaVibracionMasIntensaYDescartaLaDeSuObjetivo) {
    Solucion solucion;
    solucion.vectorDosis = {0.0, 0.0};
    Arana arana(solucion);
    vector<Vibracion> vibraciones(3);
    vibraciones[0].origen = {0.0, 0.0};  // la suya: procede de su objetivo y se descarta
    vibraciones[0].logIntensidad = -1.0;
    vibraciones[1].origen = {1.0, 0.0};
    vibraciones[1].logIntensidad = -2.0;
    vibraciones[2].origen = {5.0, 0.0};
    vibraciones[2].logIntensidad = -1.5;
    const vector<double> distancias = {0.0, 1.0, 5.0};
    EXPECT_TRUE(arana.elegirVibracion(vibraciones, distancias, 1.0, 1.0));
    EXPECT_EQ(arana.vibracionObjetivo.origen, vibraciones[1].origen);
    EXPECT_DOUBLE_EQ(arana.vibracionObjetivo.logIntensidad, -3.0);
    EXPECT_EQ(arana.iteracionesSinCambio, 0);
    arana.vibracionObjetivo.logIntensidad = 0.0;
    EXPECT_FALSE(arana.elegirVibracion(vibraciones, distancias, 1.0, 1.0));
    EXPECT_EQ(arana.iteracionesSinCambio, 1);
}

TEST(Arana, LaMascaraNoCambiaConElContadorEnCero) {
    Solucion solucion;
    solucion.vectorDosis.assign(10, 0.0);
    Arana arana(solucion);
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
    const Instancia& instancia = completa();
    const AlgoritmoSSA ssa(instancia, ParametrosSSA{});
    Aleatorio azar(11);
    vector<Vibracion> vibraciones(5);
    for (auto& vibracion : vibraciones) vibracion.origen = ssa.posicionInicial(azar).vectorDosis;
    Arana arana(ssa.posicionInicial(azar));
    arana.vibracionObjetivo.origen = vibraciones[0].origen;
    arana.mascara.assign(arana.mascara.size(), 1);  // también sigue a arañas al azar
    for (int paso = 0; paso < 20; ++paso) {
        arana.caminar(vibraciones, instancia.dosisLimite, azar);
        for (int i = 0; i < instancia.dimension(); ++i) {
            ASSERT_GE(arana.posicion.vectorDosis[i], 0.0);
            ASSERT_LE(arana.posicion.vectorDosis[i], instancia.dosisLimite[i]);
        }
    }
}

TEST(AlgoritmoSSA, ReproducibleConLaMismaSemillaYAdmisible) {
    const Instancia& instancia = completa();
    ParametrosSSA parametros;
    parametros.poblacion = 10;
    parametros.iteraciones = 50;
    const AlgoritmoSSA ssa(instancia, parametros);
    Aleatorio azar1(12345), azar2(12345), azar3(12346);
    const ResultadoSSA resultado1 = ssa.ejecutar(azar1), resultado2 = ssa.ejecutar(azar2),
                       resultado3 = ssa.ejecutar(azar3);
    EXPECT_EQ(resultado1.mejor.vectorDosis, resultado2.mejor.vectorDosis);
    EXPECT_EQ(resultado1.mejor.aptitud, resultado2.mejor.aptitud);
    EXPECT_NE(resultado1.mejor.aptitud, resultado3.mejor.aptitud);
    EXPECT_TRUE(admisible(instancia, resultado1.mejor));
    EXPECT_EQ(resultado1.iteraciones, 50);
    EXPECT_EQ(resultado1.evaluaciones, 500);
}

TEST(ModuloEjecucion, LeeLosParametrosDelAlgoritmo) {
    const ConfiguracionEjecucion configuracion = ModuloEjecucion::leerConfiguracion(DATOS + "/parametros_ssa.csv");
    EXPECT_EQ(configuracion.parametros.poblacion, 30);
    EXPECT_DOUBLE_EQ(configuracion.parametros.tasaAtenuacion, 1.0);
    EXPECT_DOUBLE_EQ(configuracion.parametros.probCambioMascara, 0.7);
    EXPECT_DOUBLE_EQ(configuracion.parametros.probMascara, 0.1);
    EXPECT_EQ(configuracion.parametros.iteraciones, 1000);
    EXPECT_EQ(configuracion.repeticiones, 30);
    EXPECT_THROW(ModuloEjecucion::leerConfiguracion(DATOS + "/no_existe.csv"), runtime_error);
}

TEST(ModuloEjecucion, EscribeUnaFilaPorRepeticionQueSeReproduceConSuSemilla) {
    const Instancia& instancia = completa();
    ConfiguracionEjecucion configuracion;
    configuracion.parametros.poblacion = 10;
    configuracion.parametros.iteraciones = 20;
    configuracion.repeticiones = 3;
    const fs::path salida = fs::temp_directory_path() / "ssa_basico_salida.csv";
    const vector<ResultadoSSA> resultados =
        ModuloEjecucion::ejecutar(instancia, configuracion, salida.string(), nullptr);
    ASSERT_EQ(resultados.size(), 3u);

    ifstream archivo(salida);
    string linea;
    getline(archivo, linea);
    EXPECT_EQ(linea, "repeticion,semilla,Z,D,K,costo,seleccionados,iteraciones,evaluaciones");
    int filas = 0;
    while (getline(archivo, linea)) {
        vector<string> campos;
        stringstream flujo(linea);
        for (string campo; getline(flujo, campo, ',');) campos.push_back(campo);
        ASSERT_EQ(campos.size(), 9u);
        ++filas;
        EXPECT_EQ(stoi(campos[0]), filas);
        EXPECT_LE(stoi(campos[6]), instancia.maxFertilizantes);
        EXPECT_EQ(stol(campos[8]), 200);
        EXPECT_TRUE(admisible(instancia, resultados[filas - 1].mejor));
        Aleatorio azar(stoull(campos[1]));
        const ResultadoSSA resultado = AlgoritmoSSA(instancia, configuracion.parametros).ejecutar(azar);
        EXPECT_EQ(resultado.mejor.aptitud, strtod(campos[2].c_str(), nullptr));
    }
    EXPECT_EQ(filas, 3);
    archivo.close();
    fs::remove(salida);
}
