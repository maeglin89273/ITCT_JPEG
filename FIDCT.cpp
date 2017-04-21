//
// Created by maeglin89273 on 4/20/17.
//

#include "FIDCT.h"

FIDCT::FIDCT() {
    this->iclip = new int[1024];
    this->iclp = this->iclip + 512;
    for (int i = -512; i<512; i++)
        iclp[i] = (i < -256 ) ? -256 : ((i > 255) ? 255 : i);
}


void FIDCT::idctRow(int *block) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;

/* shortcut */
    if (!((x1 = block[4]<<11) | (x2 = block[6]) | (x3 = block[2]) |
    (x4 = block[1]) | (x5 = block[7]) | (x6 = block[5]) | (x7 = block[3]))) {
        block[0]=block[1]=block[2]=block[3]=block[4]=block[5]=block[6]=block[7]=block[0]<<3;
        return;
    }

    x0 = (block[0]<<11) + 128; /* for proper rounding in the fourth stage */

    /* first stage */
    x8 = W7*(x4+x5);
    x4 = x8 + (W1-W7)*x4;
    x5 = x8 - (W1+W7)*x5;
    x8 = W3*(x6+x7);
    x6 = x8 - (W3-W5)*x6;
    x7 = x8 - (W3+W5)*x7;

    /* second stage */
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6*(x3+x2);
    x2 = x1 - (W2+W6)*x2;
    x3 = x1 + (W2-W6)*x3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;

    /* third stage */
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181*(x4+x5)+128)>>8;
    x4 = (181*(x4-x5)+128)>>8;

    /* fourth stage */
    block[0] = (x7+x1)>>8;
    block[1] = (x3+x2)>>8;
    block[2] = (x0+x4)>>8;
    block[3] = (x8+x6)>>8;
    block[4] = (x8-x6)>>8;
    block[5] = (x0-x4)>>8;
    block[6] = (x3-x2)>>8;
    block[7] = (x7-x1)>>8;
}


void FIDCT::idctCol(int * blk) {
    int x0, x1, x2, x3, x4, x5, x6, x7, x8;

    /* shortcut */
    if (!((x1 = (blk[8*4]<<8)) | (x2 = blk[8*6]) | (x3 = blk[8*2]) |
    (x4 = blk[8*1]) | (x5 = blk[8*7]) | (x6 = blk[8*5]) | (x7 = blk[8*3])))
    {
    blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]=blk[8*6]=blk[8*7]=
    this->iclp[(blk[8*0]+32)>>6];
    return;
    }

    x0 = (blk[8*0]<<8) + 8192;

    /* first stage */
    x8 = W7*(x4+x5) + 4;
    x4 = (x8+(W1-W7)*x4)>>3;
    x5 = (x8-(W1+W7)*x5)>>3;
    x8 = W3*(x6+x7) + 4;
    x6 = (x8-(W3-W5)*x6)>>3;
    x7 = (x8-(W3+W5)*x7)>>3;

    /* second stage */
    x8 = x0 + x1;
    x0 -= x1;
    x1 = W6*(x3+x2) + 4;
    x2 = (x1-(W2+W6)*x2)>>3;
    x3 = (x1+(W2-W6)*x3)>>3;
    x1 = x4 + x6;
    x4 -= x6;
    x6 = x5 + x7;
    x5 -= x7;

    /* third stage */
    x7 = x8 + x3;
    x8 -= x3;
    x3 = x0 + x2;
    x0 -= x2;
    x2 = (181*(x4+x5)+128)>>8;
    x4 = (181*(x4-x5)+128)>>8;

    /* fourth stage */
    blk[8*0] = this->iclp[(x7+x1)>>14];
    blk[8*1] = this->iclp[(x3+x2)>>14];
    blk[8*2] = this->iclp[(x0+x4)>>14];
    blk[8*3] = this->iclp[(x8+x6)>>14];
    blk[8*4] = this->iclp[(x8-x6)>>14];
    blk[8*5] = this->iclp[(x0-x4)>>14];
    blk[8*6] = this->iclp[(x3-x2)>>14];
    blk[8*7] = this->iclp[(x7-x1)>>14];
}

void FIDCT::doFIDCT(int *block) {
    int i;

    for (i=0; i<8; i++)
        this->idctRow(block + 8 * i);

    for (i=0; i<8; i++)
        this->idctCol(block+i);
}

FIDCT::~FIDCT() {
    delete [] this->iclip;
}
