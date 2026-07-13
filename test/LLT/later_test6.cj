// EXEC: cjc %import-path %L %l %f
// EXEC: ./main

import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*
import std.unittest.*
import std.unittest.testmacro.*

main(): Int64 {
    var ret = 0
    var t = later_test()
    t.later_test06()
    return ret
}

@Test
public class c_test_test {
    @TestCase
    func later_test06(): Unit {
        var latex = LaTeX("res")
        var str = ###"
\begin{array}{cc}\fbox{\text{A framed box with \textdbend}}&\shadowbox{\text{A shadowed box}}\cr\doublebox{\text{A double framed box}}&\ovalbox{\text{An oval framed box}}\cr\end{array}
"###
        var r = latex.parse(str, 2000000000, 40.0, 10.0, 0xFF000000)
        var w = r.getWidth()
        var h = r.getHeight()
        println(w)
        println(h)
        var g2 = Graphic2D(r, COLOR_FORMAT_BGRA_8888)
        r.draw(g2, 0xFFFFFFFF)

        var arr = r.toBitmap(g2)

        var file: File = File("test.bmp", OpenOption.CreateOrTruncate(false))
        file.write(arr)
        file.close()
    }
}
