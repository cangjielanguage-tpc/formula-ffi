import { ArrayList, util } from '@kit.ArkTS'
import { common } from '@kit.AbilityKit'
import { resourceManager } from '@kit.LocalizationKit'
import { ImageComponent } from './image_component'
import { LatexClass } from './latex_class'
import { LatexMathColorFormat } from '@cangjie-tpc/formula_hybrid'

@Entry
@Component
struct Index1 {
  private context = getContext(this) as common.UIAbilityContext
  @State strList: ArrayList<LatexClass> = new ArrayList<LatexClass>()

  aboutToAppear(): void {
    let manager: resourceManager.ResourceManager = this.context.resourceManager
    let files: string[] = manager.getRawFileListSync('')
    for (let index = 0; index < files.length; index++) {
      let content: Uint8Array = manager.getRawFileContentSync(files[index])
      let textDecoderOptions: util.TextDecoderOptions = {
        fatal: false,
        ignoreBOM: true
      }
      let decodeToStringOptions: util.DecodeToStringOptions = {
        stream: false
      }
      let textDecoder = util.TextDecoder.create('utf-8', textDecoderOptions)
      let retStr = textDecoder.decodeToString(content, decodeToStringOptions)
      this.strList.add(new LatexClass(retStr, fp2px(12), 0xFF000000, 0xFFFFFFFF,
        LatexMathColorFormat.COLOR_FORMAT_RGB_565))
    }
  }

  build() {
    Scroll() {
      Column() {
        ForEach(this.strList.convertToArray(), (item: LatexClass,index:number) => {
          ImageComponent(
            {
              latexMathTextString: item.latexMathTextString,
              latexMathTextSize: item.latexMathTextSize,
              latexMathTextColor: item.latexMathTextColor,
              latexMathBackGroupColor: item.latexMathBackGroupColor,
              latexMathColorFormat: item.latexMathColorFormat,
              index:index
            }
          ).margin({ top: 10, bottom: 10 })
        })
      }
      .width('100%')
      .alignItems(HorizontalAlign.Start)
    }
    .scrollBar(BarState.Off)
    .backgroundColor(Color.White)
  }
}

