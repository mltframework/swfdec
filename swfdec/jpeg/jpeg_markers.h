
#define JPEG_MARKER_STUFFED		0x00
#define JPEG_MARKER_TEM			0x01
#define JPEG_MARKER_RES			0x02

#define JPEG_MARKER_SOF_0		0xc0
#define JPEG_MARKER_SOF_1		0xc1
#define JPEG_MARKER_SOF_2		0xc2
#define JPEG_MARKER_SOF_3		0xc3
#define JPEG_MARKER_DHT			0xc4
#define JPEG_MARKER_SOF_5		0xc5
#define JPEG_MARKER_SOF_6		0xc6
#define JPEG_MARKER_SOF_7		0xc7
#define JPEG_MARKER_JPG			0xc8
#define JPEG_MARKER_SOF_9		0xc9
#define JPEG_MARKER_SOF_10		0xca
#define JPEG_MARKER_SOF_11		0xcb
#define JPEG_MARKER_DAC			0xcc
#define JPEG_MARKER_SOF_13		0xcd
#define JPEG_MARKER_SOF_14		0xce
#define JPEG_MARKER_SOF_15		0xcf

#define JPEG_MARKER_RST_0		0xd0
#define JPEG_MARKER_RST_1		0xd1
#define JPEG_MARKER_RST_2		0xd2
#define JPEG_MARKER_RST_3		0xd3
#define JPEG_MARKER_RST_4		0xd4
#define JPEG_MARKER_RST_5		0xd5
#define JPEG_MARKER_RST_6		0xd6
#define JPEG_MARKER_RST_7		0xd7
#define JPEG_MARKER_SOI			0xd8
#define JPEG_MARKER_EOI			0xd9
#define JPEG_MARKER_SOS			0xda
#define JPEG_MARKER_DQT			0xdb
#define JPEG_MARKER_DNL			0xdc
#define JPEG_MARKER_DRI			0xdd
#define JPEG_MARKER_DHP			0xde
#define JPEG_MARKER_EXP			0xdf
#define JPEG_MARKER_APP(x)		(0xe0 + (x))
#define JPEG_MARKER_JPG_(x)		(0xf0 + (x))
#define JPEG_MARKER_COM			0xfe

#define JPEG_MARKER_JFIF		JPEG_MARKER_APP(0)

