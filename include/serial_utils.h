#ifndef __SERIAL_UTILS_H__
#define __SERIAL_UTILS_H__
#include <termios.h>
#include <pthread.h>
extern void (*on_serial_data_received)(char *buffer, int nbytes);
extern void (*process_command)(int fd);

int open_serial(const char* portdev, int *p_fd);
void config_serial(struct termios *options, int fd);
void serial_start(char *port_dev, pthread_t *thread);
void serial_end(pthread_t *thread);
#endif
