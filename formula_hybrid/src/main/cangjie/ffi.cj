/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */
package formula

foreign {
    func memcpy_s(dest: CPointer<UInt8>, destsz: Int64, src: CPointer<UInt8>, count: Int64): CPointer<UInt8>
    func formula_string_find(dStr:CString, sStr:CString): Int32
    func formula_tag(dStr:CString): CString
    func formula_xingTag(dStr:CString): CString

    /* LaTeX */
    func LaTeX_init(rootDir: CString): Unit
    func LaTeX_release(): Unit
    func LaTeX_parse(ltx: CString, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32): CPointer<UInt8>
    func LaTeX_parse_with_error(ltx: CString, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32, resultCode: CPointer<Int32>, errorMsg: CPointer<UInt8>, errorMsgSize: Int32): CPointer<UInt8>
    func LaTeX_setDebug(debug: Bool): Unit

    /* TeXRender */
    func TeXRender_draw(r: CPointer<UInt8>, bitmap: CPointer<UInt8>, x: Int32, y: Int32, background: UInt32): Unit
    func TeXRender_getTextSize(r: CPointer<UInt8>): Float32
    func TeXRender_getHeight(r: CPointer<UInt8>): Int32
    func TeXRender_getDepth(r: CPointer<UInt8>): Int32
    func TeXRender_getWidth(r: CPointer<UInt8>): Int32
    func TeXRender_getBaseline(r: CPointer<UInt8>): Float32
    func TeXRender_setTextSize(r: CPointer<UInt8>, size: Float32): Unit
    func TeXRender_setForeground(r: CPointer<UInt8>, c: Int32): Unit
    func TeXRender_setWidth(r: CPointer<UInt8>, width: UInt32, align: UInt32): Unit
    func TeXRender_setHeight(r: CPointer<UInt8>, height: UInt32, align: UInt32): Unit
    func TeXRender_finalize(r: CPointer<UInt8>): Unit
    func TeXRender_toBitmap(r: CPointer<UInt8>, bitmap: CPointer<UInt8>, colorFormat: Int32): UInt8Data
    func TeXRender_getMapData(r: CPointer<UInt8>, bitmap: CPointer<UInt8>, colorFormat: Int32): UInt8Data

    /* Graphics2D */
    func initGraphics2D_ffi(x: UInt32, y: UInt32, colorFormat: Int32): CPointer<UInt8>
}

func find_substring(targetString:String, subSringArr:Array<String>): Int32 {
    unsafe {
        var t : CString = LibC.mallocCString(targetString)

        for(i in subSringArr) {
            var s : CString = LibC.mallocCString(i)
            var ret = formula_string_find(t, s)

            if (ret == 1) {
                LibC.free(t)
                LibC.free(s)
                return 1
            } else if (ret == 2) {
                LibC.free(t)
                LibC.free(s)
                return 2
            }
            LibC.free(s)
        }

        LibC.free(t)
        return 0;
    }
}
