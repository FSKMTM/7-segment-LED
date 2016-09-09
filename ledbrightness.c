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

#define HT16K33_BLINK_CMD       0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF       0
#define HT16K33_BLINK_2HZ       1
#define HT16K33_BLINK_1HZ       2
#define HT16K33_BLINK_HALFHZ    3

#define HT16K33_CMD_SETUP       0x21
#define HT16K33_CMD_BRIGHTNESS  0xE0
#define HT16K33_ADDR            0x70

#define BUFFER_SIZE             8

unsigned short displaybuffer[ BUFFER_SIZE ];
int handle  = 0;
int i2c_bus = 1;


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

int main( int argc, char *argv[] )
{

    if ( argc < 2 )
    {
        printf ( "Usage: %s brightness\n", argv[0] );
	exit( 1 );
    }

    int t;
    if(sscanf( argv[1], "%d", &t) == 0)
    {
        fprintf ( stderr, "Error: brightness must be number\n", strerror ( errno ) );
        exit ( 1 );
    }

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
    int n = ( int )strtoul( argv[ 1 ], NULL, 0 );
    if ( n < 0 || n > 15 )
    {
        fprintf ( stderr, "Warning: valid brightness values are between 0 and 15\n", strerror ( errno ) );
    }
    if ( n < 0 )
        n = 0;
    if ( n > 15 )
        n = 15;
    buffer_clear();
    ht16k33_brightness ( n );
    /*ht16k33_update();*/
    printf ( "LED brightness set to %d\n", n );
    close( handle );
    return 0;
}

