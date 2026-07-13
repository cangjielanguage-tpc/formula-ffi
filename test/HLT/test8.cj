// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class test {
    @TestCase
    func test8(): Unit {

    var latex = LaTeX("res")
    var str = ###"
\sideset{^\backprime}{'}
\sum_{x=1}^{\infty} x
\sideset{a_1^2}{}\sum_{x=1}^\infty x_0
\\
\sideset{_\text{left bottom}'''}{_{\text{right bottom}}'''} 
\sum_{\text{quite long text}}^\infty x
\\
\sideset{}{'}
\sum_{n<k,\;\text{$n$ odd}} nE_n
\\
\sideset{}{'}
\sum^{n<k,\;\text{$n$ odd}} nE_n
\\
M_x''' M'''_x M^{'''}_x M_x{'''} M^{\prime\backprime}
"###

    var r = latex.parse(str, 10, 40.0, 10.0, 0xFF000000)  //宽度问题
    var w = r.getWidth()
    var h = r.getHeight()
    println(w)
    println(h)
    var g2 = Graphic2D(r,COLOR_FORMAT_BGRA_8888)
    r.draw(g2,0xFFFFFFFF)
    var arr = r.toBitmap(g2)     
    var file: File = File("test.bmp", OpenOption.CreateOrTruncate(false))
    file.write(arr)
    file.close()
    }
}