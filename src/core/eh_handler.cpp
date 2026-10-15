#include "lfx_exception_handler/core/eh_handler.h"
#include "eh_clock.h"
#include "eh_dispatcher.h"
#include "eh_action_excecutor.h"
#include <chrono>

namespace lfx::eh {

    ExceptionHandler::ExceptionHandler(const HandlerConfig& config)
        : m_config(config)
        , m_clock(std::make_unique<AdaptiveClock>(config))
        , m_executor(std::make_unique<ActionExecutor>()) 
    {}

    ExceptionHandler::~ExceptionHandler() {
        StopAsyncLoop();
    }

    void ExceptionHandler::BindExceptionConsumer(ConsumerCallback cb) {
        m_brokerConsumer = std::move(cb);
    }

    void ExceptionHandler::BindActionProducer(ProducerCallback cb) {
        m_executor->SetProducer(std::move(cb));
    }

    void ExceptionHandler::BindClockSyncConsumer(ClockSyncCallback cb) {
        m_clockConsumer = std::move(cb);
    }

    void ExceptionHandler::UpdateClock(const DummyClockSyncPayload& sync) noexcept {
        m_clock->UpdateFromClocks(sync, m_config);
    }

    uint32_t ExceptionHandler::GetCurrentSleepMicros() const noexcept {
        return m_clock->GetSleepMicros();
    }

    size_t ExceptionHandler::DrainAndDispatchOnce() noexcept {
        if (m_clockConsumer) {
            DummyClockSyncPayload newSync{};
            while (m_clockConsumer(newSync)) {
                m_clock->UpdateFromClocks(newSync, m_config);
            }
        }

        if (!m_brokerConsumer) return 0;

        size_t processedCount = 0;
        ExceptionInPayload inEv{};

        while (m_brokerConsumer(inEv)) {
            ExceptionOutPayload outAction = Dispatcher::Evaluate(inEv.errorMask);
            m_executor->Execute(outAction, m_config);
            ++processedCount;
        }

        return processedCount;
    }

    void ExceptionHandler::StartAsyncLoop() {
        bool expected = false;
        if (m_isRunning.compare_exchange_strong(expected, true)) {
            m_workerThread = std::thread(&ExceptionHandler::PollingLoop, this);
        }
    }

    void ExceptionHandler::StopAsyncLoop() {
        bool expected = true;
        if (m_isRunning.compare_exchange_strong(expected, false)) {
            if (m_workerThread.joinable()) {
                m_workerThread.join();
            }
        }
    }

    void ExceptionHandler::PollingLoop() noexcept {
        while (m_isRunning.load(std::memory_order_relaxed)) {
            const auto startTime = std::chrono::steady_clock::now();

            DrainAndDispatchOnce();

            const auto sleepDuration = std::chrono::microseconds(m_clock->GetSleepMicros());
            const auto elapsed = std::chrono::steady_clock::now() - startTime;

            if (elapsed < sleepDuration) {
                std::this_thread::sleep_for(sleepDuration - elapsed);
            }
        }
    }

} // namespace lfx::eh