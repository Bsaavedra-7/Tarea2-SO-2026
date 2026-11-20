COMPILACION

Para compilar el proyecto se requiere un compilador con soporte para C++17 y hebras POSIX (-pthread) en entorno Linux.

Generar el ejecutable rw:
make

Limpiar archivos compilados:
make clean


EJECUCION

Sintaxis:
./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]

Parametros:
<modo>       : cond (monitor) o atomic (operaciones atomicas)
<politica>   : lectores, escritores o fifo (atomic solo admite escritores)
<L>          : numero de hebras lectoras (L >= 1)
<E>          : numero de hebras escritoras (E >= 1, con L + E <= 1000)
<K>          : cantidad de operaciones por hebra (K >= 1)
<dur_lec_us> : duracion simulada de cada lectura en microsegundos (>= 0)
<dur_esc_us> : duracion simulada de cada escritura en microsegundos (>= 0)
[notraza]    : opcional, desactiva el registro de la traza de admision


EJEMPLOS DE EJECUCION (PRUEBAS E1 Y E2)

./rw cond lectores 6 2 100 500 200
./rw cond escritores 6 2 100 500 200
./rw cond fifo 6 2 100 500 200
./rw atomic escritores 6 2 100 500 200
./rw cond escritores 8 8 10000 0 0 notraza
./rw atomic escritores 8 8 10000 0 0 notraza