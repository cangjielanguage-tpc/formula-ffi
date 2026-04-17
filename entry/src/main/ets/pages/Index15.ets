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
        this.item.formula, fp2px(40), 0xFF000000, 0xFFFFFFFF,
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
        Image(this.pixelMap)
          .objectFit(ImageFit.Contain)
          .width(Math.min(this.imgWidth, 320))
          .height(this.imgHeight * Math.min(1, 320 / this.imgWidth))
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
    // { description: '[6] 上下标 - 上标电荷+', formula: '\\chemfig{A^{+}}' },
    // { description: '[6] 上下标 - 上标电荷-', formula: '\\chemfig{A^{-}}' },
    // { description: '[6] 上下标 - 上标电荷2+', formula: '\\chemfig{A^{2+}}' },
    // { description: '[6] 上下标 - 电荷链A+-B-', formula: '\\chemfig{A^{+}-B^{-}}' },
    // { description: '[6] 上下标 - 上下标混合A2+3', formula: '\\chemfig{A^{2+}_3}' },
    // { description: '[6] 上下标 - 上下标链A1^2-B2^1', formula: '\\chemfig{A_1^2-B_2^1}' },
    // { description: '[6] 上下标 - 氧离子O2-', formula: '\\chemfig{O^{2-}}' },
    // { description: '[6] 上下标 - 复杂下标(aq)', formula: '\\chemfig{A_{(aq)}}' },
    // { description: '[6] 上下标 - 复杂下标n+1', formula: '\\chemfig{A_{n+1}}' },

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

    // { description: '[12] 钩子 - 简单锚点?[a]', formula: '\\chemfig{A?[a]-B?[a]}' },
    // { description: '[12] 钩子 - 多锚点?[x]?[y]', formula: '\\chemfig{A?[x]-B?[y]-C?[x]-D?[y]}' },
    // { description: '[12] 钩子 - 数字锚点?[1]?[2]', formula: '\\chemfig{A?[1]-B?[2]-C?[1]-D?[2]}' },
    //
    // { description: '[13] 锚点 - 原子锚点@{n1}', formula: '\\chemfig{A@{n1}-B@{n2}-C@{n3}}' },
    // { description: '[13] 锚点 - 键位置锚点@{mid1}', formula: '\\chemfig{A-[@{mid1},1]B-[@{mid2},2]C}' },
    // { description: '[13] 锚点 - 键位置偏移@{p1,0.3}', formula: '\\chemfig{A-[@{p1,0.3},1]B}' },

    // { description: '[14] 键偏移 - #(5pt)', formula: '\\chemfig{A-#(5pt)B}' },
    // { description: '[14] 键偏移 - #(10pt)链', formula: '\\chemfig{A-#(10pt)B-#(10pt)C}' },
    // { description: '[14] 键偏移 - 非对称#(10pt,5pt)', formula: '\\chemfig{A-#(10pt,5pt)B}' },
    // { description: '[14] 键偏移 - 非对称#(5pt,10pt)', formula: '\\chemfig{A-#(5pt,10pt)B}' },
    // { description: '[14] 键偏移 - 双键偏移', formula: '\\chemfig{A=#(5pt)B}' },

    { description: '[18] 真实分子 - 甲烷', formula: '\\chemfig{C(-[2]H)(-[4]H)(-[6]H)(-[8]H)}' },
    { description: '[18] 真实分子 - 乙醇', formula: '\\chemfig{CH_3-CH_2-OH}' },
    { description: '[18] 真实分子 - 乙酸', formula: '\\chemfig{CH_3-C(=[1]O)(-[7]OH)}' },
    { description: '[18] 真实分子 - 苯', formula: '\\chemfig{*6(=-=-=-)}' },
    { description: '[18] 真实分子 - 葡萄糖', formula: '\\chemfig{*6((=O)-(-[2]OH)-(-[2]OH)-(-[2]OH)-(-[2]OH)-)}' },
    { description: '[18] 真实分子 - 咖啡因', formula: '\\chemfig{*6((=O)-N(-CH_3)-=N(-CH_3)-=)}' },
    { description: '[18] 真实分子 - 阿司匹林', formula: '\\chemfig{*6(=-(-COOH)=(-OC(=O)CH_3)-=)}' },
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
