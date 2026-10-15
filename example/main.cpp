#include <iostream>
#include <queue>
#include "lfx_exception_handler/LfxExceptionHandler.h"

using namespace lfx::eh;

// Simuliamo i canali SPSC separati del tuo Broker (1 In e 1 Out per ogni motore)
struct MockBrokerChannels {
    std::queue<ExceptionInPayload> audioToEhChannel;
    std::queue<ExceptionInPayload> graphicsToEhChannel;

    std::queue<ExceptionOutPayload> ehToAudioChannel;
    std::queue<ExceptionOutPayload> ehToGraphicsChannel;
};

int main() {
    HandlerConfig cfg;
    cfg.monitorAudio       = true;
    cfg.monitorGraphics    = true;
    cfg.monitorMidi        = false;
    cfg.oversamplingFactor = 2;

    ExceptionHandler handler(cfg);
    MockBrokerChannels broker;

    // 1. I motori comunicano i loro clock all'avvio:
    // Grafica = 120 FPS (8.33 ms) | Audio = 48000 Hz / 192 samples = 250 UPS (4 ms -> Più veloce!)
    handler.UpdateClock({ .graphicsFps = 120, .audioSampleRate = 48000, .audioBufferSize = 192 });
    std::cout << "[Clock] Motore piu' veloce: Audio (250 UPS / 4ms). "
              << "Risveglio Handler impostato a: " << handler.GetCurrentSleepMicros() << " us (2 ms)\n\n";

    // 2. Attacchiamo il LISTENER (Consumatore) ai canali di ingresso specifici del Broker
    handler.BindExceptionConsumer([&broker](ExceptionInPayload& inEv) {
        if (!broker.audioToEhChannel.empty()) {
            inEv = broker.audioToEhChannel.front();
            broker.audioToEhChannel.pop();
            return true;
        }
        if (!broker.graphicsToEhChannel.empty()) {
            inEv = broker.graphicsToEhChannel.front();
            broker.graphicsToEhChannel.pop();
            return true;
        }
        return false;
    });

    // 3. Attacchiamo l'ACTION EXECUTOR (Produttore) ai canali di uscita specifici del Broker
    handler.BindActionProducer([&broker](const ExceptionOutPayload& out) {
        switch (out.targetModule) {
            case SourceModule::Audio:
                broker.ehToAudioChannel.push(out);
                return true;
            case SourceModule::Graphics:
                broker.ehToGraphicsChannel.push(out);
                return true;
            default:
                return false;
        }
    });

    // --- SIMULAZIONE ---
    // Il Motore Audio ha un problema critico (Buffer Underrun = codice 0x00192) e scrive nel SUO canale
    broker.audioToEhChannel.push({ bitmask::Pack(Severity::Critical, SourceModule::Audio, 0x00192) });

    // Il Motore Grafico ha un piccolo Warning (Frame Drop = codice 0x00001) e scrive nel SUO canale
    broker.graphicsToEhChannel.push({ bitmask::Pack(Severity::Warning, SourceModule::Graphics, 0x00001) });

    // L'Exception Handler si sveglia e drena tutti i canali del Broker
    handler.DrainAndDispatchOnce();

    // --- VERIFICA SUI CANALI DI RITORNO DEI SINGOLI MOTORI ---
    if (!broker.ehToAudioChannel.empty()) {
        auto msg = broker.ehToAudioChannel.front();
        std::cout << "[Audio Engine] Ricevuto ordine sul canale ehToAudio! "
                  << "Azione: " << static_cast<int>(msg.action) << " (RestartModule) "
                  << "| Dettaglio: 0x" << std::hex << bitmask::UnpackDetail(msg.originalMask) << std::dec << "\n";
    }

    if (!broker.ehToGraphicsChannel.empty()) {
        auto msg = broker.ehToGraphicsChannel.front();
        std::cout << "[Graphics Engine] Ricevuto ordine sul canale ehToGraphics! "
                  << "Azione: " << static_cast<int>(msg.action) << " (LogOnly) "
                  << "| Dettaglio: 0x" << std::hex << bitmask::UnpackDetail(msg.originalMask) << std::dec << "\n";
    }

    return 0;
}