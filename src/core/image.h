#ifndef TSS_IMAGE_H // лок от дубля
#define TSS_IMAGE_H

typedef struct
{
    int width;
    int height;
    int channels;
    unsigned char* pixels;
} Image;

Image* image_create(int width, int height, int channels);
void image_destroy(Image* image);

#endif