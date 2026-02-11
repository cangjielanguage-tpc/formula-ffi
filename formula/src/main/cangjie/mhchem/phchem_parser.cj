package formula.mhchem

/**
 * 用于解析化学式和相关化学表达式,并可生成对应的 LaTeX 表达式。
 */
class MhchemParser {
    /**
     * mhchem 解析主循环。
     *
     * 按状态机驱动解析输入字符串，通过 transitionsMap 进行匹配，
     * 调用 machineActions / mhchemParserActions 生成 ParserValue 输出。
     *
     * @param source 待解析的字符串
     * @param stateMachine 初始状态机（默认 "ce"）
     * @return 解析后的 ParserValue 列表
     */
    func go(source: String, stateMachine!: String = "ce"): ArrayList<ParserValue> {
        var input = source

        if (input == "") {
            return ArrayList<ParserValue>()
        }

        // 创建空对象，用来存储解析过程中的临时信息
        var buffer: Buffer = Buffer()
        buffer.parenthesisLevel = 0
        var state: String = "0"

        // 清理输入字符串
        input = Regex("\n").replaceAll(input, " ")
        input = Regex("\u{2212}|\u{2013}|\u{2014}|\u{2010}").replaceAll(input, "-")
        input = Regex("\u{2026}").replaceAll(input, "...")

        var lastInput: String = ""
        var watchdog: Int32 = 10
        let output: ArrayList<ParserValue> = ArrayList<ParserValue>()

        while (true) {
            if (lastInput != input) {
                watchdog = 10
                lastInput = input
            } else {
                watchdog--
            }

            var machine = MhchemJsonParser.transitionsMap.get(stateMachine)
            var t: ArrayList<Transitions> = ArrayList<Transitions>()

            if (let Some(v) <- machine && v.contains(state) && let Some(vState) <- v.get(state)) {
                t = vState
            } else if (let Some(v) <- machine && let Some(vState) <- v.get('*')) {
                t = vState
            }

            for (i in 0..t.size) {
                var matchs: Option<Match> = match_(t[i].pattern, input)
                if (let Some(matches) <- matchs) {
                    var task = t[i].task
                    for (iA in 0..task.action_.size) {
                        var o: Option<ParserValue> = Option<ParserValue>.None
                        if (machineActionsHasType(stateMachine, task.action_[iA].type_)) {
                            o = machineActions(stateMachine, task.action_[iA].type_, buffer: buffer,
                                mList: matches.match_, option: task.action_[iA])
                        } else if (mhchemParserActionsHasType(task.action_[iA].type_)) {
                            o = mhchemParserActions(task.action_[iA].type_, buffer, mList: matches.match_,
                                option: task.action_[iA])
                        } else {
                            // Trying to use non-existing action
                            println("[MhchemParser-go] mhchem bug A. Please report. ${task.action_[iA].type_}")
                        }
                        concatArray(output, o)
                    }

                    if (task.nextState != "") {
                        state = task.nextState
                    } else {
                        state = state
                    }

                    if (input.size > 0) {
                        if (!task.revisit) {
                            input = matches.remainder
                        }
                        if (!task.toContinue) {
                            break
                        }
                    } else {
                        return output
                    }
                }
            }

            if (watchdog <= 0) {
                println("[MhchemParser-go] mhchem bug U. Please report.")
            }
        }

        return ArrayList<ParserValue>()
    }

    /**
     * 根据当前状态机和动作类型执行对应的解析逻辑。
     *
     * 这是 mhchem 解析的核心调度函数：负责维护 Buffer 状态，
     * 在不同 stateMachine 下生成对应的 ParserValue 输出。
     *
     * @param stateMachine 当前状态机名称
     * @param type_ 动作类型
     * @param buffer 解析缓冲区
     * @param option 附加动作参数
     * @param mList 匹配到的字符串列表
     * @return 生成的 ParserValue，无输出则返回 None
     */
    private func machineActions(stateMachine: String, type_: String, buffer!: Buffer = Buffer(),
        option!: Option<Action> = Option<Action>.None, mList!: ArrayList<String> = ArrayList<String>()): Option<ParserValue> {
        var m: String = ""
        var m0: String = ""
        var m1: String = ""
        var m2: String = ""

        if (mList.size > 0 && let Some(v) <- mList.get(0)) {
            m = v
            m0 = v
        }
        if (mList.size > 1 && let Some(v) <- mList.get(1)) {
            m1 = v
        }
        if (mList.size > 2 && let Some(v) <- mList.get(2)) {
            m2 = v
        }
        var ret: Option<ParserValue>

        if (stateMachine == "ce") {
            match (type_) {
                case 'o after d' =>
                    var ret: Option<ParserValue>
                    if (Regex("^[0-9]+$").matches(buffer.d.getOrDefault({=> ""}))) {
                        var tmp = buffer.d
                        buffer.d = Option<String>.None
                        ret = machineActions("ce", "output", buffer: buffer)
                        buffer.b = tmp
                    } else {
                        ret = machineActions("ce", "output", buffer: buffer)
                    }
                    mhchemParserActions("o=", buffer, mList: ArrayList<String>([m]))
                    return ret

                case 'd= kv' =>
                    buffer.d = m
                    buffer.dType = 'kv'

                case 'charge or bond' =>
                    if (buffer.beginsWithBond) {
                        var ret: PVList = PVList()
                        concatArray(ret.getItems(), machineActions("ce", "output", buffer: buffer))
                        concatArray(ret.getItems(),
                            mhchemParserActions("bond", buffer, mList: ArrayList<String>([m]), k: "-"))
                        return ret
                    } else {
                        buffer.d = m
                    }

                case '- after o/d' =>
                    var c1 = match_('orbital', buffer.o.getOrDefault({=> ""}))
                    var c2 = match_('one lowercase greek letter $', buffer.o.getOrDefault({=> ""}))
                    var c3 = match_('one lowercase latin letter $', buffer.o.getOrDefault({=> ""}))
                    var c4 = match_('$one lowercase latin letter$ $', buffer.o.getOrDefault({=> ""}))

                    var c1remainder = ""
                    if (let Some(v) <- c1) {
                        c1remainder = v.remainder
                    }

                    var hyphenFollows = m == "-" && ((c1.isSome() && c1remainder == "") || c2.isSome() || c3.isSome() ||
                        c4.isSome())
                    if (hyphenFollows && buffer.a.isNone() && buffer.b.isNone() && buffer.p.isNone() && buffer.d.isNone() &&
                        buffer.q.isNone() && !c1.isSome() && c3.isSome()) {
                        buffer.o = '$' + buffer.o.getOrDefault({=> ""}) + '$'
                    }

                    var ret: PVList = PVList()
                    if (hyphenFollows) {
                        concatArray(ret.getItems(), machineActions("ce", "output", buffer: buffer))
                        ret.add(PVObject("hyphen"))
                    } else {
                        var isAfterD: Bool = false

                        if (let Some(v) <- option) {
                            if (let Some(optionBool) <- v.optionBool) {
                                isAfterD = optionBool
                            }
                        }
                        c1 = match_('digits', buffer.d.getOrDefault({=> ""}))
                        if (isAfterD && c1.isSome()) {
                            concatArray(ret.getItems(), mhchemParserActions("d=", buffer, mList: ArrayList<String>([m])))
                            concatArray(ret.getItems(), machineActions("ce", "output", buffer: buffer))
                        } else {
                            concatArray(ret.getItems(), machineActions("ce", "output", buffer: buffer))
                            concatArray(ret.getItems(),
                                mhchemParserActions("bond", buffer, mList: ArrayList<String>([m]), k: "-"))
                        }
                    }
                    return ret

                case 'a to o' =>
                    buffer.o = buffer.a
                    buffer.a = Option<String>.None

                case 'sb=true' => buffer.sb = true

                case 'sb=false' => buffer.sb = false

                case 'beginsWithBond=true' => buffer.beginsWithBond = true

                case 'beginsWithBond=false' => buffer.beginsWithBond = false

                case 'parenthesisLevel++' => buffer.parenthesisLevel++

                case 'parenthesisLevel--' => buffer.parenthesisLevel--

                case 'state of aggregation' =>
                    var ret = PVObject("state of aggregation")
                    var p1List: PVList = PVList()
                    for (item in go(m, stateMachine: 'o')) {
                        p1List.add(item)
                    }
                    ret.put("p1", p1List)
                    return ret

                case 'comma' =>
                    let r = Regex("\\s*$")
                    var a = r.replace(m, '')
                    var withSpace = (a != m)
                    if (withSpace && buffer.parenthesisLevel == 0) {
                        var ret = PVObject("comma enumeration L")
                        ret.put("p1", PVString(a))
                        return ret
                    } else {
                        var ret = PVObject("comma enumeration M")
                        ret.put("p1", PVString(a))
                        return ret
                    }

                case 'output' =>
                    var entityFollows: Option<Int64> = Option<Int64>.None

                    if (let Some(v) <- option) {
                        if (let Some(optionNum) <- v.optionNum) {
                            entityFollows = optionNum
                        }
                    }
                    var ret = PVList()
                    if (buffer.r == "") {
                        if (buffer.a.isNone() && buffer.b.isNone() && buffer.p.isNone() && buffer.o.isNone() &&
                            buffer.q.isNone() && buffer.d.isNone() && entityFollows.isNone()) {
                            ret = PVList()
                        } else {
                            if (buffer.sb) {
                                var entitySkipObj: PVObject = PVObject("entitySkip")
                                ret.add(entitySkipObj)
                            }
                            if (buffer.o.isNone() && buffer.q.isNone() && buffer.d.isNone() && buffer.b.isNone() && buffer.p.isNone() &&
                                entityFollows != 2) {
                                buffer.o = buffer.a
                                buffer.a = Option<String>.None
                            } else if (buffer.o.isNone() && buffer.q.isNone() && buffer.d.isNone() && (buffer.b.isSome() ||
                                buffer.p.isSome())) {
                                buffer.o = buffer.a
                                buffer.d = buffer.b
                                buffer.q = buffer.p
                                buffer.a = Option<String>.None
                                buffer.b = Option<String>.None
                                buffer.p = Option<String>.None
                            } else {
                                if (buffer.o.isSome() && buffer.dType == 'kv' &&
                                    match_('d-oxidation$', buffer.d.getOrDefault({=> ""})).isSome()) {
                                    buffer.dType = 'oxidation'
                                } else if (buffer.o.isSome() && buffer.dType == 'kv' && buffer.q.isNone()) {
                                    buffer.dType = Option<String>.None
                                }
                            }

                            var d = ""
                            if (buffer.dType == 'oxidation') {
                                d = 'oxidation'
                            } else {
                                d = 'bd'
                            }

                            var chemfiveObj: PVObject = PVObject("chemfive")
                            chemfiveObj.put("a", PVList(go(buffer.a.getOrDefault({=> ""}), stateMachine: "a")))
                            chemfiveObj.put("b", PVList(go(buffer.b.getOrDefault({=> ""}), stateMachine: "bd")))
                            chemfiveObj.put("p", PVList(go(buffer.p.getOrDefault({=> ""}), stateMachine: "pq")))
                            chemfiveObj.put("o", PVList(go(buffer.o.getOrDefault({=> ""}), stateMachine: "o")))
                            chemfiveObj.put("q", PVList(go(buffer.q.getOrDefault({=> ""}), stateMachine: "pq")))
                            chemfiveObj.put("d", PVList(go(buffer.d.getOrDefault({=> ""}), stateMachine: d)))
                            chemfiveObj.put("dType", PVString(buffer.dType.getOrDefault({=> ""})))
                            ret.add(chemfiveObj)
                        }
                    } else {
                        var rd: PVList
                        if (buffer.rdt == 'M') {
                            rd = PVList(go(buffer.rd, stateMachine: 'tex-math'))
                        } else if (buffer.rdt == 'T') {
                            rd = PVList()
                            rd.add(PVObject("text").put("p1", PVString(buffer.rd)))
                        } else {
                            rd = PVList(go(buffer.rd))
                        }

                        var rq: PVList
                        if (buffer.rqt == 'M') {
                            rq = PVList(go(buffer.rq, stateMachine: 'tex-math'))
                        } else if (buffer.rqt == 'T') {
                            rq = PVList()
                            rq.add(PVObject("text").put("p1", PVString(buffer.rq)))
                        } else {
                            rq = PVList(go(buffer.rq))
                        }

                        var arrow = PVObject("arrow")
                        arrow.put("r", PVString(buffer.r))
                        arrow.put("rd", rd)
                        arrow.put("rq", rq)
                        ret.add(arrow)
                    }
                    buffer.clear()
                    return ret

                case 'oxidation-output' =>
                    var ret: ArrayList<ParserValue> = ArrayList<ParserValue>()
                    ret.add(PVString("{"))
                    var oxidationList = go(m, stateMachine: 'oxidation')
                    for (item in oxidationList) {
                        ret.add(item)
                    }
                    ret.add(PVString("}"))
                    return PVList(ret)

                case 'frac-output' => return PVObject("frac-ce")
                    .put("p1", PVList(go(m0)))
                    .put("p2", PVList(go(m1)))

                case 'overset-output' => return PVObject("overset")
                    .put("p1", PVList(go(m0)))
                    .put("p2", PVList(go(m1)))

                case 'underset-output' => return PVObject("underset")
                    .put("p1", PVList(go(m0)))
                    .put("p2", PVList(go(m1)))

                case 'underbrace-output' => return PVObject("underbrace")
                    .put("p1", PVList(go(m0)))
                    .put("p2", PVList(go(m1)))

                case 'color-output' => return PVObject("color")
                    .put("color1", PVString(m0))
                    .put("color2", PVList(go(m1)))

                case 'r=' => buffer.r = m
                case 'rdt=' => buffer.rdt = m
                case 'rd=' => buffer.rd = m
                case 'rqt=' => buffer.rqt = m
                case 'rq=' => buffer.rq = m
                case 'operator' =>
                    var p1: Option<String> = Option<String>.None

                    if (let Some(v) <- option) {
                        if (let Some(option) <- v.option) {
                            p1 = option
                        }
                    }
                    if (p1.isSome()) {
                        return PVObject("operator").put("kind_", PVString(p1.getOrDefault({=> ""})))
                    } else {
                        return PVObject("operator").put("kind_", PVString(m))
                    }
                case _ => ()
            }
        } else if (stateMachine == "text") {
            match (type_) {
                case 'output' =>
                    if (buffer.text_ != "") {
                        let ret = PVObject("text").put("p1", PVString(buffer.text_))
                        buffer.clearAll()
                        return ret
                    }
                case _ => ()
            }
        } else if (stateMachine == "pq") {
            match (type_) {
                case 'state of aggregation' => return PVObject("state of aggregation subscript").put("p1",
                    PVList(go(m, stateMachine: 'o')))
                case 'color-output' =>
                return PVObject("color").put("color1", PVString(m0)).put("color2", PVList(go(m1, stateMachine: 'pq')))
                case _ => ()
            }
        } else if (stateMachine == "bd") {
            match (type_) {
                case 'color-output' =>
                return PVObject("color").put("color1", PVString(m0)).put("color2", PVList(go(m1, stateMachine: 'bd')))
                case _ => ()
            }
        } else if (stateMachine == "oxidation") {
            match (type_) {
                case 'roman-numeral' => return PVObject("roman numeral").put("p1", PVString(m))
                case _ => ()
            }
        } else if (stateMachine == "tex-math") {
            match (type_) {
                case 'output' =>
                    if (buffer.o.isSome()) {
                        let ret = PVObject("tex-math").put("p1", PVString(buffer.o.getOrDefault({=> ""})))
                        buffer.clearAll()
                        return ret
                    }
                case _ => ()
            }
        } else if (stateMachine == "tex-math tight") {
            match (type_) {
                case 'tight operator' => buffer.o = buffer.o.getOrDefault({=> ""}) + "{" + m + "}"
                case 'output' =>
                    if (buffer.o.isSome()) {
                        let ret = PVObject("tex-math").put("p1", PVString(buffer.o.getOrDefault({=> ""})))
                        buffer.clearAll()
                        return ret
                    }
                case _ => ()
            }
        } else if (stateMachine == "9,9") {
            match (type_) {
                case 'comma' => return PVObject("commaDecimal")
                case _ => ()
            }
        } else if (stateMachine == "pu") {
            match (type_) {
                case 'enumber' =>
                    var m3: String = ""
                    var m4: String = ""
                    var m5: String = ""
                    if (mList.size > 3 && let Some(v) <- mList.get(3)) {
                        m3 = v
                    }
                    if (mList.size > 4 && let Some(v) <- mList.get(4)) {
                        m4 = v
                    }
                    if (mList.size > 5 && let Some(v) <- mList.get(5)) {
                        m5 = v
                    }

                    var ret = PVList()

                    if (m0 == "+-" || m0 == "+/-") {
                        ret.add(PVString("\\pm "))
                    } else if (m0 != "") {
                        ret.add(PVString(m0))
                    }
                    if (m1 != "") {
                        concatArray(ret.getItems(), PVList(go(m1, stateMachine: 'pu-9,9')))
                        if (m2 != "") {
                            if (Regex("[,.]").matches(m2)) {
                                concatArray(ret.getItems(), PVList(go(m2, stateMachine: 'pu-9,9')))
                            } else {
                                ret.add(PVString(m2))
                            }
                        }
                        if (m3 != "" || m4 != "") {
                            if (m3 == "e" || m4 == "*") {
                                ret.add(PVObject("cdot"))
                            } else {
                                ret.add(PVObject("times"))
                            }
                        }
                    }
                    if (m5 != "") {
                        ret.add(PVString("10^{" + m5 + "}"))
                    }
                    return ret
                case 'number^' =>
                    var ret = PVList()
                    if (m0 == "+-" || m0 == "+/-") {
                        ret.add(PVString("\\pm "))
                    } else if (m0 != "") {
                        ret.add(PVString(m0))
                    }
                    concatArray(ret.getItems(), PVList(go(m1, stateMachine: 'pu-9,9')))
                    ret.add(PVString("^{" + m2 + "}"))

                    return ret
                case 'operator' =>
                    var p1: Option<String> = Option<String>.None

                    if (let Some(v) <- option) {
                        if (let Some(option) <- v.option) {
                            p1 = option
                        }
                    }

                    if (p1.isSome()) {
                        return PVObject("operator").put("kind_", PVString(p1.getOrDefault({=> ""})))
                    } else {
                        return PVObject("operator").put("kind_", PVString(m))
                    }

                case 'space' => return PVObject("pu-space-1")

                case 'output' =>
                    var ret = PVList()
                    var md = match_('{(...)}', buffer.d.getOrDefault({=> ""}))
                    var mdremainder = ""
                    if (let Some(v) <- md) {
                        mdremainder = v.remainder
                    }
                    if (md.isSome() && mdremainder == "") {
                        if (let Some(v) <- md) {
                            if (v.match_.size > 0 && let Some(vMatch) <- v.match_.get(0)) {
                                buffer.d = vMatch
                            }
                        }
                    }
                    var mq = match_('{(...)}', buffer.q.getOrDefault({=> ""}))
                    var mqremainder = ""
                    if (let Some(v) <- mq) {
                        mqremainder = v.remainder
                    }
                    if (mq.isSome() && mqremainder == "") {
                        if (let Some(v) <- mq) {
                            if (v.match_.size > 0 && let Some(vMatch) <- v.match_.get(0)) {
                                buffer.q = vMatch
                            }
                        }
                    }
                    if (buffer.d.isSome()) {
                        buffer.d = Regex("\u{00B0}C|\\^oC|\\^{o}C").replaceAll(buffer.d.getOrDefault({=> ""}), "{}^{\\circ}C")
                        buffer.d = Regex("\u{00B0}F|\\^oF|\\^{o}F").replaceAll(buffer.d.getOrDefault({=> ""}), "{}^{\\circ}F")
                    }
                    if (buffer.q.isSome()) {
                        buffer.q = Regex("\u{00B0}C|\\^oC|\\^{o}C").replaceAll(buffer.q.getOrDefault({=> ""}), "{}^{\\circ}C")
                        buffer.q = Regex("\u{00B0}F|\\^oF|\\^{o}F").replaceAll(buffer.q.getOrDefault({=> ""}), "{}^{\\circ}F")
                        var d = go(buffer.d.getOrDefault({=> ""}), stateMachine: 'pu')
                        var q = go(buffer.q.getOrDefault({=> ""}), stateMachine: 'pu')

                        if (buffer.o == '//') {
                            ret = ret.add(PVObject("pu-frac").put("p1", PVList(d)).put("p2", PVList(q)))
                        } else {
                            ret = PVList(d)
                            if (d.size > 1 || q.size > 1) {
                                ret = ret.add(PVObject(' / '))
                            } else {
                                ret = ret.add(PVObject('/'))
                            }
                            concatArray(ret.getItems(), PVList(q))
                        }
                    } else {
                        ret = PVList(go(buffer.d.getOrDefault({=> ""}), stateMachine: 'pu-2'))
                    }
                    buffer.clearAll()
                    return ret
                case _ => ()
            }
        } else if (stateMachine == "pu-2") {
            match (type_) {
                case 'cdot' => return PVObject("tight cdot")

                case '^(-1)' => buffer.rm += "^{" + m + "}"
                case 'space' => return PVObject("pu-space-2")
                case 'output' =>
                    var ret = PVList()
                    if (buffer.rm != "") {
                        var mrm = match_('{(...)}', buffer.rm)
                        var mrmremainder = ""
                        if (let Some(v) <- mrm) {
                            mrmremainder = v.remainder
                        }
                        if (mrm.isSome() && mrmremainder == "") {
                            if (let Some(mrmMatch) <- mrm) {
                                goArray(ret, mrmMatch.match_, 'pu')
                            }
                        } else {
                            ret.add(PVObject("rm").put("p1", PVString(buffer.rm)))
                        }
                    }
                    buffer.clearAll()
                    return ret

                case _ => ()
            }
        } else if (stateMachine == "pu-9,9") {
            match (type_) {
                case 'comma' => return PVObject("commaDecimal")

                case 'output-0' =>
                    var ret = PVList()

                    if (buffer.text_.size > 4) {
                        var a = buffer.text_.size % 3
                        if (a == 0) {
                            a = 3
                        }
                        for (i in (buffer.text_.size - 3)..=0 : -3) {
                            ret.add(PVString(buffer.text_[i..i + 2]))
                            ret.add(PVObject('1000 separator'))
                        }
                        ret.add(PVString(buffer.text_[0..a]))
                        ret.getItems().reverse()
                    } else {
                        ret.add(PVString(buffer.text_))
                    }
                    buffer.clearAll()
                    return ret

                case 'output-o' =>
                    var ret = PVList()
                    if (buffer.text_.size > 4) {
                        var a = buffer.text_.size - 3
                        var index = 0
                        for (i in 0..a : 3) {
                            ret.add(PVString(buffer.text_[i..3]))
                            ret.add(PVObject('1000 separator'))
                            index = i
                        }
                        ret.add(PVString(buffer.text_[index..buffer.text_.size]))
                    } else {
                        ret.add(PVString(buffer.text_))
                    }
                    buffer.clearAll()
                    return ret
                case _ => ()
            }
        } else {
        }
        return Option<ParserValue>.None
    }

    /**
     * 执行 mhchem 解析过程中的具体动作。
     *
     * 根据 type_ 对 buffer 进行状态修改，或直接生成对应的 ParserValue。
     *
     * @param type_ 动作类型标识
     * @param buffer 当前解析缓冲区
     * @param mList 匹配到的字符串列表
     * @param option 附加动作参数
     * @param a 额外参数（用于 insert / bond 等）
     * @param k 键值参数（用于 bond 类型）
     * @return 生成的 ParserValue，若无输出则返回 None
     */
    private func mhchemParserActions(type_: String, buffer: Buffer, mList!: ArrayList<String> = ArrayList<String>(),
        option!: Option<Action> = Option<Action>.None, k!: String = ""): Option<ParserValue> {
        var m: String = ""
        var m0: String = ""
        var m1: String = ""

        if (mList.size > 0 && let Some(v) <- mList.get(0)) {
            m = v
            m0 = v
        }
        if (mList.size > 1 && let Some(v) <- mList.get(1)) {
            m1 = v
        }

        var a: Option<String> = Option<String>.None

        if (let Some(v) <- option) {
            if (let Some(option) <- v.option) {
                a = option
            }
        }
        match (type_) {
            case 'a=' => buffer.a = buffer.a.getOrDefault({=> ""}) + m
            case 'b=' => buffer.b = buffer.b.getOrDefault({=> ""}) + m
            case 'p=' => buffer.p = buffer.p.getOrDefault({=> ""}) + m
            case 'o=' => buffer.o = buffer.o.getOrDefault({=> ""}) + m

            case 'q=' => buffer.q = buffer.q.getOrDefault({=> ""}) + m
            case 'd=' => buffer.d = buffer.d.getOrDefault({=> ""}) + m
            case 'rm=' => buffer.rm = buffer.rm + m
            case 'text=' => buffer.text_ = buffer.text_ + m

            case 'insert' => return PVObject(a.getOrDefault({=> ""}))
            case 'insert+p1' => return PVObject(a.getOrDefault({=> ""})).put("p1", PVString(m))

            case 'insert+p1+p2' => return PVObject(a.getOrDefault({=> ""})).put("p1", PVString(m0)).put("p2", PVString(m1))

            case 'copy' => return PVString(m)
            case 'rm' => return PVObject('rm').put("p1", PVString(m))

            case 'text' => return PVList(go(m, stateMachine: 'text'))
            case '{text}' =>
                var ret: ArrayList<ParserValue> = ArrayList<ParserValue>()

                ret.add(PVString("{"))

                var list = go(m, stateMachine: 'text')

                for (item in list) {
                    ret.add(item)
                }

                ret.add(PVString("}"))

                return PVList(ret)

            case 'tex-math' => return PVList(go(m, stateMachine: 'tex-math'))
            case 'tex-math tight' => return PVList(go(m, stateMachine: 'tex-math tight'))

            case 'bond' =>
                if (k != "") {
                    return PVObject('bond').put("kind_", PVString(k))
                } else {
                    return PVObject('bond').put("kind_", PVString(m))
                }
            case 'color0-output' => return PVObject('color0').put("color", PVString(m0))

            case 'ce' => return PVList(go(m))

            case '1/2' =>
                var ret = PVList()

                if (Regex("^[+\\-]").matches(m)) {
                    ret.add(PVString(m[0..1]))
                    m = m[1..m.size]
                }

                var r = Regex("^([0-9]+|\$[a-z]\$|[a-z])\\/([0-9]+)(\$[a-z]\$|[a-z])?$")

                match (r.find(m, group: true)) {
                    case Some(n) =>
                        if (n.groupCount() >= 1) {
                            var n1 = Regex('\$').replaceAll(n.matchString(1), "")
                            var n2 = ""
                            if (n.groupCount() >= 2) {
                                n2 = n.matchString(2)
                            }
                            ret.add(PVObject('frac').put("p1", PVString(n1)).put("p2", PVString(n2)))
                            if (n.groupCount() > 3) {
                                var n3 = Regex('\$').replaceAll(n.matchString(3), "")
                                ret.add(PVObject('tex-math').put("p1", PVString(n3)))
                            }
                            return ret
                        }
                    case None => ()
                }
                return ret
            case '9,9' => return PVList(go(m, stateMachine: '9,9'))

            case _ => ()
        }

        return Option<ParserValue>.None
    }

    /**
     * 根据指定模式匹配输入字符串。
     *
     * 支持多种预定义模式（字母、数字、希腊字母、运算符等）或自定义模式方法。
     * 返回匹配的子串列表和剩余未匹配部分。
     *
     * @param pattern 要匹配的模式标识或自定义方法名
     * @param input 待匹配的字符串
     * @return 匹配结果和剩余字符串，如果未匹配则返回 None
     */
    private func match_(pattern: String, input: String): Option<Match> {
        var value = ""
        if (pattern == "") {
            println("[MhchemParser-match_] mhchem bug P. Please report")
            return Option<Match>.None
        }
        match (pattern) {
            case 'empty' => value = "^$"
            case 'else' => value = "^."
            case 'else2' => value = "^."
            case 'space' => value = "^\\s"
            case 'space A' => value = "^\\s(?=[A-Z\\\\$])"
            case 'space$' => value = "^\\s$"
            case 'a-z' => value = "^[a-z]"
            case 'x' => value = "^x"
            case 'x$' => value = "^x$"
            case 'i$' => value = "^i$"
            case 'letters' => value = "^(?:[a-zA-Z\u{03B1}-\u{03C9}\u{0391}-\u{03A9}?@]|(?:\\\\(?:alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa|lambda|mu|nu|xi|omicron|pi|rho|sigma|tau|upsilon|phi|chi|psi|omega|Gamma|Delta|Theta|Lambda|Xi|Pi|Sigma|Upsilon|Phi|Psi|Omega)(?:\\s+|\\{\\}|(?![a-zA-Z]))))+"
            case '\\greek' => value = "^\\\\(?:alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa|lambda|mu|nu|xi|omicron|pi|rho|sigma|tau|upsilon|phi|chi|psi|omega|Gamma|Delta|Theta|Lambda|Xi|Pi|Sigma|Upsilon|Phi|Psi|Omega)(?:\\s+|\\{\\}|(?![a-zA-Z]))"
            case 'one lowercase latin letter $' => value = "^(?:([a-z])(?:$|[^a-zA-Z]))$"
            case '$one lowercase latin letter$ $' => value = "^\\$(?:([a-z])(?:$|[^a-zA-Z]))\\$$"
            case 'one lowercase greek letter $' => value = "^(?:\\$?[\u{03B1}-\u{03C9}]\\$?|\\$?\\\\(?:alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa|lambda|mu|nu|xi|omicron|pi|rho|sigma|tau|upsilon|phi|chi|psi|omega)\\s*\\$?)(?:\\s+|\\{\\}|(?![a-zA-Z]))$"
            case 'digits' => value = "^[0-9]+"
            case '-9.,9' => value = "^[+\\-]?(?:[0-9]+(?:[,.][0-9]+)?|[0-9]*(?:\\.[0-9]+))"
            case '-9.,9 no missing 0' => value = "^[+\\-]?[0-9]+(?:[.,][0-9]+)?"
            case '_{(state of aggregation)}$' => value = "^_\\{(\\([a-z]{1,3}\\))\\}"
            case '{[(' => value = "^(?:\\\\\\{|\\[|\\()"
            case ')]}' => value = "^(?:\\)|\\]|\\\\\\})"
            case ', ' => value = "^[,;]\\s*"
            case ',' => value = "^[,;]"
            case '.' => value = "^[.]"
            case '. ' => value = "^(\\.|⋅|·|•)\\s*" // 对标/^([.\u22C5\u00B7\u2022])\s*/
            case '...' => value = "^\\.\\.\\.(?=$|[^.])"
            case '* ' => value = "^([*])\\s*"
            case '^a' => value = "^\\^([0-9]+|[^\\\\_])"
            case '^\\x' => value = "^\\^(\\\\[a-zA-Z]+)\\s*"
            case '^(-1)' => value = "^\\^(-?\\d+)"
            case '\'' => value = "^'"
            case '_9' => value = "^_([+\\-]?[0-9]+|[^\\\\])"
            case '_\\x' => value = "^_(\\\\[a-zA-Z]+)\\s*"
            case '^_' => value = "^(?:\\^(?=_)|\\_(?=\\^)|[\\^_]$)"
            case '{}' => value = "^\\{\\}"
            case '=<>' => value = "^[=<>]"
            case '#' => value = "^(#|≡)" // 对标/^[#\u2261]/
            case '+' => value = "^\\+"
            case '-$' => value = "^-(?=[\\s_},;\\]/]|$|\\([a-z]+\\))" // -space -, -; -] -/ -$ -state-of-aggregation
            case '-9' => value = "^-(?=[0-9])"
            case '- orbital overlap' => value = "^-(?=(?:[spd]|sp)(?:$|[\\s,;\\)\\]\\}]))"
            case '-' => value = "^-"
            case 'pm-operator' => value = "^(?:\\\\pm|\\$\\\\pm\\$|\\+-|\\+\\/\\-)"
            case 'operator' => value = "^(?:\\+|(?:[\\-=<>]|<<|>>|\\\\approx|\\$\\\\approx\\$)(?=\\s|$|-?[0-9]))"
            case 'arrowUpDown' => value = "^(?:v|\\(v\\)|\\^|\\(\\^\\))(?=$|[\\s,;\\)\\]\\}])"
            case '->' => value = "^(?:<->|<-->|->|<-|<=>>|<<=>|<=>|→|⟶|⇌)" // |→|⟶|⇌ 对应|[\u{2192}\u{27F6}\u{21CC}]
            case 'CMT' => value = "^[CMT](?=\\[)"
            case '1st-level escape' => value = "^(&|\\\\\\\\|\\\\hline)\\s*"
            case '\\,' => value = "^(?:\\\\[,\\ ;:])" // \\x - but output no space before
            case '\\ca' => value = "^\\\\ca(?:\\s+|(?![a-zA-Z]))"
            case '\\x' => value = "^(?:\\\\[a-zA-Z]+\\s*|\\\\[_&{}%])"
            case 'orbital' => value = "^(?:[0-9]{1,2}[spdfgh]|[0-9]{0,2}sp)(?=$|[^a-zA-Z])" // only those with numbers in front, because the others will be formatted correctly anyway
            case 'others' => value = "^[\\/~|]"
            case 'oxidation$' => value = "^(?:[+-][IVX]+|\\\\pm\\s*0|\\$\\\\pm\\$\\s*0)$"
            case 'd-oxidation$' => value = "^(?:[+-]?\\s?[IVX]+|\\\\pm\\s*0|\\$\\\\pm\\$\\s*0)$" // 0 could be oxidation or charge
            case 'roman numeral' => value = "^[IVX]+"
            case '1/2$' => value = "^[+\\-]?(?:[0-9]+|\\$[a-z]\\$|[a-z])\\/[0-9]+(?:\\$[a-z]\\$|[a-z])?$"
            case '(KV letters),' => value = "^(?:[A-Z][a-z]{0,2}|i)(?=,)"
            case 'uprightEntities' => value = "^(?:pH|pOH|pC|pK|iPr|iBu)(?=$|[^a-zA-Z])"
            case '/' => value = "^\\s*(\\/)\\s*"
            case '//' => value = "^\\s*(\\/\\/)\\s*"
            case '*' => value = "^\\s*[*.]\\s*"
            case _ => return patternsFunc(pattern, input) // 如果是方法
        }

        var r = Regex(value, [RegexFlag.Unicode])
        var match_: ArrayList<String> = ArrayList<String>()

        match (r.find(input, group: true)) {
            case Some(md) =>
                // 统一用 rune 处理
                var runes = input.toRuneArray()

                // 完整匹配位置（第 0 组）
                var p0 = md.matchPosition(0)
                if (p0.start < 0 || p0.end < 0) {
                    return Option<Match>.None
                }
                var remaining = String.empty
                var groupCount = md.groupCount()
                if (groupCount == 1) {
                    var p1 = md.matchPosition(1)
                    if (p1.start >= 0 && p1.end >= 0) {
                        // 只返回捕获组内容（比如 "2"）
                        match_.add(String(runes[p1.start..p1.end]))
                    } else {
                        match_.add(String.empty)
                    }
                    if (p0.end < runes.size) {
                        remaining = String(runes[p0.end..runes.size])
                    }
                } else if (groupCount > 1) {
                    for (i in 1..groupCount + 1) {
                        var pi = md.matchPosition(i)
                        if (pi.start >= 0 && pi.end >= 0) {
                            match_.add(String(runes[pi.start..pi.end]))
                        } else {
                            match_.add(String.empty)
                        }
                    }
                    if (p0.end < runes.size) {
                        remaining = String(runes[p0.end..runes.size])
                    }
                } else {
                    match (byteRangeToRuneRange(input, p0.start, p0.end)) {
                        case Some(fullRange) =>
                            var fullMatch = String(runes[fullRange.start..fullRange.end])
                            match_.add(fullMatch)
                            remaining = String(runes[fullRange.end..runes.size])
                        case None => match_.add(String.empty)
                    }
                }
                return Match(match_, remaining)

            case None => return Option<Match>.None
        }
    }

    func byteRangeToRuneRange(input: String, byteStart: Int64, byteEnd: Int64): Option<RuneRange> {
        if (input.isEmpty() || byteStart < 0 || byteEnd < byteStart) {
            return Option<RuneRange>.None
        }

        var runes = input.toRuneArray()
        var currentByteIndex: Int64 = 0
        var runeStart: Int64 = -1
        var runeEnd: Int64 = -1

        for (i in 0..runes.size) {
            var r = runes[i]
            var byteLen = Rune.utf8Size(r)
            var nextByteIndex = currentByteIndex + byteLen

            if (runeStart == -1 && byteStart >= currentByteIndex && byteStart < nextByteIndex) {
                runeStart = i
            }

            if (byteEnd <= nextByteIndex) {
                runeEnd = i + 1
                break
            }

            currentByteIndex = nextByteIndex
        }

        if (runeEnd == -1) {
            runeEnd = runes.size
        }

        if (runeStart == -1) {
            return Option<RuneRange>.None
        }

        var range = RuneRange()
        range.start = runeStart
        range.end = runeEnd
        return Option.Some(range)
    }

    /**
     * 根据指定模式匹配输入字符串，处理各种化学表达式和格式。
     * 支持指数、分数、状态、颜色、化学式等多种模式，以及调用 findObserveGroups 或 amount 进行进一步解析。
     *
     * @param pattern 要匹配的模式标识
     * @param input 待匹配的字符串
     * @return 匹配结果及剩余字符串，如果未匹配返回空 Match
     */
    private func patternsFunc(pattern: String, input: String): Option<Match> {
        var match_: ArrayList<String> = ArrayList<String>()
        var remainder: String = ""
        match (pattern) {
            case '(-)(9.,9)(e)(99)' =>
                var remainder = input
                let r = Regex("^(\\+\\-|\\+\\/\\-|\\+|\\-|\\\\pm\\s?)?([0-9]+(?:[,.][0-9]+)?|[0-9]*(?:\\.[0-9]+))?(\\((?:[0-9]+(?:[,.][0-9]+)?|[0-9]*(?:\\.[0-9]+))\\))?(?:(?:([eE])|\\s*(\\*|x|\\\\times|\u{00D7})\\s*10\\^)([+\\-]?[0-9]+|\\{[+\\-]?[0-9]+\\}))?")
                match (r.find(input, group: true)) {
                    case Some(md) =>
                        // 捕获组索引从 1 开始
                        for (i in 1..md.groupCount() + 1) {
                            match_.add(md.matchString(i))
                        }
                        // 安全处理 remainder，按 rune 切片
                        let runes = input.toRuneArray()
                        let matchRunes = md.matchString().toRuneArray()
                        remainder = String(runes[matchRunes.size..runes.size])
                        return Match(match_, remainder)
                    case None => return Option<Match>.None
                }
            case '(-)(9)^(-9)' =>
                let r = Regex(
                    "^(\\+\\-|\\+\\/\\-|\\+|\\-|\\pm\\s?)?([0-9]+(?:[,.][0-9]+)?|[0-9]*(?:\\.[0-9]+)?)\\^([+\\-]?[0-9]+|\\{[+\\-]?[0-9]+\\})")
                match (r.find(input, group: true)) {
                    case Some(md) =>
                        for (i in 1..md.groupCount() + 1) {
                            match_.add(md.matchString(i))
                        }
                        let runes = input.toRuneArray()
                        let matchRunes = md.matchString().toRuneArray()
                        remainder = String(runes[matchRunes.size..runes.size])

                        return Match(match_, remainder)
                    case None => return Option<Match>.None
                }

            case 'state of aggregation $' =>
                var a = findObserveGroups(input, "", "^\\([a-z]{1,3}(?=[\\),])", true, ")", "", false) // (aq), (aq,$\infty$), (aq, sat)

                if (let Some(v) <- a && Regex("^($|[\\s,;\\)\\]\\}])").matches(v.remainder)) {
                    return a // AND end of 'phrase'
                }

                let r = Regex("^(?:\\((?:\\ca\\s?)?\\$[amothc]\\$\\))") // OR crystal system ($o$) (\ca$c$)
                match (r.find(input, group: true)) {
                    case Some(md) =>
                        match_.add(md.matchString(0))

                        let runes = input.toRuneArray()
                        let matchRunes = md.matchString().toRuneArray()
                        remainder = String(runes[matchRunes.size..runes.size])

                        return Match(match_, remainder)
                    case None => return Option<Match>.None
                }
            case '^{(...)}' => return findObserveGroups(input, "^{", "", false, "", "}", false)
            case '^($...$)' => return findObserveGroups(input, "^", "$", false, "$", "", false)
            case '^\\x{}{}' => return findObserveGroups(input, "^", "^\\[a-zA-Z]+\\{", true, "}", "", false,
                beg2Excl: "", beg2Incl: "{", end2Incl: "}", end2Excl: "", combine: true)
            case '^\\x{}' => return findObserveGroups(input, "^", "^\\[a-zA-Z]+\\{", true, "}", "", false)
            case '_{(...)}' => return findObserveGroups(input, "_{", "", false, "", "}", false)
            case '_($...$)' => return findObserveGroups(input, "_", "$", false, "$", "", false)
            case '_\\x{}{}' => return findObserveGroups(input, "_", "^\\[a-zA-Z]+\\{", true, "}", "", false,
                beg2Excl: "", beg2Incl: "{", end2Incl: "}", end2Excl: "", combine: true)
            case '_\\x{}' => return findObserveGroups(input, "_", "^\\[a-zA-Z]+\\{", true, "}", "", false)
            case '{...}' => return findObserveGroups(input, "", "{", false, "}", "", false)
            case '{(...)}' => return findObserveGroups(input, "{", "", false, "", "}", false)
            case '$...$' => return findObserveGroups(input, "", "$", false, "$", "", false)
            case #'${(...)}$'# => return findObserveGroups(input, "\${", "", false, "", "}\$", false)
            case '$(...)$' => return findObserveGroups(input, "$", "", false, "", "$", false)
            case '\\bond{(...)}' => return findObserveGroups(input, "\\bond{", "", false, "", "}", false)
            case '[(...)]' => return findObserveGroups(input, "[", "", false, "", "]", false)
            case '\\x{}{}' => return findObserveGroups(input, "", "^\\[a-zA-Z]+\\{", true, "}", "", false, beg2Excl: "",
                beg2Incl: "{", end2Incl: "}", end2Excl: "", combine: true)
            case '\\x{}' => return findObserveGroups(input, "", "^\\\\[a-zA-Z]+\\{", true, "}", "", false)
            case '\\frac{(...)}' => return findObserveGroups(input, "\\frac{", "", false, "", "}", false, beg2Excl: "{",
                beg2Incl: "", end2Incl: "", end2Excl: "}")
            case '\\overset{(...)}' => return findObserveGroups(input, "\\overset{", "", false, "", "}", false,
                beg2Excl: "{", beg2Incl: "", end2Incl: "", end2Excl: "}")
            case '\\underset{(...)}' => return findObserveGroups(input, "\\underset{", "", false, "", "}", false,
                beg2Excl: "{", beg2Incl: "", end2Incl: "", end2Excl: "}")
            case '\\underbrace{(...)}' => return findObserveGroups(input, "\\underbrace{", "", false, "", "}_", false,
                beg2Excl: "{", beg2Incl: "", end2Incl: "", end2Excl: "}")
            case '\\color{(...)}0' => return findObserveGroups(input, "\\color{", "", false, "", "}", false)
            case '\\color{(...)}{(...)}1' => return findObserveGroups(input, "\\color{", "", false, "", "}", false,
                beg2Excl: "{", beg2Incl: "", end2Incl: "", end2Excl: "}")
            case '\\color(...){(...)}2' => return findObserveGroups(input, "\\color", "\\", false, "", "^(?=\\{)", true,
                beg2Excl: "{", beg2Incl: "", end2Incl: "", end2Excl: "}")
            case '\\ce{(...)}' => return findObserveGroups(input, "\\ce{", "", false, "", "}", false)

            case 'amount' =>
                // e.g. 2, 0.5, 1/2, -2, n/2, +;  $a$ could be added later in parsing
                return amount(input)
            case 'amount2' => return amount(input)
            case 'formula$' =>
                if (Regex("^\\([a-z]+\\)$").matches(input)) {
                    // state of aggregation = no formula
                    return Option<Match>.None
                }

                let r = Regex(
                    "^(?:[a-z]|(?:[0-9\\ \\+\\-\\,\\.\\(\\)]+[a-z])+[0-9\\ \\+\\-\\,\\.\\(\\)]*|(?:[a-z][0-9\\ \\+\\-\\,\\.\\(\\)]+)+[a-z]?)$")

                match (r.find(input, group: true)) {
                    case Some(md) =>
                        match_.add(md.matchString(0))

                        let runes = input.toRuneArray()
                        let matchRunes = md.matchString(0).toRuneArray()
                        remainder = String(runes[matchRunes.size..runes.size])

                        return Match(match_, remainder)
                    case None => return Option<Match>.None
                }

            case _ => ()
        }
        return Match(match_, remainder)
    }

    /**
     * 匹配化学式中的数量表达式，例如整数、小数、分数、带符号的量，支持 $…$ 包裹的表达式。
     *
     * @param input 待匹配的字符串
     * @return 匹配结果及剩余字符串，如果未匹配返回 None
     */
    private func amount(input: String): Option<Match> {
        var match_: ArrayList<String> = ArrayList<String>()
        var remainder: String = ""

        // 正则匹配常规数值
        let r = Regex(
            "^(?:(?:(?:\\([+\\-]?[0-9]+\\/[0-9]+\\)|[+\\-]?(?:[0-9]+|\\$[a-z]\\$|[a-z])\\/[0-9]+|[+\\-]?[0-9]+[.,][0-9]+|[+\\-]?\\.[0-9]+|[+\\-]?[0-9]+)(?:[a-z](?=\\s*[A-Z]))?)|[+\\-]?[a-z](?=\\s*[A-Z])|\\+(?!\\s))")
        match (r.find(input, group: true)) {
            case Some(md) =>
                match_.add(md.matchString(0))

                let runes = input.toRuneArray()
                let matchRunes = md.matchString(0).toRuneArray()
                remainder = String(runes[matchRunes.size..runes.size])

                return Match(match_, remainder)
            case None => ()
        }

        // findObserveGroups 匹配 $ ... $
        var a = findObserveGroups(input, "", "$", false, "$", "", false)

        if (let Some(v) <- a) {
            // 2.1 括号表达式，例如 $(2n+1)$
            let r1 = Regex("^\\$\\([0-9a-z+-]+\\)\\$")
            match (r1.find(v.match_[0], group: true)) {
                case Some(md) =>
                    let m = md.matchString(0)
                    match_.add(m)

                    let runes = input.toRuneArray()
                    let matchRunes = m.toRuneArray()
                    remainder = String(runes[matchRunes.size..runes.size])

                    return Match(match_, remainder)
                case None => ()
            }

            // 2.2 数字+字母组合，例如 $n$, $2n$, $2n-1$
            let r2 = Regex("^\\$[0-9]*[a-z][0-9a-z+-]*\\$")
            match (r2.find(v.match_[0], group: true)) {
                case Some(md) =>
                    let m = md.matchString(0)
                    match_.add(m)

                    let runes = input.toRuneArray()
                    let matchRunes = m.toRuneArray()
                    remainder = String(runes[matchRunes.size..runes.size])

                    return Match(match_, remainder)
                case None => ()
            }

            // 2.3 单纯符号，例如 $+$, $-$
            let r3 = Regex("^\\$[+-]\\$")
            match (r3.find(v.match_[0], group: true)) {
                case Some(md) =>
                    let m = md.matchString(0)
                    match_.add(m)

                    let runes = input.toRuneArray()
                    let matchRunes = m.toRuneArray()
                    remainder = String(runes[matchRunes.size..runes.size])

                    return Match(match_, remainder)
                case None => ()
            }
        }

        return Option<Match>.None
    }

    /**
     * 查找字符串中的观察组。
     *
     * 根据给定的开始和结束标记（可选正则），
     * 从源字符串中提取一个或多个连续的匹配片段（观察组）。
     * 支持：
     * - 开始/结束标记包含或不包含在结果中
     * - 可选的第二组匹配（嵌套或连续匹配）
     * - 将多个匹配组合为一个字符串返回
     *
     * @param source 待搜索的源字符串
     * @param begExcl 不包含在结果中的起始匹配
     * @param begIncl 包含在结果中的起始匹配
     * @param isBegInclRegExp begIncl 是否为正则
     * @param endIncl 包含在结果中的结束匹配
     * @param endExcl 不包含在结果中的结束匹配
     * @param isEndExclRegExp endExcl 是否为正则
     * @param beg2Excl 第二组不包含起始匹配（可选）
     * @param beg2Incl 第二组包含起始匹配（可选）
     * @param end2Incl 第二组包含结束匹配（可选）
     * @param end2Excl 第二组不包含结束匹配（可选）
     * @param combine 是否将多个匹配组合为一个字符串返回
     * @return 匹配结果和剩余未匹配字符串，如果未找到匹配则返回 None
     */
    private func findObserveGroups(source: String, begExcl: String, begIncl: String, isBegInclRegExp: Bool,
        endIncl: String, endExcl: String, isEndExclRegExp: Bool, beg2Excl!: String = "", beg2Incl!: String = "",
        end2Incl!: String = "", end2Excl!: String = "", combine!: Bool = false): Option<Match> {
        var match_: ArrayList<String> = ArrayList<String>()
        var remainder: String = ""
        var input = source

        var matchResult = _match(input, begExcl, false)
        if (matchResult.isNone()) {
            return Option<Match>.None
        }

        if (let Some(v) <- matchResult) {
            input = input[v.size..input.size]
        }
        matchResult = _match(input, begIncl, isBegInclRegExp)

        if (matchResult.isNone()) {
            return Option<Match>.None
        }

        var end = ""
        var isRegExp = false
        if (endIncl != "") {
            end = endIncl
        } else {
            end = endExcl
            isRegExp = isEndExclRegExp
        }
        var matchLength = 0
        if (let Some(v) <- matchResult) {
            matchLength = v.size
        }

        var value = _findObserveGroups(input, matchLength, end, isRegExp)

        if (let Some(e) <- value) {
            var endMatch: Int64 = 0
            if (endIncl != "") {
                endMatch = e.endMatchEnd
            } else {
                endMatch = e.endMatchBegin
            }

            let runes = input.toRuneArray()
            var match1 = String(runes[0..endMatch])
            var rest = String(runes[e.endMatchEnd..runes.size])

            if (beg2Excl == "" && beg2Incl == "") {
                match_.add(match1)
                return Match(match_, rest)
            } else {
                var group2 = this.findObserveGroups(rest, beg2Excl, beg2Incl, false, end2Incl, end2Excl, false)
                if (group2.isNone()) {
                    return Option<Match>.None
                }
                var matchRet: ArrayList<String> = ArrayList<String>()
                var str: StringBuilder = StringBuilder()
                matchRet.add(match1)
                str.append(match1)
                if (let Some(v) <- group2) {
                    for (i in 0..v.match_.size) {
                        matchRet.add(v.match_[i])
                        str.append(v.match_[i])
                    }

                    if (combine) {
                        var matchRetStr: ArrayList<String> = ArrayList<String>()
                        matchRetStr.add(str.toString())
                        // 返回字符串
                        return Match(matchRetStr, v.remainder)
                    } else {
                        // 返回数组
                        return Match(matchRet, v.remainder)
                    }
                } else {
                    return Option<Match>.None
                }
            }
        } else {
            return Option<Match>.None
        }
    }

    /**
     * 匹配输入字符串。
     *
     * 根据 isRegExp 判断使用正则匹配或普通字符串前缀匹配：
     * - isRegExp 为 true 时：使用正则 pattern 匹配输入，返回第一个匹配。
     * - isRegExp 为 false 时：检查输入是否以 pattern 开头，匹配则返回 pattern。
     *
     * @param input 待匹配的字符串
     * @param pattern 正则或普通字符串模式
     * @param isRegExp 是否使用正则匹配
     * @return 匹配结果字符串，如果未匹配则返回 None
     */
    private func _match(input: String, pattern: String, isRegExp: Bool): Option<String> {
        if (isRegExp) {
            // 正则类型
            var regex = Regex(pattern)
            match (regex.find(input)) {
                case Some(md) => return md.matchString(0)
                case None => return Option<String>.None
            }
        } else {
            //字符串类型
            if (input.indexOf(pattern) != 0) {
                return Option<String>.None
            }
            return pattern
        }
    }

    /**
     * 查找观察组的结束位置（支持嵌套大括号）。
     *
     * 从指定索引开始扫描输入字符串，寻找符合 endChars 的结束位置。
     * 在查找过程中，会正确处理大括号嵌套：
     * - 遇到 `{` 时增加嵌套层数
     * - 遇到 `}` 时减少嵌套层数
     * 只有在嵌套层为 0 时匹配结束标记才生效。
     *
     * @param input 待搜索的字符串
     * @param index 开始扫描的索引
     * @param endChars 结束匹配字符或正则
     * @param isRegExp endChars 是否为正则
     * @return 匹配的起止位置 MatchPosition，如果未找到或嵌套不匹配返回 None
     */
    private func _findObserveGroups(
        input: String,
        index: Int64,
        endChars: String,
        isRegExp: Bool
    ): Option<MatchPosition> {
        let runes = input.toRuneArray()
        var braces = 0
        var i: Int64 = index

        while (i < runes.size) {
            let a = String([runes[i]])
            let rest = String(runes[i..runes.size])
            let m = _match(rest, endChars, isRegExp)

            if (m.isSome() && braces == 0 && let Some(v) <- m) {
                return MatchPosition(i, i + v.size)
            } else if (a == "{") {
                braces++
            } else if (a == "}") {
                if (braces == 0) {
                    println("[MhchemParser-_findObserveGroups] Extra close brace or missing open brace")
                    return Option<MatchPosition>.None
                } else {
                    braces--
                }
            }
            i++
        }

        if (braces > 0) {
            return Option<MatchPosition>.None
        }

        return Option<MatchPosition>.None
    }

    /**
     * 判断指定状态机是否支持某种类型的动作或输出。
     *
     * @param stateMachine 状态机名称（如 "ce", "text", "pq" 等）
     * @param type_ 要检测的动作类型或输出类型
     * @return 如果状态机支持该类型返回 true，否则返回 false
     */
    private func machineActionsHasType(stateMachine: String, type_: String): Bool {
        var hasType = false

        if (stateMachine == "ce") {
            match (type_) {
                case 'o after d' => hasType = true
                case 'd= kv' => hasType = true
                case 'charge or bond' => hasType = true
                case '- after o/d' => hasType = true
                case 'a to o' => hasType = true
                case 'sb=true' => hasType = true
                case 'sb=false' => hasType = true
                case 'beginsWithBond=true' => hasType = true
                case 'beginsWithBond=false' => hasType = true
                case 'parenthesisLevel++' => hasType = true
                case 'parenthesisLevel--' => hasType = true
                case 'state of aggregation' => hasType = true
                case 'comma' => hasType = true
                case 'output' => hasType = true
                case 'oxidation-output' => hasType = true
                case 'frac-output' => hasType = true
                case 'overset-output' => hasType = true
                case 'underset-output' => hasType = true
                case 'underbrace-output' => hasType = true
                case 'color-output' => hasType = true
                case 'r=' => hasType = true
                case 'rdt=' => hasType = true
                case 'rd=' => hasType = true
                case 'rqt=' => hasType = true
                case 'rq=' => hasType = true
                case 'operator' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "text") {
            match (type_) {
                case 'output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "pq") {
            match (type_) {
                case 'state of aggregation' => hasType = true
                case 'color-output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "bd") {
            match (type_) {
                case 'color-output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "oxidation") {
            match (type_) {
                case 'roman-numeral' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "tex-math") {
            match (type_) {
                case 'output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "tex-math tight") {
            match (type_) {
                case 'tight operator' => hasType = true
                case 'output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "9,9") {
            match (type_) {
                case 'comma' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "pu") {
            match (type_) {
                case 'enumber' => hasType = true
                case 'number^' => hasType = true
                case 'operator' => hasType = true
                case 'space' => hasType = true
                case 'output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "pu-2") {
            match (type_) {
                case 'cdot' => hasType = true
                case '^(-1)' => hasType = true
                case 'space' => hasType = true
                case 'output' => hasType = true
                case _ => ()
            }
        } else if (stateMachine == "pu-9,9") {
            match (type_) {
                case 'comma' => hasType = true
                case 'output-0' => hasType = true
                case 'output-o' => hasType = true
                case _ => ()
            }
        }

        return hasType
    }

    /**
     * 判断 MhchemParser 是否支持某种类型的动作。
     *
     * @param type_ 要检测的动作类型字符串
     * @return 如果 MhchemParser 支持该类型返回 true，否则返回 false
     */
    private func mhchemParserActionsHasType(type_: String): Bool {
        var hasType = false

        match (type_) {
            case 'a=' => hasType = true
            case 'b=' => hasType = true
            case 'p=' => hasType = true
            case 'o=' => hasType = true
            case 'q=' => hasType = true
            case 'd=' => hasType = true
            case 'rm=' => hasType = true
            case 'text=' => hasType = true
            case 'insert' => hasType = true
            case 'insert+p1' => hasType = true
            case 'insert+p1+p2' => hasType = true
            case 'copy' => hasType = true
            case 'rm' => hasType = true
            case 'text' => hasType = true
            case '{text}' => hasType = true
            case 'tex-math' => hasType = true
            case 'tex-math tight' => hasType = true
            case 'bond' => hasType = true
            case 'color0-output' => hasType = true
            case 'ce' => hasType = true
            case '1/2' => hasType = true
            case '9,9' => hasType = true
            case _ => ()
        }

        return hasType
    }

    private func goArray(ret: PVList, arr: ArrayList<String>, state: String) {
        for (s in arr) {
            ret.add(PVList(go(s, stateMachine: state)))
        }
    }

    /**
     * 将 ParserValue b 拼接到列表 a 中。
     *
     * 如果 b 是 PVList，则将其中所有元素加入 a；
     * 否则直接将 b 加入 a。
     *
     * @param a 目标列表
     * @param b 可选的 ParserValue
     */
    private func concatArray(a: ArrayList<ParserValue>, b: Option<ParserValue>) {
        if (let Some(v) <- b) {
            if (v is PVList) {
                match (v as PVList) {
                    case Some(list) => for (item in list.getItems()) {
                        a.add(item)
                    }
                    case None => return
                }
            } else {
                a.add(v)
            }
        }
    }
}

struct RuneRange {
    var start: Int64 = 0
    var end: Int64 = 0
}