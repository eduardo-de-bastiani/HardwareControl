#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include "register.h"
#include <string.h>

#define FILE_PATH "registers.bin"
#define FILE_SIZE 1024
#define NUM_REGISTERS 16
#define MAX_STRING_LENGHT 1024

unsigned short *reg[NUM_REGISTERS];
unsigned short *base_address = NULL;

char* registers_map(const char* file_path, int file_size, int* fd) {
    *fd = open(file_path, O_RDWR | O_CREAT, 0666);
    if (*fd == -1) {
        perror("Error opening or creating file");
        return NULL;
    }

    if (ftruncate(*fd, file_size) == -1) {
        perror("Error setting file size");
        close(*fd);
        return NULL;
    }

    char *map = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (map == MAP_FAILED) {
        perror("Error mapping file");
        close(*fd);
        return NULL;
    }

    base_address = (unsigned short *)map;

    for (int i = 0; i < NUM_REGISTERS; i++){
        reg[i] = (unsigned short *)(base_address + i);
    }

    return map;
}

int registers_release(void* map, int file_size, int fd) {
    if (munmap(map, file_size) == -1) {
        perror("Error unmapping the file");
        close(fd);
        return -1;
    }

    if (close(fd) == -1) {
        perror("Error closing file");
        return -1;
    }

    return 0;
}

//  FUNÇÕES DO R0
uint16_t getDisplayOn() {return *reg[0] & 0x01;}
const char* convertedGetDisplayOn(){
    int displayOn = getDisplayOn();

    if (displayOn == 1) {
        return "Display On";
    }
    return "Display Off";
}
void setDisplayOn(int v) {
    if(v > 1 || v < 0) return;
    *reg[0] = v ? (*reg[0] | 0x01) : (*reg[0] & ~0x01);
}

uint16_t getDisplayMode() {return (*reg[0] & 0x0006) >> 1;}

const char* convertedGetDisplayMode(){
    int displayMode = getDisplayMode();

    switch (displayMode) {
        case 0b00: // Modo estático
            return "Static";
            break;
        case 0b01: // Modo deslizante
            return "Sliding";
            break;
        case 0b10: // Modo intermitente
            return "Flashing";
            break;
        case 0b11: // Modo deslizante e intermitente
            return "Sliding and Flashing";
            break;
        default:
            break;
    }
}
void setDisplayMode(int mode) {
    if(mode > 4 || mode < 1) return;

    *reg[0] &= ~(0x3 << 1);

    switch (mode) {
        case 1: 
            *reg[0] |= (0x0 << 1);
            break;
        case 2:
            *reg[0] |= (0x1 << 1);
            break;
        case 3:
            *reg[0] |= (0x2 << 1);
            break;
        case 4:
            *reg[0] |= (0x3 << 1);
            break;
    }
}

uint16_t getDisplaySpeed() {return (*reg[0] >> 3) & 0x3F;}
const char* convertedGetDisplaySpeed(){
    int displaySpeed = getDisplaySpeed();

    static char displaySpeedString[7];

    snprintf(displaySpeedString, sizeof(displaySpeedString), "%d ms", displaySpeed * 100);

    return displaySpeedString;
}
void setDisplaySpeed(int speed) {
    if (speed < 0 || speed > 63) return;
    *reg[0] &= ~(0x3F << 3);
    *reg[0] |= (speed & 0x3F) << 3;
}

uint16_t getOperationLedOn() {return (*reg[0] >> 9) & 0x01;}
const char* convertedGetOperationLedOn(){
    int operationLed = getOperationLedOn();

    if(operationLed == 1){
        return "LED On";
    }
    return "LED Off";
}
void setOperationLedOn(int v) {
    if (v > 1 || v < 0) return;
    *reg[0] &= ~0x0200;
    *reg[0] |= (v << 9) & 0x0200;
}

uint16_t getStatusLedColor() {return (*reg[0] >> 10) & 0x07;}
const char* convertedGetStatusLedColor(){
    int statusColor = getStatusLedColor();

    switch (statusColor)
    {
    case 2:
        return "Verde [High/Medium]";
        break;
    case 4:
        return "Vermelho [Critical]";
        break;
    case 6:
        return "Amarelo [Low]";
        break;
    default:
        return "Unknown"; 
        break;
    }

}
void setStatusLedColor(int *r, int *g, int *b) {
    if(*r>1 || *r<0 || *g>1 || *g<0 || *b>1 || *b<0) return;

    *reg[0] &= ~(0x07 << 10);
    *reg[0] |= (*b > 0.5) << 10;
    *reg[0] |= (*g > 0.5) << 11;
    *reg[0] |= (*r > 0.5) << 12;  
}

void resetRegisters() {
    *reg[0] |= (1 << 13);
}

//  FUNÇÕES DO R1/R2

void getDisplayColor(uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = *reg[1] & 0xFF;
    *g = (*reg[1] >> 8) & 0xFF;
    *b = *reg[2] & 0xFF;
}
void setDisplayColor(uint8_t r, uint8_t g, uint8_t b) {
    if(r > 255 || r < 0 || g > 255 || g < 0 || b > 255 || b < 0) return;
    *reg[1] = r & 0xFF;
    *reg[1] |= (g << 8) & 0xFF00;
    *reg[2] = b & 0xFF;
}

//  FUNÇÕES DO R3

uint16_t getBatteryLevel(){return *reg[3] & 0x3;}

const char* convertedGetBatteryLevel() {
    int batteryLevel = getBatteryLevel();
    const char* batteryString;

    switch (batteryLevel)
    {
    case 0:
        batteryString = "critical";
        break;
    case 1:
        batteryString = "low";
        break;
    case 2:
        batteryString = "medium";
        break;
    case 3:
        batteryString = "high";
        break;
    default:
        break;
    }

    return batteryString;
}

void setBatteryLevel(uint16_t batteryLevel) {
    *reg[3] &= 0xFFFC;       
    *reg[3] |= (batteryLevel & 0x3); 

    int r, g, b;
    int level = getBatteryLevel();
    switch (level) {
        case 0:
            r = 1;
            g = 0;
            b = 0;
            break;
        case 1:
        case 2:
            r = 1;
            g = 1;
            b = 0;
            break;
        case 3:
            r = 0;
            g = 1;
            b = 0;
            break;
        default:
            return;
    }

    setStatusLedColor(&r, &g, &b);
}

float getTemperature(){
    uint16_t maskedValue = (*reg[3] & 0xFFC0) >> 6;

    int signBit = maskedValue & 0x0200;

    int16_t temperature = (int16_t)(maskedValue & 0x01FF);

    if (signBit){
       temperature |= 0xFE00;
    }
    
    float tempFloat = (float)temperature/10;

    return tempFloat;
}

void setTemperature(int temperature){ 
    int16_t tempValue;

    if( temperature >= 0 && temperature <= 999)
        tempValue = (int16_t)(temperature);
    else {   // Temperatura negativa
        tempValue = (int16_t)(-temperature);
        tempValue = ~tempValue + 1; // Complemento de dois [inverte os bits e soma 1]
        tempValue |= 0x8000;
    }

    *reg[3] &= ~(0xFFC0); 

    *reg[3] |= (((uint16_t)tempValue) << 6) & 0xFFC0;
}

uint16_t getDisplayCount() {return (*reg[3] >> 2) & 0xF;}

//  FUNÇÕES DO R4-R15

void clearInputBuffer(){
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
}

void setDisplayString(const char *msg) {
    int length = strlen(msg);  
    resetDisplayString();

    for (int i = 0; i < length; i++) {
        int reg_index = 4 + (i / 2);

        if (i % 2 == 0) {
            *reg[reg_index] = (unsigned short)msg[i];
        } else{
            *reg[reg_index] |= (unsigned short)(msg[i] << 8);
        }
    }
} 

void resetDisplayString() {
    memset(reg[4], 0, 12 * sizeof(unsigned short));
}
