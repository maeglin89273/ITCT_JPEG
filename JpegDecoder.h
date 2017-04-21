//
// Created by maeglin89273 on 4/18/17.
//

#ifndef ITCT_JPEG_JPEGDECODER_H
#define ITCT_JPEG_JPEGDECODER_H

#include "FIDCT.h"

typedef unsigned short word;
typedef unsigned char byte;

class JpegDecoder {
public:
    JpegDecoder();
    Image* decode(unsigned char *buf);
    ~JpegDecoder();

private:
    class QTable {
    public:
        int precision;
        byte* tableData;
        QTable() {
            this->tableData = nullptr;
        }
        ~QTable() {
            delete [] tableData;
        }
    };

    class HTree {
    private:
        class HNode {
        public:
            HNode* node0;
            HNode* node1;
            byte value;
            HNode() {
                this->node0 = nullptr;
                this->node1 = nullptr;
            }
            bool isLeaf() {
                return node0 == nullptr && node1 == nullptr;
            }
            ~HNode() {
                delete node0;
                delete node1;
            }
        };

    public:
        const static unsigned int MAX_CODE_LEN = 16;
        HNode* root;
        HTree() {
            this->root = nullptr;
        }

        void buildTree(byte* lenNums, byte* values);
        ~HTree() {
            delete root;
        }

        byte decode(byte *&buf, unsigned int &posInByte, bool& error);
    };


    class ColorComponentInfo {
    public:
        int id;
        int hSamplingFactor;
        int vSamplingFactor;
        QTable* qTable;
        HTree* dcHTree;
        HTree* acHTree;
        int prevDC;
        ColorComponentInfo() {
            prevDC = 0;
            this->qTable = nullptr;
            this->dcHTree = nullptr;
            this->acHTree = nullptr;
            //we don't delete them in destructor,
            // since they are just references, so they should be managed by decoder.
        }

    };

    class Block {
    private:
        byte* ptr;
        int height;
        int width;
        int trueHeight;
        int trueWidth;
        int cIdx;
        int sbWidth;
        int sX;
        int sY;
    public:
        Block(byte *superBlockPtr, int height, int width, int cIdx, int superBlockWidth, int upScaleY=1, int upSscaleX=1);
        ~Block();
        void set(int x, int y, byte value);
        void set(int i, byte value);
        byte get(int x, int y);
        byte get(int i);
        int getWidth();
        int getHeight();

        void setSuperBlockPtr(byte *superBlockPtr);

        void setCIndex(int cIdx);
    };

    const static int ZIG_ZAG_ORDER[];

    const static word SOI = 0xFFD8;
    const static word APP0 = 0xFFE0;
    const static word SOF = 0xFFC0;
    const static word DQT = 0xFFDB;
    const static word DHT = 0xFFC4;
    const static word SOS = 0xFFDA;
    const static word EOI = 0xFFD9;
    const static word WORD_SIZE = sizeof(word);

    byte samplePrecision;
    int imgHeight;
    int imgWidth;
    int colorCompNum;

    const static int Y = 1;
    const static int Cb = 2;
    const static int Cr = 3;
    ColorComponentInfo* colorComps;

    QTable* qTables;
    HTree* dcHTrees;
    HTree* acHTrees;
    Image* bgrImage;

    bool decodeError;
    unsigned int posInByte;

    int* qCoefs;
    int* coefsCopy;
    FIDCT* fidct;

    bool decodeSOF(byte *buf, int len);
    bool decodeDQT(byte *buf, int len);
    bool decodeDHT(byte *buf, int len);
    bool decodeSOS(byte *buf, int len);

    int decodeData(byte *buf);

    int discard0xFF00(byte* buf, byte *&filteredBuf);

    void dequantizeBlock(int *block, ColorComponentInfo *colorComp);
    int * decodeBlockToQuantizedCoefs(byte *&buf, ColorComponentInfo *colorComp);

    void decodeMCU(byte *&buf, int mcuH, int mcuW, byte *output, int imgWidth);
    void decodeBlock(byte *&buf, ColorComponentInfo *colorComp, Block *output);

    void deZigZag(int *coefs);

    void inverseDCT(int *coefs, Block *output);

    Image *convertAndCropToBGRImage(Image *yCbCrImage, int height, int width);

    void skipRemaining1s(byte *&buf);
};


#endif //ITCT_JPEG_JPEGDECODER_H
