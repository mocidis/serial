#ifndef __RIUC4_UART_H__
#define __RIUC4_UART_H__

#include <pthread.h>

#include "serial_utils.h"

#define N_RIUC 4

typedef enum {
    RIUC_SIGNAL_PTT=0,
    RIUC_SIGNAL_SQ,
    RIUC_SIGNAL_ERROR,
    RIUC_SIGNAL_TX,
    RIUC_SIGNAL_RX
} riuc4_signal_t;

extern char *RIUC4_SIGNAL_NAME[];

typedef struct _uart4_status_t {
    int ptt;
    int sq;
    int tx;
    int rx;
    void *user_data;
} uart4_status_t;

typedef struct riuc4_s {
    struct {
        riuc4_signal_t signal; // 1=ptt(L) or 2=sq(Q) or 3=error(E) or 4=tx(T) or 5=rx(R). See define RIUC_SIGNAL_...
        int f_parsing_port; // parsing port or parsing value (1: parsing port)
        int f_complete; // complete or not
        int f_on; // on or off
        int port; // the port that output is saying about
    } parser4;

    uart4_status_t riuc4_status[N_RIUC];

    struct {
        int command[4];
    } uart4_command;

    void (*on_riuc4_status)(int port, riuc4_signal_t signal, uart4_status_t *data);
    void (*on_riuc4_error)(int error_code);


} riuc4_t;

// DEFAULT ACTIONS : INIT, START, END
void riuc4_init(serial_t *serial, riuc4_t *riuc4, void (*cb)(int port, riuc4_signal_t signal, uart4_status_t *ustatus));
void riuc4_start(serial_t *serial, char *port_dev);
void riuc4_end(serial_t *serial);

// RIUC COMMAND PROCESSING LOGIC
void riuc4_process_command(serial_t *serial, int fd);

// SPECIFIC RIUC COMMANDS
void riuc4_probe_sq(riuc4_t *, int port_idx); // Probe SQ status
void riuc4_probe_ptt(riuc4_t *, int port_idx); // Probe PTT status
void riuc4_probe_tx(riuc4_t *, int port_idx);
void riuc4_probe_rx(riuc4_t *, int port_idx);

void riuc4_enable_tx(riuc4_t *, int port_idx);
void riuc4_disable_tx(riuc4_t *, int port_idx);

void riuc4_enable_rx(riuc4_t *, int port_idx);
void riuc4_disable_rx(riuc4_t *, int port_idx);

void riuc4_on_ptt(riuc4_t *, int port_idx); // Turn PTT on
void riuc4_off_ptt(riuc4_t *, int port_idx); // Turn PTT off
#endif
