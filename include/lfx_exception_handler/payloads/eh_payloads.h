#pragma once
#include <cstdint>

namespace lfx::eh {

    // --- DEFINIZIONE CAMPI BITMASK (4 bit Gravità | 8 bit Sorgente | 20 bit Dettaglio) ---
    enum class Severity : uint8_t {
        Info     = 0x1, // 0001
        Warning  = 0x2, // 0010
        Error    = 0x3, // 0011
        Critical = 0x4  // 0100
    };

    enum class SourceModule : uint8_t {
        None       = 0x00,
        ECS        = 0x01,
        Audio      = 0x02,
        Graphics   = 0x03,
        Midi       = 0x04,
        AssetLoad  = 0x05,
        EventBus   = 0x06
    };

    // Helper constexpr per impacchettare e spacchettare i 32 bit in 1 ciclo di CPU
    namespace bitmask {
        constexpr uint32_t Pack(Severity sev, SourceModule src, uint32_t detailCode) noexcept {
            return ((static_cast<uint32_t>(sev) & 0x0F) << 28) |
                   ((static_cast<uint32_t>(src) & 0xFF) << 20) |
                   (detailCode & 0xFFFFF);
        }

        constexpr Severity UnpackSeverity(uint32_t mask) noexcept {
            return static_cast<Severity>((mask >> 28) & 0x0F);
        }

        constexpr SourceModule UnpackSource(uint32_t mask) noexcept {
            return static_cast<SourceModule>((mask >> 20) & 0xFF);
        }

        constexpr uint32_t UnpackDetail(uint32_t mask) noexcept {
            return mask & 0xFFFFF;
        }
    }

    // 1. PAYLOAD INGRESSO (Dai vari moduli -> Exception Handler)
    struct ExceptionInPayload {
        uint32_t errorMask = 0; // Bitmask 4-8-20
    };

    // 2. PAYLOAD USCITA (Dall'Action Executor -> verso i moduli tramite Broker)
    enum class RecoveryAction : uint8_t {
        None = 0,
        LogOnly,
        FlushQueues,
        RestartModule,
        EmergencyShutdown
    };

    struct ExceptionOutPayload {
        SourceModule targetModule = SourceModule::None;
        RecoveryAction action     = RecoveryAction::None;
        uint32_t originalMask     = 0;
    };

    // 3. PAYLOAD CLOCK FINTO (Per aggiornare la frequenza di risveglio del loop)
    struct DummyClockSyncPayload {
        uint32_t graphicsFps      = 60;    // es. 120 FPS
        uint32_t audioSampleRate  = 48000; // es. 48000 Hz
        uint32_t audioBufferSize  = 512;   // es. 192 campioni -> 250 UPS
    };

} // namespace lfx::eh
