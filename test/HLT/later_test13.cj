// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class later_test {
    @TestCase
    func later_test013(): Unit {                

    var latex = LaTeX("res")
    var str = ###"
\mathbf{V}_1 \times \mathbf{V}_2 =  
\begin{vmatrix}  
  \mathbf{i}& \mathbf{j}& \mathbf{k} \\  
  \frac{\partial X}{\partial u}& \frac{\partial Y}{\partial u}& 0 \\  
  \frac{\partial X}{\partial v}& \frac{\partial Y}{\partial v}& 0 \\  
\end{vmatrix} 
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