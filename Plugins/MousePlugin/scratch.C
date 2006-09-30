/*  MousePlugin
 *  Copyleft (C) 2002 Dan Bethell <dan@pawfal.org>
 *                    Dave Griffiths <dave@pawfal.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/ 
#include "scratch.h"

using namespace std;

#define BAUDRATE B1200
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

int c, res;
int count, total;
volatile int STOP=FALSE;        
int fd; // the serial port device
struct termios oldtio,newtio;
unsigned char buf[3];
char *MODEMDEVICE;

scratch::scratch(char *dev)
{
    openSerialPort(dev);	
    pthread_mutex_init( &mutex, NULL);
    pthread_create( &scratch_thread, NULL, (void *(*)(void *))listen, this);    
}

scratch::~scratch()
{
    closeSerialPort();
}

void scratch::setData(char c)
{
    pthread_mutex_lock( &mutex );
    data = c;
    pthread_mutex_unlock( &mutex );
}

char scratch::getData()
{
    char result;
	pthread_mutex_lock( &mutex );
    result = data;
    pthread_mutex_unlock( &mutex );
    
    return result;
}

void scratch::stop()
{
    pthread_mutex_lock( &mutex );
    STOP=TRUE;
    pthread_mutex_unlock( &mutex );
}

void openSerialPort(char *dev) 
{
    char *MODEMDEVICE = dev;

    fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY ); 
    if (fd <0) 
    {
        perror(MODEMDEVICE); exit(-1); 
    }
        
    tcgetattr(fd,&oldtio); /* save current port settings */
        
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS7 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
        
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
         
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 3;   /* blocking read until 5 chars received */
        
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd,TCSANOW,&newtio);
}

void closeSerialPort()
{    
    tcsetattr(fd,TCSANOW,&oldtio); // reset serial port settings
    close(fd); // close pipe to serial port
}

void listen(scratch *scr)
{
    while (STOP==FALSE) 
    {       /* loop for input */
        res = read(fd,buf,3);   /* returns after 3 chars have been input */
        if (buf[0]>>6&1)
        {   
            char c;
            
            c = ((buf[0]&12)<<4)|(buf[2]&63);     
            if (c!=0x0) {          
                scr->setData(c);
                // some random logging stuff
                /*
                total += (int)c;
                count ++;
                
                if (count%20==0) {
                    FILE *ptr = fopen("/tmp/EYEMUSH.log", "a+");
                    fprintf(ptr, "EYEMUSH LOG:\tScratch\n");    
                    fclose(ptr);
                }
                */
            }
        }
        else
        {
            read(fd,buf,1);
        }
    }
}
