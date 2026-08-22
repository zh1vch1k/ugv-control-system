#ifndef SOURCES_MODULES_EBYTE433_T20D_HPP_
#define SOURCES_MODULES_EBYTE433_T20D_HPP_

#include <cstdint>
#include <optional>
#include "packet.hpp"
#include "stm32f4xx_hal.h"

typedef struct {
	UART_HandleTypeDef* huart;
	GPIO_TypeDef*       auxPort;
	uint16_t            auxPin;
    GPIO_TypeDef*       m0Port;
	uint16_t            m0Pin;
	GPIO_TypeDef*       m1Port;
	uint16_t            m1Pin;
} EbyteConfig;

typedef struct {
	EbyteConfig cfg;
	uint8_t address[2];
	uint8_t channel;
} EbyteDriver;


#pragma pack(push, 1)

typedef struct {
    uint8_t air_data_rate : 3; // 000=0.3k, 001=1.2k, 010=2.4k(def), 011=4.8k, 100=9.6k, 101=19.2k
    uint8_t uart_baud_rate: 3; // 000=1200, 011=9600(def), 111=115200
    uint8_t uart_parity   : 2; // 00=8N1(def), 01=8O1, 10=8E1
} EbyteSpeed_t;

typedef struct {
    uint8_t tx_power      : 2; // 00=20dBm(def), 01=17dBm, 10=14dBm, 11=10dBm
    uint8_t fec           : 1; // 1=Forward Error Correction On(def), 0=Off
    uint8_t wireless_time : 3; // Awake time в Mode 1 (000=250ms def)
    uint8_t drive_mode    : 1; // 1=Push-Pull AUX(def), 0=Open-Drain
    uint8_t fixed_mode    : 1; // 1=Fixed-point TX, 0=Transparent(def)
} EbyteOption_t;

typedef struct {
    uint8_t       head;
    uint8_t       addh;
    uint8_t       addl;
    EbyteSpeed_t  speed;
    uint8_t       chan;
    EbyteOption_t option;
} EbyteSettings_t;


enum class EbyteMode{
	Normal = 0,
	WakeUp = 1,
	PowerSaving = 2,
	Config = 3
} ;

#pragma pack(pop)


//changeSettings();

bool changeMode(EbyteMode mode, const EbyteConfig& settings, uint32_t timeout_ms = 100);

bool waitForAuxHigh(GPIO_TypeDef* auxPort, uint16_t auxPin, uint32_t timeout_ms = 100);

std::optional<LoraRxFrame_t> readLoRa(const EbyteConfig& cfg, uint32_t timeout_ms = 100);

std::optional<EbyteSettings_t> EbyteCfg(const EbyteConfig& cfg, uint32_t timeout_ms = 100);

void sendTelemetry();

#endif /* SOURCES_MODULES_EBYTE433_T20D_HPP_ */
