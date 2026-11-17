// Generador de números al azar de una ejecución. Cada ejecución crea el suyo con su propia semilla.
// No se usan las distribuciones de la biblioteca estándar, cuyo algoritmo depende de la
// implementación: así la misma semilla da la misma secuencia con cualquier compilador.
#pragma once

#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

class Aleatorio {
public:
    explicit Aleatorio(std::uint64_t semilla) : motor_(semilla), semilla_(semilla) {}

    // Real uniforme en [0, 1), con 53 bits.
    double uniforme() { return static_cast<double>(motor_() >> 11) * 0x1.0p-53; }
    // Real entre a y b: a + (b − a)·u.
    double uniforme(double a, double b) { return a + (b - a) * uniforme(); }
    // Entero uniforme en [0, n).
    int entero(int n) {
        const int i = static_cast<int>(uniforme() * n);
        return i < n ? i : n - 1;
    }
    // k índices distintos de [0, n), al azar (Fisher-Yates parcial).
    std::vector<int> muestra(int n, int k) {
        std::vector<int> indices(n);
        std::iota(indices.begin(), indices.end(), 0);
        for (int i = 0; i < k; ++i) std::swap(indices[i], indices[i + entero(n - i)]);
        indices.resize(k);
        return indices;
    }

    std::uint64_t semilla() const { return semilla_; }

private:
    std::mt19937_64 motor_;
    std::uint64_t semilla_;
};

// Semilla de la repetición r de un caso: SplitMix64 (Steele, Lea y Flood, 2014) aplicado al FNV-1a
// del texto del caso combinado con r. Solo depende de esos dos datos.
inline std::uint64_t derivarSemilla(const std::string& caso, std::uint64_t r) {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    for (unsigned char c : caso) {
        h ^= c;
        h *= 0x100000001B3ULL;
    }
    std::uint64_t x = (h ^ r) + 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}
