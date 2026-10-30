<div align="center">
<h1>formula_hybrid</h1>
</div>

<p align="center">
<img alt="" src="https://img.shields.io/badge/release-v2.0.0-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/build-pass-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/cjc-v1.1.3-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/cjcov-NA-red" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/project-open-brightgreen" style="display: inline-block;" />
</p>

## 介绍

formula_hybrid 是 formula 的 Hybrid（ArkTS 混合开发）形态包，主要目的是在 ArkTS 应用中显示用 LaTeX 编写的公式。

### 特性

- 🚀 公式解析

  提供解析 LaTeX 公式接口，支持 `latexStringToImageWithError` 方法返回详细错误码和错误信息

- 🚀 位图输出

  提供生成图片数据接口，支持 RGB_565 和 BGRA_8888 两种像素格式

## 软件架构

formula_hybrid 通过 ohpm 依赖预编译 base 包 `@cangjie-tpc/formula`（`./har/formula.har`）获取 LaTeX 解析/渲染核心能力，自身仅保留仓颉互操作层与 ArkTS 接口层：仓颉侧经 JSModule 把接口注册到 `libformula_hybrid.so`，ArkTS 侧通过 `requireCJLib` 加载并对外导出。

### 源码目录

```shell
formula_hybrid
├─ example                                          # 互操作调用示例
├─ src\main
│  ├─ cangjie                                       # 仓颉互操作层
│  │  ├─ loader                                     # libark_interop_loader.so 依赖声明
│  │  ├─ interop.cj                                 # JSModule 桥接，注册对外接口
│  │  ├─ public_latex.cj                            # 公开 API 封装（调用 base 包）
│  │  └─ scope.cj                                   # NAPI handle scope 管理
│  ├─ ets\components\MainPage.ets                   # ArkTS 接口声明与导出
│  ├─ module.json5                                  # 模块配置
│  └─ resources\base                                # 模块资源
├─ Index.ets                                        # 对外导出入口
├─ cjpm.toml                                        # 仓颉构建配置（链接 base 包 cjbins）
├─ build-profile.json5                              # 模块构建配置（cangjieOptions / nativeLib 过滤）
├─ oh-package.json5                                 # 模块依赖配置
├─ CHANGELOG.md / LICENSE / README.OpenSource
└─ README.md
```

- `interop.cj` - JS 互操作桥接，注册 `latexStringToImage` / `latexStringToImageWithError` 到 JSModule；调用在 `spawn` 线程异步执行（互斥锁串行化），仓颉侧异常兜底：`AppLog` 记录日志并通过 `promise.reject` 返回错误
- `public_latex.cj` - 公开 API 封装层，调用 base 包的 `LaTeX`/`Render`/`Graphic2D` 等
- `scope.cj` - NAPI handle scope 管理
- `MainPage.ets` - ArkTS 接口声明与导出，通过 `requireCJLib("libformula_hybrid.so")` 加载仓颉侧实现
- `cjpm.toml` - 通过 `${cangjie_tpc_formula}/libs/{ABI}/cjbins/formula` 链接 base 包预编译产物，自身不直接链接 stdx

### 接口说明

主要类和函数接口说明

```ets
/*
* 通过文本参数生成 LaTeX 公式图片数组数据
*
* 参数 - latexMathTextString LaTeX 公式文本内容
* 参数 - latexMathTextSize LaTeX 公式文字大小 - 单位px
* 参数 - latexMathTextColor LaTeX 公式文字颜色
* 参数 - latexMathBackGroupColor LaTeX 公式背景颜色
* 参数 - latexMathColorFormat LaTeX 公式图片格式
* 参数 - resPath 字体资源路径。 默认"/data/storage/el1/bundle/entry/resources/resfile/res"
*
* 返回值 - Promise<ArrayBuffer> 图片数组数据
*/
latexStringToImage(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: LatexMathColorFormat, resPath?: string): Promise<ArrayBuffer>

/**
 * 图片格式枚举
 */
enum LatexMathColorFormat {
  COLOR_FORMAT_RGB_565, // RGB_565
  COLOR_FORMAT_BGRA_8888 // BGRA_8888
}

/**
 * TeX解析结果码枚举
 */
enum TeXResultCode {
  Success = 0,              // 解析成功
  SyntaxError = 1,          // 语法错误
  InvalidMatrixError = 2,   // 无效矩阵错误
  InvalidDelimiterError = 3, // 无效分隔符错误
  TeXError = 4,             // TeX错误
  UnknownError = 99         // 未知错误
}

/**
 * TeX解析结果接口
 */
interface TeXParseResult {
  imageBytes: ArrayBuffer   // 图片字节数组（成功时有效）
  formula: string           // 原始公式文本
  resultCode: TeXResultCode // 结果码（Success表示成功）
  errorMessage: string      // 错误信息
}

/*
* 通过文本参数生成 LaTeX 公式图片数组数据（带错误信息）
* 当公式解析失败时，返回详细的错误信息，包括结果码和错误描述
*
* 参数 - latexMathTextString LaTeX 公式文本内容
* 参数 - latexMathTextSize LaTeX 公式文字大小 - 单位px
* 参数 - latexMathTextColor LaTeX 公式文字颜色
* 参数 - latexMathBackGroupColor LaTeX 公式背景颜色
* 参数 - latexMathColorFormat LaTeX 公式图片格式
* 参数 - resPath 字体资源路径。 默认"/data/storage/el1/bundle/entry/resources/resfile/res"
*
* 返回值 - Promise<TeXParseResult> 解析结果对象
*          - 成功时：resultCode=Success, imageBytes包含图片数据
*          - 失败时：resultCode为错误码, imageBytes为空, errorMessage包含错误详情
*/
latexStringToImageWithError(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: LatexMathColorFormat, resPath?: string): Promise<TeXParseResult>
```

完整接口说明详见 [API](../doc/feature_api.md)。

## 使用说明

### ohpm安装使用

```cmd
ohpm install @cangjie-tpc/formula_hybrid
```

### 功能示例

#### 解析LaTeX公式

```ets
import { LatexMathColorFormat, latexStringToImage } from '@cangjie-tpc/formula_hybrid';
import { image } from '@kit.ImageKit';

@Entry
@Component
struct Index0 {
  str: string = "(a \\pm b)^2 = a^2 \\pm 2ab + b^2"
  @State pixelMap: image.PixelMap = undefined!;
  @State imageWidth: number = 0;
  @State imageHeight: number = 0;

  async aboutToAppear(): Promise<void> {
    try {
      // 通过接口解析 LaTeX 公式获取公式图片数组数据
      let buf: ArrayBuffer = await latexStringToImage(this.str, fp2px(20), 0xFF000000, 0xFFFFFFFF,
        LatexMathColorFormat.COLOR_FORMAT_BGRA_8888)
      let imageSource = image.createImageSource(buf)
      // 图片pixelMap
      this.pixelMap = imageSource.createPixelMapSync()
      let size: Size = this.pixelMap.getImageInfoSync().size
      // 图片宽度
      this.imageWidth = px2vp(size.width)
      // 图片高度
      this.imageHeight = px2vp(size.height)
    } catch (e) {
    }
  }

  build() {
    Scroll() {
      Column() {
        Image(this.pixelMap)
          .objectFit(ImageFit.Contain)
          .width(this.imageWidth)
          .height(this.imageHeight)
          .margin({ top: 5, bottom: 5 })
      }
      .width('100%')
      .height('100%')
      .alignItems(HorizontalAlign.Start)
      .justifyContent(FlexAlign.Start)
    }
    .height('100%')
    .scrollBar(BarState.Off)
    .backgroundColor(Color.White)
  }
}
```

#### 执行结果如下

LaTeX 公式效果：

![img1](../doc/assets/img.png)

![img1](../doc/assets/img.jpg)

## 约束与限制

在下述版本验证通过：

| 编号 | 依赖构建工具                           | 版本号       |
|----|----------------------------------------------|-----------|
| 1  | **DevEco Studio**                            | 6.1.1.280 |
| 2  | **cjc**                                      | v1.1.3    |
| 3  | **DevEco Studio-Cangjie Plugin 6.1.1 Beta1** | 6.1.1.280 |
| 4  | **compatibility-sdk-6.1.1.280**              | 6.1.1.280 |

formula（base 包）依赖三方库：

| 编号 | 依赖三方库         | 版本号      |
|----|---------------|----------|
| 1  | stdx          | v1.1.3.1 |

三方库静态链接和动态链接区别

- 静态链接：在编译期间，将所有依赖stdx的库函数和代码会被链接到最终的可执行文件中；生成的so中包含了所有需要的代码，不需要再依赖外部的stdx库。

  **缺点**： 由于静态链接将所有代码（包括库函数）都包含进最终的可执行文件，因此生成的可执行文件会比动态链接的文件要大。

- 动态链接：编译的har包中，会将所有使用到的stdx的二进制so添加到har的libs文件夹内；动态链接程序依赖于共享库（例如 .dll、.so），这些库在程序启动时或者运行时被加载到内存中。

  **缺点**：由于stdx版本之前存在不兼容，因此三方库和hap包依赖的stdx版本不一致情况下，存在运行crash情况。

formula_hybrid 自身的 cjpm.toml 不直接链接 stdx，而是链接 base 包的预编译 cjbins：

```toml
[target.aarch64-linux-ohos.bin-dependencies]
  path-option = [ "${AARCH64_LIBS}", "${AARCH64_MACRO_LIBS}", "${AARCH64_KIT_LIBS}", "${cangjie_tpc_formula}/libs/arm64-v8a/cjbins/formula" ]

[target.x86_64-linux-ohos.bin-dependencies]
  path-option = [ "${X86_64_OHOS_LIBS}", "${X86_64_OHOS_MACRO_LIBS}", "${X86_64_OHOS_KIT_LIBS}", "${cangjie_tpc_formula}/libs/x86_64/cjbins/formula" ]
```

stdx 的静态/动态链接方式在构建 base 包（formula.har）时确定，如有动态链接需求，请修改 formula-ffi 仓 `formula/cjpm.toml` 的 stdx 链接目录并重新构建 har，详见 formula-ffi 仓 README。

1. `resPath`默认参数`"/data/storage/el1/bundle/entry/resources/resfile/res"`，如果修改`entry`命名，需要改成对应的`"/data/storage/el1/bundle/xxxx/resources/resfile/res"`。

2. unicode扩展支持的字符范围请参考[U1D400](https://www.unicode.org/charts/PDF/U1D400.pdf)。

3. NewCommand扩展包仅支持\def,\newenvironment,\renewenvironment

## 开源协议

本项目基于 [MIT License](./LICENSE)，请自由的享受和参与开源。

## 参与贡献

欢迎给我们提交PR，欢迎给我们提交Issue，欢迎参与任何形式的贡献。
