package formula.mhchem

/**
 * 将 MhchemParser 解析出的 AST 转换为 LaTeX 公式。
 */
class Texify {

    /**
     * 将 ParserValue 列表整体转换为 LaTeX 字符串。
     *
     * 遍历输入列表，如果是 PVString，直接取值；
     * 如果是 PVObject，则调用 _go2() 转换。
     * 对于非内部调用的化学式，如果未遇到 1st-level escape，则在结果外层加上 {}。
     *
     * @param input 输入 ParserValue 列表
     * @param isInner 是否是内部调用（避免外层再加 {}）
     * @return 转换后的 LaTeX 字符串
     */
    func go(input: ArrayList<ParserValue>, isInner!: Bool = false): String {
        if (input.size == 0) {
            return ""
        }

        var res: String = ""
        var cee: Bool = false

        for (i in 0..input.size) {
            var inputi = input[i]
            if (inputi is PVString && let Some(v) <- (inputi as PVString)) {
                res += v.getValue()
            } else if (inputi is PVObject && let Some(v) <- (inputi as PVObject)) {
                res += _go2(v)
                if (v.getType() == "1st-level escape") {
                    cee = true
                }
            }
        }

        if (!isInner && !cee && res.size > 0) {
            res = "{" + res + "}"
        }

        return res
    }

    /**
     * 内部调用 go() 时使用，标记 isInner 为 true。
     *
     * @param input 输入 ParserValue 列表
     * @return 转换后的 LaTeX 字符串
     */
    private func _goInner(input: ArrayList<ParserValue>): String {
        if (input.size == 0) {
            return ""
        }

        return go(input, isInner: true)
    }

    /**
     * 核心转换函数，将单个 PVObject 转为 LaTeX 字符串。
     *
     * 根据 PVObject 的类型（chemfive、rm、text、bond、arrow 等）生成对应 LaTeX。
     * 处理化学式的上下标、分数、颜色、箭头、运算符等。
     *
     * @param buffer 输入的 PVObject
     * @return 转换后的 LaTeX 字符串
     */
    private func _go2(buffer: ParserValue): String {
        var res: String = ""

        if (buffer is PVObject && let Some(buf) <- (buffer as PVObject)) {
            var p1 = buf.getString("p1")
            var p2 = buf.getString("p2")
            var kind_ = buf.getString("kind_")
            var r = buf.getString("r")
            var p1Arr = buf.getList("p1").getItems()
            var p2Arr = buf.getList("p2").getItems()

            match (buf.getType()) {
                case "chemfive" =>
                    res = ""
                    var a: String = _goInner(buf.getList("a").getItems())
                    var b: String = _goInner(buf.getList("b").getItems())
                    var p: String = _goInner(buf.getList("p").getItems())
                    var o: String = _goInner(buf.getList("o").getItems())
                    var q: String = _goInner(buf.getList("q").getItems())
                    var d: String = _goInner(buf.getList("d").getItems())

                    if (a != "") {
                        if (Regex("^[+-]").matches(a)) {
                            a = "{" + a + "}"
                        }
                        res += a + "\\,"
                    }

                    if (b != "" || p != "") {
                        res += "{\\vphantom{X}}"
                        res += "^{\\hphantom{" + b + "}}_{\\hphantom{" + p + "}}"
                        res += "{\\vphantom{X}}"
                        res += "^{\\smash[t]{\\vphantom{2}}\\llap{" + b + "}}"
                        res += "_{\\vphantom{2}\\llap{\\smash[t]{" + p + "}}}"
                    }

                    if (o != "") {
                        if (Regex("^[+-]").matches(o)) {
                            o = "{" + o + "}"
                        }
                        res += o
                    }

                    // 根据 dType 处理上下标
                    if (buf.getString("dType") != "") {
                        if (buf.getString("dType") == "kv") {
                            if (d != "" || q != "") {
                                res += "{\\vphantom{X}}"
                            }
                            if (d != "") {
                                res += "^{" + d + "}"
                            }
                            if (q != "") {
                                res += "_{\\smash[t]{" + q + "}}"
                            }
                        } else if (buf.getString("dType") == "oxidation") {
                            if (d != "") {
                                res += "{\\vphantom{X}}"
                                res += "^{" + d + "}"
                            }
                            if (q != "") {
                                res += "{\\vphantom{X}}"
                                res += "_{\\smash[t]{" + q + "}}"
                            }
                        }
                    } else {
                        if (q != "") {
                            res += "{\\vphantom{X}}"
                            res += "_{\\smash[t]{" + q + "}}"
                        }
                        if (d != "") {
                            res += "{\\vphantom{X}}"
                            res += "^{" + d + "}"
                        }
                    }

                case "rm" =>
                    // 普通文本
                    res = "\\mathrm{" + p1 + "}"

                case "text" =>
                    // 文本处理，包含 ^ 或 _ 的特殊处理
                    if (p1 != "") {
                        if (Regex("[\\^_]").matches(p1)) {
                            res = "\\mathrm{" + p1
                                .replace(" ", "~")
                                .replace("-", "\\text{-}") + "}"
                        } else {
                            res = "\\text{" + p1 + "}"
                        }
                    }

                case "roman numeral" =>
                    // 罗马数字
                    if (p1 != "") {
                        res = "\\mathrm{" + p1 + "}"
                    }

                case "state of aggregation" =>
                // 对标res = "\\mskip2mu " + _goInner(p1Arr)
                res = "\\hspace{2mu}" + _goInner(p1Arr)
                case "state of aggregation subscript" =>
                // 对标res = "\\mskip1mu " + _goInner(p1Arr)
                res = "\\hspace{1mu}" + _goInner(p1Arr)
                case "bond" =>
                res = getBond(kind_)
                if (res == "") {
                    println("[Texify-_go2] mhchem Error. Unknown bond type (${kind_})")
		    return res
                }
                case "frac" =>
                    var c = "\\frac{" + p1 + "}{" + p2 + "}"
                      // 对标res = "\\mathchoice{\\textstyle" + c + "}{" + c + "}{" + c + "}{" + c + "}"
                      res = "{\\textstyle" + c + "}"
                case "pu-frac" =>
                    var d = "\\frac{" + _goInner(p1Arr) + "}{" + _goInner(p2Arr) + "}"
                      // 对标res = "\\mathchoice{\\textstyle" + d + "}{" + d + "}{" + d + "}{" + d + "}"
                      res = "{\\textstyle" + d + "}"
                case "tex-math" => res = replaceCeOnly(p1)
                case "frac-ce" => res = "\\frac{" + _goInner(p1Arr) + "}{" + _goInner(p2Arr) + "}"
                case "overset" => res = "\\overset{" + _goInner(p1Arr) + "}{" + _goInner(p2Arr) + "}"
                case "underset" => res = "\\underset{" + _goInner(p1Arr) + "}{" + _goInner(p2Arr) + "}"
                case "underbrace" => res = "\\underbrace{" + _goInner(p1Arr) + "}{" + _goInner(p2Arr) + "}"
                case "color" => res = "{\\color{" + buf.getString("color1") + "}{" + _goInner(
                    buf.getList("color2").getItems()) + "}}"
                case "color0" => res = "\\color{" + buf.getString("color") + "}"
                case "arrow" =>
                    // 处理箭头和上下标
                    var rd: String = _goInner(buf.getList("rd").getItems())
                    var rq: String = _goInner(buf.getList("rq").getItems())
                    var arrow = getArrow(r)
                    if (rd != "" || rq != "") {
                        if (r == "<=>" || r == "<=>>" || r == "<<=>" || r == "<-->") {
                            arrow = "\\long" + arrow
                            if (rd != "") {
                                arrow = "\\overset{" + rd + "}{" + arrow + "}"
                            }
                            if (rq != "") {
                                if (r == "<-->") {
                                    // 对标arrow = "\\underset{\\lower2mu{" + rq + "}}{" + arrow + "}"
                                    arrow = "\\underset{\\vspace{2mu}{" + rq + "}}{" + arrow + "}"
                                } else {
                                    // 对标arrow = "\\underset{\\lower6mu{" + rq + "}}{" + arrow + "}"
                                    arrow = "\\underset{\\vspace{6mu}{" + rq + "}}{" + arrow + "}"
                                }
                            }
                            arrow = " {}\\mathrel{" + arrow + "}{} "
                        } else {
                            if (rq != "") {
                                arrow += "[{" + rq + "}]"
                            }
                            arrow += "{" + rd + "}"
                            arrow = " {}\\mathrel{\\x" + arrow + "}{} "
                        }
                    } else {
                        arrow = " {}\\mathrel{\\long" + arrow + "}{} "
                    }
                    res = arrow

                case "operator" => res = getOperator(kind_)
                case "1st-level escape" => res = p1 + " "
                case "space" => res = " "
                case "entitySkip" => res = "~"
                case "pu-space-1" => res = "~"
                // 对标res = "\\mkern3mu "
                case "pu-space-2" => res = "\\hspace{3mu} "
                // 对标res = "\\mkern2mu "
                case "1000 separator" => res = "\\hspace{2mu} "
                case "commaDecimal" => res = "{,}"
                // 对标res = "{" + p1 + "}\\mkern6mu "
                case "comma enumeration L" => res = "{" + p1 + "}\\hspace{6mu} "
                // 对标res = "{" + p1 + "}\\mkern3mu "
                case "comma enumeration M" => res = "{" + p1 + "}\\hspace{3mu} "
                // 对标res = "{" + p1 + "}\\mkern1mu "
                case "comma enumeration S" => res = "{" + p1 + "}\\hspace{1mu} "
                case "hyphen" => res = "\\text{-}"
                case "addition compound" => res = "\\,{\\cdot}\\,"
                // 对标res = "\\mkern1mu \\bullet\\mkern1mu "
                case "electron dot" => res = "\\hspace{1mu} \\bullet\\hspace{1mu} "
                case "KV x" => res = "{\\times}"
                case "prime" => res = "\\prime "
                case "cdot" => res = "\\cdot "
                // 对标"\\mkern1mu{\\cdot}\\mkern1mu "
                case "tight cdot" => res = "\\hspace{1mu}{\\cdot}\\hspace{1mu} "
                case "times" => res = "\\times "
                case "circa" => res = "{\\sim}"
                case "^" => res = "uparrow"
                case "v" => res = "downarrow"
                case "ellipsis" => res = "\\ldots "
                case "/" => res = "/"
                case " / " => res = "\\,/\\,"
                case _ => println("[Texify-_go2] mhchem bug T. Please report.") // Missing texify rule or unknown MhchemParser output
            }
        }

        return res
    }

    /**
     * 仅替换文本中的 \ce{...} 化学公式。
     *
     * 扫描输入字符串，找到所有 \ce{...} 结构，
     * 对其中的内容进行 mhchem 解析并转换为 LaTeX，
     * 其余普通文本保持不变。
     *
     * @param input 原始输入字符串
     * @return 替换后的字符串
     */
    private func replaceCeOnly(input: String): String {
        var res = ""
        var i: Int64 = 0

        while (i < input.size) {
            // 找到从当前位置开始的下一个 \ce{
            match (input.indexOf("\\ce{", i)) {
                case Some(idx) =>
                    // 普通文本追加到结果里
                    if (idx > i) {
                        res += input[i..idx]
                    }

                    // 从 idx 开始解析 \ce{ ... }
                    var start: Int64 = idx + 4
                    var level: Int64 = 1
                    var j: Int64 = start

                    // 找匹配的右大括号
                    while (j < input.size && level > 0) {
                        match (input.get(j)) {
                            case Some(b) =>
                                if (b == 123) { // '{'
                                    level += 1
                                } else if (b == 125) { // '}'
                                    level -= 1
                                }
                            case None => break
                        }
                        j += 1
                    }

                    // 括号不匹配，原样输出
                    if (level != 0) {
                        res += "\\ce{"
                        i = start
                        continue
                    }

                    // 截取 \ce{} 内部内容
                    var ceContent: String = input[start..(j - 1)]

                    // 调用 mhchem 解析
                    var mhchemParser = MhchemParser()
                    let inner = mhchemParser.go(ceContent, stateMachine: "ce")
                    var texify = Texify()
                    var latex = texify.go(inner, isInner: true)

                    // 替换到结果
                    res += latex

                    // 移动指针到 '}' 后面
                    i = j

                case None =>
                    // 没找到 \ce{，剩余部分直接追加
                    res += input[i..input.size]
                    break
            }
        }

        return res
    }

    /**
     * 将箭头字符串转换为 LaTeX 对应箭头命令
     *
     * @param source 输入箭头符号
     * @return LaTeX 对应箭头
     */
    private func getArrow(source: String): String {
        match (source) {
            case "->" => return "rightarrow"
            case "\u{2192}" => return "rightarrow"
            case "\u{27F6}" => return "rightarrow"
            case "<-" => return "leftarrow"
            case "<->" => return "leftrightarrow"
            case "<-->" => return "leftrightarrows"
            case "<=>" => return "rightleftharpoons"
            case "\u{21CC}" => return "rightleftharpoons"
            case "<=>>" => return "Rightleftharpoons"
            case "<<=>" => return "Leftrightharpoons"
            case _ =>
            println("[Texify-getArrow] mhchem bug T. Please report.")
            return ""
        }
    }

    /**
     * 将化学键类型转换为 LaTeX 表达式
     *
     * @param source 键类型字符串
     * @return LaTeX 化学键
     */
    private func getBond(source: String): String {
        match (source) {
            case "-" => return "{-}"
            case "1" => return "{-}"
            case "=" => return "{=}"
            case "2" => return "{=}"
            case "#" => return "{\\equiv}"
            case "3" => return "{\\equiv}"
            case "~" => return "{\\tripledash}"
            case "~-" => return "{\\rlap{\\lower.1em{-}}\\raise.1em{\\tripledash}}"
            case "~=" => return "{\\rlap{\\lower.2em{-}}\\rlap{\\raise.2em{\\tripledash}}-}"
            case "~--" => return "{\\rlap{\\lower.2em{-}}\\rlap{\\raise.2em{\\tripledash}}-}"
            case "-~-" => return "{\\rlap{\\lower.2em{-}}\\rlap{\\raise.2em{-}}\\tripledash}"
            case "..." => return "{{\\cdot}{\\cdot}{\\cdot}}"
            case "...." => return "{{\\cdot}{\\cdot}{\\cdot}{\\cdot}}"
            case "->" => return "{\\rightarrow}"
            case "<-" => return "{\\leftarrow}"
            case "<" => return "{<}"
            case ">" => return "{>}"
            case _ =>
            println("[Texify-getBond] mhchem bug T. Please report.")
            return ""
        }
    }

    /**
     * 将运算符转换为 LaTeX 表达式
     *
     * @param source 运算符字符串
     * @return LaTeX 运算符
     */
    private func getOperator(source: String): String {
        match (source) {
            case "+" => return " {}+{} "
            case "-" => return " {}-{} "
            case "=" => return " {}={} "
            case "<" => return " {}<{} "
            case ">" => return " {}>{} "
            case "<<" => return " {}\\ll{} "
            case ">>" => return " {}\\gg{} "
            case "\\pm" => return " {}\\pm{} "
            case "\\approx" => return " {}\\approx{} "
            case "$\\approx$" => return " {}\\approx{} "
            case "v" => return " \\downarrow{} "
            case "(v)" => return " \\downarrow{} "
            case "^" => return " \\uparrow{} "
            case "(^)" => return " \\uparrow{} "
            case _ =>
            println("[Texify-getOperator] mhchem bug T. Please report.")
            return ""
        }
    }
}
