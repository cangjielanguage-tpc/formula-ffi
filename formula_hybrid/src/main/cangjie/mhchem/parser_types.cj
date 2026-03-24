package formula.mhchem

// 用于化学式解析器的中间数据结构、状态机与 AST 表示

/**
 * 正则或模式匹配的返回结果
 * - match_    : 匹配到的字符串数组
 * - remainder : 剩余未消费的输入
 */
class Match {

    // 匹配结果（可能是单个字符串，也可能是多个分组）
    var match_: ArrayList<String> = ArrayList<String>()

    // 剩余未解析的输入字符串
    var remainder: String = ""

    init(match_: ArrayList<String>, remainder: String) {
        this.match_ = match_
        this.remainder = remainder
    }
}

/**
 * 查找观察分组（observe group）时的位置信息
 * 用于记录结束符的位置
 */
class MatchPosition {

    // 结束匹配的起始位置（不包含结束符）
    var endMatchBegin: Int64 = 0

    // 结束匹配的结束位置（包含结束符）
    var endMatchEnd: Int64 = 0

    init(endMatchBegin: Int64, endMatchEnd: Int64) {
        this.endMatchBegin = endMatchBegin
        this.endMatchEnd = endMatchEnd
    }
}

/**
 * 解析过程中的临时缓存
 * 用于保存一个化学符号或结构在解析中的各个组成部分
 */
class Buffer {

    // 左上角上标 / 数量（如 2H 中的 2）
    var a: Option<String> = Option<String>.None

    // 元素符号（如 H、Na、Cl）
    var o: Option<String> = Option<String>.None

    // 左上角上标（另一种标记方式）
    var b: Option<String> = Option<String>.None

    // 左下角下标（如 H₂ 中的 2）
    var p: Option<String> = Option<String>.None

    // 右下角下标
    var q: Option<String> = Option<String>.None

    // 右上角上标
    var d: Option<String> = Option<String>.None

    // 箭头类型（如 ->, <-, ⇌）
    var r: String = ""

    // 箭头上方文本类型
    var rdt: String = ""

    // 箭头上方文本内容
    var rd: String = ""

    // 箭头下方文本类型
    var rqt: String = ""

    // 箭头下方文本内容
    var rq: String = ""

    // 普通文本缓存
    var text_: String = ""

    // 特殊标记（如反应条件标志）
    var rm: String = ""

    // 箭头上标类型（如 'kv'、'oxidation'）
    var dType: Option<String> = Option<String>.None

    // 当前括号嵌套层级
    var parenthesisLevel: Int32 = 0

    // 前一个 token 前是否有空格
    var sb: Bool = false

    // 是否以键合符号（- = ≡ 等）开头
    var beginsWithBond: Bool = false

    /**
     * 清空所有缓存字段（包括括号层级和键合状态）
     */
    func clearAll() {
        a = Option<String>.None
        o = Option<String>.None
        b = Option<String>.None
        p = Option<String>.None
        q = Option<String>.None
        d = Option<String>.None
        r = ""
        rdt = ""
        rd = ""
        rqt = ""
        rq = ""
        text_ = ""
        rm = ""
        dType = Option<String>.None
        sb = false
        parenthesisLevel = 0
        beginsWithBond = false
    }

    /**
     * 清空当前缓存
     * 保留括号层级parenthesisLevel 和 beginsWithBond 状态
     */
    func clear() {
        a = Option<String>.None
        o = Option<String>.None
        b = Option<String>.None
        p = Option<String>.None
        q = Option<String>.None
        d = Option<String>.None
        r = ""
        rdt = ""
        rd = ""
        rqt = ""
        rq = ""
        text_ = ""
        rm = ""
        dType = Option<String>.None
        sb = false
    }
}

/**
 * 状态机中的一个动作定义
 */
class Action {

    // 动作类型名称
    var type_: String

    // 字符串参数
    var option: Option<String>

    // 数值参数
    var optionNum: Option<Int64>

    // 布尔参数
    var optionBool: Option<Bool>

    init(
        type_: String,
        option!: Option<String> = Option<String>.None,
        optionNum!: Option<Int64> = Option<Int64>.None,
        optionBool!: Option<Bool> = Option<Bool>.None
    ) {
        this.type_ = type_
        this.option = option
        this.optionNum = optionNum
        this.optionBool = optionBool
    }
}

/**
 * 状态机中的一个任务
 * 包含动作列表和状态迁移信息
 */
class Task {

    // 要执行的动作列表
    var action_: ArrayList<Action> = ArrayList<Action>()

    // 允许的状态集合
    var stateArray: ArrayList<String> = ArrayList<String>()

    // 是否需要回溯
    var revisit: Bool

    // 是否继续解析
    var toContinue: Bool

    // 下一个状态名
    var nextState: String

    init(
        action_: ArrayList<Action>,
        stateArray: ArrayList<String>,
        revisit!: Bool = false,
        toContinue!: Bool = false,
        nextState!: String = ""
    ) {
        this.action_ = action_
        this.stateArray = stateArray
        this.revisit = revisit
        this.toContinue = toContinue
        this.nextState = nextState
    }
}

/**
 * 状态转移定义
 * pattern 命中后执行对应 task
 */
class Transitions {

    // 匹配用的模式名
    var pattern: String = ""

    // 对应的任务
    var task: Task = Task(ArrayList<Action>(), ArrayList<String>())

    init(pattern: String, task: Task) {
        this.pattern = pattern
        this.task = task
    }
}

/**
 * 解析值的公共接口
 */
interface ParserValue {}

/**
 * 字符串类型的解析值
 */
class PVString <: ParserValue {

    // 实际字符串值
    private var value: String

    init(value: String) {
        this.value = value
    }

    func getValue(): String {
        return value
    }
}

/**
 * 列表类型的解析值
 * 用于表示一组子节点
 */
class PVList <: ParserValue {

    // 子节点列表
    private var items: ArrayList<ParserValue> = ArrayList<ParserValue>()

    init() {}

    init(items: ArrayList<ParserValue>) {
        this.items = items
    }

    // 添加一个子节点
    func add(value: ParserValue) {
        items.add(value)
        return this
    }

    // 获取子节点列表
    func getItems(): ArrayList<ParserValue> {
        return items
    }

    // 是否为空列表
    func isEmpty(): Bool {
        return items.size == 0
    }
}

/**
 * 对象类型的解析值
 * 用于表示带字段的 AST 节点
 */
class PVObject <: ParserValue {

    // 节点类型标识
    private var type_: String

    // 字段映射表
    private var fields: HashMap<String, ParserValue> = HashMap<String, ParserValue>()

    init(type_: String) {
        this.type_ = type_
    }

    func getType(): String {
        return type_
    }

    // 设置字段
    func put(key: String, value: ParserValue) {
        fields.add(key, value)
        return this
    }

    // 获取字段（原始值）
    func get(key: String): ?ParserValue {
        if (fields.contains(key)) {
            return fields.get(key)
        }
        return None
    }

    // 安全获取列表字段
    func getList(key: String): PVList {
        var v: ?ParserValue = get(key)
        if (let Some(value) <- v) {
            if (value is PVList) {
                match (value as PVList) {
                    case Some(v) => return v
                    case None => return PVList()
                }
            }
        }
        return PVList()
    }

    // 安全获取字符串字段
    func getString(key: String): String {
        var v: ?ParserValue = get(key)
        if (let Some(value) <- v) {
            if (value is PVString) {
                match (value as PVString) {
                    case Some(v) => return v.getValue()
                    case None => return ""
                }
            }
        }
        return ""
    }
}