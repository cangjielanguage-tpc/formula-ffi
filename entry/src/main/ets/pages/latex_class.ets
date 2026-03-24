import { LatexMathColorFormat } from '@cangjie-tpc/formula_hybrid';

export class LatexClass {
  latexMathTextString: string = ""
  latexMathTextSize: number = 0
  latexMathTextColor: number = 0
  latexMathBackGroupColor: number = 0
  latexMathColorFormat: LatexMathColorFormat = LatexMathColorFormat.COLOR_FORMAT_RGB_565

  constructor(latexMathTextString: string, latexMathTextSize: number,
    latexMathTextColor: number, latexMathBackGroupColor: number,
    latexMathColorFormat: LatexMathColorFormat) {
    this.latexMathTextString = latexMathTextString
    this.latexMathTextSize = latexMathTextSize
    this.latexMathTextColor = latexMathTextColor
    this.latexMathBackGroupColor = latexMathBackGroupColor
    this.latexMathColorFormat = latexMathColorFormat
  }
}