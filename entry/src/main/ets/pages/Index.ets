import router from '@ohos.router'

@Entry
@Component
struct Index {
  build() {
    Scroll() {
      Column() {
        Row() {
          Text("")
            .width(10)
          Button("0、演示样例")
            .height(50)
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index0' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("1、主题背景白色。字体12fp。字体颜色黑色。公式背景白色。图片格式RGB_565")
            .height(50)
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index1' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("2、主题背景白色。字体30fp。字体颜色黑色。公式背景白色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index2' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("3、主题背景白色。字体12fp。字体颜色红色。公式背景白色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index3' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("4、主题背景白色。字体12fp。字体颜色绿色。公式背景白色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index4' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("5、主题背景白色。字体12fp。字体颜色蓝色。公式背景白色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index5' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("6、主题背景黑色。字体12fp。字体颜色白色。公式背景黑色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index6' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("7、主题背景白色。字体18fp。字体颜色黑色。公式背景白色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index7' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("8、主题背景白色。字体12fp。字体颜色白色。公式背景黑色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index8' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("9、主题背景白色。字体30fp。字体颜色黑色。公式背景透明。图片格式BGRA_8888")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index9' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("10、主题背景白色。字体30fp。字体颜色白色。公式背景黑色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index10' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("11、主题背景黑色。字体30fp。字体颜色黑色。公式背景白色。图片格式RGB_565")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index11' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("12、主题背景变色。字体30fp。字体颜色红色。公式背景透明。图片格式BGRA_8888")
            .height(50)// 高度
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index12' })
            })
          Text("")
            .width(10)
        }
        .width('100%')

        Row() {
          Text("")
            .width(10)
          Button("13、LaTeX异常公式检测演示")
            .height(50)
            .layoutWeight(1)
            .fontColor(0xFFFFFFFF)
            .backgroundColor(0xFFFF0000)
            .margin({ top: 5, bottom: 5 })
            .align(Alignment.Center)
            .fontSize(10)
            .onClick(e => {
              router.pushUrl({ url: 'pages/Index13' })
            })
          Text("")
            .width(10)
        }
        .width('100%')
      }
      .width('100%')
      .alignItems(HorizontalAlign.Start)
    }
    .scrollBar(BarState.Off)
  }
}

