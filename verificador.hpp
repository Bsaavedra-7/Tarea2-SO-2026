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
    int hebra;      // identificador de la hebra
    Tipo tipo;
    Accion accion;
    long turno;    
};


extern thread_local int id_hebra;