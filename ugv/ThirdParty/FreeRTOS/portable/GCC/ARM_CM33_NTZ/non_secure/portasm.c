#include <stdint.h>
#include "FreeRTOSConfig.h"
#include "portasm.h"

extern void vTaskSwitchContext( void );
extern uint32_t *pxCurrentTCB;

__attribute__(( naked )) void vRestoreContextOfFirstTask( void )
{
    __asm volatile
    (
        "   ldr r2, pxCurrentTCBConst2      \n"
        "   ldr r1, [r2]                    \n"
        "   ldr r0, [r1]                    \n"
        "   ldmia r0!, {r4-r11}             \n"
        "   msr psp, r0                     \n"
        "   isb                             \n"
        "   mov r0, #0                      \n"
        "   msr basepri, r0                 \n"
        "   bx r14                          \n"
        "                                   \n"
        "   .align 4                        \n"
        "pxCurrentTCBConst2: .word pxCurrentTCB\n"
    );
}

__attribute__(( naked )) void vStartFirstTask( void )
{
    __asm volatile
    (
        "   ldr r0, =0xE000ED08             \n" /* VTOR */
        "   ldr r0, [r0]                    \n"
        "   ldr r0, [r0]                    \n"
        "   msr msp, r0                     \n"
        "   cpsie i                         \n"
        "   cpsie f                         \n"
        "   dsb                             \n"
        "   isb                             \n"
        "   svc 0                           \n" /* Виклик SVC для запуску першої задачі */
        "   nop                             \n"
        "   .ltorg                          \n"
    );
}

__attribute__(( naked )) void PendSV_Handler( void )
{
    __asm volatile
    (
        "   mrs r0, psp                     \n"
        "   isb                             \n"
        "                                   \n"
        "   ldr r3, pxCurrentTCBConst       \n" /* r3 = &pxCurrentTCB */
        "   ldr r2, [r3]                    \n" /* r2 = pxCurrentTCB */
        "                                   \n"
        "   stmdb r0!, {r4-r11}             \n" /* Збереження регістрів */
        "   str r0, [r2]                    \n" /* Збереження PSP у TCB */
        "                                   \n"
        "   stmdb sp!, {r3, r14}            \n"
        "   mov r0, %0                      \n"
        "   msr basepri, r0                 \n"
        "   dsb                             \n"
        "   isb                             \n"
        "   bl vTaskSwitchContext           \n"
        "   mov r0, #0                      \n"
        "   msr basepri, r0                 \n"
        "   ldmia sp!, {r3, r14}            \n"
        "                                   \n"
        "   ldr r1, [r3]                    \n" /* Новий TCB */
        "   ldr r0, [r1]                    \n" /* Новий PSP */
        "   ldmia r0!, {r4-r11}             \n" /* Відновлення регістрів */
        "   msr psp, r0                     \n"
        "   isb                             \n"
        "   bx r14                          \n"
        "                                   \n"
        "   .align 4                        \n"
        "pxCurrentTCBConst: .word pxCurrentTCB \n"
        :: "i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
    );
}

__attribute__(( naked )) void SVC_Handler( void )
{
    __asm volatile
    (
        "   tst lr, #4                      \n"
        "   ite eq                          \n"
        "   mrseq r0, msp                   \n"
        "   mrsne r0, psp                   \n"
        "   ldr r1, [r0, #24]               \n"
        "   ldrb r2, [r1, #-2]              \n"
        "   cmp r2, #0                      \n"
        "   bne vPortSVCHandler_C           \n"
        "   b vRestoreContextOfFirstTask    \n"
    );
}

uint32_t ulSetInterruptMask( void )
{
    uint32_t ulMask;
    __asm volatile
    (
        "   mrs %0, basepri                 \n"
        "   mov r1, %1                      \n"
        "   msr basepri, r1                 \n"
        "   isb                             \n"
        : "=r" ( ulMask )
        : "i" ( configMAX_SYSCALL_INTERRUPT_PRIORITY )
        : "r1", "memory"
    );
    return ulMask;
}

void vClearInterruptMask( uint32_t ulMask )
{
    __asm volatile
    (
        "   msr basepri, %0                 \n"
        "   isb                             \n"
        :: "r" ( ulMask )
        : "memory"
    );
}
