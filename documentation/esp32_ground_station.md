# ESP32 Microcontroller Specification (Ground Station Remote Controller)
# Специфікація мікроконтролера ESP32 (Пульт керування / Ground Station)

---

## [ENGLISH VERSION]

### 1. General Specifications

* **Microcontroller**: ESP32-D0WDQ6 (integrated on the LilyGO TTGO LoRa32 V1 board)
* **Core**: Dual-core 32-bit Xtensa LX6 microprocessor clocked up to 240 MHz
* **Memory**: 520 KB SRAM, 448 KB ROM, 4 MB external SPI Flash
* **Integrated Board Peripherals**:
  * Semtech SX1276 / SX1278 LoRa transceiver (SPI bus)
  * 0.96" 128×64 monochrome OLED display (SSD1306 on I2C bus)
  * TP4054 single-cell LiPo/Li-Ion battery charge management IC
* **Subsystem Role**: Operator Ground Station Transmitter. Reads raw analog and digital inputs from a physical gamepad/controller via RS-485, encapsulates data into the RFBP framing format, encrypts payload using the SPECK-64/128 block cipher, and transmits continuous radio packets via LoRa.

---

### 2. Pinout Table (TTGO LoRa32 V1)

| ESP32 Pin | Peripheral Block | Role on Board | Voltage Level | Connected Component |
| :---: | :--- | :--- | :---: | :--- |
| **GPIO27** | `SPI_MOSI` | SPI Data Out to LoRa | 3.3V | SX1276/78 `MOSI` |
| **GPIO19** | `SPI_MISO` | SPI Data In from LoRa | 3.3V | SX1276/78 `MISO` |
| **GPIO5** | `SPI_SCLK` | SPI Clock Line | 3.3V | SX1276/78 `SCK` |
| **GPIO18** | `GPIO_Output` | LoRa Chip Select (NSS) | 3.3V | SX1276/78 `NSS` (Active LOW) |
| **GPIO26** | `GPIO_Input` | LoRa Interrupt Line | 3.3V | SX1276/78 `DIO0` (TxDone / RxDone) |
| **GPIO23** | `GPIO_Output` | LoRa Hardware Reset | 3.3V | SX1276/78 `RESET` (Active LOW) |
| **GPIO21** | `I2C0_SDA` | OLED Display Data Line | 3.3V | SSD1306 OLED `SDA` |
| **GPIO22** | `I2C0_SCL` | OLED Display Clock Line | 3.3V | SSD1306 OLED `SCL` |
| **GPIO16** | `GPIO_Output` | OLED Display Reset | 3.3V | SSD1306 OLED `RST` |
| **GPIO13** | `UART2_RXD` | RS-485 Input (Gamepad) | 3.3V (LVTTL) | MAX3485 / SP3485 `RO` (Receiver Out) |
| **GPIO14** | `UART2_TXD` | RS-485 Output (Tx Reserve)| 3.3V (LVTTL) | MAX3485 / SP3485 `DI` (Driver In) |
| **GPIO15** | `GPIO_Output` | RS-485 Direction Control| 3.3V | MAX3485 `RE`/`DE` (held LOW for RX) |
| **GPIO1** | `UART0_TXD` | USB Serial Debug Out | 3.3V | Onboard CP2102 / CH9102 USB Bridge |
| **GPIO3** | `UART0_RXD` | USB Serial Programming In| 3.3V | Onboard CP2102 / CH9102 USB Bridge |

---

### 3. Hardware Interface & Protocol Specifications

#### 3.1. LoRa Modem (Semtech SX1276/78 via SPI)
* **Bus**: Hardware `VSPI` peripheral at 8.0 MHz clock rate.
* **RF Configuration**:
  * **Carrier Frequency**:
    * 868 MHz Band: $868.000\text{ MHz}$ (`RegFrf = 0xE4C000`).
    * 433 MHz Band: $433.000\text{ MHz}$ (`RegFrf = 0x6C8000`).
  * **Bandwidth (BW)**: 125 kHz (`RegModemConfig1 = 0x72`).
  * **Spreading Factor (SF)**: **SF9** (`RegModemConfig2 = 0x94`, CRC enabled).
  * **Coding Rate (CR)**: 4/5.
  * **Preamble Length**: **16 symbols** (`RegPreambleLsb = 0x10`) — matches the Ebyte E32 hardware receiver sync window.
  * **Sync Word**: `0x12` (Ebyte default network identifier).
  * **Output Power**: +14 $\dots$ +17 dBm (PA_BOOST output enabled).

#### 3.2. Gamepad Controller Input (`UART2` / RS-485)
* **Port**: `UART_NUM_2`
* **Pins**: `GPIO13` (RX), `GPIO14` (TX), `GPIO15` (`RE/DE`).
* **Direction Control**: `GPIO15` is pulled to logic 0 (`gpio_set_level(15, 0)`), placing the MAX3485 transceiver in permanent listen mode.
* **Baudrate**: 115200 baud, 8N1.
* **Packet Structure**:
  * Byte 0: Header marker `0xAA`.
  * Byte 1: Left Stick X (`st_l_ax_x`, 0..128..255).
  * Byte 2: Left Stick Y (`st_l_ax_y`).
  * Byte 3: Right Stick X (`st_r_ax_x`, Steering).
  * Byte 4: Right Stick Y (`st_r_ax_y`, Throttle Forward/Reverse).
  * Byte 5: D-Pad & Digital Buttons bitmask.
  * Byte 6: Mode & Trigger flags.
  * Byte 7: Checksum (CRC8).

#### 3.3. Graphical OLED Display SSD1306 (`I2C0`)
* **Port**: `I2C_NUM_0` (Fast Mode 400 kHz).
* **Pins**: `GPIO21` (SDA), `GPIO22` (SCL), `GPIO16` (Reset).
* **I2C Address**: `0x3C`.
* **Telemetry UI**: Displays active FSM session state, transmitted packet count, ACK confirmation count, throttle/steering percentage, and RF carrier frequency.

#### 3.4. SPECK-64/128 Cryptographic Pipeline
* **Cipher**: SPECK-64/128 block cipher (64-bit block size, 128-bit key).
* **Anti-Replay Mechanism**: 32-bit incremental session counter prevents playback attacks.

---

## [УКРАЇНСЬКА ВЕРСІЯ]

### 1. Загальні характеристики

* **Мікроконтролер**: ESP32-D0WDQ6 (у складі налагоджувальної плати LilyGO TTGO LoRa32 V1)
* **Ядро**: Двоядерний 32-бітний мікропроцесор Xtensa LX6 з тактовою частотою до 240 МГц
* **Пам'ять**: 520 КБ SRAM, 448 КБ ROM, 4 МБ зовнішньої SPI Flash
* **Інтегрована периферія плати**:
  * Трансивер LoRa Semtech SX1276 / SX1278 (шина SPI)
  * Графічний монохромний OLED-дисплей 0.96" 128×64 (SSD1306 на шині I2C)
  * Контролер заряду LiPo/Li-Ion акумуляторів (TP4054)
* **Роль підсистеми**: Наземна станція оператора (Ground Station Transmitter). Зчитує дані з фізичного геймпада/джойстика через інтерфейс RS-485, пакує їх у формат RFBP, шифрує алгоритмом SPECK-64/128 та транслює в радіоефір через LoRa.

---

### 2. Таблиця розподілу виводів (TTGO LoRa32 V1)

| Вивід ESP32 | Периферійний блок | Призначення на платі | Рівень напруги | Підключений компонент |
| :---: | :--- | :--- | :---: | :--- |
| **GPIO27** | `SPI_MOSI` | Передача даних SPI в LoRa | 3.3 В | SX1276/78 `MOSI` |
| **GPIO19** | `SPI_MISO` | Прийом даних SPI з LoRa | 3.3 В | SX1276/78 `MISO` |
| **GPIO5** | `SPI_SCLK` | Тактування шини SPI | 3.3 В | SX1276/78 `SCK` |
| **GPIO18** | `GPIO_Output` | Вибір чіпа LoRa (Chip Select) | 3.3 В | SX1276/78 `NSS` (Active LOW) |
| **GPIO26** | `GPIO_Input` | Переривання пакета LoRa | 3.3 В | SX1276/78 `DIO0` (TxDone / RxDone) |
| **GPIO23** | `GPIO_Output` | Апаратне скидання LoRa | 3.3 В | SX1276/78 `RESET` (Active LOW) |
| **GPIO21** | `I2C0_SDA` | Лінія даних дисплея | 3.3 В | SSD1306 OLED `SDA` |
| **GPIO22** | `I2C0_SCL` | Тактування дисплея | 3.3 В | SSD1306 OLED `SCL` |
| **GPIO16** | `GPIO_Output` | Апаратне скидання дисплея | 3.3 В | SSD1306 OLED `RST` |
| **GPIO13** | `UART2_RXD` | Вхід RS-485 (геймпад) | 3.3 В (LVTTL) | MAX3485 / SP3485 `RO` (Receiver Out) |
| **GPIO14** | `UART2_TXD` | Вихід RS-485 (передача) | 3.3 В (LVTTL) | MAX3485 / SP3485 `DI` (Driver In) |
| **GPIO15** | `GPIO_Output` | Керування напрямком RS-485 | 3.3 В | MAX3485 `RE`/`DE` (притягнуто до 0) |
| **GPIO1** | `UART0_TXD` | Налагодження USB-UART (TX) | 3.3 В | Вбудований міст CP2102 / CH9102 |
| **GPIO3** | `UART0_RXD` | Прошивка USB-UART (RX) | 3.3 В | Вбудований міст CP2102 / CH9102 |

---

### 3. Специфікація апаратних інтерфейсів та протоколів

#### 3.1. Радіомодем LoRa (Semtech SX1276/78 через SPI)
* **Шина**: Апаратний контролер `VSPI` на частоті 8.0 МГц.
* **Радіочастотні параметри**:
  * **Несуча частота**:
    * Діапазон 868 МГц: $868.000\text{ МГц}$ (`RegFrf = 0xE4C000`).
    * Діапазон 433 МГц: $433.000\text{ МГц}$ (`RegFrf = 0x6C8000`).
  * **Ширина смуги (BW)**: 125 кГц (`RegModemConfig1 = 0x72`).
  * **Фактор розширення спектра (SF)**: **SF9** (`RegModemConfig2 = 0x94`, контроль CRC увімкнено).
  * **Кодова швидкість (CR)**: 4/5.
  * **Довжина преамбули**: **16 символів** (`RegPreambleLsb = 0x10`) — забезпечує синхронізацію з Ebyte E32 на борті.
  * **Слово синхронізації (SyncWord)**: `0x12`.
  * **Вихідна потужність**: +14 $\dots$ +17 dBm (вихід `PA_BOOST`).

#### 3.2. Вхідний інтерфейс геймпада (`UART2` / RS-485)
* **Порт**: `UART_NUM_2`
* **Виводи**: `GPIO13` (RX), `GPIO14` (TX), `GPIO15` (`RE/DE`).
* **Керування лінією**: `GPIO15` притягнуто до 0 (`gpio_set_level(15, 0)`), що фіксує мікросхему MAX3485 у режимі постійного прийому.
* **Параметри**: 115200 бод, 8N1.
* **Структура пакета**:
  * Байт 0: Заголовок `0xAA`.
  * Байт 1: Лівий джойстик X (`st_l_ax_x`, 0..128..255).
  * Байт 2: Лівий джойстик Y (`st_l_ax_y`).
  * Байт 3: Правий джойстик X (`st_r_ax_x`, поворот).
  * Байт 4: Правий джойстик Y (`st_r_ax_y`, газ вперед/назад).
  * Байт 5: Кнопки D-Pad та цифрові входи.
  * Байт 6: Прапорці режимів.
  * Байт 7: Контрольна сума CRC8.

#### 3.3. Графічний OLED-дисплей SSD1306 (`I2C0`)
* **Порт**: `I2C_NUM_0` (Fast Mode 400 кГц).
* **Виводи**: `GPIO21` (SDA), `GPIO22` (SCL), `GPIO16` (Reset).
* **I2C адреса**: `0x3C`.
* **Відображення даних**: Статус FSM сесії, лічильники надісланих пакетів та ACK відповідей, відсоток відхилення джойстиків, частота радіоканалу.

#### 3.4. Криптографічний захист SPECK-64/128
* **Шифр**: SPECK-64/128 (блок 64 біти, ключ 128 біт).
* **Захист від повтору**: 32-бітний сесійний лічильник виключає виконання перехоплених команд.
