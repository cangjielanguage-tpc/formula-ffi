package formula.mhchem

/**
 * mhchem 规则加载器。
 * 负责读取状态机配置，并构建用于化学公式解析的 transitionsMap。
 */
class MhchemJsonParser {

    // 状态转移表
    // 第一层 key：规则名称
    // 第二层 key：状态名（如 "0" / "o" / "*"）
    // value：Transitions 列表
    protected static var transitionsMap: HashMap<String, HashMap<String, ArrayList<Transitions>>> = HashMap<String, HashMap<String,
            ArrayList<Transitions>>>()

    /**
     * 加载并解析化学公式解析规则。
     * 读取化学公式规则配置，并将其解析为状态转换结构，缓存到内存中，供化学公式解析器在处理 \ce{...} 语法时使用。
     *
     * 主要流程：
     * 1. 清空已有规则缓存
     * 2. 读取 mhchemData 内容
     * 3. 解析多层规则结构（规则 → 状态 → 转换）
     * 4. 构建并保存状态转换映射表
     */
    static func loadJson() {
        if (!transitionsMap.isEmpty()) {
            return
        }
        var root = JsonValue.fromStr(mhchemData).asObject()

        // 第一层：规则 key
        var fields: HashMap<String, JsonValue> = root.getFields()
        for ((key, value) in fields) {
            var jsonObject = value.asObject()
            var transitions: HashMap<String, ArrayList<Transitions>> = HashMap<String, ArrayList<Transitions>>()

            // 第二层：状态名（"0" / "o" / "*"）
            for ((stateName, stateValue) in jsonObject.getFields()) {
                var list: ArrayList<Transitions> = ArrayList<Transitions>()
                var transArray = stateValue.asArray()

                // 第三层：Transitions 数组
                for (item in transArray.getItems()) {
                    var obj = item.asObject()

                    // pattern
                    var pattern = ""
                    if (let Some(v) <- obj.get("pattern")) {
                        pattern = v.asString().getValue()
                    }

                    // task
                    var taskObj: JsonObject = JsonObject()
                    if (let Some(v) <- obj.get("task")) {
                        taskObj = v.asObject()
                    }

                    var task = parseTask(taskObj)
                    list.add(Transitions(pattern, task))
                }
                transitions.add(stateName, list)
            }
            transitionsMap.add(key, transitions)
        }
    }

    /**
     * 解析 task 节点，将 JSON 中的 task 对象转换为 Task 实例
     */
    private static func parseTask(obj: JsonObject): Task {
        // action_ 列表
        var actionList: ArrayList<Action> = ArrayList<Action>()

        var actionArr: JsonArray = JsonArray()
        if (let Some(v) <- obj.get("action_")) {
            actionArr = v.asArray()
        }

        for (a in actionArr.getItems()) {
            var aObj = a.asObject()

            var type_ = ""
            if (let Some(v) <- aObj.get("type_")) {
                type_ = v.asString().getValue()
            }

            var option: Option<String> = Option<String>.None
            var optionNum: Option<Int64> = Option<Int64>.None
            var optionBool: Option<Bool> = Option<Bool>.None

            if (let Some(opt) <- aObj.get("option")) {
                match (opt.kind()) {
                    case JsString => option = opt.asString().getValue()
                    case JsInt => optionNum = opt.asInt().getValue()
                    case JsBool => optionBool = opt.asBool().getValue()
                    case _ => ()
                }
            }

            actionList.add(
                Action(
                    type_,
                    option: option,
                    optionNum: optionNum,
                    optionBool: optionBool
                )
            )
        }

        // stateArray
        var stateArray: ArrayList<String> = ArrayList<String>()
        var stateArrJson: JsonArray = JsonArray()
        if (let Some(v) <- obj.get("stateArray")) {
            stateArrJson = v.asArray()
        }

        for (s in stateArrJson.getItems()) {
            stateArray.add(s.asString().getValue())
        }

        // 可选字段：revisit
        var revisit = false
        if (let Some(v) <- obj.get("revisit")) {
            revisit = v.asBool().getValue()
        }

        // 可选字段：toContinue
        var toContinue = false
        if (let Some(v) <- obj.get("toContinue")) {
            toContinue = v.asBool().getValue()
        }

        // 可选字段：nextState
        var nextState = ""
        if (let Some(v) <- obj.get("nextState")) {
            nextState = v.asString().getValue()
        }

        return Task(
            actionList,
            stateArray,
            revisit: revisit,
            toContinue: toContinue,
            nextState: nextState
        )
    }
}
