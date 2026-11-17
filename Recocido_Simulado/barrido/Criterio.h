#pragma once

#include <filesystem>
#include <vector>

namespace barrido {

struct Rango {
    double min = 0.0;
    double max = 0.0;
};

// Lo que publica la Tabla 5 de Guerra Ibarra et al. (2026) para un γ: el rango de sus diez RE y de sus
// diez costos.
struct RangoPublicado {
    double gamma = 0.0;
    Rango RE;
    Rango costo;
};

// Medianas de una configuración en un γ.
struct Medianas {
    double RE = 0.0;
    double costo = 0.0;
};

// Rangos de γ = 0,65, 0,75 y 0,85, con las cifras exactas del CSV exportado de la Tabla 5.
std::vector<RangoPublicado> rangosPublicados(const std::filesystem::path& tabla5);

double mediana(std::vector<double> valores);

// Distancia de un valor a un rango, en décadas: 0 dentro; log10(min/v) por debajo; log10(v/max) por
// encima; con un tope de tres décadas, de modo que estar mil veces fuera o más cuenta igual. Sin el
// tope, el costo cero de la dosificación nula valdría infinito y dominaría las medias de los efectos.
constexpr double kTopeDecadas = 3.0;
double distanciaLog(double valor, const Rango& rango);

// Criterio de éxito: en los tres γ a la vez, la mediana de RE dentro del rango publicado de RE y la
// mediana de costo dentro del de costos. Seis condiciones.
bool cumple(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados);

// Suma de las seis distancias: 0 si cumple.
double distancia(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados);

// Filtro de la fase 1: en los tres γ, la mediana de RE a menos de un factor de diez del rango publicado.
bool pasaCribado(const std::vector<Medianas>& medianas, const std::vector<RangoPublicado>& publicados);

}  // namespace barrido
