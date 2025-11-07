#include <xc.h>
#include "sysconfig.h"
#include "usb_cdc_lib.h"
#include "main.h"
#include <string.h>
#include <stdlib.h>

#pragma config FOSC = HSPLL_HS
#pragma config PLLDIV = 2
#pragma config CPUDIV = OSC1_PLL2
#pragma config USBDIV = 2
#pragma config WDT = OFF, LVP = OFF, MCLRE = ON

#define _XTAL_FREQ 48000000

#define LED_FLAP LATCbits.LATC0
#define SEG_PORT LATD
#define DIGIT0 LATAbits.LATA0  // DIS0 (dizaines)
#define DIGIT1 LATAbits.LATA1  // DIS1 (unités)

// --- Ultrason (HC-SR04) ---
#define ULTRA_TRIG LATCbits.LATC1
#define ULTRA_ECHO PORTBbits.RB1

// --- Prototypes ---
void sendFlap(void);
unsigned char readButtonE0Asm(void);
void checkUSBReceive(void);
void updateDisplay(void);
void setScore(unsigned char s);
void resetScore(void);
void buzzer_beep(unsigned short duration);
void updateBuzzer(void);

unsigned int readUltrasonicAsm(void);
void sendUltrasonicUSB(unsigned int duration_units);

// --- Variables ---
volatile unsigned char result;
unsigned char display_digits[2];
char usb_buffer[64];
unsigned char score = 0;

// --- Buzzer ---
volatile unsigned short buzzer_timer = 0;
unsigned short buzzer_toggle_count = 0;

volatile unsigned int ultra_duration = 0; // durée mesurée

const unsigned char SEGMENTS[10] = {
    0x3F,0x06,0x5B,0x4F,0x66,
    0x6D,0x7D,0x07,0x7F,0x6F
};

// --- Initialisation ---
void main(void) {
    TRISC = 0x00; LATC = 0x00;
    TRISD = 0x00; LATD = 0x00;
    TRISA = 0x00; LATA = 0x00;
    TRISEbits.TRISE0 = 1; // bouton
    TRISEbits.TRISE1 = 0; // buzzer

    TRISCbits.TRISC1 = 0; // TRIG
    TRISBbits.TRISB1 = 1; // ECHO
    LATCbits.LATC1 = 0;

    ADCON1 = 0x0F;
    CMCON = 0x07;

    initUSBLib();

    unsigned char last_btn_state = 1;
    resetScore();

    // Test buzzer au démarrage
    LATEbits.LATE1 = 1;
    __delay_ms(200);
    LATEbits.LATE1 = 0;

    unsigned short ultra_tick = 0;

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

            if(++ultra_tick >= 10) {
                ultra_tick = 0;
                unsigned int dur = readUltrasonicAsm();
                sendUltrasonicUSB(dur);
            }
        }

        updateDisplay();
        updateBuzzer();
    }
}

// --- USB ---
void checkUSBReceive(void) {
    static char cmd_buffer[64];
    static unsigned char cmd_index = 0;
    uint8_t len = getsUSBUSART((uint8_t*)usb_buffer, sizeof(usb_buffer));
    if(len > 0) {
        for(uint8_t i=0;i<len;i++) {
            char c = usb_buffer[i];
            if(c == '\n' || c == '\r') {
                cmd_buffer[cmd_index] = '\0';
                cmd_index = 0;

                if(strncmp(cmd_buffer,"BUF:SCORE=",10)==0){
                    unsigned char val = (unsigned char)atoi(&cmd_buffer[10]);
                    if(val<=99){
                        setScore(val);
                        buzzer_beep(80);
                    }
                }
                else if(strncmp(cmd_buffer,"BUF:GAMEOVER=1",14)==0){
                    buzzer_beep(500);
                }
                else if(strcmp(cmd_buffer,"R")==0){
                    resetScore();
                }
                cmd_buffer[0]='\0';
            } else {
                if(cmd_index < sizeof(cmd_buffer)-1)
                    cmd_buffer[cmd_index++] = c;
                else
                    cmd_index = 0;
            }
        }
    }
    CDCTxService();
}

void setScore(unsigned char s) {
    score = s;
    display_digits[0] = score / 10;
    display_digits[1] = score % 10;
}

void resetScore(void){
    score=0;
    display_digits[0]=0;
    display_digits[1]=0;
}

// --- Display ---
void updateDisplay(void){
    static unsigned char toggle=0;
    if(toggle==0){
        DIGIT0=0;
        SEG_PORT=SEGMENTS[display_digits[0]];
        DIGIT1=1;
    } else {
        DIGIT1=0;
        SEG_PORT=SEGMENTS[display_digits[1]];
        DIGIT0=1;
    }
    toggle = !toggle;
    for(volatile int i=0;i<500;i++);
}

// --- Flap ---
void sendFlap(void){
    char msg[]="FLAP\r\n";
    putUSBUSART((uint8_t*)msg,sizeof(msg)-1);
    CDCTxService();
}

// --- Bouton ---
unsigned char readButtonE0Asm(void){
    result = 1;
    asm("btfss PORTE,0\n clrf _result\n");
    return result;
}

// --- Buzzer ---
void buzzer_beep(unsigned short duration_ms){
    buzzer_timer = duration_ms*2;
}

void updateBuzzer(void){
    if(buzzer_timer>0){
        if(++buzzer_toggle_count>=10){
            LATEbits.LATE1 = !LATEbits.LATE1;
            buzzer_toggle_count=0;
        }
        buzzer_timer--;
    } else {
        LATEbits.LATE1=0;
    }
}

// --- Ultrason ---
unsigned int readUltrasonicAsm(void){
    ultra_duration=0;

    ULTRA_TRIG=1;
    __delay_us(1);
    ULTRA_TRIG=0;

    // Attente front montant
    asm(
        "wait_echo_high:\n"
        "btfss PORTB,1\n"
        "goto wait_echo_high\n"
    );

    // Compte durée front haut
    asm(
        "clrf _ultra_duration\n"
        "clrf _ultra_duration+1\n"
        "count_echo:\n"
        "btfsc PORTB,1\n"
        "goto inc_and_loop\n"
        "goto end_count\n"
        "inc_and_loop:\n"
        "incf _ultra_duration,f\n"
        "btfsc STATUS,2\n"  // <-- Z flag corrigé
        "incf _ultra_duration+1,f\n"
        "goto count_echo\n"
        "end_count:\n"
    );

    return ultra_duration;
}

void sendUltrasonicUSB(unsigned int duration_units){
    unsigned int duration_us = duration_units;
    unsigned int cm = duration_us / 58;
    if(cm>500) cm=500;

    char msg[24];
    int len=0;
    unsigned int hundreds=cm/100;
    unsigned int tens=(cm/10)%10;
    unsigned int ones=cm%10;

    if(hundreds>0){
        msg[len++]='0'+(char)hundreds;
        msg[len++]='0'+(char)tens;
        msg[len++]='0'+(char)ones;
    } else if(tens>0){
        msg[len++]='0'+(char)tens;
        msg[len++]='0'+(char)ones;
    } else {
        msg[len++]='0'+(char)ones;
    }

    char out[24];
    int pos=0;
    out[pos++]='U'; out[pos++]='L'; out[pos++]='T';
    out[pos++]='R'; out[pos++]='A'; out[pos++]=':';
    for(int i=0;i<len;i++) out[pos++]=msg[i];
    out[pos++]='c'; out[pos++]='m';
    out[pos++]='\r'; out[pos++]='\n';

    putUSBUSART((uint8_t*)out,pos);
    CDCTxService();
}

// --- Interruption USB ---
void __interrupt() mainISR(void){
    processUSBTasks();
}
