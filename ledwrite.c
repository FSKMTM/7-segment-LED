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

#define HT16K33_BLINK_CMD	   	0x80
#define HT16K33_BLINK_DISPLAYON 0x01
#define HT16K33_BLINK_OFF	   	0
#define HT16K33_BLINK_2HZ	   	1
#define HT16K33_BLINK_1HZ	   	2
#define HT16K33_BLINK_HALFHZ	3

#define HT16K33_CMD_SETUP	   	0x21
#define HT16K33_CMD_BRIGHTNESS  0xE0
#define HT16K33_ADDR			0x70

#define BUFFER_SIZE			 8

unsigned short displaybuffer[ BUFFER_SIZE ];
int handle  = 0;
int i2c_bus = 1;

const unsigned char numbertable[] =
/*
7-segment bitmap:

	 --A--
	|     |
	F     B
.	|     |
H	 --G--
.	|     |
	E     C
	|     |
	 --D--

bit order	76543210
segment #	HGFEDCBA
*/

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
	0x77, /*10 A */
	0x7C, /*11 b */
	0x39, /*12 C */
	0x5E, /*13 d */
	0x79, /*14 E */
	0x71, /*15 F */
	0x76, /*16 H */
	0x1E, /*17 J */
	0x38, /*18 L */
	0x54, /*19 n */
	0x5C, /*20 o */
	0x73, /*21 P */
	0x50, /*22 q */
	0x50, /*23 r */
	0x78, /*24 t */
	0x3E, /*25 U */
	0x6E, /*26 Y */
	0x58, /*27 c */
	0x74, /*28 h */
	0x10, /*29 i */
	0x63, /*30 ° */
	0x40, /*31 - */
	0x00, /*32   */
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
	if ( ( digit > 4 ) || ( number > 32 ) )
		return;

	displaybuffer[digit] = numbertable[number];
}

void buffer_write_digit_raw( int digit, int number )
{
	if ( ( digit > 4 ) || ( number > 65535 ) )
		return;

	displaybuffer[digit] = number;
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
		ht16k33_blink_rate( HT16K33_BLINK_OFF );
		ht16k33_brightness( 7 );
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

	if ( argc == 2 && strcmp(argv [1],"--help") == 0 )
	{
		printf ("Use the following values for CH1-4:\n");
		printf ("16=H, 17=J, 18=L, 19=n, 20=o, 21=P,\n");
		printf ("22=q, 23=r, 24=t, 25=U, 26=Y, 27=c,\n");
		printf ("28=h, 29=i, 30=°, 31=-, 32=<space>\n");
		exit ( 0 );
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
	if ( ht16k33_init() == 0 )
	{
		if ( argc >= 5 )
		{
				buffer_write_digit( 0, strtol(argv [1], (char **)NULL, 0));
				buffer_write_digit( 1, strtol(argv [2], (char **)NULL, 0));
				buffer_write_digit( 3, strtol(argv [3], (char **)NULL, 0));
				buffer_write_digit( 4, strtol(argv [4], (char **)NULL, 0));
			if ( argc == 6 )
		{
					buffer_write_digit_raw( 2, strtol(argv [5], (char **)NULL, 0));
				}
		ht16k33_update();
		exit ( 0 );
	}
		else
		{
		printf ( "Usage: %s CH1 CH2 CH3 CH4 [DOTS]\n", argv[0] );
			exit( 1 );

		}
	}

	close( handle );
	return 0;
}
