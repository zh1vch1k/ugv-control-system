# Hardware Interconnect & Cable Harness Specification (UGV System)
# Специфікація міжмодульної комутації та кабельних з'єднань (UGV System)

---

## [ENGLISH VERSION]

### 1. Power Topology & Logic Signal Levels

All microcontrollers in this platform operate using **3.3V LVTTL / CMOS logic levels**.

> [!CAUTION]
> **Overvoltage Hazard**: Supplying 5.0V signals directly to ESP32-C6, ESP32, or analog-capable STM32H562 pins **will cause permanent damage**. All inter-board lines must strictly operate at $0 \dots 3.3\text{V}$. The common ground (`GND`) of all boards, motor drivers, and sensors must be connected in a single "star" point topology to eliminate ground bounce and ground loops.

---

### 2. Onboard UGV Wiring Specifications

#### 2.1. Interprocessor Bus: STM32H562VGT6 $\longleftrightarrow$ ESP32-C6

| Signal Name | STM32H562 Pin | ESP32-C6 Pin | Line Description |
| :--- | :---: | :---: | :--- |
| **TX $\rightarrow$ RX** | `PA2` (USART2_TX, Pin 25) | `GPIO20` (UART0_RX) | Handshake ACKs & responses from STM32 to radio bridge |
| **RX $\leftarrow$ TX** | `PA3` (USART2_RX, Pin 26) | `GPIO21` (UART0_TX) | Decrypted control frames from radio bridge to STM32 |
| **GND** | System Ground (GND) | System Ground (GND) | Shared reference ground |

---

#### 2.2. Radio Bridge: ESP32-C6 $\longleftrightarrow$ Ebyte E32 LoRa Module

| Signal Name | ESP32-C6 Pin | Ebyte E32 Pin | Line Description |
| :--- | :---: | :---: | :--- |
| **TX $\rightarrow$ RX** | `GPIO6` (UART1_TX) | `RXD` | Byte stream for wireless transmission |
| **RX $\leftarrow$ TX** | `GPIO7` (UART1_RX) | `TXD` | Received wireless byte stream |
| **M0** | `GPIO4` (Output) | `M0` | Ebyte operating mode selector (bit 0) |
| **M1** | `GPIO5` (Output) | `M1` | Ebyte operating mode selector (bit 1) |
| **AUX** | `GPIO14` (Input PullUp) | `AUX` | Buffer status output (Low = module busy) |
| **VCC** | 3.3V / 5.0V Power | `VCC` | Ebyte power rail (5V 1A recommended for RF bursts) |
| **GND** | Common Ground | `GND` | Power ground |

---

#### 2.3. Chassis Motor Drivers $\longleftrightarrow$ STM32H562VGT6

Each axis is driven by 4 hardware PWM channels:

##### Axis 1: Left Track (`TIM3`, 4.0 kHz PWM)
| PWM Channel | STM32 Pin | LQFP100 Pin | Motor Driver Input |
| :--- | :---: | :---: | :--- |
| `TIM3_CH1` | `PA6` | Pin 31 | Left Driver, Motor 1 Forward (`IN1` / `PWM_FWD_1`) |
| `TIM3_CH2` | `PA7` | Pin 32 | Left Driver, Motor 1 Reverse (`IN2` / `PWM_REV_1`) |
| `TIM3_CH3` | `PB0` | Pin 35 | Left Driver, Motor 2 Forward (`IN3` / `PWM_FWD_2`) |
| `TIM3_CH4` | `PB1` | Pin 36 | Left Driver, Motor 2 Reverse (`IN4` / `PWM_REV_2`) |

##### Axis 2: Right Track (`TIM4`, 4.0 kHz PWM)
| PWM Channel | STM32 Pin | LQFP100 Pin | Motor Driver Input |
| :--- | :---: | :---: | :--- |
| `TIM4_CH1` | `PB6` | Pin 92 | Right Driver, Motor 1 Forward (`IN1` / `PWM_FWD_1`) |
| `TIM4_CH2` | `PB7` | Pin 93 | Right Driver, Motor 1 Reverse (`IN2` / `PWM_REV_1`) |
| `TIM4_CH3` | `PB8` | Pin 95 | Right Driver, Motor 2 Forward (`IN3` / `PWM_FWD_2`) |
| `TIM4_CH4` | `PB9` | Pin 96 | Right Driver, Motor 2 Reverse (`IN4` / `PWM_REV_2`) |

##### Axis 3: Auxiliary / Center Axis (`TIM2`, 4.0 kHz PWM)
| PWM Channel | STM32 Pin | LQFP100 Pin | Motor Driver Input |
| :--- | :---: | :---: | :--- |
| `TIM2_CH1` | `PA0` | Pin 23 | Axis 3 Driver, Motor 1 Forward (`IN1` / `PWM_FWD_1`) |
| `TIM2_CH2` | `PA1` | Pin 24 | Axis 3 Driver, Motor 1 Reverse (`IN2` / `PWM_REV_1`) |
| `TIM2_CH3` | `PB10`| Pin 47 | Axis 3 Driver, Motor 2 Forward (`IN3` / `PWM_FWD_2`) |
| `TIM2_CH4` | `PC4` | Pin 33 | Axis 3 Driver, Motor 2 Reverse (`IN4` / `PWM_REV_2`) |

---

#### 2.4. Sensor Bus `I2C3`: LSM6DS & INA219/226 $\longleftrightarrow$ STM32H562

| I2C Bus Line | STM32 Pin | LQFP100 Pin | Sensor Module Connection |
| :--- | :---: | :---: | :--- |
| **I2C3_SCL** | `PA8` | Pin 67 | `SCL` of LSM6DS and `SCL` of INA219/226 |
| **I2C3_SDA** | `PC9` | Pin 66 | `SDA` of LSM6DS and `SDA` of INA219/226 |
| **VCC (3.3V)** | 3.3V Rail | - | Sensor VDD power rail |
| **GND** | System Ground | - | Common sensor ground |

---

#### 2.5. Navigation & Rangefinding $\longleftrightarrow$ STM32H562

##### Radar / Lidar Module (`USART1` + GPDMA1)
* `STM32 PA9` (TX, Pin 68) $\longrightarrow$ Radar `RXD`
* `STM32 PA10` (RX, Pin 69) $\longleftarrow$ Radar `TXD`
* Baudrate: 115200 8N1.

##### GPS Navigation Receiver (`UART4`)
* `STM32 PC10` (TX, Pin 78) $\longrightarrow$ GPS `RXD`
* `STM32 PC11` (RX, Pin 79) $\longleftarrow$ GPS `TXD`
* Baudrate: Auto-detected (115200, 9600, or 38400 baud).

##### Optical Wheel Encoders
* Left Track Encoder Output $\longrightarrow$ `STM32 PE14` (Pin 45, EXTI14).
* Right Track Encoder Output $\longrightarrow$ `STM32 PE15` (Pin 46, EXTI15).

---

### 3. Operator Ground Station Wiring

#### ESP32 (TTGO LoRa32 V1) $\longleftrightarrow$ RS-485 Gamepad Receiver (MAX3485)

| ESP32 Pin | Board Header Pin | MAX3485 Pin | Line Role |
| :--- | :---: | :---: | :--- |
| `UART2_RXD` | `GPIO13` | `RO` (Receiver Output) | Gamepad telemetry input (115200 8N1) |
| `UART2_TXD` | `GPIO14` | `DI` (Driver Input) | Transmit line (reserved) |
| `RS485_CTRL`| `GPIO15` | `RE` + `DE` (Tied together) | Pulled to GND (permanent receive mode) |
| **3.3V** | `3V3` | `VCC` | Power supply |
| **GND** | `GND` | `GND` | Reference ground |
| **Line A** | - | RS-485 Terminal `A` | Non-inverting differential bus signal |
| **Line B** | - | RS-485 Terminal `B` | Inverting differential bus signal |

---

## [УКРАЇНСЬКА ВЕРСІЯ]

### 1. Топологія живлення та логічні рівні сигналів

Усі мікроконтролери комплексу використовують логічні рівні **3.3V LVTTL / CMOS**.

> [!CAUTION]
> **Небезпека пошкодження входів**: Подача сигналів 5.0 В безпосередньо на виводи ESP32-C6, ESP32 або аналогові виводи STM32H562 **призведе до виходу з ладу**. Усі міжблочні лінії зв'язку мають працювати виключно в межах $0 \dots 3.3\text{ В}$. Загальний провід (`GND`) усіх плат, драйверів та сенсорів повинен бути з'єднаний за схемою "зірка" для виключення паразитного падіння напруги.

---

### 2. Специфікація бортових з'єднань UGV

#### 2.1. Міжпроцесорна шина: STM32H562VGT6 $\longleftrightarrow$ ESP32-C6

| Назва сигналу | Вивід STM32H562 | Вивід ESP32-C6 | Опис лінії |
| :--- | :---: | :---: | :--- |
| **TX $\rightarrow$ RX** | `PA2` (USART2_TX, Пін 25) | `GPIO20` (UART0_RX) | Відповіді підтвердження (ACK) від STM32 до радіомоста |
| **RX $\leftarrow$ TX** | `PA3` (USART2_RX, Пін 26) | `GPIO21` (UART0_TX) | Розшифровані пакети керування від радіомоста до STM32 |
| **GND** | Загальна земля (GND) | Загальна земля (GND) | Спільний опорний потенціал |

---

#### 2.2. Радіоміст: ESP32-C6 $\longleftrightarrow$ Модуль LoRa Ebyte E32

| Назва сигналу | Вивід ESP32-C6 | Вивід Ebyte E32 | Опис лінії |
| :--- | :---: | :---: | :--- |
| **TX $\rightarrow$ RX** | `GPIO6` (UART1_TX) | `RXD` | Потік байтів для бездротової передачі |
| **RX $\leftarrow$ TX** | `GPIO7` (UART1_RX) | `TXD` | Потік байтів, прийнятих з ефіру |
| **M0** | `GPIO4` (Output) | `M0` | Селектор режиму роботи Ebyte (біт 0) |
| **M1** | `GPIO5` (Output) | `M1` | Селектор режиму роботи Ebyte (біт 1) |
| **AUX** | `GPIO14` (Input PullUp) | `AUX` | Лінія зайнятості модуля (Low = зайнятий) |
| **VCC** | Живлення 3.3V / 5.0V | `VCC` | Живлення Ebyte (рекомендовано 5V 1A для пікових струмів) |
| **GND** | Загальна земля | `GND` | Земля живлення |

---

#### 2.3. Драйвери моторів шасі $\longleftrightarrow$ STM32H562VGT6

Кожна вісь керується 4 апаратними лініями ШІМ:

##### Вісь 1: Лівий борт (`TIM3`, ШІМ 4.0 кГц)
| Канал ШІМ | Вивід STM32 | Вивід LQFP100 | Вхід драйвера моторів |
| :--- | :---: | :---: | :--- |
| `TIM3_CH1` | `PA6` | Пін 31 | Лівий драйвер, Мотор 1 Вперед (`IN1` / `PWM_FWD_1`) |
| `TIM3_CH2` | `PA7` | Пін 32 | Лівий драйвер, Мотор 1 Назад (`IN2` / `PWM_REV_1`) |
| `TIM3_CH3` | `PB0` | Пін 35 | Лівий драйвер, Мотор 2 Вперед (`IN3` / `PWM_FWD_2`) |
| `TIM3_CH4` | `PB1` | Пін 36 | Лівий драйвер, Мотор 2 Назад (`IN4` / `PWM_REV_2`) |

##### Вісь 2: Правий борт (`TIM4`, ШІМ 4.0 кГц)
| Канал ШІМ | Вивід STM32 | Вивід LQFP100 | Вхід драйвера моторів |
| :--- | :---: | :---: | :--- |
| `TIM4_CH1` | `PB6` | Пін 92 | Правий драйвер, Мотор 1 Вперед (`IN1` / `PWM_FWD_1`) |
| `TIM4_CH2` | `PB7` | Пін 93 | Правий драйвер, Мотор 1 Назад (`IN2` / `PWM_REV_1`) |
| `TIM4_CH3` | `PB8` | Пін 95 | Правий драйвер, Мотор 2 Вперед (`IN3` / `PWM_FWD_2`) |
| `TIM4_CH4` | `PB9` | Пін 96 | Правий драйвер, Мотор 2 Назад (`IN4` / `PWM_REV_2`) |

##### Вісь 3: Додаткова вісь / Середній міст (`TIM2`, ШІМ 4.0 кГц)
| Канал ШІМ | Вивід STM32 | Вивід LQFP100 | Вхід драйвера моторів |
| :--- | :---: | :---: | :--- |
| `TIM2_CH1` | `PA0` | Пін 23 | Драйвер осі 3, Мотор 1 Вперед (`IN1` / `PWM_FWD_1`) |
| `TIM2_CH2` | `PA1` | Пін 24 | Драйвер осі 3, Мотор 1 Назад (`IN2` / `PWM_REV_1`) |
| `TIM2_CH3` | `PB10`| Пін 47 | Драйвер осі 3, Мотор 2 Вперед (`IN3` / `PWM_FWD_2`) |
| `TIM2_CH4` | `PC4` | Пін 33 | Драйвер осі 3, Мотор 2 Назад (`IN4` / `PWM_REV_2`) |

---

#### 2.4. Шина сенсорів `I2C3`: LSM6DS та INA219/226 $\longleftrightarrow$ STM32H562

| Лінія шини I2C | Вивід STM32 | Вивід LQFP100 | Підключення до датчиків |
| :--- | :---: | :---: | :--- |
| **I2C3_SCL** | `PA8` | Пін 67 | Контакт `SCL` LSM6DS та `SCL` INA219/226 |
| **I2C3_SDA** | `PC9` | Пін 66 | Контакт `SDA` LSM6DS та `SDA` INA219/226 |
| **VCC (3.3V)** | Шина 3.3 В | - | Живлення датчиків 3.3 В |
| **GND** | Загальна земля | - | Земля датчиків |

---

#### 2.5. Навігація та далекометрія $\longleftrightarrow$ STM32H562

##### Радар / Лідар (`USART1` + GPDMA1)
* `STM32 PA9` (TX, Пін 68) $\longrightarrow$ Вхід `RXD` радара
* `STM32 PA10` (RX, Пін 69) $\longleftarrow$ Вихід `TXD` радара
* Швидкість: 115200 8N1.

##### GPS-приймач (`UART4`)
* `STM32 PC10` (TX, Пін 78) $\longrightarrow$ Вхід `RXD` GPS
* `STM32 PC11` (RX, Пін 79) $\longleftarrow$ Вихід `TXD` GPS
* Швидкість: Автовизначення (115200, 9600 або 38400 бод).

##### Оптичні енкодери коліс
* Лівий енкодер: Сигнальний вихід $\longrightarrow$ `STM32 PE14` (Пін 45, EXTI14).
* Правий енкодер: Сигнальний вихід $\longrightarrow$ `STM32 PE15` (Пін 46, EXTI15).

---

### 3. Схема комутації на пульті оператора (Ground Station)

#### ESP32 (TTGO LoRa32 V1) $\longleftrightarrow$ Трансивер RS-485 (MAX3485)

| Лінія ESP32 | Вивід плати | Вивід MAX3485 | Призначення лінії |
| :--- | :---: | :---: | :--- |
| `UART2_RXD` | `GPIO13` | `RO` (Receiver Output) | Прийом телеметрії з геймпада (115200 8N1) |
| `UART2_TXD` | `GPIO14` | `DI` (Driver Input) | Передача даних у лінію (резерв) |
| `RS485_CTRL`| `GPIO15` | `RE` + `DE` (З'єднані) | Притягнуто до GND (постійний прийом) |
| **3.3V** | `3V3` | `VCC` | Живлення мікросхеми 3.3 В |
| **GND** | `GND` | `GND` | Опорне заземлення |
| **Лінія A** | - | Клема `A` шини RS-485 | Неінвертуючий диференціальний сигнал |
| **Лінія B** | - | Клема `B` шини RS-485 | Інвертуючий диференціальний сигнал |
