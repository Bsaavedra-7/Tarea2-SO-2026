# Tarea 2 - Sistemas Operativos (2026)

En esta tarea se implementa un problema de sincronizacion en **C++17**  de **Lectores y Escritores** donde se compara el costo de la espera bloqueante y la activa.


## Integrantes
- Rodrigo Domingo Larenas
- Nicolas Garcia Medina
- Felipe Grandon Contreras
- Benjamin Saavedra Uribe

## Compilación: 
Para compilar el proyecto se requiere de un compilador con **C++17** (`g++`) y soporte hebras POSIX en linux.

Si se cuenta con los requisitos previamente mencionados ejecute: `make` y ejecute como en los ejemplos de abajo

## Ejecución:
`./rw <modo> <politica> <L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]`

## Ejemplos:
- ./rw cond escritores 6 2 100 200 500
- ./rw atomic escritores 6 2 100 200 500
- ./rw cond fifo 6 2 100 200 500 notraza
