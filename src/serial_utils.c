#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

#include "ansi-utils.h"
#include "serial_utils.h"

int open_serial(char* portdev, int *p_fd) {
    //*p_fd = open(portdev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    *p_fd = open(portdev, O_RDWR | O_NOCTTY);
    if( (*p_fd) < 0 ) {
        printf( "Cannot open port: %s", strerror(errno));
        return 0 - 100;
    }
    fcntl(*p_fd, F_SETFL, 0);
    return 0;
}

void config_serial(struct termios *options, int fd) {
    tcgetattr(fd, options);

    cfsetispeed(options, B9600);
    cfsetospeed(options, B9600);

    options->c_cflag |= (CREAD |CLOCAL);
    options->c_cflag &= ~CSIZE; /* Mask the character size bits */
    options->c_cflag |= CS8;    /* Select 8 data bits */
    options->c_cflag &= ~CRTSCTS;
    options->c_iflag &= ~(IXON | IXOFF | IXANY);
    //options->c_lflag |= (ICANON | ECHO | ECHOE); // Canonical mode

    options->c_lflag &= ~(ICANON | ECHO); // RAW mode
    options->c_cc[VMIN] = 0;
    options->c_cc[VTIME] = 2; // measured in 0.1 second

    tcsetattr(fd, TCSANOW, options);
}

int do_thing(void *data) {
    //char *portdev = "/dev/cu.usbserial";
    //char *portdev = "/dev/ttyO1";
    
    int serial_fd;

    serial_t *serial = (serial_t *)data;
    char *port_dev = serial->port_dev;
    struct termios options;

    int nbytes;
    char buffer[10];

    ANSI_CHECK(__FILE__, open_serial(port_dev, &serial_fd));
    config_serial(&options, serial_fd);

    serial->fQuit = 0;
    while(!serial->fQuit) {
        nbytes = read(serial_fd, buffer, sizeof(buffer));
        if(nbytes > 0) {
            buffer[nbytes] = '\0';
            serial->on_serial_data_received(serial, buffer, nbytes);
        }
        serial->process_command(serial, serial_fd);
    }

    close(serial_fd);
    return 0;
}

void serial_start(serial_t *serial) {
    pj_thread_create(serial->pool, NULL, do_thing, serial, PJ_THREAD_DEFAULT_STACK_SIZE, 0, &serial->master_thread);
}

void serial_end(serial_t *serial) {
    serial->fQuit = 1;
    pj_thread_join(serial->master_thread);
}

