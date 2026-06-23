package ohos_app_cangjie_entry

import ohos.base.*
import ohos.component.*
import ohos.state_manage.*
import ohos.state_macro_manage.*
import ohos.image.PixelMap
import ohos.image.ImageSource
import ohos.image.createImageSource
import ohos.resource_manager.ResourceManager
import std.collection.ArrayList
import formula.*

class FormulaItem {
    var fileName: String = ""
    var formulaText: String = ""
    var pixelMap: Option<PixelMap> = Option<PixelMap>.None
    var width: Int32 = 0
    var height: Int32 = 0
    var loaded: Bool = false
    var errorMessage: String = ""
    var resultCode: TeXResultCode = TeXResultCode.UnknownError
}

func getLatexmathWidth(strLatexMath: String): Int32 {
    if (strLatexMath.contains("aligned") || strLatexMath.contains("align") || 
        strLatexMath.contains("flalign") || strLatexMath.contains("gather") || 
        strLatexMath.contains("multline") || strLatexMath.contains("eqnarray") || 
        strLatexMath.contains("equation")) {
        Int32(10)
    } else {
        Int32(2000)
    }
}

@Entry
@Component
class EntryView {
    @State
    var formulaItems: Array<FormulaItem> = []
    @State
    var rendering: Bool = false

    public override func aboutToAppear(): Unit {
        rendering = true
        renderAllFormulas()
        rendering = false
    }

    func renderAllFormulas(): Unit {
        var items = ArrayList<FormulaItem>()
        let contextOpt = globalContext
        if (let Some(context) <- contextOpt) {
            let resMgr = context.resourceManager
            let files = resMgr.getRawFileList("")
            if (files.size == 0) { return }
            for (i in 0..files.size-1) {
                let fileName = files[i]
                if (!fileName.endsWith(".txt")) {
                    continue
                }
                var item = FormulaItem()
                item.fileName = fileName
                var latexOpt: ?LaTeX = None
                var rOpt: ?Render = None
                var g2Opt: ?Graphic2D = None
                try {
                    let content = resMgr.getRawFileContent(fileName)
                    let str = String.fromUtf8(content)
                    item.formulaText = str
                    let latex = LaTeX("/data/storage/el1/bundle/entry/resources/resfile/res")
                    latexOpt = latex
                    var r = latex.parse(str, 2000, 15.0, 10.0, 0xFF000000)
                    rOpt = r
                    var g2 = Graphic2D(r, COLOR_FORMAT_RGB_565)
                    g2Opt = g2
                    r.draw(g2, 0xFFFFFFFF)
                    var arr = r.toBitmap(g2)
                    let imageSourceApi: ImageSource = createImageSource(arr)
                    let pixelMap: Option<PixelMap> = imageSourceApi.createPixelMap()
                    if (let Some(p) <- pixelMap) {
                        item.width = p.getImageInfo().size.width
                        item.height = p.getImageInfo().size.height
                        item.pixelMap = p
                        item.loaded = true
                    }
                } catch (e: TeXParseException) {
                    AppLog.error("Failed to render ${fileName}: ${e.resultCode.toString()} - ${e.message}")
                    item.errorMessage = "${e.resultCode.toString()}: ${e.message}"
                    item.resultCode = e.resultCode
                } catch (e: Exception) {
                    AppLog.error("Failed to render ${fileName}: ${e.message}")
                    item.errorMessage = e.message
                } finally {
                    if (let Some(g2) <- g2Opt) {
                        g2.release()
                    }
                    if (let Some(r) <- rOpt) {
                        r.finalize()
                    }
                    if (let Some(latex) <- latexOpt) {
                        latex.release()
                    }
                }
                items.add(item)
            }
        }
        formulaItems = items.toArray()
    }

    func build() {
        Row {
            Column(10) {
                if (rendering) {
                    Text("Rendering...")
                        .fontSize(18)
                        .margin(top: 50)
                } else if (formulaItems.size > 0) {
                    Scroll() {
                        Column(10) {
                            ForEach(
                                formulaItems,
                                itemGeneratorFunc: { item: FormulaItem, index: Int64 =>
                                    Column(5) {
                                        Text(item.fileName)
                                            .fontSize(14)
                                            .fontColor(0xFF666666)
                                        if (item.loaded) {
                                            Image(item.pixelMap.getOrThrow())
                                                .height(Int64(item.height))
                                                .width(Int64(item.width))
                                        } else {
                                            Text(item.formulaText)
                                                .fontSize(12)
                                                .fontColor(0xFFFF0000)
                                            if (item.errorMessage.size > 0) {
                                                Text("Error: ${item.errorMessage}")
                                                    .fontSize(10)
                                                    .fontColor(0xFFFF6666)
                                            }
                                        }
                                    }.width(100.percent).padding(10)
                                },
                                keyGeneratorFunc: { item: FormulaItem, index: Int64 =>
                                    item.fileName
                                }
                            )
                        }.width(100.percent)
                    }.width(100.percent).layoutWeight(1)
                } else {
                    Text("No files found")
                        .fontSize(18)
                        .margin(top: 50)
                }
            }.width(100.percent)
        }.height(100.percent)
    }
}
