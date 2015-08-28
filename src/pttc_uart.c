#include <unistd.h>
#include <pthread.h>
#include "pttc_uart.h"
#include "serial_utils.h"
#include "ansi-utils.h"

static void on_pttc_data_received(serial_t *serial, char *buffer, int nbytes);
static void pttc_process_command(serial_t *serial, int fd);

/* Parser for PTT Card control */
#define PTTC_MAX_IDLE_CNT 10
#define PTTC_MAX_PROBE_CNT 3

static void pttc_parser_set_complete(pttc_t *pttc) {
    pttc->parser.f_complete = 1;
}
static void pttc_parser_set_incomplete(pttc_t *pttc) {
    pttc->parser.f_complete = 0;
}
static int pttc_parser_is_complete(pttc_t *pttc) {
    return pttc->parser.f_complete == 1;
}
static void pttc_parser_set_on(pttc_t *pttc) {
    pttc->parser.f_on = 1;
}
static void pttc_parser_set_off(pttc_t *pttc) {
    pttc->parser.f_on = 0;
}
static int pttc_parser_is_on(pttc_t *pttc) {
    return pttc->parser.f_on == 1;
}

static int pttc_prober_should_probe(pttc_t *pttc) {
    pttc->prober.idle_cnt = (pttc->prober.idle_cnt+1) % PTTC_MAX_IDLE_CNT;
    return (pttc->prober.idle_cnt == 0);
}
static void pttc_prober_reset_probe_count(pttc_t *pttc) {
    pttc->prober.probe_cnt = 0;
}
static void pttc_prober_inc_probe_count(pttc_t *pttc) {
    pttc->prober.probe_cnt++;
}
static int pttc_prober_is_negative_result(pttc_t *pttc) {
    return (pttc->prober.probe_cnt > PTTC_MAX_PROBE_CNT);
}

static void pttc_fdtor_reset(pttc_t *pttc) {
    pttc->fdtor.state = 0;
}
static int pttc_fdtor_is_flipped(pttc_t *pttc, int new_state) {
    int ret = (new_state != pttc->fdtor.state);
    pttc->fdtor.state = new_state;
    return ret;
}


static void on_pttc_ptt_default(int ptt) {
    fprintf(stdout, "PTT is %d\n", ptt);
}

void pttc_init( serial_t *serial, pttc_t *pttc, void (*cb)(int) ) {
    serial->on_serial_data_received = on_pttc_data_received;
    serial->process_command = pttc_process_command;

    if( cb != 0) 
        pttc->on_pttc_ptt = cb;
    else 
        pttc->on_pttc_ptt = on_pttc_ptt_default;

    serial->user_data = pttc;
}

void pttc_start( serial_t *serial, char *port_dev ) {
    pttc_t *pttc = (pttc_t *)serial->user_data;

    pttc_prober_reset_probe_count(pttc);
    pttc_fdtor_reset(pttc);

    strncpy(serial->port_dev, port_dev, sizeof(serial->port_dev));

    serial_start(serial);
}

void pttc_end( serial_t* serial ) {
    serial_end(serial);
}

static void on_pttc_data_received(serial_t *serial, char *buffer, int nbytes) {
    int i;
    pttc_t *pttc = (pttc_t *)serial->user_data;
    pttc_prober_reset_probe_count(pttc);
    for( i = 0; i < nbytes; i++ ) {
        switch (buffer[i]) {
        case 'L':
            pttc_parser_set_incomplete(pttc);
            break;
        case '0':
            pttc_parser_set_complete(pttc);
            pttc_parser_set_off(pttc);
            break;
        case '1':
            pttc_parser_set_complete(pttc);
            pttc_parser_set_on(pttc);
            break;
        default:
            fprintf(stdout, "invalid parsed input: %c (%d)\n", buffer[i], buffer[i]);
            break;
        }
        if( pttc_parser_is_complete(pttc) && pttc_fdtor_is_flipped(pttc, pttc_parser_is_on(pttc)) ) {
            pttc->on_pttc_ptt(pttc_parser_is_on(pttc));
        }
    }
}
static void pttc_process_command(serial_t *serial, int fd) {
    int n;
    pttc_t *pttc = (pttc_t *)serial->user_data;
    /* periodically probe PTT Card */
    if( pttc_prober_is_negative_result(pttc) ) {
        PANIC(__FILE__, "Disconnected from PTT Card");
    }
    else if( pttc_prober_should_probe(pttc) ) {
        pttc_prober_inc_probe_count(pttc);
        n = write(fd, "Q", 1); // do probe
    }
}

