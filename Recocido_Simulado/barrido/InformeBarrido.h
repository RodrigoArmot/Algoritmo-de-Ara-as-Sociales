#pragma once

#include <filesystem>
#include <vector>

#include "recocido/barrido/Criterio.h"

namespace barrido {

// Lee los barrido_fase*.csv que existan en la carpeta y escribe:
//   barrido_resumen.csv — una fila por configuración, con las medianas y los rangos por γ de la fase más
//                         avanzada en que corrió, si cumple el criterio en cada fase y la distancia a los
//                         rangos publicados, ordenadas por esa distancia;
//   barrido_informe.md  — cuántas pasaron cada fase, las diez más cercanas, los efectos principales y la
//                         conclusión en una frase.
void escribirResumenEInforme(const std::filesystem::path& carpeta, const std::vector<RangoPublicado>& publicados);

}  // namespace barrido
