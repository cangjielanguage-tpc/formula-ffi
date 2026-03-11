package formula
import formula.mhchem.Mhchem

public enum TeXErrorCode <: ToString {
    | Success
    | SyntaxError
    | InvalidMatrixError
    | InvalidDelimiterError
    | TeXError
    | UnknownError

    public func toString(): String {
        match (this) {
            case TeXErrorCode.Success => "Success"
            case TeXErrorCode.SyntaxError => "SyntaxError"
            case TeXErrorCode.InvalidMatrixError => "InvalidMatrixError"
            case TeXErrorCode.InvalidDelimiterError => "InvalidDelimiterError"
            case TeXErrorCode.TeXError => "TeXError"
            case TeXErrorCode.UnknownError => "UnknownError"
        }
    }

    public func toInt(): Int32 {
        match (this) {
            case TeXErrorCode.Success => 0
            case TeXErrorCode.SyntaxError => 1
            case TeXErrorCode.InvalidMatrixError => 2
            case TeXErrorCode.InvalidDelimiterError => 3
            case TeXErrorCode.TeXError => 4
            case TeXErrorCode.UnknownError => 99
        }
    }

    public static func fromInt(code: Int32): TeXErrorCode {
        match (code) {
            case 0 => TeXErrorCode.Success
            case 1 => TeXErrorCode.SyntaxError
            case 2 => TeXErrorCode.InvalidMatrixError
            case 3 => TeXErrorCode.InvalidDelimiterError
            case 4 => TeXErrorCode.TeXError
            case _ => TeXErrorCode.UnknownError
        }
    }
}

public class TeXParseException <: Exception {
    public let formula: String
    public let errorCode: TeXErrorCode

    public init(formula: String, errorCode: TeXErrorCode, message: String) {
        super(message)
        this.formula = formula
        this.errorCode = errorCode
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

    public func parseWithError(ltx: String, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32): Render {
        if (width <= 0) {
            throw TeXParseException(ltx, TeXErrorCode.UnknownError, "Width must be positive")
        }
        if (textSize <= 0.0) {
            throw TeXParseException(ltx, TeXErrorCode.UnknownError, "Text size must be positive")
        }
        if (lineSpace < 0.0) {
            throw TeXParseException(ltx, TeXErrorCode.UnknownError, "Line space must be non-negative")
        }
        
        unsafe {
            var processedLtx = ltx
            if (ltx.contains("\\ce{") || ltx.contains("\\pu{")) {
                processedLtx = Mhchem().process(ltx)
            }
            var cstr = LibC.mallocCString(processedLtx)
            var errorCodePtr = LibC.malloc<Int32>(count: 1)
            var errorMsgSize: Int32 = 1024
            var errorMsgPtr = LibC.malloc<UInt8>(count: Int64(errorMsgSize))
            
            try {
                var ptr = LaTeX_parse_with_error(cstr, width, textSize, lineSpace, foreground, errorCodePtr, errorMsgPtr, errorMsgSize)
                var errorCode = errorCodePtr.read()
                
                if (!ptr.isNull()) {
                    Render(ptr)
                } else {
                    var errorMsg = CString(errorMsgPtr).toString()
                    var texErrorCode = TeXErrorCode.fromInt(errorCode)
                    throw TeXParseException(ltx, texErrorCode, errorMsg)
                }
            } finally {
                LibC.free(cstr)
                LibC.free(errorCodePtr)
                LibC.free(errorMsgPtr)
            }
        }
    }
}