#include "Instancia.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace {

// CSV sencillo: separador coma, sin comillas y con encabezado en la primera fila.
struct Tabla {
    std::string archivo;
    std::vector<std::string> encabezado;
    std::vector<std::vector<std::string>> filas;

    std::size_t columna(const std::string& nombre) const {
        for (std::size_t i = 0; i < encabezado.size(); ++i)
            if (encabezado[i] == nombre) return i;
        throw std::runtime_error(archivo + ": falta la columna '" + nombre + "'");
    }
};

std::string recortar(const std::string& texto) {
    const char* blancos = " \t\r\n";
    const auto inicio = texto.find_first_not_of(blancos);
    if (inicio == std::string::npos) return "";
    const auto fin = texto.find_last_not_of(blancos);
    return texto.substr(inicio, fin - inicio + 1);
}

Tabla leerCsv(const std::string& ruta) {
    std::ifstream archivo(ruta, std::ios::binary);
    if (!archivo) throw std::runtime_error("No se pudo abrir " + ruta);
    Tabla tabla;
    tabla.archivo = ruta;
    std::string linea;
    while (std::getline(archivo, linea)) {
        if (tabla.encabezado.empty() && linea.rfind("\xEF\xBB\xBF", 0) == 0) linea.erase(0, 3);  // BOM
        if (recortar(linea).empty()) continue;
        std::vector<std::string> campos;
        std::size_t inicio = 0, coma;
        while ((coma = linea.find(',', inicio)) != std::string::npos) {
            campos.push_back(recortar(linea.substr(inicio, coma - inicio)));
            inicio = coma + 1;
        }
        campos.push_back(recortar(linea.substr(inicio)));
        if (tabla.encabezado.empty())
            tabla.encabezado = campos;
        else if (campos.size() != tabla.encabezado.size())
            throw std::runtime_error(ruta + ": una fila no tiene el número de campos del encabezado");
        else
            tabla.filas.push_back(campos);
    }
    if (tabla.encabezado.empty()) throw std::runtime_error(ruta + ": el archivo está vacío");
    return tabla;
}

int indiceEspecie(const Instancia& I, const std::string& simbolo) {
    for (int e = 0; e < I.E; ++e)
        if (I.especies[e].simbolo == simbolo) return e;
    throw std::runtime_error("especie desconocida '" + simbolo + "'");
}

}  // namespace

// Derivados de la instancia (capítulos 4 y 5).
void Instancia::calcularDerivados() {
    Instancia& I = *this;
    const int E = I.E, F = I.F, S = I.S;

    I.aportes.assign(F, {});
    for (int f = 0; f < F; ++f) {
        const Fertilizante& fert = I.fertilizantes[f];
        for (int e = 0; e < E; ++e) {
            const int n = fert.atomos[e];
            if (n == 0) continue;
            I.aportes[f].push_back({e, 1000.0 * I.especies[e].pesoAtomico * n / (fert.mw * I.V)});
        }
    }

    // Cref: el punto medio del intervalo, o la mitad de la cota máxima si no hay requerimiento.
    I.Cref.assign(E * S, 0.0);
    for (int i = 0; i < E * S; ++i)
        I.Cref[i] = I.Cmin[i] > 0.0 ? (I.Cmin[i] + I.Cmax[i]) / 2.0 : I.Cmax[i] / 2.0;

    I.L = 9.819 * I.Emax - 1.462;

    // Peso equivalente w_e = a_e / carga, solo para las especies de E+.
    I.u = 0.0;
    for (int e = 0; e < E; ++e)
        if (I.especies[e].carga > 0) I.u += I.C0[e] / (I.especies[e].pesoAtomico / I.especies[e].carga);

    I.coefCationes.assign(F, 0.0);
    for (int f = 0; f < F; ++f)
        for (const Aporte& a : I.aportes[f]) {
            const Especie& esp = I.especies[a.especie];
            if (esp.carga > 0) I.coefCationes[f] += a.alfa / (esp.pesoAtomico / esp.carga);
        }

    // D(0): con la dosificación nula la concentración es la del agua de partida.
    I.D0 = 0.0;
    for (int s = 0; s < S; ++s)
        for (int e = 0; e < E; ++e) {
            const int i = e * S + s;
            I.D0 += (std::max(0.0, I.Cmin[i] - I.C0[e]) + std::max(0.0, I.C0[e] - I.Cmax[i])) / I.Cref[i];
        }

    // β_(f,s) = mín{(Cmax_(e,s) + D(0)·Cref_(e,s))/α_(e,f) : α_(e,f) > 0}.
    I.beta.assign(F * S, std::numeric_limits<double>::infinity());
    for (int f = 0; f < F; ++f)
        for (int s = 0; s < S; ++s)
            for (const Aporte& a : I.aportes[f]) {
                const int i = a.especie * S + s;
                I.beta[f * S + s] = std::min(I.beta[f * S + s], (I.Cmax[i] + I.D0 * I.Cref[i]) / a.alfa);
            }
}

// Una instancia inconsistente se rechaza al leerla, antes de ejecutar nada.
void Instancia::validar() const {
    for (int i = 0; i < E * S; ++i)
        if (Cmin[i] > Cmax[i])
            throw std::runtime_error(nombre + ": límite inferior mayor que el superior en " + especies[i / S].simbolo);
    for (int f = 0; f < F; ++f)
        if (aportes[f].empty())
            throw std::runtime_error(nombre + ": " + fertilizantes[f].nombre + " no aporta ninguna especie de la instancia");
    if (!(u < L)) throw std::runtime_error(nombre + ": el agua de partida agota por sí sola el límite de conductividad");
}

double aReal(const std::string& texto, const std::string& contexto) {
    double valor = 0.0;
    const char* fin = texto.data() + texto.size();
    const auto [ptr, error] = std::from_chars(texto.data(), fin, valor);
    if (texto.empty() || error != std::errc() || ptr != fin)
        throw std::runtime_error(contexto + ": '" + texto + "' no es un número");
    return valor;
}

int aEntero(const std::string& texto, const std::string& contexto) {
    int valor = 0;
    const char* fin = texto.data() + texto.size();
    const auto [ptr, error] = std::from_chars(texto.data(), fin, valor);
    if (texto.empty() || error != std::errc() || ptr != fin)
        throw std::runtime_error(contexto + ": '" + texto + "' no es un entero");
    return valor;
}

std::map<std::string, std::string> leerClaveValor(const std::string& ruta) {
    const Tabla t = leerCsv(ruta);
    const std::size_t cClave = t.columna("clave"), cValor = t.columna("valor");
    std::map<std::string, std::string> valores;
    for (const auto& fila : t.filas) valores[fila[cClave]] = fila[cValor];
    return valores;
}

Instancia Instancia::leer(const std::string& carpeta) {
    Instancia I;
    std::string ruta = carpeta;
    while (!ruta.empty() && (ruta.back() == '/' || ruta.back() == '\\')) ruta.pop_back();
    I.nombre = ruta.substr(ruta.find_last_of("/\\") + 1);

    const auto p = leerClaveValor(ruta + "/parametros.csv");
    auto valor = [&p](const std::string& clave) {
        const auto it = p.find(clave);
        if (it == p.end()) throw std::runtime_error("parametros.csv: falta la clave '" + clave + "'");
        return it->second;
    };
    I.S = aEntero(valor("etapas"), "etapas");
    I.V = aReal(valor("V"), "V");
    I.Emax = aReal(valor("Emax"), "Emax");
    I.k = aEntero(valor("k"), "k");
    I.gamma = aReal(valor("gamma"), "gamma");
    I.Kref = aReal(valor("Kref"), "Kref");

    const Tabla te = leerCsv(ruta + "/especies.csv");
    for (const auto& fila : te.filas) {
        Especie e;
        e.simbolo = fila[te.columna("especie")];
        e.pesoAtomico = aReal(fila[te.columna("peso_atomico")], "especies.csv");
        e.carga = aEntero(fila[te.columna("carga_E_mas")], "especies.csv");
        I.especies.push_back(e);
    }
    I.E = static_cast<int>(I.especies.size());

    const Tabla tl = leerCsv(ruta + "/limites.csv");
    if (static_cast<int>(tl.filas.size()) != I.E * I.S)
        throw std::runtime_error("limites.csv: se espera una fila por especie y etapa");
    I.Cmin.assign(I.E * I.S, 0.0);
    I.Cmax.assign(I.E * I.S, 0.0);
    for (const auto& fila : tl.filas) {
        const int e = indiceEspecie(I, fila[tl.columna("especie")]);
        const int s = aEntero(fila[tl.columna("etapa")], "limites.csv") - 1;
        if (s < 0 || s >= I.S) throw std::runtime_error("limites.csv: etapa fuera de rango");
        I.Cmin[e * I.S + s] = aReal(fila[tl.columna("Cmin")], "limites.csv");
        I.Cmax[e * I.S + s] = aReal(fila[tl.columna("Cmax")], "limites.csv");
    }

    const Tabla ta = leerCsv(ruta + "/agua.csv");
    I.C0.assign(I.E, 0.0);
    for (const auto& fila : ta.filas)
        I.C0[indiceEspecie(I, fila[ta.columna("especie")])] = aReal(fila[ta.columna("C0")], "agua.csv");

    const Tabla tf = leerCsv(ruta + "/fertilizantes.csv");
    for (const auto& fila : tf.filas) {
        Fertilizante f;
        f.nombre = fila[tf.columna("nombre")];
        f.mw = aReal(fila[tf.columna("mw")], f.nombre);
        f.precio = aReal(fila[tf.columna("precio")], f.nombre);
        f.epsilon = aReal(fila[tf.columna("epsilon")], f.nombre);
        f.delta = aReal(fila[tf.columna("delta")], f.nombre);
        for (const Especie& e : I.especies) f.atomos.push_back(aEntero(fila[tf.columna(e.simbolo)], f.nombre));
        I.fertilizantes.push_back(f);
    }
    I.F = static_cast<int>(I.fertilizantes.size());

    I.calcularDerivados();
    I.validar();
    return I;
}
