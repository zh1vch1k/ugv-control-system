#ifndef PORTASM_H
#define PORTASM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void vRestoreContextOfFirstTask( void );
void vStartFirstTask( void );

void vPortSVCHandler( void );
void xPortPendSVHandler( void );
void xPortSysTickHandler( void );

uint32_t ulSetInterruptMask( void );
void vClearInterruptMask( uint32_t ulMask );

#ifdef __cplusplus
}
#endif

#endif /* PORTASM_H */
