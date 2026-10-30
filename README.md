<div align="center">
<h1>formula-ffi</h1>
</div>

<p align="center">
<img alt="" src="https://img.shields.io/badge/release-v2.0.0-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/build-pass-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/cjc-v1.1.3-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/cjcov-NA-red" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/project-open-brightgreen" style="display: inline-block;" />
</p>

## 介绍

formula 主要目的是显示用 LaTeX 编写的公式。

### 特性

- 🚀 公式解析

  提供解析 LaTeX 公式接口，支持 `parseWithError` 方法返回详细错误码和错误信息

- 🚀 位图输出

  提供生成 bitmap 接口，支持 RGB_565 和 BGRA_8888 两种像素格式

- 💪 图片资源

  提供生成图片资源接口

## 软件架构

整体分三层：`formula/src/main/cangjie` 提供仓颉对外接口（LaTeX 解析、Render/Graphic2D 渲染封装、mhchem 语法扩展）；经 FFI 调用 `formula/src/main/cpp/ffi` C++ 桥接层；底层链接上游 latex C++ 渲染库（`formula/src/main/cpp/latex` 源码树由 `cpp/cmake/clone-repo.cmake` 在构建时克隆，编译产物 `liblatex.so` 预置于 `formula/libs` 下按 ABI 存放）。

### 源码目录

```shell
formula-ffi
├─ AppScope                                                # 应用级配置
├─ doc                                                     # 文档目录
│  ├─ assets                                               # 示例效果图
│  └─ feature_api.md                                      # 接口说明文档
├─ entry                                                   # 示例应用模块
│  └─ src\main
│     ├─ cangjie                                           # 示例页面代码
│     └─ resources\rawfile                                 # 测试公式文件
├─ formula                                                 # formula 库模块
│  ├─ libs                                                 # 预编译 liblatex.so（按 ABI 存放，供 ffi.c.latex 链接）
│  │  ├─ arm64-v8a
│  │  └─ x86_64
│  └─ src\main
│     ├─ cangjie                                           # 仓颉侧核心代码
│     │  ├─ mhchem                                         # mhchem 语法扩展
│     │  ├─ latex.cj                                       # LaTeX 解析入口封装
│     │  ├─ render.cj                                      # Render 渲染接口封装
│     │  ├─ graphic_2d.cj                                  # Graphic2D 与像素格式封装
│     │  └─ ffi.cj                                         # FFI 声明
│     ├─ cpp                                               # C++ 侧代码
│     │  ├─ cmake\clone-repo.cmake                         # latex 上游源码克隆脚本
│     │  ├─ ffi                                            # FFI 桥接层（latex_ffi/render_ffi/graphic_ohos 等）
│     │  └─ latex                                          # 上游 latex C++ 源码树（构建时克隆）
│     └─ resources\resfile\res                             # 渲染资源（字体、符号映射等）
├─ hvigor                                                 # 构建工具配置
├─ scripts
│  └─ pre_stdx_install.mjs                                 # stdx 依赖下载脚本（preInstall 钩子）
├─ test                                                    # 测试目录
│  ├─ DOC
│  ├─ HLT
│  └─ LLT
├─ build-profile.json5                                     # 工程构建配置
├─ oh-package.json5                                        # 工程依赖与 hooks 配置
├─ CHANGELOG.md
├─ LICENSE
└─ README.md
```

### 接口说明

主要类和函数接口说明详见 [API](./doc/feature_api.md)

## 使用说明

### 编译构建

使用 DevEco Studio 打开工程根目录，依赖同步阶段 preInstall 钩子（`scripts/pre_stdx_install.mjs`）会自动下载 stdx 预编译库到 `formula/stdx_bin/` 目录，随后直接编译运行 `entry` 模块即可。

### 功能示例

#### 生成bitmap功能示例

示例代码如下：

```cangjie
import ohos.base.*
import ohos.component.*
import ohos.state_manage.*
import ohos.state_macro_manage.*
import ohos.image.PixelMap
import ohos.image.ImageSource
import ohos.image.createImageSource
import formula.*

@Entry
@Component
class EntryView {
    let array: Array<String> = [
        ##"
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
        "##,
        ##"\ce{H2O} \ce{Sb2O3} \ce{H+} \ce{CrO4^2-} \ce{[AgCl2]-} \ce{Y^99+} \ce{Y^{99+}}"##,
        ##"\ce{^{227}_{90}Th+} \ce{^227_90Th+} \ce{^{0}_{-1}n^{-}} \ce{^0_-1n-}"##,
        ##"\ce{H{}^3HO} \ce{H^3HO}"##,
        ##"\ce{A -> B} \ce{A <- B} \ce{A <-> B}"##,
        ##"\ce{SO4^2- + Ba^2+ -> BaSO4 v}"##,
        ##"\ce{A v B (v) -> B ^ B (^)}"##,
        ##"\ce{$K = \frac{[\ce{Hg^2+}][\ce{Hg}]}{[\ce{Hg2^2+}]}$}"##,
        ##"\ce{$K = \ce{\frac{[Hg^2+][Hg]}{[Hg2^2+]}}$}"##,
        ##"\ce{Hg^2+ ->[I-] $\underset{\mathrm{red}}{\ce{HgI2}}$ ->[I-]$\underset{\mathrm{red}}{\ce{[Hg^{II}I4]^2-}}$}"##,
        ##"\ce{2H2O} \ce{0.5 H2O} \ce{1/2H2O} \ce{(1/2)H2O} \ce{$n$ H2O}"##,
        ##"\ce{A <--> B} \ce{A <=> B} \ce{A <=>> B} \ce{A <<=> B}"##,
        ##"\ce{A ->[H2O] B} \ce{A ->[{上方文字}][{下方文字}] B}"##,
        ##"\ce{Zn^2+ <=>[+ 2OH-][+ 2H+] $\underset{\text{amphoteres Hydroxid}}{\ce{Zn(OH)2 v}}$ <=>[+2OH-][+ 2H+] $\underset{\text{Hydroxozikat}}{\ce{[Zn(OH)4]^2-}}$}"##
    ]

    @State
    var count = 0
    @State
    var imagePixelMap: Option<PixelMap> = Option<PixelMap>.None
    @State
    var loaded: Bool = false
    @State
    var width: Int32 = 0
    @State
    var height: Int32 = 0
    func build() {
        Row {
            Column(10) {
                if (loaded) {
                    Image(imagePixelMap.getOrThrow())
                        .height(Int64(height))
                        .width(Int64(width))
                }
                Button("test")
                    .width(90.percent)
                    .height(50)
                    .shape(ShapeType.Normal)
                    .onClick {
                        evt => spawn {
                            var latex = LaTeX("/data/storage/el1/bundle/entry/resources/resfile/res")
                            var str = array[count]
                            var r = latex.parse(str, 2000, 15.0, 10.0, 0xFF000000)
                            var g2 = Graphic2D(r, COLOR_FORMAT_RGB_565)
                            r.draw(g2, 0xFFFFFFFF)
                            var arr = r.toBitmap(g2)
                            let imageSourceApi: ImageSource = createImageSource(arr)
                            let pixelMap: Option<PixelMap> = imageSourceApi.createPixelMap()
                            if (let Some(p) <- pixelMap) {
                                width = p.getImageInfo().size.width
                                height = p.getImageInfo().size.height
                                imagePixelMap = p
                                loaded = true
                            }
                            if (count < array.size - 1) {
                                count++
                            } else {
                                count = 0
                            }
                        }
                    }
            }.width(100.percent)
        }.height(100.percent)
    }
}
```

#### 使用 parseWithError 进行错误处理示例

```cangjie
import formula.*

func renderWithError(ltx: String): Unit {
    var latex = LaTeX("/data/storage/el1/bundle/entry/resources/resfile/res")
    try {
        var r = latex.parseWithError(ltx, 2000, 15.0, 10.0, 0xFF000000)
        var g2 = Graphic2D(r, COLOR_FORMAT_BGRA_8888)
        r.draw(g2, 0xFFFFFFFF)
        var arr = r.toBitmap(g2)
        // 使用 arr ...
        g2.release()
        r.finalize()
    } catch (e: TeXParseException) {
        // e.formula    - 出错的公式文本
        // e.resultCode - 错误码（TeXResultCode）
        // e.message    - 错误信息
        println("解析失败: ${e.resultCode.toString()} - ${e.message}")
    } finally {
        latex.release()
    }
}
```

执行结果如下：

![test](./doc/assets/test.jpg)

## 约束与限制

在下述版本验证通过：

| 编号 | 依赖构建工具                           | 版本号       |
|----|----------------------------------|-----------|
| 1  | **DevEco Studio**                | 6.1.1     |
| 2  | **cjc**                          | v1.1.3    |

formula依赖三方库：

| 编号 | 依赖三方库         | 版本号      |
|----|---------------|----------|
| 1  | stdx          | v1.1.3.1 |

三方库静态链接和动态链接区别

- 静态链接：在编译期间，将所有依赖stdx的库函数和代码会被链接到最终的可执行文件中；生成的so中包含了所有需要的代码，不需要再依赖外部的stdx库。

  **缺点**： 由于静态链接将所有代码（包括库函数）都包含进最终的可执行文件，因此生成的可执行文件会比动态链接的文件要大。

- 动态链接：编译的har包中，会将所有使用到的stdx的二进制so添加到har的libs文件夹内；动态链接程序依赖于共享库（例如 .dll、.so），这些库在程序启动时或者运行时被加载到内存中。

  **缺点**：由于stdx版本之前存在不兼容，因此三方库和hap包依赖的stdx版本不一致情况下，存在运行crash情况。

当前三方库默认通过静态链接方式，如果有动态链接需求可以通过修改库代码中的cjpm.toml文件链接到stdx动态链接库目录，

```
[target]
  [target.aarch64-linux-ohos]
  	  ...
      path-option = [ "${AARCH64_LIBS}", "${AARCH64_MACRO_LIBS}", "${AARCH64_KIT_LIBS}", "./stdx_bin/linux_ohos_aarch64_cjnative/static/stdx" ]
      [target.aarch64-linux-ohos.bin-dependencies.package-option]
  [target.x86_64-linux-ohos]
      ...
      path-option = [ "${X86_64_OHOS_LIBS}", "${X86_64_OHOS_MACRO_LIBS}", "${X86_64_OHOS_KIT_LIBS}", "./stdx_bin/linux_ohos_x86_64_cjnative/static/stdx" ]
  [target.x86_64-unknown-windows-gnu]
    [target.x86_64-unknown-windows-gnu.bin-dependencies]
      path-option = [ "${X86_64_LIBS}", "${X86_64_MACRO_LIBS}", "./stdx_bin/windows_x86_64_cjnative/static/stdx" ]
      [target.x86_64-unknown-windows-gnu.bin-dependencies.package-option]
```

修改如下：

```
[target]
  [target.aarch64-linux-ohos]
      ...
      path-option = [ "${AARCH64_LIBS}", "${AARCH64_MACRO_LIBS}", "${AARCH64_KIT_LIBS}", "./stdx_bin/linux_ohos_aarch64_cjnative/dynamic/stdx" ]
      [target.aarch64-linux-ohos.bin-dependencies.package-option]
  [target.x86_64-linux-ohos]
      ...
      path-option = [ "${X86_64_OHOS_LIBS}", "${X86_64_OHOS_MACRO_LIBS}", "${X86_64_OHOS_KIT_LIBS}", "./stdx_bin/linux_ohos_x86_64_cjnative/dynamic/stdx" ]
  [target.x86_64-unknown-windows-gnu]
    [target.x86_64-unknown-windows-gnu.bin-dependencies]
      path-option = [ "${X86_64_LIBS}", "${X86_64_MACRO_LIBS}", "./stdx_bin/windows_x86_64_cjnative/dynamic/stdx" ]
      [target.x86_64-unknown-windows-gnu.bin-dependencies.package-option]
```

1. `resPath`默认参数`"/data/storage/el1/bundle/entry/resources/resfile/res"`，如果修改`entry`命名，需要改成对应的`"/data/storage/el1/bundle/xxxx/resources/resfile/res"`。

2. unicode扩展支持的字符范围请参考[U1D400](https://www.unicode.org/charts/PDF/U1D400.pdf)。

3. NewCommand扩展包仅支持\def,\newenvironment,\renewenvironment

## 开源协议

本项目基于 [MIT License](./LICENSE)，请自由的享受和参与开源。

## 参与贡献

欢迎给我们提交PR，欢迎给我们提交Issue，欢迎参与任何形式的贡献。
