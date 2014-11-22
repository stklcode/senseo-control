/**	SenseoControl 2.0
 *
 *  File:     main.h
 *  Author:   Stefan Kalscheuer
 *  Date:     22.04.2013
 *
 *  License:  GNU GPL v3 (see License.txt)
 */

/*******************
 * USER SETTINGS
 */
#define TIME_1_ESPRESSO       15    // pump times in seconds
#define TIME_2_ESPRESSO       28
#define TIME_1_COFFEE         26
#define TIME_2_COFFEE         52
#define OPERATING_TEMPERATURE 125   // ADC threshold for water temperature
/*
 *******************
 */

//	functions for setting and clearing bits
#define	set_bit(var, bit)	((var) |= (1 << (bit)))
#define	clear_bit(var, bit)	((var) &= (unsigned)~(1 << (bit)))

#define ZERO_CROSSING_w     PORTA   // zero crossing detection
#define ZERO_CROSSING_r     PINA
#define ZERO_CROSSING_pin   0
#define ZERO_CROSSING_ddr   DDRA
#define ZERO_CROSSING_adc   0

#define BUTTON_1_CUP_w      PORTB   // left button
#define BUTTON_1_CUP_r      PINB
#define BUTTON_1_CUP_pin    4
#define BUTTON_1_CUP_ddr    DDRB

#define BUTTON_2_CUP_w      PORTB   // right button
#define BUTTON_2_CUP_r      PINB
#define BUTTON_2_CUP_pin    5
#define BUTTON_2_CUP_ddr    DDRB

#define BUTTON_POWER_w      PORTB   // power button
#define BUTTON_POWER_r      PINB
#define BUTTON_POWER_pin    6
#define BUTTON_POWER_ddr    DDRB

#define LED_RED_w           PORTA   // red LED
#define LED_RED_r           PINA
#define LED_RED_pin         3
#define LED_RED_ddr         DDRA
#define LED_RED_ON          0
#define LED_RED_BLINK       1

#define LED_GREEN_w         PORTA   // green LED
#define LED_GREEN_r	        PINA
#define LED_GREEN_pin       1
#define LED_GREEN_ddr       DDRA
#define LED_GREEN_ON        2
#define LED_GREEN_BLINK     3

#define LED_BLUE_w          PORTA   // blue LED
#define LED_BLUE_r          PINA
#define LED_BLUE_pin        2
#define LED_BLUE_ddr        DDRA
#define LED_BLUE_ON         4
#define LED_BLUE_BLINK      5

#define SENSOR_MAGNET_w     PORTA   // hall switch (water)
#define SENSOR_MAGNET_r     PINA
#define SENSOR_MAGNET_pin   5
#define SENSOR_MAGNET_ddr   DDRA
#define SENSOR_MAGNET_adc   4

#define WATER_LOW           30      // ADC threshold for low water
#define	WATER_OK            100     // ADC threshold for water OK

#define SENSOR_TEMP_w       PORTA   // NTC (temperature)
#define SENSOR_TEMP_r       PINA
#define SENSOR_TEMP_pin     4
#define SENSOR_TEMP_ddr     DDRA
#define SENSOR_TEMP_adc     3

#define TRIAC_BOILER_w      PORTA   // boiler triac
#define TRIAC_BOILER_r      PINA
#define TRIAC_BOILER_pin    6
#define TRIAC_BOILER_ddr    DDRA

#define TRIAC_PUMP_w        PORTA   // pump triac
#define TRIAC_PUMP_r        PINA
#define TRIAC_PUMP_pin      7
#define TRIAC_PUMP_ddr      DDRA

#define	AUTO_OFF_THRESHOLD  180     // AutoOff threshold (seconds)
#define	BUTTON_CLEAN_THR    30      // button threshold for cleaning mode (ms)
#define BUTTON_THRESHOLD    100     // button threshold (ms)
#define BUTTON_LONG_THR     1500    // button threshold for long time push (ms)

                                      // prototypes:
void init ();                         //  initialization
void power_off ();                    //  power off to sleep mode
bool get_water ();                    //  update water state
bool get_temperature ();              //  update tehmerature state
unsigned int detect_zero_crossing (); //  detect zero crossing
