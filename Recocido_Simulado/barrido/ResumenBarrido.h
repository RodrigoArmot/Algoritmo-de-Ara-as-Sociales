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

std::string encabezadoFilas();
std::string lineaFila(const Fila& fila);
std::vector<Fila> leerFilas(const std::filesystem::path& ruta);

struct EstadisticosGamma {
    double medianaRE = 0.0, minRE = 0.0, maxRE = 0.0;
    double medianaCosto = 0.0, minCosto = 0.0, maxCosto = 0.0;
};

struct ResumenConfiguracion {
    std::string fase;
    ConfiguracionBarrido configuracion;
    int repeticiones = 0;
    std::vector<EstadisticosGamma> final;
    std::vector<EstadisticosGamma> mejor;
    bool cumpleFinal = false, cumpleMejor = false;
    bool cribadoFinal = false, cribadoMejor = false;
    double distanciaFinal = 0.0, distanciaMejor = 0.0;
    long evaluaciones = 0;

    double distanciaMinima() const { return std::min(distanciaFinal, distanciaMejor); }
    bool cumpleAlguna() const { return cumpleFinal || cumpleMejor; }
    bool cribadoAlguna() const { return cribadoFinal || cribadoMejor; }
};

std::vector<ResumenConfiguracion> resumir(const std::vector<Fila>& filas,
                                          const std::vector<RangoPublicado>& publicados);

// ------------------------------------------------------------------ efectos principales
struct EfectoPrincipal {
    std::string factor;
    std::vector<std::pair<std::string, double>> mediaPorNivel;
    double amplitud = 0.0;
};

std::vector<EfectoPrincipal> efectosPrincipales(const std::vector<ResumenConfiguracion>& resumenes, bool mejor);

}  // namespace barrido
