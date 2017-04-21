#include <iostream>

#include <opencv2/opencv.hpp>
#include <fstream>
#include "Image.h"
#include "JpegDecoder.h"

using namespace cv;



long estimateFileSize(std::ifstream& file) {
    long begin = file.tellg();
    file.seekg(0, std::ios_base::end);
    long end = file.tellg();
    file.seekg(0, std::ios_base::beg);
    return end - begin;
}

Image* readJPEG(char* fileName) {
    std::ifstream jpegFile;
    jpegFile.open(fileName, std::ios::in | std::ios::binary);

    long fileSize = estimateFileSize(jpegFile);
    if (fileSize == 0) {
        std::cout << "file " << fileName << " not found" << std::endl;
        return nullptr;
    }
    std::cout << "file size: " << fileSize << " bytes" << std::endl;

    byte* fileBuf = new byte[fileSize];
    jpegFile.read((char *) fileBuf, fileSize);
    jpegFile.close();

    JpegDecoder decoder;
    return decoder.decode(fileBuf);
}

int main(int argc, char** argv)
{
    if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    Image* jpegImage = readJPEG(argv[1]);
    if (jpegImage == nullptr) {
        std::cout << "null image" << std::endl;
        return -1;
    }

    Mat image(jpegImage->getHeight(), jpegImage->getWidth(), CV_8UC3, jpegImage->getData());


    namedWindow("Display Image", WINDOW_AUTOSIZE );
    imshow("Display Image", image);
    waitKey(0);
//    imwrite("road.bmp", image);
    delete jpegImage;
    return 0;
}

