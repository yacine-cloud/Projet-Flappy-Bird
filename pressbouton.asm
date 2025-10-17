INCLUDE <P18F4550.INC>

    CONFIG FOSC = HS          ; Oscillateur haute vitesse
    CONFIG WDT = OFF          ; Watchdog désactivé
    CONFIG MCLRE = ON         ; MCLR activé
    CONFIG DEBUG = OFF
    CONFIG CPUDIV = OSC1_PLL2 ; Division CPU
    CONFIG PBADEN = OFF       ; PORTB en I/O digital

    CBLOCK 0x20
        count1          ; registre pour le délai externe
        count2          ; registre pour le délai interne
    ENDC

RES_VECT CODE 0x0000
    GOTO INIT

;======================
;  SECTION PRINCIPALE
;======================
MAIN_PROG CODE

INIT:
    CLRF PORTC          ; Mets PORTC à 0
    CLRF LATC           ; Valeur logique à 0
    CLRF TRISC          ; Configure PORTC en sortie (LEDs)
    BSF  TRISD, 0       ; RD0 configuré en entrée (bouton)
    GOTO MAIN

MAIN:
MAIN_LOOP:
    BTFSS PORTD, 0      ; Teste le bit RD0 : 1 = bouton appuyé ?
    GOTO NO_PRESS      


    BSF LATC, 0         ; Allume LED RC0
    CALL DEBOUNCE_DELAY 

WAIT_RELEASE:
    BTFSC PORTD, 0      ; Attends que le bouton soit relâché
    GOTO WAIT_RELEASE

    BCF LATC, 0         ; Éteint la LED
    GOTO MAIN_LOOP

NO_PRESS:
    BCF LATC, 0         ; Assure que la LED reste éteinte
    GOTO MAIN_LOOP

DEBOUNCE_DELAY:
    MOVLW D'255'
    MOVWF count1

DELAY_LOOP1:
    MOVLW D'255'
    MOVWF count2

DELAY_LOOP2:
    NOP
    DECFSZ count2, F
    GOTO DELAY_LOOP2

    DECFSZ count1, F
    GOTO DELAY_LOOP1

    RETURN

    END
