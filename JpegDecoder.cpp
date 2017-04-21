//
// Created by maeglin89273 on 4/18/17.
//

#include "Image.h"
#include <iostream>
#include <queue>
#include <iomanip>
#include "JpegDecoder.h"


const int JpegDecoder::ZIG_ZAG_ORDER[] = {
0,   1,   5,  6,   14,  15,  27,  28,
2,   4,   7,  13,  16,  26,  29,  42,
3,   8,  12,  17,  25,  30,  41,  43,
9,   11, 18,  24,  31,  40,  44,  53,
10,  19, 23,  32,  39,  45,  52,  54,
20,  22, 33,  38,  46,  51,  55,  60,
21,  34, 37,  47,  50,  56,  59,  61,
35,  36, 48,  49,  57,  58,  62,  63 };

byte clamp(float value) {
    if (value > 255) {
        return 255;
    }
    if (value < 0) {
        return 0;
    }
    return (byte)(round(value));
}

word readWord(byte* buf, int &index) {
    word byte1 = buf[index++];
    return byte1 << 8 | buf[index++];
}

int toCoefVal(unsigned int val, int n) {
    bool neg = val < (1<<(n-1));
    return !neg? (int)val: -(int)(~val & (((unsigned int)1 << n) - 1));
}

byte readBit(byte*& buf, unsigned int &posInByte) {
    byte bitValue = (buf[0] >> (7 - posInByte)) & ((byte)0x01);
    posInByte++;
    if (posInByte == 8) {
        buf = &(buf[1]);
        posInByte = 0;
    }
    return bitValue;
}

unsigned int grabNBits(byte*& buf, unsigned int &posInByte, int n) {
    unsigned int val = 0;
    for (int i = 0; i < n; i++) {
        val = (val << 1) | readBit(buf, posInByte);
    }
    return val;
}

void toLast0xFF(byte* buf, int &index) {
    while(buf[index] == 0xff) {index++;}
    index--;
}

void splitByte(byte b, int& h4, int& l4) {
    h4 = b >> 4;
    l4 = b & 0x0f;
}

int JpegDecoder::discard0xFF00(byte* buf, byte *&filteredBuf) {
    bool detectEOI = false;
    int newBufSize = 1;
    int oldBufSize = 1;
    word previousByte = buf[0];
    int i = 1;
    while (!detectEOI) {
        if (previousByte == 0xff) {
            word testByte = ((word)0xff00 | buf[i]);
            if (testByte == EOI) {
                detectEOI = true;
            } else if (testByte == 0xff00) {
                newBufSize--;
            }
        }
        newBufSize++;
        oldBufSize++;
        previousByte = buf[i++];
    }

    byte* newBuf = new byte[newBufSize];
    i = 0;
    previousByte = newBuf[i++] = buf[0];
    detectEOI = false;
    int j = 1;
    bool skipCurByte = false;
    while (!detectEOI) {
        if (previousByte == 0xff) {
            word testByte =  ((word)0xff00 | buf[j]);
            if (testByte == EOI) {
                detectEOI = true;
            } else if (testByte == 0xff00) {
                skipCurByte = true;
            }
        }

        if (skipCurByte) {
            skipCurByte = false;
        } else {
            newBuf[i++] = buf[j];
        }
        previousByte = buf[j++];
    }

    filteredBuf = newBuf;
    return oldBufSize - newBufSize;
}


JpegDecoder::~JpegDecoder() {
    delete [] this->colorComps;
    delete [] this->qTables;
    delete [] this->dcHTrees;
    delete [] this->acHTrees;
    delete [] this->qCoefs;
    delete [] this->coefsCopy;
    delete this->fidct;
    this->bgrImage = nullptr;
}

JpegDecoder::JpegDecoder() {
    decodeError = false;
    this->colorComps = nullptr;
    this->qTables = new QTable[4]; // max table number is 4
    this->dcHTrees = new HTree[2];
    this->acHTrees = new HTree[2];
    this->qCoefs = new int[64];
    this->coefsCopy = new int[64];
    this->fidct = new FIDCT;
}

Image* JpegDecoder::decode(unsigned char *buf) {
    int index = 0;

    if (readWord(buf, index) != SOI) {
        std::cout << "it is not a image file" << std::endl;
        return nullptr;
    }

    while(true) {
        if (buf[index] == 0xff) {
            toLast0xFF(buf, index);
            word marker = readWord(buf, index);
            if (marker == EOI) {
                break;
            }
            int infoLen = readWord(buf, index) - WORD_SIZE;
            byte* infoBegin = &buf[index];
            bool success = true;
            switch (marker) {

                case APP0:
                    std::cout << "APP0 maker detected, I'm too lazy to decode it." << std::endl;
                    break;
                case SOF:
                    success = this->decodeSOF(infoBegin, infoLen);
                    break;
                case DQT:
                    success = this->decodeDQT(infoBegin, infoLen);
                    break;
                case DHT:
                    success = this->decodeDHT(infoBegin, infoLen);
                    break;
                case SOS:
                    success = this->decodeSOS(infoBegin, infoLen);
                    if (success) {
                        infoLen += discard0xFF00(infoBegin + infoLen, infoBegin);
                        infoLen += this->decodeData(infoBegin);

                        delete [] infoBegin;
                    }
                    break;

                default:
                    std::cout << "Warning: unsupport marker 0x" << std::hex << std::uppercase << (word)(marker) << std::endl;
                    break;
            }
            if (!success) {
                std::cout << "encounter errors during decoding QQ" << std::endl;
                return nullptr;
            }
            index += infoLen;

        } else {
            std::cout << "byte 0x" << std::hex << std::uppercase << (word)(buf[index]) << " detected, not a jpeg format!" << std::endl;
            return nullptr;
        }
    }

    std::cout << "uncompress jpeg successfully" << std::endl;
    return this->bgrImage;
}

bool JpegDecoder::decodeSOF(byte *buf, int len) {
    int index = 0;
    this->samplePrecision = buf[index++];
    this->imgHeight = readWord(buf, index);
    this->imgWidth = readWord(buf, index);
    this->colorCompNum = buf[index++];

    if (this->colorComps != nullptr) {
        delete [] this->colorComps; // avoid duplicate markers
    }

    this->colorComps = new ColorComponentInfo[this->colorCompNum + 1]; //1 based

    for (int i = 0; i < this->colorCompNum; i++) {
        int id = buf[index++];
        ColorComponentInfo* comp = &(this->colorComps[id]);
        comp->id = id;
        byte sf = buf[index++];
        splitByte(sf, comp->hSamplingFactor, comp->vSamplingFactor);
        comp->qTable = &(this->qTables[buf[index++]]);
    }

    return len == index;
}

bool JpegDecoder::decodeDQT(byte *buf, int len) {
    int index = 0;

    while (index < len) {
        int precision, id;
        splitByte(buf[index++], precision, id);
        QTable* qTable = &(this->qTables[id]);
        qTable->precision = precision;
        unsigned int tableSize = (unsigned int) (64 * (precision + 1));
        if (qTable->tableData != nullptr) {
            delete [] qTable->tableData; // avoid duplicate markers
        }
        qTable->tableData = new byte[tableSize];
        memcpy(qTable->tableData, &(buf[index]), tableSize);
        index += tableSize;
    }

    return len == index;
}

bool JpegDecoder::decodeDHT(byte *buf, int len) {
    int index = 0;
    byte* lenNums = new byte[HTree::MAX_CODE_LEN];
    byte* values = nullptr;
    while (index < len) {
        int dac, id;
        splitByte(buf[index++], dac, id);
        HTree* hTree = dac == 0? &(dcHTrees[id]): &(acHTrees[id]);

        unsigned int totalCodeNum = 0;
        for (int i = 0; i < HTree::MAX_CODE_LEN; i++) {
            lenNums[i] = buf[index++];
            totalCodeNum += lenNums[i];
        }

        if (values != nullptr) {
            delete [] values;
        }
        values = new byte[totalCodeNum];
        memcpy(values, &(buf[index]), totalCodeNum);
        index += totalCodeNum;
        hTree->buildTree(lenNums, values);
    }
    delete [] lenNums;
    return len == index;
}

bool JpegDecoder::decodeSOS(byte *buf, int len) {
    int index = 0;
    if (buf[index++] != this->colorCompNum) {
        std::cout << "color component number is inconsistant! " << std::endl;
        return false;
    }

    for (int i = 0; i < this->colorCompNum; i++) {
        byte colorId = buf[index++];
        int dcTreeId, acTreeId;
        splitByte(buf[index++], dcTreeId, acTreeId);
        ColorComponentInfo* colorComp = &(this->colorComps[colorId]);
        colorComp->dcHTree = &(this->dcHTrees[dcTreeId]);
        colorComp->acHTree = &(this->acHTrees[acTreeId]);
    }

    index += 3; // skip Spectral selection and approximation info

    return len == index;
}

int JpegDecoder::decodeData(byte *buf) {
    byte* bufStart = buf;
    int maxH = this->colorComps[Y].hSamplingFactor;
    int maxV = this->colorComps[Y].vSamplingFactor;
    int mcuH = maxH * 8;
    int mcuV = maxV * 8;

    int fillMcuV = this->imgHeight % mcuV == 0? 0: (mcuV - this->imgHeight % mcuV);
    int fillMcuH = this->imgWidth % mcuH == 0? 0: (mcuH - this->imgWidth % mcuH);
    int extendedH = this->imgHeight + fillMcuV;
    int extendedW = this->imgWidth + fillMcuH;


    Image *yccImage = new Image(extendedH, extendedW);
    this->posInByte = 0;
    for (int y = 0; y < extendedH; y += mcuV) {
        for (int x = 0; x < extendedW; x += mcuH) {
            byte * output = &(yccImage->getData()[x * 3 + y * 3 * extendedW]);
            this->decodeMCU(buf, maxV, maxH, output, extendedW);
        }
    }

    this->bgrImage = this->convertAndCropToBGRImage(yccImage, this->imgHeight, this->imgWidth);
    delete yccImage;

    skipRemaining1s(buf);

    return (int) (buf - bufStart);
}

void JpegDecoder::skipRemaining1s(byte *&buf) {
    int remainingBitNum = 8 - this->posInByte;
    for (int i = 0; i < remainingBitNum; i++) {
        if (!readBit(buf, this->posInByte)) {
            std::cout << "encounter 0 in end padding. the decoding process may break" << std::endl;
        }
    }
}

void JpegDecoder::decodeMCU(byte *&buf, int mcuH, int mcuW, byte *output, int imgWidth) {

    ColorComponentInfo *ccY = &(this->colorComps[Y]);
    Block *oBlock = new Block(output, 8, 8, 0, imgWidth);
    for (int y = 0; y < 8 * mcuH; y += 8) {
        for (int x = 0; x < 8 * mcuW; x += 8) {
            byte *ptr = output + x * 3 + y * 3 * imgWidth;
            oBlock->setSuperBlockPtr(ptr);
            this->decodeBlock(buf, ccY, oBlock);
        }
    }
    delete oBlock;

    oBlock = new Block(output, 8, 8, 1, imgWidth, mcuH, mcuW);
    this->decodeBlock(buf, &(this->colorComps[Cb]), oBlock);
    oBlock->setCIndex(2);
    this->decodeBlock(buf, &(this->colorComps[Cr]), oBlock);
    delete oBlock;

}

void JpegDecoder::decodeBlock(byte *&buf, ColorComponentInfo *colorComp, Block *output) {
    int *coefs = decodeBlockToQuantizedCoefs(buf, colorComp);
    this->dequantizeBlock(coefs, colorComp);
    this->deZigZag(coefs);
    this->inverseDCT(coefs, output);

}

int * JpegDecoder::decodeBlockToQuantizedCoefs(byte *&buf, ColorComponentInfo *colorComp) {


    int qCoef = 0;
    memset(this->qCoefs, 0, 64 * sizeof(int));

    int grabLen = colorComp->dcHTree->decode(buf, this->posInByte, this->decodeError);

    if (this->decodeError) {
        return nullptr;
    }

    if (grabLen > 0) {
        unsigned int val = grabNBits(buf, this->posInByte, grabLen);
        qCoef = toCoefVal(val, grabLen);
    }
    colorComp->prevDC = this->qCoefs[0] = colorComp->prevDC + qCoef;



    bool eob = false;
    int coefI = 1;
    while(!eob && coefI < 64) {

        byte rle = colorComp->acHTree->decode(buf, this->posInByte, this->decodeError);

        if (this->decodeError) {
            return nullptr;
        }
        int run;
        splitByte(rle, run, grabLen);

        if (rle == 0x00) {
            eob = true;
        } else {

            coefI += run;
            qCoef = 0;
            if (grabLen > 0) {
                unsigned int val = grabNBits(buf, posInByte, grabLen);
                qCoef = toCoefVal(val, grabLen);
            }

            this->qCoefs[coefI++] = qCoef;
        }
    }

    return this->qCoefs;
}

void JpegDecoder::dequantizeBlock(int *block, ColorComponentInfo *colorComp) {
    QTable* qTable = colorComp->qTable;
    for (int i = 0; i < 64; i++) {
        block[i] =  block[i] * (int)qTable->tableData[i];
    }
}

void JpegDecoder::deZigZag(int *coefs) {

    memcpy(this->coefsCopy, coefs, 64 * sizeof(int));
    for (int i = 0; i < 64; i++) {
        coefs[i] = this->coefsCopy[ZIG_ZAG_ORDER[i]];
    }
}

void JpegDecoder::inverseDCT(int *coefs, JpegDecoder::Block *output) {
    this->fidct->doFIDCT(coefs);
    for (int i = 0; i < 64; i++) {
        output->set(i, clamp(coefs[i] + 128));
    }
}

Image *JpegDecoder::convertAndCropToBGRImage(Image *yCbCrImage, int height, int width) {
    Block *inputY = new Block(yCbCrImage->getData(), yCbCrImage->getHeight(), yCbCrImage->getWidth(), 0, yCbCrImage->getWidth());
    Block *inputCb = new Block(yCbCrImage->getData(), yCbCrImage->getHeight(), yCbCrImage->getWidth(), 1, yCbCrImage->getWidth());
    Block *inputCr = new Block(yCbCrImage->getData(), yCbCrImage->getHeight(), yCbCrImage->getWidth(), 2, yCbCrImage->getWidth());

    Image *bgrImage = new Image(height, width);
    Block *outputB = new Block(bgrImage->getData(), height, width, 0, width);
    Block *outputG = new Block(bgrImage->getData(), height, width, 1, width);
    Block *outputR = new Block(bgrImage->getData(), height, width, 2, width);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float yc = inputY->get(x, y);
            float cb = inputCb->get(x, y);
            float cr = inputCr->get(x, y);
            outputB->set(x, y, clamp(yc + 1.772f * (cb - 128)));
            outputG->set(x, y, clamp(yc - 0.34414f * (cb - 128) - 0.71414f * (cr - 128)));
            outputR->set(x, y, clamp(yc + 1.402f * (cr - 128)));
        }
    }

    delete inputY;
    delete inputCb;
    delete inputCr;
    delete outputB;
    delete outputG;
    delete outputR;

    return bgrImage;
}

void JpegDecoder::HTree::buildTree(byte *lenNums, byte *values) {
    if (this->root != nullptr) {
        delete this->root;
    }

    this->root = new HNode;
    HNode* node = this->root;

    std::queue<HNode*> curLevelNodes;
    std::queue<HNode*> nextLevelNodes;

    node->node0 = new HNode;
    node->node1 = new HNode;
    curLevelNodes.push(node->node0);
    curLevelNodes.push(node->node1);

    int k = 0;
    for (int i = 0; i < MAX_CODE_LEN; i++) {
        for (int j = 0; j < lenNums[i]; j++) { // construct leaves
            node = curLevelNodes.front();
            curLevelNodes.pop();
            node->value = values[k++];
        }
        while (!curLevelNodes.empty()) { // expand next level by remaining nodes
            node = curLevelNodes.front();
            curLevelNodes.pop();
            node->node0 = new HNode;
            node->node1 = new HNode;
            nextLevelNodes.push(node->node0);
            nextLevelNodes.push(node->node1);
        }
        curLevelNodes.swap(nextLevelNodes);
    }
}

byte JpegDecoder::HTree::decode(byte*& buf, unsigned int &posInByte, bool& error) {
    HNode* curNode = this->root;
    for (int i = 0; i <= MAX_CODE_LEN; i++) {
        if (curNode->isLeaf()) {
            return curNode->value;
        }
        byte bit = readBit(buf, posInByte);
        curNode = bit? curNode->node1: curNode->node0;
    }

    std::cout << "Huffman decode error!" << std::endl;
    error = true;
    return (byte) -1;
}



JpegDecoder::Block::Block(byte *superBlockPtr, int height, int width, int cIdx, int superBlockWidth, int upScaleY, int upScaleX) {
    this->ptr = superBlockPtr + cIdx;
    this->cIdx = cIdx;
    this->width = width;
    this->height = height;
    this->sbWidth = 3 * superBlockWidth;
    this->sX = upScaleX;
    this->sY = upScaleY;
    this->trueHeight = upScaleY * this->height;
    this->trueWidth = upScaleX * this->width;
}

JpegDecoder::Block::~Block() {
    this->ptr = nullptr;
}

void JpegDecoder::Block::set(int x, int y, byte value) {

    for (int ty = sY * y; ty < sY * y + sY; ty++) {
        for (int tx = sX * x; tx < sX * x + sX; tx++) {
            this->ptr[tx * 3 + ty * this->sbWidth] = value;
        }
    }
}

byte JpegDecoder::Block::get(int x, int y) {
    x = sX * x;
    y = sY * y;
    return this->ptr[x * 3 + y * this->sbWidth];
}

int JpegDecoder::Block::getWidth() {
    return this->width;
}

int JpegDecoder::Block::getHeight() {
    return this->height;
}

void JpegDecoder::Block::set(int i, byte value) {
    int y = i / this->width;
    int x = i % this->width;
    this->set(x, y, value);
}

byte JpegDecoder::Block::get(int i) {
    int y = i / this->width;
    int x = i % this->width;
    return get(x, y);
}

void JpegDecoder::Block::setSuperBlockPtr(byte *superBlockPtr) {
    this->ptr = superBlockPtr + this->cIdx;
}

void JpegDecoder::Block::setCIndex(int cIdx) {
    this->ptr = this->ptr - this->cIdx + cIdx;
    this->cIdx = cIdx;
}
