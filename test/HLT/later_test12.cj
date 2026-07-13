// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class later_test {
    @TestCase
    func later_test012(): Unit {                

    var latex = LaTeX("res")
    var str = ###"
\begin{array}{c} 
  H_{n}=\frac{n}{\sum \limits_{i=1}^{n}\frac{1}{x_{i}}}= \frac{n}{\frac{1}{x_{1}}+ \frac{1}{x_{2}}+ \cdots + \frac{1}{x_{n}}} \\ G_{n}=\sqrt[n]{\prod \limits_{i=1}^{n}x_{i}}= \sqrt[n]{x_{1}x_{2}\cdots x_{n}} \\ A_{n}=\frac{1}{n}\sum \limits_{i=1}^{n}x_{i}=\frac{x_{1}+ x_{2}+ \cdots + x_{n}}{n} \\ Q_{n}=\sqrt{\sum \limits_{i=1}^{n}x_{i}^{2}}= \sqrt{\frac{x_{1}^{2}+ x_{2}^{2}+ \cdots + x_{n}^{2}}{n}} \\ H_{n}\leq G_{n}\leq A_{n}\leq Q_{n} 
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