#include <gtest/gtest.h>
#include <queue>
#include <vector>
#include "lfx_exception_handler/LfxExceptionHandler.h"

using namespace lfx::eh;

// 1. Test Bitmask 4-8-20 (Gravità, Sorgente, Dettaglio)
TEST(ExceptionHandlerTest, BitmaskPackAndUnpack) {
    uint32_t mask = bitmask::Pack(Severity::Critical, SourceModule::Audio, 0xABCDE);

    EXPECT_EQ(bitmask::UnpackSeverity(mask), Severity::Critical);
    EXPECT_EQ(bitmask::UnpackSource(mask), SourceModule::Audio);
    EXPECT_EQ(bitmask::UnpackDetail(mask), 0xABCDEu);
}

// 2. Test Calcolo Clock (120 FPS vs 48000Hz/192 campioni = 250 UPS -> 4ms / 2 = 2000 microsecondi)
TEST(ExceptionHandlerTest, AdaptiveClockSelectsFastestEngine) {
    HandlerConfig cfg;
    cfg.monitorGraphics    = true;
    cfg.monitorAudio       = true;
    cfg.oversamplingFactor = 2;

    ExceptionHandler handler(cfg);

    DummyClockSyncPayload clocks{
        .graphicsFps     = 120,   // 8.33 ms
        .audioSampleRate = 48000, // 48000 / 192 = 250 UPS (4.0 ms -> il più veloce!)
        .audioBufferSize = 192
    };

    handler.UpdateClock(clocks);

    // 4000 us / 2 (oversampling) = 2000 us (2 ms)
    EXPECT_EQ(handler.GetCurrentSleepMicros(), 2000u);
}

// 3. Test Flusso Completo: Consumer -> Dispatcher -> Action Executor (Producer)
TEST(ExceptionHandlerTest, DrainAndDispatchPipeline) {
    HandlerConfig cfg;
    cfg.monitorAudio = true;
    cfg.monitorMidi  = false; // MIDI spento!

    ExceptionHandler handler(cfg);

    std::queue<ExceptionInPayload> fakeBrokerInQueue;
    std::vector<ExceptionOutPayload> fakeBrokerOutActions;

    handler.BindExceptionConsumer([&](ExceptionInPayload& outEv) {
        if (fakeBrokerInQueue.empty()) return false;
        outEv = fakeBrokerInQueue.front();
        fakeBrokerInQueue.pop();
        return true;
    });

    handler.BindActionProducer([&](const ExceptionOutPayload& inAction) {
        fakeBrokerOutActions.push_back(inAction);
        return true;
    });

    fakeBrokerInQueue.push({ bitmask::Pack(Severity::Critical, SourceModule::Audio, 404) });
    fakeBrokerInQueue.push({ bitmask::Pack(Severity::Error, SourceModule::Midi, 101) });

    size_t processed = handler.DrainAndDispatchOnce();
    EXPECT_EQ(processed, 2u);

    // Solo l'azione per l'Audio deve essere stata prodotta (MIDI era spento nella config!)
    ASSERT_EQ(fakeBrokerOutActions.size(), 1u);
    EXPECT_EQ(fakeBrokerOutActions[0].targetModule, SourceModule::Audio);
    EXPECT_EQ(fakeBrokerOutActions[0].action, RecoveryAction::RestartModule);
}