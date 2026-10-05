#include "Instancia.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <stdexcept>

using namespace std;

namespace {

struct Tabla {
    string archivo;
    vector<string> encabezado;
    vector<vector<string>> filas;

    size_t columna(const string& nombre) const {
        for (size_t i = 0; i < encabezado.size(); ++i)
            if (encabezado[i] == nombre) return i;
        throw runtime_error(archivo + ": falta la columna '" + nombre + "'");
    }
};

string recortar(const string& texto) {
    const char* blancos = " \t\r\n";
    const auto inicio = texto.find_first_not_of(blancos);
    if (inicio == string::npos) return "";
    const auto fin = texto.find_last_not_of(blancos);
    return texto.substr(inicio, fin - inicio + 1);
}

Tabla leerCsv(const string& ruta) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo) throw runtime_error("No se pudo abrir " + ruta);
    Tabla tabla;
    tabla.archivo = ruta;
    string linea;
    while (getline(archivo, linea)) {
        if (tabla.encabezado.empty() && linea.rfind("\xEF\xBB\xBF", 0) == 0) linea.erase(0, 3);  // BOM
        if (recortar(linea).empty()) continue;
        vector<string> campos;
        size_t inicio = 0, posicionComa;
        while ((posicionComa = linea.find(',', inicio)) != string::npos) {
            campos.push_back(recortar(linea.substr(inicio, posicionComa - inicio)));
            inicio = posicionComa + 1;
        }
        campos.push_back(recortar(linea.substr(inicio)));
        if (tabla.encabezado.empty())
            tabla.encabezado = campos;
        else if (campos.size() != tabla.encabezado.size())
            throw runtime_error(ruta + ": una fila no tiene el número de campos del encabezado");
        else
            tabla.filas.push_back(campos);
    }
    if (tabla.encabezado.empty()) throw runtime_error(ruta + ": el archivo está vacío");
    return tabla;
}

int indiceEspecie(const Instancia& instancia, const string& simbolo) {
    for (int especie = 0; especie < instancia.numEspecies; ++especie)
        if (instancia.especies[especie].simbolo == simbolo) return especie;
    throw runtime_error("especie desconocida '" + simbolo + "'");
}

}  // namespace

void Instancia::calcularDerivados() {
    Instancia& instancia = *this;
    const int numEspecies = instancia.numEspecies;
    const int numFertilizantes = instancia.numFertilizantes;
    const int numEtapas = instancia.numEtapas;

    instancia.aportes.assign(numFertilizantes, {});
    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante) {
        const Fertilizante& datosFertilizante = instancia.fertilizantes[fertilizante];
        for (int especie = 0; especie < numEspecies; ++especie) {
            const int atomos = datosFertilizante.atomosPorEspecie[especie];
            if (atomos == 0) continue;
            instancia.aportes[fertilizante].push_back(
                {especie, 1000.0 * instancia.especies[especie].pesoAtomico * atomos /
                              (datosFertilizante.pesoMolecular * instancia.volumen)});
        }
    }

    instancia.concRef.assign(numEspecies * numEtapas, 0.0);
    for (int i = 0; i < numEspecies * numEtapas; ++i)
        instancia.concRef[i] = instancia.concMin[i] > 0.0 ? (instancia.concMin[i] + instancia.concMax[i]) / 2.0
                                                          : instancia.concMax[i] / 2.0;

    instancia.limiteCationes = 9.819 * instancia.conductividadMax - 1.462;

    instancia.cationesAgua = 0.0;
    for (int especie = 0; especie < numEspecies; ++especie) {
        const Especie& datosEspecie = instancia.especies[especie];
        if (datosEspecie.carga > 0)
            instancia.cationesAgua += instancia.concAgua[especie] / (datosEspecie.pesoAtomico / datosEspecie.carga);
    }

    instancia.coefCationes.assign(numFertilizantes, 0.0);
    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
        for (const Aporte& aporte : instancia.aportes[fertilizante]) {
            const Especie& datosEspecie = instancia.especies[aporte.especie];
            if (datosEspecie.carga > 0)
                instancia.coefCationes[fertilizante] +=
                    aporte.aporteUnitario / (datosEspecie.pesoAtomico / datosEspecie.carga);
        }

    instancia.desviacionInicial = 0.0;
    for (int etapa = 0; etapa < numEtapas; ++etapa)
        for (int especie = 0; especie < numEspecies; ++especie) {
            const int indice = especie * numEtapas + etapa;
            instancia.desviacionInicial +=
                (max(0.0, instancia.concMin[indice] - instancia.concAgua[especie]) +
                 max(0.0, instancia.concAgua[especie] - instancia.concMax[indice])) /
                instancia.concRef[indice];
        }

    instancia.dosisLimite.assign(numFertilizantes * numEtapas, numeric_limits<double>::infinity());
    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
        for (int etapa = 0; etapa < numEtapas; ++etapa)
            for (const Aporte& aporte : instancia.aportes[fertilizante]) {
                const int indice = aporte.especie * numEtapas + etapa;
                instancia.dosisLimite[fertilizante * numEtapas + etapa] =
                    min(instancia.dosisLimite[fertilizante * numEtapas + etapa],
                        (instancia.concMax[indice] + instancia.desviacionInicial * instancia.concRef[indice]) /
                            aporte.aporteUnitario);
            }
}

void Instancia::validar() const {
    for (int i = 0; i < numEspecies * numEtapas; ++i)
        if (concMin[i] > concMax[i])
            throw runtime_error(nombre + ": límite inferior mayor que el superior en " +
                                especies[i / numEtapas].simbolo);
    for (int fertilizante = 0; fertilizante < numFertilizantes; ++fertilizante)
        if (aportes[fertilizante].empty())
            throw runtime_error(nombre + ": " + fertilizantes[fertilizante].nombre +
                                " no aporta ninguna especie de la instancia");
    if (!(cationesAgua < limiteCationes))
        throw runtime_error(nombre + ": el agua de partida agota por sí sola el límite de conductividad");
}

double aReal(const string& texto, const string& contexto) {
    double valor = 0.0;
    const char* fin = texto.data() + texto.size();
    const auto [ptr, error] = from_chars(texto.data(), fin, valor);
    if (texto.empty() || error != errc() || ptr != fin)
        throw runtime_error(contexto + ": '" + texto + "' no es un número");
    return valor;
}

int aEntero(const string& texto, const string& contexto) {
    int valor = 0;
    const char* fin = texto.data() + texto.size();
    const auto [ptr, error] = from_chars(texto.data(), fin, valor);
    if (texto.empty() || error != errc() || ptr != fin)
        throw runtime_error(contexto + ": '" + texto + "' no es un entero");
    return valor;
}

map<string, string> leerClaveValor(const string& ruta) {
    const Tabla tabla = leerCsv(ruta);
    const size_t columnaClave = tabla.columna("clave"), columnaValor = tabla.columna("valor");
    map<string, string> valores;
    for (const auto& fila : tabla.filas) valores[fila[columnaClave]] = fila[columnaValor];
    return valores;
}

Instancia Instancia::leer(const string& carpeta) {
    Instancia instancia;
    string ruta = carpeta;
    while (!ruta.empty() && (ruta.back() == '/' || ruta.back() == '\\')) ruta.pop_back();
    instancia.nombre = ruta.substr(ruta.find_last_of("/\\") + 1);

    const auto parametros = leerClaveValor(ruta + "/parametros.csv");
    auto valor = [&parametros](const string& clave) {
        const auto it = parametros.find(clave);
        if (it == parametros.end()) throw runtime_error("parametros.csv: falta la clave '" + clave + "'");
        return it->second;
    };
    instancia.numEtapas = aEntero(valor("etapas"), "etapas");
    instancia.volumen = aReal(valor("V"), "V");
    instancia.conductividadMax = aReal(valor("Emax"), "Emax");
    instancia.maxFertilizantes = aEntero(valor("k"), "k");
    instancia.pesoDesviacion = aReal(valor("gamma"), "gamma");
    instancia.costoReferencia = aReal(valor("Kref"), "Kref");

    const Tabla tablaEspecies = leerCsv(ruta + "/especies.csv");
    for (const auto& fila : tablaEspecies.filas) {
        Especie especie;
        especie.simbolo = fila[tablaEspecies.columna("especie")];
        especie.pesoAtomico = aReal(fila[tablaEspecies.columna("peso_atomico")], "especies.csv");
        especie.carga = aEntero(fila[tablaEspecies.columna("carga_E_mas")], "especies.csv");
        instancia.especies.push_back(especie);
    }
    instancia.numEspecies = static_cast<int>(instancia.especies.size());

    const Tabla tablaLimites = leerCsv(ruta + "/limites.csv");
    if (static_cast<int>(tablaLimites.filas.size()) != instancia.numEspecies * instancia.numEtapas)
        throw runtime_error("limites.csv: se espera una fila por especie y etapa");
    instancia.concMin.assign(instancia.numEspecies * instancia.numEtapas, 0.0);
    instancia.concMax.assign(instancia.numEspecies * instancia.numEtapas, 0.0);
    for (const auto& fila : tablaLimites.filas) {
        const int especie = indiceEspecie(instancia, fila[tablaLimites.columna("especie")]);
        const int etapa = aEntero(fila[tablaLimites.columna("etapa")], "limites.csv") - 1;
        if (etapa < 0 || etapa >= instancia.numEtapas) throw runtime_error("limites.csv: etapa fuera de rango");
        instancia.concMin[especie * instancia.numEtapas + etapa] =
            aReal(fila[tablaLimites.columna("Cmin")], "limites.csv");
        instancia.concMax[especie * instancia.numEtapas + etapa] =
            aReal(fila[tablaLimites.columna("Cmax")], "limites.csv");
    }

    const Tabla tablaAgua = leerCsv(ruta + "/agua.csv");
    instancia.concAgua.assign(instancia.numEspecies, 0.0);
    for (const auto& fila : tablaAgua.filas)
        instancia.concAgua[indiceEspecie(instancia, fila[tablaAgua.columna("especie")])] =
            aReal(fila[tablaAgua.columna("C0")], "agua.csv");

    const Tabla tablaFertilizantes = leerCsv(ruta + "/fertilizantes.csv");
    for (const auto& fila : tablaFertilizantes.filas) {
        Fertilizante fertilizante;
        fertilizante.nombre = fila[tablaFertilizantes.columna("nombre")];
        fertilizante.pesoMolecular = aReal(fila[tablaFertilizantes.columna("mw")], fertilizante.nombre);
        fertilizante.precio = aReal(fila[tablaFertilizantes.columna("precio")], fertilizante.nombre);
        fertilizante.dosisMin = aReal(fila[tablaFertilizantes.columna("epsilon")], fertilizante.nombre);
        fertilizante.dosisMax = aReal(fila[tablaFertilizantes.columna("delta")], fertilizante.nombre);
        for (const Especie& especie : instancia.especies)
            fertilizante.atomosPorEspecie.push_back(
                aEntero(fila[tablaFertilizantes.columna(especie.simbolo)], fertilizante.nombre));
        instancia.fertilizantes.push_back(fertilizante);
    }
    instancia.numFertilizantes = static_cast<int>(instancia.fertilizantes.size());

    instancia.calcularDerivados();
    instancia.validar();
    return instancia;
}
