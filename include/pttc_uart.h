#ifndef __PTT_UART_H__
#define __PTT_UART_H__
#include <pthread.h>
#include "serial_utils.h"
typedef struct pttc_s {
    struct {
        int f_complete;
        int f_on;
    } parser;

    struct {
        int idle_cnt;
        int probe_cnt;
    } prober;

    struct {
        int state;
    } fdtor; // flip detector
	
    pthread_t thread;
    void (*on_pttc_ptt)(int ptt);
} pttc_t;

void pttc_init( serial_t *serial, pttc_t *pttc, void (*cb)(int) );
void pttc_start( serial_t *serial, char *port_dev );
void pttc_end( serial_t *serial );
#endif
