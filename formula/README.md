<div align="center">
<h1>formula-ffi</h1>
</div>

<p align="center">
<img alt="" src="https://img.shields.io/badge/release-v1.3.2-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/build-pass-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/cjc-v1.0.5-brightgreen" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/cjcov-NA-red" style="display: inline-block;" />
<img alt="" src="https://img.shields.io/badge/project-open-brightgreen" style="display: inline-block;" />
</p>

## 介绍

formula 主要目的是显示用 LaTeX 编写的数学公式。支持显示化学公式。

### 特性

- 🚀 特性1

  提供生成解析数学公式和化学公式接口

- 🚀 特性2

  提供生成bitmap接口

- 💪 特性3

  提供生成图片资源接口

## 软件架构

### 源码目录

```shell
formula
├─ doc                                                   # 文档目录
│  ├─ assets
│  ├─ design.md
│  └─ feature_api.md
├─ entry                                                 # 示例代码文件夹
├─ formula                                               # formula库
│  ├─ src\main\resources\resfile\res                     # 字体资源      
│  ├─ src\main\cangjie                                   # 仓颉侧核心代码
│  │             └─ mhchem                               # 化学公式模块
│  └─ cpp\ffi                                            # 仓颉ffi核心代码
├─ README.md
└─ test                                                  # 测试目录 
   ├─ HLT
   └─ LLT

```

### 接口说明

主要类和函数接口说明详见 [API](https://gitcode.com/Cangjie-TPC/formula-ffi/blob/formula-ffi_cangjie-plugin_5.1.1/doc/feature_api.md)

## 使用说明

### 编译构建

直接编译运行DevEco Studio


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

## 约束与限制

    在下述版本验证通过：    
| 编号 | 依赖构建工具                           | 版本号       |
|----|----------------------------------|-----------|
| 1  | **DevEco Studio**                | 5.1.1.851 |
| 2  | **cjc**                          | v1.0.5    |

formula依赖三方库： 

| 编号 | 依赖三方库         | 版本号      |
|----|---------------|----------|
| 1  | stdx          | v1.0.1.1 |

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
      path-option = [ "${AARCH64_LIBS}", "${AARCH64_MACRO_LIBS}", "${AARCH64_KIT_LIBS}", "../stdx_bin/linux_ohos_aarch64_llvm/static/stdx" ]
      [target.aarch64-linux-ohos.bin-dependencies.package-option]
  [target.x86_64-linux-ohos]
      ...
      path-option = [ "${X86_64_OHOS_LIBS}", "${X86_64_OHOS_MACRO_LIBS}", "${X86_64_OHOS_KIT_LIBS}", "../stdx_bin/linux_ohos_x86_64_llvm/static/stdx" ]
  [target.x86_64-unknown-windows-gnu]
    [target.x86_64-unknown-windows-gnu.bin-dependencies]
      path-option = [ "${X86_64_LIBS}", "${X86_64_MACRO_LIBS}", "../stdx_bin/windows_x86_64_llvm/static/stdx" ]
      [target.x86_64-unknown-windows-gnu.bin-dependencies.package-option]
```

修改如下：

```
[target]
  [target.aarch64-linux-ohos]
      ...
      path-option = [ "${AARCH64_LIBS}", "${AARCH64_MACRO_LIBS}", "${AARCH64_KIT_LIBS}", "../stdx_bin/linux_ohos_aarch64_llvm/dynamic/stdx" ]
      [target.aarch64-linux-ohos.bin-dependencies.package-option]
  [target.x86_64-linux-ohos]
      ...
      path-option = [ "${X86_64_OHOS_LIBS}", "${X86_64_OHOS_MACRO_LIBS}", "${X86_64_OHOS_KIT_LIBS}", "../stdx_bin/linux_ohos_x86_64_llvm/dynamic/stdx" ]
  [target.x86_64-unknown-windows-gnu]
    [target.x86_64-unknown-windows-gnu.bin-dependencies]
      path-option = [ "${X86_64_LIBS}", "${X86_64_MACRO_LIBS}", "../stdx_bin/windows_x86_64_llvm/dynamic/stdx" ]
      [target.x86_64-unknown-windows-gnu.bin-dependencies.package-option]
```

1. `resPath`默认参数`"/data/storage/el1/bundle/entry/resources/resfile/res"`，如果修改`entry`命名，需要改成对应的`"/data/storage/el1/bundle/xxxx/resources/resfile/res"`。

2. unicode扩展支持的字符范围请参考[U1D400](https://www.unicode.org/charts/PDF/U1D400.pdf)。

3. NewCommand扩展包仅支持\def,\newenvironment,\renewenvironment
## 开源协议

本项目基于 [MIT License](./LICENSE)，请自由的享受和参与开源。

## 参与贡献

欢迎给我们提交PR，欢迎给我们提交Issue，欢迎参与任何形式的贡献。
