 /*
 * This file is part of the Pico Keys SDK distribution (https://github.com/polhenarejos/pico-keys-sdk).
 * Copyright (c) 2022 Pol Henarejos.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "picokeys.h"
#include "led/led.h"
#include "pico_time.h"
#if defined(ESP_PLATFORM)
#include "driver/gpio.h"
#elif defined(ENABLE_EMULATION)
#include "emulation.h"
#endif

led_driver_t *led_driver = NULL;

static uint32_t led_mode = MODE_NOT_MOUNTED;
static bool led_mode_changed = true;
static bool pin_hint_active = false;
static uint32_t pin_hint_started = 0;

void led_pin_hint_begin(void) {
    __atomic_store_n(&pin_hint_started, board_millis(), __ATOMIC_SEQ_CST);
    __atomic_store_n(&pin_hint_active, true, __ATOMIC_SEQ_CST);
}

void led_pin_hint_cancel(void) {
    __atomic_store_n(&pin_hint_active, false, __ATOMIC_SEQ_CST);
}

static volatile bool blink_pending = false;
static volatile uint8_t blink_count = 0;
static volatile uint8_t blink_color = LED_COLOR_GREEN;
static volatile uint32_t blink_on_ms = 0;
static volatile uint32_t blink_off_ms = 0;

void led_set_mode(uint32_t mode) {
    if (mode == MODE_BUTTON) {
        // Physical-presence requests always take priority over the PIN hint.
        led_pin_hint_cancel();
    }
    if (led_mode != mode) {
        led_mode_changed = true;
    }
    led_mode = mode;
}

uint32_t led_get_mode(void) {
    return led_mode;
}

void led_blink_n_times(uint8_t count, uint8_t color, uint32_t on_ms, uint32_t off_ms) {
    if (count == 0 || on_ms == 0 || off_ms == 0) {
        return;
    }
    blink_count = count;
    blink_color = color;
    blink_on_ms = on_ms;
    blink_off_ms = off_ms;
    blink_pending = true;
}

void led_blinking_task(void) {
#if defined(PICO_PLATFORM) || defined(ESP_PLATFORM)
    static uint32_t start_ms = 0;
    static uint32_t stop_ms = 0;
    static uint32_t last_led_update_ms = 0;
    static uint8_t led_state = false;
    static bool blink_active = false;
    static bool blink_on = false;
    static uint8_t blinks_remaining = 0;
    static uint8_t active_blink_color = LED_COLOR_GREEN;
    static uint32_t active_blink_on_ms = 0;
    static uint32_t active_blink_off_ms = 0;
    static uint32_t blink_deadline_ms = 0;
    static uint32_t displayed_mode = UINT32_MAX;

    uint32_t now = board_millis();
    bool pin_hint = __atomic_load_n(&pin_hint_active, __ATOMIC_SEQ_CST);
    if (pin_hint && (uint32_t)(now - __atomic_load_n(&pin_hint_started, __ATOMIC_SEQ_CST)) >= LED_PIN_HINT_TIMEOUT_MS) {
        led_pin_hint_cancel();
        pin_hint = false;
    }
    uint32_t display_mode = pin_hint && led_mode != MODE_BUTTON ? MODE_PIN_HINT : led_mode;
    if (blink_pending) {
        blink_pending = false;
        blink_active = true;
        blink_on = true;
        blinks_remaining = blink_count;
        active_blink_color = blink_color;
        active_blink_on_ms = blink_on_ms;
        active_blink_off_ms = blink_off_ms;
        blink_deadline_ms = now + active_blink_on_ms;
        led_driver->set_color(active_blink_color, MAX_BTNESS, 1.0f);
        return;
    }
    if (blink_active) {
        if (now < blink_deadline_ms) {
            return;
        }
        if (blink_on) {
            blink_on = false;
            blink_deadline_ms = now + active_blink_off_ms;
            led_driver->set_color(LED_COLOR_OFF, 0, 0.0f);
            return;
        }
        if (--blinks_remaining == 0) {
            blink_active = false;
        }
        else {
            blink_on = true;
            blink_deadline_ms = now + active_blink_on_ms;
            led_driver->set_color(active_blink_color, MAX_BTNESS, 1.0f);
            return;
        }
    }
    uint8_t state = led_state;
#ifdef PICO_DEFAULT_LED_PIN_INVERTED
    state = !state;
#endif
    uint32_t led_brightness = (display_mode & LED_BTNESS_MASK) >> LED_BTNESS_SHIFT;
    uint32_t led_color = (display_mode & LED_COLOR_MASK) >> LED_COLOR_SHIFT;
    uint32_t led_off = (display_mode & LED_OFF_MASK) >> LED_OFF_SHIFT;
    uint32_t led_on = (display_mode & LED_ON_MASK) >> LED_ON_SHIFT;

    bool slow_status = display_mode == MODE_NOT_MOUNTED ||
                       display_mode == MODE_MOUNTED || display_mode == MODE_BUTTON ||
                       display_mode == MODE_PIN_HINT;
    if (slow_status) {
        led_on = (display_mode == MODE_PIN_HINT || display_mode == MODE_BUTTON) ?
                 LED_SLOW_ON_MS : LED_IDLE_ON_MS;
        led_off = display_mode == MODE_BUTTON ? LED_BUTTON_OFF_MS :
                  display_mode == MODE_PIN_HINT ? LED_PIN_OFF_MS : LED_IDLE_OFF_MS;
    }
    if (led_mode_changed || displayed_mode != display_mode) {
        // Show a changed state immediately, rather than waiting out an idle
        // idle dark interval before displaying the confirmation request.
        start_ms = now;
        stop_ms = now + led_on;
        led_state = true;
        state = true;
        led_mode_changed = false;
        displayed_mode = display_mode;
        led_driver->set_color(led_color, led_brightness, 1.0f);
        last_led_update_ms = now;
    }

    float progress = 0;

    if (stop_ms > start_ms) {
        progress = (float)(now - start_ms) / (stop_ms - start_ms);
    }

    if (!state) {
        progress = 1. - progress;
    }
    if (phy_data.opts & PHY_OPT_LED_STEADY) {
        progress = 1;
    }

    if (slow_status) {
        // Keep a real 500-ms pulse; do not shorten it through the dimming ramp.
        progress = state ? 1.0f : 0.0f;
    }

    // limit the frequency of LED status updates
    if (now - last_led_update_ms > 2) {
        led_driver->set_color(led_color, led_brightness, progress);
        last_led_update_ms = now;
    }

    if (now >= stop_ms){
        start_ms = stop_ms;
        led_state ^= 1; // toggle
        stop_ms = start_ms + (led_state ? led_on : led_off);
    }
#endif
}

void led_off_all(void) {
#if defined(PICO_PLATFORM) || defined(ESP_PLATFORM)
    led_driver->set_color(LED_COLOR_OFF, 0, 0);
#endif
}

extern led_driver_t led_driver_pico;
extern led_driver_t led_driver_cyw43;
extern led_driver_t led_driver_ws2812;
extern led_driver_t led_driver_neopixel;
extern led_driver_t led_driver_pimoroni;

static void led_driver_init_dummy(void) {
    // Do nothing
}

static void led_driver_color_dummy(uint8_t color, uint32_t led_brightness, float progress) {
    (void)color;
    (void)led_brightness;
    (void)progress;
    // Do nothing
}

led_driver_t led_driver_dummy = {
    .init = led_driver_init_dummy,
    .set_color = led_driver_color_dummy,
};

void led_init(void) {
    led_driver = &led_driver_dummy;
#if defined(PICO_PLATFORM) || defined(ESP_PLATFORM)
    // Guess default driver
#if defined(PIMORONI_TINY2040) || defined(PIMORONI_TINY2350)
    led_driver = &led_driver_pimoroni;
    phy_data.led_driver = phy_data.led_driver_present ? phy_data.led_driver : PHY_LED_DRIVER_PIMORONI;
    phy_data.led_gpio = phy_data.led_gpio_present ? phy_data.led_gpio : PICO_DEFAULT_LED_PIN;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    led_driver = &led_driver_cyw43;
    phy_data.led_driver = phy_data.led_driver_present ? phy_data.led_driver : PHY_LED_DRIVER_CYW43;
    phy_data.led_gpio = phy_data.led_gpio_present ? phy_data.led_gpio : CYW43_WL_GPIO_LED_PIN;
#elif defined(PICO_DEFAULT_WS2812_PIN)
    led_driver = &led_driver_ws2812;
    phy_data.led_driver = phy_data.led_driver_present ? phy_data.led_driver : PHY_LED_DRIVER_WS2812;
    phy_data.led_gpio = phy_data.led_gpio_present ? phy_data.led_gpio : PICO_DEFAULT_WS2812_PIN;
#elif defined(ESP_PLATFORM)
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
        #define NEOPIXEL_PIN GPIO_NUM_48
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
        #define NEOPIXEL_PIN GPIO_NUM_15
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        #define NEOPIXEL_PIN GPIO_NUM_8
    #else
        #define NEOPIXEL_PIN GPIO_NUM_27
    #endif
    led_driver = &led_driver_neopixel;
    phy_data.led_driver = phy_data.led_driver_present ? phy_data.led_driver : PHY_LED_DRIVER_NEOPIXEL;
    phy_data.led_gpio = phy_data.led_gpio_present ? phy_data.led_gpio : NEOPIXEL_PIN;
#elif defined(PICO_DEFAULT_LED_PIN)
    led_driver = &led_driver_pico;
    phy_data.led_driver = phy_data.led_driver_present ? phy_data.led_driver : PHY_LED_DRIVER_PICO;
    phy_data.led_gpio = phy_data.led_gpio_present ? phy_data.led_gpio : PICO_DEFAULT_LED_PIN;
#endif
    if (phy_data.led_driver_present) {
        switch (phy_data.led_driver) {
            case PHY_LED_DRIVER_PICO:
                led_driver = &led_driver_pico;
                break;
#ifdef ESP_PLATFORM
            case PHY_LED_DRIVER_NEOPIXEL:
                led_driver = &led_driver_neopixel;
                break;
#else
#ifdef CYW43_WL_GPIO_LED_PIN
            case PHY_LED_DRIVER_CYW43:
                led_driver = &led_driver_cyw43;
                break;
#endif
            case PHY_LED_DRIVER_WS2812:
                led_driver = &led_driver_ws2812;
                break;
            case PHY_LED_DRIVER_PIMORONI:
                led_driver = &led_driver_pimoroni;
                break;
#endif
            default:
                break;
        }
    }
    phy_data.led_driver_present = true;
    phy_data.led_gpio_present = true;
    led_driver->init();
    led_set_mode(MODE_NOT_MOUNTED);
#endif
}
