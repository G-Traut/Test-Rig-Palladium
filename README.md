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

  ### 🔌 Connection Diagram (Pin Mapping)

| Function | MAX31855 Pin | Nucleo-F767ZI Pin | STM32 Internal Pin |
| :--- | :--- | :--- | :--- |
| **Power** | Vin (3.3V - 5V) | 3.3V | VCC |
| **Ground** | GND | GND | GND |
| **SPI Clock** | CLK | D13 | PA5 |
| **SPI MISO** | DO (Data Out) | D12 | PA6 |
| **Chip Select** | CS | D10 | PD14 |
