#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

#include "ansi-utils.h"

void (*on_serial_data_received)(char *buffer, int nbytes);
void (*process_command)(int fd);

volatile int fQuit;

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
    options->c_cc[VTIME] = 5; // measured in 0.1 second

    tcsetattr(fd, TCSANOW, options);
}

static void serial_read_and_parse(int fd) {
    int nbytes;
    char buffer[10];
    memset(buffer, 0, sizeof(buffer));
    nbytes = read(fd, buffer, sizeof(buffer));
    if(nbytes > 0) {
        on_serial_data_received(buffer, nbytes);
        //on_riuc_status(&uart_status);
    }
}

static void *do_thing(void *data) {
    //char *portdev = "/dev/cu.usbserial";
    //char *portdev = "/dev/ttyO1";
    
    int serial_fd;

    char *port_dev = (char *)data;
    struct termios options;

    CHECK(__FILE__, open_serial(port_dev, &serial_fd));
    config_serial(&options, serial_fd);

    while(!fQuit) {
        serial_read_and_parse(serial_fd);
        process_command(serial_fd);
    }

    close(serial_fd);
    return 0;
}

void serial_start(char *port_dev, pthread_t *thread) {
    fQuit = 0;
    CHECK(__FILE__, pthread_create(thread, NULL, do_thing, port_dev));
}

void serial_end(pthread_t *thread) {
    fQuit = 1;
    pthread_join(*thread, NULL);
}

