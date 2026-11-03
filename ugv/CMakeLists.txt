cmake_minimum_required(VERSION 3.20)

###################### CONSTANTS ######################################
set (PROJECT_TYPE_EXECUTABLE          "exe")
set (PROJECT_TYPE_STATIC_LIBRARY      "static-lib")
set (MCPU_CORTEX_M33                  "-mcpu=cortex-m33")
set (MFPU_FPV5_D16                    "-mfpu=fpv5-sp-d16")
set (RUNTIME_LIBRARY_REDUCED_C        "--specs=nano.specs")
set (RUNTIME_LIBRARY_SYSCALLS_MINIMAL "--specs=nosys.specs")
set (MFLOAT_ABI_HARDWARE              "-mfloat-abi=hard")
#######################################################################

###################### VARIABLES ######################################
set (PROJECT_NAME             "ugv")
set (PROJECT_TYPE             "exe")
set (LINKER_SCRIPT            "${CMAKE_CURRENT_SOURCE_DIR}/STM32H562VGTX_FLASH.ld")
set (MCPU                     "-mcpu=cortex-m33")
set (MFPU                     "-mfpu=fpv5-sp-d16")
set (MFLOAT_ABI               "-mfloat-abi=hard")
set (RUNTIME_LIBRARY          "--specs=nano.specs")
set (RUNTIME_LIBRARY_SYSCALLS "--specs=nosys.specs")

set (PROJECT_SOURCES
    Startup/startup_stm32h562vgtx.s
    Sources/main.cpp
    Sources/msp.cpp
    Sources/it.cpp
    Sources/syscalls.c
    Sources/sysmem.c
    Sources/Modules/Led/led_config.cpp
    Sources/Modules/Optopair/wheel_encoder.cpp
    Sources/Modules/LSM6DS/lsm6ds.cpp
    Sources/Modules/Protocol/I2C/STM32_adapter.cpp
    Sources/Modules/Protocol/DMA/dma_manager.cpp
)

set (PROJECT_DEFINES
    STM32H562xx
)

################## PROJECT SETUP ######################################
project(${PROJECT_NAME} C CXX ASM)

if (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_EXECUTABLE})
  add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)
elseif (${PROJECT_TYPE} MATCHES ${PROJECT_TYPE_STATIC_LIBRARY})
  add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

add_compile_definitions(${PROJECT_DEFINES} USE_HAL_DRIVER)

set (CMAKE_EXECUTABLE_SUFFIX ".elf")
set (CMAKE_STATIC_LIBRARY_SUFFIX ".a")

set (CPU_FLAGS "${MCPU} -mthumb ${MFPU} ${MFLOAT_ABI}")

set (CMAKE_C_FLAGS   "${CPU_FLAGS} -std=gnu11 ${RUNTIME_LIBRARY} -Wall")
set (CMAKE_CXX_FLAGS "${CPU_FLAGS} -std=gnu++17 ${RUNTIME_LIBRARY} -Wall -fno-exceptions -fno-rtti")
set (CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp")

set (CMAKE_EXE_LINKER_FLAGS "-T${LINKER_SCRIPT} ${RUNTIME_LIBRARY_SYSCALLS} -Wl,-Map=${PROJECT_NAME}.map -Wl,--gc-sections -static -Wl,--start-group -lc -lm -lstdc++ -Wl,--end-group")

target_include_directories(${PROJECT_NAME} PRIVATE
    Inc
    Sources
    Sources/Modules/Led
    Sources/Modules/Optopair
    Sources/Modules/LSM6DS
    Sources/Modules/Protocol/I2C
    Sources/Modules/Protocol/DMA
    Drivers/CMSIS/Core/Include
    Drivers/CMSIS/Device/ST/STM32H5xx/Include
    Drivers/STM32H5xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/rfbp/lib
    ThirdParty/FreeRTOS/include
    ThirdParty/FreeRTOS/portable/GCC
    ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    configENABLE_TRUSTZONE=0
    configRUN_FREERTOS_SECURE_ONLY=0
    configENABLE_MPU=0
    configENABLE_FPU=1
)

target_sources(${PROJECT_NAME} PRIVATE
    Drivers/CMSIS/Device/ST/STM32H5xx/Source/Templates/system_stm32h5xx.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_cortex.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pwr.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_pwr_ex.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rcc.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_rcc_ex.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_gpio.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dma.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_dma_ex.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_uart.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_uart_ex.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_tim.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_tim_ex.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i2c.c
    Drivers/STM32H5xx_HAL_Driver/Src/stm32h5xx_hal_i2c_ex.c
    
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/rfbp/src/packet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/rfbp/src/speck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/rfbp/src/fsm.cpp
    ThirdParty/FreeRTOS/tasks.c
    ThirdParty/FreeRTOS/queue.c
    ThirdParty/FreeRTOS/list.c
    ThirdParty/FreeRTOS/timers.c
    ThirdParty/FreeRTOS/event_groups.c
    ThirdParty/FreeRTOS/stream_buffer.c
	ThirdParty/FreeRTOS/portable/MemMang/heap_4.c
    ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
    ThirdParty/FreeRTOS/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
    Sources/freertos_stubs.c
)