// Inclusión de librerías estándar para el microcontrolador y funciones de cadena/entrada-salida
#include <xc.h>         // Librería específica para el compilador XC8 de Microchip
#include <string.h>     // Para funciones de manejo de cadenas (strcpy, strstr, etc.)
#include <stdio.h>      // Para funciones de entrada/salida estándar (no se usa directamente aquí)

// Configuración de los bits de configuración del PIC16F877A
#pragma config FOSC = HS        // Selecciona el oscilador de alta velocidad (High Speed)
#pragma config WDTE = OFF       // Desactiva el Watchdog Timer (temporizador de vigilancia)
#pragma config PWRTE = ON       // Activa el Power-up Timer para estabilidad al iniciar
#pragma config BOREN = ON       // Habilita el Brown-out Reset (reset por bajo voltaje)
#pragma config LVP = OFF        // Desactiva la programación en bajo voltaje
#pragma config CPD = OFF        // Desactiva la protección de la memoria EEPROM
#pragma config WRT = OFF        // Desactiva la protección de escritura de la memoria flash
#pragma config CP = OFF         // Desactiva la protección de código de la memoria flash

// Definición de la frecuencia del cristal oscilador (20MHz)
#define _XTAL_FREQ 20000000     // Necesario para las funciones de delay

// Definiciones para el control de LEDs
#define LED_GREEN RD0           // LED verde conectado al pin RD0
#define LED_RED RD1             // LED rojo conectado al pin RD1
#define LED_ON 1                // Valor para encender un LED (nivel alto)
#define LED_OFF 0               // Valor para apagar un LED (nivel bajo)

// Variables globales para el GPS
char gps_buffer[82];            // Buffer circular para almacenar datos NMEA recibidos
char latitude[15];              // Almacena la latitud en formato de cadena
char longitude[15];             // Almacena la longitud en formato de cadena
char ns_indicator, ew_indicator; // Indicadores Norte/Sur y Este/Oeste
unsigned char buffer_index = 0;  // Índice actual para escribir en gps_buffer
unsigned char gps_data_ready = 0; // Bandera que indica que hay una trama NMEA completa

// Prototipos de funciones
void UART_Init(void);           // Inicializa el módulo UART
void UART_WriteChar(char data); // Escribe un carácter por UART
void UART_WriteString(const char *str); // Escribe una cadena por UART
void Process_GPS_Data(void);    // Procesa los datos GPS recibidos

// Función principal
void main(void) {
    // Configuración de pines de LEDs como salidas
    TRISD0 = 0;  // Configura RD0 (LED verde) como salida
    TRISD1 = 0;  // Configura RD1 (LED rojo) como salida
    
    // Apaga ambos LEDs al iniciar
    LED_GREEN = LED_OFF;
    LED_RED = LED_OFF;
    
    // Inicializa el módulo UART para comunicación serial
    UART_Init();
    
    // Envia mensajes iniciales por UART
    UART_WriteString("\r\nGPS Coordinate Reader\r\n");      // Título del sistema
    UART_WriteString("PIC16F877A with NEO-6M GPS\r\n");    // Descripción del hardware
    UART_WriteString("Waiting for GPS data...\r\n\r\n");    // Mensaje de espera
    
    // Bucle principal infinito
    while(1) {
        // Si hay datos GPS listos para procesar
        if (gps_data_ready) {
            Process_GPS_Data();     // Procesa los datos GPS
            gps_data_ready = 0;     // Resetea la bandera
            buffer_index = 0;       // Reinicia el índice del buffer
            memset(gps_buffer, 0, sizeof(gps_buffer)); // Limpia el buffer
        }
        
        // Contador para el "latido" (heartbeat) del LED verde
        static unsigned int heartbeat_counter = 0;
        if (++heartbeat_counter >= 2000) {  // Cada 2000 iteraciones (~2 segundos)
            LED_GREEN = !LED_GREEN;         // Cambia estado del LED verde
            __delay_ms(20);                 // Pequeño retardo para el parpadeo
            LED_GREEN = !LED_GREEN;         // Vuelve al estado original
            heartbeat_counter = 0;          // Reinicia el contador
        }
    
        // Pequeño retardo para controlar la velocidad del bucle
        __delay_ms(1);
    }
}

// Función para inicializar el módulo UART
void UART_Init() {
    TRISC7 = 1;             // Configura RC7 (RX) como entrada
    TRISC6 = 0;             // Configura RC6 (TX) como salida
    
    // Configuración del baud rate (9600 bps a 20MHz)
    SPBRG = 129;            // Valor del registro para 9600 baudios
    BRGH = 1;               // Usa alta velocidad de baudios
    
    // Configuración del modo de operación
    SYNC = 0;               // Modo asíncrono
    SPEN = 1;               // Habilita el puerto serial
    CREN = 1;               // Habilita la recepción continua
    
    // Configuración de formato de datos
    TX9 = 0;                // Transmisión de 8 bits
    RX9 = 0;                // Recepción de 8 bits
    
    TXEN = 1;               // Habilita la transmisión
    
    // Configuración de interrupciones
    RCIE = 1;               // Habilita interrupción por recepción
    PEIE = 1;               // Habilita interrupciones periféricas
    GIE = 1;                // Habilita interrupciones globales
    
    // Verificación de la inicialización
    if (SPEN && CREN) {
        LED_GREEN = LED_ON;  // Breve parpadeo del LED verde para indicar éxito
        __delay_ms(100);
        LED_GREEN = LED_OFF;
    } else {
        LED_RED = LED_ON;    // Enciende LED rojo si hay error
        while(1);            // Bucle infinito (sistema detenido)
    }
}

// Función para escribir un carácter por UART
void UART_WriteChar(char data) {
    while(!TXIF);           // Espera hasta que el buffer de transmisión esté libre
    TXREG = data;           // Escribe el carácter en el registro de transmisión
}

// Función para escribir una cadena por UART
void UART_WriteString(const char *str) {
    while (*str) {          // Mientras no sea el carácter nulo
        UART_WriteChar(*str++); // Escribe cada carácter de la cadena
    }
}

// Rutina de servicio de interrupción (ISR)
void __interrupt() ISR(void) {
    if (RCIF) {             // Si hay una interrupción por recepción UART
        char received = RCREG; // Lee el carácter recibido
        
        // Verifica que no haya desbordamiento del buffer
        if (buffer_index < sizeof(gps_buffer) - 1) {
            gps_buffer[buffer_index++] = received; // Almacena el carácter
            
            // Detecta fin de trama NMEA (carácter de nueva línea)
            if (received == '\n') {
                gps_data_ready = 1; // Activa bandera de datos listos
            }
        }
        
        RCIF = 0;           // Limpia la bandera de interrupción
    }
}

// Función para procesar los datos GPS recibidos
void Process_GPS_Data() {
    // Busca la trama GPGGA (datos de posición global)
    if (strstr(gps_buffer, "$GPGGA")) {
        LED_GREEN = LED_ON;  // Enciende LED verde (datos válidos)
        LED_RED = LED_OFF;   // Apaga LED rojo
        
        char *token;         // Puntero para tokens
        int field = 0;       // Contador de campos
        
        // Divide la cadena usando comas como delimitadores
        token = strtok(gps_buffer, ",");
        
        // Procesa cada token (campo) de la trama NMEA
        while (token != NULL) {
            field++;         // Incrementa el contador de campos
            
            // Campo 2: Latitud (ej: "4124.8963")
            if (field == 2 && strlen(token) > 0) {
                strcpy(latitude, token); // Copia la latitud
            }
            
            // Campo 3: Indicador N/S (Norte o Sur)
            if (field == 3 && strlen(token) > 0) {
                ns_indicator = token[0]; // Almacena N o S
            }
            
            // Campo 4: Longitud (ej: "08151.6838")
            if (field == 4 && strlen(token) > 0) {
                strcpy(longitude, token); // Copia la longitud
            }
            
            // Campo 5: Indicador E/W (Este u Oeste)
            if (field == 5 && strlen(token) > 0) {
                ew_indicator = token[0]; // Almacena E o W
            }
            
            token = strtok(NULL, ","); // Obtiene el siguiente token
        }
        
        // Muestra las coordenadas formateadas por UART
        UART_WriteString("GPS Coordinates:\r\n");
        
        // Formatea y muestra la latitud (ej: "41° 24.8963' N")
        if (strlen(latitude) >= 4) {
            UART_WriteString("Latitude: ");
            // Grados (primeros dos caracteres)
            UART_WriteChar(latitude[0]);
            UART_WriteChar(latitude[1]);
            UART_WriteString("° ");
            // Minutos (siguientes dos caracteres)
            UART_WriteChar(latitude[2]);
            UART_WriteChar(latitude[3]);
            UART_WriteString(".");
            // Minutos decimales (resto de la cadena)
            UART_WriteString(&latitude[4]);
            UART_WriteString("' ");
            // Indicador N/S
            UART_WriteChar(ns_indicator);
            UART_WriteString("\r\n");
        }
        
        // Formatea y muestra la longitud (ej: "081° 51.6838' W")
        if (strlen(longitude) >= 5) {
            UART_WriteString("Longitude: ");
            // Grados (primeros tres caracteres)
            UART_WriteChar(longitude[0]);
            UART_WriteChar(longitude[1]);
            UART_WriteChar(longitude[2]);
            UART_WriteString("° ");
            // Minutos (siguientes dos caracteres)
            UART_WriteChar(longitude[3]);
            UART_WriteChar(longitude[4]);
            UART_WriteString(".");
            // Minutos decimales (resto de la cadena)
            UART_WriteString(&longitude[5]);
            UART_WriteString("' ");
            // Indicador E/W
            UART_WriteChar(ew_indicator);
            UART_WriteString("\r\n\r\n");
        }
    }
}