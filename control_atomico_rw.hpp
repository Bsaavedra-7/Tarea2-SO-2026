#pragma once

#include "control_rw.hpp"
#include <atomic>
#include <cstdint>

struct FallosAdmision {
    std::uint64_t recurso_ocupado = 0;
    std::uint64_t escritoras_esperando = 0;
    std::uint64_t cas_fallido = 0;
};

class ControlAtomicoRW final : public ControlRW {
public:
    void entrar_lectura() override;
    void salir_lectura() override;
    void entrar_escritura() override;
    void salir_escritura() override;

    //cada hebra retira sus propios resultados antes de terminar
    static FallosAdmision retirar_fallos_hebra();

private:
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    //bits 0..9: lectoras. bit 10: escritora activa. bits 11..20: pendientes
    static constexpr std::uint64_t activa = 1ULL << 10;
    static constexpr std::uint64_t mascara_lectoras = activa - 1;
    static constexpr std::uint64_t una_pendiente = 1ULL << 11;
    std::atomic<std::uint64_t> estado{0};
    static thread_local FallosAdmision fallos;
};
