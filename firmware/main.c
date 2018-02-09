/*****************************************************************************
 *  SenseoControl 2.0                                                        *
 *  Copyright (C) 2013-2018  Stefan Kalscheuer                               *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation version 3.                                  *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.  *
 *****************************************************************************/

/**
 * SenseoControl 2.0
 *
 * @file   main.c
 * @author Stefan Kalscheuer
 * @date   2013-04-22
 * @brief  Main program
 *
 * Platform:  ATtiny26
 *            Internal RC-oscillator 8 MHz, CKDIV8 Enabled
 */

#define F_CPU    1000000UL

// includes
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include "main.h"

// variables:
volatile unsigned int time_counter, user_time_counter = 0, sec_counter = 0; // Global and universal time counter (ms) and second-counter (for AutoOff).
volatile unsigned int button_1_cup_counter = 0, button_2_cup_counter = 0;   // Button counter.
volatile unsigned char button_power_counter = 0;
volatile unsigned char led = 0;                                             // LED status flags.
volatile bool water = false, temperature = false, make_clean = false;       // Water-, temperature-, clean-flags.
volatile unsigned char make_coffee = 0, pump_time = 0;                      // Pump time, clean mode flag.

/**
 * Main program.
 *
 * @return This method should never terminate.
 */
int main(void) {
    init();                                                 // Initialization.
    power_off();                                            // Power off after init sequece.

    while (1) {                                             // Main loop.
        if (sec_counter >= AUTO_OFF_THRESHOLD)
            button_power_counter = BUTTON_THRESHOLD;        // Check for AutoOff Timer (generate OnOff-button push).

        water = get_water();                                // update water state
        temperature = get_temperature();                    // update temperature

        if (button_power_counter >= BUTTON_THRESHOLD) {     // button "OnOff" pushed:
            set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);      // Boiler off
            make_coffee = 0;                                // clear coffee flag

            while (button_power_counter >
                   0);                                      // wait until button is releasd (debounce)

            power_off();                                    // call power off sequence

            button_power_counter = BUTTON_THRESHOLD;        // debounce again after wake up
            while (button_power_counter > 0);
        }

        if (button_1_cup_counter >= BUTTON_CLEAN_THR && button_2_cup_counter >= BUTTON_CLEAN_THR) {
            // Both coffee buttons pushed: enter clean mode.
            make_clean = true;  // Set clean flag.
            led = BLUE;         // Set blue LED.
            while (button_1_cup_counter > 0 && button_2_cup_counter > 0);   // Debounce buttons.
        } else if (button_1_cup_counter >= BUTTON_THRESHOLD && button_2_cup_counter < BUTTON_THRESHOLD) {
            // Left coffee button pushed: call espresso.
            sec_counter = 0;    // Reset AutoOff counter.

            if (water && temperature) {                             // Machine ready:
                while (button_1_cup_counter > 0) {                  // Check if button is pushed long time.
                    if (button_1_cup_counter > BUTTON_LONG_THR) {   // Button pushed for a long time:
                        make_coffee = 1;                            // Set coffee flag to 1 (1 espresso).
                        button_1_cup_counter = 0;                   // Clear button counter.
                    }
                }
                if (make_coffee != 1) {
                    make_coffee = 3;                                // Set coffee flag to 3 (1 coffee) otherwise.
                }
            } else if (COFFEE_WISH) {                               // Save coffee wish.
                make_coffee = 3;
            }
        } else if (button_1_cup_counter < BUTTON_THRESHOLD && button_2_cup_counter >= BUTTON_THRESHOLD) {
            // Right coffee button pushed: call coffee.
            sec_counter = 0;                                        // Reset AutoOff counter.

            if (water && temperature) { // machine ready:
                while (button_2_cup_counter > 0) {                  // Check if button is pushed long time.
                    if (button_2_cup_counter > BUTTON_LONG_THR) {   // Button pushed for a long time:
                        make_coffee = 2;                            // Set coffee flag to 2 (2 espresso).
                        button_2_cup_counter = 0;                   // Clear button counter.
                    }
                }
                if (make_coffee != 2) {
                    make_coffee = 4;                                // Set coffee flag to 4 (2 coffee) otherwise.
                }
            } else if (COFFEE_WISH) {                               // Save coffee wish
                make_coffee = 4;
            }
        }

        if (water) {                                                // Water OK:
            if (make_clean) {                                       // If clean-flag is set:
                set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);          // Boiler off.
                bool escape = false;                                // Init escape-flag.
                while (water && !escape) {                          // Pump until water is empty or escape flag is set.
                    unsigned int sense = detect_zero_crossing();    // Detect zero crossing.
                    if (sense <= 100) {
                        clear_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);    // Generate trigger impulse for pump triac.
                        _delay_ms(3);
                        set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);
                    }
                    water = get_water();                            // Update water state.

                    if (button_power_counter > BUTTON_THRESHOLD)
                        escape = true;                              // Check power button counter and set escape flag.
                }
                make_clean = false;                                 // Clear clean flag.
            } else if (temperature) {                               // Temperature OK:
                set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);          // Boiler off.

                led = GREEN;                                        // Set green LED.

                if (make_coffee > 0) {                              // If coffee flag is set:
                    if (make_coffee < 3) {
                        led = ORANGE_BLINK;                         // Set orange LED blink for espresso.
                    } else {
                        led = GREEN_BLINK;                          // Set green LED blink for coffee.
                    }

                    if (make_coffee == 1) {
                        pump_time = TIME_1_ESPRESSO;                // 1 cup of espresso (2s preinfusion included).
                    } else if (make_coffee == 2) {
                        pump_time = TIME_2_ESPRESSO;                // 2 cups of espresso (2s preinfusion included).
                    } else if (make_coffee == 3) {
                        pump_time = TIME_1_COFFEE;                  // 1 cup of coffee.
                    } else if (make_coffee == 4) {
                        pump_time = TIME_2_COFFEE;                  // 2 cups of coffee.
                    } else {
                        make_coffee = 0;
                    }

                    user_time_counter = 0;      // Reset user time counter.
                    bool escape = false;        // Init escape flag.

                    // loop until pump time is reached or water is empty
                    while (user_time_counter < (pump_time * 1000) && water && !escape) {
                        // Check for preinfusion break.
                        if (make_coffee > 2 || (user_time_counter < 2000 || user_time_counter > 4000)) {
                            unsigned int sense = detect_zero_crossing();    // Detect zero crossing.
                            if (sense <= 100) {
                                clear_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);    // Generate trigger impulse for pump triac.
                                _delay_ms(3);
                                set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);
                            }
                        }

                        water = get_water();    // Update water state.

                        if (button_power_counter > BUTTON_THRESHOLD) {
                            escape = true;      // Check for power button counter and set escape flag.
                        }
                    }

                    set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);      // Pump off
                    make_coffee = 0;                            // Clear coffee flag.
                    sec_counter = 0;                            // Reset AutoOff timer.
                }
            } else {    // Temperature too low.
                clear_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);    // Boiler on.
                if (make_coffee > 0) {                          // Set red/blue LED blink if coffee wish is saved.
                    led = VIOLET_BLINK;
                } else {                                        // Set red LED blink if no coffee wish is saved.
                    led = RED_BLINK;
                }
            }
        } else {    // Water too low:
            set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);      // Boiler off.
            set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);          // Pump off.
            led = BLUE_BLINK;                               // Set blue LED blink.
        }
    }
}

/**
 * Initializes relevant bits, timer and ADC.
 */
void init() {
    clear_bit(ZERO_CROSSING_ddr, ZERO_CROSSING_pin);    // Zero crossing dection pins as input-
    clear_bit(ZERO_CROSSING_w, ZERO_CROSSING_pin);      // No internal pull-up (for ADC).

    clear_bit(BUTTON_1_CUP_ddr, BUTTON_1_CUP_pin);      // Button pins as input.
    set_bit(BUTTON_1_CUP_w, BUTTON_1_CUP_pin);          // Activate internal pull-ups.
    clear_bit(BUTTON_2_CUP_ddr, BUTTON_2_CUP_pin);
    set_bit(BUTTON_2_CUP_w, BUTTON_2_CUP_pin);
    clear_bit(BUTTON_POWER_ddr, BUTTON_POWER_pin);
    set_bit(BUTTON_POWER_w, BUTTON_POWER_pin);

    set_bit(LED_RED_ddr, LED_RED_pin);                  // LED pins as output.
    clear_bit(LED_RED_w, LED_RED_pin);                  // Clear outputs (LEDs off).
    set_bit(LED_GREEN_ddr, LED_GREEN_pin);
    clear_bit(LED_GREEN_w, LED_GREEN_pin);
    set_bit(LED_BLUE_ddr, LED_BLUE_pin);
    clear_bit(LED_BLUE_w, LED_BLUE_pin);

    clear_bit(SENSOR_MAGNET_ddr, SENSOR_MAGNET_pin);    // Sensor pins as input.
    clear_bit(SENSOR_MAGNET_w, SENSOR_MAGNET_pin);      // No internal pull-up (for ADC).
    clear_bit(SENSOR_TEMP_ddr, SENSOR_TEMP_pin);
    clear_bit(SENSOR_TEMP_w, SENSOR_TEMP_pin);

    set_bit(TRIAC_BOILER_ddr, TRIAC_BOILER_pin);        // Triac pins as output.
    set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);          // Set outputs high (triac off).
    set_bit(TRIAC_PUMP_ddr, TRIAC_PUMP_pin);
    set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);

    ADCSR = (1 << ADEN) | (1 << ADPS1);                 // Enable ADC, prescaler division factor 4.

    // TIMER1
    set_bit(TCCR1B, CTC1);                              // Set timer 1 to CTC-Mode.
    clear_bit(TCCR1B, CS11);                            // Prescaler 8.
    set_bit(TCCR1B, CS12);
    clear_bit(TCCR1B, CS11);
    clear_bit(TCCR1B, CS10);
    OCR1C = 124;                                        // Period of 1 ms.

    cli();                                              // Disable interrupts.
    clear_bit(GIMSK, INT0);                             // Disable interrupt 0.
    set_bit(TIMSK, TOIE1);                              // Activate timer 1.
    sei();                                              // Enable interrupts.
}

/**
 * Clear bits and set controller to sleep mode.
 */
void power_off() {
    cli();                                  // Disable interrupts.
    set_bit(GIMSK, INT0);                   // Activate interrupt 0 (for wake-up).
    clear_bit(TIMSK, TOIE1);                // Deactivate timer 1.
    sei();                                  // Re-enable interrupts.

    clear_bit(LED_RED_w, LED_RED_pin);      // Clear LED outputs.
    clear_bit(LED_GREEN_w, LED_GREEN_pin);
    clear_bit(LED_BLUE_w, LED_BLUE_pin);

    set_bit(MCUCR, SM1);                    // Activate power-down mode.
    clear_bit(MCUCR, SM0);
    set_bit(MCUCR, SE);
    asm volatile("sleep"::);

    // Entrance point after wake-up.
    time_counter = 0;                       // Reset counter.
    sec_counter = 0;
    cli();                                  // Disable interrupts.
    clear_bit(GIMSK, INT0);                 // Disable interrupt 0.
    set_bit(TIMSK, TOIE1);                  // Enable timer 1.
    sei();                                  // Re-enable interrupts.
}

/**
 * Checks hall sensor for water level.
 *
 * @return @c true if water level is OK, @c false otherwise.
 */
bool get_water() {
    ADMUX = SENSOR_MAGNET_adc | (1 << ADLAR);
    set_bit(ADCSR, ADSC);
    loop_until_bit_is_clear(ADCSR, ADSC);
    unsigned char sense = ADCH;
    if ((water && sense > WATER_LOW) || (!water && sense >= WATER_OK))
        return true;
    return false;
}

/**
 * Checks NTC sensor for temperature state.
 *
 * @return @c true if temperature is OK, @c false if it is too low
 */
bool get_temperature() {
    ADMUX = SENSOR_TEMP_adc | (1 << ADLAR);
    set_bit(ADCSR, ADSC);
    loop_until_bit_is_clear(ADCSR, ADSC);
    unsigned char sense = ADCH;
    if (sense >= OPERATING_TEMPERATURE)
        return true;
    return false;
}

/**
 * Checks for zero crossing (with fixed offset)
 *
 * @return Raw ADC value.
 */
unsigned int detect_zero_crossing() {
    ADMUX = ZERO_CROSSING_adc;
    set_bit(ADCSR, ADSC);
    loop_until_bit_is_clear(ADCSR, ADSC);
    unsigned char sense_L = ADCL;
    unsigned char sense_H = ADCH;
    return (sense_H << 8) | sense_L;
}

/**
 * Dummy function for wake-up.
 */
ISR ( INT0_vect) {
    // Nothing to do here.
}

/**
 * Timer interrupt. Increments counters and controls LED.
 */
ISR ( TIMER1_OVF1_vect) {
    if (time_counter < 1000)
        time_counter++;         // Global milliseconds counter and seconds counter (for AutoOff).
    else {
        time_counter = 0;
        sec_counter++;
    }
    user_time_counter++;        // Universal counter (for pump time).

    bool leds_blink_on;         // Status flag for blinking LEDs with 1Hz.
    if (time_counter < 499)
        leds_blink_on = true;
    else
        leds_blink_on = false;

    if (led & (1 << LED_RED_ON) || (led & (1 << LED_RED_BLINK) && leds_blink_on))
        set_bit(LED_RED_w, LED_RED_pin);
    else
        clear_bit(LED_RED_w, LED_RED_pin);
    if (led & (1 << LED_GREEN_ON)
        || (led & (1 << LED_GREEN_BLINK) && leds_blink_on))
        set_bit(LED_GREEN_w, LED_GREEN_pin);
    else
        clear_bit(LED_GREEN_w, LED_GREEN_pin);
    if (led & (1 << LED_BLUE_ON)
        || (led & (1 << LED_BLUE_BLINK) && leds_blink_on))
        set_bit(LED_BLUE_w, LED_BLUE_pin);
    else
        clear_bit(LED_BLUE_w, LED_BLUE_pin);

    if (bit_is_clear(BUTTON_1_CUP_r, BUTTON_1_CUP_pin)) {   // Left button counter.
        if (button_1_cup_counter < 65535)
            button_1_cup_counter++;
    } else {
        if (button_1_cup_counter > 0)
            button_1_cup_counter--;
    }

    if (bit_is_clear(BUTTON_2_CUP_r, BUTTON_2_CUP_pin)) {   // Right button counter.
        if (button_2_cup_counter < 65535)
            button_2_cup_counter++;
    } else {
        if (button_2_cup_counter > 0)
            button_2_cup_counter--;
    }

    if (bit_is_clear(BUTTON_POWER_r, BUTTON_POWER_pin)) {   // Power button counter.
        if (button_power_counter < 255)
            button_power_counter++;
    } else {
        if (button_power_counter > 0)
            button_power_counter--;
    }
}
