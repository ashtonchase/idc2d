

#ifndef   O_IMAGE
#define   O_IMAGE
#include <FL/Fl_Shared_Image.H>

int oi_setup(int height, int width);
void oi_set_colormap(const int map);
void oi_set_source(const int src_index);

Fl_Shared_Image* oi_get_image();

int oi_save_image(char *filename);
int oi_save_data(const char *filename);
int oi_get_image_pixel( const int x
                      , const int y
                      , uchar &r
                      , uchar &g
                      , uchar &b
                      );
#endif /* O_IMAGE   */
