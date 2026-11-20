/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */
package formula

let RGB_565: Int32 = 2
let BGRA_8888: Int32 = 5
// 渲染位图单边最大像素数，与 microtex 上游 MAX_RENDER_DIM 钳位值保持一致
let MAX_GRAPHIC_DIM: UInt32 = 8192

public enum ColorFormat {
    | COLOR_FORMAT_RGB_565
    | COLOR_FORMAT_BGRA_8888
}

public class Graphic2D {
    var g2: CPointer<UInt8>
    var h: UInt32
    var w: UInt32
    var cf: Int32

    public prop width: UInt32 {
        get() {
            w
        }
    }

    public prop height: UInt32 {
        get() {
            h
        }
    }

    public init(render: Render, colorFormat: ColorFormat) {
        var w = render.getWidth()
        var h = render.getHeight()
        var textSize = render.getTextSize()
        this.w = w + UInt32(textSize / 9.0) + 1
        this.h = h + UInt32(textSize / 9.0) + 1
        if (this.w > MAX_GRAPHIC_DIM || this.h > MAX_GRAPHIC_DIM) {
            throw Exception("bitmap size ${this.w}x${this.h} exceeds limit ${MAX_GRAPHIC_DIM}")
        }
        this.cf = match (colorFormat) {
            case COLOR_FORMAT_RGB_565 => RGB_565
            case COLOR_FORMAT_BGRA_8888 => BGRA_8888
        }
        g2 = unsafe { initGraphics2D_ffi(this.w, this.h, this.cf) }
        if (g2.isNull()) {
            throw Exception("initGraphics2D_ffi failed to create bitmap ${this.w}x${this.h}")
        }
    }

    public func getG2(): CPointer<UInt8> {
        return g2
    }

    public func getColorFormat(): Int32 {
        return cf
    }

    public func release(): Unit {
        if (g2.isNull()) {
            return
        }
        unsafe { destroyGraphics2D_ffi(g2) }
        g2 = unsafe { CPointer<UInt8>() }
    }

}