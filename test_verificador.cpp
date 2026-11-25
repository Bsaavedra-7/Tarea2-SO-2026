#include "verificador.hpp"

#include <iostream>
#include <vector>

bool comprobar(
    const char* nombre,
    const std::vector<AnotacionTraza>& traza,
    Politica politica,
    bool esperado
) {
    ResultadoVerificacion resultado =
        verificar_traza(traza, politica);

    bool paso = resultado.valida == esperado;

    std::cout << (paso ? "[OK] " : "[FALLO] ")
              << nombre
              << " | valida=" << resultado.valida
              << " | esperado=" << esperado
              << " | evidencia=" << resultado.evidencia
              << "\n";

    return paso;
}

int main() {
    std::vector<AnotacionTraza> lectores_valida = {
        {2, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Lectora, Accion::Entra, -1},
        {1, Tipo::Lectora, Accion::Sale, -1},
        {2, Tipo::Escritora, Accion::Entra, -1},
        {2, Tipo::Escritora, Accion::Sale, -1}
    };

    std::vector<AnotacionTraza> escritores_valida = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {1, Tipo::Escritora, Accion::Sale, -1},
        {2, Tipo::Lectora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Sale, -1}
    };

    std::vector<AnotacionTraza> fifo_valida = {
        {1, Tipo::Lectora, Accion::Pide, 0},
        {2, Tipo::Escritora, Accion::Pide, 1},
        {1, Tipo::Lectora, Accion::Entra, 0},
        {1, Tipo::Lectora, Accion::Sale, 0},
        {2, Tipo::Escritora, Accion::Entra, 1},
        {2, Tipo::Escritora, Accion::Sale, 1}
    };

    std::vector<AnotacionTraza> exclusion_invalida = {
        {1, Tipo::Escritora, Accion::Pide, -1},
        {1, Tipo::Escritora, Accion::Entra, -1},
        {2, Tipo::Lectora, Accion::Pide, -1},
        {2, Tipo::Lectora, Accion::Entra, -1}
    };

    std::vector<AnotacionTraza> fifo_invalida = {
        {1, Tipo::Lectora, Accion::Pide, 0},
        {2, Tipo::Escritora, Accion::Pide, 1},
        {2, Tipo::Escritora, Accion::Entra, 1},
        {2, Tipo::Escritora, Accion::Sale, 1},
        {1, Tipo::Lectora, Accion::Entra, 0},
        {1, Tipo::Lectora, Accion::Sale, 0}
    };

    bool correcto = true;

    correcto &= comprobar(
    "Lectores válida",
    lectores_valida,
    Politica::Lectores,
    true
    );

    correcto &= comprobar(
        "Escritores válida",
        escritores_valida,
        Politica::Escritores,
        true
    );

    correcto &= comprobar(
        "FIFO válida",
        fifo_valida,
        Politica::Fifo,
        true
    );

    correcto &= comprobar(
        "Exclusión inválida",
        exclusion_invalida,
        Politica::Escritores,
        false
    );

    correcto &= comprobar(
        "FIFO inválida",
        fifo_invalida,
        Politica::Fifo,
        false
    );
    if (correcto) {
        std::cout << "Todas las pruebas pasaron\n";
        return 0;
    }

    std::cout << "Algunas pruebas fallaron\n";
    return 1;
}
