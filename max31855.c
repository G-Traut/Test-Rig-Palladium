#include "max31855.h"

void MAX31855_ReadData(MAX31855_Handle *dev) {
    uint8_t rx[4] = {0};

    // 1. Pull CS Low to start SPI transfer
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
    
    // 2. Read 4 bytes (32 bits) from the sensor
    HAL_SPI_Receive(dev->hspi, rx, 4, 100);
    
    // 3. Pull CS High to end transfer
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);

    // 4. Combine bytes into a 32-bit integer
    uint32_t raw = (rx[0] << 24) | (rx[1] << 16) | (rx[2] << 8) | rx[3];

    // 5. Check the Fault Bit (Bit 16)
    dev->fault = (raw >> 16) & 0x01;

    // 6. Extract Thermocouple Temperature (Bits 31 to 18)
    // We shift right by 18 and cast to int16_t to preserve the sign bit
    int16_t temp_raw = (int16_t)(raw >> 18);
    if (temp_raw & 0x2000) { // If 14-bit sign bit is set
        temp_raw |= 0xC000;  // Manually sign-extend to 16-bit
    }
    dev->thermocouple_temp = temp_raw * 0.25f;

    // 7. Extract Internal Chip Temperature (Bits 15 to 4)
    int16_t internal_raw = (int16_t)((raw >> 4) & 0x0FFF);
    if (internal_raw & 0x0800) { // If 12-bit sign bit is set
        internal_raw |= 0xF000;  // Manually sign-extend to 16-bit
    }
    dev->internal_temp = internal_raw * 0.0625f;
}