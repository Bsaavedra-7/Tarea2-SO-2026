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
    int lectores_pendientes = 0;
    int escritores_pendientes = 0;

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

            if (evento.tipo == Tipo::Lectora){
                ++lectores_pendientes;
            }else{
                ++escritores_pendientes;
            }

            break;

        case Accion::Entra:
            if (!estado.pendiente || estado.tipo != evento.tipo) {
                resultado.valida = false;
                return resultado;
            }

            if (evento.tipo == Tipo::Lectora) {
                if (escritor_activo != -1) {//rechaza la entrad si hay un escritor activo
                    resultado.valida = false;
                    return resultado;
                }
                //rechaza la entrrada de una lectora si hay una escritora esperando, prioridad de escritoras
                if (politica == Politica::Escritores && escritores_pendientes > 0){
                    resultado.valida = false;
                    return resultado;
                }

                if (politica == Politica::Lectores && escritores_pendientes > 0){
                    ++resultado.evidencia;
                }


                --lectores_pendientes;//indica que la lectora dejo de estar pendiente


                //aca se actualiza el estado de la hebra, ya no espera y esta dentro de la seccion critica
                estado.pendiente = false;
                estado.activa = true;
                lectores_activos.insert(evento.hebra);//registra que esa lectora esta activa
            } else {
                if (escritor_activo != -1 || !lectores_activos.empty()) {
                    resultado.valida = false;
                    return resultado;
                }

                //con prioridad de lectores una escritora no puede entrar is hay una lectora esperando
                if (politica == Politica::Lectores && lectores_pendientes > 0){
                    resultado.valida = false;
                    return resultado;
                }

                --escritores_pendientes;

                if (politica == Politica::Escritores && lectores_pendientes > 0){
                    ++resultado.evidencia;
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