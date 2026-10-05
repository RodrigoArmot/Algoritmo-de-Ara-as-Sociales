#pragma once

#include <map>
#include <string>
#include <vector>

using namespace std;

struct Especie {
    string simbolo;
    double pesoAtomico = 0.0;
    int carga = 0;
};

struct Fertilizante {
    string nombre;
    double pesoMolecular = 0.0;
    double precio = 0.0;
    double dosisMin = 0.0;
    double dosisMax = 0.0;
    vector<int> atomosPorEspecie;
};

struct Aporte {
    int especie;
    double aporteUnitario;
};

class Instancia {
public:
    static Instancia leer(const string& carpeta);

    int dimension() const { return numFertilizantes * numEtapas; }

    string nombre;
    vector<Especie> especies;
    vector<Fertilizante> fertilizantes;
    int numEspecies = 0, numFertilizantes = 0, numEtapas = 0;
    double volumen = 0.0;
    double conductividadMax = 0.0;
    double pesoDesviacion = 0.0;
    double costoReferencia = 0.0;
    int maxFertilizantes = 0;
    vector<double> concMin, concMax, concRef;
    vector<double> concAgua;

    vector<vector<Aporte>> aportes;
    vector<double> coefCationes;
    double limiteCationes = 0.0;
    double cationesAgua = 0.0;
    double desviacionInicial = 0.0;
    vector<double> dosisLimite;

private:
    void calcularDerivados();
    void validar() const;
};

map<string, string> leerClaveValor(const string& ruta);

double aReal(const string& texto, const string& contexto);
int aEntero(const string& texto, const string& contexto);
