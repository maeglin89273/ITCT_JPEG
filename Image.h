//
// Created by maeglin89273 on 4/18/17.
//

#ifndef ITCT_JPEG_IMAGE_H
#define ITCT_JPEG_IMAGE_H


#include <opencv2/core/types_c.h>

class Image {
private:
    unsigned char* bgr_values;
    int width;
    int height;

public:
    Image(int height, int width);
    int getWidth();
    int getHeight();
    unsigned char* getData();
    ~Image();
};


#endif //ITCT_JPEG_IMAGE_H
