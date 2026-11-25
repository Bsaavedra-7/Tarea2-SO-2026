#include "verificador.hpp"

#include <unordered_map>

thread_local int id_hebra = -1;

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica
) {
    struct EstadoHebra {
        Tipo tipo = Tipo::Lectora;
        bool pendiente = false;
        bool activa = false;
        long turno = -1;
    };

    ResultadoVerificacion resultado;
    std::unordered_map<int, EstadoHebra> estados;
    long lectoras_activas = 0;
    long lectoras_pendientes = 0;
    long escritoras_pendientes = 0;
    int escritora_activa = -1;
    long siguiente_turno_pedido = 0;
    long siguiente_turno_entrada = 0;
    int lectora_que_debe_entrar = -1;

    auto fallo = [&](std::size_t indice, const char* regla) {
        resultado.valida = false;
        resultado.anotacion_error = indice;
        resultado.regla_incumplida = regla;
        return resultado;
    };

    for (std::size_t i = 0; i < traza.size(); ++i) {
        const auto& evento = traza[i];

        // En prioridad de lectoras, Pide y Entra son consecutivos si no hay escritora activa.
        if (lectora_que_debe_entrar != -1) {
            if (evento.hebra != lectora_que_debe_entrar ||
                evento.tipo != Tipo::Lectora || evento.accion != Accion::Entra) {
                return fallo(i, "lectora debe entrar inmediatamente despues de Pide");
            }
            lectora_que_debe_entrar = -1;
        }

        auto& estado = estados[evento.hebra];
        if (evento.accion == Accion::Pide) {
            if (estado.pendiente || estado.activa) {
                return fallo(i, "solicitud duplicada de una hebra");
            }
            if (politica == Politica::Fifo) {
                if (evento.turno != siguiente_turno_pedido) {
                    return fallo(i, "turnos de solicitud FIFO no consecutivos");
                }
                ++siguiente_turno_pedido;
            } else if (evento.turno != -1) {
                return fallo(i, "turno debe ser -1 fuera de FIFO");
            }

            // R6 mide solicitudes de lectoras mientras hay escritoras pendientes.
            if (politica == Politica::Escritores &&
                evento.tipo == Tipo::Lectora && escritoras_pendientes > 0) {
                ++resultado.evidencia;
            }
            if (politica == Politica::Lectores &&
                evento.tipo == Tipo::Lectora && escritora_activa == -1) {
                lectora_que_debe_entrar = evento.hebra;
            }

            estado.tipo = evento.tipo;
            estado.turno = evento.turno;
            estado.pendiente = true;
            if (evento.tipo == Tipo::Lectora) {
                ++lectoras_pendientes;
            } else {
                ++escritoras_pendientes;
            }
        } else if (evento.accion == Accion::Entra) {
            if (!estado.pendiente || estado.tipo != evento.tipo) {
                return fallo(i, "Entra sin Pide correspondiente");
            }
            if (evento.turno != estado.turno) {
                return fallo(i, "turno distinto al registrado en Pide");
            }
            if (politica == Politica::Fifo &&
                evento.turno != siguiente_turno_entrada) {
                return fallo(i, "ingresos fuera del orden FIFO");
            }

            if (evento.tipo == Tipo::Lectora) {
                if (escritora_activa != -1) {
                    return fallo(i, "lectora coincide con escritora activa");
                }
                if (politica == Politica::Escritores && escritoras_pendientes > 0) {
                    return fallo(i, "lectora entra con escritoras pendientes");
                }
                if (politica == Politica::Lectores && escritoras_pendientes > 0) {
                    ++resultado.evidencia;
                }
                if (politica == Politica::Fifo && lectoras_activas > 0) {
                    ++resultado.evidencia;
                }
                --lectoras_pendientes;
                ++lectoras_activas;
            } else {
                if (escritora_activa != -1 || lectoras_activas > 0) {
                    return fallo(i, "escritora entra con otra hebra activa");
                }
                if (politica == Politica::Lectores && lectoras_pendientes > 0) {
                    return fallo(i, "escritora entra con lectoras pendientes");
                }
                --escritoras_pendientes;
                escritora_activa = evento.hebra;
            }
            estado.pendiente = false;
            estado.activa = true;
            if (politica == Politica::Fifo) {
                ++siguiente_turno_entrada;
            }
        } else if (evento.accion == Accion::Sale) {
            if (!estado.activa || estado.tipo != evento.tipo) {
                return fallo(i, "Sale sin Entra correspondiente");
            }
            if (evento.turno != estado.turno) {
                return fallo(i, "turno distinto al registrado en Pide");
            }
            estado.activa = false;
            if (evento.tipo == Tipo::Lectora) {
                --lectoras_activas;
            } else {
                escritora_activa = -1;
            }
        } else {
            return fallo(i, "accion desconocida");
        }
    }

    if (lectora_que_debe_entrar != -1) {
        return fallo(traza.size(), "falta Entra inmediatamente despues de Pide");
    }
    for (const auto& par : estados) {
        if (par.second.pendiente || par.second.activa) {
            return fallo(traza.size(), "solicitud sin Entra o Sale final");
        }
    }
    return resultado;
}
