
#include <xc.h>
#include "sysconfig.h"
#include "usb_cdc_lib.h"
#include "main.h"

#pragma config FOSC = HSPLL_HS
#pragma config PLLDIV = 2
#pragma config CPUDIV = OSC1_PLL2
#pragma config USBDIV = 2
#pragma config WDT = OFF, LVP = OFF, MCLRE = ON
// retire cette ligne : #pragma config PBADEN = OFF

// Définition des LEDs
#define LED_FLAP      LATCbits.LATC0
#define LED_PAUSE     LATCbits.LATC1
#define LED_REPRENDRE LATCbits.LATC2
#define LED_QUITTER_SET()   (LATC |= 0x08)   // RC3 à 1
#define LED_QUITTER_CLEAR() (LATC &= ~0x08)  // RC3 à 0

// Prototypes
void debounceDelay(void);
void sendFlap(void);
void sendPause(void);
void sendReprendre(void);
void sendQuitter(void);
unsigned char readButton0Asm(void);
unsigned char readButton1Asm(void);
unsigned char readButton2Asm(void);
unsigned char readButton3Asm(void);

volatile unsigned char result;

void main(void) {
    // Initialisation des ports
    TRISC = 0x00;           // Port C en sortie
    LATC = 0x00;            // LEDs éteintes
    TRISDbits.TRISD0 = 1;   // Bouton FLAP
    TRISDbits.TRISD1 = 1;   // Bouton PAUSE
    TRISDbits.TRISD2 = 1;   // Bouton REPRENDRE
    TRISDbits.TRISD3 = 1;   // Bouton QUITTER

    // Initialisation USB
    initUSBLib();

    unsigned char last_btn0_state = 1;
    unsigned char last_btn1_state = 1;
    unsigned char last_btn2_state = 1;
    unsigned char last_btn3_state = 1;

    while(1) {
        USBDeviceTasks();

        if (isUSBReady()) {
            unsigned char btn0_state = readButton0Asm();
            unsigned char btn1_state = readButton1Asm();
            unsigned char btn2_state = readButton2Asm();
            unsigned char btn3_state = readButton3Asm();

            // --- Bouton FLAP (RD0) ---
            if (btn0_state == 0 && last_btn0_state == 1) {
                LED_FLAP = 1;
                sendFlap();
            }
            if (btn0_state == 1 && last_btn0_state == 0) {
                LED_FLAP = 0;
                sendFlap();
            }
            last_btn0_state = btn0_state;

            // --- Bouton PAUSE (RD1) ---
            if (btn1_state == 0 && last_btn1_state == 1) {
                LED_PAUSE = 1;
                sendPause();
            }
            if (btn1_state == 1 && last_btn1_state == 0) {
                LED_PAUSE = 0;
                sendPause();
            }
            last_btn1_state = btn1_state;

            // --- Bouton REPRENDRE (RD2) ---
            if (btn2_state == 0 && last_btn2_state == 1) {
                LED_REPRENDRE = 1;
                sendReprendre();
            }
            if (btn2_state == 1 && last_btn2_state == 0) {
                LED_REPRENDRE = 0;
                sendReprendre();
            }
            last_btn2_state = btn2_state;

            // --- Bouton QUITTER (RD3) ---
            if (btn3_state == 0 && last_btn3_state == 1) {
                LED_QUITTER_SET();
                sendQuitter();
            }
            if (btn3_state == 1 && last_btn3_state == 0) {
                LED_QUITTER_CLEAR();
                sendQuitter();
            }
            last_btn3_state = btn3_state;
        }
    }
}

// --- Envoi des messages ---
void sendFlap(void) {
    char msg[] = "FLAP\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService();
}

void sendPause(void) {
    char msg[] = "PAUSE\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService();
}

void sendReprendre(void) {
    char msg[] = "REPRENDRE\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService();
}

void sendQuitter(void) {
    char msg[] = "QUITTER\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService();
}

// --- Lecture des boutons en ASM ---
unsigned char readButton0Asm(void) {
    result = 1;
    asm(
        "btfss PORTD, 0\n"
        "clrf _result\n"
    );
    return result;
}

unsigned char readButton1Asm(void) {
    result = 1;
    asm(
        "btfss PORTD, 1\n"
        "clrf _result\n"
    );
    return result;
}

unsigned char readButton2Asm(void) {
    result = 1;
    asm(
        "btfss PORTD, 2\n"
        "clrf _result\n"
    );
    return result;
}

unsigned char readButton3Asm(void) {
    result = 1;
    asm(
        "btfss PORTD, 3\n"
        "clrf _result\n"
    );
    return result;
}

// Délai anti-rebond
void debounceDelay(void) {
    for (volatile int i=0; i<255; i++)
        for (volatile int j=0; j<255; j++);
}

// Interruption USB
void __interrupt() mainISR(void) {
    processUSBTasks();
}
