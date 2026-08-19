cmake_minimum_required(VERSION 3.22)

# Название проекта
project(ugv_firmware C CXX ASM)

# Стандарт C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Флаги архитектуры Cortex-M4 с аппаратным FPU для Black Pill
set(CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

add_compile_options(
    ${CPU_FLAGS}
    -Wall
    -Wextra
    -fdata-sections
    -ffunction-sections
)

# Обязательные дефайны для HAL и конкретного чипа
add_definitions(
    -DSTM32F411xE
    -DUSE_HAL_DRIVER
)

# Исходники проекта и библиотеки HAL
set(SOURCES
    Sources/main.cpp
    Sources/syscalls.c
    Sources/sysmem.c
    Startup/startup_stm32f411ceux.s
    
    Drivers/cmsis_device_f4/Source/Templates/system_stm32f4xx.c

    # HAL Core & Drivers
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Пути к заголовочным файлам
target_include_directories(${PROJECT_NAME} PRIVATE
    Includes
    Sources
    Drivers/STM32F4xx_HAL_Driver/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    Drivers/cmsis_device_f4/Include
    Drivers/CMSIS_5/CMSIS/Core/Include
)

# Флаги линковщика
target_link_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32F411CEUX_FLASH.ld
    --specs=nosys.specs
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--gc-sections
    -static
)