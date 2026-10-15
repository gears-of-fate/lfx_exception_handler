#pragma once
#include <functional>
#include "lfx_exception_handler/core/eh_config.h"
#include "lfx_exception_handler/payloads/eh_payloads.h"

namespace lfx::eh {

    class ActionExecutor {
    public:
        using ProducerCallback = std::function<bool(const ExceptionOutPayload&)>;

    private:
        ProducerCallback m_brokerProducer;

        static bool IsModuleEnabled(SourceModule mod, const HandlerConfig& cfg) noexcept {
            switch (mod) {
                case SourceModule::ECS:      return cfg.monitorEcs;
                case SourceModule::Audio:    return cfg.monitorAudio;
                case SourceModule::Graphics: return cfg.monitorGraphics;
                case SourceModule::Midi:     return cfg.monitorMidi;
                default:                     return true;
            }
        }

    public:
        void SetProducer(ProducerCallback cb) {
            m_brokerProducer = std::move(cb);
        }

        bool Execute(const ExceptionOutPayload& outAction, const HandlerConfig& cfg) noexcept {
            if (outAction.action == RecoveryAction::None) {
                return false;
            }

            if (!IsModuleEnabled(outAction.targetModule, cfg)) {
                return false;
            }

            if (m_brokerProducer) {
                return m_brokerProducer(outAction);
            }
            return false;
        }
    };

} // namespace lfx::eh