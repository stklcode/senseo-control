/**	SenseoControl 2.0
 *
 *	File:     main.c
 *	Author:   Stefan Kalscheuer
 *	Date:     22.04.2013
 *	Comments: Main program
 *            Previous project by Paul Wilhelm (2009) - http://mosfetkiller.de/?s=kaffeecontroller
 *
 * Platform:  ATtiny26
 *            Internal RC-oscillator 8 MHz, CKDIV8 Enabled
 *
 * License:   GNU GPL v3 (see License.txt)
 */

#define F_CPU	1000000UL

//	includes
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdbool.h>
#include "main.h"

                                                                            //	variables:
volatile unsigned int time_counter, user_time_counter = 0, sec_counter = 0; //	global and universal time counter (ms) and second-counter (for AutoOff)
volatile unsigned int button_1_cup_counter = 0, button_2_cup_counter = 0;   //	button counter
volatile unsigned char button_power_counter = 0;
volatile unsigned char led = 0;                                             //	LED status flags
volatile bool water = false, temperature = false, make_clean = false;       //	water-, temperature-, clean-flags
volatile unsigned char make_coffee = 0, pump_time = 0;                      //	pump time, clean mode flag

int main (void)
{
  init ();                                              //	initialization
  power_off ();                                         //	power off after init sequece

  while (1)                                             //	main loop
  {
    if (sec_counter >= AUTO_OFF_THRESHOLD)
      button_power_counter = BUTTON_THRESHOLD;          //	check for AutoOff Timer (generate OnOff-button push)

    water = get_water ();                               //	update water state
    temperature = get_temperature ();                   //	update temperature

    if (button_power_counter >= BUTTON_THRESHOLD)       //	button "OnOff" pushed:
    {
      set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);        //	Boiler off
      make_coffee = 0;                                  //	clear coffee flag

      while (button_power_counter > 0)
        ;                                               //	wait until button is releasd (debounce)

      power_off ();                                     //	call power off sequence

      button_power_counter = BUTTON_THRESHOLD;          //	debounce again after wake up
      while (button_power_counter > 0)
        ;
    }

    if (button_1_cup_counter >= BUTTON_CLEAN_THR
        && button_2_cup_counter >= BUTTON_CLEAN_THR)    //	both coffee buttons pushed: clean mode:
    {
      make_clean = true;                                //	clean flag true
      led = 0b00010000;                                 //	set blue LED
      while (button_1_cup_counter > 0 && button_2_cup_counter > 0)
        ;                                               //	debounce buttons
    }

    else if (button_1_cup_counter >= BUTTON_THRESHOLD
        && button_2_cup_counter < BUTTON_THRESHOLD)     //	left coffee button pushed: call espresso
    {
      sec_counter = 0;                                  //	reset AutoOff counter

      if (water && temperature)                         //	machine ready:
      {
        while (button_1_cup_counter > 0)                //	check if button is pushed long time
        {
          if (button_1_cup_counter > BUTTON_LONG_THR)   //	button pushed for a long time:
          {
            make_coffee = 1;                            //	set coffee flag to 1 (1 espresso)
            button_1_cup_counter = 0;                   //	clear button counter
          }
        }
        if (make_coffee != 1)
          make_coffee = 3;                              //	set coffee flag to 3 (1 coffee) else
      }
    }

    else if (button_1_cup_counter < BUTTON_THRESHOLD
        && button_2_cup_counter >= BUTTON_THRESHOLD)    //	right coffee button pushed: call coffee
    {
      sec_counter = 0;                                  //	reset AutoOff counter

      if (water && temperature)                         //	machine ready:
      {
        while (button_2_cup_counter > 0)                //	check if button is pushed long time
        {
          if (button_2_cup_counter > BUTTON_LONG_THR)   //	button pushed for a long time:
          {
            make_coffee = 2;                            //	set coffee flag to 2 (2 espresso)
            button_2_cup_counter = 0;                   //	clear button counter
          }
        }
        if (make_coffee != 2)
          make_coffee = 4;                              //	set coffee flag to 4 (2 coffee) else
      }
    }

    if (water)                                          // water OK:
    {
      if (make_clean)                                   // if clean-flag is set:
      {
        set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);      // boiler off
        bool escape = false;                            // init escape-flag
        while (water && !escape)
        {                                               // pump until water is empty or escape flag is set

          unsigned int sense = detect_zero_crossing (); // detect zero crossing
          if (sense <= 100)
          {
            clear_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);  // generate trigger impulse for pump triac
            _delay_ms (3);
            set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);
          }
          water = get_water ();                       // update water state

          if (button_power_counter > BUTTON_THRESHOLD)
            escape = true;                            // check for power button counter and set escape flag
        }
        make_clean = false;                           // clear clean flag
      }

      else if (temperature)                           // temperature OK:
      {
        set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);    // boiler off

        led = 0b00000100;                             // set green LED

        if (make_coffee > 0)                          // if coffee flag is set:
        {
          if (make_coffee < 3)
            led = 0b00001010;                         // set orange LED blink
          else
            led = 0b00001000;                         // set green LED blink

          if (make_coffee == 1)
            pump_time = TIME_1_ESPRESSO;              // 1 cup of espresso (2s preinfusion included)
          else if (make_coffee == 2)
            pump_time = TIME_2_ESPRESSO;              // 2 cups of espresso (2s preinfusion included)
          else if (make_coffee == 3)
            pump_time = TIME_1_COFFEE;                // 1 cup of coffee
          else if (make_coffee == 4)
            pump_time = TIME_2_COFFEE;                // 2 cups of coffee
          else
            make_coffee = 0;

          user_time_counter = 0;                      // reset user time counter
          bool escape = false;                        // init escape flag
          while (user_time_counter < (pump_time * 1000) && water && !escape)
          {                                           // loop until pump time is reached or water is empty
            if (make_coffee > 2
                || (user_time_counter < 2000 || user_time_counter > 4000))
            {                                               // check for preinfusion break
              unsigned int sense = detect_zero_crossing (); // detect zero crossing
              if (sense <= 100)
              {
                clear_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);    // generate trigger impulse for pump triac
                _delay_ms (3);
                set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);
              }
            }

            water = get_water ();                     // update water state

            if (button_power_counter > BUTTON_THRESHOLD)
              escape = true;                          // check for power button counter and set escape flag
          }

          set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);      // pump off

          make_coffee = 0;                            // clear coffee flag

          sec_counter = 0;                            // reset AutoOff timer
        }
      }
      else                                            // temperature too low:
      {
        clear_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);  //	boiler on
        led = 0b00000010;                             // set red LED blink
      }
    }
    else                                              // water too low:
    {
      set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);      // boiler off
      set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);          // pump off

      led = 0b00100000;                               // set blue LED blink
    }

  }
}

/* function:  init()
 * return:    void
 *
 * Initializes relevant bits, timer and ADC.
 */
void init ()
{
  clear_bit(ZERO_CROSSING_ddr, ZERO_CROSSING_pin);  // zero crossing dection pins as input
  clear_bit(ZERO_CROSSING_w, ZERO_CROSSING_pin);    // no internal pull-up (for ADC)

  clear_bit(BUTTON_1_CUP_ddr, BUTTON_1_CUP_pin);    // button pins as input
  set_bit(BUTTON_1_CUP_w, BUTTON_1_CUP_pin);        // activate internal pull-ups
  clear_bit(BUTTON_2_CUP_ddr, BUTTON_2_CUP_pin);
  set_bit(BUTTON_2_CUP_w, BUTTON_2_CUP_pin);
  clear_bit(BUTTON_POWER_ddr, BUTTON_POWER_pin);
  set_bit(BUTTON_POWER_w, BUTTON_POWER_pin);

  set_bit(LED_RED_ddr, LED_RED_pin);                // LED pins as output
  clear_bit(LED_RED_w, LED_RED_pin);                // clear outputs (LEDs off)
  set_bit(LED_GREEN_ddr, LED_GREEN_pin);
  clear_bit(LED_GREEN_w, LED_GREEN_pin);
  set_bit(LED_BLUE_ddr, LED_BLUE_pin);
  clear_bit(LED_BLUE_w, LED_BLUE_pin);

  clear_bit(SENSOR_MAGNET_ddr, SENSOR_MAGNET_pin);  // sensor pins as input
  clear_bit(SENSOR_MAGNET_w, SENSOR_MAGNET_pin);    // no internal pull-up (for ADC)
  clear_bit(SENSOR_TEMP_ddr, SENSOR_TEMP_pin);
  clear_bit(SENSOR_TEMP_w, SENSOR_TEMP_pin);

  set_bit(TRIAC_BOILER_ddr, TRIAC_BOILER_pin);      // triac pins as output
  set_bit(TRIAC_BOILER_w, TRIAC_BOILER_pin);        // set outputs high (triac off)
  set_bit(TRIAC_PUMP_ddr, TRIAC_PUMP_pin);
  set_bit(TRIAC_PUMP_w, TRIAC_PUMP_pin);

  ADCSR = (1 << ADEN) | (1 << ADPS1);               // enable ADC, prescaler division factor 4

  //	TIMER1
  set_bit(TCCR1B, CTC1);                            // set timer 1 to CTC-Mode
  clear_bit(TCCR1B, CS11);                          // prescaler 8
  set_bit(TCCR1B, CS12);
  clear_bit(TCCR1B, CS11);
  clear_bit(TCCR1B, CS10);
  OCR1C = 124;                                      // period of 1 ms

  cli ();                                           // disable interrupts

  clear_bit(GIMSK, INT0);                           // disable interrupt 0

  set_bit(TIMSK, TOIE1);                            // activate timer 1

  sei ();                                           // enable interrupts
}

/* function:  power_off()
 * return:    void
 *
 * Clear bits and set controller to sleep mode.
 */
void power_off ()
{
  cli ();                                   // disable interrupts
  set_bit(GIMSK, INT0);                     // activate interrupt 0 (for wake-up)
  clear_bit(TIMSK, TOIE1);                  // deactivate timer 1
  sei ();                                   // enable interrupts

  clear_bit(LED_RED_w, LED_RED_pin);        // clear LED outputs
  clear_bit(LED_GREEN_w, LED_GREEN_pin);
  clear_bit(LED_BLUE_w, LED_BLUE_pin);

  set_bit(MCUCR, SM1);                      // activate power-down mode
  clear_bit(MCUCR, SM0);
  set_bit(MCUCR, SE);
  asm volatile("sleep"::);

                                            // entrance after wake-up:
  time_counter = 0;                         // reset counter
  sec_counter = 0;
  cli ();                                   // disable interrupts
  clear_bit(GIMSK, INT0);                   // disable interrupt 0
  set_bit(TIMSK, TOIE1);                    // enable timer 1
  sei ();                                   //	enable interrupts
}

/* function:  get_water()
 * return:    true    water OK
 *            false   not enough water
 *
 * Checks hall sensor for water state.
 */
bool get_water ()
{
  ADMUX = SENSOR_MAGNET_adc | (1 << ADLAR); // ADLAR
  set_bit(ADCSR, ADSC);
  loop_until_bit_is_clear (ADCSR, ADSC);
  unsigned char sense = ADCH;
  if ((water && sense > WATER_LOW) || (!water && sense >= WATER_OK))
    return true;
  return false;
}

/* function:  get_temperature()
 * return:    true    temperature OK
 *            false   temperature too low
 *
 * Checks NTC sensor for temperature state.
 */
bool get_temperature ()
{
  ADMUX = SENSOR_TEMP_adc | (1 << ADLAR);   // ADLAR
  set_bit(ADCSR, ADSC);
  loop_until_bit_is_clear (ADCSR, ADSC);
  unsigned char sense = ADCH;
  if (sense >= OPERATING_TEMPERATURE)
    return true;
  return false;
}

/* function:  detect_zero_crossing()
 * return:    unsigned int    ADC value
 *
 * Checks for zero crossing (with fixed offset)
 */
unsigned int detect_zero_crossing ()
{
  ADMUX = ZERO_CROSSING_adc;
  set_bit(ADCSR, ADSC);
  loop_until_bit_is_clear (ADCSR, ADSC);
  unsigned char sense_L = ADCL;
  unsigned char sense_H = ADCH;
  return (sense_H << 8) | sense_L;
}

/* interrupt function:  INT0_vect
 *
 * Dummy function for wake-up.
 */
ISR ( INT0_vect)
{
}

/* interrupt function:  TIMER1_OVF1_vect
 *
 * Timer interrupt. Increments counters and controls LED.
 */
ISR ( TIMER1_OVF1_vect)
{
  if (time_counter < 1000)
    time_counter++;           // global milliseconds counter and seconds counter (for AutoOff)
  else
  {
    time_counter = 0;
    sec_counter++;
  }
  user_time_counter++;        // universal counter (for pump time)

  bool leds_blink_on;         // status flag for blinking LEDs with 1Hz
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

  if (bit_is_clear (BUTTON_1_CUP_r, BUTTON_1_CUP_pin))
  {                           // left button counter
    if (button_1_cup_counter < 65535)
      button_1_cup_counter++;
  }
  else
  {
    if (button_1_cup_counter > 0)
      button_1_cup_counter--;
  }

  if (bit_is_clear (BUTTON_2_CUP_r, BUTTON_2_CUP_pin))
  {                           // right button counter
    if (button_2_cup_counter < 65535)
      button_2_cup_counter++;
  }
  else
  {
    if (button_2_cup_counter > 0)
      button_2_cup_counter--;
  }

  if (bit_is_clear (BUTTON_POWER_r, BUTTON_POWER_pin))
  {                           // power button counter
    if (button_power_counter < 255)
      button_power_counter++;
  }
  else
  {
    if (button_power_counter > 0)
      button_power_counter--;
  }
}
