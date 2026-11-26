#pragma once

#include "control_rw.hpp"
#include "verificador.hpp"

#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <vector>

class MonitorRW : public ControlRW {
public:
    explicit MonitorRW(Politica p, bool con_traza = true);
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;
    std::vector<AnotacionTraza> obtener_traza();

private:
    void anotar(Tipo tipo, Accion accion, long turno);

    Politica politica;
    bool con_traza;
    std::mutex m;
    std::condition_variable cv;
    long lectoras_activas = 0;
    long lectoras_pendientes = 0;
    long escritoras_pendientes = 0;
    bool escritora_activa = false;
    long siguiente_turno = 0;
    long siguiente_admision = 0;
    std::unordered_map<int, long> turnos_activos;
    std::vector<AnotacionTraza> traza;
};
