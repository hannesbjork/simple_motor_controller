// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

#include <stdlib.h>
#include <pthread.h>

int open_port(void);

int serial_port;

void *recieveThread(void *vargp)
{

	
	while(1){
		
		char sing[1];

		// Read bytes. The behaviour of read() (e.g. does it block?,
		// how long does it block for?) depends on the configuration
		// settings above, specifically VMIN and VTIME
		int n = read(serial_port, &sing, sizeof(sing));
		printf("%s", sing);

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be negative to signal an error.
	}
}

void *sendThread(void *vargp)
{

	char *send_msg;
		
	sleep(1);
	send_msg = "Hejsan\r\n";
	write(serial_port, send_msg, sizeof(send_msg));
	
	sleep(1);
	send_msg = "Du ok?\r\n";
	write(serial_port, send_msg, sizeof(send_msg));
	
	sleep(1);
	send_msg = "hoppas\r\n";
	write(serial_port, send_msg, sizeof(send_msg));
	
	sleep(1);
	send_msg = "du mår\r\n";
	write(serial_port, send_msg, sizeof(send_msg));
	
	sleep(1);
	send_msg = "bra!!!\r\n";
	write(serial_port, send_msg, sizeof(send_msg));
	
	sleep(1);
	pthread_exit(NULL);
}

   
void main(void)
{

	serial_port = open_port();


	pthread_t threads[2];
	
	pthread_create(&threads[0], NULL, recieveThread, NULL);
	pthread_create(&threads[1], NULL, sendThread, NULL);

	pthread_exit(NULL);
}

int open_port(void){
	int sp = open("/dev/ttyUSB0", O_RDWR);
	// Check for errors
	if (sp < 0) {
		printf("Error %i from open: %s\n", errno, strerror(errno));
	}
	// Create new termios struct, we call it 'tty' for convention
	// No need for "= {0}" at the end as we'll immediately write the existing
	// config to this struct
	struct termios tty;

	// Read in existing settings, and handle any error
	// NOTE: This is important! POSIX states that the struct passed to tcsetattr()
	// must have been initialized with a call to tcgetattr() overwise behaviour
	// is undefined
	if(tcgetattr(serial_port, &tty) != 0) {
		printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	struct termios {
		tcflag_t c_iflag;		/* input mode flags */
		tcflag_t c_oflag;		/* output mode flags */
		tcflag_t c_cflag;		/* control mode flags */
		tcflag_t c_lflag;		/* local mode flags */
		cc_t c_line;			/* line discipline */
		cc_t c_cc[NCCS];		/* control characters */
	};

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)

	tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication

	tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
	tty.c_cflag |= CS8; // 8 bits per byte (most common)

	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)

	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;

	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo

	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)


	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// Set in/out baud rate to be 9600
	cfsetispeed(&tty, B4800);
	cfsetospeed(&tty, B4800);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
		printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}
	
	return sp;
}
