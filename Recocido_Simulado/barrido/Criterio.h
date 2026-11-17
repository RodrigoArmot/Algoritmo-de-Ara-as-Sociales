#pragma once

#include <filesystem>
#include <vector>

namespace barrido {

struct Rango {
    double min = 0.0;
    double max = 0.0;
};

struct RangoPublicado {
    double gamma = 0.0;
    Rango RE;
    Rango costo;
};

struct Medianas {
    double RE = 0.0;
    double costo = 0.0;
};

std::vector<RangoPublicado> rangosPublicados(const std::filesystem::path& tabla5);

double mediana(std::vector<double> valores);

constexpr double kTopeDecadas = 3.0;
double distanciaLog(double valor, const Rango& rango);

bool cumple(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados);

double distancia(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados);

bool pasaCribado(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados);

}  // namespace barrido
