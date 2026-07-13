// 3rd_party_lib:formula/target/release/formula_markdown
//3rd_party_lib_ohos:formula\target\aarch64-linux-ohos\release\formula_markdown
import formula_markdown.*
import std.unittest.*
import std.collection.*
import std.fs.*

@Test
public class c_test_test {               //信号5异常
    @TestCase
    func c_testr_09(): Unit {
    var latex = LaTeX("res")
    var str = ###"
\left\{
    \begin{array}{l}
    2a < -1,\\
    a + 8 \ge 5,
    \end{array}
\right.
\\
P_{r-j}=\begin{cases}
0& \text{if $r-j$ is odd},\\
r!\,(-1)^{(r-j)/2}& \text{if $r-j$ is even}.
\end{cases}\text{Cases}
\\
P_{r-j}=\left\{\begin{array}{@{}ll@{\,}}
0& \text{if $r-j$ is odd},\\
r!\,(-1)^{(r-j)/2}& \text{if $r-j$ is even}.
\end{array}\right.\text{Cases}
\\
P_{r-j}=\begin{cases}
4-x\geq 0 \\
3-x\geq 1
\end{cases}\text{Cases}
\\
\left\{\begin{array}{@{}ll}
1 & 2\\
3 & 4
\end{array}\right.\text{Equation}
\\
\left\{\begin{array}{l@{}l}
1 & 2\\
3 & 4
\end{array}\right.\text{Equation}
\\
\left\{\begin{array}{ll@{}}
1 & 2\\
3 & 4
\end{array}\right.\text{Equation}
\\
\begin{split}
H_c&=\frac{1}{2n} \sum^n_{l=0}(-1)^{l}(n-{l})^{p-2}
\sum_{l _1+\dots+ l _p=l}\prod^p_{i=1} \binom{n_i}{l _i}\\
&\quad\cdot[(n-l )-(n_i-l _i)]^{n_i-l _i}\cdot
\Bigl[(n-l )^2-\sum^p_{j=1}(n_i-l _i)^2\Bigr].
\end{split}
\\
\begin{align}
A_1&=N_0(\lambda;\Omega’)-\phi(\lambda;\Omega’),\\
A_2&=\phi(\lambda;\Omega’)-\phi(\lambda;\Omega),\\
\intertext{and}
A_3&=\mathcal{N}(\lambda;\omega).
\end{align}
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