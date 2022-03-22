/*
 * EasyButtonAtInt01.cpp.h
 *
 *  Arduino library for handling push buttons connected between ground and INT0 and / or INT1 pin.
 *  INT0 and INT1 are connected to Pin 2 / 3 on most Arduinos (ATmega328), to PB6 / PA3 on ATtiny167 and on ATtinyX5 we have only INT0 at PB2.
 *  The library is totally based on interrupt.
 *  Debouncing is implemented in a not blocking way! It is merely done by ignoring a button change within the debouncing time.
 *  So button state is instantly available without debouncing delay!
 *
 *  Usage:
 *  #define USE_BUTTON_0
 *  #include "EasyButtonAtInt01.h"
 *  EasyButton Button0AtPin2(true);
 *
 *  Copyright (C) 2018  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of EasyButtonAtInt01 https://github.com/ArminJo/EasyButtonAtInt01.
 *
 *  EasyButtonAtInt01 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef EASY_BUTTON_AT_INT01_H_
#define EASY_BUTTON_AT_INT01_H_

#define VERSION_EASY_BUTTON "3.1.0"
#define VERSION_EASY_BUTTON_MAJOR 3
#define VERSION_EASY_BUTTON_MINOR 1

/*
 *  Version 3.1.0 - 6/2020
 *  - 2 sets of constructors, one for only one button used and one for the second button if two buttons used.
 *  - Map pin numbers for Digispark pro boards, for use with with digispark library.
 *
 * Version 3.0.0 - 5/2020
 * - Added button release handler and adapted examples.
 * - Revoke change for "only one true result per press for checkForLongPressBlocking()". It is superseded by button release handler.
 * - Support buttons which are active high by defining BUTTON_IS_ACTIVE_HIGH.
 * - Improved detection of maximum bouncing period used in DebounceTest.
 *
 * Version 2.1.0 - 5/2020
 * - Avoid 1 ms delay for checkForLongPressBlocking() if button is not pressed.
 * - Only one true result per press for checkForLongPressBlocking().
 *
 * Version 2.0.0 - 1/2020
 * - Ported to ATtinyX5 and ATiny167.
 * - Support also PinChangeInterrupt for button 1 on Pin PA0 to PA7 for ATtiniy87/167.
 * - Long press detection support.
 * - Double press detection support.
 * - Renamed to EasyButtonAtInt01.cpp.h
 */

#if defined(__AVR__)
#include <Arduino.h>

/*
 * Usage:
 * #define USE_BUTTON_0  // Enable code for button at INT0
 * #define USE_BUTTON_1  // Enable code for button at INT1 (PCINT0 for ATtinyX5)
 * #include "EasyButtonAtInt01.h"
 * EasyButton Button0AtPin2(true);  // true  -> Button is connected to INT0
 * EasyButton Button0AtPin3(false, &Button3CallbackHandler); // false -> button is not connected to INT0 => connected to INT1
 * ...
 * digitalWrite(LED_BUILTIN, Button0AtPin2.ButtonToggleState);
 * ...
 *
 */

/*
 * Define BUTTON_IS_ACTIVE_HIGH if you buttons are active high.
 */
//#define BUTTON_IS_ACTIVE_HIGH
/*
 * Define USE_ATTACH_INTERRUPT to force use of the arduino function attachInterrupt().
 * Required if you get the error " multiple definition of `__vector_1'" (or `__vector_2'), because another library uses the attachInterrupt() function.
 * For one button it needs additional 160 bytes FLASH, for 2 buttons it needs additional 88 bytes.
 */
//#define USE_ATTACH_INTERRUPT
//
/*
 * You can define your own value if you have buttons which are worse or better than the one I have.
 * Since debouncing is not done with blocking wait, reducing this value makes not much sense, except you expect regular short button presses,
 * which durations are shorter than BUTTON_DEBOUNCING_MILLIS.
 * Press duration below 50 ms are almost impossible to generate by normal button pressing, but they can generated by just hitting the button.
 *
 * Test your own new value with the DebounceTest example
 *
 * Analyze the button actual debounce value with defining ANALYZE_MAX_BOUNCING_PERIOD and looking at MaxBouncingPeriodMillis.
 * Defining ANALYZE_MAX_BOUNCING_PERIOD computes the maximum bouncing period.
 * this is the time between first level change and last bouncing level change during BUTTON_DEBOUNCING_MILLIS
 */
//#define ANALYZE_MAX_BOUNCING_PERIOD
#ifndef BUTTON_DEBOUNCING_MILLIS
#define BUTTON_DEBOUNCING_MILLIS 50 // 35 millis measured for my button :-).
#endif

/*
 * Comment this out to save 2 bytes RAM and 64 bytes FLASH
 */
//#define NO_BUTTON_RELEASE_CALLBACK
//
/*
 * Return values for checkForLongPress()
 */
#define EASY_BUTTON_LONG_PRESS_STILL_POSSIBLE 0
#define EASY_BUTTON_LONG_PRESS_ABORT 1 // button was released, no long press detection possible
#define EASY_BUTTON_LONG_PRESS_DETECTED 2

#define EASY_BUTTON_LONG_PRESS_DEFAULT_MILLIS 400
#define EASY_BUTTON_DOUBLE_PRESS_DEFAULT_MILLIS 400

/*
 * Activate LED_BUILTIN as long as button is pressed
 */
//#define LED_FEEDBACK_TEST
#if defined(LED_FEEDBACK_TEST)
#  if ! defined(BUTTON_TEST_FEEDBACK_LED_PIN)
#    if defined(LED_BUILTIN)
#    define BUTTON_TEST_FEEDBACK_LED_PIN LED_BUILTIN  // if not specified, use built in led - pin 13 on Uno board
#    else
#    error "LED_FEEDBACK_TEST defined but no BUTTON_TEST_FEEDBACK_LED_PIN or LED_BUILTIN defined"
#    endif
#  endif
#endif

// For external measurement of code timing
//#define MEASURE_EASY_BUTTON_INTERRUPT_TIMING

#if defined(MEASURE_EASY_BUTTON_INTERRUPT_TIMING)
#  ifndef INTERRUPT_TIMING_OUTPUT_PIN
#define INTERRUPT_TIMING_OUTPUT_PIN 6  // use pin 6
//#define INTERRUPT_TIMING_OUTPUT_PIN 12  // use pin 12
#  endif
#endif

//#define TRACE
#ifdef TRACE
#warning If using TRACE, the timing of the interrupt service routine changes, e.g. you will see more spikes, than expected!
#endif

/*
 * These defines are here to enable saving of 150 bytes FLASH if only one button is needed
 */
//#define USE_BUTTON_0
//#define USE_BUTTON_1
#if ! (defined(USE_BUTTON_0) || defined(USE_BUTTON_1))
#error USE_BUTTON_0 and USE_BUTTON_1 are not defined, please define them or remove the #include "EasyButtonAtInt01.h"
#endif

/*
 * Pin and port definitions for Arduinos
 */
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define INT0_PIN 2
#define INT0_DDR_PORT (DDRB)
#define INT0_IN_PORT  (PINB)
#define INT0_OUT_PORT (PORTB)

#  if defined(USE_BUTTON_1)
#    if ! defined(INT1_PIN)
#define INT1_PIN 3
#    elif (INT1_PIN != 2) && (INT1_PIN > 5)
#error INT1_PIN (for PCINT0 interrupt) can only be 0,1,3,4,5
#    endif
#define INT1_DDR_PORT (DDRB)
#define INT1_IN_PORT  (PINB)
#define INT1_OUT_PORT (PORTB)
#  endif // defined(USE_BUTTON_1)

#elif defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
// from here we use only ATtinyCore / PAx / PBx numbers, since on Digispark board and core library there is used a strange enumeration of pins
#define INT0_PIN 14 // PB6 / INT0 is connected to USB+ on DigisparkPro boards and labeled with 3 (D3)
#define INT0_DDR_PORT (DDRB)
#define INT0_IN_PORT  (PINB)
#define INT0_OUT_PORT (PORTB)


#  if defined(USE_BUTTON_1)
#    if ! defined(INT1_PIN)
#define INT1_PIN 3 // PA3 labeled 9 on DigisparkPro boards
#    endif // ! defined(INT1_PIN)

// Check for pin range and map digispark to PA pin number
#    if defined(ARDUINO_AVR_DIGISPARKPRO)
#      if INT1_PIN == 5
#undef INT1_PIN
#define INT1_PIN 7 // PA7
#      elif INT1_PIN == 6
#undef INT1_PIN
#define INT1_PIN 0 // PA0
#      elif INT1_PIN == 7
#undef INT1_PIN
#define INT1_PIN 1 // PA1
#      elif INT1_PIN == 8
#undef INT1_PIN
#define INT1_PIN 2 // PA2
#      elif INT1_PIN == 9
#undef INT1_PIN
#define INT1_PIN 3 // PA3
#      elif INT1_PIN == 10
#undef INT1_PIN
#define INT1_PIN 4 // PA4
#      elif INT1_PIN == 11
#undef INT1_PIN
#define INT1_PIN 5 // PA5
#      elif INT1_PIN == 12
#undef INT1_PIN
#define INT1_PIN 6 // PA6
#      else
#error INT1_PIN (for PCINT0 interrupt) can only be 5 to 12
#      endif
#    else // defined(ARDUINO_AVR_DIGISPARKPRO)
#      if (INT1_PIN > 7)
#error INT1_PIN (for PCINT0 interrupt) can only be 0 to 7
#      endif
#    endif // defined(ARDUINO_AVR_DIGISPARKPRO)
#define INT1_DDR_PORT (DDRA)
#define INT1_IN_PORT  (PINA)
#define INT1_OUT_PORT (PORTA)
#  endif // defined(USE_BUTTON_1)
#else

// other AVR MCUs
#define INT0_PIN 2
#define INT0_DDR_PORT (DDRD)
#define INT0_IN_PORT  (PIND)
#define INT0_OUT_PORT (PORTD)

#  if defined(USE_BUTTON_1)
#    if ! defined(INT1_PIN)
#define INT1_PIN 3
#    elif (INT1_PIN > 7)
#error INT1_PIN (for PCINT2 interrupt) can only be Arduino pins 0 to 7 (PD0 to PD7)
#    endif
#define INT1_DDR_PORT (DDRD)
#define INT1_IN_PORT  (PIND)
#define INT1_OUT_PORT (PORTD)
#  endif // defined(USE_BUTTON_1)
#endif // defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)

#if defined(USE_BUTTON_1)
#define INT1_BIT INT1_PIN
#endif

#if defined(USE_BUTTON_1) && ((! defined(ISC10)) || ((defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)) && INT1_PIN != 3)) \
    && ! defined(INTENTIONALLY_USE_PCI0_FOR_BUTTON1) && !(defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__))
#warning Using PCINT0 interrupt for button 1
#endif

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
#define EICRA  MCUCR
#define EIFR   GIFR
#define EIMSK  GIMSK
#endif

#if (INT0_PIN >= 8)
#define INT0_BIT (INT0_PIN - 8)
#else
#define INT0_BIT INT0_PIN
#endif

#define BUTTON_AT_INT0 ((bool)true)
#define BUTTON_AT_INT1_OR_PCINT ((bool)false)

class EasyButton {

public:

    /*
     * These constructors are deterministic if only one button is enabled
     * If two buttons are enabled they can be taken for the 1. button at INT0
     */
    EasyButton();
    EasyButton(void (*aButtonPressCallback)(bool aButtonToggleState));
#if ! defined(NO_BUTTON_RELEASE_CALLBACK)
    EasyButton(void (*aButtonPressCallback)(bool aButtonToggleState),
            void (*aButtonReleaseCallback)(bool aButtonToggleState, uint16_t aButtonPressDurationMillis));
#endif
    /*
     * These constructors use the first (bool) parameter to decide which button to take.
     */
    EasyButton(bool aIsButtonAtINT0);
    EasyButton(bool aIsButtonAtINT0, void (*aButtonPressCallback)(bool aButtonToggleState));
#if ! defined(NO_BUTTON_RELEASE_CALLBACK)
    EasyButton(bool aIsButtonAtINT0, void (*aButtonPressCallback)(bool aButtonToggleState),
            void (*aButtonReleaseCallback)(bool aButtonToggleState, uint16_t aButtonPressDurationMillis));
#endif

    void init(bool aIsButtonAtINT0);

    /*
     * !!! checkForDoublePress() works only reliable if called in button press callback function !!!
     */
    bool checkForDoublePress(uint16_t aDoublePressDelayMillis = EASY_BUTTON_DOUBLE_PRESS_DEFAULT_MILLIS);

    bool readButtonState();
    bool readDebouncedButtonState();
    bool updateButtonState();
    uint16_t updateButtonPressDuration(); // Updates the ButtonPressDurationMillis by polling, since this cannot be done by interrupt.

    bool checkForForButtonNotPressedTime(uint16_t aTimeoutMillis);

    //!!! Consider to use button release callback handler and check the ButtonPressDurationMillis
    uint8_t checkForLongPress(uint16_t aLongPressThresholdMillis = EASY_BUTTON_LONG_PRESS_DEFAULT_MILLIS);
    bool checkForLongPressBlocking(uint16_t aLongPressThresholdMillis = EASY_BUTTON_LONG_PRESS_DEFAULT_MILLIS);

    void handleINT01Interrupts(); // internal use only

    bool LastBounceWasChangeToInactive; // Internal state, reflects actual reading with spikes and bouncing. Negative logic: true / active means button pin is LOW
    volatile bool ButtonStateIsActive;  // Negative logic: true / active means button pin is LOW. If last press duration < BUTTON_DEBOUNCING_MILLIS it holds wrong value (true instead of false) :-(
    volatile bool ButtonToggleState;    // Toggle is on press, not on release - initial value is false

    /*
     * Flag to enable action only once. Only set to true by library. Can be checked and set to false my main program to enable only one action per button press
     */
    volatile bool ButtonStateHasJustChanged;

    /*
     *  Duration of active state. Is is set at button release. Can in theory not be less than BUTTON_DEBOUNCING_MILLIS.
     *  By definition, shorter presses are recognized as bouncing.
     *  To cover this case you can call updateButtonState() from an outside loop which updates the button state in this case.
     */
    volatile uint16_t ButtonPressDurationMillis;

    /*
     * Milliseconds of last button change, going active or inactive.
     * If bouncing occurs the time is not updated with the time of the bouncing.
     * So ButtonPressDurationMillis is the complete time and not the time after the last bounce.
     */
    volatile unsigned long ButtonLastChangeMillis;

    /*
     * If last button change is going inactive, it contains the same value as ButtonLastChangeMillis
     * It is required for double press recognition, which is done when button is active and ButtonLastChangeMillis has just changed.
     */
    volatile unsigned long ButtonReleaseMillis;

#if defined(ANALYZE_MAX_BOUNCING_PERIOD)
    volatile unsigned int MaxBouncingPeriodMillis = 0; // Maximum bouncing period. Time between first level change and last bouncing level change during BUTTON_DEBOUNCING_MILLIS
#endif

    volatile bool isButtonAtINT0;
    void (*ButtonPressCallback)(bool aButtonToggleState) = NULL; // If not null, is called on every button press with ButtonToggleState as parameter
#if ! defined(NO_BUTTON_RELEASE_CALLBACK)
    void (*ButtonReleaseCallback)(bool aButtonToggleState, uint16_t aButtonPressDurationMillis) = NULL; // If not null, is called on every button release with ButtonPressDurationMillis as parameter
#endif

#if defined(USE_BUTTON_0)
    static EasyButton *sPointerToButton0ForISR;
#endif
#if defined(USE_BUTTON_1)
    static EasyButton *sPointerToButton1ForISR;
#endif
};
// end of class definition

void handleINT0Interrupt();
void handleINT1Interrupt();

/*
 * This functions are weak and can be replaced by your own code
 */
#if defined(USE_BUTTON_0)
void __attribute__ ((weak)) handleINT0Interrupt();
#endif

#if defined(USE_BUTTON_1)
void __attribute__ ((weak)) handleINT1Interrupt();
#endif

#endif // defined(__AVR__)
#endif /* EASY_BUTTON_AT_INT01_H_ */

#pragma once
