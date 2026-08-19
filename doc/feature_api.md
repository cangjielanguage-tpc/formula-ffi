# formula库

## 介绍

formula 主要目的是显示用 LaTeX 编写的数学公式。参考地址：https://github.com/NanoMichael/MicroTeX

## formula

前置条件：NA

场景：

约束：

### 1. ColorFormat

#### 1.1 枚举定义

```
public enum ColorFormat {
    | COLOR_FORMAT_RGB_565      // RGB_565 像素格式
    | COLOR_FORMAT_BGRA_8888   // BGRA_8888 像素格式
}
```

### 2. TeXResultCode

#### 2.1 枚举定义

公式解析结果码，用于 `parseWithError` 方法返回错误信息。

```
public enum TeXResultCode <: ToString & Equatable<TeXResultCode> {
    | Success                 // 解析成功
    | SyntaxError             // 语法错误
    | InvalidMatrixError      // 矩阵格式错误
    | InvalidDelimiterError   // 定界符错误
    | TeXError                // TeX 错误
    | UnknownError            // 未知错误
}
```

#### 2.2 主要接口

```
/*
* 转换为字符串
*
* 返回值 - String
*/
public func toString(): String

/*
* 转换为 Int32
*
* 返回值 - Int32（Success=0, SyntaxError=1, InvalidMatrixError=2, InvalidDelimiterError=3, TeXError=4, UnknownError=99）
*/
public func toInt(): Int32

/*
* 从 Int32 转换为 TeXResultCode
*
* 参数 - code Int32 错误码
*
* 返回值 - TeXResultCode
*/
public static func fromInt(code: Int32): TeXResultCode
```

### 3. TeXParseException

#### 3.1 主要接口

公式解析异常类，包含出错的公式文本、错误码和错误信息。

```
public class TeXParseException <: Exception {
    public let formula: String           // 出错的公式文本
    public let resultCode: TeXResultCode // 错误码

    /*
    * 初始化
    *
    * 参数 - formula 出错的公式文本
    * 参数 - resultCode 错误码
    * 参数 - message 错误信息
    *
    * 返回值 - Unit
    */
    public init(formula: String, resultCode: TeXResultCode, message: String)
}
```

### 4. Graphic2D

#### 4.1 主要接口

```
public class Graphic2D {
    /*
    * 初始化画布
    *
    * 参数 - render Render
    * 参数 - colorFormat 位图像素存储格式
    *
    * 返回值 - Unit
    */
    public init(render: Render, colorFormat: ColorFormat)

    /*
    * 获取画布ffi指针
    *
    * 返回值 - CPointer<UInt8>
    */
    public func getG2(): CPointer<UInt8>

    /*
    * 获取位图像素存储格式(Int32格式)
    *
    * 返回值 - Int32
    */
    public func getColorFormat(): Int32

    /*
    * 画布宽度
    */
    public prop width: UInt32

    /*
    * 画布高度
    */
    public prop height: UInt32

    /*
    * 释放画布资源
    *
    * 返回值 - Unit
    */
    public func release(): Unit
}
```

### 5. LaTeX

#### 5.1 主要接口

```
public class LaTeX {
    /*
    * 初始化LaTex
    *
    * 参数 - rootDir 字体资源路径
    *
    * 返回值 - Unit
    */
    public init(rootDir: String)

    /*
    * 销毁LaTeX
    *
    * 返回值 - Unit
    */
    public func release(): Unit

    /*
    * 解析数学公式
    *
    * 参数 - ltx 数学公式字符串
    * 参数 - width 画布宽度（预设宽度）
    * 参数 - textSize 字体大小
    * 参数 - lineSpace 行距
    * 参数 - foreground 前景色（画笔颜色），ARGB格式，透明度A不能设置为0，不然颜色可能是随机的
    *
    * 返回值 - Render
    */
    public func parse(ltx: String, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32): Render

    /*
    * 解析数学公式（带错误处理）
    *
    * 解析失败时抛出 TeXParseException 异常，包含错误码和错误信息。
    * 参数不合法（width <= 0, textSize <= 0, lineSpace < 0）时也会抛出异常。
    *
    * 参数 - ltx 数学公式字符串
    * 参数 - width 画布宽度（预设宽度），必须大于0
    * 参数 - textSize 字体大小，必须大于0
    * 参数 - lineSpace 行距，必须大于等于0
    * 参数 - foreground 前景色（画笔颜色），ARGB格式，透明度A不能设置为0，不然颜色可能是随机的
    *
    * 返回值 - Render
    * 异常 - TeXParseException 解析失败时抛出
    */
    public func parseWithError(ltx: String, width: Int32, textSize: Float32, lineSpace: Float32, foreground: UInt32): Render
}
```

### 6. Render

#### 6.1 主要接口

```
public class Render {
    /*
    * 初始化
    *
    * 参数 - ptr ffi指针
    *
    * 返回值 - Unit
    */
    public init(ptr: CPointer<UInt8>)

    /*
    * 绘制图片
    *
    * 参数 - g2 Graphic2D
    * 参数 - background 背景色，ARGB格式
    *
    * 返回值 - Unit
    */
    public func draw(g2: Graphic2D, background: UInt32): Unit

    /*
    * 获取字体大小
    *
    * 返回值 - Float32
    */
    public func getTextSize(): Float32

    /*
    * 获取绘制的实际高度（非画布高度）
    *
    * 返回值 - UInt32
    */
    public func getHeight(): UInt32

    /*
    * 获取绘制的实际宽度（非画布宽度）
    *
    * 返回值 - UInt32
    */
    public func getWidth(): UInt32

    /*
    * 销毁Render
    *
    * 返回值 - Unit
    */
    public func finalize(): Unit

    /*
    * 生成bitmap图片资源
    *
    * 参数 - g2 Graphic2D
    *
    * 返回值 - Array<UInt8>
    */
    public func toBitmap(g2: Graphic2D): Array<UInt8>

    /*
    * 获取生成图片资源
    *
    * 注意：不支持 COLOR_FORMAT_RGB_565 格式，使用该格式会抛出异常
    *
    * 参数 - g2 Graphic2D
    *
    * 返回值 - Array<UInt8>
    * 异常 - Exception 当 colorFormat 为 RGB_565 时抛出
    */
    public func getMapData(g2: Graphic2D): Array<UInt8>
}
```

### 示例

test.cj

```
import formula.*
import std.fs.*

main(): Int64 {

    var latex = LaTeX("res")
    var str = ###"
\sideset{^\backprime}{'}\sum_{x=1}^{\infty} x\sideset{a_1^2}{}\sum_{x=1}^\infty x_0
\\
\sideset{_\text{left bottom}'''}{_{\text{right bottom}}'''}\sum_{\text{quite long text}}^\infty x
\\
\sideset{}{'}
\sum_{n<k,\;\text{$n$ odd}} nE_n
\\
\sideset{}{'}
\sum^{n<k,\;\text{$n$ odd}} nE_n
\\
M_x''' M'''_x M^{'''}_x M_x{'''} M^{\prime\backprime}
"###
    var r = latex.parse(str, 2000, 40.0, 10.0, 0xFF000000)
    var w = r.getWidth()
    var h = r.getHeight()
    var g2 = Graphic2D(r, COLOR_FORMAT_RGB_565)
    r.draw(g2, 0xFFFFFFFF)

    var arr = r.toBitmap(g2)

    var file: File = File("test.bmp", OpenOption.CreateOrTruncate(false))
    file.write(arr)
    file.close()

    return 0
}
```

编译运行：

```
cjc --import-path target/aarch64-linux-ohos/release -Ltarget/aarch64-linux-ohos/release/formula -Llib -lnative_drawing  -llatex -lformula_formula test.cj -o main
```

运行结果如下：

![test](./assets/test.bmp)
