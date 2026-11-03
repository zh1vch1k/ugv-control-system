# ESP32-C6 Microcontroller Specification (UGV Onboard Radio Bridge)
# Специфікація мікроконтролера ESP32-C6 (Бортовий радіоміст UGV)

---

## [ENGLISH VERSION]

### 1. General Specifications

* **Microcontroller**: ESP32-C6 (ESP32-C6-WROOM-1 module / DevKitC-1 development board)
* **Core**: Single-core 32-bit RISC-V microprocessor clocked up to 160 MHz
* **Memory**: 512 KB SRAM, 320 KB ROM, 4 MB embedded SPI Flash
* **Wireless Capabilities**: 2.4 GHz Wi-Fi 6 (802.11ax), Bluetooth 5 (LE), IEEE 802.15.4 (Zigbee / Thread)
* **Operating System**: ESP-IDF FreeRTOS (SMP)
* **Subsystem Role**: Hardware wireless gateway. Receives LoRa telemetry packets from the Ebyte E32 module, manages the communication session FSM, and forwards sanitized control commands to the STM32H562 platform controller via high-speed UART.

---

### 2. Pinout Table (ESP32-C6-DevKitC-1)

| GPIO Pin | Function / Peripheral | Role in Radio Bridge | Voltage Level | Connected External Node |
| :---: | :--- | :--- | :---: | :--- |
| **GPIO20** | `UART0_RXD` | UART RX from STM32 | 3.3V (LVTTL) | Connects to `PA2` (`USART2_TX`) of STM32H562 |
| **GPIO21** | `UART0_TXD` | UART TX to STM32 | 3.3V (LVTTL) | Connects to `PA3` (`USART2_RX`) of STM32H562 |
| **GPIO6** | `UART1_TXD` | UART TX to LoRa Module | 3.3V (LVTTL) | Connects to `RXD` of Ebyte E32 Module |
| **GPIO7** | `UART1_RXD` | UART RX from LoRa Module | 3.3V (LVTTL) | Connects to `TXD` of Ebyte E32 Module |
| **GPIO4** | `GPIO_Output` | Ebyte Mode Select M0 | 3.3V | Connects to `M0` pin of Ebyte E32 |
| **GPIO5** | `GPIO_Output` | Ebyte Mode Select M1 | 3.3V | Connects to `M1` pin of Ebyte E32 |
| **GPIO14** | `GPIO_Input` (PullUp)| Ebyte Busy/Status (AUX)| 3.3V | Connects to `AUX` pin of Ebyte E32 |
| **GPIO8** | `RMT` (WS2812) | Addressable RGB Data Line| 3.3V | Onboard RGB LED on ESP32-C6-DevKitC-1 |
| **GND** | Ground | Common System Ground | 0V | Ground reference for STM32 and Ebyte |
| **3V3 / 5V**| Power Input | Board Power Supply | 3.3V / 5.0V | Regulated 5V / 3.3V DC-DC power rail |

---

### 3. Hardware Interface Specifications

#### 3.1. STM32H562 Core Link (`UART0`)
* **Port**: `UART_NUM_0`
* **Pins**: `GPIO21` (TX), `GPIO20` (RX).
* **Parameters**: 115200 baud, 8 data bits, no parity, 1 stop bit (8N1).
* **Buffer Size**: 2048 bytes ring buffer (`UART_BUF_SIZE * 2`).
* **Packet Forwarding**: Incoming 8-byte RFBP frames received from LoRa are immediately transferred to `UART0`. Handshake acknowledgements (`STM32_ACK_MAGIC = 0xA4A3A2A1`) from STM32 update the FSM to `RadioFSM::ACTIVE`.

#### 3.2. Ebyte E32 LoRa Module Interface (`UART1` + GPIO)
* **Port**: `UART_NUM_1`
* **Pins**: `GPIO6` (TX to Ebyte RX), `GPIO7` (RX from Ebyte TX).
* **Baudrate**: 9600 baud, 8N1 (default factory setting for Ebyte E32 transparent mode).
* **Control Pins**:
  * `M0_PIN` (`GPIO4`) and `M1_PIN` (`GPIO5`):
    * **Mode 0 (Normal)**: `M0 = 0`, `M1 = 0` — Transparent over-the-air transmission/reception active.
    * **Mode 1 (Wake-up)**: `M0 = 1`, `M1 = 0` — Adds preamble for wake-up.
    * **Mode 2 (Power Save)**: `M0 = 0`, `M1 = 1` — Air wake-up receive only.
    * **Mode 3 (Sleep / Config)**: `M0 = 1`, `M1 = 1` — Module parameter configuration mode.
  * `AUX_PIN` (`GPIO14`): Module busy line. `AUX = 0` indicates RF transmission in progress or internal buffer busy; `AUX = 1` indicates ready state.

#### 3.3. Status Addressable RGB LED (WS2812 / `GPIO8`)
Driven by the ESP32-C6 **RMT** (Remote Control Peripheral) operating at 10 MHz:
* **Pin**: `GPIO8`
* **Status Color Mapping**:
  * **Blue (`Idle`)**: Waiting for handshake / remote connection.
  * **Orange (`HandshakePending`)**: Handshake request sent, waiting for STM32 reply.
  * **Mint / Green (`SessionValid`)**: Session authenticated, control stream active.
  * **Cyan (`LoraRx`)**: Receiving LoRa radio packet.
  * **Magenta (`LoraTx`)**: Transmitting packet.
  * **Red (`Error`)**: Loss of signal, packet watchdog timeout, or hardware fault.

---

### 4. FreeRTOS Task Architecture

1. **`radio_bridge_task`** (Priority 10, Stack 4096 bytes):
   * Streams raw bytes from `UART1` (Ebyte LoRa).
   * Validates frame boundaries (8-byte packets).
   * Detects handshake command (`0xA5`) and updates `RadioFSM` state.
   * Forwards valid packets to `UART0` (STM32).
   * Implements a 2000 ms watchdog timer for fail-safe fallback.

2. **`led_control_task`** (Priority 2, Stack 2048 bytes):
   * Cycles every 50 ms to update WS2812 LED color according to the atomic `state`.

---

## [УКРАЇНСЬКА ВЕРСІЯ]

### 1. Загальні характеристики

* **Мікроконтролер**: ESP32-C6 (модуль ESP32-C6-WROOM-1 / плата DevKitC-1)
* **Ядро**: Одноядерний 32-бітний мікропроцесор RISC-V з тактовою частотою до 160 МГц
* **Пам'ять**: 512 КБ SRAM, 320 КБ ROM, 4 МБ вбудованої Flash
* **Бездротові інтерфейси**: 2.4 GHz Wi-Fi 6 (802.11ax), Bluetooth 5 (LE), IEEE 802.15.4 (Zigbee / Thread)
* **Операційна система**: ESP-IDF FreeRTOS (SMP)
* **Роль підсистеми**: Апаратний бездротовий шлюз. Забезпечує прийом пакетів LoRa з модуля Ebyte E32, обробку FSM сесії зв'язку та трансляцію команд у контролер платформи STM32H562 по швидкісному UART.

---

### 2. Таблиця розподілу виводів (ESP32-C6-DevKitC-1)

| Вивід GPIO | Функція / Периферія | Роль у радіомості | Рівень напруги | Підключення до зовнішнього вузла |
| :---: | :--- | :--- | :---: | :--- |
| **GPIO20** | `UART0_RXD` | Вхід UART зв'язку зі STM32 | 3.3 В (LVTTL) | Підключається до `PA2` (`USART2_TX`) STM32H562 |
| **GPIO21** | `UART0_TXD` | Вихід UART зв'язку зі STM32 | 3.3 В (LVTTL) | Підключається до `PA3` (`USART2_RX`) STM32H562 |
| **GPIO6** | `UART1_TXD` | Передача в модуль LoRa Ebyte | 3.3 В (LVTTL) | Підключається до `RXD` модуля Ebyte E32 |
| **GPIO7** | `UART1_RXD` | Прийом з модуля LoRa Ebyte | 3.3 В (LVTTL) | Підключається до `TXD` модуля Ebyte E32 |
| **GPIO4** | `GPIO_Output` | Керування режимом Ebyte (M0) | 3.3 В | Підключається до виводу `M0` модуля Ebyte E32 |
| **GPIO5** | `GPIO_Output` | Керування режимом Ebyte (M1) | 3.3 В | Підключається до виводу `M1` модуля Ebyte E32 |
| **GPIO14** | `GPIO_Input` (PullUp)| Контроль зайнятості (AUX) | 3.3 В | Підключається до виводу `AUX` модуля Ebyte E32 |
| **GPIO8** | `RMT` (WS2812) | Лінія даних адресного RGB | 3.3 В | Вбудований RGB LED на платі DevKitC-1 |
| **GND** | Заземлення | Спільна сигнальна земля | 0 В | Спільна земля для STM32 та Ebyte |
| **3V3 / 5V**| Вхід живлення | Напруга живлення плати | 3.3 В / 5.0 В | Стабілізована шина 5V / 3.3V DC-DC |

---

### 3. Опис апаратних інтерфейсів

#### 3.1. Лінк зі STM32H562 (`UART0`)
* **Порт**: `UART_NUM_0`
* **Виводи**: `GPIO21` (TX), `GPIO20` (RX).
* **Параметри**: 115200 бод, 8 біт даних, без парності, 1 стоп-біт (8N1).
* **Розмір буфера**: 2048 байт (`UART_BUF_SIZE * 2`).
* **Логіка передачі**: Прийняті з ефіру 8-байтні пакети RFBP негайно передаються в `UART0`. Відповіді підтвердження сесії (`STM32_ACK_MAGIC = 0xA4A3A2A1`) від STM32 перемикають автомат станів у `RadioFSM::ACTIVE`.

#### 3.2. Інтерфейс радіомодуля LoRa Ebyte E32 (`UART1` + GPIO)
* **Порт**: `UART_NUM_1`
* **Виводи**: `GPIO6` (TX до Ebyte RX), `GPIO7` (RX від Ebyte TX).
* **Швидкість**: 9600 бод, 8N1 (заводське налаштування прозорого режиму Ebyte E32).
* **Лінії керування**:
  * `M0_PIN` (`GPIO4`) та `M1_PIN` (`GPIO5`):
    * **Режим 0 (Нормальний)**: `M0 = 0`, `M1 = 0` — Бездротовий канал активний, прозорий прийом та передача.
    * **Режим 1 (Пробудження)**: `M0 = 1`, `M1 = 0` — Додавання преамбули пробудження.
    * **Режим 2 (Енергозбереження)**: `M0 = 0`, `M1 = 1` — Прийом лише по преамбулі.
    * **Режим 3 (Сон / Налаштування)**: `M0 = 1`, `M1 = 1` — Режим AT-команд та конфігурації.
  * `AUX_PIN` (`GPIO14`): Лінія зайнятості буфера. `AUX = 0` — модуль виконує передачу або обробку даних; `AUX = 1` — буфер вільний для запису.

#### 3.3. Адресний статусний світлодіод (WS2812 / `GPIO8`)
Керування реалізовано через блок **RMT** на базовій частоті 10 МГц:
* **Вивід**: `GPIO8`
* **Колірна індикація станів**:
  * **Синій (`Idle`)**: Очікування підключення / рукостискання пульта.
  * **Помаранчевий (`HandshakePending`)**: Надіслано запит сесії, очікування відповіді від STM32.
  * **М'ятний / Зелений (`SessionValid`)**: Сесія активна, стабільний потік керування.
  * **Блакитний (`LoraRx`)**: Прийом радіопакета LoRa.
  * **Пурпурний (`LoraTx`)**: Відправка пакета в ефір.
  * **Червоний (`Error`)**: Втрата сигналу, таймаут сесії або апаратна помилка.

---

### 4. Архітектура задач FreeRTOS

1. **`radio_bridge_task`** (Пріоритет: 10, Стек: 4096 байт):
   * Вичитує потік байт з `UART1` (Ebyte LoRa).
   * Перевіряє цілісність кадрів (8 байт).
   * Фіксує команду рукостискання (`0xA5`) та синхронізує стан `RadioFSM`.
   * Транслює валідні пакети в `UART0` (STM32).
   * Відстежує таймаут надходження даних (Watchdog 2000 мс).

2. **`led_control_task`** (Пріоритет: 2, Стек: 2048 байт):
   * Кожні 50 мс оновлює колір світлодіода WS2812 відповідно до стану `state`.
