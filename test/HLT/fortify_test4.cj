// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
// sources_file:..\..\lib\liblatex.so
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class c_test_test {
    @TestCase
    func c_testr_01(): Unit {
    var latex = LaTeX("res")
    var str = ###"
%split环境（用$对齐）(一个公式分为多行排版)
\begin{equation}
	\begin{split}
	\cos 2x &= \cos^2 x - \sin, \overset{\underset{\mathrm{def}}{}}{=}^2 x\\
	        &= 2\cos^2 x - 1, \simeq, \backsimeq,\not\ll \nleqq,,
	\end{split}
\end{equation}
"###
    var r = latex.parse(str, 200000, 40.0, 10.0, 0xFF000000)         
    var w = r.getWidth()
    var h = r.getHeight()
    println(w)
    println(h)
    @Assert(r.getTextSize(),40.0)  
    var g2 = Graphic2D(r,COLOR_FORMAT_BGRA_8888)
    println(g2.width)
    println(g2.height)
    r.draw(g2, 0xFFFFFFFF)
    var arr = r.toBitmap(g2)
    var file: File = File("test.bmp", OpenOption.CreateOrTruncate(false))
    file.write(arr)
    file.close()
}
}

