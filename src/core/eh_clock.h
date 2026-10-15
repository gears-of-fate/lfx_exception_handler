#pragma once
#include <algorithm>
#include <cstdint>
#include "lfx_exception_handler/core/eh_config.h"
#include "lfx_exception_handler/payloads/eh_payloads.h"

namespace lfx::eh {

    class AdaptiveClock {
    private:
        uint32_t m_sleepDurationMicros;
        uint32_t m_oversampling;

    public:
        explicit AdaptiveClock(const HandlerConfig& cfg)
            : m_sleepDurationMicros(cfg.fallbackSleepMicros)
            , m_oversampling(std::max(1u, cfg.oversamplingFactor)) {}

        // Ricalcola l'intervallo di risveglio in base al motore attivo più veloce
        void UpdateFromClocks(const DummyClockSyncPayload& sync, const HandlerConfig& cfg) noexcept {
            double fastestHz = 0.0;

            if (cfg.monitorGraphics && sync.graphicsFps > 0) {
                fastestHz = static_cast<double>(sync.graphicsFps);
            }

            if (cfg.monitorAudio && sync.audioSampleRate > 0 && sync.audioBufferSize > 0) {
                double audioUps = static_cast<double>(sync.audioSampleRate) / sync.audioBufferSize;
                if (audioUps > fastestHz) {
                    fastestHz = audioUps;
                }
            }

            if (fastestHz <= 0.0) {
                m_sleepDurationMicros = cfg.fallbackSleepMicros;
                return;
            }

            // Periodo del motore più veloce in microsecondi (es. 1.000.000 / 250 Hz = 4000 us)
            double fastestPeriodMicros = 1000000.0 / fastestHz;

            // Dividiamo per l'oversampling (es. 4000 / 2 = 2000 us = 2 ms)
            uint32_t targetMicros = static_cast<uint32_t>(fastestPeriodMicros / m_oversampling);
            
            // Pavimento minimo di sicurezza di 250 microsecondi per non saturare la CPU
            m_sleepDurationMicros = std::max(250u, targetMicros);
        }

        uint32_t GetSleepMicros() const noexcept { return m_sleepDurationMicros; }
    };

} // namespace lfx::eh