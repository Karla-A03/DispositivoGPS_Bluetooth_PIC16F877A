# Proyecto: Lector de Coordenadas GPS con PIC16F877A y Envío por Bluetooth

## Descripción General

Este proyecto utiliza un microcontrolador **PIC16F877A** para recibir datos de un módulo GPS (**NEO-6M**), procesarlos y enviar las coordenadas de latitud y longitud a un dispositivo móvil a través de un módulo Bluetooth (**HC-05**).  
Inicialmente, la simulación se realizó en **Proteus** utilizando una **Virtual Terminal** para visualizar los datos. Posteriormente, se integró el módulo Bluetooth para su comunicación inalámbrica con una aplicación móvil desarrollada en **MIT App Inventor**.

## Justificación

Transmitir coordenadas GPS a dispositivos móviles es una funcionalidad base en aplicaciones de **rastreo de activos**, **localización de vehículos**, **control de drones**, **sistemas de geolocalización personal** o **proyectos de monitoreo de campo**.  
Este proyecto busca demostrar cómo, con **componentes de bajo costo** y **programación básica en C**, se puede diseñar un sistema funcional de recolección y envío de ubicación, abriendo la puerta a aplicaciones en **logística, seguridad o automatización**.

## Tecnologías y Hardware Utilizados

- Microcontrolador **PIC16F877A**
- Módulo GPS **NEO-6M**
- Módulo Bluetooth **HC-05**
- 2 LEDs (indicadores de estado)
- **Proteus 8** para simulación
- **MPLAB X IDE** + **XC8 Compiler**
- **MIT App Inventor** (para la app móvil)

## Estructura del Proyecto

- `main.c`: Código principal que recibe, procesa y envía datos GPS.
- `Proteus Simulation`: Archivo de simulación con PIC, GPS, Virtual Terminal y LEDs.
- `App móvil`: Aplicación sencilla en MIT App Inventor para recibir y mostrar las coordenadas.
