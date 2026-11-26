#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), con_traza(registrar_traza) {}

void MonitorRW::anotar(Tipo tipo, Accion accion, long turno) {
    if (con_traza) {
        traza.push_back({id_hebra, tipo, accion, turno});
    }
}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);
    const long turno = (politica == Politica::Fifo) ? siguiente_turno++ : -1;
    ++lectoras_pendientes;
    anotar(Tipo::Lectora, Accion::Pide, turno);

    cv.wait(lock, [&] {
        if (escritora_activa) return false;
        if (politica == Politica::Escritores && escritoras_pendientes > 0) return false;
        if (politica == Politica::Fifo && turno != siguiente_admision) return false;
        return true;
    });

    --lectoras_pendientes;
    ++lectoras_activas;
    if (politica == Politica::Fifo) ++siguiente_admision;
    if (con_traza && politica == Politica::Fifo) turnos_activos[id_hebra] = turno;
    anotar(Tipo::Lectora, Accion::Entra, turno);
    cv.notify_all();
}

void MonitorRW::salir_lectura() {
    std::lock_guard<std::mutex> lock(m);
    --lectoras_activas;
    const long turno = (con_traza && politica == Politica::Fifo) ? turnos_activos.at(id_hebra) : -1;
    anotar(Tipo::Lectora, Accion::Sale, turno);
    if (con_traza && politica == Politica::Fifo) turnos_activos.erase(id_hebra);
    if (lectoras_activas == 0) cv.notify_all();
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);
    const long turno = (politica == Politica::Fifo) ? siguiente_turno++ : -1;
    ++escritoras_pendientes;
    anotar(Tipo::Escritora, Accion::Pide, turno);

    cv.wait(lock, [&] {
        if (escritora_activa || lectoras_activas > 0) return false;
        if (politica == Politica::Lectores && lectoras_pendientes > 0) return false;
        if (politica == Politica::Fifo && turno != siguiente_admision) return false;
        return true;
    });

    --escritoras_pendientes;
    escritora_activa = true;
    if (politica == Politica::Fifo) ++siguiente_admision;
    if (con_traza && politica == Politica::Fifo) turnos_activos[id_hebra] = turno;
    anotar(Tipo::Escritora, Accion::Entra, turno);
}

void MonitorRW::salir_escritura() {
    std::lock_guard<std::mutex> lock(m);
    escritora_activa = false;
    const long turno = (con_traza && politica == Politica::Fifo) ? turnos_activos.at(id_hebra) : -1;
    anotar(Tipo::Escritora, Accion::Sale, turno);
    if (con_traza && politica == Politica::Fifo) turnos_activos.erase(id_hebra);
    cv.notify_all();
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::lock_guard<std::mutex> lock(m);
    return traza;
}
