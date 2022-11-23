/*****************************************************************************
 *  SenseoControl 2.0                                                        *
 *  Copyright (C) 2013-2022  Stefan Kalscheuer                               *
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
#include "main.h"

// variables:
volatile unsigned int time_counter;                 // Global time counter (ms).
volatile unsigned int user_time_counter = 0;        // Universal time counter (ms).
volatile unsigned int sec_counter = 0;              // Second-counter (for AutoOff).
volatile unsigned int button_1_cup_counter = 0;     // Left button counter (1 cup).
volatile unsigned int button_2_cup_counter = 0;     // Left button counter (2 cups).
volatile unsigned char button_power_counter = 0;    // Power button counter.
volatile unsigned char led = 0;                     // LED status flags.
volatile unsigned char state;                       // Water-, temperature-, clean-flags.
volatile unsigned char make_coffee = NO_COFFEE;     // Coffee mode flag.
volatile unsigned char pump_time = 0;               // Pump time.

/**
 * Main program.
 *
 * @return This method should never terminate.
 */
int main(void) {
    init();                                                 // Initialization.
    power_off();                                            // Power off after init sequence.

    while (1) {                                             // Main loop.
        if (sec_counter >= AUTO_OFF_THRESHOLD) {
            button_power_counter = BUTTON_THRESHOLD;        // Check for AutoOff Timer (generate OnOff-button push).
        }

        update_water();                                     // Update water state.
        update_temperature();                               // Update temperature.

        if (button_power_counter >= BUTTON_THRESHOLD) {     // Button "OnOff" pushed:
            set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);      // Boiler off
            make_coffee = NO_COFFEE;                        // Clear coffee flag.

            while (button_power_counter > 0);               // Wait until button is released (debounce)

            power_off();                                    // Call power off sequence

            button_power_counter = BUTTON_THRESHOLD;        // Debounce again after wake up
            while (button_power_counter > 0);
        }

        if (button_1_cup_counter >= BUTTON_CLEAN_THR && button_2_cup_counter >= BUTTON_CLEAN_THR) {
            // Both coffee buttons pushed: enter clean mode.
            set_bit(state, S_CLEAN);    // Set clean flag.
            led = BLUE;                 // Set blue LED.
            while (button_1_cup_counter > 0 && button_2_cup_counter > 0);   // Debounce buttons.
        } else if (button_1_cup_counter >= BUTTON_THRESHOLD && button_2_cup_counter < BUTTON_THRESHOLD) {
            // Left coffee button pushed: call espresso.
            sec_counter = 0;    // Reset AutoOff counter.

            if ((state & S_WATER) && (state & S_TEMP)) {            // Machine ready:
                while (button_1_cup_counter > 0) {                  // Check if button is pushed long time.
                    if (button_1_cup_counter > BUTTON_LONG_THR) {   // Button pushed for a long time:
                        make_coffee = ONE_ESPRESSO;                   // Set coffee flag to 1 (1 espresso).
                        button_1_cup_counter = 0;                   // Clear button counter.
                    }
                }
                if (make_coffee != ONE_ESPRESSO) {
                    make_coffee = ONE_COFFEE;                       // Set coffee flag to 3 (1 coffee) otherwise.
                }
            } else if (COFFEE_WISH) {                               // Save coffee wish.
                make_coffee = ONE_COFFEE;
            }
        } else if (button_1_cup_counter < BUTTON_THRESHOLD && button_2_cup_counter >= BUTTON_THRESHOLD) {
            // Right coffee button pushed: call coffee.
            sec_counter = 0;                                        // Reset AutoOff counter.

            if ((state & S_WATER) && (state & S_TEMP)) {            // Machine ready:
                while (button_2_cup_counter > 0) {                  // Check if button is pushed long time.
                    if (button_2_cup_counter > BUTTON_LONG_THR) {   // Button pushed for a long time:
                        make_coffee = TWO_ESPRESSO;                 // Set coffee flag to 2 (2 espresso).
                        button_2_cup_counter = 0;                   // Clear button counter.
                    }
                }
                if (make_coffee != TWO_ESPRESSO) {
                    make_coffee = TWO_COFFEE;                       // Set coffee flag to 4 (2 coffee) otherwise.
                }
            } else if (COFFEE_WISH) {                               // Save coffee wish
                make_coffee = TWO_COFFEE;
            }
        }

        if (state & S_WATER) {                                      // Water OK:
            if (state & S_CLEAN) {                                  // If clean-flag is set:
                set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);          // Boiler off.
                clear_bit(state, S_ESC);                            // Init escape-flag.
                while ((state & S_WATER) && (state & S_ESC)) {      // Pump until water is empty or escape flag is set.
                    if (detect_zero_crossing() <= 100) {            // Detect zero crossing.
                        clear_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);    // Generate trigger impulse for pump triac.
                        _delay_ms(3);
                        set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);
                    }
                    update_water();                                 // Update water state.

                    if (button_power_counter > BUTTON_THRESHOLD) {
                        set_bit(state, S_ESC);                      // Check power button counter and set escape flag.
                    }
                }
                clear_bit(state, S_CLEAN);                          // Clear clean flag.
            } else if (state & S_TEMP) {                            // Temperature OK:
                set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);          // Boiler off.

                led = GREEN;                                        // Set green LED.

                if (make_coffee > NO_COFFEE) {                      // If coffee flag is set:
                    if (IS_ESPRESSO(make_coffee)) {
                        led = ORANGE_BLINK;                         // Set orange LED blink for espresso.
                    } else {
                        led = GREEN_BLINK;                          // Set green LED blink for coffee.
                    }

                    switch (make_coffee) {
                        case ONE_ESPRESSO:
                            pump_time = TIME_1_ESPRESSO;            // 1 cup of espresso (2s preinfusion included).
                            break;
                        case 2:
                            pump_time = TIME_2_ESPRESSO;            // 2 cups of espresso (2s preinfusion included).
                            break;
                        case ONE_COFFEE:
                            pump_time = TIME_1_COFFEE;              // 1 cup of coffee.
                            break;
                        case TWO_COFFEE:
                            pump_time = TIME_2_COFFEE;              // 2 cups of coffee.
                            break;
                        default:
                            make_coffee = NO_COFFEE;
                    }

                    user_time_counter = 0;      // Reset user time counter.
                    clear_bit(state, S_ESC);    // Init escape flag.

                    // loop until pump time is reached or water is empty
                    while (user_time_counter < (pump_time * 1000) && (state & S_WATER) && !(state & S_ESC)) {
                        // Check for preinfusion break.
                        if ((IS_COFFEE(make_coffee) || (user_time_counter < 2000 || user_time_counter > 4000)) &&
                            detect_zero_crossing() <= 100) {            // Detect zero crossing.
                            clear_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);    // Generate trigger impulse for pump triac.
                            _delay_ms(3);
                            set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);
                        }

                        update_water();             // Update water state.

                        if (button_power_counter > BUTTON_THRESHOLD) {
                            set_bit(state, S_ESC);  // Check for power button counter and set escape flag.
                        }
                    }

                    set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);      // Pump off
                    make_coffee = NO_COFFEE;                    // Clear coffee flag.
                    sec_counter = 0;                            // Reset AutoOff timer.
                }
            } else {    // Temperature too low.
                clear_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);    // Boiler on.
                if (make_coffee > NO_COFFEE) {                  // Set red/blue LED blink if coffee wish is saved.
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
    clear_bit(ZERO_CROSSING_ddr, ZERO_CROSSING_pin);    // Zero crossing detection pins as input.
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
void power_off(void) {
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
 */
void update_water(void) {
    ADMUX = SENSOR_MAGNET_adc | (1 << ADLAR);
    set_bit(ADCSR, ADSC);
    loop_until_bit_is_clear(ADCSR, ADSC);
    unsigned char sense = ADCH;
    if (((state & S_WATER) && sense > WATER_LOW) || (!(state & S_WATER) && sense >= WATER_OK)) {
        set_bit(state, S_WATER);
    } else {
        clear_bit(state, S_WATER);
    }
}

/**
 * Checks NTC sensor for temperature state.
 */
void update_temperature(void) {
    ADMUX = SENSOR_TEMP_adc | (1 << ADLAR);
    set_bit(ADCSR, ADSC);
    loop_until_bit_is_clear(ADCSR, ADSC);
    unsigned char sense = ADCH;
    if (sense >= OPERATING_TEMPERATURE) {
        set_bit(state, S_TEMP);
    } else {
        clear_bit(state, S_TEMP);
    }
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

    unsigned char leds_blink_on;         // Status flag for blinking LEDs with 1Hz.
    if (time_counter < 499) {
        leds_blink_on = 1;
    } else {
        leds_blink_on = 0;
    }

    if (led & (1 << LED_RED_ON) || (led & (1 << LED_RED_BLINK) && leds_blink_on)) {
        set_bit(LED_RED_w, LED_RED_pin);
    } else {
        clear_bit(LED_RED_w, LED_RED_pin);
    }
    if (led & (1 << LED_GREEN_ON) || (led & (1 << LED_GREEN_BLINK) && leds_blink_on)) {
        set_bit(LED_GREEN_w, LED_GREEN_pin);
    } else {
        clear_bit(LED_GREEN_w, LED_GREEN_pin);
    }
    if (led & (1 << LED_BLUE_ON) || (led & (1 << LED_BLUE_BLINK) && leds_blink_on)) {
        set_bit(LED_BLUE_w, LED_BLUE_pin);
    } else {
        clear_bit(LED_BLUE_w, LED_BLUE_pin);
    }

    if (bit_is_clear(BUTTON_1_CUP_r, BUTTON_1_CUP_pin)) {   // Left button counter.
        if (button_1_cup_counter < 65535) {
            button_1_cup_counter++;
        }
    } else {
        if (button_1_cup_counter > 0) {
            button_1_cup_counter--;
        }
    }

    if (bit_is_clear(BUTTON_2_CUP_r, BUTTON_2_CUP_pin)) {   // Right button counter.
        if (button_2_cup_counter < 65535) {
            button_2_cup_counter++;
        }
    } else {
        if (button_2_cup_counter > 0) {
            button_2_cup_counter--;
        }
    }

    if (bit_is_clear(BUTTON_POWER_r, BUTTON_POWER_pin)) {   // Power button counter.
        if (button_power_counter < 255) {
            button_power_counter++;
        }
    } else {
        if (button_power_counter > 0) {
            button_power_counter--;
        }
    }
}
