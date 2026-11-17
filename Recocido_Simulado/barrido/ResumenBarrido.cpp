#include "recocido/barrido/ResumenBarrido.h"

#include <cstdio>
#include <map>
#include <stdexcept>

#include "nucleo/Csv.h"

namespace barrido {
namespace {

std::string real(double x) {
    char texto[40];
    std::snprintf(texto, sizeof texto, "%.10g", x);
    return texto;
}

EstadisticosGamma estadisticos(const std::vector<double>& re, const std::vector<double>& costo) {
    EstadisticosGamma e;
    e.medianaRE = mediana(re);
    e.minRE = *std::min_element(re.begin(), re.end());
    e.maxRE = *std::max_element(re.begin(), re.end());
    e.medianaCosto = mediana(costo);
    e.minCosto = *std::min_element(costo.begin(), costo.end());
    e.maxCosto = *std::max_element(costo.begin(), costo.end());
    return e;
}

std::vector<Medianas> medianas(const std::vector<EstadisticosGamma>& porGamma) {
    std::vector<Medianas> m;
    for (const auto& e : porGamma) m.push_back({e.medianaRE, e.medianaCosto});
    return m;
}

}  // namespace

std::string encabezadoFilas() {
    std::string h = "fase,id";
    for (const auto& f : nombresFactores()) h += "," + f;
    return h + ",gamma,repeticion,semilla,RE_final,costo_final,RE_mejor,costo_mejor,evaluaciones,t0,temperaturas,"
               "aceptadas";
}

std::string lineaFila(const Fila& f) {
    std::string l = f.fase + "," + f.configuracion.identificador();
    for (const auto& nivel : niveles(f.configuracion)) l += "," + nivel;
    return l + "," + textoGamma(f.gamma) + "," + std::to_string(f.repeticion) + "," + std::to_string(f.semilla) +
           "," + real(f.reFinal) + "," + real(f.costoFinal) + "," + real(f.reMejor) + "," + real(f.costoMejor) + "," +
           std::to_string(f.evaluaciones) + "," + real(f.t0) + "," + std::to_string(f.temperaturas) + "," +
           std::to_string(f.aceptadas);
}

std::vector<Fila> leerFilas(const std::filesystem::path& ruta) {
    std::map<std::string, ConfiguracionBarrido> porId;
    for (const auto& c : todas()) porId[c.identificador()] = c;

    const auto tabla = nucleo::csv::leer(ruta);
    auto col = [&](const char* nombre) { return static_cast<std::size_t>(tabla.columna(nombre)); };
    const auto cFase = col("fase"), cId = col("id"), cG = col("gamma"), cR = col("repeticion"), cS = col("semilla"),
               cRF = col("RE_final"), cCF = col("costo_final"), cRM = col("RE_mejor"), cCM = col("costo_mejor"),
               cE = col("evaluaciones"), cT = col("t0"), cTe = col("temperaturas"), cA = col("aceptadas");
    std::vector<Fila> filas;
    for (const auto& c : tabla.filas) {
        const auto it = porId.find(c[cId]);
        if (it == porId.end()) throw std::runtime_error(tabla.origen + ": configuración desconocida " + c[cId]);
        Fila f;
        f.fase = c[cFase];
        f.configuracion = it->second;
        f.gamma = nucleo::csv::aReal(c[cG], "gamma");
        f.repeticion = nucleo::csv::aEntero(c[cR], "repeticion");
        f.semilla = std::stoull(c[cS]);
        f.reFinal = nucleo::csv::aReal(c[cRF], "RE_final");
        f.costoFinal = nucleo::csv::aReal(c[cCF], "costo_final");
        f.reMejor = nucleo::csv::aReal(c[cRM], "RE_mejor");
        f.costoMejor = nucleo::csv::aReal(c[cCM], "costo_mejor");
        f.evaluaciones = std::stol(c[cE]);
        f.t0 = nucleo::csv::aReal(c[cT], "t0");
        f.temperaturas = nucleo::csv::aEntero(c[cTe], "temperaturas");
        f.aceptadas = std::stol(c[cA]);
        filas.push_back(f);
    }
    return filas;
}

std::vector<ResumenConfiguracion> resumir(const std::vector<Fila>& filas,
                                          const std::vector<RangoPublicado>& publicados) {
    // Agrupa por (fase, configuración) conservando el orden de aparición.
    std::map<std::string, std::size_t> indice;
    std::vector<std::vector<const Fila*>> grupos;
    for (const auto& f : filas) {
        const std::string clave = f.fase + "|" + f.configuracion.identificador();
        auto it = indice.find(clave);
        if (it == indice.end()) {
            it = indice.emplace(clave, grupos.size()).first;
            grupos.emplace_back();
        }
        grupos[it->second].push_back(&f);
    }

    std::vector<ResumenConfiguracion> resumenes;
    for (const auto& grupo : grupos) {
        ResumenConfiguracion r;
        r.fase = grupo.front()->fase;
        r.configuracion = grupo.front()->configuracion;
        for (const auto& publicado : publicados) {
            std::vector<double> reF, costoF, reM, costoM;
            for (const Fila* f : grupo) {
                if (f->gamma != publicado.gamma) continue;
                reF.push_back(f->reFinal);
                costoF.push_back(f->costoFinal);
                reM.push_back(f->reMejor);
                costoM.push_back(f->costoMejor);
            }
            if (reF.empty()) throw std::runtime_error("configuración sin ejecuciones en algún γ: " +
                                                      r.configuracion.identificador());
            r.repeticiones = static_cast<int>(reF.size());
            r.final.push_back(estadisticos(reF, costoF));
            r.mejor.push_back(estadisticos(reM, costoM));
        }
        for (const Fila* f : grupo) r.evaluaciones += f->evaluaciones;
        const auto mF = medianas(r.final), mM = medianas(r.mejor);
        r.cumpleFinal = cumple(mF, publicados);
        r.cumpleMejor = cumple(mM, publicados);
        r.cribadoFinal = pasaCribado(mF, publicados);
        r.cribadoMejor = pasaCribado(mM, publicados);
        r.distanciaFinal = distancia(mF, publicados);
        r.distanciaMejor = distancia(mM, publicados);
        resumenes.push_back(r);
    }
    return resumenes;
}

std::vector<EfectoPrincipal> efectosPrincipales(const std::vector<ResumenConfiguracion>& resumenes, bool mejor) {
    std::vector<EfectoPrincipal> efectos;
    const auto& factores = nombresFactores();
    for (std::size_t k = 0; k < factores.size(); ++k) {
        std::vector<std::string> orden;  // niveles en orden de aparición
        std::map<std::string, std::pair<double, int>> acumulado;
        for (const auto& r : resumenes) {
            const std::string nivel = niveles(r.configuracion)[k];
            if (!acumulado.count(nivel)) orden.push_back(nivel);
            auto& a = acumulado[nivel];
            a.first += mejor ? r.distanciaMejor : r.distanciaFinal;
            a.second += 1;
        }
        EfectoPrincipal e;
        e.factor = factores[k];
        double menor = 0.0, mayor = 0.0;
        for (const auto& nivel : orden) {
            const double media = acumulado[nivel].first / acumulado[nivel].second;
            e.mediaPorNivel.emplace_back(nivel, media);
            if (e.mediaPorNivel.size() == 1 || media < menor) menor = media;
            if (e.mediaPorNivel.size() == 1 || media > mayor) mayor = media;
        }
        e.amplitud = mayor - menor;
        efectos.push_back(e);
    }
    std::sort(efectos.begin(), efectos.end(),
              [](const EfectoPrincipal& a, const EfectoPrincipal& b) { return a.amplitud > b.amplitud; });
    return efectos;
}

}  // namespace barrido
