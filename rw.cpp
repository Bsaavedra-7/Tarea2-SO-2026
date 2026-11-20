#include <iostream>
#include <string>
#include "control_rw.hpp"
#include "monitor_rw.hpp"
#include "control_atomico_rw.hpp"
#include "verificador.hpp"

/* 
Main

Punto de entrada del programa, 
Valida los argumentos de consola, instancia MonitorRW o ControlAtomicoRW segun corresponda,
Crea y espera las L + E hebras de lectores y escritores,
mide los tiempos de pared, CPU y espera
y por ultimo ejecuta el verificador y muetra le resultado en pantalla.

*/

int main(int argc, char* argv[]) {
    if (argc < 8 || argc > 9) {
        std::cerr << "Uso: ./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
        return 1;
    }
    std::cout << "Estructura base compilada correctamente.\n";
    return 0;
}

//std::this_thread::sleep_for(std::chrono::microseconds(dur_esc_us));
