// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class later_test {
    @TestCase
    func later_test04(): Unit {

    var latex = LaTeX("res")
    var str = ###"
\begin{array}{|c|c|c|c|}
\multicolumn{4}{c}{\shadowbox{\text{\Huge An image from the \LaTeX3 project}}}\cr
\hline
\text{Left}\includegraphics{src/test/resources/lion.png}\text{Right} & \text{Left}\includegraphics[width=3cm,interpolation=bicubic]{src/test/resources/lion.png}\text{Right} & \text{Left}\includegraphics[angle=45,width=3cm]{src/test/resources/lion.png}\text{Right} & \text{Left}\includegraphics[angle=160]{src/test/resources/lion.png}\text{Right} \cr
\hline
\text{\backslash includegraphics\{src/test/resources/lion.png\}} & \text{\backslash includegraphics[width=3cm,interpolation=bicubic]\{lion.png\}} & \text{\backslash includegraphics[angle=45,width=3cm]\{lion.png\}} & \text{\backslash includegraphics[angle=160]\{lion.png\}}\cr
\hline
\end{array}
"###
    var r = latex.parse(str, 2000000000, 40.0, 10.0, 0xFF000000)       
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