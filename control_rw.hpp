#pragma once

/* 
En este archivo se define la clase ControlRW, con los metodos 
  entrar_lectura,
  salir_lectura, 
  entrar_escritura, 
  salir_escritura
y la estructura Registro.
Como esta en un .hpp compartido tanto el monitor como la verison atomica heredaran de ella

*/


class ControlRW {
public:
    virtual ~ControlRW() = default;
    virtual void entrar_lectura() = 0;
    virtual void salir_lectura() = 0;
    virtual void entrar_escritura() = 0;
    virtual void salir_escritura() = 0;
};

struct Registro {
    long version = 0;
    long doble_version = 0;
};
