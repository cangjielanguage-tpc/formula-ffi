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
    t.later_test01()
    return ret
}

@Test
public class c_test_test {
    @TestCase
    func later_test01(): Unit {
        var latex = LaTeX("res")
        var str = ###"
\begin{array}{lr}\mbox{\textcolor{Blue}{Russian}}&\mbox{\textcolor{Melon}{Greek}}\\\mbox{ПРИВЕТ МИР}&\mbox{ΓΕΙΆ ΚΌΣΜΟ}\\\mbox{привет мир}&\mbox{γειά κόσμο}\\\mathbf{\mbox{привет мир}}&\mathbf{\mbox{γειά κόσμο}}\\\mathit{\mbox{привет мир}}&\mathit{\mbox{γειά κόσμο}}\\\mathsf{\mbox{привет мир}}&\mathsf{\mbox{γειά κόσμο}}\\\mathtt{\mbox{привет мир}}&\mathtt{\mbox{γειά κόσμο}}\\\mathbf{\mathit{\mbox{привет мир}}}&\mathbf{\mathit{\mbox{γειά κόσμο}}}\\\mathbf{\mathsf{\mbox{привет мир}}}&\mathbf{\mathsf{\mbox{γειά κόσμο}}}\\\mathsf{\mathit{\mbox{привет мир}}}&\mathsf{\mathit{\mbox{γειά κόσμο}}}\\&\\\mbox{\textcolor{Salmon}{Bulgarian}}&\mbox{\textcolor{Tan}{Serbian}}\\\mbox{здравей свят}&\mbox{Хелло уорлд}\\&\\\mbox{\textcolor{Turquoise}{Bielorussian}}&\mbox{\textcolor{LimeGreen}{Ukrainian}}\\\mbox{прывітаньне Свет}&\mbox{привіт світ}\\\end{array}
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
