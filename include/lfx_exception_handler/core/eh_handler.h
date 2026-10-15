#pragma once
#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include "eh_config.h"
#include "../payloads/eh_payloads.h"

namespace lfx::eh {

    // Forward declarations per isolamento totale
    class AdaptiveClock;
    class ActionExecutor;

    class ExceptionHandler {
    public:
        using ConsumerCallback  = std::function<bool(ExceptionInPayload&)>;
        using ProducerCallback  = std::function<bool(const ExceptionOutPayload&)>;
        using ClockSyncCallback = std::function<bool(DummyClockSyncPayload&)>;

    private:
        HandlerConfig m_config;
        std::unique_ptr<AdaptiveClock> m_clock;
        std::unique_ptr<ActionExecutor> m_executor;

        ConsumerCallback m_brokerConsumer;
        ClockSyncCallback m_clockConsumer;

        std::atomic<bool> m_isRunning{false};
        std::thread m_workerThread;

        void PollingLoop() noexcept;

    public:
        explicit ExceptionHandler(const HandlerConfig& config);
        ~ExceptionHandler();

        ExceptionHandler(const ExceptionHandler&) = delete;
        ExceptionHandler& operator=(const ExceptionHandler&) = delete;

        // --- BINDING AL BROKER ---
        void BindExceptionConsumer(ConsumerCallback cb);
        void BindActionProducer(ProducerCallback cb);
        void BindClockSyncConsumer(ClockSyncCallback cb);

        // --- CONTROLLO DEL CLOCK E POLLING ---
        void UpdateClock(const DummyClockSyncPayload& sync) noexcept;
        uint32_t GetCurrentSleepMicros() const noexcept;

        // Svuota tutte le eccezioni attualmente presenti nel Broker (1 ciclo di drain)
        size_t DrainAndDispatchOnce() noexcept;

        // --- GESTIONE THREAD DEDICATO (Soft Real-Time) ---
        void StartAsyncLoop();
        void StopAsyncLoop();
        bool IsRunning() const noexcept { return m_isRunning.load(std::memory_order_relaxed); }

        const HandlerConfig& GetConfig() const noexcept { return m_config; }
    };

} // namespace lfx::eh