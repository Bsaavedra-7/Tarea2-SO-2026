#include <algorithm>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>/**
 * @file rw.cpp
 * @brief Orquestador de linea de comandos, ejecucion concurrente y reporte de metricas
 *        para el problema de Lectores y Escritores (modos "cond" y "atomic").
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "control_atomico_rw.hpp"
#include "control_rw.hpp"
#include "monitor_rw.hpp"
#include "verificador.hpp"

namespace {

/**
 * @brief Parametros validados recibidos por linea de comandos.
 */
struct ConfigCLI {
    std::string modo_str;
    std::string politica_str;
    Politica politica = Politica::Lectores;
    bool es_atomico = false;
    long L = 0;
    long E = 0;
    long K = 0;
    long dur_lec_us = 0;
    long dur_esc_us = 0;
    bool con_traza = true;
};

/**
 * @brief Parsea una cadena como entero long validando rango y formato completo.
 */
bool parse_long_estricto(const char* texto, long& salida) {
    if (texto == nullptr || *texto == '\0') {
        return false;
    }
    errno = 0;
    char* fin = nullptr;
    long long val = std::strtoll(texto, &fin, 10);
    if (errno == ERANGE || fin == texto || *fin != '\0') {
        return false;
    }
    if (val < static_cast<long long>(std::numeric_limits<long>::min()) ||
        val > static_cast<long long>(std::numeric_limits<long>::max())) {
        return false;
    }
    salida = static_cast<long>(val);
    return true;
}

/**
 * @brief Comprueba que los productos con K no desborden el tipo long usando __int128.
 */
bool verificar_productos_sin_overflow(long L, long E, long K) {
    // Usamos __int128 en GCC/Clang en Linux de 64 bits para evitar overflow intermedio
    __int128 l128 = static_cast<__int128>(L);
    __int128 e128 = static_cast<__int128>(E);
    __int128 k128 = static_cast<__int128>(K);
    __int128 long_max = static_cast<__int128>(std::numeric_limits<long>::max());

    __int128 dos_ek = 2 * e128 * k128;
    __int128 total_op = (l128 + e128) * k128;

    return dos_ek <= long_max && total_op <= long_max;
}

/**
 * @brief Valida y carga los argumentos del programa en ConfigCLI.
 */
bool parsear_argumentos(int argc, char* argv[], ConfigCLI& cfg) {
    if (argc < 8 || argc > 9) {
        std::cerr << "Uso: ./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
        return false;
    }

    cfg.modo_str = argv[1];
    cfg.politica_str = argv[2];

    if (cfg.modo_str == "cond") {
        cfg.es_atomico = false;
    } else if (cfg.modo_str == "atomic") {
        cfg.es_atomico = true;
    } else {
        std::cerr << "Error: modo invalido '" << cfg.modo_str << "' (use cond o atomic)\n";
        return false;
    }

    if (cfg.politica_str == "lectores") {
        cfg.politica = Politica::Lectores;
    } else if (cfg.politica_str == "escritores") {
        cfg.politica = Politica::Escritores;
    } else if (cfg.politica_str == "fifo") {
        cfg.politica = Politica::Fifo;
    } else {
        std::cerr << "Error: politica invalida '" << cfg.politica_str
                  << "' (use lectores, escritores o fifo)\n";
        return false;
    }

    if (cfg.es_atomico && cfg.politica != Politica::Escritores) {
        std::cerr << "Error: el modo atomic solo admite la politica escritores\n";
        return false;
    }

    if (!parse_long_estricto(argv[3], cfg.L) ||
        !parse_long_estricto(argv[4], cfg.E) ||
        !parse_long_estricto(argv[5], cfg.K) ||
        !parse_long_estricto(argv[6], cfg.dur_lec_us) ||
        !parse_long_estricto(argv[7], cfg.dur_esc_us)) {
        std::cerr << "Error: argumentos numericos invalidos o fuera de rango long\n";
        return false;
    }

    // Evaluamos L + E en long long para evitar desbordamiento antes de comprobar el limite
    const long long suma_le = static_cast<long long>(cfg.L) + static_cast<long long>(cfg.E);
    if (cfg.L < 1 || cfg.E < 1 || cfg.L > 1000 || cfg.E > 1000 ||
        suma_le > 1000LL || cfg.K < 1 ||
        cfg.dur_lec_us < 0 || cfg.dur_esc_us < 0) {
        std::cerr << "Error: valores fuera del rango permitido (L>=1, E>=1, L+E<=1000, K>=1, dur>=0)\n";
        return false;
    }

    if (!verificar_productos_sin_overflow(cfg.L, cfg.E, cfg.K)) {
        std::cerr << "Error: 2*E*K o (L+E)*K desborda el tipo long\n";
        return false;
    }

    if (argc == 9) {
        if (std::string(argv[8]) == "notraza") {
            cfg.con_traza = false;
        } else {
            std::cerr << "Error: parametro opcional desconocido '" << argv[8]
                      << "' (solo se admite 'notraza')\n";
            return false;
        }
    } else {
        cfg.con_traza = true;
    }

    return true;
}

/**
 * @brief Mide el tiempo de CPU acumulado del proceso en segundos.
 */
double tiempo_cpu_proceso_seg() {
    struct timespec ts {};
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) {
        return 0.0;
    }
    return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) * 1e-9;
}

/**
 * @brief Contadores atomicos para detectar violaciones de exclusion mutua e inconsistencias.
 */
struct DiagnosticoRW {
    std::atomic<long> lectoras_activas{0};
    std::atomic<long> escritoras_activas{0};
    std::atomic<long> max_lectoras_simultaneas{0};
    std::atomic<long> inconsistencias{0};
    std::atomic<long> violaciones_exclusion{0};
};

/**
 * @brief Metricas locales acumuladas por cada hebra (esperas y fallos atomicos).
 */
struct MetricasHebra {
    double suma_espera_us = 0.0;
    double max_espera_us = 0.0;
    long conteo = 0;
    FallosAdmision fallos_atomicos{};
};

/**
 * @brief Actualiza de forma atomica el maximo de lectoras concurrentes.
 */
void actualizar_max_lectoras(std::atomic<long>& max_lec, long actual) {
    long prev = max_lec.load(std::memory_order_relaxed);
    while (actual > prev &&
           !max_lec.compare_exchange_weak(prev, actual,
                                          std::memory_order_relaxed,
                                          std::memory_order_relaxed)) {
    }
}

} // namespace

int main(int argc, char* argv[]) {
    ConfigCLI cfg;
    if (!parsear_argumentos(argc, argv, cfg)) {
        return 1;
    }

    // Instancia el control de sincronizacion y propaga notraza (cfg.con_traza) al monitor
    std::unique_ptr<ControlRW> control;
    MonitorRW* monitor_ptr = nullptr;

    if (cfg.es_atomico) {
        control = std::make_unique<ControlAtomicoRW>();
    } else {
        auto mon = std::make_unique<MonitorRW>(cfg.politica, cfg.con_traza);
        monitor_ptr = mon.get();
        control = std::move(mon);
    }

    Registro registro;
    DiagnosticoRW diag;

    const long long suma_le = static_cast<long long>(cfg.L) + static_cast<long long>(cfg.E);
    const std::size_t total_hebras = static_cast<std::size_t>(suma_le);
    std::vector<std::thread> hebras;
    hebras.reserve(total_hebras);
    std::vector<MetricasHebra> metricas(total_hebras);

    const double cpu_inicio = tiempo_cpu_proceso_seg();
    const auto pared_inicio = std::chrono::steady_clock::now();

    bool error_creacion_hebras = false;

    // Rutina de hebra lectora: mide espera, comprueba exclusion/consistencia y lee Registro
    auto rutina_lectora = [&](int idx, int id_logico) {
        id_hebra = id_logico;
        auto& m_hebra = metricas[static_cast<std::size_t>(idx)];

        for (long op = 0; op < cfg.K; ++op) {
            const auto t0 = std::chrono::steady_clock::now();
            control->entrar_lectura();
            const auto t1 = std::chrono::steady_clock::now();

            double espera_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
            m_hebra.suma_espera_us += espera_us;
            if (espera_us > m_hebra.max_espera_us) {
                m_hebra.max_espera_us = espera_us;
            }
            ++m_hebra.conteo;

            long lec_act = diag.lectoras_activas.fetch_add(1, std::memory_order_acq_rel) + 1;
            long esc_act = diag.escritoras_activas.load(std::memory_order_acquire);
            if (esc_act != 0) {
                diag.violaciones_exclusion.fetch_add(1, std::memory_order_relaxed);
            }
            actualizar_max_lectoras(diag.max_lectoras_simultaneas, lec_act);

            long v = registro.version;
            long dv = registro.doble_version;
            if (dv != 2L * v) {
                diag.inconsistencias.fetch_add(1, std::memory_order_relaxed);
            }

            if (cfg.dur_lec_us > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(cfg.dur_lec_us));
            }

            diag.lectoras_activas.fetch_sub(1, std::memory_order_acq_rel);
            control->salir_lectura();
        }

        if (cfg.es_atomico) {
            m_hebra.fallos_atomicos = ControlAtomicoRW::retirar_fallos_hebra();
        }
    };

    // Rutina de hebra escritora: mide espera, verifica exclusion mutua y actualiza Registro
    auto rutina_escritora = [&](int idx, int id_logico) {
        id_hebra = id_logico;
        auto& m_hebra = metricas[static_cast<std::size_t>(idx)];

        for (long op = 0; op < cfg.K; ++op) {
            const auto t0 = std::chrono::steady_clock::now();
            control->entrar_escritura();
            const auto t1 = std::chrono::steady_clock::now();

            double espera_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
            m_hebra.suma_espera_us += espera_us;
            if (espera_us > m_hebra.max_espera_us) {
                m_hebra.max_espera_us = espera_us;
            }
            ++m_hebra.conteo;

            long esc_act = diag.escritoras_activas.fetch_add(1, std::memory_order_acq_rel) + 1;
            long lec_act = diag.lectoras_activas.load(std::memory_order_acquire);
            if (esc_act != 1 || lec_act != 0) {
                diag.violaciones_exclusion.fetch_add(1, std::memory_order_relaxed);
            }

            registro.version += 1;
            registro.doble_version = 2L * registro.version;

            if (cfg.dur_esc_us > 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(cfg.dur_esc_us));
            }

            diag.escritoras_activas.fetch_sub(1, std::memory_order_acq_rel);
            control->salir_escritura();
        }

        if (cfg.es_atomico) {
            m_hebra.fallos_atomicos = ControlAtomicoRW::retirar_fallos_hebra();
        }
    };

    // Crea las hebras lectoras y luego las escritoras
    for (long i = 0; i < cfg.L; ++i) {
        int idx = static_cast<int>(hebras.size());
        int id_logico = idx;
        try {
            hebras.emplace_back(rutina_lectora, idx, id_logico);
        } catch (const std::system_error& e) {
            std::cerr << "Error al crear hebra lectora " << i << ": " << e.what() << "\n";
            error_creacion_hebras = true;
            break;
        }
    }

    if (!error_creacion_hebras) {
        for (long j = 0; j < cfg.E; ++j) {
            int idx = static_cast<int>(hebras.size());
            int id_logico = idx;
            try {
                hebras.emplace_back(rutina_escritora, idx, id_logico);
            } catch (const std::system_error& e) {
                std::cerr << "Error al crear hebra escritora " << j << ": " << e.what() << "\n";
                error_creacion_hebras = true;
                break;
            }
        }
    }

    for (auto& th : hebras) {
        if (th.joinable()) {
            th.join();
        }
    }

    const auto pared_fin = std::chrono::steady_clock::now();
    const double cpu_fin = tiempo_cpu_proceso_seg();

    if (error_creacion_hebras) {
        std::cerr << "Ejecucion abortada tras esperar a las hebras ya creadas.\n";
        return 1;
    }

    const double tiempo_pared_seg =
        std::chrono::duration<double>(pared_fin - pared_inicio).count();
    const double tiempo_cpu_seg = std::max(0.0, cpu_fin - cpu_inicio);

    // Consolida tiempos de espera y fallos de admision de todas las hebras
    double suma_esp_lec = 0.0;
    double max_esp_lec = 0.0;
    long cnt_lec = 0;

    double suma_esp_esc = 0.0;
    double max_esp_esc = 0.0;
    long cnt_esc = 0;

    FallosAdmision total_fallos{};

    for (std::size_t i = 0; i < hebras.size(); ++i) {
        const auto& mh = metricas[i];
        if (static_cast<long>(i) < cfg.L) {
            suma_esp_lec += mh.suma_espera_us;
            if (mh.max_espera_us > max_esp_lec) {
                max_esp_lec = mh.max_espera_us;
            }
            cnt_lec += mh.conteo;
        } else {
            suma_esp_esc += mh.suma_espera_us;
            if (mh.max_espera_us > max_esp_esc) {
                max_esp_esc = mh.max_espera_us;
            }
            cnt_esc += mh.conteo;
        }

        total_fallos.recurso_ocupado += mh.fallos_atomicos.recurso_ocupado;
        total_fallos.escritoras_esperando += mh.fallos_atomicos.escritoras_esperando;
        total_fallos.cas_fallido += mh.fallos_atomicos.cas_fallido;
    }

    const double prom_esp_lec_us = (cnt_lec > 0) ? (suma_esp_lec / static_cast<double>(cnt_lec)) : 0.0;
    const double prom_esp_esc_us = (cnt_esc > 0) ? (suma_esp_esc / static_cast<double>(cnt_esc)) : 0.0;

    const long version_esperada = cfg.E * cfg.K;
    const long doble_version_esperada = 2L * cfg.E * cfg.K;
    const long inconsistencias = diag.inconsistencias.load(std::memory_order_relaxed);
    const long violaciones_excl = diag.violaciones_exclusion.load(std::memory_order_relaxed);
    const long max_lec_sim = diag.max_lectoras_simultaneas.load(std::memory_order_relaxed);

    const bool invariantes_ok =
        (registro.version == version_esperada) &&
        (registro.doble_version == doble_version_esperada) &&
        (inconsistencias == 0) &&
        (violaciones_excl == 0);

    // Imprime el resumen de ejecucion y metricas finales
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "modo=" << cfg.modo_str
              << " politica=" << cfg.politica_str
              << " L=" << cfg.L
              << " E=" << cfg.E
              << " K=" << cfg.K
              << " traza=" << (cfg.con_traza ? "SI" : "NO") << "\n";
    std::cout << "version_final=" << registro.version
              << " (esperada=" << version_esperada << ")"
              << " doble_version_final=" << registro.doble_version
              << " (esperada=" << doble_version_esperada << ")"
              << " inconsistencias=" << inconsistencias
              << " violaciones_exclusion=" << violaciones_excl
              << " max_lectoras_simultaneas=" << max_lec_sim << "\n";
    std::cout << "tiempo_pared_s=" << tiempo_pared_seg
              << " tiempo_cpu_s=" << tiempo_cpu_seg << "\n";
    std::cout << "espera_lectoras_prom_us=" << prom_esp_lec_us
              << " espera_lectoras_max_us=" << max_esp_lec
              << " espera_escritoras_prom_us=" << prom_esp_esc_us
              << " espera_escritoras_max_us=" << max_esp_esc << "\n";
    std::cout << "correcto=" << (invariantes_ok ? "SI" : "NO") << "\n";

    if (cfg.es_atomico) {
        std::cout << "fallos_recurso_ocupado=" << total_fallos.recurso_ocupado
                  << " fallos_escritoras_esperando=" << total_fallos.escritoras_esperando
                  << " fallos_cas=" << total_fallos.cas_fallido << "\n";
    } else if (cfg.con_traza && monitor_ptr != nullptr) {
        auto traza = monitor_ptr->obtener_traza();
        ResultadoVerificacion res = verificar_traza(traza, cfg.politica);
        std::cout << "politica_ok=" << (res.valida ? "SI" : "NO")
                  << " evidencia=" << res.evidencia << "\n";
    }

    return invariantes_ok ? 0 : 1;
}

