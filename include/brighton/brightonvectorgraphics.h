#ifndef _BRISTOL_BVG
#define _BRISTOL_BVG

#define BVG_RED		0x00ff0000
#define BVG_GREEN	0x0000ff00
#define BVG_BLUE	0x000000fe
#define BVG_WHITE	0x00ffffff
#define BVG_BLACK	0x00000000

#define BVG_TYPE_MASK	0xff00
#define BVG_STYLE_MASK	0x00ff
#define BVG_NULL	0

#define BVG_LINE	0x100
#define BVG_SQUARE	0x200
#define BVG_STRING	0x300
#define BVG_VECT	0x400 // Uses string rendering using customer character
#define BVG_IMAGE	0x500 // Recurse

#define VG_OPTS_MASK	0xff00

typedef struct BvgCoords {
	short x, y;
} bvgCoords;

typedef struct BvgVect {
	int count;
	bvgCoords *coords;
} bvgVect;

// This can be line and fill
typedef struct BvgLine {
	unsigned short type;
	unsigned int color;
	unsigned short x, y, X, Y;
	char *nothing;
} bvgLine;

typedef struct BvgString {
	unsigned short type;
	unsigned int color;
	unsigned short x, y, W, H;
	char *string;
} bvgString;

typedef struct BvgVector {
	unsigned short type;
	unsigned int color;
	unsigned short x, y, W, H;
	bvgVect *vector;
} bvgVector;

typedef struct BvgReimage {
	unsigned short type;
	unsigned int color;
	unsigned short x, y, w, h;
	struct BvgImage *image;
} bvgReimage;

typedef struct BvgImage {
	int width, height;
	int color;
	int count;
	union {
		bvgLine line;
		bvgString string;
		bvgVector vector;
		bvgReimage image;
	} element[];
} bvgImage;

typedef struct ImageMap {
	char name[1024];
	bvgImage *image;
} iMap;
#endif //_BRISTOL_BVG
