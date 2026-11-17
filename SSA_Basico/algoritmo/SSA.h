// Algoritmo de Arañas Sociales adaptado al espacio de búsqueda mixto (R4, capítulo 5).
//
// La estructura de la iteración se deriva de SSA::run, SSA::fitness_calculation y
// SSA::vibration_generation de la implementación de referencia de James J. Q. Yu, publicada bajo
// licencia MIT (https://github.com/James-Yu/social-spider-algorithm, cpp/SSA.cpp); ver
// LICENCIAS_TERCEROS.txt. Son propios la población inicial, la cota de búsqueda y la reparación.
#pragma once

#include <cstdint>
#include <vector>

#include "Aleatorio.h"
#include "Arana.h"
#include "FuncionObjetivo.h"
#include "Instancia.h"
#include "OperadorReparacion.h"
#include "Solucion.h"

// Parámetros de control (Yu y Li, 2015a), con los valores de su implementación de referencia.
struct ParametrosSSA {
    int poblacion = 30;   // número de arañas
    double ra = 1.0;      // tasa de atenuación
    double pc = 0.7;      // probabilidad de conservar la máscara
    double pm = 0.1;      // tasa de máscara
    int iteraciones = 1000;
};

struct ResultadoSSA {
    Solucion mejor;  // la mejor solución evaluada
    int iteraciones = 0;
    long evaluaciones = 0;
    std::uint64_t semilla = 0;
};

class AlgoritmoSSA {
public:
    AlgoritmoSSA(const Instancia& instancia, const ParametrosSSA& parametros);

    // Una ejecución completa con el generador dado.
    ResultadoSSA ejecutar(Aleatorio& azar) const;

    // Posición inicial: k fertilizantes al azar y, para cada uno y cada etapa, una dosis al azar
    // entre ε_f y β_(f,s). No repara.
    Solucion posicionInicial(Aleatorio& azar) const;

    // Distancia de Manhattan entre dos posiciones (Yu y Li, 2015a, ec. 2).
    static double distancia(const std::vector<double>& a, const std::vector<double>& b);
    // Promedio, sobre las dimensiones, de la desviación estándar poblacional de las posiciones que
    // emitieron las vibraciones.
    static double promedioDesviaciones(const std::vector<Vibracion>& vibraciones);

private:
    const Instancia& I;
    ParametrosSSA P;
    FuncionObjetivo objetivo;
    OperadorReparacion reparacion;
};
