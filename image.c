#include <stdio.h>
#include <stdlib.h>
#include "lib/png.h"
#include "lib/gif_lib.h"
#include "lib/jpeglib.h"
#include "lib/bmplib.h"
#include "lib/tga.h"
#include "display.h"
#include "image.h"

/* 
#define PB  (1.0f/3.0f)
#define PC  (1.0f/3.0f)
#define P0  ((  6.0f- 2.0f*PB       )/6.0f)
#define P2  ((-18.0f+12.0f*PB+ 6.0f*PC)/6.0f)
#define P3  (( 12.0f- 9.0f*PB- 6.0f*PC)/6.0f)
#define Q0  ((       8.0f*PB+24.0f*PC)/6.0f)
#define Q1  ((     -12.0f*PB-48.0f*PC)/6.0f)
#define Q2  ((       6.0f*PB+30.0f*PC)/6.0f)
#define Q3  ((     - 1.0f*PB- 6.0f*PC)/6.0f)

__inline float sinc():
	if (x < -1.0f)
		return(Q0-x*(Q1-x*(Q2-x*Q3)));
	if (x < 0.0f)
		return(P0+x*x*(P2-x*P3));
	if (x < 1.0f)
		return(P0+x*x*(P2+x*P3));
	return(Q0+x*(Q1+x*(Q2+x*Q3)));
*/

/* You can replace default sinc() with following anyone for filter changing

__inline float sinc(float x)
{
	if (x < -1.0f)
		return(0.5f*(4.0f+x*(8.0f+x*(5.0f+x))));
	if (x < 0.0f)
		return(0.5f*(2.0f+x*x*(-5.0f-3.0f*x)));
	if (x < 1.0f)
		return(0.5f*(2.0f+x*x*(-5.0f+3.0f*x)));
	return(0.5f*(4.0f+x*(-8.0f+x*(5.0f-x))));
}

__inline float sinc(float x)
{
	if (x < 0.1f)
		return((2.0f+x)*(2.0f+x)*(2.0f+x)/6.0f);
	if (x < 0.0f)
		return((4.0f+x*x*(-6.0f-3.0f*x))/6.0f);
	if (x < 1.0f)
		return((4.0f+x*x*(-6.0f+3.0f*x))/6.0f);
	return((2.0f-x)*(2.0f-x)*(2.0f-x)/6.0f);
}

__inline float sinc(float x)
{
	float s = (x < 0) ? -x : x;
	if (s <= 1.0f)
		return 1.0f - 2.0f * x * x + x * x * s;
	else
		return 4.0f - 8.0f * s + 5.0f * x * x - x * x * s;
}
*/

__inline float sinc_n2(float x)
{
	return (2.0f + x) * (2.0f + x) * (1.0f + x);
}
__inline float sinc_n1(float x)
{
	return (1.0f - x - x * x) * (1.0f + x);
}
__inline float sinc_1(float x)
{
	return (1.0f + x - x * x) * (1.0f - x);
}
__inline float sinc_2(float x)
{
	return (2.0f - x) * (2.0f - x) * (1.0f - x);
}

__inline dword bicubic(dword i1, dword i2, dword i3, dword i4, float u1, float u2, float u3, float u4)
{
	int r, g, b;
	r = u1 * RGB_R(i1) + u2 * RGB_R(i2) + u3 * RGB_R(i3) + u4 * RGB_R(i4) + 0.5f;
	if(r > 255) r = 255;
	else if(r < 0) r = 0;

	g = u1 * RGB_G(i1) + u2 * RGB_G(i2) + u3 * RGB_G(i3) + u4 * RGB_G(i4) + 0.5f;
	if(g > 255) g = 255;
	else if(g < 0) g = 0;

	b = u1 * RGB_B(i1) + u2 * RGB_B(i2) + u3 * RGB_B(i3) + u4 * RGB_B(i4) + 0.5f;
	if(b > 255) b = 255;
	else if(b < 0) b = 0;

	return RGB(r, g, b);
}

extern void image_zoom_bicubic(dword * src, int srcwidth, int srcheight, dword * dest, int destwidth, int destheight)
{
	dword * temp1, * temp2, * temp3, * temp4, * tempdst;
	float u, v, u1[destwidth], u2[destwidth], u3[destwidth], u4[destwidth];
	int x = 0, y, x2, y1, y2, y3, y4, i, j;

	for(i = 0; i < destheight; i ++)
	{
		x += srcheight;
		x2 = x / destheight;
		temp2 = src + x2 * srcwidth;
		if(x2 == 0) temp1 = temp2; else temp1 = temp2 - srcwidth;
		if(x2 + 2 > srcheight)
		{
			temp3 = temp2;
			if(x2 + 3 > srcheight) temp4 = temp3; else temp4 = temp3 + srcwidth;
		} else {
			temp3 = temp2 + srcwidth;
			temp4 = temp3 + srcwidth;
		}
		tempdst = dest;

		v = ((float)(x - x2 * destheight)) / destheight;
		float v1 = sinc_2(v + 1.0f), v2 = sinc_1(v), v3 = sinc_n1(v - 1.0f), v4 = sinc_n2(v - 2.0f);
		y = 0;

		if(i == 0)
		{
			for(j = 0; j < destwidth; j ++)
			{
				y += srcwidth;
				y2 = y / destwidth;
				y1 = y2 - 1;
				y3 = y2 + 1;
				y4 = y2 + 2;
				if(y1 < 0) y1 = 0;
				if(y3 > srcwidth - 1) y3 = srcwidth - 1;
				if(y4 > srcwidth - 1) y4 = srcwidth - 1;

				u = ((float)y / destwidth) - y2;
				u1[j] = sinc_2(u + 1.0f), u2[j] = sinc_1(u), u3[j] = sinc_n1(u - 1.0f), u4[j] = sinc_n2(u - 2.0f);
				tempdst[j] = bicubic(bicubic(temp1[y1], temp1[y2], temp1[y3], temp1[y4], u1[j], u2[j], u3[j], u4[j]), bicubic(temp2[y1], temp2[y2], temp2[y3], temp2[y4], u1[j], u2[j], u3[j], u4[j]), bicubic(temp3[y1], temp3[y2], temp3[y3], temp3[y4], u1[j], u2[j], u3[j], u4[j]), bicubic(temp4[y1], temp4[y2], temp4[y3], temp4[y4], u1[j], u2[j], u3[j], u4[j]), v1, v2, v3, v4);
			}
		}
		else
		{
			for(j = 0; j < destwidth; j ++)
			{
				y += srcwidth;
				y2 = y / destwidth;
				y1 = y2 - 1;
				y3 = y2 + 1;
				y4 = y2 + 2;
				if(y1 < 0) y1 = 0;
				if(y3 > srcwidth - 1) y3 = srcwidth - 1;
				if(y4 > srcwidth - 1) y4 = srcwidth - 1;

				tempdst[j] = bicubic(bicubic(temp1[y1], temp1[y2], temp1[y3], temp1[y4], u1[j], u2[j], u3[j], u4[j]), bicubic(temp2[y1], temp2[y2], temp2[y3], temp2[y4], u1[j], u2[j], u3[j], u4[j]), bicubic(temp3[y1], temp3[y2], temp3[y3], temp3[y4], u1[j], u2[j], u3[j], u4[j]), bicubic(temp4[y1], temp4[y2], temp4[y3], temp4[y4], u1[j], u2[j], u3[j], u4[j]), v1, v2, v3, v4);
			}
		}
		dest += destwidth;
	}
} 

__inline dword bilinear(dword i1, dword i2, int u, int s)
{
	int r1, g1, b1, r2, g2, b2;
	r1 = RGB_R(i1); g1 = RGB_G(i1); b1 = RGB_B(i1);
	r2 = RGB_R(i2); g2 = RGB_G(i2); b2 = RGB_B(i2);
	return RGB((r1 + u * (r2 - r1) / s), (g1 + u * (g2 - g1) / s), (b1 + u * (b2 - b1) / s));
}

extern void image_zoom_bilinear(dword * src, int srcwidth, int srcheight, dword * dest, int destwidth, int destheight)
{
	dword * temp1, * temp2, * tempdst;
	int x = 0, y, u, v;
	int x2, y1, y2, i, j;

	for(i = 0; i < destheight; i ++)
	{
		x += srcheight;
		x2 = x / destheight;
		temp1 = src + x2 * srcwidth;
		if(x2 < srcheight - 1)
			temp2 = temp1 + srcwidth;
		else
			temp2 = temp1;
		tempdst = dest;

		v = x - x2 * destheight;
		y = 0;

		for(j = 0; j < destwidth; j ++)
		{
			y += srcwidth;
			y1 = y / destwidth;
			if(y1 < srcwidth - 1)
				y2 = y1;
			else
				y2 = y1 + 1;

			u = y - y1 * destwidth;
			tempdst[j] = bilinear(bilinear(temp1[y1], temp1[y2], u, destwidth), bilinear(temp2[y1], temp2[y2], u, destwidth), v, destheight);
		}
		dest += destwidth;
	}
} 

extern void image_rotate(dword * imgdata, dword * pwidth, dword * pheight, dword organgle, dword newangle)
{
	dword ca;
	int temp;
	if(newangle < organgle)
	{
		ca = newangle + 360 - organgle;
	}
	else
		ca = newangle - organgle;
	if(ca == 0)
		return;
	dword * newdata = malloc(sizeof(dword) * *pwidth * *pheight);
	if(newdata == NULL)
		return;
	dword i, j;
	switch(ca)
	{
	case 90:
		for(j = 0; j < *pheight; j ++)
			for(i = 0; i < *pwidth; i ++)
				newdata[i * *pheight + (*pheight - j - 1)] = imgdata[j * *pwidth + i];
		temp = *pheight; *pheight = *pwidth; *pwidth = temp;
		break;
	case 180:
		for(j = 0; j < *pheight; j ++)
			for(i = 0; i < *pwidth; i ++)
				newdata[(*pheight - j - 1) * *pwidth + (*pwidth - i - 1)] = imgdata[j * *pwidth + i];
		break;
	case 270:
		for(j = 0; j < *pheight; j ++)
			for(i = 0; i < *pwidth; i ++)
				newdata[(*pwidth - i - 1) * *pheight + j] = imgdata[j * *pwidth + i];
		temp = *pheight; *pheight = *pwidth; *pwidth = temp;
		break;
	default:
		return;
	}
	memcpy(imgdata, newdata, sizeof(dword) * *pwidth * *pheight);
	free((void *)newdata);
}

/* PNG processing */

/* return value = 0 for success, 1 for bad sig/hdr, 4 for no mem
display_exponent == LUT_exponent * CRT_exponent */

static int image_readpng2(FILE *infile, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	byte sig[8];

	fread(sig, 1, 8, infile);
	if (!png_check_sig(sig, 8))
		return 1;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return 4;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 4;
	}


	if (setjmp(png_ptr->jmpbuf)) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return 1;
	}


	png_init_io(png_ptr, infile);
	png_set_sig_bytes(png_ptr, 8);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_BGR, NULL);

	*pwidth = info_ptr->width;
	*pheight = info_ptr->height;

	if (info_ptr->bit_depth == 16) {
		*bgcolor = RGB(info_ptr->background.red >> 8, info_ptr->background.green >> 8, info_ptr->background.blue >> 8);
	} else if (info_ptr->color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8) {
		if (info_ptr->bit_depth == 1)
			*bgcolor = info_ptr->background.gray ? 0xFFFFFFFF : 0;
		else if (info_ptr->bit_depth == 2)
			*bgcolor = RGB((255/3) * info_ptr->background.gray, (255/3) * info_ptr->background.gray, (255/3) * info_ptr->background.gray);
		else
			*bgcolor = RGB((255/15) * info_ptr->background.gray, (255/15) * info_ptr->background.gray, (255/15) * info_ptr->background.gray);
	} else {
		*bgcolor = RGB(info_ptr->background.red, info_ptr->background.green, info_ptr->background.blue);
	}

	if((*image_data = (dword *)malloc(sizeof(dword) * info_ptr->width * info_ptr->height)) == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return 4;
	}

	png_byte **prowtable = info_ptr->row_pointers;
	dword x, y;
	byte r=0, g=0, b=0;
	dword * imgdata = * image_data;
	switch(info_ptr->color_type){
	case PNG_COLOR_TYPE_GRAY:
		for (y = 0; y < info_ptr->height; y ++){
			png_byte *prow = prowtable[y];
			for (x = 0; x < info_ptr->width; x ++){
				r = g = b = *prow++;
				*imgdata++ = RGB(r,g,b);
			}
		}
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		for (y = 0; y < info_ptr->height; y ++){
			png_byte *prow = prowtable[y];
			for (x = 0; x < info_ptr->width; x ++){
				r = g = b = *prow++;
				prow++;
				*imgdata++ = RGB(r,g,b);
			}
		}
		break;
	case PNG_COLOR_TYPE_RGB:
		for (y = 0; y < info_ptr->height; y ++){
			png_byte *prow = prowtable[y];
			for (x = 0; x < info_ptr->width; x ++){
				b = *prow++;
				g = *prow++;
				r = *prow++;
				*imgdata++ = RGB(r,g,b);
			}
		}
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		for (y = 0; y < info_ptr->height; y ++){
			png_byte *prow = prowtable[y];
			for (x = 0; x < info_ptr->width; x ++){
				b = *prow++;
				g = *prow++;
				r = *prow++;
				prow++;
				*imgdata++ = RGB(r,g,b);
			}
		}
		break;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	png_ptr = NULL;
	info_ptr = NULL;

	return 0;
}

extern int image_readpng(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
	FILE * fp = fopen(filename, "rb");
	if(fp == NULL)
		return -1;
	int result = image_readpng2(fp, pwidth, pheight, image_data, bgcolor);
	fclose(fp);
	return result;
}

extern int image_readgif(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
#define gif_color(c) RGB(palette->Colors[c].Red, palette->Colors[c].Green, palette->Colors[c].Blue)
	GifRecordType RecordType;
	GifByteType *Extension;
	GifRowType LineIn = NULL;
	GifFileType *GifFileIn = NULL;
	ColorMapObject *palette;
	int ExtCode;
	dword i, j;
	if ((GifFileIn = DGifOpenFileName(filename)) == NULL)
		return 1;
	*bgcolor = 0;
	*pwidth = 0;
	*pheight = 0;
	*image_data = NULL;

	do {
		if (DGifGetRecordType(GifFileIn, &RecordType) == GIF_ERROR)
		{
			DGifCloseFile(GifFileIn);
			return 1;
		}

		switch (RecordType) {
			case IMAGE_DESC_RECORD_TYPE:
				if (DGifGetImageDesc(GifFileIn) == GIF_ERROR)
				{
					DGifCloseFile(GifFileIn);
					return 1;
				}
				if((palette = (GifFileIn->SColorMap != NULL) ? GifFileIn->SColorMap : GifFileIn->Image.ColorMap) == NULL)
				{
					DGifCloseFile(GifFileIn);
					return 1;
				}
				*pwidth = GifFileIn->Image.Width;
				*pheight = GifFileIn->Image.Height;
				*bgcolor = gif_color(GifFileIn->SBackGroundColor);
				if((LineIn = (GifRowType) malloc(GifFileIn->Image.Width * sizeof(GifPixelType))) == NULL)
				{
					DGifCloseFile(GifFileIn);
					return 1;
				}
				if((*image_data = (dword *)malloc(sizeof(dword) * GifFileIn->Image.Width * GifFileIn->Image.Height)) == NULL)
				{
					free((void *)LineIn);
					DGifCloseFile(GifFileIn);
					return 1;
				}
				dword * imgdata = *image_data;
				for (i = 0; i < GifFileIn->Image.Height; i ++) {
					if (DGifGetLine(GifFileIn, LineIn, GifFileIn->Image.Width) == GIF_ERROR)
					{
						free((void *)*image_data);
						free((void *)LineIn);
						DGifCloseFile(GifFileIn);
						return 1;
					}
					for(j = 0; j < GifFileIn->Image.Width; j ++)
						*imgdata++ = gif_color(LineIn[j]);
				}
				break;
			case EXTENSION_RECORD_TYPE:
				if (DGifGetExtension(GifFileIn, &ExtCode, &Extension) == GIF_ERROR)
				{
					if(*image_data != NULL)
						free((void *)*image_data);
					if(LineIn != NULL)
						free((void *)LineIn);
					DGifCloseFile(GifFileIn);
					return 1;
				}
				while (Extension != NULL) {
					if (DGifGetExtensionNext(GifFileIn, &Extension) == GIF_ERROR)
					{
						if(*image_data != NULL)
							free((void *)*image_data);
						if(LineIn != NULL)
							free((void *)LineIn);
						DGifCloseFile(GifFileIn);
						return 1;
					}
				}
				break;
			case TERMINATE_RECORD_TYPE:
				break;
			default:
				break;
		}
	}
	while (RecordType != TERMINATE_RECORD_TYPE);

	if(LineIn != NULL)
		free((void *)LineIn);
	DGifCloseFile(GifFileIn);

	return 0;
}

static void my_error_exit(j_common_ptr cinfo)
{
}

static void output_no_message(j_common_ptr cinfo)
{
}

static int image_readjpg2(FILE *infile, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW sline;
	memset(&cinfo, 0, sizeof(struct jpeg_decompress_struct));
	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit = my_error_exit;
	jerr.output_message = output_no_message;
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	if(jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK)
	{
		jpeg_destroy_decompress(&cinfo);
		return 1;
	}
	cinfo.out_color_space = JCS_RGB;
	cinfo.quantize_colors = FALSE;
	cinfo.scale_num   = 1;
	cinfo.scale_denom = 1;
	cinfo.dct_method = JDCT_FASTEST;
	cinfo.do_fancy_upsampling = FALSE;
	if(!jpeg_start_decompress(&cinfo))
	{
		jpeg_destroy_decompress(&cinfo);
		return 1;
	}
	*bgcolor = 0;
	*pwidth = cinfo.output_width;
	*pheight = cinfo.output_height;
	if((sline = (JSAMPROW)malloc(sizeof(JSAMPLE) * 3 * cinfo.output_width)) == NULL)
	{
		jpeg_abort_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return 1;
	}
	if((*image_data = (dword *)malloc(sizeof(dword) * cinfo.output_width * cinfo.output_height)) == NULL)
	{
		free((void *)sline);
		jpeg_abort_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return 1;
	}
	dword * imgdata = *image_data;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		int i;
		jpeg_read_scanlines(&cinfo, &sline, (JDIMENSION) 1);
		for(i = 0; i < cinfo.output_width; i ++)
			*imgdata++ = RGB(sline[i * 3], sline[i * 3 + 1], sline[i * 3 + 2]);
	}
	free((void *)sline);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return 0;
}

extern int image_readjpg(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
	FILE * fp = fopen(filename, "rb");
	if(fp == NULL)
		return -1;
	int result = image_readjpg2(fp, pwidth, pheight, image_data, bgcolor);
	fclose(fp);
	return result;
}

static int image_bmp_to_32color( DIB dib, dword * width, dword * height, dword ** imgdata)  {
    BITMAPINFOHEADER *bi;
    int row_bytes, i, j;
    uchar *src, *pixel_data;
	dword *dest;
    DIB tmp_dib = NULL;
    RGBQUAD *palette;

    bi = (BITMAPINFOHEADER *)dib;
	*imgdata = (dword *)malloc(sizeof(dword) * bi->biWidth * bi->biHeight);
	if(*imgdata == NULL)
		return 1;

    if( bi->biCompression == BI_RLE8 ||
        bi->biCompression == BI_RLE4
    ) {
        tmp_dib = bmp_expand_dib_rle( dib );
        if( tmp_dib == NULL ) {
			free(*imgdata);
			*imgdata = NULL;
            return 1;
        }
        dib = tmp_dib;
    }
	*width = bi->biWidth;
	*height = bi->biHeight;

    palette = (RGBQUAD *)(dib + sizeof(BITMAPINFOHEADER));
    pixel_data = (uchar *)palette + bi->biClrUsed * sizeof(RGBQUAD);
    row_bytes = (bi->biBitCount * bi->biWidth + 31 ) / 32 * 4;

    for( i = 0; i < bi->biHeight; i++ ) {
        int   pos;
        src  = &pixel_data[ (bi->biHeight - i - 1) * row_bytes ];
        dest = (*imgdata) + i * bi->biWidth;
        switch( bi->biBitCount ) {
          case 1:
            for( j = 0; j < bi->biWidth; j++ ) {
                if( *src & ( 0x80 >> (i % 8) ) ) pos = 1;
                else pos = 0;
                if( ( i % 8 ) == 7) src++;
                *dest++ = RGB(palette[ pos ].rgbRed, palette[ pos ].rgbGreen, palette[ pos ].rgbBlue);
            }
            break;
          case 4:
            for( j = 0; j < bi->biWidth; j++ ) {
                pos = *src;
                if( (j % 2) == 0 ) pos >>= 4;
                else  { pos &= 0x0F; src++; }
                *dest++ = RGB(palette[ pos ].rgbRed, palette[ pos ].rgbGreen, palette[ pos ].rgbBlue);
            }
            break;
          case 8:
            for( j = 0; j < bi->biWidth; j++ ) {
                pos = *src++;
                *dest++ = RGB(palette[ pos ].rgbRed, palette[ pos ].rgbGreen, palette[ pos ].rgbBlue);
            }
            break;
		  case 15:
          case 16:
            for( j = 0; j < bi->biWidth; j++ ) {
                ushort rgb = *(ushort *)src;
                src += 2;
                *dest++ = RGB_16to32(rgb);
            }
            break;
          case 24:
            for( j = 0; j < bi->biWidth; j++ ) {
                *dest++ = RGB(*(src + 2), *(src + 1), *src);
                src += 3;
            }
			break;
          case 32:
            for( j = 0; j < bi->biWidth; j++ ) {
                *dest++ = RGB(*(src + 2), *(src + 1), *src);
                src += 4;
            }
            break;
          default:
            for( j = 0; j < bi->biWidth; j++ )
			  *dest++ = 0;
            break;
        }
    }

    if( tmp_dib ) free( tmp_dib );

	return 0;
}

extern int image_readbmp(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
	FILE * fp = fopen(filename, "rb");
	if(fp == NULL)
		return -1;
	DIB bmp;
	bmp = bmp_read_dib_file(fp);
	fclose(fp);
	if(bmp == NULL)
		return 1;
	*bgcolor = 0;
	int result = image_bmp_to_32color(bmp, pwidth, pheight, image_data);
	fclose(fp);
	free((void *)bmp);
	return result;
}

static void image_freetgadata(TGAData * data)
{
	if(data->img_id != NULL)
		free((void *)data->img_id);
	if(data->cmap != NULL)
		free((void *)data->cmap);
	if(data->img_data != NULL);
		free((void *)data->img_data);
	free((void *)data);
}

extern int image_readtga(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor)
{
	TGA *in;
	TGAData *data;

	if((data = (TGAData*)calloc(1, sizeof(TGAData))) == NULL)
		return 1;
	in = TGAOpen((char *)filename, "r");
	if(in == NULL)
	{
		image_freetgadata(data);
		return 1;
	}
	data->flags = TGA_IMAGE_ID | TGA_IMAGE_DATA | TGA_RGB;
	if(in->last != TGA_OK || TGAReadImage(in, data) != TGA_OK)
	{
		TGAClose(in);
		image_freetgadata(data);
		return 1;
	}

	*pwidth = in->hdr.width;
	*pheight = in->hdr.height;
	*bgcolor = 0;
	if((*image_data = (dword *)malloc(sizeof(dword) * *pwidth * *pheight)) == NULL)
	{
		TGAClose(in);
		image_freetgadata(data);
		return 1;
	}

	byte * srcdata = data->img_data;
	dword * imgdata = (*image_data) + in->hdr.x + in->hdr.y * *pwidth;
	int i, j;
	for(j = 0; j < *pheight; j ++)
	{
		for(i = 0; i < *pwidth; i ++)
		{
			switch(in->hdr.depth)
			{
			case 32:
				*imgdata = RGB(*srcdata, *(srcdata + 1), *(srcdata + 2));
				srcdata += 4;
				break;
			case 8:
				*imgdata = (*srcdata > 0) ? 0xFFFFFFFF : 0;
				srcdata ++;
				break;
			default:
				*imgdata = RGB(*srcdata * 255 / 31, *(srcdata + 1) * 255 / 31, *(srcdata + 2) * 255 / 31);
				srcdata += 3;
			}
			if(in->hdr.vert == TGA_RIGHT)
				imgdata ++;
			else
				imgdata --;
		}
		if(in->hdr.vert == TGA_RIGHT && in->hdr.horz == TGA_TOP)
			imgdata -= 2 * *pwidth;
		else if(in->hdr.vert == TGA_LEFT && in->hdr.horz == TGA_BOTTOM)
			imgdata += 2 * *pwidth;
	}

	TGAClose(in);
	image_freetgadata(data);
	return 0;
}
