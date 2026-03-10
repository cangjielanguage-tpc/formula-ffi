import { LatexMathColorFormat, latexStringToImageWithError, TeXErrorCode, TeXParseResult } from '@cangjie-tpc/formula_hybrid';
import { image } from '@kit.ImageKit';

interface ErrorFormulaItem {
  formula: string;
  description: string;
  expectedErrorType: string;
}

interface FormulaTestResult {
  formula: string;
  description: string;
  expectedErrorType: string;
  isSuccess: boolean;
  errorCode: TeXErrorCode;
  errorMessage: string;
  pixelMap?: image.PixelMap;
  imageWidth?: number;
  imageHeight?: number;
}

const SUCCESS_COLOR: string = '#4CAF50';
const FAILURE_COLOR: string = '#FF9800';
const WARNING_COLOR: string = '#FFB74D';
const GRAY_COLOR: string = '#757575';
const SUCCESS_BG_COLOR: string = '#E8F5E9';
const FAILURE_BG_COLOR: string = '#FFEBEE';
const SUCCESS_BORDER_COLOR: string = '#C8E6C9';
const FAILURE_BORDER_COLOR: string = '#FFCCBC';

@Entry
@Component
struct Index13 {
  @State results: FormulaTestResult[] = [];
  @State currentTab: number = 0;

  normalFormulas: ErrorFormulaItem[] = [
    // 1. 分数命令 - 正常用例
    { formula: "\\frac{a}{b}", description: "分数: 标准分数", expectedErrorType: "Success" },
    { formula: "\\frac{x+1}{x-1}", description: "分数: 复杂表达式", expectedErrorType: "Success" },
    { formula: "\\frac{}{b}", description: "分数: 空分子", expectedErrorType: "Success" },
    { formula: "\\frac{a}{}", description: "分数: 空分母", expectedErrorType: "Success" },
    { formula: "\\frac{1}{2}", description: "分数: 数字分数", expectedErrorType: "Success" },
    
    // 2. 根号命令 - 正常用例
    { formula: "\\sqrt{x}", description: "根号: 平方根", expectedErrorType: "Success" },
    { formula: "\\sqrt[3]{x}", description: "根号: 立方根", expectedErrorType: "Success" },
    { formula: "\\sqrt[n]{x}", description: "根号: 变量指数", expectedErrorType: "Success" },
    { formula: "\\sqrt{}", description: "根号: 空内容", expectedErrorType: "Success" },
    { formula: "\\sqrt[]{x}", description: "根号: 空指数", expectedErrorType: "Success" },
    
    // 3. 环境匹配 - 正常用例
    { formula: "\\begin{matrix}a\\end{matrix}", description: "环境: matrix环境", expectedErrorType: "Success" },
    { formula: "\\begin{pmatrix}a\\end{pmatrix}", description: "环境: pmatrix环境", expectedErrorType: "Success" },
    { formula: "\\begin{bmatrix}a\\end{bmatrix}", description: "环境: bmatrix环境", expectedErrorType: "Success" },
    { formula: "\\begin{matrix}\\begin{matrix}a\\end{matrix}b\\end{matrix}", description: "环境: 嵌套环境", expectedErrorType: "Success" },
    { formula: "\\begin{cases}x & y\\end{cases}", description: "环境: cases环境", expectedErrorType: "Success" },
    
    // 4. left/right配对 - 正常用例
    { formula: "\\left( x \\right)", description: "分隔符: 圆括号", expectedErrorType: "Success" },
    { formula: "\\left[ x \\right]", description: "分隔符: 方括号", expectedErrorType: "Success" },
    { formula: "\\left\\{ x \\right\\}", description: "分隔符: 花括号", expectedErrorType: "Success" },
    { formula: "\\left. x \\right|", description: "分隔符: 不可见分隔符", expectedErrorType: "Success" },
    { formula: "\\left( x \\right]", description: "分隔符: 混合括号", expectedErrorType: "Success" },
    
    // 5. 上下标 - 正常用例
    { formula: "x_1", description: "上下标: 简单下标", expectedErrorType: "Success" },
    { formula: "x^2", description: "上下标: 简单上标", expectedErrorType: "Success" },
    { formula: "x_1^2", description: "上下标: 同时上下标", expectedErrorType: "Success" },
    { formula: "x_{n+1}^{2k}", description: "上下标: 复杂表达式", expectedErrorType: "Success" },
    { formula: "x_ 1", description: "上下标: 空格后下标", expectedErrorType: "Success" },
    
    // 6. 花括号匹配 - 正常用例
    { formula: "{a}", description: "花括号: 简单花括号", expectedErrorType: "Success" },
    { formula: "{a{b}c}", description: "花括号: 嵌套花括号", expectedErrorType: "Success" },
    { formula: "\\frac{a}{b}", description: "花括号: 命令中的花括号", expectedErrorType: "Success" },
    
    // 7. 对齐符 - 正常用例
    { formula: "\\begin{matrix}a & b\\end{matrix}", description: "对齐符: matrix中对齐", expectedErrorType: "Success" },
    { formula: "\\begin{cases}x & y\\end{cases}", description: "对齐符: cases中对齐", expectedErrorType: "Success" },
    { formula: "\\begin{aligned}a & b\\\\c & d\\end{aligned}", description: "对齐符: aligned中对齐", expectedErrorType: "Success" },
    
    // 8. 矩阵列数一致 - 正常用例
    { formula: "\\begin{matrix}a & b\\\\c & d\\end{matrix}", description: "矩阵: 2x2矩阵", expectedErrorType: "Success" },
    { formula: "\\begin{matrix}a & b & c\\\\d & e & f\\end{matrix}", description: "矩阵: 2x3矩阵", expectedErrorType: "Success" },
    { formula: "\\begin{gathered}a\\\\b\\end{gathered}", description: "矩阵: gathered环境", expectedErrorType: "Success" },
    
    // 9. 重音命令 - 正常用例
    { formula: "\\hat{x}", description: "重音: hat命令", expectedErrorType: "Success" },
    { formula: "\\overline{a+b}", description: "重音: overline命令", expectedErrorType: "Success" },
    { formula: "\\vec{}", description: "重音: 空参数", expectedErrorType: "Success" },
    { formula: "\\hat{=}", description: "重音: 特殊字符", expectedErrorType: "Success" },
    { formula: "\\tilde{x}", description: "重音: tilde命令", expectedErrorType: "Success" },
    
    // 10. cases环境 - 正常用例
    { formula: "\\begin{cases}x & y\\end{cases}", description: "cases: 单行", expectedErrorType: "Success" },
    { formula: "\\begin{cases}x & y\\\\a & b\\end{cases}", description: "cases: 多行", expectedErrorType: "Success" },
    
    // 综合测试
    { formula: "1 + 2", description: "基础运算: 加法", expectedErrorType: "Success" },
    { formula: "x = \\frac{-b \\pm \\sqrt{b^2-4ac}}{2a}", description: "综合: 求根公式", expectedErrorType: "Success" }
  ];

  errorFormulas: ErrorFormulaItem[] = [
    // 1. 分数命令 - 异常用例
    { formula: "\\frac{a}{", description: "分数: 缺少右花括号", expectedErrorType: "SyntaxError" },
    { formula: "\\frac{1}{2", description: "分数: 分母未闭合", expectedErrorType: "SyntaxError" },
    { formula: "\\frac a b", description: "分数: 缺少花括号", expectedErrorType: "SyntaxError" },
    { formula: "\\frac{a", description: "分数: 只有分子", expectedErrorType: "SyntaxError" },
    
    // 2. 根号命令 - 异常用例
    { formula: "\\sqrt", description: "根号: 缺少参数", expectedErrorType: "SyntaxError" },
    { formula: "\\sqrt{x", description: "根号: 未闭合花括号", expectedErrorType: "SyntaxError" },
    { formula: "\\sqrt[3{x}", description: "根号: 指数未闭合", expectedErrorType: "SyntaxError" },
    
    // 3. 环境匹配 - 异常用例
    { formula: "\\begin{matrix}a", description: "环境: 缺少end", expectedErrorType: "SyntaxError" },
    { formula: "\\begin{matrix}a\\end{pmatrix}", description: "环境: begin/end不匹配", expectedErrorType: "SyntaxError" },
    { formula: "\\end{matrix}", description: "环境: 缺少begin", expectedErrorType: "SyntaxError" },
    { formula: "\\begin{matrix}a\\begin{pmatrix}b\\end{matrix}", description: "环境: 嵌套不匹配", expectedErrorType: "SyntaxError" },
    
    // 4. left/right配对 - 异常用例
    { formula: "\\left( x", description: "分隔符: 缺少right", expectedErrorType: "SyntaxError" },
    { formula: "\\right)", description: "分隔符: 缺少left", expectedErrorType: "SyntaxError" },
    { formula: "\\left( x \\right", description: "分隔符: right后缺少分隔符", expectedErrorType: "SyntaxError" },
    { formula: "\\left x \\right)", description: "分隔符: left后缺少分隔符", expectedErrorType: "SyntaxError" },
    
    // 5. 上下标 - 异常用例
    { formula: "x_", description: "上下标: 下标无内容", expectedErrorType: "SyntaxError" },
    { formula: "x^", description: "上下标: 上标无内容", expectedErrorType: "SyntaxError" },
    { formula: "x_1_2", description: "上下标: 双重下标", expectedErrorType: "SyntaxError" },
    { formula: "x^2^3", description: "上下标: 双重上标", expectedErrorType: "SyntaxError" },
    { formula: "x_{", description: "上下标: 花括号未闭合", expectedErrorType: "SyntaxError" },
    
    // 6. 花括号匹配 - 异常用例
    { formula: "{a", description: "花括号: 缺少右括号", expectedErrorType: "SyntaxError" },
    { formula: "a}", description: "花括号: 缺少左括号", expectedErrorType: "SyntaxError" },
    { formula: "{a{b}", description: "花括号: 嵌套不匹配", expectedErrorType: "SyntaxError" },
    
    // 7. 对齐符 - 异常用例
    { formula: "a & b", description: "对齐符: 环境外使用", expectedErrorType: "SyntaxError" },
    { formula: "x_1 & x_2", description: "对齐符: 上下标环境中使用", expectedErrorType: "SyntaxError" },
    
    // 8. 矩阵列数一致 - 异常用例
    { formula: "\\begin{matrix}a & b\\\\c\\end{matrix}", description: "矩阵: 列数不一致", expectedErrorType: "SyntaxError" },
    { formula: "\\begin{matrix}a & b & c\\\\d & e\\end{matrix}", description: "矩阵: 行列数不匹配", expectedErrorType: "SyntaxError" },
    
    // 9. 重音命令 - 异常用例
    { formula: "\\hat", description: "重音: 缺少参数", expectedErrorType: "SyntaxError" },
    { formula: "\\overline", description: "重音: overline缺少参数", expectedErrorType: "SyntaxError" },
    { formula: "\\vec", description: "重音: vec缺少参数", expectedErrorType: "SyntaxError" },
    
    // 10. cases环境 - 异常用例
    { formula: "\\begin{cases}x & y & z\\end{cases}", description: "cases: 列数不为2", expectedErrorType: "SyntaxError" },
    { formula: "\\begin{cases}x\\end{cases}", description: "cases: 缺少第二列", expectedErrorType: "SyntaxError" },
    
    // 空公式
    { formula: "", description: "空公式", expectedErrorType: "SyntaxError" }
  ];

  async aboutToAppear(): Promise<void> {
    await this.testAllFormulas();
  }

  async testAllFormulas(): Promise<void> {
    this.results = [];
    
    const formulasToTest = this.currentTab === 0 ? this.normalFormulas : this.errorFormulas;
    
    for (let item of formulasToTest) {
      try {
        let result: TeXParseResult = await latexStringToImageWithError(
          item.formula,
          fp2px(20),
          0xFF000000,
          0xFFFFFFFF,
          LatexMathColorFormat.COLOR_FORMAT_BGRA_8888
        );

        if (result.isSuccess) {
          let imageSource = image.createImageSource(result.imageBytes);
          let pixelMap = imageSource.createPixelMapSync();
          let size: Size = pixelMap.getImageInfoSync().size;
          
          let testResult: FormulaTestResult = {
            formula: item.formula,
            description: item.description,
            expectedErrorType: item.expectedErrorType,
            isSuccess: true,
            errorCode: result.errorCode,
            errorMessage: result.errorMessage,
            pixelMap: pixelMap,
            imageWidth: px2vp(size.width),
            imageHeight: px2vp(size.height)
          };
          this.results.push(testResult);
        } else {
          let testResult: FormulaTestResult = {
            formula: item.formula,
            description: item.description,
            expectedErrorType: item.expectedErrorType,
            isSuccess: false,
            errorCode: result.errorCode,
            errorMessage: result.errorMessage
          };
          this.results.push(testResult);
        }
      } catch (e) {
        let testResult: FormulaTestResult = {
          formula: item.formula,
          description: item.description,
          expectedErrorType: item.expectedErrorType,
          isSuccess: false,
          errorCode: TeXErrorCode.UnknownError,
          errorMessage: e instanceof Error ? e.message : String(e)
        };
        this.results.push(testResult);
      }
    }
  }

  getErrorCodeName(code: TeXErrorCode): string {
    switch (code) {
      case TeXErrorCode.Success:
        return "Success";
      case TeXErrorCode.SyntaxError:
        return "SyntaxError";
      case TeXErrorCode.InvalidMatrixError:
        return "InvalidMatrixError";
      case TeXErrorCode.InvalidDelimiterError:
        return "InvalidDelimiterError";
      case TeXErrorCode.TeXError:
        return "TeXError";
      case TeXErrorCode.UnknownError:
        return "UnknownError";
      default:
        return "Unknown";
    }
  }

  @Builder
  resultItemBuilder(item: FormulaTestResult, index: number) {
    Column() {
      Row() {
        Text(`#${index + 1}`)
          .fontSize(14)
          .fontWeight(FontWeight.Bold)
          .width(30)
        
        Column() {
          Text(item.description)
            .fontSize(14)
            .fontColor(item.isSuccess ? SUCCESS_COLOR : FAILURE_COLOR)
            .fontWeight(FontWeight.Medium)
          
          Text(`公式: ${item.formula}`)
            .fontSize(12)
            .fontColor(GRAY_COLOR)
            .margin({ top: 2 })
          
          Text(`预期: ${item.expectedErrorType}`)
            .fontSize(11)
            .fontColor('#888888')
            .margin({ top: 1 })
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)
      }
      .width('100%')
      .alignItems(VerticalAlign.Top)

      if (item.isSuccess) {
        Row() {
          Text("解析结果: ")
            .fontSize(12)
            .fontColor(SUCCESS_COLOR)
          Text("成功")
            .fontSize(12)
            .fontColor(SUCCESS_COLOR)
            .fontWeight(FontWeight.Bold)
        }
        .margin({ top: 8 })

        if (item.pixelMap) {
          Image(item.pixelMap)
            .objectFit(ImageFit.Contain)
            .width(item.imageWidth)
            .height(item.imageHeight)
            .margin({ top: 8 })
            .border({ width: 1, color: GRAY_COLOR, radius: 4 })
        }
      } else {
        Column() {
          Row() {
            Text("解析结果: ")
              .fontSize(12)
              .fontColor(FAILURE_COLOR)
            Text("失败")
              .fontSize(12)
              .fontColor(FAILURE_COLOR)
              .fontWeight(FontWeight.Bold)
          }

          Row() {
            Text("错误码: ")
              .fontSize(12)
              .fontColor(WARNING_COLOR)
            Text(this.getErrorCodeName(item.errorCode))
              .fontSize(12)
              .fontColor(WARNING_COLOR)
              .fontWeight(FontWeight.Bold)
          }
          .margin({ top: 4 })

          Row() {
            Text("错误信息: ")
              .fontSize(12)
              .fontColor(WARNING_COLOR)
          }
          .margin({ top: 4 })

          Text(item.errorMessage)
            .fontSize(11)
            .fontColor(GRAY_COLOR)
            .margin({ top: 2 })
            .maxLines(3)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
        }
        .alignItems(HorizontalAlign.Start)
        .margin({ top: 8 })
      }
    }
    .width('95%')
    .padding(12)
    .margin({ bottom: 10 })
    .borderRadius(8)
    .backgroundColor(item.isSuccess ? SUCCESS_BG_COLOR : FAILURE_BG_COLOR)
    .border({ width: 1, color: item.isSuccess ? SUCCESS_BORDER_COLOR : FAILURE_BORDER_COLOR })
  }

  build() {
    Column() {
      Text("LaTeX 公式检测演示")
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .margin({ top: 10, bottom: 10 })

      Row() {
        Button("正常公式")
          .fontSize(14)
          .backgroundColor(this.currentTab === 0 ? SUCCESS_COLOR : '#E0E0E0')
          .fontColor(this.currentTab === 0 ? '#FFFFFF' : '#757575')
          .borderRadius(8)
          .margin({ right: 10 })
          .onClick(() => {
            this.currentTab = 0;
            this.testAllFormulas();
          })
        
        Button("异常公式")
          .fontSize(14)
          .backgroundColor(this.currentTab === 1 ? FAILURE_COLOR : '#E0E0E0')
          .fontColor(this.currentTab === 1 ? '#FFFFFF' : '#757575')
          .borderRadius(8)
          .onClick(() => {
            this.currentTab = 1;
            this.testAllFormulas();
          })
      }
      .justifyContent(FlexAlign.Center)
      .margin({ bottom: 10 })

      Text(`共 ${this.results.length} 个测试用例，${this.currentTab === 0 ? '正常' : '异常'}场景`)
        .fontSize(14)
        .fontColor(GRAY_COLOR)
        .margin({ bottom: 10 })

      Scroll() {
        Column() {
          ForEach(this.results, (item: FormulaTestResult, index: number) => {
            this.resultItemBuilder(item, index)
          })

          Blank().height(20)
        }
      }
      .layoutWeight(1)
      .scrollBar(BarState.Auto)
      .backgroundColor('#F5F5F5')
    }
    .height('100%')
    .width('100%')
  }
}