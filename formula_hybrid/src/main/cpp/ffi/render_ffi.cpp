/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "config.h"

#ifdef __OS_ohos__

#include "graphic_ohos.h"
#include "latex.h"
#include "utils.h"

using namespace tex;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    int64_t len;
} UInt8Data;

void TeXRender_draw(TeXRender *r, OH_Drawing_Bitmap *bitmap, int x, int y, uint32_t background) {
    Graphics2D_ohos g2(bitmap, background);
    r->draw(g2, x, y);
}

float TeXRender_getTextSize(TeXRender *r) {
    return r->getTextSize();
}

int TeXRender_getHeight(TeXRender *r) {
    return r->getHeight();
}

int TeXRender_getDepth(TeXRender *r) {
    return r->getDepth();
}

int TeXRender_getWidth(TeXRender *r) {
    return r->getWidth();
}

float TeXRender_getBaseline(TeXRender *r) {
    return r->getBaseline();
}

void TeXRender_setTextSize(TeXRender *r, float size) {
    r->setTextSize(size);
}

void TeXRender_setForeground(TeXRender *r, int c) {
    r->setForeground((color)c);
}

void TeXRender_setWidth(TeXRender *r, int width, int align) {
    r->setWidth(width, align);
}

void TeXRender_setHeight(TeXRender *r, int height, int align) {
    r->setHeight(height, align);
}

void TeXRender_finalize(TeXRender *r) {
    delete r;
}

static UInt8Data toBitmapRGB_565(TeXRender *r, OH_Drawing_Bitmap *bitmap) {
    BitMapFileHeader bmfHdr; // 定义文件头
    BitMapInfoHeader bmiHdr; // 定义信息头
    RgbQuad bmiClr[3]; // 定义调色板

    uint32_t w = OH_Drawing_BitmapGetWidth(bitmap);
    uint32_t h = OH_Drawing_BitmapGetHeight(bitmap);
    uint8_t *bitmapAddr = (uint8_t *)OH_Drawing_BitmapGetPixels(bitmap);

    bmiHdr.biSize = sizeof(BitMapInfoHeader);
    bmiHdr.biWidth = w; // 指定图像的宽度，单位是像素
    bmiHdr.biHeight = h; // 指定图像的高度，单位是像素
    bmiHdr.biPlanes = 1; // 目标设备的级别，必须是1
    bmiHdr.biBitCount = 16; // 表示用到颜色时用到的位数 16位表示高彩色图
    bmiHdr.biCompression = BI_BITFIELDS; // RGB565格式
    bmiHdr.biSizeImage = (w + w % 2) * h * 2; // 指定实际位图所占字节数
    bmiHdr.biXPelsPerMeter = 0; // 水平分辨率，单位长度内的像素数
    bmiHdr.biYPelsPerMeter = 0; // 垂直分辨率，单位长度内的像素数
    bmiHdr.biClrUsed = 0; // 位图实际使用的彩色表中的颜色索引数（设为0的话，则说明使用所有调色板项）
    bmiHdr.biClrImportant = 0; // 说明对图象显示有重要影响的颜色索引的数目，0表示所有颜色都重要

    // RGB565格式掩码
    bmiClr[0].rgbBlue = 0;
    bmiClr[0].rgbGreen = 0xF8;
    bmiClr[0].rgbRed = 0;
    bmiClr[0].rgbReserved = 0;

    bmiClr[1].rgbBlue = 0xE0;
    bmiClr[1].rgbGreen = 0x07;
    bmiClr[1].rgbRed = 0;
    bmiClr[1].rgbReserved = 0;

    bmiClr[2].rgbBlue = 0x1F;
    bmiClr[2].rgbGreen = 0;
    bmiClr[2].rgbRed = 0;
    bmiClr[2].rgbReserved = 0;

    bmfHdr.bfType = (WORD)0x4D42; // 文件类型，0x4D42也就是字符'BM'
    bmfHdr.bfSize = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + sizeof(bmiClr) + bmiHdr.biSizeImage); // 文件大小
    bmfHdr.bfReserved1 = 0; // 保留，必须为0
    bmfHdr.bfReserved2 = 0; // 保留，必须为0
    bmfHdr.bfOffBits = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader)+ sizeof(bmiClr)); // 实际图像数据偏移量

    uint8_t *data = (uint8_t *)malloc(bmfHdr.bfSize);
    memset_s(data, bmfHdr.bfSize, 0, bmfHdr.bfOffBits);
    uint8_t *start = data;
    int32_t destsz = bmfHdr.bfSize;
    memcpy_s(start, destsz, &bmfHdr, sizeof(BitMapFileHeader));
    start += sizeof(BitMapFileHeader);
    destsz -= sizeof(BitMapFileHeader);
    memcpy_s(start, destsz, &bmiHdr, sizeof(BitMapInfoHeader));
    start += sizeof(BitMapInfoHeader);
    destsz -= sizeof(BitMapInfoHeader);
    memcpy_s(start, destsz, &bmiClr, sizeof(bmiClr));
    start += sizeof(bmiClr);
    destsz -= sizeof(bmiClr);
    for (int i = 0; i < h; i++) {
        memcpy_s(start, destsz, bitmapAddr + (w * (h - i - 1) * 2), w * 2);
        start += (w + w % 2) * 2;
        destsz -= (w + w % 2) * 2;
    }

    UInt8Data u8data;
    u8data.data = data;
    u8data.len = bmfHdr.bfSize;
    return u8data;
}


// 手动定义必要的结构体
typedef struct {
    LONG fx;
    LONG fy;
    LONG fz;
} CIEXYZ1;

typedef struct {
    CIEXYZ1 ciexyzRed;
    CIEXYZ1 ciexyzGreen;
    CIEXYZ1 ciexyzBlue;
} CIEXYZTRIPLE1;

typedef struct {
    DWORD bV4Size;
    LONG  bV4Width;
    LONG  bV4Height;
    WORD  bV4Planes;
    WORD  bV4BitCount;
    DWORD bV4Compression;
    DWORD bV4SizeImage;
    LONG  bV4XPelsPerMeter;
    LONG  bV4YPelsPerMeter;
    DWORD bV4ClrUsed;
    DWORD bV4ClrImportant;
    DWORD bV4RedMask;
    DWORD bV4GreenMask;
    DWORD bV4BlueMask;
    DWORD bV4AlphaMask;
    DWORD bV4CSType;
    CIEXYZTRIPLE1 bV4Endpoints;
    DWORD bV4GammaRed;
    DWORD bV4GammaGreen;
    DWORD bV4GammaBlue;
} BitMapV4Header1;

static UInt8Data toBitmapBGRA_8888(TeXRender *r, OH_Drawing_Bitmap *bitmap) {
    BitMapFileHeader bmfHdr;
    BitMapV4Header1 bmiHdr;

    uint32_t w = OH_Drawing_BitmapGetWidth(bitmap);
    uint32_t h = OH_Drawing_BitmapGetHeight(bitmap);
    uint8_t *bitmapAddr = (uint8_t *)OH_Drawing_BitmapGetPixels(bitmap);

    // 初始化BITMAPV4HEADER
    memset(&bmiHdr, 0, sizeof(BitMapV4Header1));
    bmiHdr.bV4Size = sizeof(BitMapV4Header1);
    bmiHdr.bV4Width = w;
    bmiHdr.bV4Height = h;
    bmiHdr.bV4Planes = 1;
    bmiHdr.bV4BitCount = 32;
    bmiHdr.bV4Compression = BI_BITFIELDS; // 使用BITFIELDS表示有自定义颜色掩码
    bmiHdr.bV4SizeImage = w * h * 4;
    bmiHdr.bV4XPelsPerMeter = 0;
    bmiHdr.bV4YPelsPerMeter = 0;
    bmiHdr.bV4ClrUsed = 0;
    bmiHdr.bV4ClrImportant = 0;
    
    // 设置颜色掩码 - BGRA顺序
    bmiHdr.bV4RedMask = 0x00FF0000;   // 红色掩码
    bmiHdr.bV4GreenMask = 0x0000FF00; // 绿色掩码
    bmiHdr.bV4BlueMask = 0x000000FF;  // 蓝色掩码
    bmiHdr.bV4AlphaMask = 0xFF000000; // Alpha通道掩码
    
    // 设置颜色空间为sRGB
    bmiHdr.bV4CSType = 0x73524742; // 'sRGB'

    // 设置文件头
    bmfHdr.bfType = (WORD)0x4D42; // "BM"
    bmfHdr.bfSize = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapV4Header1) + bmiHdr.bV4SizeImage);
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)(sizeof(BitMapFileHeader) + sizeof(BitMapV4Header1));

    // 分配内存
    uint8_t *data = (uint8_t *)malloc(bmfHdr.bfSize);
    if (!data) {
        UInt8Data empty = {NULL, 0};
        return empty;
    }
    
    uint8_t *start = data;
    int32_t destsz = bmfHdr.bfSize;
    
    // 复制文件头
    memcpy_s(start, destsz, &bmfHdr, sizeof(BitMapFileHeader));
    start += sizeof(BitMapFileHeader);
    destsz -= sizeof(BitMapFileHeader);
    
    // 复制信息头
    memcpy_s(start, destsz, &bmiHdr, sizeof(BitMapV4Header1));
    start += sizeof(BitMapV4Header1);
    destsz -= sizeof(BitMapV4Header1);
    
    // 复制像素数据（注意BMP是从下到上存储的）
    for (int i = 0; i < h; i++) {
        // 从源位图的最后一行开始复制（BMP是从下到上存储的）
        uint8_t *srcLine = bitmapAddr + (w * (h - i - 1) * 4);
        memcpy_s(start, destsz, srcLine, w * 4);
        start += w * 4;
        destsz -= w * 4;
    }
    
    UInt8Data u8data;
    u8data.data = data;
    u8data.len = bmfHdr.bfSize;
    return u8data;
}

UInt8Data TeXRender_toBitmap(TeXRender *r, OH_Drawing_Bitmap *bitmap, OH_Drawing_ColorFormat colorFormat) {
    if (colorFormat == COLOR_FORMAT_RGB_565) {
        return toBitmapRGB_565(r, bitmap);
    } else if (colorFormat == COLOR_FORMAT_BGRA_8888) {
        return toBitmapBGRA_8888(r, bitmap);
    }
}

UInt8Data TeXRender_getMapData(TeXRender *r, OH_Drawing_Bitmap *bitmap, OH_Drawing_ColorFormat colorFormat) {
    if (colorFormat == COLOR_FORMAT_RGB_565) {
        return toBitmapRGB_565(r, bitmap);
    } else if (colorFormat == COLOR_FORMAT_BGRA_8888) {
        uint32_t w = OH_Drawing_BitmapGetWidth(bitmap);
        uint32_t h = OH_Drawing_BitmapGetHeight(bitmap);
        int size = h * w * 4;
        uint8_t *bitmapAddr = (uint8_t *)OH_Drawing_BitmapGetPixels(bitmap);
        UInt8Data u8data;
        u8data.data = bitmapAddr;
        u8data.len = size;
        return u8data;
    }
}

#ifdef __cplusplus
}
#endif

#endif  // __OS_ohos__