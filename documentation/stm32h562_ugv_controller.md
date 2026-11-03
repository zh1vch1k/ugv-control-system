# STM32H562VGT6 Microcontroller Specification (UGV Core Controller)
# Специфікація мікроконтролера STM32H562VGT6 (Головний контролер UGV)

---

## [ENGLISH VERSION]

### 1. General Specifications

* **Microcontroller**: STM32H562VGT6
* **Architecture**: 32-bit ARM Cortex-M33 with single-precision FPU and DSP instructions
* **Maximum Core Frequency**: Up to 250 MHz (UGV base firmware: 64 MHz HSI internal oscillator without PLL, AHB/APB prescalers = 1)
* **Package**: LQFP100 (14 × 14 mm, 100 pins, 0.5 mm pin pitch)
* **Core & I/O Voltage**: 3.3V nominal ($V_{DD}$ range: 1.71V to 3.6V)
* **Power Regulator Mode**: VOS0 (highest performance scale)
* **System Timebase**: General-purpose timer `TIM6` (APB1 bus, 1 ms period, 1000 Hz `HAL_IncTick` interrupt)

---

### 2. Full Pinout & Alternate Function Table (LQFP100)

All pin assignments strictly comply with STMicroelectronics official datasheet **`DS14258 Rev 6`**:

| LQFP100 Pin | Pin Name | Subsystem Role in UGV | GPIO Mode / Alternate Function | Speed / Pull | Target Connected Device |
| :---: | :---: | :--- | :--- | :--- | :--- |
| **23** | **PA0** | TIM2_CH1 (Axis 3, Motor 1+) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Axis 3 Motor Driver (PWM Fwd 1) |
| **24** | **PA1** | TIM2_CH2 (Axis 3, Motor 1-) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Axis 3 Motor Driver (PWM Rev 1) |
| **25** | **PA2** | USART2_TX (Radio Bridge) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART2`) | High / PullUp | ESP32-C6 (`GPIO20` / RX) |
| **26** | **PA3** | USART2_RX (Radio Bridge) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART2`) | High / PullUp | ESP32-C6 (`GPIO21` / TX) |
| **31** | **PA6** | TIM3_CH1 (Left Track, Motor 1+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Left Track H-Bridge (PWM Fwd 1) |
| **32** | **PA7** | TIM3_CH2 (Left Track, Motor 1-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Left Track H-Bridge (PWM Rev 1) |
| **67** | **PA8** | I2C3_SCL (Sensor Bus) | `GPIO_MODE_AF_OD`, **AF4** (`GPIO_AF4_I2C3`) | High / PullUp | LSM6DS (SCL), INA219/226 (SCL) |
| **68** | **PA9** | USART1_TX (Radar / Lidar) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART1`) | High / PullUp | Radar / Lidar Module (RX) |
| **69** | **PA10** | USART1_RX (Radar / DMA) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART1`) | High / PullUp | Radar / Lidar Module (TX) |
| **72** | **PA13** | SWDIO (ST-Link Debug) | Dedicated SWD Debug Interface | High / PullUp | ST-Link V2/V3 Programmer |
| **76** | **PA14** | SWCLK (ST-Link Debug) | Dedicated SWD Debug Interface | High / PullDown | ST-Link V2/V3 Programmer |
| **35** | **PB0** | TIM3_CH3 (Left Track, Motor 2+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Left Track H-Bridge (PWM Fwd 2) |
| **36** | **PB1** | TIM3_CH4 (Left Track, Motor 2-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Left Track H-Bridge (PWM Rev 2) |
| **37** | **PB2** | Board Heartbeat LED | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Onboard Status LED (Active LOW) |
| **92** | **PB6** | TIM4_CH1 (Right Track, Motor 1+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Right Track H-Bridge (PWM Fwd 1) |
| **93** | **PB7** | TIM4_CH2 (Right Track, Motor 1-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Right Track H-Bridge (PWM Rev 1) |
| **95** | **PB8** | TIM4_CH3 (Right Track, Motor 2+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Right Track H-Bridge (PWM Fwd 2) |
| **96** | **PB9** | TIM4_CH4 (Right Track, Motor 2-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Right Track H-Bridge (PWM Rev 2) |
| **47** | **PB10** | TIM2_CH3 (Axis 3, Motor 2+) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Axis 3 Motor Driver (PWM Fwd 2) |
| **33** | **PC4** | TIM2_CH4 (Axis 3, Motor 2-) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Axis 3 Motor Driver (PWM Rev 2) |
| **63** | **PC6** | USART6_TX (Telemetry / Log) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART6`) | High / PullUp | USB-UART Serial Terminal / Logger |
| **64** | **PC7** | USART6_RX (Telemetry / Log) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART6`) | High / PullUp | USB-UART Serial Terminal / Logger |
| **66** | **PC9** | I2C3_SDA (Sensor Bus) | `GPIO_MODE_AF_OD`, **AF4** (`GPIO_AF4_I2C3`) | High / PullUp | LSM6DS (SDA), INA219/226 (SDA) |
| **78** | **PC10** | UART4_TX (GPS Module) | `GPIO_MODE_AF_PP`, **AF8** (`GPIO_AF8_UART4`) | High / PullUp | GPS Navigation Module (RX) |
| **79** | **PC11** | UART4_RX (GPS Module) | `GPIO_MODE_AF_PP`, **AF8** (`GPIO_AF8_UART4`) | High / PullUp | GPS Navigation Module (TX) |
| **1** | **PE2** | RGB LED (Red Channel) | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Onboard RGB LED (Active HIGH) |
| **2** | **PE3** | RGB LED (Green Channel) | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Onboard RGB LED (Active HIGH) |
| **3** | **PE4** | RGB LED (Blue Channel) | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Onboard RGB LED (Active HIGH) |
| **45** | **PE14** | EXTI14 (Left Wheel Encoder) | `GPIO_MODE_IT_RISING` | Low / PullUp | Optical Sensor Left Track |
| **46** | **PE15** | EXTI15 (Right Wheel Encoder)| `GPIO_MODE_IT_RISING` | Low / PullUp | Optical Sensor Right Track |

---

### 3. Subsystem Configurations

#### 3.1. Motor Control Trajectory (4.0 kHz PWM)
* **`TIM3` (Left Track)**: Pins `PA6` (CH1), `PA7` (CH2), `PB0` (CH3), `PB1` (CH4).
* **`TIM4` (Right Track)**: Pins `PB6` (CH1), `PB7` (CH2), `PB8` (CH3), `PB9` (CH4).
* **`TIM2` (Axis 3 / Auxiliary Drive)**: Pins `PA0` (CH1), `PA1` (CH2), `PB10` (CH3), `PC4` (CH4).

PWM Formula ($f_{TIM} = 64\text{ MHz}$ on APB1):
$$f_{PWM} = \frac{f_{TIM}}{(PSC + 1) \cdot (ARR + 1)} = \frac{64\,000\,000}{(15 + 1) \cdot (999 + 1)} = 4000\text{ Hz (4.0 kHz)}$$
* **Duty Cycle Resolution**: $0 \dots 999$ (0 = Stop, 999 = 100% duty cycle).
* **Directional Logic**:
  * Forward: $CCR_{fwd} = PWM$, $CCR_{rev} = 0$.
  * Reverse: $CCR_{fwd} = 0$, $CCR_{rev} = PWM$.
  * Neutral Stop: $CCR_{fwd} = 0$, $CCR_{rev} = 0$.

#### 3.2. Radio Bridge Link (`USART2`)
* Pins: `PA2` (TX), `PA3` (RX). Connected cross-over to ESP32-C6 (`GPIO20` RX, `GPIO21` TX).
* Baudrate: 115200 8N1. NVIC Preempt Priority = 5.
* Session Handshake: ESP32 sends command `0xA5` + 32-bit Session ID. STM32 replies with `0xA4A3A2A1` (`STM32_ACK_MAGIC`).
* Encryption: Frames decrypted using SPECK-64/128 block cipher with anti-replay rolling counter validation.

#### 3.3. Sensor Bus `I2C3` (IMU & Power Monitor)
* Pins: `PA8` (SCL), `PC9` (SDA). Open-drain with pull-ups. Clock: 100 kHz (`TIMINGR = 0x10B0DCFB`).
* **LSM6DS IMU**:
  * Primary Address: `0x6A` (HAL shifted: `0xD4`), Secondary Address: `0x6B` (HAL shifted: `0xD6`).
  * Configured for 104 Hz, $\pm2g$ scale (`CTRL1_XL = 0x40`).
* **INA219 / INA226 Power Monitor**:
  * Connected in parallel on `PA8`/`PC9`. Address `0x40` (A0=GND, A1=GND). Zero address conflict with IMU.

#### 3.4. Radar Interface (`USART1` + GPDMA1)
* Pins: `PA9` (TX), `PA10` (RX). Baudrate: 115200 8N1.
* DMA: `GPDMA1_Channel0` linked to `huart1.hdmarx`.
* Transfer Mode: Circular memory transfer triggered by `HAL_UARTEx_ReceiveToIdle_DMA` for instant packet boundary detection.

#### 3.5. GPS Navigation Interface (`UART4`)
* Pins: `PC10` (TX), `PC11` (RX).
* Automatic baudrate detection: 115200 $\rightarrow$ 9600 $\rightarrow$ 38400 baud.
* Byte-by-byte interrupt parsing filtering NMEA-0183 strings (`$` to `\r`/`\n`).

#### 3.6. Optical Wheel Encoders (`EXTI14` / `EXTI15`)
* Left Encoder: `PE14`, Right Encoder: `PE15`.
* Edge: `GPIO_MODE_IT_RISING`, pull-up enabled. Software debounce filter: 10 ms. 20 pulses per wheel revolution.

---

## [УКРАЇНСЬКА ВЕРСІЯ]

### 1. Загальні характеристики

* **Мікроконтролер**: STM32H562VGT6
* **Архітектура**: 32-бітний ARM Cortex-M33 з апаратним FPU одинарної точності та інструкціями DSP
* **Максимальна частота ядра**: до 250 МГц (базова прошивка UGV: 64 МГц від внутрішнього генератора HSI без PLL, дільники AHB/APB = 1)
* **Корпус**: LQFP100 (14 × 14 мм, 100 виводів, крок 0.5 мм)
* **Напруга живлення ядра та периферії**: 3.3 В (робочий діапазон $V_{DD}$ від 1.71 В до 3.6 В)
* **Режим регулятора напруги**: VOS0 (максимальна швидкодія)
* **Системний квантувач часу**: Таймер `TIM6` (шина APB1, період 1 мс, 1000 Гц переривання `HAL_IncTick`)

---

### 2. Повна таблиця розподілу виводів (Pinout LQFP100)

Усі виводи суворо відповідають офіційному документу STMicroelectronics **`DS14258 Rev 6`**:

| Вивід LQFP100 | Назва виводу | Роль у системі UGV | Режим GPIO / Альтернативна функція | Швидкість / Підтяжка | Цільовий підключений пристрій |
| :---: | :---: | :--- | :--- | :--- | :--- |
| **23** | **PA0** | TIM2_CH1 (Вісь 3, Мотор 1+) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Драйвер моторів 3-ї осі (ШІМ Fwd 1) |
| **24** | **PA1** | TIM2_CH2 (Вісь 3, Мотор 1-) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Драйвер моторів 3-ї осі (ШІМ Rev 1) |
| **25** | **PA2** | USART2_TX (Радіоміст) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART2`) | High / PullUp | ESP32-C6 (`GPIO20` / RX) |
| **26** | **PA3** | USART2_RX (Радіоміст) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART2`) | High / PullUp | ESP32-C6 (`GPIO21` / TX) |
| **31** | **PA6** | TIM3_CH1 (Лівий борт, Мотор 1+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Драйвер лівого борту (ШІМ Fwd 1) |
| **32** | **PA7** | TIM3_CH2 (Лівий борт, Мотор 1-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Драйвер лівого борту (ШІМ Rev 1) |
| **67** | **PA8** | I2C3_SCL (Шина сенсорів) | `GPIO_MODE_AF_OD`, **AF4** (`GPIO_AF4_I2C3`) | High / PullUp | LSM6DS (SCL), INA219/226 (SCL) |
| **68** | **PA9** | USART1_TX (Радар / Лідар) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART1`) | High / PullUp | Модуль радара / лідара (RX) |
| **69** | **PA10** | USART1_RX (Радар / DMA) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART1`) | High / PullUp | Модуль радара / лідара (TX) |
| **72** | **PA13** | SWDIO (Налагодження ST-Link) | Виділений інтерфейс SWD Debug | High / PullUp | Програматор ST-Link V2/V3 |
| **76** | **PA14** | SWCLK (Налагодження ST-Link) | Виділений інтерфейс SWD Debug | High / PullDown | Програматор ST-Link V2/V3 |
| **35** | **PB0** | TIM3_CH3 (Лівий борт, Мотор 2+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Драйвер лівого борту (ШІМ Fwd 2) |
| **36** | **PB1** | TIM3_CH4 (Лівий борт, Мотор 2-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM3`) | Low / NoPull | Драйвер лівого борту (ШІМ Rev 2) |
| **37** | **PB2** | Board Heartbeat LED | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Бортовий світлодіод (Active LOW) |
| **92** | **PB6** | TIM4_CH1 (Правий борт, Мотор 1+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Драйвер правого борту (ШІМ Fwd 1) |
| **93** | **PB7** | TIM4_CH2 (Правий борт, Мотор 1-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Драйвер правого борту (ШІМ Rev 1) |
| **95** | **PB8** | TIM4_CH3 (Правий борт, Мотор 2+) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Драйвер правого борту (ШІМ Fwd 2) |
| **96** | **PB9** | TIM4_CH4 (Правий борт, Мотор 2-) | `GPIO_MODE_AF_PP`, **AF2** (`GPIO_AF2_TIM4`) | Low / NoPull | Драйвер правого борту (ШІМ Rev 2) |
| **47** | **PB10** | TIM2_CH3 (Вісь 3, Мотор 2+) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Драйвер моторів 3-ї осі (ШІМ Fwd 2) |
| **33** | **PC4** | TIM2_CH4 (Вісь 3, Мотор 2-) | `GPIO_MODE_AF_PP`, **AF1** (`GPIO_AF1_TIM2`) | Low / NoPull | Драйвер моторів 3-ї осі (ШІМ Rev 2) |
| **63** | **PC6** | USART6_TX (Телеметрія / Лог) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART6`) | High / PullUp | USB-UART термінал / Логер |
| **64** | **PC7** | USART6_RX (Телеметрія / Лог) | `GPIO_MODE_AF_PP`, **AF7** (`GPIO_AF7_USART6`) | High / PullUp | USB-UART термінал / Логер |
| **66** | **PC9** | I2C3_SDA (Шина сенсорів) | `GPIO_MODE_AF_OD`, **AF4** (`GPIO_AF4_I2C3`) | High / PullUp | LSM6DS (SDA), INA219/226 (SDA) |
| **78** | **PC10** | UART4_TX (GPS Модуль) | `GPIO_MODE_AF_PP`, **AF8** (`GPIO_AF8_UART4`) | High / PullUp | Модуль GPS навігації (RX) |
| **79** | **PC11** | UART4_RX (GPS Модуль) | `GPIO_MODE_AF_PP`, **AF8** (`GPIO_AF8_UART4`) | High / PullUp | Модуль GPS навігації (TX) |
| **1** | **PE2** | RGB LED (Червоний канал) | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Бортовий RGB LED (Active HIGH) |
| **2** | **PE3** | RGB LED (Зелений канал) | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Бортовий RGB LED (Active HIGH) |
| **3** | **PE4** | RGB LED (Синій канал) | `GPIO_MODE_OUTPUT_PP` | Low / NoPull | Бортовий RGB LED (Active HIGH) |
| **45** | **PE14** | EXTI14 (Енкодер лівого колеса) | `GPIO_MODE_IT_RISING` | Low / PullUp | Оптопара лівого колеса |
| **46** | **PE15** | EXTI15 (Енкодер правого колеса)| `GPIO_MODE_IT_RISING` | Low / PullUp | Оптопара правого колеса |

---

### 3. Конфігурація підсистем

#### 3.1. Тракт керування моторами (ШІМ 4.0 кГц)
* **`TIM3` (Лівий борт)**: Виводи `PA6` (CH1), `PA7` (CH2), `PB0` (CH3), `PB1` (CH4).
* **`TIM4` (Правий борт)**: Виводи `PB6` (CH1), `PB7` (CH2), `PB8` (CH3), `PB9` (CH4).
* **`TIM2` (3-я вісь / Додатковий привід)**: Виводи `PA0` (CH1), `PA1` (CH2), `PB10` (CH3), `PC4` (CH4).

Розрахунок частоти ($f_{TIM} = 64\text{ МГц}$ на шині APB1):
$$f_{PWM} = \frac{f_{TIM}}{(PSC + 1) \cdot (ARR + 1)} = \frac{64\,000\,000}{(15 + 1) \cdot (999 + 1)} = 4000\text{ Гц (4.0 кГц)}$$
* **Роздільна здатність шпаруватості**: $0 \dots 999$ (0 = Мотор вимкнено, 999 = 100% потужності).
* **Логіка напрямку обертання**:
  * Вперед: $CCR_{fwd} = PWM$, $CCR_{rev} = 0$.
  * Назад: $CCR_{fwd} = 0$, $CCR_{rev} = PWM$.
  * Нейтраль/Гальмо: $CCR_{fwd} = 0$, $CCR_{rev} = 0$.

#### 3.2. Інтерфейс радіомоста (`USART2`)
* Виводи: `PA2` (TX), `PA3` (RX). Підключені перехресно до ESP32-C6 (`GPIO20` RX, `GPIO21` TX).
* Швидкість: 115200 8N1. Пріоритет NVIC Preempt = 5.
* Рукостискання сесії: ESP32 надсилає команду `0xA5` та 32-бітний Session ID. STM32 відповідає маркером `0xA4A3A2A1` (`STM32_ACK_MAGIC`).
* Шифрування: Пакети розшифровуються блочним шифром SPECK-64/128 з контролем лічильника для захисту від повторних атак.

#### 3.3. Шина сенсорів `I2C3` (IMU та монітор живлення)
* Виводи: `PA8` (SCL), `PC9` (SDA). Відкритий колектор з підтяжкою. Частота: 100 кГц (`TIMINGR = 0x10B0DCFB`).
* **LSM6DS IMU**:
  * Основна адреса: `0x6A` (HAL зсув: `0xD4`), додаткова адреса: `0x6B` (HAL зсув: `0xD6`).
  * Налаштування: 104 Гц, шкала $\pm2g$ (`CTRL1_XL = 0x40`).
* **INA219 / INA226 (Датчик струму/напруги)**:
  * Підключається паралельно на виводи `PA8`/`PC9`. Базова адреса `0x40` (A0=GND, A1=GND). Конфлікт адрес відсутній.

#### 3.4. Інтерфейс радара (`USART1` + GPDMA1)
* Виводи: `PA9` (TX), `PA10` (RX). Швидкість: 115200 8N1.
* DMA: Канал `GPDMA1_Channel0`, прив'язаний до `huart1.hdmarx`.
* Робота: Кільцевий прийом у пам'ять за викликом `HAL_UARTEx_ReceiveToIdle_DMA` для миттєвої фіксації меж кадрів по Idle Line.

#### 3.5. Інтерфейс навігації GPS (`UART4`)
* Виводи: `PC10` (TX), `PC11` (RX).
* Автоматичний підбір швидкості: 115200 $\rightarrow$ 9600 $\rightarrow$ 38400 бод.
* Побайтовий прийом у кільцевий буфер з виділенням рядків NMEA-0183 (від `$` до `\r`/`\n`).

#### 3.6. Оптичні енкодери коліс (`EXTI14` / `EXTI15`)
* Лівий енкодер: `PE14`, правий енкодер: `PE15`.
* Режим: Наростаючий фронт `GPIO_MODE_IT_RISING`, внутрішня підтяжка. Програмний антибрязкіт: 10 мс. 20 імпульсів на 1 повний оберт колеса.
