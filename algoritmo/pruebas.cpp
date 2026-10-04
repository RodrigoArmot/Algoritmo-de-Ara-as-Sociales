#include <cmath>
#include <cstdio>
#include <exception>
#include <string>

#include "Aleatorio.h"
#include "Instancia.h"
#include "Modelo.h"
#include "SSA.h"

using namespace std;

namespace {

int pruebasFallidas = 0;

void comprobar(bool condicion, const char* nombre) {
    printf("[%s] %s\n", condicion ? " OK " : "FALLA", nombre);
    if (!condicion) ++pruebasFallidas;
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

bool admisible(const Instancia& instancia, const Solucion& solucion) {
    if (numSeleccionados(instancia, solucion) > instancia.maxFertilizantes) return false;
    for (int fertilizante = 0; fertilizante < instancia.numFertilizantes; ++fertilizante)
        for (int etapa = 0; etapa < instancia.numEtapas; ++etapa) {
            const double dosis = solucion.dosis[fertilizante * instancia.numEtapas + etapa];
            if (dosis != 0.0 && (dosis < instancia.fertilizantes[fertilizante].dosisMin ||
                                 dosis > instancia.fertilizantes[fertilizante].dosisMax))
                return false;
        }
    for (int etapa = 0; etapa < instancia.numEtapas; ++etapa)
        if (sumaCationes(instancia, solucion, etapa) > instancia.limiteCationes + 1e-9) return false;
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fputs("Uso: pruebas <carpeta de la instancia>\n", stderr);
        return 2;
    }
    try {
        const Instancia instancia = leerInstancia(argv[1]);

        {
            const int fertilizante = indiceFertilizante(instancia, "Calciumnitrate");
            const int especie = indiceEspecie(instancia, "Nnit");
            double aporteUnitario = 0.0;
            if (fertilizante >= 0 && especie >= 0)
                for (const Aporte& aporte : instancia.aportes[fertilizante])
                    if (aporte.especie == especie) aporteUnitario = aporte.aporteUnitario;
            comprobar(fabs(aporteUnitario - 0.1708) < 1e-4, "aporte unitario del nitrato de calcio");
        }

        {
            Solucion solucion;
            solucion.dosis.assign(instancia.dimension(), 0.0);
            evaluar(instancia, solucion);
            comprobar(solucion.desviacion == instancia.desviacionInicial && solucion.costo == 0.0 &&
                          solucion.costoRelativo == -1.0 &&
                          fabs(solucion.objetivo - (instancia.pesoDesviacion * instancia.desviacionInicial -
                                                    (1.0 - instancia.pesoDesviacion))) < 1e-12,
                      "función objetivo de la dosificación nula");
        }

        {
            Aleatorio azar(1);
            bool todasAdmisibles = true;
            for (int prueba = 0; prueba < 200; ++prueba) {
                Solucion solucion;
                solucion.dosis.assign(instancia.dimension(), 0.0);
                for (int i = 0; i < instancia.dimension(); ++i)
                    solucion.dosis[i] =
                        azar.uniforme(0.0, 2.0 * (isfinite(instancia.dosisLimite[i]) ? instancia.dosisLimite[i] : 1000.0));
                reparar(instancia, solucion);
                todasAdmisibles = todasAdmisibles && admisible(instancia, solucion);
            }
            comprobar(todasAdmisibles, "la reparación deja posiciones admisibles");
        }

        ParametrosSSA parametros;
        parametros.poblacion = 10;
        parametros.iteraciones = 50;
        Aleatorio azar1(12345), azar2(12345), azar3(12346);
        const ResultadoSSA resultado1 = ejecutarSSA(instancia, parametros, azar1);
        const ResultadoSSA resultado2 = ejecutarSSA(instancia, parametros, azar2);
        const ResultadoSSA resultado3 = ejecutarSSA(instancia, parametros, azar3);

        comprobar(resultado1.mejor.dosis == resultado2.mejor.dosis &&
                      resultado1.mejor.objetivo == resultado2.mejor.objetivo,
                  "la misma semilla reproduce la misma solución");
        comprobar(resultado1.mejor.objetivo != resultado3.mejor.objetivo, "otra semilla da otra ejecución");
        comprobar(admisible(instancia, resultado1.mejor) && resultado1.iteraciones == 50 &&
                      resultado1.evaluaciones == 500,
                  "la mejor solución es admisible y se respetan iteraciones y evaluaciones");
    } catch (const exception& error) {
        fprintf(stderr, "Error: %s\n", error.what());
        return 1;
    }
    printf("%s\n", pruebasFallidas == 0 ? "Todas las pruebas aprobadas." : "Hay pruebas fallidas.");
    return pruebasFallidas == 0 ? 0 : 1;
}
