#include "verificador.hpp"

#include <unordered_map>
#include <unordered_set>

thread_local int id_hebra = -1;

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica
) {
    ResultadoVerificacion resultado{true, 0};

    struct EstadoHebra {
        Tipo tipo;
        bool pendiente = false;
        bool activa = false;
    };

    std::unordered_map<int, EstadoHebra> estados;
    std::unordered_set<int> lectores_activos;

    int escritor_activo = -1;

    for (const AnotacionTraza& evento : traza) {
        EstadoHebra& estado = estados[evento.hebra];

        switch (evento.accion) {
        case Accion::Pide:
            if (estado.pendiente || estado.activa) {
                resultado.valida = false;
                return resultado;
            }

            estado.tipo = evento.tipo;
            estado.pendiente = true;
            break;

        case Accion::Entra:
            if (!estado.pendiente || estado.tipo != evento.tipo) {
                resultado.valida = false;
                return resultado;
            }

            if (evento.tipo == Tipo::Lectora) {
                if (escritor_activo != -1) {
                    resultado.valida = false;
                    return resultado;
                }

                estado.pendiente = false;
                estado.activa = true;
                lectores_activos.insert(evento.hebra);
            } else {
                if (escritor_activo != -1 || !lectores_activos.empty()) {
                    resultado.valida = false;
                    return resultado;
                }

                estado.pendiente = false;
                estado.activa = true;
                escritor_activo = evento.hebra;
            }

            break;

        case Accion::Sale:
            if (!estado.activa || estado.tipo != evento.tipo) {
                resultado.valida = false;
                return resultado;
            }

            estado.activa = false;

            if (evento.tipo == Tipo::Lectora) {
                lectores_activos.erase(evento.hebra);
            } else {
                escritor_activo = -1;
            }

            break;
        }
    }

    for (const auto& par : estados) {
        if (par.second.pendiente || par.second.activa) {
            resultado.valida = false;
            return resultado;
        }
    }

    return resultado;
}