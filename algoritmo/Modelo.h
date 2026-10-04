#pragma once

#include <vector>

#include "Aleatorio.h"
#include "Instancia.h"

using namespace std;

struct Solucion {
    vector<double> dosis;
    double costo = 0.0;
    double desviacion = 0.0;
    double costoRelativo = 0.0;
    double objetivo = 0.0;
};

bool seleccionado(const Instancia& instancia, const Solucion& solucion, int fertilizante);
int numSeleccionados(const Instancia& instancia, const Solucion& solucion);

double sumaCationes(const Instancia& instancia, const Solucion& solucion, int etapa);

double aporteRelativo(const Instancia& instancia, const Solucion& solucion, int fertilizante);

void evaluar(const Instancia& instancia, Solucion& solucion);

void reparar(const Instancia& instancia, Solucion& solucion);

Solucion posicionInicial(const Instancia& instancia, Aleatorio& azar);
