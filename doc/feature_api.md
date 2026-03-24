### formula

```ets
/*
* 通过文本参数生成数学公式图片数组数据
*
* 参数 - latexMathTextString 数学公式文本内容
* 参数 - latexMathTextSize 数学公式文字大小 - 单位px
* 参数 - latexMathTextColor 数学公式文字颜色
* 参数 - latexMathBackGroupColor 数学公式背景颜色
* 参数 - latexMathColorFormat 数学公式图片格式
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
* 通过文本参数生成数学公式图片数组数据（带错误信息）
* 当公式解析失败时，返回详细的错误信息，包括结果码和错误描述
*
* 参数 - latexMathTextString 数学公式文本内容
* 参数 - latexMathTextSize 数学公式文字大小 - 单位px
* 参数 - latexMathTextColor 数学公式文字颜色
* 参数 - latexMathBackGroupColor 数学公式背景颜色
* 参数 - latexMathColorFormat 数学公式图片格式
* 参数 - resPath 字体资源路径。 默认"/data/storage/el1/bundle/entry/resources/resfile/res"
*
* 返回值 - Promise<TeXParseResult> 解析结果对象
*          - 成功时：resultCode=Success, imageBytes包含图片数据
*          - 失败时：resultCode为错误码, imageBytes为空, errorMessage包含错误详情
*/
latexStringToImageWithError(latexMathTextString: string, latexMathTextSize: number, latexMathTextColor: number, latexMathBackGroupColor: number, latexMathColorFormat: LatexMathColorFormat, resPath?: string): Promise<TeXParseResult>
```

### 结果码说明

| 结果码 | 名称 | 说明 | 示例场景 |
|--------|------|------|----------|
| 0 | Success | 解析成功 | 正常的LaTeX公式 |
| 1 | SyntaxError | 语法错误 | 缺少参数、括号不匹配、命令拼写错误等 |
| 2 | InvalidMatrixError | 无效矩阵错误 | 矩阵列数不一致、矩阵格式错误等 |
| 3 | InvalidDelimiterError | 无效分隔符错误 | left/right不匹配、分隔符使用错误等 |
| 4 | TeXError | TeX错误 | 未定义的命令、不支持的LaTeX特性等 |
| 99 | UnknownError | 未知错误 | 其他未分类的错误 |

### 使用示例

```typescript
import { latexStringToImageWithError, TeXResultCode, LatexMathColorFormat } from '@cangjie-tpc/formula_hybrid';

// 示例1: 正常公式解析
async function parseNormalFormula() {
  const result = await latexStringToImageWithError(
    "\\frac{a}{b}",
    20,
    0xFF000000,
    0xFFFFFFFF,
    LatexMathColorFormat.COLOR_FORMAT_BGRA_8888
  );
  
  if (result.resultCode === TeXResultCode.Success) {
    console.log("解析成功");
    // 使用 result.imageBytes 创建图片
  } else {
    console.log("解析失败:", result.errorMessage);
  }
}

// 示例2: 异常公式处理
async function parseErrorFormula() {
  const result = await latexStringToImageWithError(
    "\\frac{a}{",  // 缺少右括号
    20,
    0xFF000000,
    0xFFFFFFFF,
    LatexMathColorFormat.COLOR_FORMAT_BGRA_8888
  );
  
  if (result.resultCode !== TeXResultCode.Success) {
    console.log("结果码:", result.resultCode); // TeXResultCode.SyntaxError
    console.log("错误信息:", result.errorMessage);
    console.log("原始公式:", result.formula);
  }
}

// 示例3: 根据结果码进行不同处理
async function handleFormulaWithResultCode(formula: string) {
  const result = await latexStringToImageWithError(
    formula,
    20,
    0xFF000000,
    0xFFFFFFFF,
    LatexMathColorFormat.COLOR_FORMAT_BGRA_8888
  );
  
  switch (result.resultCode) {
    case TeXResultCode.Success:
      console.log("公式解析成功");
      break;
    case TeXResultCode.SyntaxError:
      console.log("语法错误，请检查公式格式");
      break;
    case TeXResultCode.InvalidMatrixError:
      console.log("矩阵格式错误，请检查矩阵定义");
      break;
    case TeXResultCode.InvalidDelimiterError:
      console.log("分隔符错误，请检查left/right配对");
      break;
    case TeXResultCode.TeXError:
      console.log("TeX错误，可能使用了不支持的命令");
      break;
    case TeXResultCode.UnknownError:
      console.log("未知错误:", result.errorMessage);
      break;
  }
  
  return result;
}
```
