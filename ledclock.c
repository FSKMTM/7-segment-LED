#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <endian.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <time.h>

#define HT16K33_BLINK_CMD          0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF          0
#define HT16K33_BLINK_2HZ          1
#define HT16K33_BLINK_1HZ          2
#define HT16K33_BLINK_HALFHZ    3

#define HT16K33_CMD_SETUP          0x21
#define HT16K33_CMD_BRIGHTNESS  0xE0
#define HT16K33_ADDR                    0x70

#define BUFFER_SIZE                      8

unsigned short displaybuffer[ BUFFER_SIZE ];
int handle  = 0;
int i2c_bus = 1;

const unsigned char numbertable[] =
{
        0x3F, /* 0 */
        0x06, /* 1 */
        0x5B, /* 2 */
        0x4F, /* 3 */
        0x66, /* 4 */
        0x6D, /* 5 */
        0x7D, /* 6 */
        0x07, /* 7 */
        0x7F, /* 8 */
        0x6F, /* 9 */
        0x77, /* a */
        0x7C, /* b */
        0x39, /* C */
        0x5E, /* d */
        0x79, /* E */
        0x71, /* F */
        0x76, /*16 H */
        0x1E, /*17 J */
        0x38, /*18 L */
        0x54, /*19 n */
        0x5C, /*20 o */
        0x73, /*21 P */
        0x50, /*22 r */
        0x78, /*23 t */
        0x3E, /*24 U */
        0x6E, /*25 Y */
        0x63, /*26 ° */
        0x00, /*27   */
};

int i2c_write( void *buf, int len )
{
        int rc = -1;

        if ( write( handle, buf, len ) != len )
        {
                printf( "I2C write failed: %s\n", strerror( errno ) );
        }
        else
        {
                rc = 0;
        }

        return rc;
}

void buffer_clear( void )
{
        int i;

        for ( i = 0; i < BUFFER_SIZE; i++ )
        {
                displaybuffer[ i ] = 0;
        }
}

void buffer_write_digit( int digit, int number )
{
        if ( ( digit > 4 ) || ( number > 0xFF ) )
                return;

        if ( ( digit == 2 ) )
        {
                displaybuffer[ digit ] = number;
        }
        else
        {
                displaybuffer[ digit ] = numbertable[ number ];
        }
}

int ht16k33_update( void )
{
        unsigned short buf[ BUFFER_SIZE ];
        int i;

        for( i = 0; i < BUFFER_SIZE; i++ )
        {
                buf[ i ] = htobe16( displaybuffer[ i ] );
        }

        return i2c_write( buf, sizeof( displaybuffer ) );
}

int ht16k33_brightness( unsigned char brightness )
{
        unsigned char buf[ 2 ];

        if ( brightness > 15 )
                brightness = 15;

        buf[ 0 ] = ( HT16K33_CMD_BRIGHTNESS | brightness );

        return i2c_write( buf, 1 );
}

int ht16k33_blink_rate( unsigned char rate )
{
        unsigned char buf[ 2 ];

        if ( rate > 3 )
                rate = 0; // turn off if not sure

        buf[ 0 ] = ( HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | ( rate << 1 ) );

        return i2c_write( buf, 1 );
}

int ht16k33_init( void )
{
        unsigned char buf[ 2 ];
        int rc = 0;

        buf[ 0 ] = HT16K33_CMD_SETUP;
        if ( i2c_write( buf, 1 ) == 0 )
        {
                ht16k33_update();
        }
        else
        {
                rc = 1;
        }

        return rc;
}

int msleep(unsigned long milisec)
{
        struct timespec req={0};
        time_t sec=(int)(milisec/1000);
        milisec=milisec-(sec*1000);
        req.tv_sec=sec;
        req.tv_nsec=milisec*1000000L;
        while(nanosleep(&req,&req)==-1)
                continue;
        return 1;
}

int main( int argc, char *argv[] )
{

        fprintf ( stdout, "LED Clock V6.00" );

        char filename[ 20 ];

        snprintf( filename, 19, "/dev/i2c-%d", i2c_bus );
        handle = open( filename, O_RDWR );
        if ( handle < 0 )
        {
                fprintf( stderr, "Error opening device: %s\n", strerror( errno ) );
                exit( 1 );
        }

        if ( ioctl( handle, I2C_SLAVE, HT16K33_ADDR ) < 0 )
        {
                fprintf( stderr, "IOCTL Error: %s\n", strerror( errno ) );
                exit( 1 );
        }

        /* Usage */
        ht16k33_blink_rate( 0 );
        if ( ht16k33_init() == 0 )
        {
                int cmin;
                int chrs;
                int colon = 0;

                /* Display test */
/*              int n;
                for ( n = 0; n <= 26; n++ )
                {
                        ht16k33_brightness( n );
                        buffer_write_digit( 0, n );
                        buffer_write_digit( 1, n );
                        buffer_write_digit( 3, n );
                        buffer_write_digit( 4, n );
                        ht16k33_update();
                        msleep( 50 );
                }
                ht16k33_blink_rate( 1 );
                buffer_write_digit( 0, 17 );
                buffer_write_digit( 1, 24 );
                buffer_write_digit( 3, 16 );
                buffer_write_digit( 4, 24 );
                ht16k33_update();
                msleep(2500);
                ht16k33_blink_rate( 0 );
                ht16k33_update();
*/

                /* Main loop */
                while (1)
                {
                        time_t ltime;
                        struct tm *Tm;

                        ltime=time(NULL);
                        Tm=localtime(&ltime);

                        cmin = Tm->tm_min;
                        chrs = Tm->tm_hour;

                        if ( chrs < 10 )
                        {
                                buffer_write_digit (0, 27);
                        }
                        else
                        {
                                buffer_write_digit( 0, chrs / 10 );
                        }
                        buffer_write_digit( 1, chrs % 10 );
                        buffer_write_digit( 2, colon );
                        buffer_write_digit( 3, cmin / 10 );
                        buffer_write_digit( 4, cmin % 10 );

                        colon = abs( colon - 2 );

                        ht16k33_update();

                        msleep( 500 );
                }
        }
        close( handle );
        return 0;
}

