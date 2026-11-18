#pragma once

#include <map>
#include <string>
#include <vector>

struct Especie {
    std::string simbolo;
    double pesoAtomico = 0.0;
    int carga = 0;
};

struct Fertilizante {
    std::string nombre;
    double mw = 0.0;
    double precio = 0.0;
    double epsilon = 0.0;
    double delta = 0.0;
    std::vector<int> atomos;
};

struct Aporte {
    int especie;
    double alfa;
};

class Instancia {
public:
    static Instancia leer(const std::string& carpeta);

    int dimension() const { return F * S; }

    std::string nombre;
    std::vector<Especie> especies;
    std::vector<Fertilizante> fertilizantes;
    int E = 0, F = 0, S = 0;
    double V = 0.0, Emax = 0.0, gamma = 0.0, Kref = 0.0;
    int k = 0;
    std::vector<double> Cmin, Cmax, Cref;
    std::vector<double> C0;

    std::vector<std::vector<Aporte>> aportes;
    std::vector<double> coefCationes;
    double L = 0.0;
    double u = 0.0;
    double D0 = 0.0;
    std::vector<double> beta;

private:
    void calcularDerivados();
    void validar() const;
};

std::map<std::string, std::string> leerClaveValor(const std::string& ruta);

double aReal(const std::string& texto, const std::string& contexto);
int aEntero(const std::string& texto, const std::string& contexto);
