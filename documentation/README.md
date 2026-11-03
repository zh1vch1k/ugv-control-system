# Hardware Interfaces & Connection Specifications (UGV Control System)
# Специфікація апаратних інтерфейсів та підключень (UGV Control System)

---

## [ENGLISH VERSION]

### 1. Compute Nodes Architecture

The UGV (Unmanned Ground Vehicle) control system utilizes a distributed, heterogeneous microcontroller architecture:

```
[ Operator Ground Station / Remote Controller ]
+-----------------------------------------------------------+
| ESP32-D0WDQ6 (LilyGO TTGO LoRa32 V1)                      |
|  - Control Input: RS-485 / Gamepad Reader (UART2)         |
|  - Display: 0.96" SSD1306 OLED (I2C0)                     |
|  - Radio Link: SX1276/78 LoRa SPI (868 / 433 MHz)         |
+-----------------------------------------------------------+
                             |
                    Wireless Radio Link
                       (RFBP / LoRa)
                             |
                             v
[ UGV Onboard Chassis ]
+-----------------------------------------------------------+
| ESP32-C6-DevKitC-1 (Radio Bridge)                         |
|  - Radio Receiver: Ebyte E32-433T20D / E32-868 (UART1/GPIO)|
|  - Status Indicator: Addressable RGB LED WS2812 (RMT GPIO8)|
|  - Interprocessor Bus: UART0 (115200 8N1)                 |
+-----------------------------------------------------------+
                             |
                     Inter-board UART
                    (PA2/PA3 <-> 20/21)
                             |
                             v
+-----------------------------------------------------------+
| STM32H562VGT6 (Platform Core Controller / LQFP100)        |
|  - Motor Driver Axis 1 (Left Track)  : TIM3 (4 channels)  |
|  - Motor Driver Axis 2 (Right Track) : TIM4 (4 channels)  |
|  - Motor Driver Axis 3 (Aux Axis)    : TIM2 (4 channels)  |
|  - Orientation Sensor                : I2C3 (LSM6DS IMU)  |
|  - Power Monitor (Optional)          : I2C3 (INA219/226)  |
|  - Wheel Encoders                    : EXTI14, EXTI15     |
|  - Rangefinder / Radar               : USART1 (DMA RX)    |
|  - GPS Navigation Module             : UART4              |
|  - Telemetry & Diagnostic Logger     : USART6             |
+-----------------------------------------------------------+
```

### 2. Documentation Directory Index

| Document | Target MCU / Subsystem | Purpose & Scope |
| :--- | :--- | :--- |
| **[stm32h562_ugv_controller.md](./stm32h562_ugv_controller.md)** | **STM32H562VGT6** (LQFP100) | Main platform controller: 3-axis motor PWM, I2C sensor bus, circular DMA radar, GPS, encoders, telemetry |
| **[esp32c6_radio_bridge.md](./esp32c6_radio_bridge.md)** | **ESP32-C6-DevKitC-1** | Onboard radio gateway: Ebyte E32 LoRa handling, session FSM resolver, fast UART bridge to STM32 |
| **[esp32_ground_station.md](./esp32_ground_station.md)** | **ESP32 TTGO LoRa32 V1** | Operator ground station: RS-485 gamepad telemetry, SPECK-64/128 crypto, LoRa TX, OLED status UI |
| **[hardware_interconnect.md](./hardware_interconnect.md)** | Wiring & Interconnect | Complete wiring map, 3.3V/5V power domains, star ground layout, cable harnesses and pinouts |

### 3. Summary Interface Parameters Table

| Path / Interface | Physical Layer | Speed / Frequency | Protocol / Framing | Checksum / Security |
| :--- | :--- | :--- | :--- | :--- |
| **Gamepad $\rightarrow$ ESP32 TX** | RS-485 / UART2 | 115200 8N1 | Controller Packet (`0xAA` header, 8 bytes) | CRC8 |
| **ESP32 TX $\rightarrow$ ESP32-C6** | LoRa 868 / 433 MHz | BW 125 kHz, SF9 | RFBP Frame (8 bytes) | SPECK-64/128 + rolling counter |
| **ESP32-C6 $\rightarrow$ STM32H562** | Full-Duplex UART | 115200 8N1 | Binary RFBP (8 bytes) + ACK Handshake | SPECK-64/128 |
| **STM32H562 $\rightarrow$ Motors** | Push-Pull PWM | 4.0 kHz | Timers TIM2, TIM3, TIM4 (ARR=999, PSC=15) | Hardware PWM |
| **STM32H562 $\leftarrow$ LSM6DS** | Fast I2C (Open-Drain) | 100 kHz | Register access (WHO_AM_I, CTRL, OUT_XL) | Hardware I2C ACK |
| **STM32H562 $\leftarrow$ Radar** | UART (GPDMA1) | 115200 8N1 | Detection frames with Idle Line detection | Radar CRC / Packet checksum |
| **STM32H562 $\leftarrow$ GPS** | UART (Interrupts) | 115200 / 9600 8N1 | NMEA-0183 (`$GNRMC`, `$GNGGA`, etc.) | NMEA XOR Checksum (`*XX`) |
| **STM32H562 $\rightarrow$ PC/Logger** | UART (USART6) | 115200 8N1 | ASCII POST diagnostics & telemetry stream | - |

---

## [УКРАЇНСЬКА ВЕРСІЯ]

### 1. Архітектура обчислювальних вузлів

Система керування комплексом UGV (Unmanned Ground Vehicle) побудована на розподіленій гетерогенній мікроконтролерній архітектурі:

```
[ Наземна станція оператора / Пульт керування ]
+-----------------------------------------------------------+
| ESP32-D0WDQ6 (LilyGO TTGO LoRa32 V1)                      |
|  - Вхід керування: RS-485 / Gamepad Reader (UART2)        |
|  - Дисплей: 0.96" SSD1306 OLED (I2C0)                     |
|  - Радіоканал: SX1276/78 LoRa SPI (868 / 433 МГц)         |
+-----------------------------------------------------------+
                             |
                   Бездротовий радіоканал
                       (RFBP / LoRa)
                             |
                             v
[ Борт платформи (UGV Chassis) ]
+-----------------------------------------------------------+
| ESP32-C6-DevKitC-1 (Радіоміст)                            |
|  - Радіоприймач: Ebyte E32-433T20D / E32-868 (UART1/GPIO) |
|  - Індикатор статусу: Адресний RGB LED WS2812 (RMT GPIO8) |
|  - Міжпроцесорна шина: UART0 (115200 8N1)                 |
+-----------------------------------------------------------+
                             |
                      Міжплатний UART
                    (PA2/PA3 <-> 20/21)
                             |
                             v
+-----------------------------------------------------------+
| STM32H562VGT6 (Головний контролер платформи / LQFP100)    |
|  - Драйвер моторів вісь 1 (лівий борт) : TIM3 (4 канали)  |
|  - Драйвер моторів вісь 2 (правий борт): TIM4 (4 канали)  |
|  - Драйвер моторів вісь 3 (додаткова)  : TIM2 (4 канали)  |
|  - Сенсори орієнтації                  : I2C3 (LSM6DS IMU)|
|  - Монітор живлення (опціонально)      : I2C3 (INA219/226)|
|  - Енкодери коліс                      : EXTI14, EXTI15   |
|  - Далекомір / Радар                   : USART1 (DMA RX)  |
|  - Навігаційний модуль GPS             : UART4            |
|  - Телеметрія та логер                 : USART6           |
+-----------------------------------------------------------+
```

### 2. Покажчик документації

| Документ | Цільовий МК / Підсистема | Призначення та зміст |
| :--- | :--- | :--- |
| **[stm32h562_ugv_controller.md](./stm32h562_ugv_controller.md)** | **STM32H562VGT6** (LQFP100) | Головний контролер шасі: ШІМ 3 осей моторів, сенсори I2C, радар по DMA, GPS, енкодери, телеметрія |
| **[esp32c6_radio_bridge.md](./esp32c6_radio_bridge.md)** | **ESP32-C6-DevKitC-1** | Бортовий радіоміст: прийом пакетів LoRa Ebyte E32, FSM сесії, швидкісний UART зв'язок зі STM32 |
| **[esp32_ground_station.md](./esp32_ground_station.md)** | **ESP32 TTGO LoRa32 V1** | Пульт оператора (Ground Station): прийом команд з контролера по RS-485, шифрування SPECK, LoRa TX, OLED дисплей |
| **[hardware_interconnect.md](./hardware_interconnect.md)** | Міжплатна комутація | Схема міжблочних з'єднань, рівні напруг (3.3V/5V), схема заземлення "зірка", розпіновка джгутів |

### 3. Зведена таблиця параметрів інтерфейсів

| Напрямок / Інтерфейс | Фізичний рівень | Швидкість / Частота | Протокол / Кадр | Контрольна сума / Безпека |
| :--- | :--- | :--- | :--- | :--- |
| **Геймпад $\rightarrow$ ESP32 TX** | RS-485 / UART2 | 115200 8N1 | Пакет контролера (заголовок `0xAA`, 8 байт) | CRC8 |
| **ESP32 TX $\rightarrow$ ESP32-C6** | LoRa 868 / 433 МГц | BW 125 кГц, SF9 | RFBP фрейм (8 байт) | SPECK-64/128 + сесійний лічильник |
| **ESP32-C6 $\rightarrow$ STM32H562** | Повнодуплексний UART| 115200 8N1 | Бінарний RFBP (8 байт) + ACK Handshake | SPECK-64/128 |
| **STM32H562 $\rightarrow$ Мотори** | ШІМ (Push-Pull) | 4.0 кГц | Таймери TIM2, TIM3, TIM4 (ARR=999, PSC=15) | Апаратний ШІМ |
| **STM32H562 $\leftarrow$ LSM6DS** | Fast I2C (Open-Drain)| 100 кГц | Регістровий доступ (WHO_AM_I, CTRL, OUT_XL) | Апаратний ACK шини I2C |
| **STM32H562 $\leftarrow$ Радар** | UART (GPDMA1) | 115200 8N1 | Кадри виявлення з фіксацією Idle Line | Контрольна сума радара |
| **STM32H562 $\leftarrow$ GPS** | UART (Переривання) | 115200 / 9600 8N1 | NMEA-0183 (`$GNRMC`, `$GNGGA` тощо) | NMEA XOR Checksum (`*XX`) |
| **STM32H562 $\rightarrow$ ПК/Логер** | UART (USART6) | 115200 8N1 | ASCII POST діагностика та потік телеметрії | - |
