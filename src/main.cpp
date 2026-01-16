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
 * - Using OC_LOG_* for debug output
 *
 * New concepts:
 * - Context: A mode of operation (standalone, DAW mode, config mode, etc.)
 * - AppBuilder: Fluent configuration of hardware drivers
 * - Requirements: Declare what APIs a context needs
 *
 * Hardware required:
 * - Teensy 4.1
 * - 2 buttons (normally open, pull-up)
 *
 * NOTE: Enable -D OC_LOG in platformio.ini build_flags to see debug output.
 *       Remove it for production (zero overhead, instant boot).
 */

#include <optional>

#include <oc/hal/teensy/Teensy.hpp>
#include <oc/app/OpenControlApp.hpp>
#include <oc/context/IContext.hpp>
#include <oc/context/Requirements.hpp>

// ═══════════════════════════════════════════════════════════════════════════
// Configuration - Adapt to your hardware
// ═══════════════════════════════════════════════════════════════════════════

namespace Config {
    constexpr uint8_t MIDI_CHANNEL = 0;
    constexpr uint8_t BUTTON1_CC = 20;
    constexpr uint8_t BUTTON2_CC = 21;

    constexpr uint32_t LONG_PRESS_MS = 500;
    constexpr uint32_t DOUBLE_TAP_MS = 300;
    constexpr uint8_t DEBOUNCE_MS = 5;

    // Button hardware definitions - ADAPT pins to your wiring
    constexpr std::array<oc::hal::common::ButtonDef, 2> BUTTONS = {{
        oc::hal::common::ButtonDef(1, oc::hal::GpioPin{32, oc::hal::GpioPin::Source::MCU}, true),  // ADAPT: pin 32
        oc::hal::common::ButtonDef(2, oc::hal::GpioPin{35, oc::hal::GpioPin::Source::MCU}, true),  // ADAPT: pin 35
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
        // Button 1: Press/Release (momentary)
        onButton(1).press().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 127);
            OC_LOG_DEBUG("Button 1: Press -> CC 127");
        });

        onButton(1).release().then([this]() {
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON1_CC, 0);
            OC_LOG_DEBUG("Button 1: Release -> CC 0");
        });

        onButton(1).longPress(Config::LONG_PRESS_MS).then([]() {
            OC_LOG_DEBUG("Button 1: Long press!");
        });

        // Button 2: Toggle behavior
        onButton(2).press().then([this]() {
            toggle_ = !toggle_;
            uint8_t value = toggle_ ? 127 : 0;
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON2_CC, value);
            OC_LOG_DEBUG("Button 2: Toggle -> CC {}", value);
        });

        onButton(2).doubleTap(Config::DOUBLE_TAP_MS).then([this]() {
            toggle_ = false;
            midi().sendCC(Config::MIDI_CHANNEL, Config::BUTTON2_CC, 0);
            OC_LOG_DEBUG("Button 2: Double tap -> Reset");
        });

        return true;
    }

    void update() override {}
    void cleanup() override {}

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
    OC_LOG_INFO("Example 03: Buttons");

    app = oc::hal::teensy::AppBuilder()
        .midi()
        .buttons(Config::BUTTONS, Config::DEBOUNCE_MS)
        .inputConfig({
            .longPressMs = Config::LONG_PRESS_MS,
            .doubleTapWindowMs = Config::DOUBLE_TAP_MS
        });

    app->registerContext<MainContext>(ContextID::MAIN, "Main");
    app->begin();

    OC_LOG_INFO("Ready");
}

void loop() {
    app->update();
}
