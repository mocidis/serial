#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
//#include <termios.h>
#include "ansi-utils.h"
#include "serial_utils.h"
#include "riuc4_uart.h"

int riuc4_error;

char *RIUC4_SIGNAL_NAME[] = {
    "PTT", "SQ", "ERROR", "TX", "RX"
};
static void on_riuc4_status_default(int port, riuc4_signal_t signal, uart4_status_t *ustatus) {
    fprintf(stdout, "RIUC4 (port %d, signal %s) - tx:%d - rx:%d - ptt:%d - sq:%d\n", port, RIUC4_SIGNAL_NAME[signal], ustatus->tx, ustatus->rx, ustatus->ptt, ustatus->sq);
}

static void on_riuc4_error_default(int error_code) {
    fprintf(stdout, "RIUC4 - Error:%d\n", error_code);
}

static void parser4_set_parsing_port(riuc4_t *riuc4) {
    riuc4->parser4.f_parsing_port = 1;
}
static void parser4_clear_parsing_port(riuc4_t *riuc4) {
    riuc4->parser4.f_parsing_port = 0;
}
static int parser4_is_parsing_port(riuc4_t *riuc4) {
    return riuc4->parser4.f_parsing_port > 0;
}

static void parser4_set_tx(riuc4_t *riuc4) {
    riuc4->parser4.signal = RIUC_SIGNAL_TX;
}
static void parser4_set_rx(riuc4_t *riuc4) {
    riuc4->parser4.signal = RIUC_SIGNAL_RX;
}
static void parser4_set_error(riuc4_t *riuc4) {
    riuc4->parser4.signal = RIUC_SIGNAL_ERROR;
}
static void parser4_set_ptt(riuc4_t *riuc4) {
    riuc4->parser4.signal = RIUC_SIGNAL_PTT;
}
static void parser4_set_sq(riuc4_t *riuc4) {
    riuc4->parser4.signal = RIUC_SIGNAL_SQ;
}
static int parser4_is_tx(riuc4_t *riuc4) {
    return riuc4->parser4.signal == RIUC_SIGNAL_TX;
}
static int parser4_is_rx(riuc4_t *riuc4) {
    return riuc4->parser4.signal == RIUC_SIGNAL_RX;
}
static int parser4_is_error(riuc4_t *riuc4) {
    return riuc4->parser4.signal == RIUC_SIGNAL_ERROR;
}
static int parser4_is_ptt(riuc4_t *riuc4) {
    return riuc4->parser4.signal == RIUC_SIGNAL_PTT;
}
static int parser4_is_sq(riuc4_t *riuc4) {
    return riuc4->parser4.signal == RIUC_SIGNAL_SQ;
}
static void parser4_set_on(riuc4_t *riuc4) {
    riuc4->parser4.f_on = 1;
}
static void parser4_set_off(riuc4_t *riuc4) {
    riuc4->parser4.f_on = 0;
}
static int parser4_is_on(riuc4_t *riuc4) {
    return riuc4->parser4.f_on > 0;
}
static void parser4_set_complete(riuc4_t *riuc4) {
    riuc4->parser4.f_complete = 1;
}
static void parser4_set_incomplete(riuc4_t *riuc4) {
    riuc4->parser4.f_complete = 0;
}
static int parser4_is_complete(riuc4_t *riuc4) {
    return riuc4->parser4.f_complete == 1;
}
static void parser4_set_port(riuc4_t *riuc4, char port) {
    riuc4->parser4.port = port - '1';
}
static int parser4_get_port(riuc4_t *riuc4) {
    return riuc4->parser4.port;
}

static void on_riuc4_data_received(serial_t *serial, char *buffer, int nbytes) {
    int i;
    int port;
    int param;
    riuc4_t *riuc4 = (riuc4_t *)serial->user_data;
    for( i = 0; i < nbytes; i++ ) {
        switch (buffer[i]) {
        case 'L':
            parser4_set_ptt(riuc4);
            parser4_set_parsing_port(riuc4);
            parser4_set_incomplete(riuc4);
            break;
        case 'Q':
            parser4_set_sq(riuc4);
            parser4_set_parsing_port(riuc4);
            parser4_set_incomplete(riuc4);
            break;
        case 'T':
            parser4_set_tx(riuc4);
            parser4_set_parsing_port(riuc4);
            parser4_set_incomplete(riuc4);
            break;
        case 'R':
            parser4_set_rx(riuc4);
            parser4_set_parsing_port(riuc4);
            parser4_set_incomplete(riuc4);
            break;
        case 'E':
            parser4_set_error(riuc4);
            parser4_clear_parsing_port(riuc4);
            parser4_set_incomplete(riuc4);
            break;
        case '0':
            if (parser4_is_parsing_port(riuc4)) {
                parser4_clear_parsing_port(riuc4);
                fprintf(stdout, "RIUC4 - invalid parsed input: (%s) (%c) (%d)\n", buffer, buffer[i], buffer[i]);
            }
            else {
                parser4_set_complete(riuc4);
                parser4_set_off(riuc4);
            }
            break;
        case '1':
            if (parser4_is_parsing_port(riuc4)) {
                parser4_clear_parsing_port(riuc4);
                parser4_set_port(riuc4, buffer[i]);
            }
            else {
                parser4_set_complete(riuc4);
                parser4_set_on(riuc4);
            }
            break;
        case '2':
        case '3':
        case '4':
            if (parser4_is_parsing_port(riuc4)) {
                parser4_clear_parsing_port(riuc4);
                parser4_set_port(riuc4, buffer[i]);
            }
            else if(parser4_is_error(riuc4)) {
                riuc4_error = buffer[i] - '0';
                parser4_set_complete(riuc4);
            }
            else {
                fprintf(stdout, "RIUC4 - invalid parsed input: (%s) (%c) (%d)\n", buffer, buffer[i], buffer[i]);
            }
            break;
        default:
            if (parser4_is_parsing_port(riuc4)) {
                parser4_clear_parsing_port(riuc4);
            }
            else if (parser4_is_error(riuc4)) {
                riuc4_error = buffer[i] - '0';
                parser4_set_complete(riuc4);
            }
            else {
                fprintf(stdout, "RIUC4 - invalid parsed input: (%s) (%c) (%d)\n", buffer, buffer[i], buffer[i]);
            }
            break;
        }

        if( parser4_is_complete(riuc4) ) {
            if (parser4_is_error(riuc4)) {
                on_riuc4_error_default(riuc4_error);
            }
            else {
                port = parser4_get_port(riuc4);
                
                if( parser4_is_ptt(riuc4) ) {
                    riuc4->riuc4_status[port].ptt = parser4_is_on(riuc4);
                }
                else if (parser4_is_sq(riuc4)) {
                    riuc4->riuc4_status[port].sq = parser4_is_on(riuc4);
                }
                else if (parser4_is_tx(riuc4)) {
                    riuc4->riuc4_status[port].tx = parser4_is_on(riuc4);
                }
                else if (parser4_is_rx(riuc4)) {
                    riuc4->riuc4_status[port].rx = parser4_is_on(riuc4);
                }
                riuc4->on_riuc4_status(port, riuc4->parser4.signal, &(riuc4->riuc4_status[port]));
            }
        }
    }
}

/*
typedef struct _uart4_command_t {
    int fLED; // check PTT
    int fSQ;  // check SQ
    int fTX;  // check TX
    int fRX;  // check RX
    int enableTX;
    int enableRX;
    int disableTX;
    int disableRX;
    int onPTT;
    int offPTT;
} uart4_command_t;
*/

/*************************** COMMAND Format *******************
 |0   | 1   |2    | 3   | 4   | 5   | 6   | 7   |  8  |9     |10|11|12|13|14|15|
 |fLED| fSQ | fTX | fRX |entTX|entRX|disTX|disRX|onPTT|offPTT|  UNUSED         |
***********************************************************************************/
#define CMD_FLED    (0x1)
#define CMD_FSQ     (0x1 << 1)
#define CMD_FTX     (0x1 << 2)
#define CMD_FRX     (0x1 << 3)
#define CMD_ENT_TX  (0x1 << 4)
#define CMD_ENT_RX  (0x1 << 5)
#define CMD_DIS_TX  (0x1 << 6)
#define CMD_DIS_RX  (0x1 << 7)
#define CMD_ON_PTT  (0x1 << 8)
#define CMD_OFF_PTT (0x1 << 9)

static void set_check_ptt(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_FLED;
}
static void set_check_sq(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_FSQ;
}
static void set_check_tx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_FTX;
}
static void set_check_rx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_FRX;
}
static void set_enable_tx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_ENT_TX;
}
static void set_disable_tx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_DIS_TX;
}
static void set_enable_rx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_ENT_RX;
}
static void set_disable_rx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_DIS_RX;
}
static void set_on_ptt(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_ON_PTT;
}
static void set_off_ptt(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] |= CMD_OFF_PTT;
}

static void reset_check_ptt(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_FLED;
}
static void reset_check_sq(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_FSQ;
}
static void reset_check_tx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_FTX;
}
static void reset_check_rx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_FRX;
}
static void reset_enable_tx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_ENT_TX;
}
static void reset_disable_tx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_DIS_TX;
}
void reset_enable_rx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_ENT_RX;
}
void reset_disable_rx(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_DIS_RX;
}
void reset_on_ptt(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_ON_PTT;
}
void reset_off_ptt(riuc4_t *riuc4, int port_idx) {
    riuc4->uart4_command.command[port_idx] &= ~CMD_OFF_PTT;
}

int is_check_ptt(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_FLED;
}
int is_check_sq(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_FSQ;
}
int is_check_tx(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_FTX;
}
int is_check_rx(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_FRX;
}
int is_enable_tx(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_ENT_TX;
}
int is_disable_tx(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_DIS_TX;
}
int is_enable_rx(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_ENT_RX;
}
int is_disable_rx(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_DIS_RX;
}
int is_on_ptt(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_ON_PTT;
}
int is_off_ptt(riuc4_t *riuc4, int port_idx) {
    return riuc4->uart4_command.command[port_idx] & CMD_OFF_PTT;
}

void do_check_ptt(int port_idx, int fd) {
    char data[] = "pttX\r";
    data[3] = port_idx + '1';
    EXIT_IF_TRUE(5 != write(fd, data, 5), "Write error\n");
}
void do_check_sq(int port_idx, int fd) {
    char data[] = "sqX\r";
    data[2] = port_idx + '1';
    EXIT_IF_TRUE(4 != write(fd, data, 4), "Write error\n");
}
void do_check_tx(int port_idx, int fd) {
    char data[] = "txX\r";
    data[2] = port_idx + '1';
    EXIT_IF_TRUE(4 != write(fd, data, 4), "Write error\n");
}
void do_check_rx(int port_idx, int fd) {
    char data[] = "rxX\r";
    data[2] = port_idx + '1';
    EXIT_IF_TRUE(4 != write(fd, data, 4), "Write error\n");
}
void do_enable_tx(int port_idx, int fd) {
    char data[] = "entxX\r";
    data[4] = port_idx + '1';
    EXIT_IF_TRUE(6 != write(fd, data, 6), "Write error\n");
}
void do_disable_tx(int port_idx, int fd) {
    char data[] = "distxX\r";
    data[5] = port_idx + '1';
    EXIT_IF_TRUE(7 != write(fd, data, 7), "Write error\n");
}
void do_enable_rx(int port_idx, int fd) {
    char data[] = "enrxX\r";
    data[4] = port_idx + '1';
    EXIT_IF_TRUE(6 != write(fd, data, 6), "Write error\n");
}
void do_disable_rx(int port_idx, int fd) {
    char data[] = "disrxX\r";
    data[5] = port_idx + '1';
    EXIT_IF_TRUE(7 != write(fd, data, 7), "Write error\n");
}
void do_on_ptt(int port_idx, int fd) {
    char data[] = "onpttX\r";
    data[5] = port_idx + '1';
    EXIT_IF_TRUE(7 != write(fd, data, 7), "Write error\n");
}
void do_off_ptt(int port_idx, int fd) {
    char data[] = "offpttX\r";
    data[6] = port_idx + '1';
    EXIT_IF_TRUE(8 != write(fd, data, 8), "Write error\n");
}

void riuc4_process_command(serial_t *serial, int fd) {
    int i;
    riuc4_t *riuc4 = (riuc4_t *)serial->user_data;
    for( i = 0; i < N_RIUC; i++ ) {
        if (is_check_ptt(riuc4, i)) {
            reset_check_ptt(riuc4, i);
            do_check_ptt(i, fd);
        }
        if (is_check_sq(riuc4, i)) {
            reset_check_sq(riuc4, i);
            do_check_sq(i, fd);
        }
        if (is_check_tx(riuc4, i)) {
            reset_check_tx(riuc4, i);
            do_check_tx(i, fd);
        }
        if (is_check_rx(riuc4, i)) {
            reset_check_rx(riuc4, i);
            do_check_rx(i, fd);
        }
        if (is_enable_tx(riuc4, i)) {
            reset_enable_tx(riuc4, i);
            do_enable_tx(i, fd);
        }
        if (is_disable_tx(riuc4, i)) {
            reset_disable_tx(riuc4, i);
            do_disable_tx(i, fd);
        }
        if (is_enable_rx(riuc4, i)) {
            reset_enable_rx(riuc4, i);
            do_enable_rx(i, fd);
        }
        if (is_disable_rx(riuc4, i)) {
            reset_disable_rx(riuc4, i);
            do_disable_rx(i, fd);
        }
        if (is_on_ptt(riuc4, i)) {
            reset_on_ptt(riuc4, i);
            do_on_ptt(i, fd);
        }
        if (is_off_ptt(riuc4, i)) {
            reset_off_ptt(riuc4, i);
            do_off_ptt(i, fd);
        }
    }
}

void riuc4_init(serial_t *serial, riuc4_t *riuc4, void (*cb)(int port, riuc4_signal_t signal, uart4_status_t *ustatus), pj_pool_t *pool) {
    serial->on_serial_data_received = on_riuc4_data_received;
    serial->process_command = riuc4_process_command;
    //on_riuc_error = on_riuc4_error_default;
    if( cb == NULL ) 
        riuc4->on_riuc4_status = on_riuc4_status_default;
    else 
        riuc4->on_riuc4_status = cb;

    serial->pool = pool;    
    serial->user_data = riuc4;
}

void riuc4_start(serial_t *serial, char *port_dev) {
    riuc4_t *riuc4 = (riuc4_t *)serial->user_data;
    riuc4->uart4_command.command[0] = 0;
    riuc4->uart4_command.command[1] = 0;
    riuc4->uart4_command.command[2] = 0;
    riuc4->uart4_command.command[3] = 0;
    strncpy(serial->port_dev, port_dev, sizeof(serial->port_dev));
    serial_start(serial);
}

void riuc4_end(serial_t *serial) {
    serial_end(serial);
}

void riuc4_probe_sq(riuc4_t *riuc4, int port_idx) {
    set_check_sq(riuc4, port_idx);
}
void riuc4_probe_ptt(riuc4_t *riuc4, int port_idx) {
    set_check_ptt(riuc4, port_idx);
}
void riuc4_probe_tx(riuc4_t *riuc4, int port_idx) {
    set_check_tx(riuc4, port_idx);
}
void riuc4_probe_rx(riuc4_t *riuc4, int port_idx) {
    set_check_rx(riuc4, port_idx);
}
void riuc4_enable_tx(riuc4_t *riuc4, int port_idx) {
    set_enable_tx(riuc4, port_idx);
}
void riuc4_disable_tx(riuc4_t *riuc4, int port_idx) {
    set_disable_tx(riuc4, port_idx);
}
void riuc4_enable_rx(riuc4_t *riuc4, int port_idx) {
    set_enable_rx(riuc4, port_idx);
}
void riuc4_disable_rx(riuc4_t *riuc4, int port_idx) {
    set_disable_rx(riuc4, port_idx);
}
void riuc4_on_ptt(riuc4_t *riuc4, int port_idx) {
    set_on_ptt(riuc4, port_idx);
}
void riuc4_off_ptt(riuc4_t *riuc4, int port_idx) {
    set_off_ptt(riuc4, port_idx);
}
