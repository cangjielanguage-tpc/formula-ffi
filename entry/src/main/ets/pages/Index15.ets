import { LatexMathColorFormat, latexStringToImageWithError, TeXResultCode, TeXParseResult } from '@cangjie-tpc/formula_hybrid';
import { image } from '@kit.ImageKit';

interface ChemfigItem {
  description: string;
  formula: string;
}

@Component
struct ChemfigCard {
  item: ChemfigItem = { description: '', formula: '' };
  index: number = 0;
  @State pixelMap: image.PixelMap | undefined = undefined;
  @State imgWidth: number = 0;
  @State imgHeight: number = 0;
  @State resultCode: TeXResultCode = TeXResultCode.UnknownError;
  @State errorMessage: string = '';

  aboutToDisappear(): void {
    if (this.pixelMap) {
      this.pixelMap.release()
      this.pixelMap = undefined
    }
  }

  async aboutToAppear(): Promise<void> {
    this.pixelMap = undefined
    this.imgWidth = 0
    this.imgHeight = 0
    this.errorMessage = ''
    this.resultCode = TeXResultCode.UnknownError

    try {
      const result: TeXParseResult = await latexStringToImageWithError(
        this.item.formula, fp2px(18), 0xFF000000, 0xFFFFFFFF,
        LatexMathColorFormat.COLOR_FORMAT_RGB_565
      );
      this.resultCode = result.resultCode;
      if (result.resultCode === TeXResultCode.Success && result.imageBytes && result.imageBytes.byteLength > 0) {
        let source: image.ImageSource | null = null;
        try {
          source = image.createImageSource(result.imageBytes);
          this.pixelMap = source.createPixelMapSync();
          let size: Size = this.pixelMap.getImageInfoSync().size;
          this.imgWidth = px2vp(size.width);
          this.imgHeight = px2vp(size.height);
        } catch (imgErr) {
          this.errorMessage = imgErr instanceof Error ? imgErr.message : '图像创建失败';
          if (this.pixelMap) {
            this.pixelMap.release();
            this.pixelMap = undefined;
          }
        } finally {
          if (source) {
            source.release();
          }
        }
      } else {
        this.errorMessage = result.errorMessage || `错误码: ${result.resultCode}`;
      }
    } catch (e) {
      this.errorMessage = e instanceof Error ? e.message : '未知异常';
    }
  }

  build() {
    Column() {
      Text(`${this.index}. ${this.item.description}`)
        .fontSize(16)
        .fontWeight(FontWeight.Medium)
        .margin({ bottom: 4 })

      Text(this.item.formula)
        .fontSize(12)
        .fontColor('#888888')
        .width('100%')
        .maxLines(3)
        .textOverflow({ overflow: TextOverflow.Ellipsis })
        .margin({ bottom: 8 })

      if (this.pixelMap) {
        Scroll() {
          Image(this.pixelMap)
            .objectFit(ImageFit.Contain)
            .width(this.imgWidth)
            .height(this.imgHeight)
        }
        .width('100%')
        .scrollable(ScrollDirection.Horizontal)
        .scrollBar(BarState.Auto)
      } else if (this.errorMessage) {
        Text(`渲染失败: ${this.errorMessage}`)
          .fontSize(12)
          .fontColor('#FF5722')
          .maxLines(2)
          .textOverflow({ overflow: TextOverflow.Ellipsis })
      }
    }
    .width('100%')
    .alignItems(HorizontalAlign.Start)
    .padding({ left: 16, right: 16, top: 12, bottom: 12 })
  }
}

@Entry
@Component
struct Index15 {
  private pageSize: number = 10
  @State currentPage: number = 0
  @State items: ChemfigItem[] = [

    { description: '[1] 基础原子和键 - 单原子A', formula: '\\chemfig{A}' },
    { description: '[1] 基础原子和键 - 单原子C', formula: '\\chemfig{C}' },
    { description: '[1] 基础原子和键 - 单原子N', formula: '\\chemfig{N}' },
    { description: '[1] 基础原子和键 - 单原子O', formula: '\\chemfig{O}' },
    { description: '[1] 基础原子和键 - 原子团CH3', formula: '\\chemfig{CH_3}' },
    { description: '[1] 基础原子和键 - 原子团OH', formula: '\\chemfig{OH}' },
    { description: '[1] 基础原子和键 - 原子团NH2', formula: '\\chemfig{NH_2}' },
    { description: '[1] 基础原子和键 - 原子团COOH', formula: '\\chemfig{COOH}' },
    { description: '[1] 基础原子和键 - 单键A-B', formula: '\\chemfig{A-B}' },
    { description: '[1] 基础原子和键 - 单键C-C', formula: '\\chemfig{C-C}' },
    { description: '[1] 基础原子和键 - 单键C-O', formula: '\\chemfig{C-O}' },
    { description: '[1] 基础原子和键 - 单键C-N', formula: '\\chemfig{C-N}' },
    { description: '[1] 基础原子和键 - 链A-B-C-D', formula: '\\chemfig{A-B-C-D}' },
    { description: '[1] 基础原子和键 - 丙烷', formula: '\\chemfig{CH_3-CH_2-CH_3}' },

    { description: '[2] 双键和三键 - 双键A=B', formula: '\\chemfig{A=B}' },
    { description: '[2] 双键和三键 - 双键C=C', formula: '\\chemfig{C=C}' },
    { description: '[2] 双键和三键 - 双键C=O', formula: '\\chemfig{C=O}' },
    { description: '[2] 双键和三键 - 双键O=O', formula: '\\chemfig{O=O}' },
    { description: '[2] 双键和三键 - 三键A~B', formula: '\\chemfig{A~B}' },
    { description: '[2] 双键和三键 - 三键C~C', formula: '\\chemfig{C~C}' },
    { description: '[2] 双键和三键 - 三键C~N', formula: '\\chemfig{C~N}' },
    { description: '[2] 双键和三键 - 混合键A-B=C~D', formula: '\\chemfig{A-B=C~D}' },
    { description: '[2] 双键和三键 - 丙烯', formula: '\\chemfig{CH_3-CH=CH_2}' },

    { description: '[3] 角度语法 - 增量角度[1]', formula: '\\chemfig{A-[1]B}' },
    { description: '[3] 角度语法 - 增量角度链[0][1][2]', formula: '\\chemfig{A-[0]B-[1]C-[2]D}' },
    { description: '[3] 角度语法 - 负增量角度[-1][-2]', formula: '\\chemfig{A-[-1]B-[-2]C}' },
    { description: '[3] 角度语法 - 绝对角度[:30]', formula: '\\chemfig{A-[:30]B}' },
    { description: '[3] 角度语法 - 绝对角度链[:0][:90][:180]', formula: '\\chemfig{A-[:0]B-[:90]C-[:180]D}' },
    { description: '[3] 角度语法 - 绝对角度[:45][:135]', formula: '\\chemfig{A-[:45]B-[:135]C}' },
    { description: '[3] 角度语法 - 相对角度[::30]', formula: '\\chemfig{A-[::30]B}' },
    { description: '[3] 角度语法 - 相对角度[::45][::-45]', formula: '\\chemfig{A-[::45]B-[::-45]C}' },
    { description: '[3] 角度语法 - 相对角度链[::60]', formula: '\\chemfig{A-[::60]B-[::60]C-[::60]D}' },
    { description: '[3] 角度语法 - 混合角度[1][:60][::-30]', formula: '\\chemfig{A-[1]B-[:60]C-[::-30]D}' },

    { description: '[4] 环语法 - 三元环*3', formula: '\\chemfig{*3(-=-)}' },
    { description: '[4] 环语法 - 四元环*4', formula: '\\chemfig{*4(=-=-)}' },
    { description: '[4] 环语法 - 五元环*5', formula: '\\chemfig{*5(-----)}' },
    { description: '[4] 环语法 - 六元环*6', formula: '\\chemfig{*6(------)}' },
    { description: '[4] 环语法 - 苯环*6(=-=-=-)', formula: '\\chemfig{*6(=-=-=-)}' },
    { description: '[4] 环语法 - 环双键*6(=--=--)', formula: '\\chemfig{*6(=--=--)}' },
    { description: '[4] 环语法 - 五元环双键*5(-=-=-)', formula: '\\chemfig{*5(-=-=-)}' },
    { description: '[4] 环语法 - 环三键*6(~-~-~-~-~)', formula: '\\chemfig{*6(~-~-~-~-~)}' },
    { description: '[4] 环语法 - 嵌套环6+5', formula: '\\chemfig{*6(-*5(-----)-=--)}' },
    { description: '[4] 环语法 - 嵌套环6+4', formula: '\\chemfig{*6(-*4(=-=-)--=)}' },
    { description: '[4] 环语法 - 嵌套环6', formula: '\\chemfig{*6(---=)}' },
    { description: '[4] 环语法 - 嵌套环6', formula: '\\chemfig{*6(--=--)}' },
    { description: '[4] 环语法 - 环上分支-[1]A', formula: '\\chemfig{*6(-[1]A------)}' },
    { description: '[4] 环语法 - 环上分支-[1]X', formula: '\\chemfig{*6(=-(-[1]X)-=--=)}' },
    { description: '[4] 环语法 - 芳香环**5', formula: '\\chemfig{**5(-----)}' },
    { description: '[4] 环语法 - 芳香环**6', formula: '\\chemfig{**6(------)}' },

    { description: '[5] 分支语法 - 单分支-[1]C', formula: '\\chemfig{A-B(-[1]C)-D}' },
    { description: '[5] 分支语法 - 单分支[:90]X', formula: '\\chemfig{A-B(-[:90]X)-C}' },
    { description: '[5] 分支语法 - 双分支-[1][7]', formula: '\\chemfig{A-B(-[1]C)(-[7]D)-E}' },
    { description: '[5] 分支语法 - 三分支-[1][3][5]', formula: '\\chemfig{A-B(-[1]X)(-[3]Y)(-[5]Z)-C}' },
    { description: '[5] 分支语法 - 嵌套分支', formula: '\\chemfig{A-B(-[1]C(-[2]D))-E}' },
    { description: '[5] 分支语法 - 复杂嵌套分支', formula: '\\chemfig{A-B(-[1]C(-[0]X)(-[2]Y))-D}' },
    { description: '[5] 分支语法 - 分支环*3', formula: '\\chemfig{A-B(-*3(-=-))-C}' },
    { description: '[5] 分支语法 - 分支环*5', formula: '\\chemfig{A-B(-[1]*5(-----))-C}' },

    { description: '[6] 上下标 - 下标A2', formula: '\\chemfig{A_2}' },
    { description: '[6] 上下标 - 下标CH3', formula: '\\chemfig{CH_3}' },
    { description: '[6] 上下标 - 下标链A1-B2-C3', formula: '\\chemfig{A_1-B_2-C_3}' },
    { description: '[6] 上下标 - 上标电荷+', formula: '\\chemfig{A^{+}}' },
    { description: '[6] 上下标 - 上标电荷-', formula: '\\chemfig{A^{-}}' },
    { description: '[6] 上下标 - 上标电荷2+', formula: '\\chemfig{A^{2+}}' },
    { description: '[6] 上下标 - 电荷链A+-B-', formula: '\\chemfig{A^{+}-B^{-}}' },
    { description: '[6] 上下标 - 上下标混合A2+3', formula: '\\chemfig{A^{2+}_3}' },
    { description: '[6] 上下标 - 上下标链A1^2-B2^1', formula: '\\chemfig{A_1^2-B_2^1}' },
    { description: '[6] 上下标 - 氧离子O2-', formula: '\\chemfig{O^{2-}}' },
    { description: '[6] 上下标 - 复杂下标(aq)', formula: '\\chemfig{A_{(aq)}}' },
    { description: '[6] 上下标 - 复杂下标n+1', formula: '\\chemfig{A_{n+1}}' },

    { description: '[7] 立体键 - 实心楔形上A>B', formula: '\\chemfig{A>B}' },
    { description: '[7] 立体键 - 实心楔形下A<B', formula: '\\chemfig{A<B}' },
    { description: '[7] 立体键 - 虚线楔形上A>:B', formula: '\\chemfig{A>:B}' },
    { description: '[7] 立体键 - 虚线楔形下A<:B', formula: '\\chemfig{A<:B}' },
    { description: '[7] 立体键 - 空心楔形上A>|B', formula: '\\chemfig{A>|B}' },
    { description: '[7] 立体键 - 空心楔形下A<|B', formula: '\\chemfig{A<|B}' },
    { description: '[7] 立体化学式 - 乳酸', formula: '\\chemfig{C(((<[:-60]OH)<:[:-30]H)-[:90]COOH)-[:-150]CH_3}' },
    { description: '[7] 立体键 - 混合立体键', formula: '\\chemfig{A>B<C>:D<:E}' },

    { description: '[8] 花括号保护 - {CH3}', formula: '\\chemfig{{CH_3}}' },
    { description: '[8] 花括号保护 - {AB}{CD}', formula: '\\chemfig{{AB}{CD}}' },
    { description: '[8] 花括号保护 - {CH2}{CH3}', formula: '\\chemfig{{CH_2}{CH_3}}' },
    { description: '[8] 花括号保护 - A-{BC}-D', formula: '\\chemfig{A-{BC}-D}' },
    { description: '[8] 花括号保护 - 丙烷完整', formula: '\\chemfig{{CH_3}-{CH_2}-{CH_3}}' },

    { description: '[9] 空格分隔 - A B C', formula: '\\chemfig{A B C}' },
    { description: '[9] 空格分隔 - C H3 O H', formula: '\\chemfig{C H_3 O H}' },
    { description: '[9] 空格分隔 - H O H', formula: '\\chemfig{H O H}' },

    { description: '[10] 管道分隔 - A|B', formula: '\\chemfig{A|B}' },
    { description: '[10] 管道分隔 - AB|CD|EF', formula: '\\chemfig{AB|CD|EF}' },
    { description: '[10] 管道分隔 - CH3|OH', formula: '\\chemfig{CH_3|OH}' },

    { description: '[12] 钩子 - 简单锚点?[a]', formula: '\\chemfig{A?[a]-B?[a]}' },
    { description: '[12] 钩子 - 多锚点?[x]?[y]', formula: '\\chemfig{A?[x]-B?[y]-C?[x]-D?[y]}' },
    { description: '[12] 钩子 - 数字锚点?[1]?[2]', formula: '\\chemfig{A?[1]-B?[2]-C?[1]-D?[2]}' },
    { description: '[12] 钩子 - 三角形钩子', formula: '\\chemfig{A?[a]-B?[a]-C?[a]}' },
    { description: '[12] 钩子 - 四边形钩子', formula: '\\chemfig{A?[a]-B?[a]-C?[a]-D?[a]}' },
    { description: '[12] 钩子 - 分支钩子', formula: '\\chemfig{A-B(-[1]C?[a])-[7]D?[a]}' },
    { description: '[12] 钩子 - 环上钩子', formula: '\\chemfig{*6(-A?[a]-B-C?[a]-D-E)}' },
    { description: '[12] 钩子 - 多组钩子', formula: '\\chemfig{A?[a]-B?[b]-C?[a]-D?[b]}' },
    { description: '[12] 钩子 - 链式钩子', formula: '\\chemfig{A?[a]-B-C?[a]-D-E?[a]}' },
    { description: '[12] 钩子 - 嵌套分支钩子', formula: '\\chemfig{A-B(-[1]C(-[2]D?[a]))-[7]E?[a]}' },

    { description: '[13] 锚点 - 原子锚点@{n1}', formula: '\\chemfig{A@{n1}-B@{n2}-C@{n3}}' },
    { description: '[13] 锚点 - 分支锚点', formula: '\\chemfig{A-B(-[1]C@{branch1})-[7]D@{branch2}}' },
    { description: '[13] 锚点 - 环上锚点', formula: '\\chemfig{*6(-A@{r1}-B-C@{r2}-D-E)}' },
    { description: '[13] 锚点 - 混合锚点钩子', formula: '\\chemfig{A@{a1}-B?[x]-C@{a2}-D?[x]}' },
    { description: '[13] 锚点 - 键位置锚点@{mid1}', formula: '\\chemfig{A-[@{mid1},1]B-[@{mid2},2]C}' },
    { description: '[13] 锚点 - 键位置偏移@{p1,0.3}', formula: '\\chemfig{A-[@{p1,0.3},1]B}' },

    { description: '[14] 键偏移 - 参照0偏移', formula: '\\chemfig{A-B}' },
    { description: '[14] 键偏移 - #(5pt)', formula: '\\chemfig{A-#(5pt)B}' },
    { description: '[14] 键偏移 - 非对称#(10pt,5pt)', formula: '\\chemfig{A-#(10pt,5pt)B}' },
    { description: '[14] 键偏移 - 非对称#(5pt,10pt)', formula: '\\chemfig{A-#(5pt,10pt)B}' },
    { description: '[14] 键偏移 - 双键偏移', formula: '\\chemfig{A=#(5pt)B}' },
    { description: '[14] 键偏移 - #(10pt)链', formula: '\\chemfig{A-#(10pt)B-#(10pt)C}' },

    { description: '[18] 真实分子 - 甲烷', formula: '\\chemfig{C(-[2]H)(-[4]H)(-[6]H)(-[8]H)}' },
    { description: '[18] 真实分子 - 乙醇', formula: '\\chemfig{CH_3-CH_2-OH}' },
    { description: '[18] 真实分子 - 乙酸', formula: '\\chemfig{CH_3-C(=[1]O)(-[7]OH)}' },
    { description: '[18] 真实分子 - 苯', formula: '\\chemfig{*6(=-=-=-)}' },
    { description: '[18] 真实分子 - 葡萄糖', formula: '\\chemfig{*6((=O)-(-[2]OH)-(-[2]OH)-(-[2]OH)-(-[2]OH)-)}' },
    { description: '[18] 真实分子 - 咖啡因', formula: '\\chemfig{*6((=O)-N(-CH_3)-=N(-CH_3)-=)}' },
    //{ description: '[18] 真实分子 - 阿司匹林', formula: '\\chemfig{*6(=-(-COOH)=(-OC(=O)CH_3)-=)}' },

    { description: '[19] issu174', formula: '\\chemfig{CH_3-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-\n' +
      '        CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-\n' +
      '        CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-\n' +
      '        CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-\n' +
      '        CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_2-CH_3}' },
    { description: '[19] issu173吲哚', formula: '\\chemfig{*6(-=N-*5(-NH-=-=)-=-=)}' },
    { description: '[19] issu172', formula: '\\chemfig{*6(-=-=-=)*6(-=-=-=)}' },
    { description: '[19] issu172', formula: '\\chemfig{*6(-=-=-=)*5(-NH-=-=)}' },
    { description: '[19] issu172', formula: '\\chemfig{*6(-=N-=*5(-S-=-=)-=)}' },
    { description: '[19] issu171', formula: '\\chemfig{CH_3-(CH_2)_{50}-CH_3}' },
    { description: '[19] issu170', formula: '\\chemfig{CH_3-(CH2)_{10}-COOH}' },
    { description: '[19] issu168', formula: '\\chemfig{(CH_3)_2CH-CH_2-*6(-=-=-=)-CH(CH_3)-COOH}' },

    { description: '[19] issu165', formula: '\\chemfig{CH3-CH2-OH} 乙醇' },
    { description: '[19] issu164', formula: '\\chemfig{CH#CH}' },
    { description: '[19] issu163', formula: '\\chemfig{CH3-CH3}' },
    { description: '[19] issu163', formula: '\\chemfig{CH2=CH2}' },
    { description: '[19] issu162', formula: '\\chemfig{*6(=-(-COOH)=(-OC(=O)CH_3)-=)}' },

    { description: '[19] issuxxx', formula: '\\chemfig{*6(-=N-=*6(-=N-=*6(-=N-=*6(-=N-=-=-)-)-)-)-}' },
    { description: '[19] issuxxx', formula: '\\chemfig{CH_3-(CH_2)_{50}-CH_3}' },
    { description: '[19] issuxxx', formula: '\\chemfig{*6(-=-=-*6(-=-*5(-N-=-=)-=*6(-=-*5(-S-=-=)-=*6(-=-*5(-O-=-=)-=-=))))}' },

    { description: '[20] 氧原子+90°矩形电子', formula: '\\chemfig{\\charge{90=\"}{O}}' },
    { description: '[20] 氧原子+45°矩形电子', formula: '\\chemfig{\\charge{45=\"}{O}}' },
    { description: '[20] 氧原子+0°矩形电子', formula: '\\chemfig{\\charge{0=\"}{O}}' },
    { description: '[20] 氧原子+135°矩形电子', formula: '\\chemfig{\\charge{135=\"}{O}}' },
    { description: '[20] 氧原子+90°线电子', formula: '\\chemfig{\\charge{90=\|}{O}}' },
    { description: '[20] 氧原子+45°线电子', formula: '\\chemfig{\\charge{45=\|}{O}}' },
    { description: '[20] 氧原子+0°线电子', formula: '\\chemfig{\\charge{0=\|}{O}}' },
    { description: '[20] 氧原子+180°线电子', formula: '\\chemfig{\\charge{180=\|}{O}}' },
    { description: '[20] 氧原子+90°单电子', formula: '\\chemfig{\\charge{90=\.}{O}}' },
    { description: '[20] 氧原子+45°单电子', formula: '\\chemfig{\\charge{45=\.}{O}}' },
    { description: '[20] 氧原子+0°单电子', formula: '\\chemfig{\\charge{0=\.}{O}}' },
    { description: '[20] 氧原子+225°单电子', formula: '\\chemfig{\\charge{225=\.}{O}}' },
    { description: '[20] 氧原子+90°孤对电子', formula: '\\chemfig{\\charge{90=\:, 0=\:}{O}}' },
    { description: '[20] 氧原子+45°孤对电子', formula: '\\chemfig{\\charge{45=\:}{O}}' },
    { description: '[20] 氧原子+0°孤对电子', formula: '\\chemfig{\\charge{0=\:}{O}}' },
    { description: '[20] 氧原子+270°孤对电子', formula: '\\chemfig{\\charge{270=\:}{O}}' },
    { description: '[20] 矩形电子+孤对电子混合显示', formula: '\\chemfig{\\charge{90=\:}{O} - C = \\charge{90=\"}{O}}' },
    { description: '[20] 正负电子氧原子', formula: '\\chemfig{\\charge{180=\\oplus}{O} - \\charge{0=\\ominus}{O}}' },
    { description: '[20] 正负电子的间距和大小调节', formula: '\\chemfig{\\charge{45:3pt=$\\scriptstyle\\ominus$}{O}}' },

    // { description: '[20] 电子移动 - 羰基共振', formula: '\\schemestart \\chemfig{C([3]-)([5]-)=[@{db,.5}]@{atoo}\\charge{0=\\|,-90=\\|}{O}} \\arrow(.mid east--.mid west){<->} \\chemfig{\\charge{90:3pt=$\\scriptstyle\\oplus$}{C}([3]-)([5]-)-\\charge{0=\\|,90=\\|,-90=\\|,45:3pt=$\\scriptstyle\\ominus$}{O}} \\schemestop\\chemmove{\\draw[shorten <=2pt, shorten >=2pt](db) ..controls +(up:5mm) and +(up:5mm)..(atoo);}' },
    { description: '[20] 电子移动 - 羰基共振[测试版]', formula: '\\schemestart \\chemfig{C([3]-)([5]-)=[@{db,.5}]@{atoo}\\charge{0=\\|,-90=\\|}{O}} \\arrow(.mid east--.mid west){<->} \\chemfig{\\charge{90:3pt=$\\scriptstyle\\oplus$}{C}([3]-)([5]-)-\\charge{0=\\|,90=\\|,-90=\\|,45:3pt=$\\scriptstyle\\ominus$}{O}} \\schemestop ' },

    { description: '[20] 基础反应式 - 简单反应', formula: '\\schemestart \\chemfig{*6(-=-=-=)}\\arrow \\chemfig{X=[1]Y}\\arrow \\chemfig{S>T} \\schemestop' },
    { description: '[20] 基础反应式 - 多步反应', formula: '\\schemestart A\\arrow{->}B\\arrow{->}C\\arrow{->}D\\schemestop' },
    { description: '[20] 箭头类型 - 正向->', formula: '\\schemestart A\\arrow{->}B\\schemestop' },
    { description: '[20] 箭头类型 - 逆向<-', formula: '\\schemestart A\\arrow{<-}B\\schemestop' },
    { description: '[20] 箭头类型 - 双向<->', formula: '\\schemestart A\\arrow{<->}B\\schemestop' },
    { description: '[20] 箭头类型 - 平衡<=>', formula: '\\schemestart A\\arrow{<=>}B\\schemestop' },
    { description: '[20] 箭头类型 - 不可见0', formula: '\\schemestart A\\arrow{0}B\\schemestop' },
    { description: '[20] 箭头标签 - 上下标签', formula: '\\schemestart A\\arrow{->[up][down]}B\\schemestop' },
    { description: '[20] 箭头标签 - 平衡标签', formula: '\\schemestart A\\arrow{<=>[up][down]}B\\schemestop' },
    { description: '[20] 箭头标签 - 正偏移4pt', formula: '\\schemestart A\\arrow{->[up][down][4pt]}B\\schemestop' },
    { description: '[20] 箭头标签 - 负偏移-4pt', formula: '\\schemestart A\\arrow{->[up][down][-4pt]}B\\schemestop' },
    { description: '[20] 箭头标签 - 标签+角度+系数', formula: '\\schemestart A\\arrow{<=>[up][down]}[30,1.5]B\\schemestop' },
    { description: '[20] 箭头方向 - 角度', formula: '\\schemestart A\\arrow[45]B\\arrow[-20]C\\schemestop' },
    { description: '[20] 箭头方向 - 长度系数', formula: '\\schemestart A\\arrow[,2]B\\arrow[,0.5]C\\schemestop' },
    { description: '[20] 箭头方向 - 样式', formula: '\\schemestart A\\arrow[,,red,thick]B\\arrow[,,dashed]C\\schemestop' },
    { description: '[20] 箭头方向 - 组合角度+系数+样式', formula: '\\schemestart A\\arrow[45,1.5,blue]B\\arrow[-20,2,dashed]C\\schemestop' },
    { description: '[20] 箭头方向 - 全局默认setchemfig', formula: '\\setchemfig{arrow angle=15,arrow coeff=1.5,arrow style={red, thick}}\\schemestart A\\arrow B\\arrow C\\schemestop\\setchemfig{arrow angle={},arrow coeff={},arrow style={}}' },
    { description: '[20] +号 - 基本加号', formula: '\\schemestart A\\+B\\+C\\arrow D\\+E\\schemestop' },
    { description: '[20] +号 - 自定义间距', formula: '\\schemestart A\\+B\\+{2em,,5pt}C\\+{0pt,0pt,-5pt}D\\arrow E\\+F\\schemestop' },
    { description: '[20] +号 - 全局间距setchemfig', formula: '\\setchemfig{+ sep left=1em,+ sep right=1em,+ vshift=0pt}\\schemestart A\\+B\\+{2em,,5pt}C\\+{0pt,0pt,-5pt}D\\arrow E\\+F\\schemestop\\setchemfig{+ sep left={},+ sep right={},+ vshift={}}' },
    { description: '[20] 化合物名称 - 名称显示', formula: '\\schemestart \\chemname{\\chemfig{R-C(-[:-30]OH)=[:30]O}}{Carboxylic acid} \\+ \\chemname{\\chemfig{R\'OH}}{Alcohol} \\arrow(.mid east--.mid west) \\chemname{\\chemfig{R-C(-[:-30]OR\')=[:30]O}}{Ester} \\+ \\chemname{\\chemfig{H_2O}}{Water} \\schemestop\\chemnameinit{}' },
    { description: '[20] 化合物名称 - 名称对齐chemnameinit', formula: '\\chemnameinit{\\chemfig{R-C(-[:-30]OH)=[:30]O}}\\schemestart \\chemname{\\chemfig{R\'OH}}{Alcohol} \\+ \\chemname{\\chemfig{R-C(-[:-30]OH)=[:30]O}}{Carboxylic acid} \\arrow(.mid east--.mid west) \\chemname{\\chemfig{R-C(-[:-30]OR\')=[:30]O}}{Ester} \\+ \\chemname{\\chemfig{H_2O}}{Water} \\schemestop\\chemnameinit{}' },
    { description: '[20] 分支 - 锚点最简常用版', formula: '\\schemestart A\\arrow(aa--bb)B\\arrow(--cc)C \\arrow(@bb--)[-90]D\\schemestop' },
    { description: '[20] 分支 - 经典常用二分支', formula: '\\schemestart A\\arrow B\\arrow[-60]C B\\arrow[60]D\\schemestop' },
    { description: '[20] 分支 - 从已有化合物分支', formula: '\\schemestart A\\arrow(aa--bb)B\\arrow(--cc)C\\arrow D \\arrow(@bb--xx1)[-90]X\\arrow[-90]Y \\arrow(@c4--)[-90]Z\\arrow W \\arrow(@xx1--xx2)[-45]XX \\arrow(@xx2--@c4)\\schemestop' },
    { description: '[20] 分支 - 不可见箭头对齐', formula: '\\schemestart A\\arrow(aa--bb)B\\arrow(--cc)C\\arrow D \\arrow(@bb--xx1)[-90]X\\arrow[-90]Y\\arrow(--xx2){0}XX \\arrow(@c4--)[-90]Z\\arrow W \\arrow(@xx1--@xx2) \\arrow(@xx2--@c4)\\schemestop' },
    { description: '[20] 锚定 - 默认中心锚点', formula: '\\schemestart \\chemfig{A*5(-----)} \\arrow \\chemfig{A*5(---(-)--)} \\schemestop' },
    { description: '[20] 锚定 - base锚点对齐', formula: '\\schemestart \\chemfig{A*5(-----)} \\arrow(.base east--.base west) \\chemfig{A*5(---(-)--)} \\schemestop' },
    { description: '[20] 子方案 - 基本子方案', formula: '\\schemestart A\\arrow \\subscheme{B\\arrow[-90,2]C} \\arrow D\\schemestop' },
    { description: '待调试[20] 子方案 - 定界符', formula: '\\schemestart A\\arrow \\chemleft[\\subscheme{B\\arrow[-90,2]C}\\chemright] \\arrow D\\schemestop' },
    { description: '[20] 电子移动 - 中介效应', formula: '\\schemestart \\chemfig{@{a1}=_[@{db}::30]-[::-60]\\charge{90=\\|}{X}} \\arrow{<->} \\chemfig{\\chemabove{\\vphantom{X}}{\\ominus}-[::30]=_[::-60]\\chemabove{X}{\\scriptstyle\\oplus}} \\schemestop\\chemmove{\\draw(db).. controls +(100:5mm) and +(145:5mm).. (a1);}' },
    { description: '[20] 电子移动 - 羰基共振', formula: '\\schemestart \\chemfig{C([3]-)([5]-)=[@{db,.5}]@{atoo}\\charge{0=\\|,-90=\\|}{O}} \\arrow(.mid east--.mid west){<->} \\chemfig{\\charge{90:3pt=$\\scriptstyle\\oplus$}{C}([3]-)([5]-)-\\charge{0=\\|,90=\\|,-90=\\|,45:3pt=$\\scriptstyle\\ominus$}{O}} \\schemestop\\chemmove{\\draw[shorten <=2pt, shorten >=2pt](db) ..controls +(up:5mm) and +(up:5mm)..(atoo);}' },
    { description: '[20] 全局配置 - 箭头系数', formula: '\\setchemfig{arrow coeff=2}\\schemestart A\\arrow{->}B\\schemestop\\setchemfig{arrow coeff={}}' },
    { description: '[20] 全局配置 - 化合物间距', formula: '\\setchemfig{compound sep=2em}\\schemestart A\\arrow{->}B\\schemestop\\setchemfig{compound sep={}}' },
    { description: '[20] 全局配置 - 箭头偏移', formula: '\\setchemfig{arrow offset=10pt}\\schemestart A\\arrow{->}B\\schemestop\\setchemfig{arrow offset={}}' },
    { description: '[待调试][20] 全局配置 - 箭头标签间距', formula: '\\setchemfig{arrow label sep=8pt}\\schemestart A\\arrow{->[above][below]}B\\schemestop\\setchemfig{arrow label sep={}}' },
    { description: '[20] 全局配置 - 箭头头部样式', formula: '\\setchemfig{arrow head=-Stealth}\\schemestart A\\arrow{->}B\\schemestop\\setchemfig{arrow head={}}' },
    { description: '[20] 全局配置 - 双箭头参数', formula: '\\setchemfig{arrow double sep=4pt,arrow double coeff=0.6,arrow double harpoon=true}\\schemestart A\\arrow{<=>}B\\arrow{<->>}C\\schemestop\\setchemfig{arrow double sep={},arrow double coeff={},arrow double harpoon={}}' },
    { description: '[20] 全局配置 - 重置resetchemfig', formula: '\\resetchemfig\\schemestart A\\arrow{->}B\\schemestop' },

    { description: '[21] 箭头类型 - 长平衡<<->', formula: '\\schemestart A\\arrow{<<->}B\\schemestop' },
    { description: '[21] 箭头类型 - 交替平衡<->>', formula: '\\schemestart A\\arrow{<->>}B\\schemestop' },
    { description: '[21] 箭头类型 - 弯钩右-/>', formula: '\\schemestart A\\arrow{-/>}B\\schemestop' },
    { description: '[21] 箭头类型 - 弯钩左</-', formula: '\\schemestart A\\arrow{</-}B\\schemestop' },
    { description: '[21] 箭头类型 - 鱼叉右-..>', formula: '\\schemestart A\\arrow{-..>}B\\schemestop' },
    { description: '[21] 箭头类型 - 鱼叉左<..-', formula: '\\schemestart A\\arrow{<..-}B\\schemestop' },
    { description: '[21] 箭头类型 - 虚线正向-->', formula: '\\schemestart A\\arrow{-->}B\\schemestop' },
    { description: '[21] 箭头类型 - 虚线平衡<=>>', formula: '\\schemestart A\\arrow{<=>>}B\\schemestop' },
    { description: '[21] 箭头类型 - 弧形正向->>', formula: '\\schemestart A\\arrow{->>}B\\schemestop' },
    { description: '[21] 箭头类型 - 弧形逆向<-<', formula: '\\schemestart A\\arrow{<-<}B\\schemestop' },
    { description: '[21] 箭头类型 - 弧形正向别名~>', formula: '\\schemestart A\\arrow{~>}B\\schemestop' },
    { description: '[21] 箭头类型 - 弧形逆向别名<~', formula: '\\schemestart A\\arrow{<~}B\\schemestop' },
    { description: '[21] 箭头类型 - U形正向-U>', formula: '\\schemestart A\\arrow{-U>}B\\schemestop' },
    { description: '[21] 箭头类型 - U形逆向<U-', formula: '\\schemestart A\\arrow{<U-}B\\schemestop' },
    { description: '[21] 箭头类型 - U形双向<U>', formula: '\\schemestart A\\arrow{<U>}B\\schemestop' },
    { description: '[21] 箭头类型 - 鱼钩<>', formula: '\\schemestart A\\arrow{\\<\\>}B\\schemestop' },

    { description: '[21] 合并箭头 - 向下合并v', formula: '\\schemestart A\\arrow B\\arrow[-90] C \\merge v(c2)(c3)--(D) \\schemestop' },
    { description: '[21] 合并箭头 - 向上合并V', formula: '\\schemestart A\\arrow B\\arrow[90] C \\merge ^(c2)(c3)--(D) \\schemestop' },
    { description: '[21] 合并箭头 - 向右合并h', formula: '\\schemestart A\\arrow[90] B\\arrow[-90] C \\merge >(c2)(c3)--(D) \\schemestop' },
    { description: '[21] 合并箭头 - 向左合并<', formula: '\\schemestart A\\arrow[90] B\\arrow[-90] C \\merge <(c2)(c3)--(D) \\schemestop' },

    { description: '[21] 电荷 - 自定义标记oplus', formula: '\\chemfig{\\charge{90=$\\oplus$}{N}}' },
    { description: '[21] 电荷 - 自定义标记ominus', formula: '\\chemfig{\\charge{90=$\\ominus$}{O}}' },
    { description: '[21] 电荷 - 自定义标记加号', formula: '\\chemfig{\\charge{90=$+$}{N}}' },
    { description: '[21] 电荷 - 自定义标记减号', formula: '\\chemfig{\\charge{90=$-$}{O}}' },
    { description: '[21] 电荷 - 四方向完整标记', formula: '\\chemfig{\\charge{90=\\:, 270=\\:, 0=\\|, 180=\\|}{O}}' },
    { description: '[21] 电荷 - 氮原子孤对+单电子', formula: '\\chemfig{\\charge{90=\\:, 270=\\:, 0=\\., 180=\\.}{N}}' },

    { description: '[21] 箭头标签 - 含化学式H2O', formula: '\\schemestart A\\arrow{->[$\\text{H}_2\\text{O}$]}B\\schemestop' },
    { description: '[21] 箭头标签 - 含条件Delta', formula: '\\schemestart A\\arrow{->[$\\Delta$]}B\\schemestop' },
    { description: '[21] 箭头标签 - 上下含化学式', formula: '\\schemestart A\\arrow{->[$\\text{H}_2\\text{SO}_4$][$\\Delta$]}B\\schemestop' },
    { description: '[21] 箭头标签 - 平衡含常数K', formula: '\\schemestart A\\arrow{<=>[$K_{eq}$]}B\\schemestop' },

    { description: '[21] 箭头样式 - 红色虚线', formula: '\\schemestart A\\arrow[,,red,dashed]B\\schemestop' },
    { description: '[21] 箭头样式 - 蓝色粗线', formula: '\\schemestart A\\arrow[,,blue,thick]B\\schemestop' },
    { description: '[21] 箭头样式 - 绿色线宽', formula: '\\schemestart A\\arrow[,,green,line width=2pt]B\\schemestop' },

    { description: '[21] 实际反应 - 酯化反应', formula: '\\schemestart \\chemname{\\chemfig{R-C(-[:-30]OH)=[:30]O}}{酸} \\+ \\chemname{\\chemfig{R\'OH}}{醇} \\arrow{->[$\\text{H}^+$][$\\Delta$]} \\chemname{\\chemfig{R-C(-[:-30]OR\')=[:30]O}}{酯} \\+ \\chemname{\\chemfig{H_2O}}{水} \\schemestop' },
    { description: '[21] 实际反应 - 苯硝化反应', formula: '\\schemestart \\chemfig{*6(=-=(-H)-=-)} \\+ \\chemfig{HNO_3} \\arrow{->[$\\text{H}_2\\text{SO}_4$]} \\chemfig{*6(=-=(-NO_2)-=-)} \\+ \\chemfig{H_2O} \\schemestop' },
    { description: '[21] 实际反应 - 乙醛氧化', formula: '\\schemestart \\chemfig{CH_3-[:30]C(=[2]O)-[:-30]H} \\arrow{->[\\text{O}_2][\\text{催化剂}]} \\chemfig{CH_3-[:30]C(=[2]O)-[:-30]OH} \\schemestop' },
  ]

  private getPageItems(): ChemfigItem[] {
    let start = this.currentPage * this.pageSize
    let end = Math.min(start + this.pageSize, this.items.length)
    return this.items.slice(start, end)
  }

  private getTotalPages(): number {
    return Math.ceil(this.items.length / this.pageSize)
  }

  build() {
    Column() {
      Scroll() {
        Column() {
          ForEach(this.getPageItems(), (item: ChemfigItem, index: number) => {
            Column() {
              ChemfigCard({ item: item, index: this.currentPage * this.pageSize + index + 1 })
              Divider().strokeWidth(0.5).color('#EEEEEE').margin({ left: 16, right: 16 })
            }
          }, (item: ChemfigItem, index: number) => `${this.currentPage}_${index}`)
        }
        .width('100%')
      }
      .layoutWeight(1)
      .scrollBar(BarState.Auto)
      .backgroundColor(Color.White)

      Row() {
        Button('上一页')
          .enabled(this.currentPage > 0)
          .onClick(() => {
            if (this.currentPage > 0) {
              this.currentPage--
            }
          })
          .fontSize(14)
          .height(36)
          .opacity(this.currentPage > 0 ? 1 : 0.4)

        Text(`${this.currentPage + 1} / ${this.getTotalPages()}`)
          .fontSize(14)
          .fontColor('#333333')
          .margin({ left: 16, right: 16 })

        Button('下一页')
          .enabled(this.currentPage < this.getTotalPages() - 1)
          .onClick(() => {
            if (this.currentPage < this.getTotalPages() - 1) {
              this.currentPage++
            }
          })
          .fontSize(14)
          .height(36)
          .opacity(this.currentPage < this.getTotalPages() - 1 ? 1 : 0.4)
      }
      .width('100%')
      .height(56)
      .justifyContent(FlexAlign.Center)
      .alignItems(VerticalAlign.Center)
      .backgroundColor(Color.White)
      .border({ width: { top: 0.5 }, color: '#EEEEEE' })
    }
    .width('100%')
    .height('100%')
  }
}
