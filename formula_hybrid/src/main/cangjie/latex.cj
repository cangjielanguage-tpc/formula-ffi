package formula

import formula.mhchem.Mhchem

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
            var processedLtx = ltx
            if (ltx.contains("\\ce{") || ltx.contains("\\pu{")) {
                processedLtx = Mhchem().process(ltx)
            }
            var cstr = LibC.mallocCString(processedLtx)
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