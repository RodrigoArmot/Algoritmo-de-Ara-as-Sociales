#pragma once

#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace std;

class Aleatorio {
public:
    explicit Aleatorio(uint64_t semilla) : motor_(semilla), semilla_(semilla) {}

    double uniforme() { return static_cast<double>(motor_() >> 11) * 0x1.0p-53; }
    double uniforme(double minimo, double maximo) { return minimo + (maximo - minimo) * uniforme(); }
    int entero(int limite) {
        const int valor = static_cast<int>(uniforme() * limite);
        return valor < limite ? valor : limite - 1;
    }
    vector<int> muestra(int total, int cantidad) {
        vector<int> indices(total);
        iota(indices.begin(), indices.end(), 0);
        for (int i = 0; i < cantidad; ++i) swap(indices[i], indices[i + entero(total - i)]);
        indices.resize(cantidad);
        return indices;
    }

    uint64_t semilla() const { return semilla_; }

private:
    mt19937_64 motor_;
    uint64_t semilla_;
};

inline uint64_t derivarSemilla(const string& caso, uint64_t repeticion) {
    uint64_t hash = 0xCBF29CE484222325ULL;
    for (unsigned char caracter : caso) {
        hash ^= caracter;
        hash *= 0x100000001B3ULL;
    }
    uint64_t mezcla = (hash ^ repeticion) + 0x9E3779B97F4A7C15ULL;
    mezcla = (mezcla ^ (mezcla >> 30)) * 0xBF58476D1CE4E5B9ULL;
    mezcla = (mezcla ^ (mezcla >> 27)) * 0x94D049BB133111EBULL;
    return mezcla ^ (mezcla >> 31);
}
