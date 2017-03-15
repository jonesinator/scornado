/**
 * Hardware abstractions for AVR GPIO pins.
 *
 * Contains abstractions for:
 *     Digital input pins.
 *     Digital output pins.
 *     Debounced push buttons.
 *     Seven segment displays.
 *
 * @author Aaron Jones <aaron@jonesinator.com>
 * @license GPLv3
 */

#ifndef __AVR_IO_HPP__
#define __AVR_IO_HPP__

#include <util/delay.h>

/**
 * The GPIO pin banks on AVR are controlled by three different registers. This
 * structure simply stores the addresses of the three related registers for a
 * particular bank.
 */
struct avr_io_bank {
    /**
     * The Data Direction Register for the port.
     */
    volatile uint8_t* const ddr;

    /**
     * The port control. For output pins it controls the output state, for input
     * pins it controls whether or not the internal pull-up register is enabled.
     */
    volatile uint8_t* const port;

    /**
     * The input register. Used to read the state of input pins.
     */
    volatile uint8_t* const pin;
};

/**
 * Abstraction for an digital input pin. This is often a building block for
 * higher-level input constructs (debounced buttons, etc.) Nothing prevents
 * multiple objects being created for the same pin, the client must ensure
 * that this does not happen.
 */
struct avr_digital_input_pin {
    /**
     * Initializes a particular pin as input, optionally enabling the pull-up
     * register.
     *
     * @param bank    The pin bank that houses this input pin.
     * @param bit     The bit (0-7) in the bank that controls this input pin.
     * @param pull_up True if the internal pull-up should be enabled, false
     *                otherwise.
     */
    avr_digital_input_pin(avr_io_bank& bank, uint8_t bit, bool pull_up):
        _bank(bank),
        _mask(1 << bit),
        _pull_up(pull_up) {
        /**
         * Enable the pin as input by clearing its bit in the data direction
         * register.
         */
        *_bank.ddr &= ~_mask;
        if (_pull_up) {
            /**
             * Enable the internal pull-up by setting its bit in the port
             * register.
             */
            *_bank.port |= _mask;
        }
    }

    /**
     * Gets the current state of this input pin.
     */
    bool read() const {
        return *_bank.pin & _mask;
    }

    /**
     * Determines whether or not the pull-up register is enabled for this pin.
     *
     * @returns True if the input is high, false if the input is low.
     */
    bool pull_up() const {
        return _pull_up;
    }

private:
    /**
     * The pin bank containing this input pin.
     */
    const avr_io_bank& _bank;

    /**
     * The bitmask used to single out just this input pin in the control
     * registers.
     */
    const uint8_t _mask;

    /**
     * Whether or not the internal pull-up is enabled. True if the pull-up is
     * enabled, false if the pull-up is not enabled.
     */
    const bool _pull_up;
};

/**
 * Abstract interface for digital output pins. This abstraction exists so we can
 * have output pins that don't actually anything, but are needed to satisfy an
 * interface, for example a seven segment display where the decimal point pin is
 * unused.
 */
struct avr_digital_output_pin_interface {
    virtual void set(bool high) const = 0;
};

/**
 * A digital output pin that doesn't actually do anything.
 */
struct avr_digital_output_pin_null : avr_digital_output_pin_interface {
    /**
     * Get a singleton instance of the null digital output pin.
     *
     * @returns The singleton instance of the null digital output pin.
     */
    static avr_digital_output_pin_null& instance() {
        static avr_digital_output_pin_null pin;
        return pin;
    }

    /**
     * "Sets" the state of the null digital output pin, doesn't actually do
     * anything, this just satisfies the digital output pin interface.
     */
    virtual void set(bool) const override {
    }
};

/**
 * Abstraction for a digital output pin. This is often a building block for
 * higher-level input constructs (seven segment displays, etc.) Nothing
 * prevents multiple objects being created for the same pin, the client must
 * ensure that this does not happen.
 */
struct avr_digital_output_pin : avr_digital_output_pin_interface {
    /**
     * Initializes a particular pin as output.
     *
     * @param bank    The pin bank that houses this input pin.
     * @param bit     The bit (0-7) in the bank that controls this input pin.
     */
    avr_digital_output_pin(avr_io_bank& bank, uint8_t bit):
        _bank(bank),
        _mask(1 << bit) {
        /**
         * Enable the pin as output by setting its bit in the data direction
         * register.
         */
        *_bank.ddr |= _mask;
    }

    /**
     * Sets the current output state of the digital output pin.
     *
     * @param high True if the pin should be set to high, false if the pin
     *             should be set to low.
     */
    virtual void set(bool high) const override {
        if (high) {
            *_bank.port |= _mask;
        } else {
            *_bank.port &= ~_mask;
        }
    }

private:
    /**
     * The pin bank containing this input pin.
     */
    const avr_io_bank& _bank;

    /**
     * The bitmask used to single out just this input pin in the control
     * registers.
     */
    const uint8_t _mask;
};

/**
 * Wrapper around an input pin that performs simple debouncing logic.
 */
struct avr_button {
    /**
     * Enumeration for the possible states of the button and whether or not the
     * button has changed states.
     */
    enum class action {
        /**
         * The button has not changed states since the last time it was checked.
         */
        none,

        /**
         * The button is currently pressed (state) or has just changed to
         * pressed (check).
         */
        pressed,

        /**
         * The button is currently released (state) or has just changed to
         * released (check).
         */
        released
    };

    /**
     * Create a button object from an input pin.
     */
    avr_button(avr_digital_input_pin& input_pin):
        _input_pin(input_pin) {
    }

    /**
     * Update the state for debounce logic and determine whether or not the
     * button's state has changed.
     *
     * @return action::none if the button's state has not changed,
     *         action::pressed if the button has just changed to pressed,
     *         action::released if the button has just changed to released.
     */
    action check() {
        /**
         * Update the state of the pin. If the internal pull-up on the input pin
         * is enabled then assume that the logic is reversed, i.e. a high
         * reading indicates that the button is not pressed.
         */
        if (_input_pin.pull_up()) {
            _states[_counter++ % 3] = _input_pin.read()
                                      ? action::released
                                      : action::pressed;
        } else {
            _states[_counter++ % 3] = _input_pin.read()
                                      ? action::pressed
                                      : action::released;
        }

        /**
         * The overall state will only change when the state has been stable for
         * some number of readings. This is very simple debounce logic.
         */
        if (_states[0] == _states[1] && _states[1] == _states[2]) {
            action new_action = _states[0];
            if (new_action == action::pressed &&
                _current_action == action::released)
            {
                _current_action = new_action;
                return action::pressed;
            } else if (new_action == action::released &&
                       _current_action == action::pressed) {
                _current_action = new_action;
                return action::released;
            }
        }

        return action::none;
    }

    /**
     * Gets the current state of the button since the last call to check.
     *
     * @returns An action enumeration representing the current state of the
     *          button.
     */
    action state() const {
        return _current_action;
    }

private:
    /**
     * The underlying input pin representing this button.
     */
    const avr_digital_input_pin& _input_pin;

    /**
     * A counter that is used to determine the index in the _states array where
     * readings should be placed.
     */
    uint8_t _counter = 0;

    /**
     * Buffer for keeping the last three states of the button for debounce
     * logic.
     */
    action _states[3] = { action::released,
                          action::released,
                          action::released };

    /**
     * The current stable state of the button.
     */
    action _current_action = action::released;
};

/**
 * Bitmasks for each individual segment of seven segment displays.
 *
 *   AA
 *  F  B
 *  F  B
 *   GG
 *  E  C
 *  E  C
 *   DD  DP
 */
static constexpr uint8_t SEG_A  = 0b00000001;
static constexpr uint8_t SEG_B  = 0b00000010;
static constexpr uint8_t SEG_C  = 0b00000100;
static constexpr uint8_t SEG_D  = 0b00001000;
static constexpr uint8_t SEG_E  = 0b00010000;
static constexpr uint8_t SEG_F  = 0b00100000;
static constexpr uint8_t SEG_G  = 0b01000000;
static constexpr uint8_t SEG_DP = 0b10000000;

/**
 * Seven segment patterns for decimal and hexadecimal numbers.
 */
static constexpr uint8_t SEVSEG[16] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F        , /* 0 */
            SEG_B | SEG_C                                , /* 1 */
    SEG_A | SEG_B |         SEG_D | SEG_E |         SEG_G, /* 2 */
    SEG_A | SEG_B | SEG_C | SEG_D |                 SEG_G, /* 3 */
            SEG_B | SEG_C |                 SEG_F | SEG_G, /* 4 */
    SEG_A |         SEG_C | SEG_D |         SEG_F | SEG_G, /* 5 */
                    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, /* 6 */
    SEG_A | SEG_B | SEG_C                                , /* 7 */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, /* 8 */
    SEG_A | SEG_B | SEG_C |                 SEG_F | SEG_G, /* 9 */
    SEG_A | SEG_B | SEG_C |         SEG_E | SEG_F | SEG_G, /* A */
                    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, /* b */
    SEG_A |                 SEG_D | SEG_E | SEG_F        , /* C */
            SEG_B | SEG_C | SEG_D | SEG_E |         SEG_G, /* d */
    SEG_A |                 SEG_D | SEG_E | SEG_F | SEG_G, /* E */
    SEG_A |                         SEG_E | SEG_F | SEG_G, /* F */
};

/**
 * Abstraction over the display pins for a seven segment display. This class is
 * not responsible for digit selection, just the segments. This allows multiple
 * seven segment digits to use the same pin set with time mulitplexing.
 */
struct avr_seven_segment_pins {
    /**
     * Creates a seven segment display controller from the given output pins.
     *
     * @param seg_a  The output pin to use for the A segment.
     * @param seg_b  The output pin to use for the B segment.
     * @param seg_c  The output pin to use for the C segment.
     * @param seg_d  The output pin to use for the D segment.
     * @param seg_e  The output pin to use for the E segment.
     * @param seg_f  The output pin to use for the F segment.
     * @param seg_g  The output pin to use for the G segment.
     * @param seg_dp The output pin to use for the decimal point segment.
     */
    avr_seven_segment_pins(const avr_digital_output_pin_interface& seg_a,
                           const avr_digital_output_pin_interface& seg_b,
                           const avr_digital_output_pin_interface& seg_c,
                           const avr_digital_output_pin_interface& seg_d,
                           const avr_digital_output_pin_interface& seg_e,
                           const avr_digital_output_pin_interface& seg_f,
                           const avr_digital_output_pin_interface& seg_g,
                           const avr_digital_output_pin_interface& seg_dp):
        _segs{&seg_a, &seg_b, &seg_c, &seg_d, &seg_e, &seg_f, &seg_g, &seg_dp} {
    }

    /**
     * Outputs a decimal number to the seven segment display pins.
     *
     * @param number The number to display.
     */
    void display_decimal(uint8_t number) {
        display_custom(SEVSEG[number % 10]);
    }

    /**
     * Outputs a hexadecimal number to the seven segment display pins.
     *
     * @param number The number to display.
     */
    void display_hex(uint8_t number) {
        display_custom(SEVSEG[number % 0x10]);
    }

    /**
     * Sets or clears the output of the decimal point segment.
     *
     * @param display True to set the decimal point, false to clear it.
     */
    void display_decimal_point(bool display) {
        if (display) {
            display_custom(_current_display | SEG_DP);
        } else {
            display_custom(_current_display & ~SEG_DP);
        }
    }

    /**
     * Sets all segments to not display.
     */
    void clear() {
        display_custom(0);
    }

    /**
     * Displays a custom pattern on the seven segment display.
     *
     * @param mask The mask to output to the seven segment display. Use the
     *             SEG_X constants to control what is displayed.
     */
    void display_custom(uint8_t mask) {
        for (uint8_t i = 0; i < 8; ++i) {
            _segs[i]->set(mask & (1 << i));
        }
        _current_display = mask;
    }

private:
    const avr_digital_output_pin_interface* const _segs[8];

    /**
     * The mask that is currently being displayed.
     */
    uint8_t _current_display = 0;
};

/**
 * Abstraction for a time multiplxed seven segment display with some number
 * of digits and a colon segment. If the display has no colon segment then
 * avr_digital_output_pin_null can be passed.
 *
 * @tparam num_digits_t The number of digits in the display.
 */
template <uint8_t num_digits_t>
struct avr_seven_segment_display {
    /**
     * Creates a seven segment display.
     *
     * @tparam digits_t Output pins for each digit.
     *
     * @param seg    The seven segment pins to use for the display.
     * @param colon  The output pin to use for the colon segment.
     * @param digits The output pins to use to select each digit.
     */
    template <typename ...digits_t>
    avr_seven_segment_display(avr_seven_segment_pins& seg,
                              const avr_digital_output_pin_interface& colon,
                              digits_t... digits):
        _seg(seg),
        _colon(colon),
        _digits{digits...} {
    }

    /**
     * Display a number in decimal.
     *
     * @param number        The number to display.
     * @param decimal_point The digit on which the decimal point segment should
     *                      be enabled. Use -1 to disable the decimal point.
     */
    void display_decimal(uint8_t number, int8_t decimal_point = -1) {
        clear_digits();
        for (uint8_t i = 0; i < num_digits_t; ++i) {
            if (number) {
                _seg.display_decimal(number % 10);
                number /= 10;
            } else if (i == 0 || (decimal_point >= 0 && i <= decimal_point)) {
		/**
                 * Always display the first digit and digits up to the decimal
		 * point.
                 */
                _seg.display_decimal(0);
            } else {
                /**
                 * Don't display digits after the first digit if the number is
                 * zero.
                 */
                _seg.clear();
            }

            if (i == decimal_point) {
                _seg.display_decimal_point(true);
            }

            _digits[i]->set(true);
            _delay_ms(DIGIT_DELAY_MS);
            _digits[i]->set(false);
            _seg.clear();
        }
    }

    /**
     * Display a number in hexadecimal.
     *
     * @param The number to display.
     * @param decimal_point The digit on which the decimal point segment should
     *                      be enabled. Use -1 to disable the decimal point.
     */
    void display_hex(uint32_t number, int8_t decimal_point = -1) {
        clear_digits();
        for (uint8_t i = 0; i < num_digits_t; ++i) {
            if (number) {
                _seg.display_hex(number % 0x10);
                number /= 0x10;
            } else if (i == 0 || (decimal_point >= 0 && i <= decimal_point)) {
		/**
                 * Always display the first digit and digits up to the decimal
		 * point.
                 */
                _seg.display_hex(0);
            } else {
                /**
                 * Don't display digits after the first digit if the number is
                 * zero.
                 */
                _seg.clear();
            }

            if (i == decimal_point) {
                _seg.display_decimal_point(true);
            }

            _digits[i]->set(true);
            _delay_ms(DIGIT_DELAY_MS);
            _digits[i]->set(false);
            _seg.clear();
        }
    }

    /**
     * Whether or not to display the colon.
     *
     * @param display True to display the colon, false to not display the colon.
     */
    void display_colon(bool display) {
        _colon.set(display);
    }

    /**
     * Display a custom pattern to a particular digit.
     *
     * @param mask  The pattern to display.
     * @param digit The digit on which the pattern should be displayed.
     */
    void display_custom(uint8_t mask, uint8_t digit) {
        if (digit <= num_digits_t) {
            clear_digits();
            _seg.display_custom(mask);
            _digits[digit]->set(true);
        }
    }

private:
    /**
     * How long to display each digit for time multiplexing.
     */
    static const int DIGIT_DELAY_MS = 3;

    /**
     * Clears all digit seletion pins.
     */
    void clear_digits() {
        for (int i = 0; i < num_digits_t; ++i) {
            _digits[i]->set(false);
        }
    }

    /**
     * The pins to use for the segments.
     */
    avr_seven_segment_pins& _seg;

    /**
     * The pin to use to display the colon.
     */
    const avr_digital_output_pin_interface& _colon;

    /**
     * The pins to use to select digits.
     */
    const avr_digital_output_pin_interface* _digits[num_digits_t];
};

#endif /* __AVR_IO_HPP__ */
