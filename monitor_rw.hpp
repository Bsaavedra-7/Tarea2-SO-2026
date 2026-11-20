#include "control_rw.hpp"
#include "verificador.hpp"
#include <mutex>
#include <condition_variable>
#include <vector>

/* 
modo cond

Declara la clase MonitorRW, derivada de ControlRW.
usa std::mitex, las varibles de condicion, las variables de estado privadas y el vector traza.

*/


class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica p);
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;
    std::vector<AnotacionTraza> obtener_traza(); // metodo para obtener la traza de eventos

private:
    Politica politica;
    std::mutex m;                      // mutex del monitor
    std::vector<AnotacionTraza> traza; 

};