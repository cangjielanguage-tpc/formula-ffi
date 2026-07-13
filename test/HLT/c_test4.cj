// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class c_test_test {
    @TestCase
    func c_testr_04(): Unit {
    var latex = LaTeX("res")
    var str = ###"
\begin{array}{|c|l|||r|c|}
  \hline
  \text{Matrix}&\multicolumn{2}{|c|}{\text{Multicolumns}}&\text{Font sizes commands}\cr
  \hline
  \begin{pmatrix}
      \alpha_{11}&\cdots&\alpha_{1n}\cr
      \hdotsfor{3}\cr
      \alpha_{n1}&\cdots&\alpha_{nn}
  \end{pmatrix}
  &\Large \text{Large Right}&\small \text{small Left}&\tiny \text{tiny Tiny}\cr
  \hline
  \multicolumn{4}{|c|}{\Huge \text{Huge Multicolumns}}\cr
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