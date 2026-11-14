// Instancia del problema (R3): los datos que el algoritmo lee y no modifica. Sus derivados (matriz
// de aporte unitario, concentraciones de referencia, límite de conductividad, desviación de la
// dosificación nula y cota de búsqueda) se calculan una sola vez, al leerla.
#pragma once

#include <map>
#include <string>
#include <vector>

struct Especie {
    std::string simbolo;
    double pesoAtomico = 0.0;  // a_e, g/mol
    int carga = 0;             // carga del catión si la especie pertenece a E+; 0 si no
};

struct Fertilizante {
    std::string nombre;
    double mw = 0.0;          // peso molecular, g/mol
    double precio = 0.0;      // USD/kg
    double epsilon = 0.0;     // dosis mínima realizable, g
    double delta = 0.0;       // cota superior de la dosis, g
    std::vector<int> atomos;  // n_(e,f), en el orden de las especies
};

// Posición distinta de cero de la columna f de la matriz de aporte unitario.
struct Aporte {
    int especie;
    double alfa;  // α_(e,f) = 1000·a_e·n_(e,f)/(mw_f·V), mg/L por gramo
};

class Instancia {
public:
    // Lee parametros.csv, especies.csv, limites.csv, agua.csv y fertilizantes.csv de la carpeta, y
    // rechaza la instancia si es inconsistente.
    static Instancia leer(const std::string& carpeta);

    int dimension() const { return F * S; }

    std::string nombre;
    std::vector<Especie> especies;
    std::vector<Fertilizante> fertilizantes;
    int E = 0, F = 0, S = 0;  // número de especies, fertilizantes y etapas
    double V = 0.0, Emax = 0.0, gamma = 0.0, Kref = 0.0;
    int k = 0;
    std::vector<double> Cmin, Cmax, Cref;  // |E|·|S|, posición e·S + s
    std::vector<double> C0;                // agua de partida, |E|

    std::vector<std::vector<Aporte>> aportes;  // por fertilizante
    std::vector<double> coefCationes;          // Σ_{e∈E+} α_(e,f)/w_e, por fertilizante
    double L = 0.0;                            // 9,819·Emax − 1,462, meq/L
    double u = 0.0;                            // suma equivalente de cationes del agua
    double D0 = 0.0;                           // desviación de la dosificación nula
    std::vector<double> beta;                  // cota de búsqueda, |F|·|S|, posición f·S + s

private:
    void calcularDerivados();
    void validar() const;
};

// Lee un CSV de dos columnas, clave y valor.
std::map<std::string, std::string> leerClaveValor(const std::string& ruta);

double aReal(const std::string& texto, const std::string& contexto);
int aEntero(const std::string& texto, const std::string& contexto);
