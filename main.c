#include <cmsis_os2.h>
#include "random.h"
#include <stdint.h>
#include <stdio.h>
#include "GLCD.h"

// Create gloabl varibales for queue information
uint32_t q1Size = 0;
uint32_t q1Sent = 0;
uint32_t q1Received = 0;
uint32_t q1Discarded = 0;

uint32_t q2Size = 0;
uint32_t q2Sent = 0;
uint32_t q2Received = 0;
uint32_t q2Discarded = 0;

uint32_t elapsedTime = 0;
uint32_t totalSent = 0;

osMessageQueueId_t q_id1;
osMessageQueueId_t q_id2;

osMutexId_t q1Mutex;
osMutexId_t q2Mutex;



int msg = 69;
//nice


// Function to convert random number to ticks
uint32_t getRandomTickCount(uint32_t avgFreq) {
    uint32_t randInt = next_event();
    uint32_t tickCount = randInt * osKernelGetTickFreq();
    return ((tickCount / avgFreq) >> 16);
}

// Create Server Threads (Receive Mesages)
void serverThread1(void *args) {
	while(1) {
		// Wait to access q1
		osMutexAcquire(q2Mutex, osWaitForever);	
		
		// Delay for random tick count
		osDelay(getRandomTickCount(10));
		
		// Process a message from q1
		osMessageQueueGet(q_id1, &msg, NULL, osWaitForever);
		osMutexRelease(q2Mutex);
		q1Received++;
		osThreadYield();
	}
}

void serverThread2(void * args) {
	while(1) {
		// Wait to access q2
		osMutexAcquire(q1Mutex, osWaitForever);	
		
		// Delay for random tick count
		osDelay(getRandomTickCount(10));
		
		// Process a message from q2
		osMessageQueueGet(q_id2, &msg, NULL, osWaitForever);
		osMutexRelease(q1Mutex);
		q2Received++;
		osThreadYield();
	}
}

// Create Client Thread (Send Messages)
void clientThread(void * args){
	while(1) {
		osStatus_t additionStatus;
		if(totalSent % 2) {
			osMutexAcquire(q1Mutex, osWaitForever);	
			osDelay(getRandomTickCount(9));			
			additionStatus = osMessageQueuePut(q_id1, &msg, NULL, 0);
			osMutexRelease(q1Mutex);
			
			if (additionStatus == osOK)
				q1Sent++;
			else {
				q1Discarded++;
			}
			totalSent++;
		} else {
			osMutexAcquire(q2Mutex, osWaitForever);
			osDelay(getRandomTickCount(9));
			additionStatus = osMessageQueuePut(q_id2, &msg, NULL, 0);
			osMutexRelease(q2Mutex);
			if (additionStatus == osOK)
				q2Sent++;
			else {
				q2Discarded++;

			}
			totalSent++;
		}
		osThreadYield();
	}
}

// Create Monitor Thread (Monitor Progress and send data)
void monitorThread(void * args) {
	
	while(1) {
		// Delay 1s
		osDelay(osKernelGetTickFreq());
		elapsedTime++;
		GLCD_SetBackColor(0x001F);
		GLCD_SetTextColor(0xFFFF);
    
		// Build Strings
    char buffer1 [50];
    char buffer2 [50];
    char buffer3 [50];
    char buffer4 [50];
    unsigned char bufferTitle [] = "    q1    q2";
		
    // Build strings into character buffers to be printed
    int stringSent = sprintf(buffer1, "sent: %i  %i", q1Sent, q2Sent);
    int stringReceived = sprintf(buffer2, "received: %i  %i", q1Received, q2Received);
    int stringOver = sprintf(buffer3, "over: %i  %i", q1Discarded, q2Discarded);
    int stringTime = sprintf(buffer4, "time: %i", elapsedTime);		
		
		// Display
		GLCD_DisplayString(1, 1, 1, bufferTitle);
    GLCD_DisplayString(2, 1, 1, (unsigned char *)buffer1);
    GLCD_DisplayString(3, 1, 1, (unsigned char *)buffer2);
    GLCD_DisplayString(4, 1, 1, (unsigned char *)buffer3);
    GLCD_DisplayString(6, 1, 1, (unsigned char *)buffer4);
		osThreadYield();
	}
}


// Make Queues
int main(void){
    // Initialize the LCD
    GLCD_Init();
		GLCD_Clear(Blue);


    // Start Kernel
		osKernelInitialize();
		q_id1 = osMessageQueueNew(10, sizeof(int), NULL);
		q_id2 = osMessageQueueNew(10, sizeof(int), NULL);
	
		q1Mutex = osMutexNew(NULL);
		q2Mutex = osMutexNew(NULL);
	
    //osThreadNew(clientThread, NULL, NULL);
		osThreadNew(clientThread, NULL, NULL);
    osThreadNew(serverThread1, NULL, NULL);
    osThreadNew(serverThread2, NULL, NULL);
    osThreadNew(monitorThread, NULL, NULL);
		
		osKernelStart();
		for(;;){};
}

