#include <xc.h>
#include "sysconfig.h"
#include "usb_cdc_lib.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>

// --- Config PIC ---
#pragma config FOSC = HSPLL_HS
#pragma config PLLDIV = 2
#pragma config CPUDIV = OSC1_PLL2
#pragma config USBDIV = 2
#pragma config WDT = OFF, LVP = OFF, MCLRE = ON

#define LED_FLAP LATCbits.LATC0
#define SEG_PORT LATD
#define DIGIT0 LATAbits.LATA0  // DIS0 (dizaines)
#define DIGIT1 LATAbits.LATA1  // DIS1 (unités)

// --- Prototypes ---
void sendFlap(void);
unsigned char readButtonE0Asm(void);
void checkUSBReceive(void);
void updateDisplay(void);
void setScore(unsigned char s);
void resetScore(void);
void buzzer_beep(unsigned short duration);
void updateBuzzer(void);

// --- Variables ---
volatile unsigned char result;
unsigned char display_digits[2];
char usb_buffer[64];
unsigned char score = 0;
unsigned char last_score = 0;     // pour détecter la montée de score
unsigned char game_over_flag = 0; // état du game over

// --- Buzzer ---
volatile unsigned short buzzer_timer = 0;
volatile unsigned char buzzer_state = 0;
unsigned short buzzer_toggle_count = 0;

const unsigned char SEGMENTS[10] = {
    0x3F,0x06,0x5B,0x4F,0x66,
    0x6D,0x7D,0x07,0x7F,0x6F
};

// --- Initialisation ---
void main(void) {
    TRISC = 0x00; LATC = 0x00;
    TRISD = 0x00; LATD = 0x00;
    TRISA = 0x00; LATA = 0x00;
    TRISEbits.TRISE0 = 1; // bouton en entrée
    TRISEbits.TRISE1 = 0; // buzzer en sortie

    ADCON1 = 0x0F;
    CMCON = 0x07;

    initUSBLib();

    unsigned char last_btn_state = 1;

    resetScore();  // Score à 0 au démarrage

    // Petit test au démarrage
    LATEbits.LATE1 = 1;
    __delay_ms(200);
    LATEbits.LATE1 = 0;

    while(1) {
        USBDeviceTasks();

        if(isUSBReady()) {
            unsigned char btn_state = readButtonE0Asm();

            if(btn_state == 0 && last_btn_state == 1) {
                LED_FLAP = 1;
                sendFlap();
            }
            if(btn_state == 1 && last_btn_state == 0) {
                LED_FLAP = 0;
                sendFlap();
            }
            last_btn_state = btn_state;

            checkUSBReceive();
        }

        updateDisplay();  // multiplexage 7 segments
        updateBuzzer();   // mise à jour du buzzer
    }
}

// --- Lecture USB et traitement commandes ---
void checkUSBReceive(void) {
    static char cmd_buffer[64];
    static unsigned char cmd_index = 0;

    uint8_t len = getsUSBUSART((uint8_t*)usb_buffer, sizeof(usb_buffer));
    if (len > 0) {
        for (uint8_t i = 0; i < len; i++) {
            char c = usb_buffer[i];

            if (c == '\n' || c == '\r') {
                cmd_buffer[cmd_index] = '\0';
                cmd_index = 0;

                // --- TRAITEMENT DE LA COMMANDE ---
                if (strncmp(cmd_buffer, "BUF:SCORE=", 10) == 0) {
                    unsigned char val = (unsigned char)atoi(&cmd_buffer[10]);
                    if (val <= 99) {
                        setScore(val);
                        buzzer_beep(80);  // bip court à chaque score
                    }
                }
                else if (strncmp(cmd_buffer, "BUF:GAMEOVER=1", 14) == 0) {
                    buzzer_beep(500);   // bip long à la fin
                }
                else if (strcmp(cmd_buffer, "R") == 0) {
                    resetScore();
                }

                cmd_buffer[0] = '\0';
            } else {
                if (cmd_index < sizeof(cmd_buffer) - 1)
                    cmd_buffer[cmd_index++] = c;
                else
                    cmd_index = 0; // overflow -> reset
            }
        }
    }
    CDCTxService(); // Important : vider le buffer TX/USB à chaque boucle
}

// --- Mise à jour score pour affichage ---
void setScore(unsigned char s) {
    score = s;
    display_digits[0] = score / 10;  // dizaines
    display_digits[1] = score % 10;  // unités
}

// --- Multiplexage 7 segments ---
void updateDisplay(void) {
    static unsigned char toggle = 0;

    if(toggle == 0) {
        DIGIT0 = 0;
        SEG_PORT = SEGMENTS[display_digits[0]];
        DIGIT1 = 1;
    } else {
        DIGIT1 = 0;
        SEG_PORT = SEGMENTS[display_digits[1]];
        DIGIT0 = 1;
    }

    toggle = !toggle;
    for(volatile int i=0;i<500;i++); // pause multiplexage
}

void resetScore(void) {
    score = 0;
    display_digits[0] = 0;
    display_digits[1] = 0;
}

// --- Envoi USB FLAP ---
void sendFlap(void) {
    char msg[] = "FLAP\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService();
}

// --- Lecture bouton RE0 en ASM ---
unsigned char readButtonE0Asm(void) {
    result = 1;
    asm("btfss PORTE, 0\n clrf _result\n");
    return result;
}

// --- Buzzer non-bloquant (~2 kHz) ---
void buzzer_beep(unsigned short duration_ms) {
    buzzer_timer = duration_ms * 2; // ajusté pour 2kHz
}

void updateBuzzer(void) {
    static unsigned short last_tick = 0;
    if (buzzer_timer > 0) {
        // toggle rapide pour produire un son (~2 kHz)
        if (++buzzer_toggle_count >= 10) {
            LATEbits.LATE1 = !LATEbits.LATE1; // ✅ si buzzer sur RE1
            buzzer_toggle_count = 0;
        }
        // décrémentation sans bloquer
        buzzer_timer--;
    } else {
        LATEbits.LATE1 = 0;  // silence
    }
}

// --- Interruption USB ---
void __interrupt() mainISR(void) {
    processUSBTasks();
}
