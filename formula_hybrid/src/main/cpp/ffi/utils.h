/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#ifndef UTIL_H
#define UTIL_H

#include <cstring>
#include <stdexcept>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

static inline void memcpy_s(void *dest, uint32_t dest_size, const void *src, uint32_t src_size)
{
    if (dest_size < src_size) {
        throw std::runtime_error("Insufficient space in the target buffer.");
    }
    std::memcpy(dest, src, src_size);
}

static inline void memset_s(void *dest, uint32_t destSize, int value, uint32_t count)
{
    if (count > destSize) {
        throw std::runtime_error("The number of bytes to be set exceeds the buffer size.");
    }
    std::memset(dest, value, count);
}

#define BI_RGB 0x0
#define BI_BITFIELDS 0x3

typedef char BYTE;
typedef short WORD;
typedef int DWORD;
typedef int LONG;

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;                            // 位图文件的类型, 在Windows中, 此字段的值总为‘BM’(1-2字节)
    DWORD bfSize;                           // 位图文件的大小, 以字节为单位(3-6字节, 低位在前)
    WORD bfReserved1;                       // 位图文件保留字, 必须为0(7-8字节)
    WORD bfReserved2;                       // 位图文件保留字, 必须为0(9-10字节)
    DWORD bfOffBits;                        // 位图数据的起始位置, 以相对于位图(11-14字节, 低位在前)
} __attribute__((packed)) BitMapFileHeader; // BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;                           // 本结构所占用字节数(15-18字节)
    LONG biWidth;                           // 位图的宽度, 以像素为单位(19-22字节)
    LONG biHeight;                          // 位图的高度, 以像素为单位(23-26字节)
    WORD biPlanes;                          // 目标设备的级别, 必须为1(27-28字节)
    WORD biBitCount;                        // 每个像素所需的位数, 1(双色), 4(16色), 8(256色), 16(高彩色), 24(真彩色),
                                            // 32位图(增强真彩色), (29-30字节)
    DWORD biCompression;                    // 位图压缩类型, 0(BI_RGB不压缩), 1(BI_RLE8压缩类型), 2(BI_RLE4压缩类型),
                                            // 3(BI_BITFIELDS), 4(BI_JPEG), 5(BI_PNG), (31-34字节)
    DWORD biSizeImage;                      // 位图的大小(其中包含了为了补齐行数是4的倍数而添加的空字节), 以字节为单位(35-38字节)
    LONG biXPelsPerMeter;                   // 位图水平分辨率, 像素数(39-42字节)
    LONG biYPelsPerMeter;                   // 位图垂直分辨率, 像素数(43-46字节)
    DWORD biClrUsed;                        // 位图实际使用的颜色表中的颜色数(47-50字节)
    DWORD biClrImportant;                   // 位图显示过程中重要的颜色数(51-54字节)
} __attribute__((packed)) BitMapInfoHeader; // BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;                     // 蓝色的亮度(值范围为0-255)
    BYTE rgbGreen;                    // 绿色的亮度(值范围为0-255)
    BYTE rgbRed;                      // 红色的亮度(值范围为0-255)
    BYTE rgbReserved;                 // 保留, 必须为0
} __attribute__((packed)) RgbQuad;    //RGBQUAD;

#ifdef __cplusplus
}
#endif

#endif