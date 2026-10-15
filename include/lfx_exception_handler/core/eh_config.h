#pragma once
#include <cstdint>

namespace lfx::eh {

    struct HandlerConfig {
        // Interruttori dei moduli attivi nel sistema
        bool monitorEcs      = true;
        bool monitorAudio    = true;
        bool monitorGraphics = true;
        bool monitorMidi     = false;

        // Quante volte mi sveglio dentro il ciclo del clock più veloce? (es. 2 o 3 volte)
        uint32_t oversamplingFactor = 2;

        // Intervallo di fallback (in microsecondi) se sia Audio che Grafica sono spenti
        uint32_t fallbackSleepMicros = 5000; // 5 ms
    };

} // namespace lfx::eh
