#include <stdint.h>

/* Заглушки Secure Context для роботи без TrustZone */
void SecureContext_SaveContext( void ) {}
void SecureContext_LoadContext( void ) {}
void vPortFreeSecureContext( uint32_t *pulSecureContext )
{
    (void)pulSecureContext;
}

uint32_t xSecureContext = 0;




