#include "software_serial_core.h"
#include "software_serial_avr_port_name_compatibility.h"
#include "core/avr_pins.h"
#include <avr/wdt.h>
#include <stddef.h>

SoftwareSerial_t *active_serial_instance = NULL;

//=============================================================================
// Helper: port register access
//=============================================================================

static inline volatile uint8_t* get_port_in(SS_Port_t port) {
    switch(port) {
        #if defined(PORTB)
        case SS_PORT_B: return &PINB;
        #endif
        #if defined(PORTC)
        case SS_PORT_C: return &PINC;
        #endif
        #if defined(PORTD)
        case SS_PORT_D: return &PIND;
        #endif
        default: return NULL;
    }
}

static inline volatile uint8_t* get_port_out(SS_Port_t port) {
    switch(port) {
        #if defined(PORTB)
        case SS_PORT_B: return &PORTB;
        #endif
        #if defined(PORTC)
        case SS_PORT_C: return &PORTC;
        #endif
        #if defined(PORTD)
        case SS_PORT_D: return &PORTD;
        #endif
        default: return NULL;
    }
}

static inline volatile uint8_t* get_ddr(SS_Port_t port) {
    switch(port) {
        #if defined(DDRB)
        case SS_PORT_B: return &DDRB;
        #endif
        #if defined(DDRC)
        case SS_PORT_C: return &DDRC;
        #endif
        #if defined(DDRD)
        case SS_PORT_D: return &DDRD;
        #endif
        default: return NULL;
    }
}

static inline volatile uint8_t* get_pcmsk(SS_Port_t port) {
    switch(port) {
        #if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
            // ATtiny uses a single PCMSK register for all pins
            case SS_PORT_B: return &PCMSK;
        #else
            #if defined(PCMSK0)
            case SS_PORT_B: return &PCMSK0;
            #endif
            #if defined(PCMSK1)
            case SS_PORT_C: return &PCMSK1;
            #endif
            #if defined(PCMSK2)
            case SS_PORT_D: return &PCMSK2;
            #endif
        #endif
        default: return NULL;
    }
}

static inline uint8_t get_pcint_bit(SS_Port_t port) {
    #if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
        // ATtiny uses a single PCIE bit in GIMSK
        return PCIE; // Return the bit mask for the PCIE bit
    #else
    switch(port) {
        #if defined(PCIE0)
        case SS_PORT_B: return PCIE0;
        #endif
        #if defined(PCIE1)
        case SS_PORT_C: return PCIE1;
        #endif
        #if defined(PCIE2)
        case SS_PORT_D: return PCIE2;
        #endif
        default: return 0;
    }
    #endif
}

//=============================================================================
// RX pin read
//=============================================================================

uint8_t software_serial_rx_pin_read(SoftwareSerial_t *ss) {
    return ((*(ss->rx_port_in)) & ss->rx_bit_mask) ? 1 : 0;
}

//=============================================================================
// Core receive routine
//=============================================================================

static void software_serial_recv(SoftwareSerial_t *ss) {
    uint8_t d = 0;
    uint8_t next_head;

    if (ss->inverse_logic ? software_serial_rx_pin_read(ss) : !software_serial_rx_pin_read(ss)) {

        // Save and disable all timer interrupts
        #if defined(TIMSK)
        uint8_t saved_timsk = TIMSK; TIMSK = 0;
        #endif
        #if defined(TIMSK0)
        uint8_t saved_timsk0 = TIMSK0; TIMSK0 = 0;
        #endif
        #if defined(TIMSK1)
        uint8_t saved_timsk1 = TIMSK1; TIMSK1 = 0;
        #endif
        #if defined(TIMSK2)
        uint8_t saved_timsk2 = TIMSK2; TIMSK2 = 0;
        #endif
        #if defined(TIMSK3)
        uint8_t saved_timsk3 = TIMSK3; TIMSK3 = 0;
        #endif
        #if defined(TIMSK4)
        uint8_t saved_timsk4 = TIMSK4; TIMSK4 = 0;
        #endif
        #if defined(TIMSK5)
        uint8_t saved_timsk5 = TIMSK5; TIMSK5 = 0;
        #endif

        if (ss->pcint_mask_reg) {
            *(ss->pcint_mask_reg) &= ~(ss->pcint_mask_value);
        }

        _delay_loop_2(ss->rx_delay_centering);

        for (uint8_t i = 8; i > 0; --i) {
            _delay_loop_2(ss->rx_delay_intrabit);
            d >>= 1;
            if (software_serial_rx_pin_read(ss)) {
                d |= 0x80;
            }
        }

        if (ss->inverse_logic) {
            d = ~d;
        }

        next_head = (ss->rx_buffer_head + 1) % SS_RX_BUFFER_SIZE;
        if (next_head != ss->rx_buffer_tail) {
            ss->rx_buffer[ss->rx_buffer_head] = d;
            ss->rx_buffer_head = next_head;
        } else {
            ss->buffer_overflow = true;
        }

        _delay_loop_2(ss->rx_delay_stopbit);

        // Restore all timer interrupts
        #if defined(TIMSK)
        TIMSK = saved_timsk;
        #endif
        #if defined(TIMSK0)
        TIMSK0 = saved_timsk0;
        #endif
        #if defined(TIMSK1)
        TIMSK1 = saved_timsk1;
        #endif
        #if defined(TIMSK2)
        TIMSK2 = saved_timsk2;
        #endif
        #if defined(TIMSK3)
        TIMSK3 = saved_timsk3;
        #endif
        #if defined(TIMSK4)
        TIMSK4 = saved_timsk4;
        #endif
        #if defined(TIMSK5)
        TIMSK5 = saved_timsk5;
        #endif

        if (ss->pcint_mask_reg) {
            *(ss->pcint_mask_reg) |= ss->pcint_mask_value;
        }
    }
}

//=============================================================================
// Interrupt handlers
//=============================================================================

void software_serial_handle_interrupt(void) {
    if (active_serial_instance != NULL) {
        software_serial_recv(active_serial_instance);
    }
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect) {
    software_serial_handle_interrupt();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect) {
    software_serial_handle_interrupt();
}
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect) {
    software_serial_handle_interrupt();
}
#endif

//=============================================================================
// Initialisation
//=============================================================================

void software_serial_init(SoftwareSerial_t *ss,
                         uint8_t digital_pin_rx,
                         uint8_t digital_pin_tx,
                         bool inverse_logic) {
    AVR_Port_t rx_port, tx_port;
    uint8_t rx_pin, tx_pin;
    SS_Port_t ss_rx_port, ss_tx_port;
    
    avr_digital_to_pin(digital_pin_rx, &rx_port, &rx_pin);
    avr_digital_to_pin(digital_pin_tx, &tx_port, &tx_pin);
    
    // Convert AVR_Port_t to SS_Port_t properly
    ss_rx_port = avr_port_to_ss_port(rx_port);
    ss_tx_port = avr_port_to_ss_port(tx_port);

    ss->rx_pin = rx_pin;
    ss->rx_port = ss_rx_port;  // Store SS_Port_t, not AVR_Port_t
    ss->tx_pin = tx_pin;
    ss->tx_port = ss_tx_port;  // Store SS_Port_t, not AVR_Port_t
    ss->inverse_logic = inverse_logic;
    ss->buffer_overflow = false;
    ss->is_listening = false;
    ss->initialized = true;
    ss->rx_buffer_head = 0;
    ss->rx_buffer_tail = 0;

    ss->rx_bit_mask = (1 << rx_pin);
    ss->rx_port_in = get_port_in(ss_rx_port);  // Use SS_Port_t

    volatile uint8_t *rx_ddr = get_ddr(ss_rx_port);  // Use SS_Port_t
    *rx_ddr &= ~(ss->rx_bit_mask);

    volatile uint8_t *rx_port_out = get_port_out(ss_rx_port);  // Use SS_Port_t
    if (!inverse_logic) {
        *rx_port_out |= ss->rx_bit_mask;   // pull‑up
    } else {
        *rx_port_out &= ~(ss->rx_bit_mask);
    }

    if (tx_pin <= 7) {
        ss->tx_bit_mask = (1 << tx_pin);
        ss->tx_port_out = get_port_out(ss_tx_port);  // Use SS_Port_t
        ss->tx_ddr = get_ddr(ss_tx_port);  // Use SS_Port_t
        *(ss->tx_ddr) |= ss->tx_bit_mask;
        if (!inverse_logic) {
            *(ss->tx_port_out) |= ss->tx_bit_mask;   // idle high
        } else {
            *(ss->tx_port_out) &= ~(ss->tx_bit_mask);
        }
    } else {
        ss->tx_port_out = NULL;
        ss->tx_ddr = NULL;
        ss->tx_bit_mask = 0;
    }

    ss->pcint_mask_reg = get_pcmsk(ss_rx_port);  // Use SS_Port_t
    ss->pcint_mask_value = (1 << rx_pin);
    ss->pcint_vector = get_pcint_bit(ss_rx_port);  // Use SS_Port_t
}

//=============================================================================
// Begin – calculates delays robustly with baud rate compensation
//=============================================================================

void software_serial_begin(SoftwareSerial_t *ss, uint32_t baud_rate) {
    if (!ss->initialized) return;

    uint16_t bit_delay = (F_CPU / baud_rate) / 4;

    // 15 cycles TX overhead (matches Arduino SoftwareSerial tuning)
    ss->tx_delay = (bit_delay > 3) ? (bit_delay - 15 / 4) : 1;

    if (ss->rx_pin <= 7) {
        #if GCC_VERSION > 40800
        // GCC 4.8+: 4 + 4 + 75 cycles to first delay, 17 to pin read, 23 loop overhead
        ss->rx_delay_centering = (bit_delay / 2 > (4 + 4 + 75 + 17 - 23) / 4)
                                  ? (bit_delay / 2 - (4 + 4 + 75 + 17 - 23) / 4) : 1;
        ss->rx_delay_intrabit  = (bit_delay > 23 / 4)
                                  ? (bit_delay - 23 / 4) : 1;
        // Re-arm happens during stop bit; 37 cycles post-last-bit, 11 to re-enable
        // Target 3/4 bit so re-enable lands at 1/4 through stop bit
        ss->rx_delay_stopbit   = (bit_delay * 3 / 4 > (37 + 11) / 4)
                                  ? (bit_delay * 3 / 4 - (37 + 11) / 4) : 1;
        #else
        // GCC 4.3: slower codegen, higher overhead
        ss->rx_delay_centering = (bit_delay / 2 > (4 + 4 + 97 + 29 - 11) / 4)
                                  ? (bit_delay / 2 - (4 + 4 + 97 + 29 - 11) / 4) : 1;
        ss->rx_delay_intrabit  = (bit_delay > 11 / 4)
                                  ? (bit_delay - 11 / 4) : 1;
        ss->rx_delay_stopbit   = (bit_delay * 3 / 4 > (44 + 17) / 4)
                                  ? (bit_delay * 3 / 4 - (44 + 17) / 4) : 1;
        #endif

        //PCICR |= (1 << ss->pcint_vector);
        // Enable pin change interrupt - different registers for ATtiny vs ATmega
        #if defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny25__)
            // ATtiny uses GIMSK with PCIE bit
            GIMSK |= (1 << PCIE);
        #else
            // ATmega uses PCICR with PCIE0, PCIE1, PCIE2 bits
            PCICR |= (1 << ss->pcint_vector);
        #endif
        *(ss->pcint_mask_reg) |= ss->pcint_mask_value;

        // Establish idle state if TX pin was low
        _delay_loop_2(ss->tx_delay);
    }

    software_serial_listen(ss);
}

//=============================================================================
// Listen / stop listening
//=============================================================================

bool software_serial_listen(SoftwareSerial_t *ss) {
    if (!ss->initialized || ss->rx_pin > 7) return false;

    if (active_serial_instance != ss) {
        if (active_serial_instance != NULL) {
            software_serial_stop_listening(active_serial_instance);
        }
        ss->rx_buffer_head = 0;
        ss->rx_buffer_tail = 0;
        ss->buffer_overflow = false;
        active_serial_instance = ss;
        ss->is_listening = true;
        *(ss->pcint_mask_reg) |= ss->pcint_mask_value;
        return true;
    }
    return false;
}

bool software_serial_stop_listening(SoftwareSerial_t *ss) {
    if (active_serial_instance == ss) {
        if (ss->pcint_mask_reg) {
            *(ss->pcint_mask_reg) &= ~(ss->pcint_mask_value);
        }
        ss->is_listening = false;
        active_serial_instance = NULL;
        return true;
    }
    return false;
}

bool software_serial_is_listening(SoftwareSerial_t *ss) {
    return (active_serial_instance == ss);
}

//=============================================================================
// End, available, read, peek, overflow, flush
//=============================================================================

void software_serial_end(SoftwareSerial_t *ss) {
    software_serial_stop_listening(ss);
    ss->rx_buffer_head = 0;
    ss->rx_buffer_tail = 0;
    ss->buffer_overflow = false;
    ss->initialized = false;
}

int software_serial_available(SoftwareSerial_t *ss) {
    if (!software_serial_is_listening(ss)) return 0;
    return (SS_RX_BUFFER_SIZE + ss->rx_buffer_head - ss->rx_buffer_tail) % SS_RX_BUFFER_SIZE;
}

uint8_t software_serial_read(SoftwareSerial_t *ss) {
    if (!software_serial_is_listening(ss)) return 0;
    if (ss->rx_buffer_head == ss->rx_buffer_tail) return 0;
    uint8_t data = ss->rx_buffer[ss->rx_buffer_tail];
    ss->rx_buffer_tail = (ss->rx_buffer_tail + 1) % SS_RX_BUFFER_SIZE;
    return data;
}

int16_t software_serial_peek(SoftwareSerial_t *ss) {
    if (!software_serial_is_listening(ss)) return -1;
    if (ss->rx_buffer_head == ss->rx_buffer_tail) return -1;
    return (int16_t)ss->rx_buffer[ss->rx_buffer_tail];
}

bool software_serial_overflow(SoftwareSerial_t *ss) {
    bool ret = ss->buffer_overflow;
    if (ret) ss->buffer_overflow = false;
    return ret;
}

void software_serial_flush(SoftwareSerial_t *ss) {
    ss->rx_buffer_head = 0;
    ss->rx_buffer_tail = 0;
    ss->buffer_overflow = false;
}

//=============================================================================
// TX – optimized with interrupt management and compensation
//=============================================================================

void software_serial_write(SoftwareSerial_t *ss, uint8_t data) {
    if (ss->tx_delay == 0 || ss->tx_port_out == NULL) return;

    volatile uint8_t *port_out = ss->tx_port_out;
    uint8_t bit_mask = ss->tx_bit_mask;
    uint8_t inv_bit_mask = ~bit_mask;
    bool inv = ss->inverse_logic;
    uint16_t delay = ss->tx_delay;

    if (inv) data = ~data;

    uint8_t old_sreg = SREG;
    cli();

    // Start bit
    if (inv) *port_out |= bit_mask;
    else *port_out &= inv_bit_mask;
    _delay_loop_2(delay);

    // Data bits
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x01) *port_out |= bit_mask;
        else *port_out &= inv_bit_mask;
        _delay_loop_2(delay);
        data >>= 1;
    }

    // Stop bit
    if (inv) *port_out &= inv_bit_mask;
    else *port_out |= bit_mask;

    SREG = old_sreg;
    _delay_loop_2(delay / 2);  // extra guard
}