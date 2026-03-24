/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */

package formula

@C
struct UInt8Data {
    var data: CPointer<UInt8> = CPointer<UInt8>()
    var len: Int64 = 0
}

public class Render {
    var nativePtr: CPointer<UInt8>

    public init(ptr: CPointer<UInt8>) {
        nativePtr = ptr
    }

    public func draw(g2: Graphic2D, background: UInt32): Unit {
        unsafe { TeXRender_draw(nativePtr, g2.getG2(), 0, 0, background) }
    }

    public func getTextSize(): Float32 {
        return unsafe { TeXRender_getTextSize(nativePtr) }
    }

    public func getHeight(): UInt32 {
        return unsafe { UInt32(TeXRender_getHeight(nativePtr)) }
    }

    public func getWidth(): UInt32 {
        return unsafe { UInt32(TeXRender_getWidth(nativePtr)) }
    }

    public func finalize(): Unit {
        return unsafe { TeXRender_finalize(nativePtr) }
    }

    public func toBitmap(g2: Graphic2D): Array<UInt8> {
        unsafe {
            var bitmapData =  TeXRender_toBitmap(nativePtr, g2.getG2(), g2.getColorFormat())
            var array = Array<UInt8>(bitmapData.len, repeat: 0)
            var cph = acquireArrayRawData(array)
            memcpy_s(cph.pointer, bitmapData.len, bitmapData.data, bitmapData.len)
            releaseArrayRawData(cph)
            LibC.free(bitmapData.data)
            return array
        }
    }

    public func getMapData(g2: Graphic2D): Array<UInt8> {
        if (g2.getColorFormat() == RGB_565) {
            throw Exception("COLOR_FORMAT_RGB_565 format not supported!")
        }
        unsafe {
            var bitmapData =  TeXRender_getMapData(nativePtr, g2.getG2(), g2.getColorFormat())
            var array = Array<UInt8>(bitmapData.len, repeat: 0)
            var cph = acquireArrayRawData(array)
            memcpy_s(cph.pointer, bitmapData.len, bitmapData.data, bitmapData.len)
            releaseArrayRawData(cph)
            LibC.free(bitmapData.data)
            return array
        }
    }
}