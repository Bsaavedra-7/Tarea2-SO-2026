#pragma once

#include <cstddef>
#include <string>
#include <vector>

/* 
R6

define los enum
Politica, Tipo, Accion

La estructura AnotacionTraz, 
La variable global "thread_local int id_hebra"
y la firma de verificar_traza()


*/


enum class Politica { Lectores, Escritores, Fifo };
enum class Tipo { Lectora, Escritora };
enum class Accion { Pide, Entra, Sale };

struct AnotacionTraza {
    int hebra;
    Tipo tipo;
    Accion accion;
    long turno;
};

struct ResultadoVerificacion {
    bool valida = true;
    long evidencia = 0;
    std::size_t anotacion_error = 0; // indice desde 0; traza.size() si falta una anotacion final
    std::string regla_incumplida;
};

extern thread_local int id_hebra;

ResultadoVerificacion verificar_traza(
    const std::vector<AnotacionTraza>& traza,
    Politica politica
);
