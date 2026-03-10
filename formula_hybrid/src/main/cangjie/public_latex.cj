/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2026. All rights reserved.
 */
package formula

public class TeXParseResult {
    public let isSuccess: Bool
    public let imageBytes: Array<Byte>
    public let formula: String
    public let errorCode: TeXErrorCode
    public let errorMessage: String

    private init(isSuccess: Bool, imageBytes: Array<Byte>, formula: String, errorCode: TeXErrorCode, errorMessage: String) {
        this.isSuccess = isSuccess
        this.imageBytes = imageBytes
        this.formula = formula
        this.errorCode = errorCode
        this.errorMessage = errorMessage
    }

    public static func ok(imageBytes: Array<Byte>): TeXParseResult {
        TeXParseResult(true, imageBytes, "", TeXErrorCode.Success, "")
    }

    public static func fail(formula: String, errorCode: TeXErrorCode, errorMessage: String): TeXParseResult {
        TeXParseResult(false, Array<Byte>(), formula, errorCode, errorMessage)
    }
}

public func latexStringToImage(latexMathTextString: String, latexMathTextSize: Float64,latexMathTextColor: UInt32, latexMathBackGroupColor: UInt32, latexMathColorFormat: Int64, resPath: String): Array<Byte> {
    try {
        var latex = LaTeX(resPath)
        let latexMathTextNewString = latexStrCallback(latexMathTextString)
        var r = latex.parse(latexMathTextNewString, getLatexmathWidth(latexMathTextNewString), Float32(latexMathTextSize), Float32(10.0), latexMathTextColor)
        var g2 = Graphic2D(r, latexMathColorFormatEnum(latexMathColorFormat))
        r.draw(g2, latexMathBackGroupColor)
        let arr = r.toBitmap(g2)
        latex.release()
        r.finalize()
        unsafe {
            if (g2.getG2().isNotNull()) {
                LibC.free(g2.getG2())
            }
        }
        return arr
    } catch (_: Exception) {
        return Array<Byte>()
    }
}

public func latexStringToImageWithError(latexMathTextString: String, latexMathTextSize: Float64, latexMathTextColor: UInt32, latexMathBackGroupColor: UInt32, latexMathColorFormat: Int64, resPath: String): TeXParseResult {
    try {
        var latex = LaTeX(resPath)
        let latexMathTextNewString = latexStrCallback(latexMathTextString)
        var r = latex.parseWithError(latexMathTextNewString, getLatexmathWidth(latexMathTextNewString), Float32(latexMathTextSize), Float32(10.0), latexMathTextColor)
        var g2 = Graphic2D(r, latexMathColorFormatEnum(latexMathColorFormat))
        r.draw(g2, latexMathBackGroupColor)
        let arr = r.toBitmap(g2)
        latex.release()
        r.finalize()
        unsafe {
            if (g2.getG2().isNotNull()) {
                LibC.free(g2.getG2())
            }
        }
        return TeXParseResult.ok(arr)
    } catch (e: TeXParseException) {
        return TeXParseResult.fail(e.formula, e.errorCode, e.message)
    } catch (e: Exception) {
        return TeXParseResult.fail(latexMathTextString, TeXErrorCode.UnknownError, e.message)
    }
}

func getLatexmathWidth(strLatexMath: String): Int32 {
    if (strLatexMath.contains("aligned") || strLatexMath.contains("align") || strLatexMath.contains("flalign") || strLatexMath.contains("gather") || strLatexMath.contains("multline") || strLatexMath.contains("eqnarray")) {
        Int32(10)
    } else {
        Int32(2000)
    }
}

func latexMathColorFormatEnum(latexMathColorFormat: Int64): ColorFormat {
    return match (latexMathColorFormat) {
        case 2 => ColorFormat.COLOR_FORMAT_RGB_565
        case 5 => ColorFormat.COLOR_FORMAT_BGRA_8888
        case _ => ColorFormat.COLOR_FORMAT_BGRA_8888
    }
}

func latexStrCallback(strLatexMath: String): String {
    var dataNew = strLatexMath.replace("{aligned*}", "{aligned}")
    dataNew = dataNew.replace("{align*}", "{align}")
    dataNew = dataNew.replace("{flalign*}", "{flalign}")
    dataNew = dataNew.replace("{gather*}", "{gather}")
    dataNew = dataNew.replace("{multline*}", "{multline}")
    dataNew = dataNew.replace("{eqnarray*}", "{eqnarray}")
    return dataNew
}
