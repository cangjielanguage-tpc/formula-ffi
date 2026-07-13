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
    t.c_testr_08()
    return ret
}

@Test
public class c_test_test {
    @TestCase
    func c_testr_08(): Unit {
        var latex = LaTeX("res")
        var str = ###"
\rlap{\overbrace{\phantom{1 + a + b + \cdots + z}}^{\text{total + 1}}}
1 + \underbrace{a + b + \cdots + z}_{\text{total}}
\\
\frac{a\cancel{b}}{\cancel{b}} = a;
\frac{a\bcancel{b}}{\bcancel{b}} = a;
\frac{a\xcancel{b}}{\xcancel{b}} = a;
\\
\text{A long division: }\longdiv{12345}{13}
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
