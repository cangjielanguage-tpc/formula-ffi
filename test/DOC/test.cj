// EXEC: cjc %import-path %L %l %f
// EXEC: ./main

import formula_markdown.*
import std.fs.*

main(): Int64 {
    var latex = LaTeX("res")
    var str = ###"
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
"###
    var r = latex.parse(str, 2000, 40.0, 10.0, 0xFF000000)
    var w = r.getWidth()
    var h = r.getHeight()
    var g2 = Graphic2D(r, COLOR_FORMAT_BGRA_8888)
    r.draw(g2, 0xFFFFFFFF)

    var arr = r.toBitmap(g2)

    var file: File = File("test.bmp", OpenOption.CreateOrTruncate(false))
    file.write(arr)
    file.close()

    return 0
}