#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */


int open_port(void);
int send_data(void);
int recieve_data(void);
/*
 * 'open_port()' - Open serial port 1.
 *
 * Returns the file descriptor on success or -1 on error.
 */
 
void main( void )
{
	int fd;
	int n;
	char t;

	fd = open_port();
	
	n = write(fd, "test", 4);
	if (n < 0)
	  fputs("write() of 4 bytes failed!\n", stderr);
	  
	while(1){
	n = write(fd, "test", 4);
	if (n < 0)
	  fputs("write() of 4 bytes failed!\n", stderr);
		t = fcntl(fd, F_SETFL, FNDELAY);
		printf("%c", t);
	}
}

int open_port(void)
{
  int fd; /* File descriptor for the port */


  fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
  {

    perror("open_port: Unable to open /dev/ttyUSB0 - ");
  }
  else
    fcntl(fd, F_SETFL, 0);

  return (fd);
}





