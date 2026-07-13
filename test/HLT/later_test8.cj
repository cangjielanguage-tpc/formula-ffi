// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class later_test {
    @TestCase
    func later_test08(): Unit {

    var latex = LaTeX("res")
    var str = ###"
\text{A long division \longdiv{12345}{13}
"###
    var r = latex.parse(str, 2000000000, 40.0, 10.0, 0)        
    var w = r.getWidth()
    var h = r.getHeight()
    println(w)
    println(h)
     var g2 = Graphic2D(r,COLOR_FORMAT_BGRA_8888)
    r.draw(g2, 0xFFFFFFFF)

    var arr = r.toBitmap(g2)

    var file: File = File("test.bmp", OpenOption.CreateOrTruncate(false))
    file.write(arr)
    file.close()

    }
}