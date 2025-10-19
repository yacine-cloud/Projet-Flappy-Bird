GLOBAL _ReadButton     ; Rendre la fonction visible depuis le C

_ReadButton:
    BTFSS PORTD, 0     ; Si RD0 = 0, bouton pas appuyé
    GOTO not_pressed
    MOVLW 0x01         ; Retourne 1 si appuyé
    RETURN
not_pressed:
    CLRW                ; Retourne 0 si non appuyé
    RETURN