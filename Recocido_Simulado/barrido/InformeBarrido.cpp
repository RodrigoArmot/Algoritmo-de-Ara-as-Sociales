#include "recocido/barrido/InformeBarrido.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include "recocido/barrido/ResumenBarrido.h"

namespace barrido {
namespace {

// Números con coma decimal, como el resto de los documentos de la tesis.
std::string coma(double x, int decimales) {
    char texto[48];
    std::snprintf(texto, sizeof texto, "%.*f", decimales, x);
    std::string s(texto);
    std::replace(s.begin(), s.end(), '.', ',');
    return s;
}

std::string significativas(double x, int cifras) {
    char texto[48];
    std::snprintf(texto, sizeof texto, "%.*g", cifras, x);
    std::string s(texto);
    std::replace(s.begin(), s.end(), '.', ',');
    return s;
}

std::string miles(long long x) {
    std::string s = std::to_string(x);
    if (s.size() <= 4) return s;
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<std::size_t>(i), " ");
    return s;
}

std::string punto(double x) {
    char texto[40];
    std::snprintf(texto, sizeof texto, "%.10g", x);
    return texto;
}

struct Fase {
    bool existe = false;
    std::vector<ResumenConfiguracion> resumenes;
    long long filas = 0;
    long long evaluaciones = 0;
};

// Lo que se sabe de una configuración a través de las fases.
struct Combinada {
    const ResumenConfiguracion* fuente = nullptr;  // la fase más avanzada en que corrió
    const ResumenConfiguracion* porFase[3] = {nullptr, nullptr, nullptr};
    int bandera[3][2] = {{-1, -1}, {-1, -1}, {-1, -1}};  // fase × (final, mejor): 1 pasa, 0 no, −1 no corrió
};

int cuenta(const std::vector<ResumenConfiguracion>& r, bool (*pred)(const ResumenConfiguracion&)) {
    return static_cast<int>(std::count_if(r.begin(), r.end(), pred));
}

std::string medianasTexto(const std::vector<EstadisticosGamma>& e, bool costo) {
    std::string s;
    for (std::size_t g = 0; g < e.size(); ++g) {
        if (g) s += " · ";
        s += costo ? coma(e[g].medianaCosto, 2) : significativas(e[g].medianaRE, 3);
    }
    return s;
}

}  // namespace

void escribirResumenEInforme(const std::filesystem::path& carpeta, const std::vector<RangoPublicado>& publicados) {
    Fase fases[3];
    for (int k = 0; k < 3; ++k) {
        const auto ruta = carpeta / ("barrido_fase" + std::to_string(k + 1) + ".csv");
        if (!std::filesystem::exists(ruta)) continue;
        const auto filas = leerFilas(ruta);
        fases[k].existe = true;
        fases[k].filas = static_cast<long long>(filas.size());
        for (const auto& f : filas) fases[k].evaluaciones += f.evaluaciones;
        fases[k].resumenes = resumir(filas, publicados);
    }
    if (!fases[0].existe) return;

    // ------------------------------------------------------------ una fila por configuración
    std::vector<std::string> orden;
    std::map<std::string, Combinada> porId;
    for (int k = 0; k < 3; ++k)
        for (const auto& r : fases[k].resumenes) {
            const std::string id = r.configuracion.identificador();
            if (!porId.count(id)) orden.push_back(id);
            Combinada& c = porId[id];
            c.fuente = &r;
            c.porFase[k] = &r;
            c.bandera[k][0] = k == 0 ? r.cribadoFinal : r.cumpleFinal;
            c.bandera[k][1] = k == 0 ? r.cribadoMejor : r.cumpleMejor;
        }
    std::stable_sort(orden.begin(), orden.end(), [&](const std::string& a, const std::string& b) {
        return porId[a].fuente->distanciaMinima() < porId[b].fuente->distanciaMinima();
    });

    {
        std::ofstream csv(carpeta / "barrido_resumen.csv");
        csv << "id";
        for (const auto& f : nombresFactores()) csv << ',' << f;
        csv << ",fase,repeticiones";
        for (const auto& p : publicados) {
            const std::string g = "g" + textoGamma(p.gamma).substr(2);
            for (const char* sol : {"final", "mejor"})
                for (const char* stat : {"mediana_RE", "min_RE", "max_RE", "mediana_costo", "min_costo", "max_costo"})
                    csv << ',' << g << '_' << sol << '_' << stat;
        }
        csv << ",cumple_final,cumple_mejor,distancia_final,distancia_mejor,f1_cribado_final,f1_cribado_mejor,"
               "f2_cumple_final,f2_cumple_mejor,f3_cumple_final,f3_cumple_mejor,evaluaciones,d_fase1_final,"
               "d_fase1_mejor,d_fase2_final,d_fase2_mejor,d_fase3_final,d_fase3_mejor\n";
        for (const auto& id : orden) {
            const Combinada& c = porId[id];
            const ResumenConfiguracion& r = *c.fuente;
            csv << id;
            for (const auto& nivel : niveles(r.configuracion)) csv << ',' << nivel;
            csv << ',' << r.fase << ',' << r.repeticiones;
            for (std::size_t g = 0; g < publicados.size(); ++g)
                for (const auto* e : {&r.final[g], &r.mejor[g]})
                    csv << ',' << punto(e->medianaRE) << ',' << punto(e->minRE) << ',' << punto(e->maxRE) << ','
                        << punto(e->medianaCosto) << ',' << punto(e->minCosto) << ',' << punto(e->maxCosto);
            csv << ',' << r.cumpleFinal << ',' << r.cumpleMejor << ',' << punto(r.distanciaFinal) << ','
                << punto(r.distanciaMejor);
            for (const auto& fase : c.bandera)
                for (int b : fase) csv << ',' << (b < 0 ? std::string() : std::to_string(b));
            csv << ',' << r.evaluaciones;
            for (const auto* p : c.porFase)
                csv << ',' << (p ? punto(p->distanciaFinal) : std::string()) << ','
                    << (p ? punto(p->distanciaMejor) : std::string());
            csv << '\n';
        }
    }

    // ------------------------------------------------------------ informe
    std::ofstream md(carpeta / "barrido_informe.md");
    md << "# Barrido de reproducción del recocido de Guerra Ibarra et al. (2026)\n\n"
       << "> Generado por `formulacion barrido` a partir de `barrido_fase*.csv`. **Criterio de éxito:** en los tres γ a la "
          "vez, la mediana de nuestras RE dentro del rango publicado de RE y la de nuestros costos dentro del de "
          "costos (Tabla 5, cifras exactas). Se evalúa por separado con la solución final y con la mejor encontrada. "
          "**Distancia:** suma, sobre las seis condiciones, de las décadas que separan la mediana de su rango (0 si "
          "cumple), con un tope de tres décadas por condición.\n\n";

    const std::set<std::string> basesCribadas = [&] {
        std::set<std::string> b;
        for (const auto& r : fases[0].resumenes)
            if (r.cribadoAlguna()) b.insert(r.configuracion.base());
        return b;
    }();
    std::set<std::string> basesFase1;
    for (const auto& r : fases[0].resumenes) basesFase1.insert(r.configuracion.base());

    md << "## Cuántas pasaron\n\n"
       << "| Fase | Configuraciones | Ejecuciones | Evaluaciones | Pasan, solución final | Pasan, mejor encontrada |\n"
       << "|---|---|---|---|---|---|\n";
    const char* nombres[3] = {"1 · cribado: RE a menos de ×10 del rango", "2 · expansión: criterio de éxito",
                              "3 · confirmación: criterio, semillas nuevas"};
    for (int k = 0; k < 3; ++k) {
        if (!fases[k].existe) {
            md << "| " << nombres[k] << " | — | — | — | — | — |\n";
            continue;
        }
        const auto& r = fases[k].resumenes;
        const int pf = k == 0 ? cuenta(r, [](const ResumenConfiguracion& x) { return x.cribadoFinal; })
                              : cuenta(r, [](const ResumenConfiguracion& x) { return x.cumpleFinal; });
        const int pm = k == 0 ? cuenta(r, [](const ResumenConfiguracion& x) { return x.cribadoMejor; })
                              : cuenta(r, [](const ResumenConfiguracion& x) { return x.cumpleMejor; });
        md << "| " << nombres[k] << " | " << miles(static_cast<long long>(r.size())) << " | " << miles(fases[k].filas)
           << " | " << miles(fases[k].evaluaciones) << " | " << pf << " | " << pm << " |\n";
    }
    md << "\nTras el cribado pasan a la fase 2 " << basesCribadas.size() << " de las " << basesFase1.size()
       << " combinaciones de los seis factores distintos de I y n, cada una con los nueve pares (I, n).";
    const bool ningunaEnFase2 = fases[1].existe && std::none_of(fases[1].resumenes.begin(), fases[1].resumenes.end(),
                                                                [](const ResumenConfiguracion& r) { return r.cumpleAlguna(); });
    if (fases[2].existe && ningunaEnFase2)
        md << " Como ninguna cumplió el criterio en la fase 2, la fase 3 confirma con semillas nuevas las "
           << fases[2].resumenes.size() << " más cercanas: así la distancia de la más cercana no depende de la suerte "
              "de la selección. Reproducir exige cumplir en la fase 2 y en la 3.";
    md << "\n\n";

    // Por qué no cumplen: con qué condiciones fallan las que aciertan la RE.
    if (fases[1].existe) {
        md << "## Qué condición falla en la fase 2\n\n"
           << "Entre las configuraciones con la mediana de RE dentro del rango publicado en los tres γ, dónde cae la "
              "mediana del costo:\n\n"
           << "| Solución | RE dentro en los tres γ | γ | Costo por debajo | Dentro | Por encima | Costo mediano (mín.–máx.) | "
              "Publicado |\n|---|---|---|---|---|---|---|---|\n";
        for (int sol = 0; sol < 2; ++sol) {
            std::vector<const ResumenConfiguracion*> aciertan;
            for (const auto& r : fases[1].resumenes) {
                const auto& e = sol == 0 ? r.final : r.mejor;
                bool todas = true;
                for (std::size_t g = 0; g < publicados.size(); ++g)
                    todas = todas && e[g].medianaRE >= publicados[g].RE.min && e[g].medianaRE <= publicados[g].RE.max;
                if (todas) aciertan.push_back(&r);
            }
            for (std::size_t g = 0; g < publicados.size(); ++g) {
                int bajo = 0, dentro = 0, sobre = 0;
                double menor = 0.0, mayor = 0.0;
                for (std::size_t i = 0; i < aciertan.size(); ++i) {
                    const double c = (sol == 0 ? aciertan[i]->final : aciertan[i]->mejor)[g].medianaCosto;
                    if (c < publicados[g].costo.min) ++bajo;
                    else if (c > publicados[g].costo.max) ++sobre;
                    else ++dentro;
                    menor = i == 0 ? c : std::min(menor, c);
                    mayor = i == 0 ? c : std::max(mayor, c);
                }
                md << "| " << (g == 0 ? (sol == 0 ? "final" : "mejor") : "") << " | "
                   << (g == 0 ? std::to_string(aciertan.size()) + " de " + std::to_string(fases[1].resumenes.size()) : "")
                   << " | " << coma(publicados[g].gamma, 2) << " | " << bajo << " | " << dentro << " | " << sobre << " | "
                   << (aciertan.empty() ? "—" : coma(menor, 2) + "–" + coma(mayor, 2)) << " | "
                   << coma(publicados[g].costo.min, 2) << "–" << coma(publicados[g].costo.max, 2) << " |\n";
            }
        }
        md << "\n";
    }

    md << "## Las más cercanas a los rangos publicados\n\n"
       << "Medianas de la solución con menor distancia, en el orden γ = 0,65 · 0,75 · 0,85. Publicado: RE "
       << significativas(publicados[0].RE.min, 3) << "–" << significativas(publicados[0].RE.max, 3) << " · "
       << significativas(publicados[1].RE.min, 3) << "–" << significativas(publicados[1].RE.max, 3) << " · "
       << significativas(publicados[2].RE.min, 3) << "–" << significativas(publicados[2].RE.max, 3) << "; costo "
       << coma(publicados[0].costo.min, 2) << "–" << coma(publicados[0].costo.max, 2) << " · "
       << coma(publicados[1].costo.min, 2) << "–" << coma(publicados[1].costo.max, 2) << " · "
       << coma(publicados[2].costo.min, 2) << "–" << coma(publicados[2].costo.max, 2) << ".\n\n";
    auto fila = [&](std::size_t i, const std::string& id, const ResumenConfiguracion& r, const std::string& previa) {
        const bool conMejor = r.distanciaMejor < r.distanciaFinal;
        const auto& e = conMejor ? r.mejor : r.final;
        md << "| " << i << " | `" << id << "` | " << previa << r.repeticiones << " | " << coma(r.distanciaFinal, 2)
           << " | " << coma(r.distanciaMejor, 2) << " | " << (conMejor ? "mejor" : "final") << " | "
           << medianasTexto(e, false) << " | " << medianasTexto(e, true) << " |\n";
    };
    // Las confirmadas con semillas nuevas van aparte: la distancia de las demás está sesgada a la baja por la
    // propia selección, y al confirmarlas suele crecer.
    std::vector<std::string> confirmadas, sinConfirmar;
    for (const auto& id : orden) (porId[id].porFase[2] ? confirmadas : sinConfirmar).push_back(id);
    if (!confirmadas.empty()) {
        md << "### Confirmadas con semillas nuevas (fase 3, " << porId[confirmadas.front()].porFase[2]->repeticiones
           << " ejecuciones por γ)\n\n"
           << "| # | Configuración | Distancia en la fase 2 | Rep. por γ | Distancia final | Distancia mejor | Solución "
              "| RE mediana | Costo mediano |\n|---|---|---|---|---|---|---|---|---|\n";
        for (std::size_t i = 0; i < confirmadas.size(); ++i) {
            const Combinada& c = porId[confirmadas[i]];
            const std::string previa = c.porFase[1] ? coma(c.porFase[1]->distanciaMinima(), 2) + " | " : "— | ";
            fila(i + 1, confirmadas[i], *c.porFase[2], previa);
        }
        md << "\n### Las diez más cercanas sin confirmar\n\n"
           << "Su distancia no se ha medido con semillas nuevas y es, por la selección, optimista.\n\n";
    }
    md << "| # | Configuración | Fase | Rep. por γ | Distancia final | Distancia mejor | Solución | RE mediana | "
          "Costo mediano |\n|---|---|---|---|---|---|---|---|---|\n";
    for (std::size_t i = 0; i < std::min<std::size_t>(10, sinConfirmar.size()); ++i) {
        const ResumenConfiguracion& r = *porId[sinConfirmar[i]].fuente;
        fila(i + 1, sinConfirmar[i], r, r.fase + " | ");
    }

    const auto efectosF = efectosPrincipales(fases[0].resumenes, false);
    const auto efectosM = efectosPrincipales(fases[0].resumenes, true);
    md << "\n## Qué factores mueven la distancia\n\n"
       << "Efectos principales sobre la distancia en la fase 1, cuyo diseño es equilibrado: la media de la "
          "distancia en cada nivel. La amplitud es la diferencia entre el mejor y el peor nivel; I y n solo "
          "tienen ahí sus dos niveles baratos.\n\n"
       << "| Factor | Amplitud, final | Media por nivel, final | Amplitud, mejor | Media por nivel, mejor |\n"
       << "|---|---|---|---|---|\n";
    for (const auto& ef : efectosF) {
        const auto itM = std::find_if(efectosM.begin(), efectosM.end(),
                                      [&](const EfectoPrincipal& x) { return x.factor == ef.factor; });
        auto mediasTexto = [](const EfectoPrincipal& x) {
            std::string s;
            for (const auto& [nivel, media] : x.mediaPorNivel) s += (s.empty() ? "" : " · ") + nivel + " " + coma(media, 2);
            return s;
        };
        md << "| " << ef.factor << " | " << coma(ef.amplitud, 2) << " | " << mediasTexto(ef) << " | "
           << coma(itM->amplitud, 2) << " | " << mediasTexto(*itM) << " |\n";
    }
    md << "\n**Nota de interpretación.** Con `solo_mejoras` la temperatura no decide ninguna aceptación: `t_final`, "
          "`p` y `delta_t0` solo actúan a través de E_v `decreciente` (que baja con t/t0) y de la regla que corta "
          "tras el primer nivel si t0 ≤ t_f. Con `solo_mejoras` y E_v `fija`, el recocido es una búsqueda local "
          "voraz, y esos tres factores apenas importan.\n";
    if (fases[1].existe) {
        const auto efectos2 = efectosPrincipales(fases[1].resumenes, false);
        const auto efectos2M = efectosPrincipales(fases[1].resumenes, true);
        md << "\nEn la fase 2, solo sobre las supervivientes (diseño no equilibrado), los pares (I, n) completos:\n\n"
           << "| Factor | Media por nivel, final | Media por nivel, mejor |\n|---|---|---|\n";
        for (const char* factor : {"I", "n"}) {
            auto buscar = [&](const std::vector<EfectoPrincipal>& v) {
                return *std::find_if(v.begin(), v.end(), [&](const EfectoPrincipal& x) { return x.factor == factor; });
            };
            auto texto = [](const EfectoPrincipal& x) {
                std::string s;
                for (const auto& [nivel, media] : x.mediaPorNivel) s += (s.empty() ? "" : " · ") + nivel + " " + coma(media, 2);
                return s;
            };
            md << "| " << factor << " | " << texto(buscar(efectos2)) << " | " << texto(buscar(efectos2M)) << " |\n";
        }
    }

    // ------------------------------------------------------------ conclusión
    const ResumenConfiguracion& mejor = *porId[orden.front()].fuente;
    const EfectoPrincipal& pesa = efectosF.front().amplitud >= efectosM.front().amplitud ? efectosF.front() : efectosM.front();
    auto alguna = [](const Fase& f) {
        return std::any_of(f.resumenes.begin(), f.resumenes.end(), [](const ResumenConfiguracion& r) { return r.cumpleAlguna(); });
    };
    // Reproduce una configuración que cumple en la fase 2 y en la confirmación, con la misma solución.
    int reproducenFinal = 0, reproducenMejor = 0;
    if (fases[2].existe)
        for (const auto& id : orden) {
            const Combinada& c = porId[id];
            reproducenFinal += c.bandera[1][0] == 1 && c.bandera[2][0] == 1;
            reproducenMejor += c.bandera[1][1] == 1 && c.bandera[2][1] == 1;
        }
    md << "\n## Conclusión\n\n";
    if (reproducenFinal + reproducenMejor > 0) {
        md << "**Sí se reproduce:** cumplen el criterio en los tres γ y lo confirman con semillas nuevas "
           << reproducenFinal << " configuraciones con la solución final y " << reproducenMejor
           << " con la mejor encontrada; la más cercana es `" << orden.front() << "`.\n";
    } else if (fases[2].existe && !confirmadas.empty()) {
        const Combinada& c = porId[confirmadas.front()];
        const ResumenConfiguracion& r3 = *c.porFase[2];
        const bool conMejor = r3.distanciaMejor < r3.distanciaFinal;
        const auto& e = conMejor ? r3.mejor : r3.final;
        bool reDentro = true;
        for (std::size_t g = 0; g < publicados.size(); ++g)
            reDentro = reDentro && e[g].medianaRE >= publicados[g].RE.min && e[g].medianaRE <= publicados[g].RE.max;
        double min2 = 1e300, max2 = -1e300, min3 = 1e300, max3 = -1e300;
        for (const auto& id : confirmadas) {
            const Combinada& ci = porId[id];
            if (ci.porFase[1]) {
                min2 = std::min(min2, ci.porFase[1]->distanciaMinima());
                max2 = std::max(max2, ci.porFase[1]->distanciaMinima());
            }
            min3 = std::min(min3, ci.porFase[2]->distanciaMinima());
            max3 = std::max(max3, ci.porFase[2]->distanciaMinima());
        }
        std::string rangosCosto;
        for (std::size_t g = 0; g < publicados.size(); ++g)
            rangosCosto += (g ? " · " : "") + coma(publicados[g].costo.min, 2) + "–" + coma(publicados[g].costo.max, 2);
        md << "**No se reproduce:** "
           << (ningunaEnFase2 ? "ninguna configuración cumple el criterio en la fase 2"
                              : "las que cumplieron el criterio en la fase 2 no lo sostienen con semillas nuevas")
           << ". La más cercana confirmada con semillas nuevas es `" << confirmadas.front() << "`, a "
           << coma(r3.distanciaMinima(), 2) << " décadas con " << r3.repeticiones << " ejecuciones por γ";
        if (c.porFase[1]) md << " (" << coma(c.porFase[1]->distanciaMinima(), 2) << " en la fase 2)";
        if (reDentro)
            md << ": acierta la RE en los tres γ, pero no el costo, con medianas de " << medianasTexto(e, true)
               << " USD frente a " << rangosCosto << " publicados";
        md << ". Al confirmarlas, las " << confirmadas.size() << " más cercanas de la fase 2 pasan de " << coma(min2, 2)
           << "–" << coma(max2, 2) << " a " << coma(min3, 2) << "–" << coma(max3, 2)
           << " décadas. El factor que más pesa es `" << pesa.factor << "`.\n";
    } else if (fases[2].existe) {
        md << "**No se reproduce:** las que cumplieron el criterio en la fase 2 no lo sostienen con semillas nuevas; "
              "la más cercana es `" << orden.front() << "`, a " << coma(mejor.distanciaMinima(), 2)
           << " décadas, y el factor que más pesa es `" << pesa.factor << "`.\n";
    } else if (fases[1].existe) {
        md << (alguna(fases[1]) ? "**Pendiente de confirmación:** hay configuraciones que cumplen el criterio en la "
                                  "fase 2; falta la fase 3. "
                                : "**No se reproduce:** ninguna configuración de la fase 2 cumple el criterio. ")
           << "La más cercana es `" << orden.front() << "`, a " << coma(mejor.distanciaMinima(), 2)
           << " décadas de los rangos publicados, y el factor que más pesa es `" << pesa.factor << "`.\n";
    } else {
        md << "**Tras el cribado:** pasan " << basesCribadas.size() << " combinaciones de seis factores; la más cercana "
              "hasta ahora es `" << orden.front() << "`, a " << coma(mejor.distanciaMinima(), 2)
           << " décadas, y el factor que más pesa es `" << pesa.factor << "`.\n";
    }
}

}  // namespace barrido
