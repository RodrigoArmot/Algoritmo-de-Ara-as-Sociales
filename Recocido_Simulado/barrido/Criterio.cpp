#include "recocido/barrido/Criterio.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "nucleo/Csv.h"

namespace barrido {

std::vector<RangoPublicado> rangosPublicados(const std::filesystem::path& tabla5) {
    const auto tabla = nucleo::csv::leer(tabla5);
    const auto cG = static_cast<std::size_t>(tabla.columna("gamma"));
    const auto cR = static_cast<std::size_t>(tabla.columna("RE"));
    const auto cC = static_cast<std::size_t>(tabla.columna("costo"));
    std::vector<RangoPublicado> rangos;
    for (double gamma : {0.65, 0.75, 0.85}) {
        std::vector<double> re, costo;
        for (const auto& fila : tabla.filas) {
            if (nucleo::csv::aReal(fila[cG], "gamma") != gamma) continue;
            re.push_back(nucleo::csv::aReal(fila[cR], "RE"));
            costo.push_back(nucleo::csv::aReal(fila[cC], "costo"));
        }
        if (re.size() != 10) throw std::runtime_error("La Tabla 5 debería tener diez filas por γ");
        rangos.push_back({gamma,
                          {*std::min_element(re.begin(), re.end()), *std::max_element(re.begin(), re.end())},
                          {*std::min_element(costo.begin(), costo.end()), *std::max_element(costo.begin(), costo.end())}});
    }
    return rangos;
}

double mediana(std::vector<double> valores) {
    if (valores.empty()) throw std::invalid_argument("mediana de una muestra vacía");
    std::sort(valores.begin(), valores.end());
    const std::size_t m = valores.size() / 2;
    return valores.size() % 2 == 1 ? valores[m] : (valores[m - 1] + valores[m]) / 2.0;
}

double distanciaLog(double valor, const Rango& rango) {
    if (valor >= rango.min && valor <= rango.max) return 0.0;
    if (!(valor > 0.0)) return kTopeDecadas;
    const double d = valor < rango.min ? std::log10(rango.min / valor) : std::log10(valor / rango.max);
    return std::min(d, kTopeDecadas);
}

bool cumple(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados) {
    for (std::size_t g = 0; g < publicados.size(); ++g) {
        const Medianas& m = medianas[g];
        const RangoPublicado& r = publicados[g];
        if (m.RE < r.RE.min || m.RE > r.RE.max || m.costo < r.costo.min || m.costo > r.costo.max) return false;
    }
    return true;
}

double distancia(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados) {
    double d = 0.0;
    for (std::size_t g = 0; g < publicados.size(); ++g)
        d += distanciaLog(medianas[g].RE, publicados[g].RE) + distanciaLog(medianas[g].costo, publicados[g].costo);
    return d;
}

bool pasaCribado(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados) {
    for (std::size_t g = 0; g < publicados.size(); ++g)
        if (distanciaLog(medianas[g].RE, publicados[g].RE) > 1.0) return false;
    return true;
}

}  // namespace barrido
