#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "recocido/barrido/ConfiguracionBarrido.h"
#include "recocido/barrido/Criterio.h"
#include "recocido/barrido/EjecutorBarrido.h"

namespace barrido {

// ------------------------------------------------------------------ filas en CSV
// Una fila por (configuración, γ, repetición): los ocho factores en columnas, RE y costo de la
// solución final y de la mejor encontrada, evaluaciones, semilla y los diagnósticos.
std::string encabezadoFilas();
std::string lineaFila(const Fila& fila);
std::vector<Fila> leerFilas(const std::filesystem::path& ruta);

// ------------------------------------------------------------------ resumen por configuración
struct EstadisticosGamma {
    double medianaRE = 0.0, minRE = 0.0, maxRE = 0.0;
    double medianaCosto = 0.0, minCosto = 0.0, maxCosto = 0.0;
};

struct ResumenConfiguracion {
    std::string fase;
    ConfiguracionBarrido configuracion;
    int repeticiones = 0;                  // por γ
    std::vector<EstadisticosGamma> final;  // por γ, tomando la solución final
    std::vector<EstadisticosGamma> mejor;  // por γ, tomando la mejor encontrada
    bool cumpleFinal = false, cumpleMejor = false;
    bool cribadoFinal = false, cribadoMejor = false;
    double distanciaFinal = 0.0, distanciaMejor = 0.0;
    long evaluaciones = 0;  // de todas sus ejecuciones

    double distanciaMinima() const { return std::min(distanciaFinal, distanciaMejor); }
    bool cumpleAlguna() const { return cumpleFinal || cumpleMejor; }
    bool cribadoAlguna() const { return cribadoFinal || cribadoMejor; }
};

// Una por (fase, configuración), en el orden en que aparecen en las filas.
std::vector<ResumenConfiguracion> resumir(const std::vector<Fila>& filas,
                                          const std::vector<RangoPublicado>& publicados);

// ------------------------------------------------------------------ efectos principales
struct EfectoPrincipal {
    std::string factor;
    std::vector<std::pair<std::string, double>> mediaPorNivel;  // distancia media de cada nivel
    double amplitud = 0.0;                                      // mayor media menos menor media
};

// Efecto de cada factor sobre la distancia a los rangos publicados (de la solución final o de la
// mejor): la media de la distancia en cada nivel. Pensado para el diseño equilibrado de la fase 1.
std::vector<EfectoPrincipal> efectosPrincipales(const std::vector<ResumenConfiguracion>& resumenes, bool mejor);

}  // namespace barrido
