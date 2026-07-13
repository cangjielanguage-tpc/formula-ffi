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
    t.c_testr_010()
    return ret
}

@Test
public class c_test_test {
    @TestCase
    func c_testr_010(): Unit {
        var latex = LaTeX("res")
        var str = ###"
\frac{\sum_{n > 0} z^n}
{\prod_{1\leq k\leq n} (1-q^k)}
\\
\frac{{\displaystyle\sum_{n > 0} z^n}}
{{\displaystyle\prod_{1\leq k\leq n} (1-q^k)}}
\\
\frac{{\displaystyle\sum\nolimits_{n> 0} z^n}}
{{\displaystyle\prod\nolimits_{1\leq k\leq n} (1-q^k)}}
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
