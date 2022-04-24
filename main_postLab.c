/*
 * File:   main_postLab.c
 * Author: javyp
 * Potenciómetro en RA0/AN0 Y RA1/AN1 que funcionan como entrada analógica y mediante 
 * módulo ADC se pasa ADRESH al PORTC donde hay un DAC y PORTD que indica el voltaje según cada potenciómetro.
 * 
 * Created on 23 de abril de 2022, 09:07 AM
 */

// PIC16F887 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>

/*
 * CONSTANTES
 */
#define _XTAL_FREQ 4000000
#define _tmr0_value 217 //TMR0 a 10 ms

/*
 * Variables y arreglos
 */
uint8_t banderas; //Bandera para selector
int conv; //Variable para conversión de ADRESH
int val_div[3] = {0,0,0}; //Arreglo que almacenará unidades, decenas y centenas
uint8_t val_disp[3] = {0,0,0}; //Arreglo que almacenará unidades, decenas y centenas traducidas al display de 7 segmentos
//Arreglo para configuración 7 segmentos, donde la posición en el arreglo es el número que representa
uint8_t tabla_7seg [10] = {0b10111111, 0b10000110,0b11011011,0b11001111,0b11100110,0b11101101,0b11111101,0b10000111,0b11111111,0b11101111};

/*
 * PROTOTIPO DE FUNCIÓN
 */
void setup(void);
void division(int CONTEO); //FUNCIÓN PARA DIVISIÓN EN UNIDADES, DECENAS Y CENTENAS
void set_displays(int CONTEO_U, int CONTEO_D, int CONTEO_C); //TRADUCCIÓN DE CONTEO A 7 SEGMENTOS

void __interrupt() isr(void){
    //Se revisa interrupción ADC
    if (PIR1bits.ADIF){
        if (ADCON0bits.CHS == 0){ //Se verifica canal AN0        
            PORTC = ADRESH; //Se muestran los 8 bits superiores en PORTC
        }
        else if (ADCON0bits.CHS == 1){ //Se verifica canal AN1        
            conv = 100*ADRESH/51; //Se hace la conversión de 0-255 a 0-500 (0 a 5 v)
            division(conv); //Se pasa el valor de la conversión a la función de división
            set_displays(val_div[0],val_div[1],val_div[2]); //Traducción unidades, decenas y centenas
        }
        PIR1bits.ADIF = 0; // Limpiamos bandera ADC
    }
    //Se revisa interrupción TMR0
    if (INTCONbits.T0IF){ 
       PORTE = 0; //Limpieza selector
       if(banderas == 0){        
           PORTD = val_disp[0];   // Seteo de valor de unidades traducido 
           PORTE = 0b00000001;   // 3er selector
           banderas = 1; 
       }
       else if(banderas == 1){        
           PORTD = val_disp[1];  // Seteo de valor de decenas traducido
           PORTE = 0b00000010;  // 2do selector
           banderas = 2; 
       }
       else if(banderas == 2){        
           PORTD = val_disp[2];  // Seteo de valor de centenas traducido
           PORTE = 0b00000100;  // 3er selector
           banderas = 0; 
       }
       
       INTCONbits.T0IF = 0; //Limpieza de bandera
       TMR0 = _tmr0_value;  //Reinicio de TMR0
    }
    return;
}

void main(void) {  
   
    setup(); // Se pasa a configurar PIC
        
    while(1)
    {
       if(ADCON0bits.GO == 0){ // Si no hay proceso de conversión
           //Es necesario el cambio de canales
           if(ADCON0bits.CHS == 0b0000)    
                ADCON0bits.CHS = 0b0001;    // Cambio a AN1
           else if(ADCON0bits.CHS == 0b0001)
                ADCON0bits.CHS = 0b0000;    // Cambio a AN0
           
            __delay_us(40); //Sample time
            ADCON0bits.GO = 1; // Se inicia proceso de conversión
        } 
    }
}

void setup(void){
    ANSEL = 0b00000011; //Se configura PORTA0/AN0 & PORTA1/AN1 como entrada analógica
    ANSELH = 0;   //I/O DIGITALES
    
    OSCCONbits.IRCF = 0b0110;   //Oscilador interno de 4 MHz
    OSCCONbits.SCS = 1;         //Oscilador interno
    
    TRISA = 0b00000011; //PORTA0/AN0 & PORTA1/AN1 como INPUT    
    PORTA = 0;    //CLEAR DE PUERTO A  
    
    TRISC = 0; //PORTC como OUTPUT
    PORTC = 0; //CLEAR DE PUERTO C
    TRISD = 0; //PORTD como OUTPUT
    PORTD = 0; //CLEAR DE PUERTO D
    TRISE = 0; //PORTE como OUTPUT
    PORTE = 0; //CLEAR DE PUERTO E
    
    //Config ADC
    ADCON0bits.ADCS = 0b01; // Fosc/8
    ADCON1bits.VCFG0 = 0;  // Referencia VDD
    ADCON1bits.VCFG1 = 0;  // Referencia VSS
    ADCON0bits.CHS = 0b0000; // Inicialmente se selecciona PORTA0/AN0
    ADCON1bits.ADFM = 0; // Se indica que se tendrá un justificado a la izquierda
    ADCON0bits.ADON = 1; // Se habilita el modulo ADC
    __delay_us(40);     // Delay para sample time
    
    //Timer0 Registers Prescaler= 256 - TMR0 Preset = 217 - Freq = 100.16 Hz - Period = 0.009984 seconds
    OPTION_REGbits.T0CS = 0;  // bit 5  TMR0 Clock Source Select bit...0 = Internal Clock (CLKO) 1 = Transition on T0CKI pin
    OPTION_REGbits.PSA = 0;   // bit 3  Prescaler Assignment bit...0 = Prescaler is assigned to the Timer0
    OPTION_REGbits.PS2 = 1;   // bits 2-0  PS2:PS0: Prescaler Rate Select bits
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS0 = 1;
    TMR0 = _tmr0_value;       // preset for timer register a 10 ms
    
    //Config interrupciones
    INTCONbits.GIE = 1; //Se habilitan interrupciones globales
    PIE1bits.ADIE = 1;  //Se habilita interrupcion del ADC
    INTCONbits.T0IE = 1; // Se habilita interrupciones del TMR0
    PIR1bits.ADIF = 0; // Limpieza de bandera del ADC
    INTCONbits.T0IF = 0; // Limpieza de bandera del TMR0
    INTCONbits.PEIE = 1; // Se habilitan interrupciones de periféricos
 }

void division(int CONTEO){
    val_div[2] = CONTEO/100; // División para obtención de Centenas
    val_div[1] = (CONTEO-val_div[2]*100)/10; // Operación para obtención de Decenas
    val_div[0] = CONTEO-val_div[2]*100-val_div[1]*10; // Operación para obtención de Unidades
    return;
}

void set_displays(int CONTEO_U, int CONTEO_D, int CONTEO_C){
    val_disp[0] = tabla_7seg[CONTEO_U]; //Traducción de unidades
    val_disp[1] = tabla_7seg[CONTEO_D]; //Traducción de decenas
    val_disp[2] = tabla_7seg[CONTEO_C]; //Traducción de centenas
    return;
}
