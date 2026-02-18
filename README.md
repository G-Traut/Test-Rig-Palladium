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

# Code 

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

# Detailed Explanation of hte code main.c

### main(void)

This is the entry point of your program. It follows a specific sequence:

Initialization: It calls HAL_Init() to reset peripherals and SystemClock_Config() to set the CPU speed. It then initializes the GPIO, SPI, and UART peripherals.

The Infinite Loop (while(1)): This is where the "work" happens. It manually pulls the Chip Select (CS) pin low to talk to the sensor, receives 4 bytes of data via SPI, pulls CS high again, and sends that raw data to your computer via UART for debugging.

### SystemClock_Config(void)

This function is the "heartbeat" of the STM32.

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


