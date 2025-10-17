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

// Définition des ports
#define LED LATCbits.LATC0

// Prototypes
void debounceDelay(void);
void sendFlap(void);
unsigned char readButtonAsm(void); // Lecture bouton en ASM inline

void main(void) {
    // Initialisation des ports
    TRISC = 0x00;           // Port C en sortie
    LATC = 0x00;            // LEDs éteintes
    TRISDbits.TRISD0 = 1;   // Bouton en entrée

    // Initialisation USB
    initUSBLib();

    while(1) {
        // Gérer les tâches USB
        USBDeviceTasks();

        // Si USB est prêt
        if (isUSBReady()) {
            unsigned char btn_state = readButtonAsm();  // Lecture du bouton
            if (btn_state) {
                LED = 1;        // Allume LED
                sendFlap();     // Envoie "flap"
                debounceDelay();
                while(readButtonAsm()); // Attente relâchement
                LED = 0;        // Éteint LED
            }
        }
    }
}

// Fonction qui envoie "flap\r\n" via USB
void sendFlap(void) {
    char msg[] = "FLAP\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService(); // S'assurer que les données sont transmises
}

unsigned char result;

unsigned char readButtonAsm(void) {
    asm(
        "movlw 0x01\n"      // result = 0 par défaut
        "btfss PORTD, 0\n"     // skip next if RD0 = 1         // bouton pressé ? 1
        "movlw 0x00\n" 
        "return\n"
       
       
        // Bouton non pressé ? result = 0
    );
    
}

// Simple délai anti-rebond
void debounceDelay(void) {
    for (volatile int i=0; i<255; i++)
        for (volatile int j=0; j<255; j++);
}

// Interruption USB (optionnelle, selon stack)
void __interrupt() mainISR(void) {
    processUSBTasks();
}
