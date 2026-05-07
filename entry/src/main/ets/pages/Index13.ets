import { LatexMathColorFormat, latexStringToImageWithError, TeXResultCode, TeXParseResult } from '@cangjie-tpc/formula_hybrid';
import { image } from '@kit.ImageKit';
import { common } from '@kit.AbilityKit';
import { resourceManager } from '@kit.LocalizationKit';
import { util } from '@kit.ArkTS';

interface FormulaItem {
  formula: string;
  description: string;
  category: 'normal' | 'error' | 'tolerant';
}

interface FormulaTestResult {
  formula: string;
  description: string;
  category: string;
  resultCode: TeXResultCode;
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
  private context = getContext(this) as common.UIAbilityContext;
  @State results: FormulaTestResult[] = [];
  @State currentTab: number = 0;
  @State rawfileFormulas: FormulaItem[] = [];

  formulas: FormulaItem[] = [
    // ==================== 正常用例 (normal) ====================
    // 基础运算 (Basic)
    { formula: "1 + 2", description: "基础运算: 简单加法", category: "normal" },
    { formula: "a - b", description: "基础运算: 简单减法", category: "normal" },
    { formula: "x \\times y", description: "基础运算: 乘法", category: "normal" },
    { formula: "a \\div b", description: "基础运算: 除法", category: "normal" },
    { formula: "123", description: "基础运算: 纯数字", category: "normal" },
    { formula: "3.14159", description: "基础运算: 小数", category: "normal" },
    { formula: "-5", description: "基础运算: 负数", category: "normal" },
    { formula: "+10", description: "基础运算: 正号", category: "normal" },
    { formula: "a + b + c + d", description: "基础运算: 多重加法", category: "normal" },
    { formula: "1 + 2 + 3 + 4 + 5", description: "基础运算: 长表达式", category: "normal" },
    
    // 分数命令 (Fraction)
    { formula: "\\frac{1}{2}", description: "分数: 简单分数", category: "normal" },
    { formula: "\\frac{a}{b}", description: "分数: 变量分数", category: "normal" },
    { formula: "\\frac{1}{2} + \\frac{3}{4}", description: "分数: 分数加法", category: "normal" },
    { formula: "\\frac{\\frac{1}{2}}{3}", description: "分数: 嵌套分数", category: "normal" },
    { formula: "\\frac{1}{\\frac{2}{3}}", description: "分数: 分母嵌套分数", category: "normal" },
    { formula: "\\frac{\\frac{a}{b}}{\\frac{c}{d}}", description: "分数: 双重嵌套分数", category: "normal" },
    { formula: "\\dfrac{1}{2}", description: "分数: display分数", category: "normal" },
    { formula: "\\tfrac{1}{2}", description: "分数: text分数", category: "normal" },
    { formula: "\\cfrac{1}{1+\\cfrac{1}{1+\\cfrac{1}{1}}}", description: "连分数: 基本连分数", category: "normal" },
    { formula: "\\frac{1+2}{3+4}", description: "分数: 表达式分数", category: "normal" },
    { formula: "\\frac{x^2}{y^2}", description: "分数: 幂分数", category: "normal" },
    { formula: "\\frac{\\sqrt{x}}{\\sqrt{y}}", description: "分数: 根号分数", category: "normal" },
    { formula: "\\frac{}{b}", description: "分数: 空分子(标准LaTeX)", category: "normal" },
    { formula: "\\frac{a}{}", description: "分数: 空分母(标准LaTeX)", category: "normal" },
    { formula: "\\frac{}{}", description: "分数: 双空(标准LaTeX)", category: "normal" },
    { formula: "\\binom{n}{k}", description: "二项式: 基本二项式", category: "normal" },
    { formula: "{a \\over b}", description: "分数线: 旧语法", category: "normal" },
    
    // 根号命令 (Root)
    { formula: "\\sqrt{x}", description: "根号: 简单平方根", category: "normal" },
    { formula: "\\sqrt{2}", description: "根号: 数字平方根", category: "normal" },
    { formula: "\\sqrt[3]{x}", description: "根号: 立方根", category: "normal" },
    { formula: "\\sqrt[n]{x}", description: "根号: n次根", category: "normal" },
    { formula: "\\sqrt{x^2 + y^2}", description: "根号: 根号内表达式", category: "normal" },
    { formula: "\\sqrt{\\frac{a}{b}}", description: "根号: 根号内分数", category: "normal" },
    { formula: "\\sqrt{\\sqrt{x}}", description: "根号: 嵌套平方根", category: "normal" },
    { formula: "\\sqrt[3]{\\sqrt[2]{x}}", description: "根号: 混合嵌套根号", category: "normal" },
    { formula: "\\sqrt{}", description: "根号: 空根号(标准LaTeX)", category: "normal" },
    { formula: "\\sqrt[]{x}", description: "根号: 空索引(标准LaTeX)", category: "normal" },
    
    // 上下标 (SubSup)
    { formula: "x_1", description: "上下标: 简单下标", category: "normal" },
    { formula: "x^2", description: "上下标: 简单上标", category: "normal" },
    { formula: "x_1^2", description: "上下标: 下标和上标", category: "normal" },
    { formula: "x^2_1", description: "上下标: 反序", category: "normal" },
    { formula: "x_{i,j}", description: "上下标: 多重下标", category: "normal" },
    { formula: "x^{n+1}", description: "上下标: 表达式上标", category: "normal" },
    { formula: "x_{n+1}^{2k}", description: "上下标: 复杂上下标", category: "normal" },
    { formula: "a_{b_{c}}", description: "上下标: 嵌套下标", category: "normal" },
    { formula: "a^{b^{c}}", description: "上下标: 嵌套上标", category: "normal" },
    { formula: "a_{b_{c_{d}}}", description: "上下标: 三重嵌套下标", category: "normal" },
    { formula: "_1", description: "上下标: 无基底下标", category: "normal" },
    { formula: "^2", description: "上下标: 无基底上标", category: "normal" },
    
    // 希腊字母 (Greek)
    { formula: "\\alpha", description: "希腊字母: 小写alpha", category: "normal" },
    { formula: "\\beta", description: "希腊字母: 小写beta", category: "normal" },
    { formula: "\\gamma", description: "希腊字母: 小写gamma", category: "normal" },
    { formula: "\\delta", description: "希腊字母: 小写delta", category: "normal" },
    { formula: "\\epsilon", description: "希腊字母: 小写epsilon", category: "normal" },
    { formula: "\\zeta", description: "希腊字母: 小写zeta", category: "normal" },
    { formula: "\\eta", description: "希腊字母: 小写eta", category: "normal" },
    { formula: "\\theta", description: "希腊字母: 小写theta", category: "normal" },
    { formula: "\\iota", description: "希腊字母: 小写iota", category: "normal" },
    { formula: "\\kappa", description: "希腊字母: 小写kappa", category: "normal" },
    { formula: "\\lambda", description: "希腊字母: 小写lambda", category: "normal" },
    { formula: "\\mu", description: "希腊字母: 小写mu", category: "normal" },
    { formula: "\\nu", description: "希腊字母: 小写nu", category: "normal" },
    { formula: "\\xi", description: "希腊字母: 小写xi", category: "normal" },
    { formula: "\\pi", description: "希腊字母: 小写pi", category: "normal" },
    { formula: "\\rho", description: "希腊字母: 小写rho", category: "normal" },
    { formula: "\\sigma", description: "希腊字母: 小写sigma", category: "normal" },
    { formula: "\\tau", description: "希腊字母: 小写tau", category: "normal" },
    { formula: "\\upsilon", description: "希腊字母: 小写upsilon", category: "normal" },
    { formula: "\\phi", description: "希腊字母: 小写phi", category: "normal" },
    { formula: "\\chi", description: "希腊字母: 小写chi", category: "normal" },
    { formula: "\\psi", description: "希腊字母: 小写psi", category: "normal" },
    { formula: "\\omega", description: "希腊字母: 小写omega", category: "normal" },
    { formula: "\\Gamma", description: "希腊字母: 大写Gamma", category: "normal" },
    { formula: "\\Delta", description: "希腊字母: 大写Delta", category: "normal" },
    { formula: "\\Theta", description: "希腊字母: 大写Theta", category: "normal" },
    { formula: "\\Lambda", description: "希腊字母: 大写Lambda", category: "normal" },
    { formula: "\\Xi", description: "希腊字母: 大写Xi", category: "normal" },
    { formula: "\\Pi", description: "希腊字母: 大写Pi", category: "normal" },
    { formula: "\\Sigma", description: "希腊字母: 大写Sigma", category: "normal" },
    { formula: "\\Upsilon", description: "希腊字母: 大写Upsilon", category: "normal" },
    { formula: "\\Phi", description: "希腊字母: 大写Phi", category: "normal" },
    { formula: "\\Psi", description: "希腊字母: 大写Psi", category: "normal" },
    { formula: "\\Omega", description: "希腊字母: 大写Omega", category: "normal" },
    
    // 数学运算符 (Operators)
    { formula: "\\pm", description: "运算符: 正负号", category: "normal" },
    { formula: "\\mp", description: "运算符: 负正号", category: "normal" },
    { formula: "\\times", description: "运算符: 乘号", category: "normal" },
    { formula: "\\div", description: "运算符: 除号", category: "normal" },
    { formula: "\\cdot", description: "运算符: 点乘", category: "normal" },
    { formula: "\\ast", description: "运算符: 星号", category: "normal" },
    { formula: "\\star", description: "运算符: 五角星", category: "normal" },
    { formula: "\\circ", description: "运算符: 圆圈", category: "normal" },
    { formula: "\\bullet", description: "运算符: 实心圆", category: "normal" },
    { formula: "\\oplus", description: "运算符: 圈加", category: "normal" },
    { formula: "\\ominus", description: "运算符: 圈减", category: "normal" },
    { formula: "\\otimes", description: "运算符: 圈乘", category: "normal" },
    { formula: "\\oslash", description: "运算符: 圈除", category: "normal" },
    { formula: "\\odot", description: "运算符: 圈点", category: "normal" },
    { formula: "\\dagger", description: "运算符: 匕首", category: "normal" },
    { formula: "\\ddagger", description: "运算符: 双匕首", category: "normal" },
    { formula: "\\cap", description: "运算符: 交集", category: "normal" },
    { formula: "\\cup", description: "运算符: 并集", category: "normal" },
    { formula: "\\vee", description: "运算符: 逻辑或", category: "normal" },
    { formula: "\\wedge", description: "运算符: 逻辑与", category: "normal" },
    { formula: "\\setminus", description: "运算符: 集合差", category: "normal" },
    { formula: "\\diamond", description: "运算符: 菱形", category: "normal" },
    { formula: "\\triangle", description: "运算符: 三角形", category: "normal" },
    { formula: "\\bigtriangleup", description: "运算符: 大三角形向上", category: "normal" },
    { formula: "\\bigtriangledown", description: "运算符: 大三角形向下", category: "normal" },
    { formula: "\\triangleleft", description: "运算符: 左三角", category: "normal" },
    { formula: "\\triangleright", description: "运算符: 右三角", category: "normal" },
    { formula: "\\iff", description: "运算符: 当且仅当", category: "normal" },
    
    // 大型运算符 (LargeOps)
    { formula: "\\sum x", description: "大型运算符: 简单求和", category: "normal" },
    { formula: "\\sum_{i=1}^n x_i", description: "大型运算符: 带限求和", category: "normal" },
    { formula: "\\sum_{i=1}^{\\infty} \\frac{1}{i^2}", description: "大型运算符: 无穷级数", category: "normal" },
    { formula: "\\prod_{i=1}^n x_i", description: "大型运算符: 连乘", category: "normal" },
    { formula: "\\int_0^\\infty f(x) dx", description: "大型运算符: 基本积分", category: "normal" },
    { formula: "\\iint_D f(x,y) dA", description: "大型运算符: 二重积分", category: "normal" },
    { formula: "\\iiint_V f(x,y,z) dV", description: "大型运算符: 三重积分", category: "normal" },
    { formula: "\\oint_C f(z) dz", description: "大型运算符: 环路积分", category: "normal" },
    { formula: "\\lim_{x \\to 0} f(x)", description: "大型运算符: 基本极限", category: "normal" },
    { formula: "\\lim_{n \\to \\infty} a_n", description: "大型运算符: 无穷极限", category: "normal" },
    { formula: "\\bigcup_{i=1}^n A_i", description: "大型运算符: 大并集", category: "normal" },
    { formula: "\\bigcap_{i=1}^n A_i", description: "大型运算符: 大交集", category: "normal" },
    { formula: "\\bigoplus_{i=1}^n V_i", description: "大型运算符: 直和", category: "normal" },
    { formula: "\\bigotimes_{i=1}^n V_i", description: "大型运算符: 张量积", category: "normal" },
    
    // 矩阵与表格 (Matrix)
    { formula: "\\begin{matrix}a & b \\\\ c & d\\end{matrix}", description: "矩阵: matrix无括号", category: "normal" },
    { formula: "\\begin{pmatrix}a & b \\\\ c & d\\end{pmatrix}", description: "矩阵: pmatrix圆括号", category: "normal" },
    { formula: "\\begin{bmatrix}a & b \\\\ c & d\\end{bmatrix}", description: "矩阵: bmatrix方括号", category: "normal" },
    { formula: "\\begin{vmatrix}a & b \\\\ c & d\\end{vmatrix}", description: "矩阵: vmatrix行列式", category: "normal" },
    { formula: "\\begin{Vmatrix}a & b \\\\ c & d\\end{Vmatrix}", description: "矩阵: Vmatrix范数", category: "normal" },
    { formula: "\\begin{array}{cc}a & b \\\\ c & d\\end{array}", description: "数组: 基本数组", category: "normal" },
    { formula: "\\begin{array}{lcr}a & b & c\\end{array}", description: "数组: 对齐格式", category: "normal" },
    { formula: "\\begin{matrix}1 & 2 & 3 \\\\ 4 & 5 & 6 \\\\ 7 & 8 & 9\\end{matrix}", description: "矩阵: 3x3矩阵", category: "normal" },
    
    // Cases和对齐环境
    { formula: "\\begin{cases}x & y\\end{cases}", description: "cases: 基本cases", category: "normal" },
    { formula: "f(x) = \\begin{cases}1 & x > 0 \\\\ 0 & x = 0 \\\\ -1 & x < 0\\end{cases}", description: "cases: 符号函数", category: "normal" },
    { formula: "\\begin{cases}x > 0 & \\text{if } a > 0 \\\\ x < 0 & \\text{if } a < 0\\end{cases}", description: "cases: 带条件cases", category: "normal" },
    { formula: "\\begin{aligned}a &= b + c \\\\ &= d + e + f\\end{aligned}", description: "aligned: 对齐环境", category: "normal" },
    { formula: "\\begin{split}a &= b \\\\ &= c\\end{split}", description: "split: 分割环境", category: "normal" },
    
    // 分隔符 (Delimiters)
    { formula: "(a)", description: "分隔符: 圆括号", category: "normal" },
    { formula: "[a]", description: "分隔符: 方括号", category: "normal" },
    { formula: "\\{a\\}", description: "分隔符: 花括号", category: "normal" },
    { formula: "\\left( a \\right)", description: "分隔符: 自适应圆括号", category: "normal" },
    { formula: "\\left[ a \\right]", description: "分隔符: 自适应方括号", category: "normal" },
    { formula: "\\left\\{ a \\right\\}", description: "分隔符: 自适应花括号", category: "normal" },
    { formula: "\\left| x \\right|", description: "分隔符: 绝对值", category: "normal" },
    { formula: "\\left\\| x \\right\\|", description: "分隔符: 范数", category: "normal" },
    { formula: "\\left\\langle a \\right\\rangle", description: "分隔符: 尖括号", category: "normal" },
    { formula: "\\left( \\frac{a}{b} \\right)", description: "分隔符: 自适应括号分数", category: "normal" },
    { formula: "\\left( \\left( \\left( a \\right) \\right) \\right)", description: "分隔符: 三重嵌套", category: "normal" },
    { formula: "\\left( a \\right]", description: "分隔符: 混合分隔符", category: "normal" },
    { formula: "\\left. a \\right|", description: "分隔符: 不可见分隔符", category: "normal" },
    
    // 重音符号 (Accent)
    { formula: "\\hat{x}", description: "重音: hat", category: "normal" },
    { formula: "\\widehat{xyz}", description: "重音: 宽hat", category: "normal" },
    { formula: "\\tilde{x}", description: "重音: tilde", category: "normal" },
    { formula: "\\widetilde{xyz}", description: "重音: 宽tilde", category: "normal" },
    { formula: "\\vec{v}", description: "重音: vec向量", category: "normal" },
    { formula: "\\dot{x}", description: "重音: dot导数", category: "normal" },
    { formula: "\\ddot{x}", description: "重音: ddot二阶导数", category: "normal" },
    { formula: "\\bar{a}", description: "重音: bar横线", category: "normal" },
    { formula: "\\overline{a}", description: "上划线: 单字符", category: "normal" },
    { formula: "\\underline{x}", description: "下划线: 单字符", category: "normal" },
    { formula: "\\overbrace{a+b+c}", description: "上花括号: 基本用法", category: "normal" },
    { formula: "\\underbrace{x_1+x_2+\\cdots+x_n}", description: "下花括号: 基本用法", category: "normal" },
    { formula: "\\overrightarrow{AB}", description: "向量箭头: 向右", category: "normal" },
    { formula: "\\overleftarrow{AB}", description: "向量箭头: 向左", category: "normal" },
    { formula: "\\grave{x}", description: "重音: 重音符", category: "normal" },
    { formula: "\\acute{x}", description: "重音: 锐音符", category: "normal" },
    { formula: "\\breve{x}", description: "重音: 短音符", category: "normal" },
    { formula: "\\check{x}", description: "重音: 检查符", category: "normal" },
    
    // 上下标与堆叠
    { formula: "\\stackrel{def}{=}", description: "堆叠: 定义符号", category: "normal" },
    { formula: "\\overset{*}{X}", description: "上方堆叠: 星号标记", category: "normal" },
    { formula: "\\underset{n}{\\sum}", description: "下方堆叠: 求和符号", category: "normal" },
    { formula: "\\sideset{^a_b}{^c_d}{\\sum}", description: "侧边设置: 双侧标记", category: "normal" },
    { formula: "\\prescript{a}{b}{X}", description: "前置上下标: 双标记", category: "normal" },
    { formula: "\\sum\\limits_{i=1}^n x_i", description: "limits: 求和上下标", category: "normal" },
    
    // 文本与字体
    { formula: "\\text{for all}", description: "文本: 基本文本", category: "normal" },
    { formula: "\\mathbf{A}", description: "数学粗体: 向量", category: "normal" },
    { formula: "\\mathbb{R}", description: "黑板粗体: 实数集", category: "normal" },
    { formula: "\\boldsymbol{\\alpha}", description: "粗体符号: 希腊字母", category: "normal" },
    { formula: "\\mathrm{sin}", description: "正体: 正弦", category: "normal" },
    { formula: "\\mathit{x}", description: "斜体: 变量", category: "normal" },
    { formula: "\\mathcal{L}", description: "花体: 拉普拉斯算子", category: "normal" },
    { formula: "\\mathscr{F}", description: "手写体: 傅里叶变换", category: "normal" },
    { formula: "\\mathfrak{g}", description: "哥特体: 李代数", category: "normal" },
    
    // 盒子与框架
    { formula: "\\boxed{a}", description: "方框: 基本方框", category: "normal" },
    { formula: "\\boxed{x^2 + y^2 = r^2}", description: "方框: 公式方框", category: "normal" },
    { formula: "\\fbox{text}", description: "方框: 文本方框", category: "normal" },
    
    // 箭头扩展
    { formula: "A \\xrightarrow{f} B", description: "箭头: 带文字箭头", category: "normal" },
    { formula: "A \\xleftarrow{f} B", description: "箭头: 带文字左箭头", category: "normal" },
    { formula: "\\to", description: "箭头: 右箭头", category: "normal" },
    { formula: "\\gets", description: "箭头: 左箭头", category: "normal" },
    { formula: "\\Rightarrow", description: "箭头: 双线右箭头", category: "normal" },
    { formula: "\\Leftarrow", description: "箭头: 双线左箭头", category: "normal" },
    { formula: "\\leftrightarrow", description: "箭头: 双向箭头", category: "normal" },
    { formula: "\\Leftrightarrow", description: "箭头: 双线双向箭头", category: "normal" },
    { formula: "\\mapsto", description: "箭头: 映射箭头", category: "normal" },
    { formula: "\\hookrightarrow", description: "箭头: 钩形箭头", category: "normal" },
    { formula: "\\nearrow", description: "箭头: 东北箭头", category: "normal" },
    { formula: "\\searrow", description: "箭头: 东南箭头", category: "normal" },
    { formula: "\\swarrow", description: "箭头: 西南箭头", category: "normal" },
    { formula: "\\nwarrow", description: "箭头: 西北箭头", category: "normal" },
    
    // 函数 (Functions)
    { formula: "\\sin x", description: "函数: 正弦", category: "normal" },
    { formula: "\\cos x", description: "函数: 余弦", category: "normal" },
    { formula: "\\tan x", description: "函数: 正切", category: "normal" },
    { formula: "\\cot x", description: "函数: 余切", category: "normal" },
    { formula: "\\sec x", description: "函数: 正割", category: "normal" },
    { formula: "\\csc x", description: "函数: 余割", category: "normal" },
    { formula: "\\arcsin x", description: "函数: 反正弦", category: "normal" },
    { formula: "\\arccos x", description: "函数: 反余弦", category: "normal" },
    { formula: "\\arctan x", description: "函数: 反正切", category: "normal" },
    { formula: "\\sinh x", description: "函数: 双曲正弦", category: "normal" },
    { formula: "\\cosh x", description: "函数: 双曲余弦", category: "normal" },
    { formula: "\\tanh x", description: "函数: 双曲正切", category: "normal" },
    { formula: "\\log x", description: "函数: 对数", category: "normal" },
    { formula: "\\ln x", description: "函数: 自然对数", category: "normal" },
    { formula: "\\exp x", description: "函数: 指数", category: "normal" },
    { formula: "\\min_{x \\in S} f(x)", description: "函数: 最小值", category: "normal" },
    { formula: "\\max_{x \\in S} f(x)", description: "函数: 最大值", category: "normal" },
    { formula: "\\sup_{x \\in S} f(x)", description: "函数: 上确界", category: "normal" },
    { formula: "\\inf_{x \\in S} f(x)", description: "函数: 下确界", category: "normal" },
    { formula: "\\det A", description: "函数: 行列式", category: "normal" },
    { formula: "\\dim V", description: "函数: 维数", category: "normal" },
    { formula: "\\ker T", description: "函数: 核", category: "normal" },
    { formula: "\\gcd(a,b)", description: "函数: 最大公约数", category: "normal" },
    { formula: "\\hom(A,B)", description: "函数: 同态", category: "normal" },
    
    // 宏定义
    { formula: "\\newcommand{\\diff}{\\mathrm{d}}\\int_0^\\infty e^{-x} \\diff x", description: "宏定义: 无参数命令", category: "normal" },
    
    // ==================== Rawfile测试公式 ====================
    // 基础数学表达式 (test00-test05)
    { formula: "x^2 + y^2 = z^2", description: "Rawfile: 勾股定理", category: "normal" },
    { formula: "E = mc^2", description: "Rawfile: 质能方程", category: "normal" },
    { formula: "a^2 + b^2 = c^2", description: "Rawfile: 勾股定理基本形式", category: "normal" },
    { formula: "\\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}", description: "Rawfile: 求根公式", category: "normal" },
    { formula: "\\int_a^b f(x) \\, dx = F(b) - F(a)", description: "Rawfile: 微积分基本定理", category: "normal" },
    { formula: "\\sum_{i=1}^{n} i = \\frac{n(n+1)}{2}", description: "Rawfile: 等差数列求和", category: "normal" },
    
    // 三角函数与几何 (test06-test09)
    { formula: "\\sin^2 \\theta + \\cos^2 \\theta = 1", description: "Rawfile: 三角恒等式", category: "normal" },
    { formula: "\\cos 2\\theta = \\cos^2 \\theta - \\sin^2 \\theta", description: "Rawfile: 二倍角公式", category: "normal" },
    { formula: "\\tan \\theta = \\frac{\\sin \\theta}{\\cos \\theta}", description: "Rawfile: 正切定义", category: "normal" },
    { formula: "e^{i\\pi} + 1 = 0", description: "Rawfile: 欧拉恒等式", category: "normal" },
    
    // 复杂公式 (test10-test19)
    { formula: "\\lim_{x \\to 0} \\frac{\\sin x}{x} = 1", description: "Rawfile: 重要极限", category: "normal" },
    { formula: "\\int e^x \\, dx = e^x + C", description: "Rawfile: 指数积分", category: "normal" },
    { formula: "\\frac{d}{dx} e^x = e^x", description: "Rawfile: 指数导数", category: "normal" },
    { formula: "\\ln(e^x) = x", description: "Rawfile: 对数恒等式", category: "normal" },
    { formula: "a^m \\cdot a^n = a^{m+n}", description: "Rawfile: 指数法则", category: "normal" },
    { formula: "(a^m)^n = a^{mn}", description: "Rawfile: 幂的幂", category: "normal" },
    { formula: "\\sqrt[n]{a^m} = a^{m/n}", description: "Rawfile: 根式与指数", category: "normal" },
    { formula: "\\log_a (xy) = \\log_a x + \\log_a y", description: "Rawfile: 对数乘法", category: "normal" },
    { formula: "\\log_a \\frac{x}{y} = \\log_a x - \\log_a y", description: "Rawfile: 对数除法", category: "normal" },
    { formula: "\\log_a x^n = n \\log_a x", description: "Rawfile: 对数幂", category: "normal" },
    
    // 矩阵与线性代数 (test20-test29)
    { formula: "(x + y)^2 = x^2 + 2xy + y^2 \\tag{2}", description: "Rawfile: 完全平方公式带标签", category: "normal" },
    { formula: "\\begin{aligned}\\frac{1}{n}\\times \\sum_{x \\in list}(x^2 - 2\\times x\\times mean_{list}+mean_{list}^2) \\\\& = \\frac{1}{n}\\times (\\sum_{x \\in list}x^2 - \\sum_{x \\in list}2\\times x\\times mean_{list}+\\sum_{x \\in list}mean_{list}^2) \\\\& = \\frac{1}{n}\\times (\\sum_{x \\in list}x^2 - 2\\times sum_{list}\\times mean_{list}+mean_{list}^2\\times n) \\\\& = \\frac{1}{n}\\times (\\sum_{x \\in list}x^2 - \\frac{2 \\times sum_{list}^2}{n}+\\frac{sum_{list}^2}{n}) \\\\& = \\frac{1}{n}\\times (\\sum_{x \\in list}x^2 - \\frac{sum_{list}^2}{n})\\end{aligned}", description: "Rawfile: 方差推导", category: "normal" },
    { formula: "\\begin{matrix} 1 & 2 \\\\ 3 & 4 \\end{matrix}", description: "Rawfile: 基本矩阵", category: "normal" },
    { formula: "\\begin{pmatrix} 1 & 2 \\\\ 3 & 4 \\end{pmatrix}", description: "Rawfile: 圆括号矩阵", category: "normal" },
    { formula: "\\begin{bmatrix} 1 & 2 \\\\ 3 & 4 \\end{bmatrix}", description: "Rawfile: 方括号矩阵", category: "normal" },
    { formula: "\\vec{v}", description: "Rawfile: 向量符号", category: "normal" },
    { formula: "\\mathbf{F}", description: "Rawfile: 粗体向量", category: "normal" },
    { formula: "\\sum_{i=1}^{n} x_i", description: "Rawfile: 求和符号", category: "normal" },
    { formula: "\\int_{a}^{b} f(x) \\, dx", description: "Rawfile: 定积分", category: "normal" },
    { formula: "\\lim_{x \\to \\infty} f(x)", description: "Rawfile: 极限", category: "normal" },
    
    // 物理公式 (test30-test39)
    { formula: "\\begin{array}{l}{\\huge \\unicode{8751}}_\\mathbb{S}  \\mathbf{E} \\cdot\\mathrm{d}s= \\cfrac{Q}{\\varepsilon_0}  \\\\{\\huge \\unicode{8751}}_\\mathbb{S}  \\mathbf{B} \\cdot\\mathrm{d}s= 0 \\\\{\\huge \\oint}_{\\mathbb{L}}^{} \\mathbf{E} \\cdot \\mathrm{d}l=-\\cfrac{\\mathrm{d}\\Phi _{\\mathbf{B}}}{\\mathrm{d}t }  \\\\{\\huge \\oint}_{\\mathbb{L}}^{} \\mathbf{B} \\cdot \\mathrm{d}l=\\mu_0I+ \\mu_0 \\varepsilon_0\\cfrac{\\mathrm{d}\\Phi _{\\mathbf{E}}}{\\mathrm{d}t }\\end{array}", description: "Rawfile: 麦克斯韦方程组", category: "normal" },
    { formula: "\\begin{pmatrix}a_{11} & \\cdots & a_{1n} \\\\\\vdots & \\ddots & \\vdots \\\\a_{m1} & \\cdots & a_{mn}\\end{pmatrix}", description: "Rawfile: 一般矩阵形式", category: "normal" },
    { formula: "\\begin{array}{c}H_{n}=\\frac{n}{\\sum \\limits_{i=1}^{n}\\frac{1}{x_{i}}}= \\frac{n}{\\frac{1}{x_{1}}+ \\frac{1}{x_{2}}+ \\cdots + \\frac{1}{x_{n}}} \\\\ G_{n}=\\sqrt[n]{\\prod \\limits_{i=1}^{n}x_{i}}= \\sqrt[n]{x_{1}x_{2}\\cdots x_{n}} \\\\ A_{n}=\\frac{1}{n}\\sum \\limits_{i=1}^{n}x_{i}=\\frac{x_{1}+ x_{2}+ \\cdots + x_{n}}{n} \\\\ Q_{n}=\\sqrt{\\sum \\limits_{i=1}^{n}x_{i}^{2}}= \\sqrt{\\frac{x_{1}^{2}+ x_{2}^{2}+ \\cdots + x_{n}^{2}}{n}} \\\\ H_{n}\\leq G_{n}\\leq A_{n}\\leq Q_{n}\\end{array}", description: "Rawfile: 平均值不等式", category: "normal" },
    { formula: "F(\\omega) = \\int_{-\\infty}^{\\infty} f(t) e^{-i\\omega t} \\, dt", description: "Rawfile: 傅里叶变换", category: "normal" },
    { formula: "S = \\frac{100 \\times (100 + 1)}{2} = 5050", description: "Rawfile: 高斯求和", category: "normal" },
    { formula: "S = \\frac{n \\times (n+1)}{2}", description: "Rawfile: 等差数列求和公式", category: "normal" },
    { formula: "S = \\frac{(a_1 + a_n) \\times n}{2}", description: "Rawfile: 等差数列求和通式", category: "normal" },
    { formula: "i\\hbar \\frac{\\partial}{\\partial t} \\Psi(\\mathbf{r}, t) = \\hat{H} \\Psi(\\mathbf{r}, t)", description: "Rawfile: 薛定谔方程", category: "normal" },
    
    // 分数练习 (test42-test47)
    { formula: "\\frac{1}{4}", description: "Rawfile: 分数1/4", category: "normal" },
    { formula: "\\frac{2}{9}", description: "Rawfile: 分数2/9", category: "normal" },
    { formula: "\\frac{3}{16}", description: "Rawfile: 分数3/16", category: "normal" },
    { formula: "\\frac{6}{49}", description: "Rawfile: 分数6/49", category: "normal" },
    { formula: "\\frac{5}{36}", description: "Rawfile: 分数5/36", category: "normal" },
    { formula: "\\frac{4}{25}", description: "Rawfile: 分数4/25", category: "normal" },
    { formula: "n", description: "Rawfile: 单变量n", category: "normal" },
    
    // 化学方程式 (test35, test49-test60, test66-test69, test78-test90)
    { formula: "\\ce{SO4^2- + Ba^2+ -> BaSO4 v}", description: "Rawfile: 化学沉淀反应", category: "normal" },
    { formula: "\\begin{aligned}\\ce{2NO + O_2 &-> 2NO_2} \\\\\\ce{3NO_2 + H_2O &-> 2HNO_3 + NO}\\end{aligned}", description: "Rawfile: 氮氧化物反应", category: "normal" },
    { formula: "\\ce{4NO + 3O_2 + 2H_2O -> 4HNO_3}", description: "Rawfile: 硝酸生成反应", category: "normal" },
    { formula: "\\begin{aligned}\\ce{Na2S + S &<=> Na2S2} \\\\\\ce{Na2S + 3S &<=> Na2S4}\\end{aligned}", description: "Rawfile: 多硫化物生成", category: "normal" },
    { formula: "\\boxed{\\ce{Na2S + (n-1) S -> Na2S_{n}}}", description: "Rawfile: 多硫化物通式", category: "normal" },
    { formula: "\\ce{Na2S -> 2Na+ + S^{2-}}", description: "Rawfile: 硫化钠电离", category: "normal" },
    { formula: "\\ce{S8 (环) -> 2S4^{\\cdot\\cdot} (双自由基)}", description: "Rawfile: 硫环裂解", category: "normal" },
    { formula: "\\ce{S^{2-} + S8 -> S9^{2-}}", description: "Rawfile: 多硫离子生成", category: "normal" },
    { formula: "\\ce{S9^{2-} <=> S4^{2-} + S5}", description: "Rawfile: 多硫离子平衡", category: "normal" },
    { formula: "\\ce{S^{2-} + H2O <=> HS^- + OH^-}", description: "Rawfile: 硫离子水解", category: "normal" },
    { formula: "\\ce{HS_{n}^-}", description: "Rawfile: 硫氢化物离子", category: "normal" },
    { formula: "\\ce{S_{n}^{2-}}", description: "Rawfile: 多硫离子", category: "normal" },
    
    // 三角计算 (test60)
    { formula: "\\begin{align*}\\\\tan \\theta &= \\frac{1}{2 - \\sqrt{3}} \\cdot \\frac{2 + \\sqrt{3}}{2 + \\sqrt{3}} \\\\\\&= \\frac{2 + \\sqrt{3}}{(2 - \\sqrt{3})(2 + \\sqrt{3})} \\\\\\end{align*}", description: "Rawfile: 三角函数化简", category: "normal" },
    
    // 动态规划与算法 (test61-test65)
    { formula: "dp[i][j][k] =\\begin{cases}s1[i] == s2[j] ?,  \\text{k = 0}  \\\\\\\\\\\\dp[i][j][k] \\cup (dp[i][j][l] \\cap dp[i+ l + 1][j + l + 1][k - l -1]) \\cup (dp[i][j + k - l][l] \\cap dp[i+l+1][j][k - l -1]), & \\text{l  in [0,k), k>0 }\\end{cases}", description: "Rawfile: DP状态转移", category: "normal" },
    { formula: "f(s,t) = \\begin{cases} T \\:\\: if \\:\\:s == t\\\\ F \\:\\: if \\:\\:\\:not\\:\\: anagram \\\\ \\bigvee_{i=0}^{n-1} f(s_x,t_x) \\wedge f(s_y, t_y)\\:\\: no\\:\\:swap \\\\ \\bigvee_{i=0}^{n-1} f(s_x,t_y') \\wedge f(s_y,t_x') \\: swapped\\end{cases}", description: "Rawfile: 递归函数定义", category: "normal" },
    { formula: "a_i = a_i'b_ix_i + a_ib_i'x_i'", description: "Rawfile: 位运算公式1", category: "normal" },
    { formula: "b_i = a_i'b_i'x_i + a_i'b_ix_i' = a_i'(b_i \\oplus x_i)", description: "Rawfile: 位运算公式2", category: "normal" },
    { formula: "\\begin{cases}\\texttt{a = (\\textasciitilde a \\& b \\& x) | (a \\& \\textasciitilde b \\& \\textasciitilde x)} \\\\\\texttt{b = \\textasciitilde a \\& (b\\^ x)}\\end{cases}", description: "Rawfile: 位运算方程组", category: "normal" },
    
    // 方程与标签 (test68)
    { formula: "x + 2y = 15 \\tag{3}", description: "Rawfile: 方程带标签", category: "normal" },
    
    // 热力学 (test69)
    { formula: "n_{\\ce{CO2}}\\Delta h_{\\ce{CO2}}+n_{\\ce{H2O}}\\Delta h_{\\ce{H2O}}+n_{\\ce{N2}}\\Delta h_{\\ce{N2}}=802,200", description: "Rawfile: 热力学方程", category: "normal" },
    
    // 圆的方程 (test70)
    { formula: "\\begin{align*}(x - 8)^2 + (y - 4)^2 &= 16 \\\\x^2 - 16x + 64 + y^2 - 8y + 16 &= 16 \\\\(x^2 + y^2) - 16x - 8y + 80 &= 16\\end{align*}", description: "Rawfile: 圆的方程展开", category: "normal" },
    
    // 矩阵表示 (test71-test73)
    { formula: "X\\in\\mathbb{R}^{u\\times r}", description: "Rawfile: 矩阵维度表示", category: "normal" },
    { formula: "u", description: "Rawfile: 变量u", category: "normal" },
    { formula: "r", description: "Rawfile: 变量r", category: "normal" },
    
    // 曲面积分 (test74-test77)
    { formula: "a + \\oiint b", description: "Rawfile: 曲面积分基本", category: "normal" },
    { formula: "\\begin{array}{c}\\oiint_A f \\\\\\oiint_B g\\end{array}", description: "Rawfile: 曲面积分阵列", category: "normal" },
    { formula: "\\oiint_{A_{\\oiint_B}} f", description: "Rawfile: 嵌套曲面积分", category: "normal" },
    { formula: "\\oiint", description: "Rawfile: 曲面积分符号", category: "normal" },
    
    // 化学式基础 (test78-test81)
    { formula: "\\ce{H2O} \\ce{Sb2O3} \\ce{H+} \\ce{CrO4^2-} \\ce{[AgCl2]-} \\ce{Y^99+} \\ce{Y^{99+}}", description: "Rawfile: 化学式集合", category: "normal" },
    { formula: "\\ce{2H2O} \\ce{0.5 H2O} \\ce{1/2H2O} \\ce{(1/2)H2O} \\ce{$n$ H2O}", description: "Rawfile: 化学计量数", category: "normal" },
    { formula: "\\ce{^{227}_{90}Th+} \\ce{^227_90Th+} \\ce{^{0}_{-1}n^{-}} \\ce{^0_-1n-}", description: "Rawfile: 同位素表示", category: "normal" },
    { formula: "\\ce{H{}^3HO} \\ce{H^3HO}", description: "Rawfile: 氚水表示", category: "normal" },
    
    // 化学反应箭头 (test82-test85)
    { formula: "\\ce{A -> B} \\ce{A <- B} \\ce{A <-> B}", description: "Rawfile: 化学反应箭头1", category: "normal" },
    { formula: "\\ce{A <--> B} \\ce{A <=> B} \\ce{A <=>> B} \\ce{A <<=> B}", description: "Rawfile: 化学反应箭头2", category: "normal" },
    { formula: "\\ce{A ->[H2O] B} \\ce{A ->[{上方文字}][{下方文字}] B}", description: "Rawfile: 带条件反应", category: "normal" },
    { formula: "\\ce{A v B (v) -> B ^ B (^)}", description: "Rawfile: 沉淀气体标记", category: "normal" },
    
    // 复杂化学方程式 (test87-test90)
    { formula: "\\ce{Zn^2+ <=>[+ 2OH-][+ 2H+] $\\underset{\\text{amphoteres Hydroxid}}{\\ce{Zn(OH)2 v}}$ <=>[+2OH-][+ 2H+] $\\underset{\\text{Hydroxozikat}}{\\ce{[Zn(OH)4]^2-}}$}", description: "Rawfile: 锌的两性反应", category: "normal" },
    { formula: "\\ce{$K = \\frac{[\\ce{Hg^2+}][\\ce{Hg}]}{[\\ce{Hg2^2+}]}$}", description: "Rawfile: 平衡常数表达式", category: "normal" },
    { formula: "\\ce{$K = \\ce{\\frac{[Hg^2+][Hg]}{[Hg2^2+]}}$}", description: "Rawfile: 平衡常数表达式2", category: "normal" },
    { formula: "\\ce{Hg^2+ ->[I-] $\\underset{\\mathrm{red}}{\\ce{HgI2}}$ ->[I-]$\\underset{\\mathrm{red}}{\\ce{[Hg^{II}I4]^2-}}$}", description: "Rawfile: 汞的络合反应", category: "normal" },
    
    // ==================== 异常用例 (error) ====================
    // 分数错误 (BadFrac)
    { formula: "\\frac{1}{", description: "分数错误: 缺少右花括号", category: "error" },
    { formula: "\\frac{1}{2", description: "分数错误: 分母未闭合", category: "error" },
    { formula: "\\frac1}{2}", description: "分数错误: 缺少左花括号", category: "error" },
    { formula: "\\frac{1", description: "分数错误: 缺少分母", category: "error" },
    { formula: "\\frac", description: "分数错误: 无参数", category: "error" },
    { formula: "\\frac{", description: "分数错误: 仅有左花括号", category: "error" },
    { formula: "\\frac{}", description: "分数错误: 仅有分子花括号", category: "error" },
    { formula: "\\frac{ }{ }", description: "分数错误: 仅空白符", category: "error" },
    { formula: "\\frac{a}", description: "分数错误: 缺少分母花括号", category: "error" },
    { formula: "\\frac{a}{", description: "分数错误: 分母未闭合", category: "error" },
    { formula: "\\frac{a{b}{c}", description: "分数错误: 括号不匹配", category: "error" },
    { formula: "\\frac{a}{b", description: "分数错误: 缺少右括号", category: "error" },
    
    // 根号错误 (BadRoot)
    { formula: "\\sqrt{x", description: "根号错误: 根号未闭合", category: "error" },
    { formula: "\\sqrt[2]{x", description: "根号错误: 带索引未闭合", category: "error" },
    { formula: "\\sqrt[", description: "根号错误: 索引不完整", category: "error" },
    { formula: "\\sqrt", description: "根号错误: 无参数", category: "error" },
    { formula: "\\sqrt[]", description: "根号错误: 可选参数为空且缺少必需参数", category: "error" },
    { formula: "\\sqrt[2]", description: "根号错误: 缺少被开方数", category: "error" },
    { formula: "\\sqrt{x+y", description: "根号错误: 缺少右括号", category: "error" },
    { formula: "\\sqrt[3{x}", description: "根号错误: 指数未闭合", category: "error" },
    
    // 上下标错误 (SubSupError)
    { formula: "x_1_2", description: "上下标错误: 双重下标(无效)", category: "error" },
    { formula: "x^1^2", description: "上下标错误: 双重上标(无效)", category: "error" },
    { formula: "x_", description: "上下标错误: 缺少下标", category: "error" },
    { formula: "x^", description: "上下标错误: 缺少上标", category: "error" },
    { formula: "x_{", description: "上下标错误: 下标花括号未闭合", category: "error" },
    { formula: "x^{", description: "上下标错误: 上标花括号未闭合", category: "error" },
    
    // 矩阵错误 (BadMatrix/MatrixError)
    { formula: "\\begin{matrix} a & b \\\\ c \\end{matrix}", description: "矩阵错误: 列数不一致", category: "error" },
    { formula: "\\begin{matrix} a & b", description: "矩阵错误: 缺少end", category: "error" },
    { formula: "\\begin{pmatrix} a & b", description: "矩阵错误: 缺少\\end{pmatrix}", category: "error" },
    { formula: "\\begin{matrix} a \\end{pmatrix}", description: "矩阵错误: 环境不匹配", category: "error" },
    { formula: "\\begin{array}{ccc} a & b \\end{array}", description: "矩阵错误: 数组列不匹配", category: "error" },
    { formula: "\\begin{matrix}a", description: "矩阵错误: 缺少end", category: "error" },
    { formula: "\\end{matrix}", description: "矩阵错误: 缺少begin", category: "error" },
    { formula: "\\begin matrix", description: "矩阵错误: 缺少花括号", category: "error" },
    { formula: "\\begin{matrix", description: "矩阵错误: 环境名未闭合", category: "error" },
    
    // Cases错误 (BadCases)
    { formula: "\\begin{cases} x & y & z \\end{cases}", description: "cases错误: cases有3列", category: "error" },
    { formula: "\\begin{cases} x \\end{cases}", description: "cases错误: cases有1列", category: "error" },
    { formula: "\\begin{cases} x \\end{aligned}", description: "cases错误: cases结束不匹配", category: "error" },
    { formula: "\\begin{cases}x & y", description: "cases错误: 缺少end", category: "error" },
    { formula: "\\begin{cases} x & y", description: "cases错误: 缺少\\end{cases}", category: "error" },
    
    // 分隔符错误 (BadDelim)
    { formula: "\\left( a", description: "分隔符错误: 缺少right", category: "error" },
    { formula: "\\left( a \\right", description: "分隔符错误: right不完整", category: "error" },
    { formula: "a \\right)", description: "分隔符错误: 无left的right", category: "error" },
    { formula: "\\left( a \\right\\invalid", description: "分隔符错误: 无效分隔符", category: "error" },
    { formula: "\\right)", description: "分隔符错误: 缺少left", category: "error" },
    { formula: "\\left( x \\right", description: "分隔符错误: right后缺少分隔符", category: "error" },
    { formula: "\\left x \\right)", description: "分隔符错误: left后缺少分隔符", category: "error" },
    { formula: "\\left( \\frac{a}{b}", description: "分隔符错误: 缺少\\right", category: "error" },
    { formula: "\\frac{a}{b} \\right)", description: "分隔符错误: 缺少\\left", category: "error" },
    { formula: "\\left( \\left( \\frac{a}{b} \\right)", description: "分隔符错误: 嵌套不完整", category: "error" },
    
    // 重音符号错误 (BadAccent)
    { formula: "\\hat", description: "重音错误: hat缺少参数", category: "error" },
    { formula: "\\bar", description: "重音错误: bar缺少参数", category: "error" },
    { formula: "\\vec", description: "重音错误: vec缺少参数", category: "error" },
    { formula: "\\dot", description: "重音错误: dot缺少参数", category: "error" },
    { formula: "\\ddot", description: "重音错误: ddot缺少参数", category: "error" },
    { formula: "\\tilde", description: "重音错误: tilde缺少参数", category: "error" },
    { formula: "\\overline", description: "重音错误: overline缺少参数", category: "error" },
    { formula: "\\underline", description: "重音错误: underline缺少参数", category: "error" },
    { formula: "\\overbrace", description: "重音错误: overbrace缺少参数", category: "error" },
    { formula: "\\underbrace", description: "重音错误: underbrace缺少参数", category: "error" },
    { formula: "\\overrightarrow", description: "重音错误: overrightarrow缺少参数", category: "error" },
    { formula: "\\overleftarrow", description: "重音错误: overleftarrow缺少参数", category: "error" },
    { formula: "\\widehat", description: "重音错误: widehat缺少参数", category: "error" },
    { formula: "\\widetilde", description: "重音错误: widetilde缺少参数", category: "error" },
    
    // 上下装饰错误
    { formula: "\\overbrace{a+b+c}_{n}", description: "上下装饰错误: overbrace应使用上标", category: "error" },
    { formula: "\\overline{a+b", description: "上下装饰错误: 缺少右括号", category: "error" },
    
    // 堆叠命令错误 (StackError)
    { formula: "\\stackrel{def}", description: "堆叠错误: stackrel缺少第二个参数", category: "error" },
    { formula: "\\stackrel{a}{b}{c}", description: "堆叠错误: stackrel多余参数", category: "error" },
    { formula: "\\overset{*}", description: "堆叠错误: overset缺少第二个参数", category: "error" },
    { formula: "\\underset{*}", description: "堆叠错误: underset缺少第二个参数", category: "error" },
    { formula: "\\underset{i=1}{\\sum\\limits_{i=1}^n}", description: "堆叠错误: 重复定义", category: "error" },
    { formula: "\\sideset{^a_b}\\sum", description: "堆叠错误: sideset缺少第二个参数", category: "error" },
    { formula: "\\sideset{^a}{^c}\\sum", description: "堆叠错误: sideset缺少第三个参数", category: "error" },
    { formula: "\\sideset{^a_b}{^c_d}", description: "堆叠错误: sideset缺少运算符", category: "error" },
    { formula: "\\sideset{^a_b}{^c_d}x", description: "堆叠错误: sideset用于非运算符", category: "error" },
    { formula: "\\prescript{a}{X}", description: "堆叠错误: prescript缺少中间参数", category: "error" },
    { formula: "\\prescript{a}{b}{X}{Y}", description: "堆叠错误: prescript多余参数", category: "error" },
    
    // 文本命令错误 (TextError)
    { formula: "\\text", description: "文本错误: text缺少参数", category: "error" },
    { formula: "\\text{}", description: "文本错误: text空参数", category: "error" },
    { formula: "\\text{\\frac{a}{b}}", description: "文本错误: 数学命令在文本中", category: "error" },
    { formula: "\\mbox", description: "文本错误: mbox缺少参数", category: "error" },
    { formula: "\\textit", description: "文本错误: textit缺少参数", category: "error" },
    { formula: "\\textbf", description: "文本错误: textbf缺少参数", category: "error" },
    { formula: "\\textsf", description: "文本错误: textsf缺少参数", category: "error" },
    { formula: "\\texttt", description: "文本错误: texttt缺少参数", category: "error" },
    { formula: "\\textrm", description: "文本错误: textrm缺少参数", category: "error" },
    { formula: "\\textsc", description: "文本错误: textsc缺少参数", category: "error" },
    { formula: "\\textsuperscript", description: "文本错误: textsuperscript缺少参数", category: "error" },
    { formula: "\\textsubscript", description: "文本错误: textsubscript缺少参数", category: "error" },
    { formula: "\\textcircled", description: "文本错误: textcircled缺少参数", category: "error" },
    
    // 字体命令错误 (FontError)
    { formula: "\\mathbf", description: "字体错误: mathbf缺少参数", category: "error" },
    { formula: "\\mathit", description: "字体错误: mathit缺少参数", category: "error" },
    { formula: "\\mathrm", description: "字体错误: mathrm缺少参数", category: "error" },
    { formula: "\\mathsf", description: "字体错误: mathsf缺少参数", category: "error" },
    { formula: "\\mathtt", description: "字体错误: mathtt缺少参数", category: "error" },
    { formula: "\\mathbb", description: "字体错误: mathbb缺少参数", category: "error" },
    { formula: "\\mathcal", description: "字体错误: mathcal缺少参数", category: "error" },
    { formula: "\\mathscr", description: "字体错误: mathscr缺少参数", category: "error" },
    { formula: "\\mathfrak", description: "字体错误: mathfrak缺少参数", category: "error" },
    { formula: "\\mathds", description: "字体错误: mathds缺少参数", category: "error" },
    { formula: "\\boldsymbol", description: "字体错误: boldsymbol缺少参数", category: "error" },
    { formula: "\\boldsymbol{}", description: "字体错误: boldsymbol空参数", category: "error" },
    { formula: "\\bold", description: "字体错误: bold缺少参数", category: "error" },
    { formula: "\\cal", description: "字体错误: cal缺少参数", category: "error" },
    { formula: "\\frak", description: "字体错误: frak缺少参数", category: "error" },
    { formula: "\\Bbb", description: "字体错误: Bbb缺少参数", category: "error" },
    { formula: "\\oldstylenums", description: "字体错误: oldstylenums缺少参数", category: "error" },
    
    // 数学类型错误 (MathTypeError)
    { formula: "\\mathop", description: "数学类型错误: mathop缺少参数", category: "error" },
    { formula: "\\mathbin", description: "数学类型错误: mathbin缺少参数", category: "error" },
    { formula: "\\mathrel", description: "数学类型错误: mathrel缺少参数", category: "error" },
    { formula: "\\mathopen", description: "数学类型错误: mathopen缺少参数", category: "error" },
    { formula: "\\mathclose", description: "数学类型错误: mathclose缺少参数", category: "error" },
    { formula: "\\mathpunct", description: "数学类型错误: mathpunct缺少参数", category: "error" },
    { formula: "\\mathord", description: "数学类型错误: mathord缺少参数", category: "error" },
    { formula: "\\mathinner", description: "数学类型错误: mathinner缺少参数", category: "error" },
    
    // 其他命令错误 (OtherError)
    { formula: "\\operatorname", description: "其他错误: operatorname缺少参数", category: "error" },
    { formula: "\\phantom", description: "其他错误: phantom缺少参数", category: "error" },
    { formula: "\\hphantom", description: "其他错误: hphantom缺少参数", category: "error" },
    { formula: "\\vphantom", description: "其他错误: vphantom缺少参数", category: "error" },
    { formula: "\\llap", description: "其他错误: llap缺少参数", category: "error" },
    { formula: "\\rlap", description: "其他错误: rlap缺少参数", category: "error" },
    { formula: "\\clap", description: "其他错误: clap缺少参数", category: "error" },
    { formula: "\\mathllap", description: "其他错误: mathllap缺少参数", category: "error" },
    { formula: "\\mathrlap", description: "其他错误: mathrlap缺少参数", category: "error" },
    { formula: "\\mathclap", description: "其他错误: mathclap缺少参数", category: "error" },
    { formula: "\\cancel", description: "其他错误: cancel缺少参数", category: "error" },
    { formula: "\\bcancel", description: "其他错误: bcancel缺少参数", category: "error" },
    { formula: "\\xcancel", description: "其他错误: xcancel缺少参数", category: "error" },
    { formula: "\\st", description: "其他错误: st缺少参数", category: "error" },
    { formula: "\\rule", description: "其他错误: rule缺少参数", category: "error" },
    { formula: "\\includegraphics", description: "其他错误: includegraphics缺少参数", category: "error" },
    { formula: "\\Braket", description: "其他错误: Braket缺少参数", category: "error" },
    { formula: "\\Set", description: "其他错误: Set缺少参数", category: "error" },
    { formula: "\\Bra", description: "其他错误: Bra缺少参数", category: "error" },
    { formula: "\\Ket", description: "其他错误: Ket缺少参数", category: "error" },
    { formula: "\\char", description: "其他错误: char缺少参数", category: "error" },
    { formula: "\\roman", description: "其他错误: roman缺少参数", category: "error" },
    { formula: "\\Roman", description: "其他错误: Roman缺少参数", category: "error" },
    { formula: "\\XML", description: "其他错误: XML缺少参数", category: "error" },
    { formula: "\\dynamic", description: "其他错误: dynamic缺少参数", category: "error" },
    { formula: "\\externalFont", description: "其他错误: externalFont缺少参数", category: "error" },
    { formula: "\\cornersize", description: "其他错误: cornersize缺少参数", category: "error" },
    
    // 盒子命令错误 (BoxError)
    { formula: "\\fbox", description: "盒子错误: fbox缺少参数", category: "error" },
    { formula: "\\boxed", description: "盒子错误: boxed缺少参数", category: "error" },
    { formula: "\\colorbox{red}", description: "盒子错误: colorbox缺少内容参数", category: "error" },
    { formula: "\\fcolorbox{blue}{red}", description: "盒子错误: fcolorbox缺少内容参数", category: "error" },
    { formula: "\\shadowbox", description: "盒子错误: shadowbox缺少参数", category: "error" },
    { formula: "\\ovalbox", description: "盒子错误: ovalbox缺少参数", category: "error" },
    { formula: "\\doublebox", description: "盒子错误: doublebox缺少参数", category: "error" },
    { formula: "\\smash", description: "盒子错误: smash缺少参数", category: "error" },
    
    // 图形变换错误 (TransformError)
    { formula: "\\rotatebox{45}", description: "图形变换错误: rotatebox缺少内容参数", category: "error" },
    { formula: "\\scalebox{2}", description: "图形变换错误: scalebox缺少内容参数", category: "error" },
    { formula: "\\resizebox{2cm}{!}", description: "图形变换错误: resizebox缺少内容参数", category: "error" },
    { formula: "\\reflectbox", description: "图形变换错误: reflectbox缺少参数", category: "error" },
    
    // 箭头扩展错误 (ArrowError)
    { formula: "A \\xrightarrow B", description: "箭头错误: xrightarrow缺少参数", category: "error" },
    { formula: "A \\xleftarrow B", description: "箭头错误: xleftarrow缺少参数", category: "error" },
    
    // 花括号匹配错误 (SyntaxBrace)
    { formula: "{", description: "花括号错误: 单个左花括号", category: "error" },
    { formula: "}", description: "花括号错误: 单个右花括号", category: "error" },
    { formula: "{a", description: "花括号错误: 缺少右括号", category: "error" },
    { formula: "a}", description: "花括号错误: 缺少左括号", category: "error" },
    { formula: "{a{b}", description: "花括号错误: 嵌套不匹配", category: "error" },
    { formula: "}{", description: "花括号错误: 顺序颠倒", category: "error" },
    
    // 命令错误 (SyntaxBackslash)
    { formula: "\\", description: "命令错误: 单个反斜杠", category: "error" },
    { formula: "x\\", description: "命令错误: 末尾不完整", category: "error" },
    { formula: "\\x", description: "命令错误: 未知命令\\x", category: "error" },
    { formula: "\\unknownCommand", description: "命令错误: 未知命令", category: "error" },
    
    // 环境错误 (SyntaxEnv/EnvMismatch)
    { formula: "\\begin{cases", description: "环境错误: 环境名未闭合", category: "error" },
    { formula: "\\begin{aligned}a\\end{cases}", description: "环境错误: begin/end不匹配", category: "error" },
    
    // 对齐符错误
    { formula: "a & b", description: "对齐符错误: 环境外使用&", category: "error" },
    { formula: "&", description: "对齐符错误: 单独&符号", category: "error" },
    
    // 空公式
    { formula: "", description: "空公式", category: "error" },
    
    // 组合错误 (ComboError)
    { formula: "\\frac{1}{2} + \\frac{3}{", description: "组合错误: 部分第二个分数", category: "error" },
    { formula: "\\sqrt{x} + \\sqrt{", description: "组合错误: 结尾部分sqrt", category: "error" },
    { formula: "\\left( a \\right) + \\left( b", description: "组合错误: 和中未闭合分隔符", category: "error" },
    { formula: "\\begin{matrix} a & b \\end{matrix} + \\begin{cases", description: "组合错误: 部分环境", category: "error" },
    
    // 宏定义错误 (MacroError)
    { formula: "\\newcommand{\\diff}", description: "宏定义错误: 缺少定义内容", category: "error" },
    { formula: "\\newcommand{\\frac}{abc}", description: "宏定义错误: 重复定义已存在的命令", category: "error" },
    { formula: "\\newcommand{\\test}[1]{#2}", description: "宏定义错误: 参数引用错误", category: "error" },
    { formula: "\\newcommand{test}{abc}", description: "宏定义错误: 命令名格式错误", category: "error" },
    { formula: "\\newcommand{\\norm}[1]{\\left\\|#1\\right\\|}\\norm{v}", description: "宏定义错误: 不支持的分隔符", category: "error" },
    
    // ==================== 容错用例 (tolerant) ====================
    // 分数命令 - 容错 (空参数被库接受)
    { formula: "\\frac{a}{b}{c}", description: "分数容错: 多余参数被忽略", category: "tolerant" },
    { formula: "\\frac{a \\over b}{c}", description: "分数容错: over在花括号内", category: "tolerant" },
    { formula: "\\cfrac{a}", description: "连分数容错: 缺少分母", category: "tolerant" },
    { formula: "\\binom{n}", description: "二项式容错: 缺少第二个参数", category: "tolerant" },
    { formula: "\\binom{}{k}", description: "二项式容错: 空上标", category: "tolerant" },
    { formula: "a \\over b", description: "分数线容错: 缺少分组", category: "tolerant" },
    
    // 根号命令 - 容错
    { formula: "\\sqrt[3]{x}{y}", description: "根号容错: 多余参数被忽略", category: "tolerant" },
    { formula: "\\sqrt[]", description: "根号容错: 可选参数为空且缺少必需参数", category: "tolerant" },
    
    // 堆叠命令 - 容错
    { formula: "\\stackrel{a}{b}{c}", description: "堆叠容错: 多余参数被忽略", category: "tolerant" },
    
    // 括号与分隔符 - 容错
    { formula: "\\big", description: "手动括号容错: 缺少分隔符", category: "tolerant" },
    { formula: "\\big a", description: "手动括号容错: 分隔符类型错误", category: "tolerant" },
    { formula: "\\Big", description: "手动括号容错: Big缺少分隔符", category: "tolerant" },
    { formula: "\\bigg", description: "手动括号容错: bigg缺少分隔符", category: "tolerant" },
    { formula: "\\Bigg", description: "手动括号容错: Bigg缺少分隔符", category: "tolerant" },
    
    // 矩阵与表格 - 容错
    { formula: "\\begin{matrx}a\\end{matrx}", description: "矩阵容错: 环境名拼写错误", category: "tolerant" },
    { formula: "\\begin{pmatrix}a , b \\\\ c , d\\end{pmatrix}", description: "矩阵容错: 分隔符错误", category: "tolerant" },
    { formula: "\\begin{array}a & b\\end{array}", description: "数组容错: 缺少列格式", category: "tolerant" },
    { formula: "\\begin{array}{cc}a & b & c\\end{array}", description: "数组容错: 列格式与内容不匹配", category: "tolerant" },
    
    // 环境使用 - 容错
    { formula: "\\begin{cases}1 \\\\ 0 \\\\ -1\\end{cases}", description: "cases容错: 缺少条件部分", category: "tolerant" },
    { formula: "\\begin{aligned}a = b + c \\\\ d = e + f\\end{aligned}", description: "aligned容错: 缺少对齐点", category: "tolerant" },
    
    // 边界情况 - 容错
    { formula: " ", description: "边界容错: 空白字符串", category: "tolerant" },
    { formula: "  ", description: "边界容错: 多个空格", category: "tolerant" },
    { formula: "\\,", description: "边界容错: 空格命令", category: "tolerant" },
    { formula: "\\;", description: "边界容错: 宽空格命令", category: "tolerant" },
    { formula: "\\!", description: "边界容错: 负空格命令", category: "tolerant" },
    { formula: "\\quad", description: "边界容错: quad空格", category: "tolerant" },
    { formula: "\\qquad", description: "边界容错: qquad空格", category: "tolerant" },
    
    // 其他容错
    { formula: "\\phantom", description: "占位符容错: 缺少参数", category: "tolerant" },
    { formula: "\\hphantom", description: "水平占位符容错: 缺少参数", category: "tolerant" },
    { formula: "\\vphantom", description: "垂直占位符容错: 缺少参数", category: "tolerant" },
    { formula: "\\smash", description: "压缩容错: 缺少参数", category: "tolerant" },
    { formula: "\\raisebox{1em}", description: "提升盒子容错: 缺少内容", category: "tolerant" },
    { formula: "\\rule{1em}", description: "规则线容错: 缺少高度", category: "tolerant" },
    { formula: "\\genfrac{(}{)}{1pt}{0}{a}{b}", description: "生成分数容错: 复杂参数", category: "tolerant" }
  ];

  async aboutToAppear(): Promise<void> {
    this.loadRawfileFormulas();
    await this.testAllFormulas();
  }

  loadRawfileFormulas(): void {
    let manager: resourceManager.ResourceManager = this.context.resourceManager;
    let files: string[] = manager.getRawFileListSync('');
    let textDecoderOptions: util.TextDecoderOptions = {
      fatal: false,
      ignoreBOM: true
    };
    let decodeToStringOptions: util.DecodeToStringOptions = {
      stream: false
    };
    let textDecoder = util.TextDecoder.create('utf-8', textDecoderOptions);

    for (let index = 0; index < files.length; index++) {
      let content: Uint8Array = manager.getRawFileContentSync(files[index]);
      let formula: string = textDecoder.decodeToString(content, decodeToStringOptions).trim();
      this.rawfileFormulas.push({
        formula: formula,
        description: `Rawfile: ${files[index]}`,
        category: 'normal'
      });
    }
  }

  async testAllFormulas(): Promise<void> {
    this.results = [];

    let filteredFormulas: FormulaItem[] = [];
    if (this.currentTab === 0) {
      filteredFormulas = this.rawfileFormulas;
    } else if (this.currentTab === 1) {
      filteredFormulas = this.formulas.filter(f => f.category === 'normal');
    } else if (this.currentTab === 2) {
      filteredFormulas = this.formulas.filter(f => f.category === 'error');
    } else {
      filteredFormulas = this.formulas.filter(f => f.category === 'tolerant');
    }
    
    for (let item of filteredFormulas) {
      try {
        let result: TeXParseResult = await latexStringToImageWithError(
          item.formula,
          fp2px(20),
          0xFF000000,
          0xFFFFFFFF,
          LatexMathColorFormat.COLOR_FORMAT_BGRA_8888
        );

        if (result.resultCode === TeXResultCode.Success) {
          let imageSource = image.createImageSource(result.imageBytes);
          let pixelMap = imageSource.createPixelMapSync();
          let size: Size = pixelMap.getImageInfoSync().size;
          
          let testResult: FormulaTestResult = {
            formula: item.formula,
            description: item.description,
            category: item.category,
            resultCode: result.resultCode,
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
            category: item.category,
            resultCode: result.resultCode,
            errorMessage: result.errorMessage
          };
          this.results.push(testResult);
        }
      } catch (e) {
        let testResult: FormulaTestResult = {
          formula: item.formula,
          description: item.description,
          category: item.category,
          resultCode: TeXResultCode.UnknownError,
          errorMessage: e instanceof Error ? e.message : String(e)
        };
        this.results.push(testResult);
      }
    }
  }

  getResultCodeName(code: TeXResultCode): string {
    switch (code) {
      case TeXResultCode.Success:
        return "Success";
      case TeXResultCode.SyntaxError:
        return "SyntaxError";
      case TeXResultCode.InvalidMatrixError:
        return "InvalidMatrixError";
      case TeXResultCode.InvalidDelimiterError:
        return "InvalidDelimiterError";
      case TeXResultCode.TeXError:
        return "TeXError";
      case TeXResultCode.ChemfigError:
        return "ChemfigError";
      case TeXResultCode.UnknownError:
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
            .fontColor(item.resultCode === TeXResultCode.Success ? SUCCESS_COLOR : (item.category === 'tolerant' ? WARNING_COLOR : FAILURE_COLOR))
            .fontWeight(FontWeight.Medium)
          
          Text(`公式: ${item.formula}`)
            .fontSize(12)
            .fontColor(GRAY_COLOR)
            .margin({ top: 2 })
          
          Text(`分类: ${item.category === 'normal' ? '正常' : (item.category === 'error' ? '异常' : '容错')}`)
            .fontSize(11)
            .fontColor('#888888')
            .margin({ top: 1 })
        }
        .alignItems(HorizontalAlign.Start)
        .layoutWeight(1)
      }
      .width('100%')
      .alignItems(VerticalAlign.Top)

      if (item.resultCode === TeXResultCode.Success) {
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
            Text(this.getResultCodeName(item.resultCode))
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
    .backgroundColor(item.resultCode === TeXResultCode.Success ? SUCCESS_BG_COLOR : FAILURE_BG_COLOR)
    .border({ width: 1, color: item.resultCode === TeXResultCode.Success ? SUCCESS_BORDER_COLOR : FAILURE_BORDER_COLOR })
  }

  build() {
    Column() {
      Text("LaTeX 公式检测演示")
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .margin({ top: 10, bottom: 10 })

      Row() {
        Button("Rawfile")
          .fontSize(14)
          .backgroundColor(this.currentTab === 0 ? '#2196F3' : '#E0E0E0')
          .fontColor(this.currentTab === 0 ? '#FFFFFF' : '#757575')
          .borderRadius(8)
          .margin({ right: 6 })
          .onClick(() => {
            this.currentTab = 0;
            this.testAllFormulas();
          })

        Button("正常用例")
          .fontSize(14)
          .backgroundColor(this.currentTab === 1 ? SUCCESS_COLOR : '#E0E0E0')
          .fontColor(this.currentTab === 1 ? '#FFFFFF' : '#757575')
          .borderRadius(8)
          .margin({ right: 6 })
          .onClick(() => {
            this.currentTab = 1;
            this.testAllFormulas();
          })

        Button("异常用例")
          .fontSize(14)
          .backgroundColor(this.currentTab === 2 ? FAILURE_COLOR : '#E0E0E0')
          .fontColor(this.currentTab === 2 ? '#FFFFFF' : '#757575')
          .borderRadius(8)
          .margin({ right: 6 })
          .onClick(() => {
            this.currentTab = 2;
            this.testAllFormulas();
          })

        Button("容错用例")
          .fontSize(14)
          .backgroundColor(this.currentTab === 3 ? WARNING_COLOR : '#E0E0E0')
          .fontColor(this.currentTab === 3 ? '#FFFFFF' : '#757575')
          .borderRadius(8)
          .onClick(() => {
            this.currentTab = 3;
            this.testAllFormulas();
          })
      }
      .justifyContent(FlexAlign.Center)
      .margin({ bottom: 10 })

      Text(`共 ${this.results.length} 个测试用例，${this.currentTab === 0 ? '正常' : (this.currentTab === 1 ? '异常' : (this.currentTab === 2 ? '容错' : 'Rawfile'))}场景`)
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