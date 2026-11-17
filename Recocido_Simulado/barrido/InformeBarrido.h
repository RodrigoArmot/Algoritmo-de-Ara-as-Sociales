#pragma once

#include <filesystem>
#include <vector>

#include "recocido/barrido/Criterio.h"

namespace barrido {

void escribirResumenEInforme(const std::filesystem::path& carpeta, const std::vector<RangoPublicado>& publicados);

}  // namespace barrido
