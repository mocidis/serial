#ifndef __PTT_UART_H__
#define __PTT_UART_H__
#include <pthread.h>
void pttc_init( void (*cb)(int) );
void pttc_start( char *port_dev, pthread_t *thread );
void pttc_end( pthread_t *thread );
#endif
