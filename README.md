## Test-Rig-Palladium
STM32 Code for the Test-Rig of the pNIPAm-Trials

### Goal is to develop a Test-Rig on basis of an STM32 Board (NUCLEO-F767ZI)
A Pump will be gesteuert via drv8876 single brushed DC Motor Driver Carrier. It can then be controlled through PWM Signals. 
Via K-Type Thermocouples a temperature detection is employed. 

### Connectivity: 

- Microcontroller: STM32 Nucleo-F767ZI (Cortex-M7)

- Sensor Interface: MAX31855 Thermocouple-to-Digital Converter

- Sensor: Type-K Thermocouple

- Protocol: SPI (Serial Peripheral Interface)

  ### Connection Diagram (Pin Mapping)

| Function | MAX31855 Pin | Nucleo-F767ZI Pin | STM32 Internal Pin |
| :--- | :--- | :--- | :--- |
| **Power** | Vin (3.3V - 5V) | 3.3V | VCC |
| **Ground** | GND | GND | GND |
| **SPI Clock** | CLK | D13 | PA5 |
| **SPI MISO** | DO (Data Out) | D12 | PA6 |
| **Chip Select** | CS | D10 | PD14 |

### Software & SPI Configuration
To ensure reliable data transmission with long jumper wires, the SPI is configured as follows:

- Baud Rate: Prescaler 256 (providing a stable clock speed < 1MHz).

- SPI Mode: Mode 0 (CPOL=0, CPHA=0) or Mode 1 (CPHA=1/2-Edge) depending on the specific breakout board manufacturer.

- Data Frame: 32-bit (read as 4 consecutive 8-bit bytes).

- NSS Mode: Disabled (Chip Select is handled manually via GPIO for better timing control). 

## 1. Requirements

- STM32CubeIDE or VS Code with the STM32 Extension.

- A Serial Terminal (e.g., PuTTY, Tera Term, or the integrated VS Code Serial Monitor).

## 2. Monitoring Data

To find the correct port on macOS/Linux:  ls /dev/tty.*

## 3. Understanding the Output

The code currently outputs RAW Hexadecimal values in the while(1) loop for debugging:
RAW: 01 A4 1C 00

Byte 0 & 1: Contain the 14-bit thermocouple temperature.

Bit 16 (Byte 2): Fault bit. If this is 1, a problem occurred.

Byte 2 & 3: Internal cold-junction temperature and specific fault codes (Short to VCC, Short to GND, or Open Circuit)

## 4. After completion of debugging (currently the only output generated is RAW: 00 00 00 00 or RAW: FF FF FF FF

### Detailed Explanation of the code main.c

## Include Section ( l. 17-22)
This section imports all required header files for the STM32 temperature measurement project.
### main.h
  Contains:

  STM32 hardware configuration
  GPIO definitions
  peripheral initialization
  project-wide declarations

### max31855.h

  Provides the driver functions for the MAX31855 thermocouple amplifier.

  Used for:

  reading thermocouple temperatures
  SPI communication with the sensor
  accessing sensor status and fault detection

### max6675.h

  Provides the driver functions for the MAX6675 thermocouple amplifier.

  Used for:

  SPI-based temperature acquisition
  reading K-type thermocouple data
  converting raw sensor values into temperatures

  This allows the project to support MAX6675 sensors in addition to MAX31855 modules.

## Multi-Channel MAX31855 Sensor Initialization (l.66-71)
```

/* USER CODE BEGIN 0 */
MAX31855_Handle htemp1; // First sensor (D10)
MAX31855_Handle htemp2; // Second sensor (D9)
MAX31855_Handle htemp3; // Third sensor (D11)
MAX31855_Handle htemp4; // Fourth sensor (D12)

Description:
This section initializes four independent MAX31855_Handle instances for a multi-channel thermocouple measurement system.
Each handle represents one MAX31855 thermocouple amplifier connected to the STM32 microcontroller via SPI.
The handles store all required communication and configuration parameters for each sensor.

Sensor Configuration
Handle	Pin	Description
htemp1	D10	Thermocouple channel 1
htemp2	D9	Thermocouple channel 2
htemp3	D11	Thermocouple channel 3
htemp4	D12	Thermocouple channel 4
```
# main(void)

This is the entry point of the program. It follows a specific sequence:

Initialization: It calls HAL_Init() to reset peripherals and SystemClock_Config() to set the CPU speed. It then initializes the GPIO, SPI, and UART peripherals.

### The Infinite Loop (while(1)): It manually pulls the Chip Select (CS) pin low to talk to the sensor, receives 4 bytes of data via SPI, pulls CS high again, and sends that raw data to the computer via UART for debugging.
```
MAX31855 SPI Sensor Configuration
htemp1.hspi = &hspi1;
htemp1.cs_port = GPIOD;
htemp1.cs_pin = GPIO_PIN_14; // D10

// Configuration Sensor 2
htemp2.hspi = &hspi1;
htemp2.cs_port = GPIOD;
htemp2.cs_pin = GPIO_PIN_15; // D9

// Configuration Sensor 3
htemp3.hspi = &hspi1;
htemp3.cs_port = GPIOD;
htemp3.cs_pin = GPIO_PIN_12; // D12

// Configuration Sensor 4
htemp4.hspi = &hspi1;
htemp4.cs_port = GPIOD;
htemp4.cs_pin = GPIO_PIN_11; // D11
```
Description

This section configures four MAX31855 thermocouple sensor handles for SPI communication with the STM32 microcontroller.

Each sensor uses:

the same SPI peripheral (hspi1)
an individual Chip Select (CS) pin

This allows multiple MAX31855 modules to operate on a shared SPI bus.

## Main Measurement Loop
``` while (1)
{
    // Read all sensors
    MAX31855_ReadData(&htemp1);
    MAX31855_ReadData(&htemp2);
    MAX31855_ReadData(&htemp3);
    MAX31855_ReadData(&htemp4); 

    char msg[128];
    
    // Simple fault detection
    if (htemp1.fault || htemp2.fault || htemp3.fault || htemp4.fault) {
        sprintf(msg, "Error: S1=%d, S2=%d, S3=%d, S4=%d\r\n",
                htemp1.fault,
                htemp2.fault,
                htemp3.fault,
                htemp4.fault);
    } else {

        // Individual temperature offset correction
        float t1 = htemp1.thermocouple_temp - 1.78f;
        float t2 = htemp2.thermocouple_temp - 1.91f;
        float t3 = htemp3.thermocouple_temp - 1.54f;
        float t4 = htemp4.thermocouple_temp - 1.07f;

        // Format: Counter; Temp1; Temp2; Temp3; Temp4
        int len = sprintf(msg,
                          "%lu; %.2f; %.2f; %.2f; %.2f\r\n",
                          log_counter++,
                          t1, t2, t3, t4);

        HAL_UART_Transmit(&huart3, (uint8_t*)msg, len, 100);
    }

    HAL_Delay(1000); 
} ```


 ### Description

  This loop is the core of the STM32-based temperature acquisition system.

  It continuously:

  -reads all thermocouple sensors
  - checks for sensor faults
  - applies calibration offsets
  - formats the measurement data
  - transmits the data via UART
  - repeats every second

### Sensor Data Acquisition

  Each MAX31855 module is read sequentially using:

  MAX31855_ReadData(...)
  
  The function retrieves:
  
  -thermocouple temperature
  -internal reference temperature
  -fault status
  
  for each sensor channel.

### Fault Detection

  The firmware performs a basic fault check:
  
  if (htemp1.fault || htemp2.fault || ...)
  
  If a fault is detected:
  
  an error message is generated
  sensor fault codes are transmitted over UART
  
  Example:
  
  Error: S1=0, S2=1, S3=0, S4=0
  
  This helps identify disconnected or malfunctioning thermocouples.

### Temperature Calibration

  Each sensor uses an individual offset correction:
  
  float t1 = htemp1.thermocouple_temp - 1.78f;
  
  This compensates for:
  
  sensor tolerances
  amplifier inaccuracies
  wiring effects
  calibration deviations
  
  Independent offsets improve overall measurement accuracy.

### UART Data Transmission

  The temperature data is formatted into a CSV-compatible string:
  
  Counter; Temp1; Temp2; Temp3; Temp4
  
  Example:
  
  15; 23.41; 24.10; 22.95; 23.87
  
  The formatted message is transmitted using:

  HAL_UART_Transmit(...)
  
  This allows external software (e.g. Python logger scripts) to capture and store the measurements.


### SystemClock_Config(void)
In the code, it uses the High-Speed External (HSE) clock from the ST-Link bypass to run the system at a high frequency.

### Error_Handler(void)

This is a safety "infinite loop." If a critical initialization fails (like SPI or UART failing to start), the code calls this function. It disables interrupts and sits in a while(1) loop, effectively stopping the program so I can debug what went wrong.


# Initilisation functions: 

### MX_GPIO_Init()

I enable the clocks for all the ports (A through G) so they have power. Specifically, I set PD14 as my "Chip Select" output. I make sure it starts "High" because the MAX31855 only talks to me when I pull that line "Low."

### MX_SPI1_Init()

This is how I set the "rules" for my conversation with the sensor.
I use a very large Prescaler (256). This makes me speak slowly and clearly so that even if the jumper wires are long or a bit loose, I won't lose any bits.

I set the Phase to 2-Edge because that's the specific timing the MAX31855 requires to ensure I read the bits at the exact moment they are stable.

### MX_USART3_UART_Init()

I use this to talk to your computer. I configure my internal UART3 hardware to 115,200 bits per second.


## Calculation of Temperature: 

### MAX31855_ReadTemp()

I receive 4 bytes and take those four 8-bit pieces and put them together into one 32-bit number.

I check for errors: I look at Bit 16. If the sensor tells me there's a "Fault," I stop immediately and return a specific error value (−1000.0).

I shift and scale: I isolate the bits that represent the temperature (31 to 18). If the temperature is negative, I handle the math (sign extension) to make sure it's correct. Finally, I multiply by 0.25 because each "step" in the sensor's data represents a quarter of a degree Celsius.

# Detailed Explanation of the setup stm32f7xx_hal_msp.c
Here is the breakdown of the MSP (MCU Support Package) file. If the main.c is the "brain" that makes decisions, this file is the "nervous system"—it connects the internal logic to the physical pins and power lines.

### 1. Global System Foundation (HAL_MspInit)

In the global initialization phase, I prepared the core power architecture of the microcontroller.

C
void HAL_MspInit(void) {
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
}

Technical Logic: I enabled the clock for the Power Interface (PWR) and the System Configuration (SYSCFG). This action ensures the internal voltage regulators and the peripheral routing matrix are energized before any specific communication protocols are initialized.

### 2. SPI Physical Interface Configuration (HAL_SPI_MspInit)

When the system triggers HAL_SPI_Init, I execute the following sequence to establish the physical link to the MAX31855 sensor.
```
<pre>
C
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi) {
  if(hspi->Instance == SPI1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
 </pre>   
 ```
Action: I enabled the high-speed clock for the SPI1 peripheral and GPIO Port A. Because STM32 peripherals are clock-gated for power efficiency, I must provide this clock signal before the peripheral registers can be accessed.
```
<pre>
C
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1; 
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}
</pre>
```
Action: I performed Pin Multiplexing by assigning pins PA5 (SCK), PA6 (MISO), and PA7 (MOSI) to Alternate Function 5 (AF5). This reconfigures the silicon's internal routing, disconnecting the pins from the standard GPIO registers and hard-wiring them directly to the SPI1 hardware engine. I set the speed to VERY_HIGH to maintain signal integrity for the digital clock transitions.

### 3. Telemetry Link Setup (HAL_UART_MspInit)

To facilitate data transmission to the host PC, I configured the low-level resources for USART3.
```
<pre>
C
PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
__HAL_RCC_USART3_CLK_ENABLE();

</pre>
```
Action: I synchronized the peripheral timing by selecting PCLK1 as the clock source for the UART baud rate generator. I then enabled the clock for the USART3 module.
<pre>
C
GPIO_InitStruct.Pin = STLK_RX_Pin|STLK_TX_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

</pre>
Action: I mapped pins PD8 (TX) and PD9 (RX) to Alternate Function 7 (AF7). These pins are physically routed to the onboard ST-Link debugger, establishing the Virtual COM Port connection to the computer.

### 5. Resource De-allocation (MspDeInit functions)

I implemented the de-initialization functions to ensure proper power management and pin state reset.
<pre>
C
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi) {
  __HAL_RCC_SPI1_CLK_DISABLE();
  HAL_GPIO_DeInit(GPIOD, GPIO_PIN_7); // Resetting used pins
}

</pre>
Action: I disabled the peripheral clocks and utilized HAL_GPIO_DeInit to return the pins to their default high-impedance state. This prevents leakage current and ensures that no parasitic power is drawn when the SPI or UART modules are not in active use.




