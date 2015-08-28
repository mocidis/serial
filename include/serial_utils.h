#ifndef __SERIAL_UTILS_H__
#define __SERIAL_UTILS_H__
#include <termios.h>
#include <pthread.h>
typedef struct serial_s serial_t;

struct serial_s {
    char port_dev[30];
    pthread_t thread;
    volatile int fQuit;

    void (*on_serial_data_received)(serial_t *serial, char *buffer, int nbytes);
    void (*process_command)(serial_t *serial, int fd);

    void *user_data;
};

int open_serial(char* portdev, int *p_fd);
void config_serial(struct termios *options, int fd);
void serial_start(serial_t *serial);
void serial_end(serial_t *serial);
#endif
