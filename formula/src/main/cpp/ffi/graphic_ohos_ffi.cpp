/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 */

#include "config.h"

#ifdef __OS_ohos__

#include <native_drawing/drawing_bitmap.h>
#include <iostream>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

OH_Drawing_Bitmap *initGraphics2D_ffi(uint32_t w, uint32_t h, OH_Drawing_ColorFormat colorFormat) {
    if (w == 0 || h == 0) {
        return nullptr;
    }
    OH_Drawing_Bitmap *bitmap = OH_Drawing_BitmapCreate();
    if (bitmap == nullptr) {
        return nullptr;
    }
    // 定义bitmap的像素格式
    OH_Drawing_BitmapFormat cFormat{colorFormat, ALPHA_FORMAT_OPAQUE};
    // 构造对应格式的bitmap
    OH_Drawing_BitmapBuild(bitmap, w, h, &cFormat);
    // BitmapBuild 无错误码返回，构造失败时像素指针为空，据此判空兜底
    if (OH_Drawing_BitmapGetPixels(bitmap) == nullptr) {
        OH_Drawing_BitmapDestroy(bitmap);
        return nullptr;
    }
    return bitmap;
}

void destroyGraphics2D_ffi(OH_Drawing_Bitmap *bitmap) 
{
    OH_Drawing_BitmapDestroy(bitmap);
}

#ifdef __cplusplus
}
#endif

#endif  // __OS_ohos__
