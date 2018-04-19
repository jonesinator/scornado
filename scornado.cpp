/**
 * Table tennis score keeper.
 *
 * @author Aaron Jones <aaron@jonesinator.com>
 * @license GPLv3
 */

#include <avr/io.h>

#include "avr_io.hpp"
#include "table_tennis.hpp"

/**
 * The atmega328p has three I/O banks and we use all of them.
 */
avr_io_bank avr_io_bank_b { &DDRB, &PORTB, &PINB };
avr_io_bank avr_io_bank_c { &DDRC, &PORTC, &PINC };
avr_io_bank avr_io_bank_d { &DDRD, &PORTD, &PIND };

/**
 * Assign low-level pin assignments.
 */
avr_digital_output_pin            sevseg_a(avr_io_bank_d, 0);       /* Pin 2  */
avr_digital_output_pin            sevseg_b(avr_io_bank_d, 1);       /* Pin 3  */
avr_digital_output_pin            sevseg_c(avr_io_bank_d, 2);       /* Pin 4  */
avr_digital_output_pin            sevseg_d(avr_io_bank_d, 3);       /* Pin 5  */
avr_digital_output_pin            sevseg_e(avr_io_bank_d, 4);       /* Pin 6  */
avr_digital_output_pin            sevseg_f(avr_io_bank_d, 5);       /* Pin 11 */
avr_digital_output_pin            sevseg_g(avr_io_bank_d, 6);       /* Pin 12 */
avr_digital_input_pin          undo_switch(avr_io_bank_d, 7, true); /* Pin 13 */
avr_digital_output_pin  p1_games_won_digit(avr_io_bank_b, 0);       /* Pin 14 */
avr_digital_output_pin p1_score_ones_digit(avr_io_bank_b, 1);       /* Pin 15 */
avr_digital_output_pin p1_score_tens_digit(avr_io_bank_b, 2);       /* Pin 16 */
avr_digital_output_pin  p2_games_won_digit(avr_io_bank_b, 3);       /* Pin 17 */
avr_digital_output_pin p2_score_ones_digit(avr_io_bank_b, 4);       /* Pin 18 */
avr_digital_output_pin p2_score_tens_digit(avr_io_bank_b, 5);       /* Pin 19 */
avr_digital_output_pin        p1_serve_led(avr_io_bank_c, 0);       /* Pin 23 */
avr_digital_output_pin        p2_serve_led(avr_io_bank_c, 1);       /* Pin 24 */
avr_digital_input_pin     game_mode_switch(avr_io_bank_c, 2, true); /* Pin 25 */
avr_digital_input_pin   first_serve_switch(avr_io_bank_c, 3, true); /* Pin 26 */
avr_digital_input_pin      p1_score_switch(avr_io_bank_c, 4, true); /* Pin 27 */
avr_digital_input_pin      p2_score_switch(avr_io_bank_c, 5, true); /* Pin 28 */

/**
 * Assign high-level pin abstractions.
 */
avr_button undo_button(undo_switch);
avr_button game_mode_button(game_mode_switch);
avr_button first_serve_button(first_serve_switch);
avr_button p1_score_button(p1_score_switch);
avr_button p2_score_button(p2_score_switch);
avr_seven_segment_pins seven_segment_pins(
    sevseg_a,
    sevseg_b,
    sevseg_c,
    sevseg_d,
    sevseg_e,
    sevseg_f,
    sevseg_g,
    avr_digital_output_pin_null::instance());
avr_seven_segment_display<2> p1_score_display(
    seven_segment_pins,
    avr_digital_output_pin_null::instance(),
    &p1_score_ones_digit,
    &p1_score_tens_digit);
avr_seven_segment_display<1> p1_games_won_display(
    seven_segment_pins,
    avr_digital_output_pin_null::instance(),
    &p1_games_won_digit);
avr_seven_segment_display<2> p2_score_display(
    seven_segment_pins,
    avr_digital_output_pin_null::instance(),
    &p2_score_ones_digit,
    &p2_score_tens_digit);
avr_seven_segment_display<1> p2_games_won_display(
    seven_segment_pins,
    avr_digital_output_pin_null::instance(),
    &p2_games_won_digit);

/**
 * Entry point for the program. Processes table tennis games.
 */
int main (int, char**) {
    table_tennis tt;
    while (true) {
        /**
         * Handle inputs.
         */
        switch (game_mode_button.check()) {
            case avr_button::action::pressed:
                tt.set_game_mode(table_tennis::game_mode::to_11);
                break;
            case avr_button::action::released:
                tt.set_game_mode(table_tennis::game_mode::to_21);
                break;
            case avr_button::action::none:
                break;
        }

        switch (first_serve_button.check()) {
            case avr_button::action::pressed:
                tt.set_first_serve(table_tennis::serve_player::p1);
                break;
            case avr_button::action::released:
                tt.set_first_serve(table_tennis::serve_player::p2);
                break;
            case avr_button::action::none:
                break;
        }

        if (undo_button.check() == avr_button::action::pressed) {
            tt.undo();
        }

        if (p1_score_button.check() == avr_button::action::pressed) {
            tt.p1_score();
        }

        if (p2_score_button.check() == avr_button::action::pressed) {
            tt.p2_score();
        }

        /**
         * Handle outputs.
         */
        p1_serve_led.set(tt.serve() == table_tennis::serve_player::p1);
        p2_serve_led.set(!(tt.serve() == table_tennis::serve_player::p1));
        p1_score_display.display_decimal(tt.get_p1_score());
        p1_games_won_display.display_decimal(tt.get_p1_games_won());
        p2_score_display.display_decimal(tt.get_p2_score());
        p2_games_won_display.display_decimal(tt.get_p2_games_won());
    }

    return 0;
}
