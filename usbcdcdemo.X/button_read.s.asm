    LIST P=18F4550
    #include <p18F4550.inc>

    GLOBAL _readButton

_readButton:
    btfss PORTD, 0
    movlw 0x00
    return
    movlw 0x01
    return
