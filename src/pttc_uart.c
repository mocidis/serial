#include <unistd.h>
#include <pthread.h>
#include "pttc_uart.h"
#include "serial_utils.h"
#include "ansi-utils.h"

void on_pttc_data_received(char *buffer, int nbytes);
void pttc_process_command(int fd);

/* Parser for PTT Card control */
struct {
    int f_complete;
    int f_on;
} pttc_parser;

#define PTTC_MAX_IDLE_CNT 10
#define PTTC_MAX_PROBE_CNT 3
struct {
    int idle_cnt;
    int probe_cnt;
} pttc_prober;

struct {
    int state;
} pttc_fdtor; // flip detector

void pttc_parser_set_complete() {
    pttc_parser.f_complete = 1;
}
void pttc_parser_set_incomplete() {
    pttc_parser.f_complete = 0;
}
int pttc_parser_is_complete() {
    return pttc_parser.f_complete == 1;
}
void pttc_parser_set_on() {
    pttc_parser.f_on = 1;
}
void pttc_parser_set_off() {
    pttc_parser.f_on = 0;
}
int pttc_parser_is_on() {
    return pttc_parser.f_on == 1;
}

int pttc_prober_should_probe() {
    pttc_prober.idle_cnt = (pttc_prober.idle_cnt+1) % PTTC_MAX_IDLE_CNT;
    return (pttc_prober.idle_cnt == 0);
}
void pttc_prober_reset_probe_count() {
    pttc_prober.probe_cnt = 0;
}
void pttc_prober_inc_probe_count() {
    pttc_prober.probe_cnt++;
}
int pttc_prober_is_negative_result() {
    return (pttc_prober.probe_cnt > PTTC_MAX_PROBE_CNT);
}

void pttc_fdtor_reset() {
    pttc_fdtor.state = 0;
}
int pttc_fdtor_is_flipped(int new_state) {
    int ret = (new_state != pttc_fdtor.state);
    pttc_fdtor.state = new_state;
    return ret;
}

void (*on_pttc_ptt)(int ptt);

void on_pttc_ptt_default(int ptt) {
    fprintf(stdout, "PTT is %d\n", ptt);
}

void pttc_init( void (*cb)(int) ) {
    on_serial_data_received = on_pttc_data_received;
    process_command = pttc_process_command;
    if( cb != 0) 
        on_pttc_ptt = cb;
    else 
        on_pttc_ptt = on_pttc_ptt_default;
}

void pttc_start( char *port_dev, pthread_t *thread ) {
    pttc_prober_reset_probe_count();
    pttc_fdtor_reset();
    serial_start(port_dev, thread);
}

void pttc_end( pthread_t *thread ) {
    serial_end(thread);
}

void on_pttc_data_received(char *buffer, int nbytes) {
    int i;
    pttc_prober_reset_probe_count();
    for( i = 0; i < nbytes; i++ ) {
        switch (buffer[i]) {
        case 'L':
            pttc_parser_set_incomplete();
            break;
        case '0':
            pttc_parser_set_complete();
            pttc_parser_set_off();
            break;
        case '1':
            pttc_parser_set_complete();
            pttc_parser_set_on();
            break;
        default:
            fprintf(stdout, "invalid parsed input: %c (%d)\n", buffer[i], buffer[i]);
            break;
        }
        if( pttc_parser_is_complete() && pttc_fdtor_is_flipped(pttc_parser_is_on()) ) {
            on_pttc_ptt(pttc_parser_is_on());
        }
    }
}
void pttc_process_command(int fd) { 
    /* periodically probe PTT Card */
    if( pttc_prober_is_negative_result() ) {
        PANIC(__FILE__, "Disconnected from PTT Card");
    }
    else if( pttc_prober_should_probe() ) {
        pttc_prober_inc_probe_count();
        write(fd, "Q", 1); // do probe
    }
}

