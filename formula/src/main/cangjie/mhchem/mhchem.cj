package formula.mhchem

/**
 * 化学公式处理器。
 *
 * 用于处理文本中的化学公式标记（如 \ce{...} \pu{...}），
 * 将其转换为标准 LaTeX 可识别、可渲染的表达形式。
 *
 * 主要职责：
 * - 识别并解析化学公式语法
 * - 按 mhchem 规则进行语义转换
 * - 输出可直接用于渲染的 LaTeX 字符串
 */
protected class Mhchem {

    /**
     * 化学公式预处理入口。
     *
     * 接收包含 \ce{...} \pu{...} 的原始字符串，
     * 并完成从化学语法到 LaTeX 的完整转换流程：
     *
     * 1. 加载化学公式解析规则（mhchem JSON）
     * 2. 解析并生成语法树（AST）
     * 3. 将 AST 转换为标准 LaTeX 表达式
     *
     * @param input 原始输入字符串（可能包含化学公式）
     * @return 转换后的 LaTeX 字符串
     */
    protected func process(input: String): String {
        MhchemJsonParser.loadJson()
        return processTopLevel(input)
    }

    func processTopLevel(unprocessed: String): String {
        let chars: Array<Rune> = unprocessed.toRuneArray()
        let sb = StringBuilder()
        var i: Int64 = 0
        var ceDepth: Int64 = 0
        var puDepth: Int64 = 0

        while (i < chars.size) {
            // 处理 \ce{}
            if (i + 3 < chars.size && chars[i] == r'\\' && chars[i + 1] == r'c' && chars[i + 2] == r'e' &&
                chars[i + 3] == r'{') {
                if (ceDepth == 0) {
                    var level: Int64 = 1
                    var j: Int64 = i + 4
                    while (j < chars.size && level > 0) {
                        if (chars[j] == r'{') {
                            level += 1
                        } else if (chars[j] == r'}') {
                            level -= 1
                        }
                        j += 1
                    }
                    if (level == 0) {
                        let innerRunes = chars[(i + 4)..(j - 1)]
                        let innerStr = String(innerRunes)
                        var parsed = Texify().go(MhchemParser().go(innerStr))
                        parsed = processNestedPu(parsed)
                        sb.append(escape(parsed))
                        i = j
                        continue
                    }
                } else {
                    ceDepth += 1
                }
            }

            // 处理 \pu{}
            if (i + 3 < chars.size && chars[i] == r'\\' && chars[i + 1] == r'p' && chars[i + 2] == r'u' &&
                chars[i + 3] == r'{') {
                if (puDepth == 0) {
                    var level: Int64 = 1
                    var j: Int64 = i + 4

                    while (j < chars.size && level > 0) {
                        if (chars[j] == r'{') {
                            level += 1
                        } else if (chars[j] == r'}') {
                            level -= 1
                        }
                        j += 1
                    }

                    if (level == 0) {
                        let innerRunes = chars[(i + 4)..(j - 1)]
                        let innerStr = String(innerRunes)
                        var parsed = Texify().go(MhchemParser().go(innerStr, stateMachine: "pu"))
                        parsed = processNestedPu(parsed)
                        sb.append(escape(parsed))
                        i = j
                        continue
                    }
                } else {
                    puDepth += 1
                }
            }
            if (ceDepth > 0 && chars[i] == r'}') {
                ceDepth -= 1
            }
            if (puDepth > 0 && chars[i] == r'}') {
                puDepth -= 1
            }
            sb.append(chars[i])
            i += 1
        }
        return sb.toString()
    }

    func processNestedPu(input: String): String {
        if (input.contains("\\pu{")) {
            return processTopLevel(input)
        } else {
            return input
        }
    }

    /**
     * 对特定字符进行转义处理
     */
    func escape(input: String): String {
        let escapeSet: Array<Rune> = [r'$']
        let chars = input.toRuneArray()
        let sb = StringBuilder()
        var i: Int64 = 0
        while (i < chars.size) {
            let current = chars[i]
            if (escapeSet.contains(current)) {
                var backslashCount: Int64 = 0
                var j: Int64 = i - 1
                while (j >= 0 && chars[j] == r'\\') {
                    backslashCount += 1
                    j -= 1
                }
                if (backslashCount % 2 == 0) {
                    sb.append(r'\\')
                }
            }
            sb.append(current)
            i += 1
        }
        return sb.toString()
    }
}
