/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.
 */

package formula_markdown

public class LaTeX {
    public init(rootDir: String) {
        unsafe {
            var cstr =  LibC.mallocCString(rootDir)
            LaTeX_init(cstr)
            LibC.free(cstr)
        }
    }

    public func release(): Unit {
        unsafe { LaTeX_release() }
    }

    public func parse(ltx: String, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32): Render {
        unsafe {
            var cstr =  LibC.mallocCString(ltx)
            var ptr = LaTeX_parse(cstr, width, textSize, lineSpace, foreground)
            if (ptr.isNull()) {
                LibC.free(cstr)
                throw Exception("parse failed, CPointer is null!")
            }
            LibC.free(cstr)
            Render(ptr)
        }
    }
}