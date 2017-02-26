#include <math.h>
#include <stdlib.h>
#include <png.h>
#include <errno.h>
#include "o_image.h"
#include "i_dc.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


#define CM_DEFAULT  0
#define CM_GRAY     1
#define CM_JET      2
#define CM_HOT      3
#define CM_COLD     4
#define CM_XY       -1

#define CS_TOTAL    0
#define CS_X        1
#define CS_Y        2
#define CS_XY       3
#define CS_V        4

//#include <omp.h>
/*****************************************************************************/
Fl_Image *o_image_p;
unsigned char   *image_data;
IDC_FP          *dvalues;

int             linesize;
int             lines;

static IDC_FP   i_max;
static IDC_FP   i_min;

int             colormap    = CM_DEFAULT;
int             src_index   = CS_TOTAL;
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void
oi_default_colormap()
{
    // converts IDC_FP value into a pix intensity
    const IDC_FP i_p75 = 0.75;
    const IDC_FP i_p50 = 0.50;
    const IDC_FP i_p25 = 0.25;
    const int    w     = i_dc_get_nx2();
    const int    h     = i_dc_get_ny2();
    const IDC_FP d     = i_max - i_min;
    unsigned long index;
    IDC_FP i_point;

    int i, j;
    
    #pragma omp parallel private(i, j, index, i_point)
    {
    #pragma omp for
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            i_point = (dvalues[i * w + j] - i_min) / d; 
            index = 3 * ((j - 1) + (i - 1) * (w - 2));            

            if (i_point > i_p75)
            {
                image_data[index++] = 0xFF;
                image_data[index++] = 0xFF;
                image_data[index]   = (unsigned char)((i_point - i_p75) * 0x3FF);
            }
            else if (i_point > i_p50)
            {
                image_data[index++] = 0xFF;
                image_data[index++] = (unsigned char)((i_point - i_p50) * 0x3FF);
                image_data[index]   = 0;
            }
            else if (i_point > i_p25)
            {
                image_data[index++] = (int)(sqrt(i_point * 4 - 1) * 0xFF);
                image_data[index++] = 0;
                image_data[index]   = (unsigned char)(0x17F * (1 - 2 * i_point));
            }
            else if (0 < i_point)
            {
                image_data[index++] = 0;
                image_data[index++] = 0;
                image_data[index]   = (unsigned char)(i_point * 0x2FF);
            }
            else
            {
                image_data[index++] = 0;
                image_data[index++] = 0;
                image_data[index]   = 0;
            }
        }
    }
    }
}
/*****************************************************************************/
void
oi_hot_colormap()
{
    const int    w  = i_dc_get_nx2();
    const int    h  = i_dc_get_ny2();

    const IDC_FP d  = i_max - i_min;
    const IDC_FP m  = (i_max + i_min) / 2;
    const IDC_FP q  = (m + i_min) / 2;
    const IDC_FP tq = (i_max + m) / 2;

    unsigned long index;
    IDC_FP i_value;

    int i, j;
    #pragma omp parallel private(i, j, index, i_value)
    {
    #pragma omp for
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            i_value = dvalues[i * w + j];
            index = 3 * ((j - 1) + (i - 1) * (w - 2));
            if (i_min <= i_value)
            {
                image_data[index++] = (unsigned char)((m < i_value) ? 0xFF : ((i_value - i_min) * 0x1FF) / d);
                image_data[index++] = (unsigned char)((q > i_value) ? 0 : (tq < i_value ? 0xFF : (((i_value - q) * 0x1FF) / d)));
                image_data[index]   = (unsigned char)((m < i_value) ? ((i_value - m) * 0x1FF) / d : 0);
            }
            else
            {
                image_data[index++] = 0;
                image_data[index++] = 0;
                image_data[index]   = 0;
            }
        }
    }
    }
}
/*****************************************************************************/
void
oi_cold_colormap()
{
    const int    w  = i_dc_get_nx2();
    const int    h  = i_dc_get_ny2();
    const IDC_FP d  = i_max - i_min;
    const IDC_FP m  = (i_max + i_min) / 2;

    int i, j;
    unsigned long index;
    IDC_FP i_value;

    #pragma omp parallel private(i, j, index, i_value)
    {
    #pragma omp for
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            i_value = dvalues[i * w + j];
            index = 3 * ((j - 1) + (i - 1) * (w - 2));            
            if (i_min < i_value)
            {
                image_data[index++] = (unsigned char)((m < i_value) ? (((i_value - m) * 0x1FF)) / d : 0);
                image_data[index++] = (unsigned char)((m < i_value) ? 0xFF : (((i_value - i_min) * 0x1FF) / d));
                image_data[index]   = (unsigned char)(((i_max - i_value) * 128) / d);
            }
            else
            {
                image_data[index++] = 0;
                image_data[index++] = 0;
                image_data[index]   = 0;
            }
        }
    }
    }
}
/*****************************************************************************/
void
oi_gray_colormap()
{
    const int    w     = i_dc_get_nx2();
    const int    h     = i_dc_get_ny2();
    const IDC_FP d     = i_max - i_min;
    IDC_FP i_value;

    int i, j;
    unsigned long index;
    unsigned char i_point;
    
    #pragma omp parallel private(i, j, index, i_point, i_value)
    {
    #pragma omp for
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            i_value = dvalues[i * w + j];
            i_point = (unsigned char)(((i_value - i_min) * 255) / d); 
            index = 3 * ((j - 1) + (i - 1) * (w - 2));
            if (i_min <= i_value)
            {
                image_data[index++] = i_point;
                image_data[index++] = i_point;
                image_data[index]   = i_point;
            }
            else
            {
                image_data[index++] = 0;
                image_data[index++] = 0;
                image_data[index]   = 0;
            }
        }
    }
    }
}
/*****************************************************************************/
void
oi_jet_colormap()
{
    const int    w  = i_dc_get_nx2();
    const int    h  = i_dc_get_ny2();
    const IDC_FP d  = i_max - i_min;
    const IDC_FP m  = (i_max + i_min) / 2;
    unsigned long index;
    IDC_FP i_point, i_value;

    int i, j;
    #pragma omp parallel private(i, j, index, i_point, i_value)
    {
    #pragma omp for
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            i_value = dvalues[i * w + j];
            i_point = (i_value - i_min) / d; 
            index = 3 * ((j - 1) + (i - 1) * (w - 2));

            if  (0.75 < i_point)
            { //6
	            image_data[index++] = 0xFF;
	            image_data[index++] = (unsigned char)(((i_max - i_value) * 0x3FF) / d);
	            image_data[index]   = 0;
            }
            else if (0.5 < i_point)
            {
	            image_data[index++] = (unsigned char)(((i_value - m) * 0x3FF) / d);
	            image_data[index++] = 0xFF;
	            image_data[index]   = 0;
            }
            else if (0.25 < i_point)
            {
	            image_data[index++] = 0;
	            image_data[index++] = 0xFF;
	            image_data[index]   = (unsigned char)(((m - i_value) * 0x3FF) / d);
            }
            else if (i_min < i_value)
            {
	            image_data[index++] = 0;
	            image_data[index++] = (unsigned char)(((i_value - i_min) * 0x3FF) / d);
                image_data[index]   = 0xFF;
            }
            else
            {
                image_data[index++] = 0;
                image_data[index++] = 0;
                image_data[index]   = 0;
            }
        }
    }
    }
}
/*****************************************************************************/
void
oi_xy_colormap()
{
    IDC_FP minX, minY, maxX, maxY;
    i_dc_get_minmax(IDC_X, minX, maxX);
    i_dc_get_minmax(IDC_Y, minY, maxY);

    IDC_FP m        = min(minX, minY);
    IDC_FP M        = max(maxX, maxY);

    const int    w  = i_dc_get_nx2();
    const int    h  = i_dc_get_ny2();
    const IDC_FP d  = M - m;

    IDC_FP *dvaluesX = i_dc_get_idc_x();
    IDC_FP *dvaluesY = i_dc_get_idc_y();

    IDC_FP i_valueX;
    IDC_FP i_valueY;

    int i, j;
    unsigned long index;
    unsigned char i_pointX;
    unsigned char i_pointY;
    
    #pragma omp parallel private(i, j, index, i_pointX, i_pointY, i_valueX, i_valueY)
    {
    #pragma omp for
    for (i = 1; i < h - 1; i++)
    {
        for (j = 1; j < w - 1; j++)
        {
            i_valueX = dvaluesX[i * w + j];
            i_valueY = dvaluesY[i * w + j];

            if (m < i_valueX)
                i_pointX = (unsigned char)(sqrt((i_valueX - m) / d) * 255); 
            else
                i_pointX = 0;
            if (m < i_valueY)
                i_pointY = (unsigned char)(sqrt((i_valueY - m) /d ) * 255); 
            else
                i_pointY = 0;

            index = 3 * ((j - 1) + (i - 1) * (w - 2));

            image_data[index]       = i_pointY;
            image_data[index + 1]   = i_pointX;
            image_data[index + 2]   = 0;
        }
    }
    }
}
/*****************************************************************************/
int          
oi_setup(int height, int width)
{
    linesize = width;
    lines = height;

    if (NULL != o_image_p)
    {
        delete o_image_p;
        o_image_p = NULL;
    }
    if (image_data)
    {
        delete [] image_data;
        image_data = NULL;
    }
    image_data = new unsigned char [3 * width * height];
    return (0);
}
/*****************************************************************************/
Fl_Shared_Image*          
oi_get_image(void)
{
    switch (src_index)
    {
    case CS_X:
        dvalues = i_dc_get_idc_x();
        i_dc_get_minmax(IDC_X, i_min, i_max);
        break;
    case CS_Y:
        dvalues = i_dc_get_idc_y();
        i_dc_get_minmax(IDC_Y, i_min, i_max);
        break;
    case CS_V:
        dvalues = i_dc_get_u();
        i_dc_get_minmax(IDC_V, i_min, i_max);
        break;
    case CS_XY:
        colormap = CM_XY;
    default:
        dvalues = i_dc_get_idc();
        i_dc_get_minmax(IDC_A, i_min, i_max);
        break;
    }    

    switch (colormap)
    {
    case CM_GRAY:
        oi_gray_colormap();
        break;
    case CM_HOT:
        oi_hot_colormap();
        break;
    case CM_COLD:
        oi_cold_colormap();
        break;
    case CM_JET:
        oi_jet_colormap();
        break;
    case CM_XY:
        oi_xy_colormap();
        break;
    default:
        oi_default_colormap();
    }
    o_image_p = new Fl_RGB_Image(image_data, linesize, lines);

    return (Fl_Shared_Image *)o_image_p;
}
/*****************************************************************************/
int
oi_save_image(char *filename)
{
    FILE           *pf;
    png_structp     png_ptr;
    png_infop       info_ptr;
    png_bytep      *row_pointers;
    int             i;

    if (NULL == (pf = fopen(filename, "wb")))
        return errno;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);                                                                                           
        fclose(pf);                                                                                                                              
        return errno;
    }

    png_init_io(png_ptr, pf);

    png_set_IHDR(png_ptr, info_ptr, linesize, lines, 8,
                PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    row_pointers = new png_bytep[lines];
    
    for (i = 0; i < lines; i++)
    {
        row_pointers[i] = image_data + o_image_p->d() * i  * linesize;
    }
    png_set_rows(png_ptr, info_ptr, row_pointers);

    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    png_write_end(png_ptr, NULL); 
    png_destroy_write_struct(&png_ptr, &info_ptr);

    delete [] row_pointers;

    fclose(pf);

    return 0;
}
/*****************************************************************************/
int
oi_save_data(const char *filename)
{
    const int   w       = i_dc_get_nx2();
    const int   h       = i_dc_get_ny2();

    if (!(filename && dvalues && w && h))
        return -1;

    FILE *pf = NULL;

    if (NULL == (pf = fopen(filename, "wb")))
    {
        return errno;
    }
    for (int i = 1; i < h - 1; i++)
    {
        for (int j = 1; j < w - 1; j++)
        {
            fprintf(pf, "%f", dvalues[i * w + j]);
            if (j + 1 < w - 1)
                fputc(0x20, pf);
        }
        fputc(0x0A, pf);
    }
    fclose(pf);

    return 0;
}

/*****************************************************************************/
void
oi_set_colormap(const int map)
{
    colormap = map;
}
/*****************************************************************************/
int 
oi_get_image_pixel( const int x
                  , const int y
                  , uchar &r
                  , uchar &g
                  , uchar &b
                  )
{
    unsigned int index;
    if (image_data 
        && (0 <= x) 
        && (x < linesize) 
        && (0 <= y)
        && (y < lines))
    {
        index = 3 * (x + linesize * y);
        r = image_data[index];
        g = image_data[index + 1];
        b = image_data[index + 2];
        return 0;
    }
    return -1;
}
/*****************************************************************************/
void
oi_set_source(const int index)
{
    src_index = index;
    i_dc_get_minmax(index, i_min, i_max);
}
/*****************************************************************************/
