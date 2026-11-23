#include "control_atomico_rw.hpp"
#include <thread>

thread_local FallosAdmision ControlAtomicoRW::fallos{};

void ControlAtomicoRW::entrar_lectura() {
    for (;;) {
        auto actual = estado.load();
        //si hay una escritora activa y otra esperando, contamos solo recurso ocupado (r10).
        if (actual & activa) {
            ++fallos.recurso_ocupado;
        } else if (actual >= una_pendiente) {
            ++fallos.escritoras_esperando;
        } else if (estado.compare_exchange_weak(actual, actual + 1)) {
            //el cas confirmó que seguía sin haber escritoras activas ni pendientes.
            return;
        } else {
            ++fallos.cas_fallido;
        }
        std::this_thread::yield();
    }
}

void ControlAtomicoRW::salir_lectura() {
    estado.fetch_sub(1);
}

void ControlAtomicoRW::entrar_escritura() {
    //nos anotamos una sola vez. desde aquí, las nuevas lectoras tienen que esperar.
    estado.fetch_add(una_pendiente);
    for (;;) {
        auto actual = estado.load();
        if (actual & (activa | mascara_lectoras)) {
            ++fallos.recurso_ocupado;
        } else {
            //pasamos de pendiente a activa en el mismo cas para que ninguna lectora vea el recurso libre en medio.
            const auto nuevo = (actual - una_pendiente) | activa;
            if (estado.compare_exchange_weak(actual, nuevo)) {
                return;
            }
            ++fallos.cas_fallido;
        }
        std::this_thread::yield();
    }
}

void ControlAtomicoRW::salir_escritura() {
    //limpiamos solo el bit de activa. otra escritora pudo anotarse mientras esta escribía.
    estado.fetch_and(~activa);
}

FallosAdmision ControlAtomicoRW::retirar_fallos_hebra() {
    //cada hebra copia sus fallos antes de terminar. el main suma las copias después de los join.
    auto resultado = fallos;
    fallos = {};
    return resultado;
}
