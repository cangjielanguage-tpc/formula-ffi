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
    OH_Drawing_Bitmap *bitmap = OH_Drawing_BitmapCreate();
    // 定义bitmap的像素格式
    OH_Drawing_BitmapFormat cFormat{colorFormat, ALPHA_FORMAT_OPAQUE};
    // 构造对应格式的bitmap
    OH_Drawing_BitmapBuild(bitmap, w, h, &cFormat);
    return bitmap;
}

#ifdef __cplusplus
}
#endif

#endif  // __OS_ohos__
