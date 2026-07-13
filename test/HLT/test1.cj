// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class test {
    @TestCase
    func test9(): Unit {

    var latex = LaTeX("res")
    var str = ###"
"###
    var color1:UInt32=0xffff0000
    var r = latex.parse(str, 10, 40.0, 10.0, color1)  
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