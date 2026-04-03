package formula
import formula.mhchem.Mhchem
import ohos.hilog.Hilog

public enum TeXResultCode <: ToString & Equatable<TeXResultCode> {
    | Success
    | SyntaxError
    | InvalidMatrixError
    | InvalidDelimiterError
    | TeXError
    | UnknownError

    public operator func ==(other: TeXResultCode): Bool {
        this.toInt() == other.toInt()
    }

    public func toString(): String {
        match (this) {
            case TeXResultCode.Success => "Success"
            case TeXResultCode.SyntaxError => "SyntaxError"
            case TeXResultCode.InvalidMatrixError => "InvalidMatrixError"
            case TeXResultCode.InvalidDelimiterError => "InvalidDelimiterError"
            case TeXResultCode.TeXError => "TeXError"
            case TeXResultCode.UnknownError => "UnknownError"
        }
    }

    public func toInt(): Int32 {
        match (this) {
            case TeXResultCode.Success => 0
            case TeXResultCode.SyntaxError => 1
            case TeXResultCode.InvalidMatrixError => 2
            case TeXResultCode.InvalidDelimiterError => 3
            case TeXResultCode.TeXError => 4
            case TeXResultCode.UnknownError => 99
        }
    }

    public static func fromInt(code: Int32): TeXResultCode {
        match (code) {
            case 0 => TeXResultCode.Success
            case 1 => TeXResultCode.SyntaxError
            case 2 => TeXResultCode.InvalidMatrixError
            case 3 => TeXResultCode.InvalidDelimiterError
            case 4 => TeXResultCode.TeXError
            case _ => TeXResultCode.UnknownError
        }
    }
}

public class TeXParseException <: Exception {
    public let formula: String
    public let resultCode: TeXResultCode

    public init(formula: String, resultCode: TeXResultCode, message: String) {
        super(message)
        this.formula = formula
        this.resultCode = resultCode
    }
}

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
            var subSringArr : Array<String> = ["aligned", "align", "flalign", "gather", "eqnarray", "multline", "equation"]
            var inputTag: String = ""
            var findRet = find_substring(ltx, subSringArr)
            Hilog.info(0x12, "formula","---- findRet: ${findRet}")
            if(findRet == 1) {
                var inputStr: CString = LibC.mallocCString(ltx)
                inputStr = formula_xingTag(inputStr)
                inputTag = inputStr.toString()
                Hilog.info(0x12, "formula","---- ${inputTag}")
                LibC.free(inputStr)
                if (inputTag.contains("\\ce{") || inputTag.contains("\\pu{")) {
                    processedLtx = Mhchem().process(inputTag)
                } else {
                    processedLtx = inputTag
                }
            } else if(findRet == 2) {
                var inputStr: CString = LibC.mallocCString(ltx)
                inputStr = formula_tag(inputStr)
                inputTag = inputStr.toString()
                Hilog.info(0x12, "formula","---- ${inputTag}")
                LibC.free(inputStr)
                if (inputTag.contains("\\ce{") || inputTag.contains("\\pu{")) {
                    processedLtx = Mhchem().process(inputTag)
                } else {
                    processedLtx = inputTag
                }
            } else if(findRet == 0) {
                if (ltx.contains("\\ce{") || ltx.contains("\\pu{")) {
                    processedLtx = Mhchem().process(ltx)
                }
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

    public func parseWithError(ltx: String, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32): Render {
        if (width <= 0) {
            throw TeXParseException(ltx, TeXResultCode.UnknownError, "Width must be positive")
        }
        if (textSize <= 0.0) {
            throw TeXParseException(ltx, TeXResultCode.UnknownError, "Text size must be positive")
        }
        if (lineSpace < 0.0) {
            throw TeXParseException(ltx, TeXResultCode.UnknownError, "Line space must be non-negative")
        }
        
        unsafe {
            var processedLtx = ltx
            if (ltx.contains("\\ce{") || ltx.contains("\\pu{")) {
                processedLtx = Mhchem().process(ltx)
            }
            var cstr = LibC.mallocCString(processedLtx)
            var resultCodePtr = LibC.malloc<Int32>(count: 1)
            var errorMsgSize: Int32 = 1024
            var errorMsgPtr = LibC.malloc<UInt8>(count: Int64(errorMsgSize))
            
            try {
                var ptr = LaTeX_parse_with_error(cstr, width, textSize, lineSpace, foreground, resultCodePtr, errorMsgPtr, errorMsgSize)
                var resultCode = resultCodePtr.read()
                
                if (!ptr.isNull()) {
                    Render(ptr)
                } else {
                    var errorMsg = CString(errorMsgPtr).toString()
                    var texResultCode = TeXResultCode.fromInt(resultCode)
                    throw TeXParseException(ltx, texResultCode, errorMsg)
                }
            } finally {
                LibC.free(cstr)
                LibC.free(resultCodePtr)
                LibC.free(errorMsgPtr)
            }
        }
    }
}
