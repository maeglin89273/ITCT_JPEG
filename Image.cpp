//
// Created by maeglin89273 on 4/18/17.
//

#include "Image.h"
#include <iostream>
Image::Image(int height, int width) {
    this->width = width;
    this->height = height;
    this->bgr_values = new unsigned char[width * height * 3];
}

Image::~Image() {
    delete [] this->bgr_values;
}

unsigned char *Image::getData() {
    return this->bgr_values;
}

int Image::getHeight() {
    return this->height;
}

int Image::getWidth() {
    return this->width;
}
