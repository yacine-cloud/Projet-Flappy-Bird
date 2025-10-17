#include <xc.h>
#include "sysconfig.h"
#include "usb_cdc_lib.h"
#include "main.h"

// Définition des ports
#define LED LATCbits.LATC0
#define BUTTON PORTDbits.RD0

// Prototypes
void debounceDelay(void);
void sendFlap(void);

void main(void) {
    // Initialisation des ports
    TRISC = 0x00;    // Port C en sortie
    LATC = 0x00;     // LEDs éteintes
    TRISDbits.TRISD0 = 1; // Bouton en entrée

    // Initialisation USB
    initUSBLib();

    while(1) {
        USBDeviceTasks();  // Gérer les tâches USB

        // Si USB est prêt
        if (isUSBReady()) {
            if (BUTTON) {
                LED = 1;       // Allume LED
                sendFlap();    // Envoie le mot "flap"
                debounceDelay();
                while(BUTTON); // Attente relâchement
                LED = 0;       // Éteint LED
            }
        }
    }
}

// Fonction qui envoie "flap\r\n" via USB
void sendFlap(void) {
    char msg[] = "flap\r\n";
    putUSBUSART((uint8_t*)msg, sizeof(msg)-1);
    CDCTxService(); // S'assurer que les données sont transmises
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
