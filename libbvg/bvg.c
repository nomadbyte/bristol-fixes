/*
 * We are going to take a file format and convert the content into a bitmap 
 * array based on basic (bristol) vector graphics. The main goal is to render
 * silkscreen bitmaps for fonts, the overlays for the synths.
 * 
 * The graphics will be referenced on 1000 by 1000 arrays? No, the format 
 * should include it's rough layout. We should have a subformat for fonts so
 * we can call 'font/size' for example. We need a 'fill' option for boxes, a
 * line option, each should include a color.
 *
 * Type: line, linearray, fill, <sub>
 * color: 0xAARRGGBB
 * coord: x/y
 */
#include <stdio.h>
#include <math.h>
#include <brightoninternals.h>

#include <font1.h>

#ifndef ANDROID_COLORS
static brightonWindow *bwin;
#endif

/*
 * We have the bitmap, we are given an area on that bitmap, we now render the
 * fill into that area.
 */
static void
bvgRenderFill(brightonBitmap *bm, bvgImage *image, int stage,
	int ox, int oy, int ow, int oh)
{
	float sx, sy, ex, ey;
	float x, y;
	int c = image->element[stage].line.color;

#ifndef ANDROID_COLORS
	c = brightonGetGC(bwin,
		(c>>8) & 0x0000ff00,
		c & 0x0000ff00,
		(c << 8) & 0x0000ff00); 
#endif

	/*
	 * Starting X point: origin-x + stage-x * stage-w / bitmap-w
	 */
	if ((sx = ox + image->element[stage].line.x * ow / image->width)
		>= bm->width)
		return;

	/*
	 * Starting Y point: origin-y + stage-y * stage-h / bitmap-h
	 */
	if ((sy = oy + image->element[stage].line.y * oh / image->height)
		>= bm->height)
		return;

	/*
	 * Ending X point: origin-x + stage-x * stage-w / bitmap-w
	 */
	if ((ex = ox + image->element[stage].line.X * ow / image->width)
		>= bm->width)
		return;

	/*
	 * Ending Y point: origin-y + stage-y * stage-h / bitmap-h
	 */
	if ((ey = oy + image->element[stage].line.Y * oh / image->height)
		>= bm->height)
		return;

	// Normalise
	if (sx > ex)
	{
		float tx = ex;
		ex = sx;
		sx = tx;
	}
	// Normalise
	if (sy > ey)
	{
		float ty = ey;
		ey = sy;
		sy = ty;
	}

	for (x = sx; x <= ex; x += 1)
	{
		for (y = sy; y <= ey; y += 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;
			// Set pixel x, y
			//printf("set %i %i\n", (int) x, (int) y);
			bm->pixels[(int) (round(x) + ((int) y) * bm->width)] = c;
		}
	}
}

static void
bvgSetPoint(brightonBitmap *bm, int style, int x, int y, int c)
{
	int nx, ny;

	for (nx = 0; nx < style; nx++)
		for (ny = 0; ny < style; ny++)
			bm->pixels[nx + x + (ny + y) * bm->width] = c;
}

/*
 * We have the bitmap, we are given an area on that bitmap, we now render the
 * line into that area.
 */
static int
bvgRenderVector(brightonBitmap *bm, bvgImage *image, bvgVect *v, int c,
	int style, int ox, int oy, int ow, int oh)
{
	float sx, sy, ex, ey;
	float dx, dy;
	float x, y;
	int i, startx, starty, w = 0;

	if (style <= 0)
		style = 1;

	if (v->count < 2)
		return(0);

#ifndef ANDROID_COLORS
	c = brightonGetGC(bwin,
		(c>>8) & 0x0000ff00,
		c & 0x0000ff00,
		(c << 8) & 0x0000ff00); 
#endif

	/*
	 * Starting X point: origin-x + stage-x * stage-w / bitmap-w
	if ((sx = ox + v->coords[0].x * ow / image->width)
	 */
	if ((sx = ox * bm->width / image->width) >= bm->width)
		return(0);

	/*
	 * Starting Y point: origin-y + stage-y * stage-h / bitmap-h
	if ((sy = oy + v->coords[0].y * oh / image->height)
	 */
	if ((sy = oy * bm->height / image->height) >= bm->height)
		return(0);

	startx = sx;
	starty = sy;

	if ((sx = startx + ow * v->coords[0].x * bm->width / (image->width * 100))
		>= bm->width)
		return(0);
	if ((sy = starty + oh * v->coords[0].y * bm->height / (image->height * 100))
		>= bm->height)
		return(0);

	for (i = 1; i < v->count; i++)
	{
		if (v->coords[i].x < 0)
		{
			if (++i >= v->count)
				return(0);

			if ((sx = startx + ow * v->coords[i].x * bm->width / (image->width * 100))
				>= bm->width)
				return(0);
			if ((sy = starty + oh * v->coords[i].y * bm->height / (image->height * 100))
				>= bm->height)
				return(0);

			continue;
		}

		/*
		 * Ending X point: origin-x + stage-x * stage-w / bitmap-w
		 */
		if ((ex = startx + ow * v->coords[i].x * bm->width / (image->width * 100))
			>= bm->width)
			return(w - startx);

		/*
		 * Ending Y point: origin-y + stage-y * stage-h / bitmap-h
		 */
		if ((ey = starty + oh * v->coords[i].y * bm->height / (image->height * 100))
			>= bm->height)
			return(w - startx);

		if (ex > w) w = ex;

		/*
		 * There are 8 cases we need to check for with 4 net actions
		 */
		if ((sx <= ex) && (sy <= ey) && ((ex - sx) >= (ey - sy)))
		{
			dy = ((float) (ey - sy)) / (ex - sx);
			y = sy;

			for (x = sx; x <= ex; x += 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				y += dy;
			}
		} else if ((sx <= ex) && (sy > ey) && ((ex - sx) >= (sy - ey))) {
			dy = ((float) (ey - sy)) / (ex - sx);
			y = sy;

			for (x = sx; x <= ex; x += 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				y += dy;
			}
		} else if ((sx >= ex) && (sy <= ey) && ((sx - ex) >= (ey - sy))) {
			dy = ((float) (sy - ey)) / (ex - sx);
			y = sy;

			for (x = sx; x >= ex; x -= 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				y += dy;
			}
		} else if ((sx >= ex) && (sy >= ey) && ((sx - ex) >= (sy - ey))) {
			dy = ((float) (sy - ey)) / (ex - sx);
			y = sy;

			for (x = sx; x >= ex; x -= 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				y += dy;
			}
		} else if ((sx <= ex) && (sy <= ey) && ((ex - sx) <= (ey - sy))) {
			dx = ((float) (ex - sx)) / (ey - sy);
			x = sx;

			for (y = sy; y <= ey; y += 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				x += dx;
			}
		} else if ((sx >= ex) && (sy <= ey) && ((sx - ex) <= (ey - sy))) {
			dx = ((float) (ex - sx)) / (ey - sy);
			x = sx;

			for (y = sy; y <= ey; y += 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				x += dx;
			}
		} else if ((sx <= ex) && (sy > ey) && ((ex - sx) >= (sy - ey))) {
			dx = ((float) (sx - ex)) / (ey - sy);
			x = sx;

			for (y = sy; y >= ey; y -= 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				x += dx;
			}
		} else {
			dx = ((float) (sx - ex)) / (ey - sy);
			x = sx;

			for (y = sy; y >= ey; y -= 1)
			{
				if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
					break;

				bvgSetPoint(bm, style, round(x), round(y), c);
				x += dx;
			}
		}

		sx = ex;
		sy = ey;
	}
	return(w - startx);
}

/*
 * We have the bitmap, we are given an area on that bitmap, we now render the
 * line into that area.
 */
static void
bvgRenderLine(brightonBitmap *bm, bvgImage *image, int stage,
	int ox, int oy, int ow, int oh)
{
	float sx, sy, ex, ey;
	float dx, dy;
	float x, y;
	int c = image->element[stage].line.color;
	int style = image->element[stage].line.type & BVG_STYLE_MASK;

	if (style <= 0)
		style = 1;

#ifndef ANDROID_COLORS
	c = brightonGetGC(bwin,
		(c>>8) & 0x0000ff00,
		c & 0x0000ff00,
		(c << 8) & 0x0000ff00); 
#endif

	/*
	 * Starting X point: origin-x + stage-x * stage-w / bitmap-w
	 */
	if ((sx = ox + image->element[stage].line.x * ow / image->width)
		>= bm->width)
		return;

	/*
	 * Starting Y point: origin-y + stage-y * stage-h / bitmap-h
	 */
	if ((sy = oy + image->element[stage].line.y * oh / image->height)
		>= bm->height)
		return;

	/*
	 * Ending X point: origin-x + stage-x * stage-w / bitmap-w
	 */
	if ((ex = ox + image->element[stage].line.X * ow / image->width)
		>= bm->width)
		return;

	/*
	 * Ending Y point: origin-y + stage-y * stage-h / bitmap-h
	 */
	if ((ey = oy + image->element[stage].line.Y * oh / image->height)
		>= bm->height)
		return;

	/*
	 * There are 8 cases we need to check for with 4 net actions
	 */
	if ((sx <= ex) && (sy <= ey) && ((ex - sx) >= (ey - sy)))
	{
		dy = ((float) (ey - sy)) / (ex - sx);
		y = sy;

		for (x = sx; x <= ex; x += 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			y += dy;
		}
	} else if ((sx <= ex) && (sy > ey) && ((ex - sx) >= (sy - ey))) {
		dy = ((float) (ey - sy)) / (ex - sx);
		y = sy;

		for (x = sx; x <= ex; x += 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			y += dy;
		}
	} else if ((sx >= ex) && (sy <= ey) && ((sx - ex) >= (ey - sy))) {
		dy = ((float) (sy - ey)) / (ex - sx);
		y = sy;

		for (x = sx; x >= ex; x -= 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			y += dy;
		}
	} else if ((sx >= ex) && (sy >= ey) && ((sx - ex) >= (sy - ey))) {
		dy = ((float) (sy - ey)) / (ex - sx);
		y = sy;

		for (x = sx; x >= ex; x -= 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			y += dy;
		}
	} else if ((sx <= ex) && (sy <= ey) && ((ex - sx) <= (ey - sy))) {
		dx = ((float) (ex - sx)) / (ey - sy);
		x = sx;

		for (y = sy; y <= ey; y += 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			x += dx;
		}
	} else if ((sx >= ex) && (sy <= ey) && ((sx - ex) <= (ey - sy))) {
		dx = ((float) (ex - sx)) / (ey - sy);
		x = sx;

		for (y = sy; y <= ey; y += 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			x += dx;
		}
	} else if ((sx <= ex) && (sy > ey) && ((ex - sx) >= (sy - ey))) {
		dx = ((float) (sx - ex)) / (ey - sy);
		x = sx;

		for (y = sy; y >= ey; y -= 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			x += dx;
		}
	} else {
		dx = ((float) (sx - ex)) / (ey - sy);
		x = sx;

		for (y = sy; y >= ey; y -= 1)
		{
			if ((x < 0) || (y < 0) || (x >= bm->width) || (y >= bm->height))
				break;

			bvgSetPoint(bm, style, round(x), round(y), c);
			x += dx;
		}
	}
}

static void
bvgRenderString(brightonBitmap *bm, bvgImage *image, int stage,
	int ox, int oy, int ow, int oh)
{
	int i = 0, step, len;

	len = strlen(image->element[stage].string.string);
	step = ow / len - 1;
	ow = step - 1;

	while (image->element[stage].string.string[i] != '\0')
	{
		if (image->element[stage].string.string[i] != ' ')
			bvgRenderVector(bm, image,
				font1[(int) image->element[stage].string.string[i]],
				image->element[stage].line.color,
				image->element[stage].line.type & BVG_STYLE_MASK,
				ox, oy, ow, oh);

		ox += step;

		i++;
	}
}

extern iMap imageMap[];

bvgImage *
findImage(char *name)
{
	int i = 0;

	if (name == NULL)
		return(NULL);

	while (imageMap[i].image != NULL)
	{
		if (strcmp(name, imageMap[i].name) == 0)
			return(imageMap[i].image);
		i++;
	}

	return NULL;
}

static void
bvgMacro(brightonBitmap *bm, bvgImage *image, int x, int y, int w, int h)
{
	int stage;

	if (image == NULL)
		return;

	for (stage = 0; stage < image->count; stage++)
	{
		switch (image->element[stage].line.type & BVG_TYPE_MASK) {
			case BVG_LINE:
				bvgRenderLine(bm, image, stage, x, y, w, h);
				break;
			case BVG_SQUARE:
				bvgRenderFill(bm, image, stage, x, y, w, h);
				break;
			case BVG_STRING:
				bvgRenderString(bm, image, stage,
					x + image->element[stage].string.x * w / 100,
					y + image->element[stage].string.y * h / 100,
					image->element[stage].string.W * w / 100,
					image->element[stage].string.H * h / 100);
				break;
			case BVG_VECT:
				bvgRenderVector(bm, image,
					image->element[stage].vector.vector,
					image->element[stage].line.color,
					image->element[stage].line.type & BVG_STYLE_MASK,
					x + image->element[stage].vector.x * w / 100,
					y + image->element[stage].vector.y * h / 100,
					image->element[stage].vector.W * w / 100,
					image->element[stage].vector.H * h / 100);
				break;
		}
	}
}

int
bvgRender(brightonBitmap *bm, bvgImage *image, int x, int y, int w, int h)
{
	int stage;

	if (image == NULL)
		return(0);

	if (image->color != 0)
		memset(bm->pixels, 0, bm->width * bm->height * sizeof(int));

	for (stage = 0; stage < image->count; stage++)
	{
		switch (image->element[stage].line.type & BVG_TYPE_MASK) {
			case BVG_LINE:
				bvgRenderLine(bm, image, stage, x, y, w, h);
				break;
			case BVG_SQUARE:
				bvgRenderFill(bm, image, stage, x, y, w, h);
				break;
			case BVG_STRING:
				bvgRenderString(bm, image, stage,
					image->element[stage].string.x,
					image->element[stage].string.y,
					image->element[stage].string.W,
					image->element[stage].string.H);
				break;
			case BVG_IMAGE:
				// This is recursion, take care with subimage coords
				bvgMacro(bm,
					image->element[stage].image.image,
					image->element[stage].image.x,
					image->element[stage].image.y,
					image->element[stage].image.w,
					image->element[stage].image.h);
				break;
			case BVG_VECT:
				bvgRenderVector(bm, image,
					image->element[stage].vector.vector,
					image->element[stage].line.color,
					image->element[stage].line.type & BVG_STYLE_MASK,
					image->element[stage].vector.x,
					image->element[stage].vector.y,
					image->element[stage].vector.W,
					image->element[stage].vector.H);
				break;
		}
	}

	return(0);
}

void
bvgRenderInit(brightonWindow *bw)
{
#ifndef ANDROID_COLORS
	bwin = bw;
#endif

	initFont1();
}

void
bvgRenderInt(brightonWindow *bw, char *name, brightonBitmap *bm)
{
	if ((bm == NULL) || (bm->pixels == NULL))
		return;

	if (font1[0] == NULL)
		bvgRenderInit(bw);

	bvgRender(bm, findImage(name), 0, 0, bm->width, bm->height);
}

#ifdef TESTING
main()
{
	brightonBitmap bm;

	bm.width = 788;
	bm.height = 244;
	bm.pixels = malloc(bm.width * bm.height * sizeof(int));

	bvgRender(&bm, &JunoImage, 0, 0, 788, 244);
	return(0);
}
#endif
