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
 * @file   main.h
 * @author Stefan Kalscheuer
 * @date   2013-04-22
 */

/********************
 * User settings:
 */
#define TIME_1_ESPRESSO       15    // Pump times in seconds.
#define TIME_2_ESPRESSO       28
#define TIME_1_COFFEE         26
#define TIME_2_COFFEE         52
#define OPERATING_TEMPERATURE 125   // ADC threshold for water temperature.
#define COFFEE_WISH           0     // Save coffee wish while heating up.
/*
 ********************/

// Function macros for setting and clearing bits.
#define set_bit(var, bit)   ((var) |= (1 << (bit)))
#define clear_bit(var, bit) ((var) &= (unsigned)~(1 << (bit)))

#define ZERO_CROSSING_w     PORTA   // Zero crossing detection.
#define ZERO_CROSSING_r     PINA
#define ZERO_CROSSING_pin   0
#define ZERO_CROSSING_ddr   DDRA
#define ZERO_CROSSING_adc   0

#define BUTTON_1_CUP_w      PORTB   // Left button.
#define BUTTON_1_CUP_r      PINB
#define BUTTON_1_CUP_pin    4
#define BUTTON_1_CUP_ddr    DDRB

#define BUTTON_2_CUP_w      PORTB   // Right button.
#define BUTTON_2_CUP_r      PINB
#define BUTTON_2_CUP_pin    5
#define BUTTON_2_CUP_ddr    DDRB

#define BUTTON_POWER_w      PORTB   // Power button.
#define BUTTON_POWER_r      PINB
#define BUTTON_POWER_pin    6
#define BUTTON_POWER_ddr    DDRB

#define LED_RED_w           PORTA   // Red LED.
#define LED_RED_r           PINA
#define LED_RED_pin         3
#define LED_RED_ddr         DDRA
#define LED_RED_ON          0
#define LED_RED_BLINK       1

#define LED_GREEN_w         PORTA   // Green LED.
#define LED_GREEN_r         PINA
#define LED_GREEN_pin       1
#define LED_GREEN_ddr       DDRA
#define LED_GREEN_ON        2
#define LED_GREEN_BLINK     3

#define LED_BLUE_w          PORTA   // Blue LED.
#define LED_BLUE_r          PINA
#define LED_BLUE_pin        2
#define LED_BLUE_ddr        DDRA
#define LED_BLUE_ON         4
#define LED_BLUE_BLINK      5

#define SENSOR_MAGNET_w     PORTA   // Hall switch (water).
#define SENSOR_MAGNET_r     PINA
#define SENSOR_MAGNET_pin   5
#define SENSOR_MAGNET_ddr   DDRA
#define SENSOR_MAGNET_adc   4

#define WATER_LOW           30      // ADC threshold for low water.
#define WATER_OK            100     // ADC threshold for water OK.

#define SENSOR_TEMP_w       PORTA   // NTC (temperature)
#define SENSOR_TEMP_r       PINA
#define SENSOR_TEMP_pin     4
#define SENSOR_TEMP_ddr     DDRA
#define SENSOR_TEMP_adc     3

#define TRIAC_BOILER_w      PORTA   // Boiler triac.
#define TRIAC_BOILER_r      PINA
#define TRIAC_BOILER_pin    6
#define TRIAC_BOILER_ddr    DDRA

#define TRIAC_PUMP_w        PORTA   // Pump triac.
#define TRIAC_PUMP_r        PINA
#define TRIAC_PUMP_pin      7
#define TRIAC_PUMP_ddr      DDRA

#define AUTO_OFF_THRESHOLD  180     // AutoOff threshold (seconds).
#define BUTTON_CLEAN_THR    30      // Button threshold for cleaning mode (ms).
#define BUTTON_THRESHOLD    100     // Button threshold (ms).
#define BUTTON_LONG_THR     1500    // Button threshold for long time push (ms).

// Global state flags.
#define S_WATER             0
#define S_TEMP              1
#define S_CLEAN             2
#define S_ESC               3

// Coffee mode flags.
#define NO_COFFEE     0
#define ONE_ESPRESSO  1
#define TWO_ESPRESSO  2
#define ONE_COFFEE    3
#define TWO_COFFEE    4
#define IS_COFFEE(VAR) (VAR > 2)
#define IS_ESPRESSO(VAR) (VAR > 0 && VAR < 3)

// LED color flags.
#define RED                 0b00000001
#define RED_BLINK           0b00000010
#define GREEN               0b00000100
#define GREEN_BLINK         0b00001000
#define BLUE                0b00010000
#define BLUE_BLINK          0b00100000
#define ORANGE              0b00000101
#define ORANGE_BLINK        0b00001010
#define VIOLET              0b00010001
#define VIOLET_BLINK        0b00100010

// Prototypes:
void init(void);                            //  Initialization.
void power_off(void);                       //  Power off to sleep mode.
void update_water(void);                    //  Update water state.
void update_temperature(void);              //  Update temperature state.
unsigned int detect_zero_crossing(void);    //  Detect zero crossing.
