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

formula_hybrid 工程是 formula 的 Hybrid（ArkTS 混合开发）形态，主要目的是在 ArkTS 应用中显示用 LaTeX 编写的公式。

### 特性

- 🚀 公式解析

  提供解析 LaTeX 公式接口，支持 `latexStringToImageWithError` 方法返回详细错误码和错误信息

- 🚀 位图输出

  提供生成图片数据接口，支持 RGB_565 和 BGRA_8888 两种像素格式

## 软件架构

工程包含 `formula_hybrid` 库模块与 `entry` 示例应用模块，`entry` 通过本地源码模块（`file:../formula_hybrid`）依赖 formula_hybrid，formula_hybrid 通过预编译 base 包 `formula_hybrid/har/formula.har`（`@cangjie-tpc/formula`，提供 LaTeX 解析/渲染核心能力）获取底层能力。formula_hybrid 自身仅保留仓颉互操作层，经 JSModule 把仓颉接口注册到 `libformula_hybrid.so`，ArkTS 侧通过 `requireCJLib` 加载；接口调用在后台线程异步执行，仓颉侧异常已兜底（记录日志并 `promise.reject`），JS 侧可通过 `catch` 捕获；`entry` 演示在 ArkTS 页面中调用接口渲染公式图片。

### 源码目录

```shell
formula-ffi_hybrid
├─ AppScope                                         # 应用级配置
├─ doc                                              # 文档目录
│  ├─ assets                                        # 示例效果图
│  └─ feature_api.md                                # 接口说明文档
├─ entry                                            # 示例应用模块
│  └─ src\main
│     ├─ ets
│     │  ├─ entryability                            # Ability
│     │  └─ pages                                   # 示例页面（Index、Index0~Index14 等）
│     └─ resources\rawfile                          # 测试公式文件（test00~test111）
├─ formula_hybrid                                   # formula_hybrid 库模块（详见模块 README）
│  ├─ example                                       # 互操作调用示例
│  ├─ har
│  │  └─ formula.har                                # 预编译 base 包（pre-install 钩子自动下载，不入库）
│  ├─ src\main
│  │  ├─ cangjie                                    # 仓颉互操作层（loader/interop/public_latex/scope）
│  │  ├─ ets\components\MainPage.ets                # ArkTS 接口声明与导出
│  │  └─ resources\base                             # 模块资源
│  └─ Index.ets / cjpm.toml / build-profile.json5 / oh-package.json5
├─ hvigor                                           # 构建工具配置
├─ scripts
│  └─ pre_har_install.mjs                           # pre-install 钩子：formula.har 缺失时自动下载
├─ build-profile.json5                              # 工程构建配置
├─ oh-package.json5                                 # 工程依赖配置
├─ CHANGELOG.md / LICENSE / README.OpenSource
└─ README.md
```

### 接口说明

主要类和函数接口说明详见 [API](./doc/feature_api.md)

## 使用说明

### 编译构建

使用 DevEco Studio 打开工程根目录，执行依赖同步（`ohpm install`）后直接编译运行 `entry` 模块即可。若 `formula_hybrid/har/formula.har` 不存在，工程 pre-install 钩子（`scripts/pre_har_install.mjs`）会自动从 Cangjie-TPC release 下载。

依赖链为 `entry` → `@cangjie-tpc/formula_hybrid`（源码模块 `file:../formula_hybrid`）→ `@cangjie-tpc/formula`（`formula_hybrid/har/formula.har`）。`entry` 与 `formula_hybrid` 的 build-profile.json5 均已配置 `nativeLib.filter.select`，排除 formula.har 内携带的仓颉 runtime 库（`libs/*/runtime/**`），保证打包产物中 runtime 库由仓颉构建插件按 SDK 版本唯一注入；由于当前 formula.har 仍携带 runtime 库，该配置在 HAR 依赖形态下必须保留，不可删除。

### ohpm安装使用

```cmd
ohpm install @cangjie-tpc/formula_hybrid
```

### 功能示例

#### 解析LaTeX公式

示例代码位于 `entry/src/main/ets/pages/Index0.ets`：

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

![img1](./doc/assets/img.png)

![img1](./doc/assets/img.jpg)

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
