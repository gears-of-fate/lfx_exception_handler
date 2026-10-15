#pragma once
#include "lfx_exception_handler/payloads/eh_payloads.h"

namespace lfx::eh {

    class Dispatcher {
    public:
        static ExceptionOutPayload Evaluate(uint32_t mask) noexcept {
            const Severity sev     = bitmask::UnpackSeverity(mask);
            const SourceModule src = bitmask::UnpackSource(mask);

            ExceptionOutPayload out{};
            out.targetModule = src;
            out.originalMask = mask;

            // Unico blocco switch decisionale basato sulla gravità
            switch (sev) {
                case Severity::Info:
                case Severity::Warning:
                    out.action = RecoveryAction::LogOnly;
                    break;

                case Severity::Error:
                    out.action = RecoveryAction::FlushQueues;
                    break;

                case Severity::Critical:
                    if (src == SourceModule::Audio || src == SourceModule::Graphics || src == SourceModule::Midi) {
                        out.action = RecoveryAction::RestartModule;
                    } else {
                        out.action = RecoveryAction::EmergencyShutdown;
                    }
                    break;

                default:
                    out.action = RecoveryAction::None;
                    break;
            }

            return out;
        }
    };

} // namespace lfx::eh