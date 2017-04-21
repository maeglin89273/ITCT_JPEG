//
// Created by maeglin89273 on 4/20/17.
// reference from https://github.com/xcore/sc_jpeg/blob/master/module_jpeg_decoder/src/idct.c

#ifndef ITCT_JPEG_IDCT_H
#define ITCT_JPEG_IDCT_H


class FIDCT {
private:
    const static int W1 = 2841;
    const static int W2 = 2676;
    const static int W3 = 2408;
    const static int W5 = 1609;
    const static int W6 = 1108;
    const static int W7 = 565;

    int *iclip;
    int *iclp;

public:
    FIDCT();
    void doFIDCT(int *block);
    void idctRow(int *block);
    void idctCol(int *block);
    ~FIDCT();
};


#endif //ITCT_JPEG_IDCT_H
