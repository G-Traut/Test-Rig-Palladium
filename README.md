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

# Detailed Explanation of the code main.c

### main(void)

This is the entry point of the program. It follows a specific sequence:

Initialization: It calls HAL_Init() to reset peripherals and SystemClock_Config() to set the CPU speed. It then initializes the GPIO, SPI, and UART peripherals.

The Infinite Loop (while(1)): It manually pulls the Chip Select (CS) pin low to talk to the sensor, receives 4 bytes of data via SPI, pulls CS high again, and sends that raw data to the computer via UART for debugging.

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


# Calculation of Temperature: 

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

<pre>
C
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi) {
  if(hspi->Instance == SPI1) {
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
 </pre>   
Action: I enabled the high-speed clock for the SPI1 peripheral and GPIO Port A. Because STM32 peripherals are clock-gated for power efficiency, I must provide this clock signal before the peripheral registers can be accessed.
<pre>
C
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1; 
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}
</pre>
Action: I performed Pin Multiplexing by assigning pins PA5 (SCK), PA6 (MISO), and PA7 (MOSI) to Alternate Function 5 (AF5). This reconfigures the silicon's internal routing, disconnecting the pins from the standard GPIO registers and hard-wiring them directly to the SPI1 hardware engine. I set the speed to VERY_HIGH to maintain signal integrity for the digital clock transitions.

### 3. Telemetry Link Setup (HAL_UART_MspInit)

To facilitate data transmission to the host PC, I configured the low-level resources for USART3.
<pre>
C
PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
__HAL_RCC_USART3_CLK_ENABLE();

</pre>
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




