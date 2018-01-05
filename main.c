/*
    FreeRTOS V8.0.1 - Copyright (C) 2014 Real Time Engineers Ltd.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public 
    License and the FreeRTOS license exception along with FreeRTOS; if not it 
    can be viewed here: http://www.freertos.org/a00114.html and also obtained 
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/
#include <stdlib.h>
#include <time.h>

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_ssp.h"
#include "lpc17xx_i2c.h"
#include "lpc17xx_timer.h"

#include "joystick.h"
#include "oled.h"
#include "led7seg.h"
#include "pca9532.h"

/* FreeRTOS.org includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo includes. */
#include "basic_io.h"

static void init_i2c(void){
	PINSEL_CFG_Type PinCfg; // initialize a pin configuration structure
	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2; // set the pin to the #2 function -> I2C pin
	PinCfg.Portnum = 0; // pin in port number #0
	PinCfg.Pinnum = 10; // the #10 pin in that port number
	PINSEL_ConfigPin(&PinCfg); // config that pin
	PinCfg.Pinnum = 11; // the #11 pin in that port number
	PINSEL_ConfigPin(&PinCfg); // config that pin
	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000); // set I2C function to have 100Khz clockrate
	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE); // enable/activate the I2C function
}

static void init_ssp(void){
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	// Initialize SPI pin connect
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;  // P0.7 - SCK;
	PINSEL_ConfigPin(&PinCfg);  PinCfg.Pinnum = 8;  // P0.8 - MISO
	PINSEL_ConfigPin(&PinCfg);  PinCfg.Pinnum = 9;  // P0.9 - MOSI
	PINSEL_ConfigPin(&PinCfg);  PinCfg.Funcnum = 0;  PinCfg.Portnum = 2;  PinCfg.Pinnum = 2;  //P2.2 - SSEL - used as GPIO
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct); // Initialize SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);   // Enable SSP peripheral }
}

/* The task function. */
void vTaskFunction1( void *pvParameters );
void vTaskFunction2( void *pvParameters );
void vTaskFunction3( void *pvParameters );
void vTaskFunction4( void *pvParameters );

/* Define the strings that will be passed in as the task parameters.  These are
defined const and off the stack to ensure they remain valid when the tasks are
executing. */
const char *pcTextForTask1 = "Task 1 is running\n";
const char *pcTextForTask2 = "Task 2 is running\n";
const char *pcTextForTask3 = "Task 3 is running\n";
const char *pcTextForTask4 = "Task 4 is running\n";

const char *welcome = "WELCOME!";
const char *congrats = "CONGRATS!";
const char *gameover = "GAME OVER!";
const char *press2play = "Press to Play";
const char *again = "Again";

static uint8_t goldPos = 5;
static uint8_t cursorPos = 4;
static uint8_t polisiPos = 0;
static uint8_t select;
static uint8_t play = 1;
static uint8_t nyawa = 3;
static uint8_t skor = 0;
static uint8_t level = 0;
/*-----------------------------------------------------------*/

int main( void )
{
	srand(time(NULL));   // should only be called once
	init_ssp(); // initialize SPI function
	init_i2c();
	pca9532_init();

	GPIO_SetDir(1, 1<<18, 1); // P1.18 set as Output
	GPIO_ClearValue(1, 1<<18); // P1.18 set LOW

	/* Create the first task at priority 1... */
	xTaskCreate( vTaskFunction1, "Task 1", 240, (void*)pcTextForTask1, 1, NULL );

	/* ... and the second task at priority 2.  The priority is the second to
	last parameter. */
	xTaskCreate( vTaskFunction2, "Task 2", 240, (void*)pcTextForTask2, 1, NULL );

	xTaskCreate( vTaskFunction3, "Task 3", 240, (void*)pcTextForTask3, 1, NULL );
	xTaskCreate( vTaskFunction4, "Task 4", 240, (void*)pcTextForTask4, 1, NULL );
	/* Start the scheduler so our tasks start executing. */
	vTaskStartScheduler();	
	
	/* If all is well we will never reach here as the scheduler will now be
	running.  If we do reach here then it is likely that there was insufficient
	heap available for the idle task to be created. */
	for( ;; );
	return 0;
}
/*-----------------------------------------------------------*/

void vTaskFunction1( void *pvParameters )
{
	char *pcTaskName;
	portTickType xLastWakeTime;

	/* The string to print out is passed in via the parameter.  Cast this to a
	character pointer. */
	pcTaskName = ( char * ) pvParameters;

	/* The xLastWakeTime variable needs to be initialized with the current tick
	count.  Note that this is the only time we access this variable.  From this
	point on xLastWakeTime is managed automatically by the vTaskDelayUntil()
	API function. */
	xLastWakeTime = xTaskGetTickCount();

	oled_init();

	oled_clearScreen(OLED_COLOR_BLACK);
	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{

		if (level == 0){
			oled_clearScreen(OLED_COLOR_BLACK);
			oled_putString(10,10,(uint8_t*)welcome, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			vTaskDelayUntil( &xLastWakeTime, ( 3000 / portTICK_RATE_MS ) );
//			oled_putString(10,20,(uint8_t*)press2play, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			/* Delay */
//			vTaskDelayUntil( &xLastWakeTime, ( 3000 / portTICK_RATE_MS ) );
			level = 8;
		}
		else if (level == 6){
			oled_clearScreen(OLED_COLOR_BLACK);
			oled_putString(10,10,(uint8_t*)congrats, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
//			vTaskDelayUntil( &xLastWakeTime, ( 2000 / portTICK_RATE_MS ) );
	//		oled_putString(10,20,(uint8_t*)press2play, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			/* Delay */
			vTaskDelayUntil( &xLastWakeTime, ( 3000 / portTICK_RATE_MS ) );
			level = 8;
		}
		else if (level == 7){
			oled_clearScreen(OLED_COLOR_BLACK);
			oled_putString(10,10,(uint8_t*)gameover, OLED_COLOR_WHITE, OLED_COLOR_BLACK);
//			vTaskDelayUntil( &xLastWakeTime, ( 2000 / portTICK_RATE_MS ) );
//			oled_putString(10,20,(uint8_t*)press2play, OLED_COLOR_WHITE, OLED_COLOR_BLACK);

			/* Delay */
			vTaskDelayUntil( &xLastWakeTime, ( 3000 / portTICK_RATE_MS ) );
			level = 8;
		} else if (level == 8){
			vTaskDelayUntil( &xLastWakeTime, ( 100 / portTICK_RATE_MS ) );
		}
		else {

			oled_clearScreen(OLED_COLOR_BLACK);
			/* Inisialisasi */
			oled_line(32, 0, 32, 64, OLED_COLOR_WHITE);
			oled_line(64, 0, 64, 64, OLED_COLOR_WHITE);
			oled_line(0, 21, 96, 21, OLED_COLOR_WHITE);
			oled_line(0, 42, 96, 42, OLED_COLOR_WHITE);

			/* Posisi Cursor */
			oled_rect((32*(cursorPos%3) + 2), (21*((cursorPos/3)+1) - 19), (32*(cursorPos%3) + 30), (21*((cursorPos/3)+1) - 2), OLED_COLOR_WHITE);
			/* Posisi Gold */
			oled_circle((32*(goldPos%3) + 16), 21*(goldPos/3) + 10, 5, OLED_COLOR_WHITE);

			/* Posisi Polisi */
			oled_line((32*(polisiPos%3) + 11), (21*(polisiPos/3) + 5), (32*(polisiPos%3) + 21), (21*(polisiPos/3) + 15), OLED_COLOR_WHITE);
			oled_line((32*(polisiPos%3) + 11), (21*(polisiPos/3) + 15), (32*(polisiPos%3) + 21), (21*(polisiPos/3) + 5), OLED_COLOR_WHITE);

			/* Delay */
			vTaskDelayUntil( &xLastWakeTime, ( 100 / portTICK_RATE_MS ) );
		}
	}
}

void vTaskFunction2( void *pvParameters )
{
	uint8_t state = 0;

	char *pcTaskName;
	portTickType xLastWakeTime;

	/* The string to print out is passed in via the parameter.  Cast this to a
	character pointer. */
	pcTaskName = ( char * ) pvParameters;

	/* The xLastWakeTime variable needs to be initialized with the current tick
	count.  Note that this is the only time we access this variable.  From this
	point on xLastWakeTime is managed automatically by the vTaskDelayUntil()
	API function. */
	xLastWakeTime = xTaskGetTickCount();

	joystick_init();

	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{

		state = joystick_read();
		// Dipencet
		if (state == 0x01){
			select = 1;
		// Atas
		} else if(state == 0x02){
			if (cursorPos > 2) {
				cursorPos -= 3;
				}
		// Bawah
		} else if(state == 0x04){
			if (cursorPos < 6) {
				cursorPos += 3;
				}
		// Kiri
		} else if(state == 0x08){
			if (cursorPos % 3 != 0) {
				cursorPos -= 1;
			}
		// Kanan
		} else if(state == 0x10){
			if (cursorPos % 3 != 2) {
				cursorPos += 1;
				}
		}

		if(skor == 0) {
			pca9532_setLeds(0x0000, 0xff00);
		} else if(skor == 1) {
			pca9532_setLeds(0x8000, 0xff00);
		} else if(skor == 2) {
			pca9532_setLeds(0xC000, 0xff00);
		} else if(skor == 3) {
			pca9532_setLeds(0xE000, 0xff00);
		} else if(skor == 4) {
			pca9532_setLeds(0xF000, 0xff00);
		} else if(skor == 5) {
			pca9532_setLeds(0xF800, 0xff00);
		} else if(skor == 6) {
			pca9532_setLeds(0xFC00, 0xff00);
		} else if(skor == 7) {
			pca9532_setLeds(0xFE00, 0xff00);
		} else if(skor == 8) {
			pca9532_setLeds(0xFF00, 0xff00);
		}
		vTaskDelayUntil( &xLastWakeTime, ( 90 / portTICK_RATE_MS ) );
	}
}
/*-----------------------------------------------------------*/
void vTaskFunction3( void *pvParameters )
{

	char *pcTaskName;
	portTickType xLastWakeTime;

	/* The string to print out is passed in via the parameter.  Cast this to a
	character pointer. */
	pcTaskName = ( char * ) pvParameters;

	/* The xLastWakeTime variable needs to be initialized with the current tick
	count.  Note that this is the only time we access this variable.  From this
	point on xLastWakeTime is managed automatically by the vTaskDelayUntil()
	API function. */
	xLastWakeTime = xTaskGetTickCount();

	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{
		goldPos = rand()%9;      // returns a pseudo-random integer between 0 and RAND_MAX
		polisiPos = rand()%9;      // returns a pseudo-random integer between 0 and RAND_MAX
		while(polisiPos == goldPos){
			polisiPos = rand()%9;
		}
		if (level == 1){
			vTaskDelayUntil( &xLastWakeTime, ( 2000 / portTICK_RATE_MS ) );
		} else if (level == 2){
			vTaskDelayUntil( &xLastWakeTime, ( 1500 / portTICK_RATE_MS ) );
		} else if (level == 3){
			vTaskDelayUntil( &xLastWakeTime, ( 1000 / portTICK_RATE_MS ) );
		} else if (level == 4){
			vTaskDelayUntil( &xLastWakeTime, ( 500 / portTICK_RATE_MS ) );
		} else if (level == 5){
			vTaskDelayUntil( &xLastWakeTime, ( 250 / portTICK_RATE_MS ) );
		}
	}
}
/*-----------------------------------------------------------*/
void vTaskFunction4( void *pvParameters )
{

	char *pcTaskName;
	portTickType xLastWakeTime;

	/* The string to print out is passed in via the parameter.  Cast this to a
	character pointer. */
	pcTaskName = ( char * ) pvParameters;

	/* The xLastWakeTime variable needs to be initialized with the current tick
	count.  Note that this is the only time we access this variable.  From this
	point on xLastWakeTime is managed automatically by the vTaskDelayUntil()
	API function. */
	xLastWakeTime = xTaskGetTickCount();

	led7seg_init();

	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{
		GPIO_SetValue(1, 1<<18); // P1.18 set HIGH
		if (select == 1) {
			if (level == 8)
			{
				nyawa = 3;
				skor = 0;
				level = 1;
			} /*else if (level == 6){
				nyawa = 3;
				level = 1;
			} else if (level == 7){
				nyawa = 3;
				level = 1;
			} */
			else {
				if(cursorPos == goldPos){
					skor++;
				}
				else if (cursorPos == polisiPos){
					nyawa--;
				}
			}
			select = 0;
		}
		if (nyawa == 0){
			skor = 0;
			level = 7;
			nyawa = 3;
		}
		if (skor == 8){
			if (level <= 5){
				level++;
			}
			skor = 0;
		}

		if ((level != 0) &&(level != 6)&&(level != 7)&&(level != 8)){
			led7seg_setChar(('0' + level), FALSE);
		} else {
			led7seg_setChar(('0'), FALSE);
		}


		if ((level != 0) &&(level != 6)&&(level != 7)&&(level != 8)){
			if(nyawa == 3){
				pca9532_setLeds(0x0007, 0x000f);
			} else if (nyawa == 2){
				pca9532_setLeds(0x0003, 0x000f);
			} else if (nyawa == 1){
				pca9532_setLeds(0x0001, 0x000f);
			} else if (nyawa == 0){
				pca9532_setLeds(0x0000, 0x000f);
			}
		}
		GPIO_ClearValue(1, 1<<18); // P1.18 set LOW

		vTaskDelayUntil( &xLastWakeTime, ( 150 / portTICK_RATE_MS ) );
	}
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* This function will only be called if an API call to create a task, queue
	or semaphore fails because there is too little heap RAM remaining. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( xTaskHandle *pxTask, signed char *pcTaskName )
{
	/* This function will only be called if a task overflows its stack.  Note
	that stack overflow checking does slow down the context switch
	implementation. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* This example does not use the idle hook to perform any processing. */
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
	/* This example does not use the tick hook to perform any processing. */
}



