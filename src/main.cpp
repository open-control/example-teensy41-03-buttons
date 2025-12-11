/**
 * @file main.cpp
 * @brief Example 03: Buttons - Input Bindings with Fluent API
 *
 * This example introduces the OpenControlApp and the fluent InputBinding API.
 * You'll learn how to bind button events to actions using a clean, readable syntax.
 *
 * What you'll learn:
 * - OpenControlApp: the main application orchestrator
 * - IContext: application modes with lifecycle (initialize/update/cleanup)
 * - Fluent InputBinding API: onButton().press().then(...)
 * - Button events: press, release, longPress, doubleTap
 *
 * New concepts:
 * - Context: A mode of operation (standalone, DAW mode, config mode, etc.)
 * - AppBuilder: Fluent configuration of hardware drivers
 * - Requirements: Declare what APIs a context needs
 *
 * Hardware required:
 * - Teensy 4.1
 * - 2 buttons (normally open, pull-up)
 *   - Button 1: pin 32
 *   - Button 2: pin 35
 */

#include <Arduino.h>
#include <optional>

#include <oc/app/OpenControlApp.hpp>
#include <oc/context/IContext.hpp>
#include <oc/context/Requirements.hpp>
#include <oc/core/Result.hpp>
#include <oc/teensy/Teensy.hpp>

// ═══════════════════════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════════════════════

namespace Config {
    constexpr uint8_t MIDI_CHANNEL = 0;
    constexpr uint8_t BUTTON1_CC = 20;
    constexpr uint8_t BUTTON2_CC = 21;

    constexpr uint32_t LONG_PRESS_MS = 500;
    constexpr uint32_t DOUBLE_TAP_MS = 300;
    constexpr uint8_t DEBOUNCE_MS = 5;

    // Button hardware definitions
    constexpr std::array<oc::common::ButtonDef, 2> BUTTONS = {{
        oc::common::ButtonDef(1, oc::hal::GpioPin{32, oc::hal::GpioPin::Source::MCU}, true),
        oc::common::ButtonDef(2, oc::hal::GpioPin{35, oc::hal::GpioPin::Source::MCU}, true),
    }};
}

// ═══════════════════════════════════════════════════════════════════════════
// Context ID (user-defined)
// ═══════════════════════════════════════════════════════════════════════════

enum class ContextID : uint8_t { MAIN = 0 };

// ═══════════════════════════════════════════════════════════════════════════
// Main Context
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Application context demonstrating button bindings
 *
 * A Context is a mode of operation. It receives lifecycle callbacks
 * and has access to hardware APIs through protected accessors.
 */
class MainContext : public oc::context::IContext {
public:
    // Declare required APIs (validated at registration)
    static constexpr oc::context::Requirements REQUIRES{
        .button = true,
        .encoder = false,
        .midi = true
    };

    bool initialize() override {
        Serial.println("[Context] Initializing...");

        // ─────────────────────────────────────────────────────
        // Button 1: Press/Release (momentary)
        // ─────────────────────────────────────────────────────
        onButton(1).press().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 127);
            Serial.println("[Button 1] Press -> CC 127");
        });

        onButton(1).release().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 0);
            Serial.println("[Button 1] Release -> CC 0");
        });

        // Long press for alternative action
        onButton(1).longPress(Config::LONG_PRESS_MS).then([]() {
            Serial.println("[Button 1] Long press!");
        });

        // ─────────────────────────────────────────────────────
        // Button 2: Toggle behavior
        // ─────────────────────────────────────────────────────
        onButton(2).press().then([this]() {
            toggle_ = !toggle_;
            uint8_t value = toggle_ ? 127 : 0;
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON2_CC, value);
            Serial.printf("[Button 2] Toggle -> CC %d\n", value);
        });

        // Double tap for reset
        onButton(2).doubleTap(Config::DOUBLE_TAP_MS).then([this]() {
            toggle_ = false;
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON2_CC, 0);
            Serial.println("[Button 2] Double tap -> Reset");
        });

        Serial.println("[Context] Ready");
        return true;
    }

    void update() override {
        // Called every frame - nothing to do here
    }

    void cleanup() override {
        Serial.println("[Context] Cleanup");
    }

    const char* getName() const override { return "Main"; }

private:
    bool toggle_ = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// Global Application
// ═══════════════════════════════════════════════════════════════════════════

std::optional<oc::app::OpenControlApp> app;

// ═══════════════════════════════════════════════════════════════════════════
// Arduino Entry Points
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}

    Serial.println("\n[Example 03] Buttons");
    Serial.println("====================\n");

    // Build application with hardware drivers
    app = oc::teensy::AppBuilder()
        .midi()
        .buttons(Config::BUTTONS, Config::DEBOUNCE_MS)
        .inputConfig({
            .longPressMs = Config::LONG_PRESS_MS,
            .doubleTapWindowMs = Config::DOUBLE_TAP_MS
        });

    // Register context
    app->registerContext<MainContext>(ContextID::MAIN, "Main");

    // Start application
    if (auto r = app->begin(); !r) {
        Serial.printf("[ERROR] %s\n", oc::core::errorCodeToString(r.error().code));
        while (true);
    }

    Serial.println("\n[OK] Ready");
    Serial.println("Button 1: Press=CC127, Release=CC0, LongPress=debug");
    Serial.println("Button 2: Toggle CC, DoubleTap=Reset\n");
}

void loop() {
    app->update();
}
