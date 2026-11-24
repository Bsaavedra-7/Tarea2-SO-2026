#include "verificador.hpp"

#include <unordered_map>
#include <unordered_set>

thread_local int id_hebra = -1;

// la funcion que recibe la traza y devuelve la validez de dicha traza y la evidencia de ello
ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica
) {
    ResultadoVerificacion resultado{true, 0};// se asume que la traza es valida, se cambia si se encuentra una incoherencia

    // ocupo la estructura para guardar la situacion de cada hebra
    struct EstadoHebra {
        Tipo tipo;//lectora o escritora
        bool pendiente = false;
        bool activa = false;
    };

    std::unordered_map<int, EstadoHebra> estados;//guarda estados de cada hebra
    std::unordered_set<int> lectores_activos;


    //contadores generales pra las hebras
    int escritor_activo = -1;
    int lectores_pendientes = 0;
    int escritores_pendientes = 0;


    long siguiente_turno_pedido = 0;
    long siguiente_turno_entrada = 0;

    //recorer la traza
    for (const AnotacionTraza& evento : traza) {
        EstadoHebra& estado = estados[evento.hebra];//esto permite actualizar la info de la hebra

        switch (evento.accion) {
        case Accion::Pide:

            if (politica == Politica::Fifo){
                if (evento.turno != siguiente_turno_pedido){
                    resultado.valida = false;
                    return resultado;
                }

                ++siguiente_turno_pedido;
            }else if (evento.turno != -1) {
                resultado.valida = false;
                return resultado;
            }



            //primero verifica que la hebra no este actuva o esperando
            if (estado.pendiente || estado.activa) {
                resultado.valida = false;
                return resultado;
            }

            estado.tipo = evento.tipo;
            estado.pendiente = true;

            //aumenta el contador correspondiente
            if (evento.tipo == Tipo::Lectora){
                ++lectores_pendientes;
            }else{
                ++escritores_pendientes;
            }

            break;

        case Accion::Entra:
        //no puede entrar una hebra que no haya hecho Pide antes
            if (!estado.pendiente || estado.tipo != evento.tipo) {
                resultado.valida = false;
                return resultado;
            }

            //esto evita que entre una hebra con turno 1 cuando todavia corresponde el turno 0
            if (politica == Politica::Fifo &&
                evento.turno != siguiente_turno_entrada) {
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
//a

            if (politica == Politica::Fifo){
                ++siguiente_turno_entrada;
            }

            break;

        case Accion::Sale:
        //comprueba que estuviera activa la hebra
            if (!estado.activa || estado.tipo != evento.tipo) {
                resultado.valida = false;
                return resultado;
            }

            estado.activa = false;//se desactiva

            if (evento.tipo == Tipo::Lectora) {
                lectores_activos.erase(evento.hebra);
            } else {
                escritor_activo = -1;
            }

            break;
        }
    }

    //al final se verifica que no queden hebras
    for (const auto& par : estados) {//se ocupa auto& para deducir el tipo y referenciarlo no usarlo directamente, osea revisa que el estado de la hebra nos ea pendiente
        if (par.second.pendiente || par.second.activa) {
            resultado.valida = false;
            return resultado;
        }
    }

    return resultado;
}