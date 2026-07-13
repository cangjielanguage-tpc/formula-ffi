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
    var t = c_test_test()
    t.c_testr_012()
    return ret
}

@Test
public class c_test_test {
    @TestCase
    func c_testr_012(): Unit {
        var latex = LaTeX("res")
        var str = ###"
\cfrac{1}{\sqrt{2}+
\cfrac{1}{\sqrt{2}+
\cfrac{1}{\sqrt{2}+\dotsb
}}}
\\
\biggl[
\sum_i a_i\biggl\lvert\sum_j x_{ij}
\biggr\rvert^p\biggr]^{1/p}
\\
\biggl[
\sum_i a_i\Bigl\lvert\sum_j x_{ij}
\Bigr\rvert^p\biggr]^{1/p}
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
