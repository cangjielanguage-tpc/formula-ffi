/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025-2026. All rights reserved.
 */
package formula

import ohos.ark_interop.JSModule
import ohos.ark_interop.JSCallInfo
import ohos.ark_interop.JSValue
import ohos.ark_interop.JSContext
import std.sync.Mutex

let latexReentrantMutex: Mutex = Mutex()

func latexStringToImage(runtime: JSContext, callInfo: JSCallInfo): JSValue {
    let value0 = callInfo[0].toString(runtime)
    let value1 = callInfo[1].toNumber()
    let value2 = callInfo[2].toNumber()
    let value3 = callInfo[3].toNumber()
    let value4 = callInfo[4].toNumber()
    let value5 = callInfo[5].toString(runtime)
    let promise = runtime.promiseCapability()
    spawn {
        synchronized(latexReentrantMutex) {
            let result = latexStringToImage(value0, value1, UInt32(value2), UInt32(value3), Int64(value4), value5)
            runtime.postJSTask {
                promise.resolve(runtime.arrayBuffer(result).toJSValue())
            }
        }
    }
    promise.toJSValue()
}

class LatexResultData {
    public var isSuccess: Bool = false
    public var imageBytes: Array<Byte> = Array<Byte>()
    public var formula: String = ""
    public var errorCode: Int32 = 0
    public var errorMessage: String = ""
}

func latexStringToImageWithError(runtime: JSContext, callInfo: JSCallInfo): JSValue {
    let value0 = callInfo[0].toString(runtime)
    let value1 = callInfo[1].toNumber()
    let value2 = callInfo[2].toNumber()
    let value3 = callInfo[3].toNumber()
    let value4 = callInfo[4].toNumber()
    let value5 = callInfo[5].toString(runtime)
    let promise = runtime.promiseCapability()
    spawn {
        let data = LatexResultData()
        synchronized(latexReentrantMutex) {
            let result = latexStringToImageWithError(value0, value1, UInt32(value2), UInt32(value3), Int64(value4), value5)
            data.isSuccess = result.isSuccess
            data.imageBytes = result.imageBytes
            data.formula = result.formula
            data.errorCode = result.errorCode.toInt()
            data.errorMessage = result.errorMessage
        }
        let successNum: Float64 = if (data.isSuccess) { 1.0 } else { 0.0 }
        let errorCodeNum: Float64 = Float64(data.errorCode)
        runtime.postJSTask {
            let obj = runtime.object()
            obj["isSuccess"] = runtime.number(successNum).toJSValue()
            obj["imageBytes"] = runtime.arrayBuffer(data.imageBytes).toJSValue()
            obj["formula"] = runtime.string(data.formula).toJSValue()
            obj["errorCode"] = runtime.number(errorCodeNum).toJSValue()
            obj["errorMessage"] = runtime.string(data.errorMessage).toJSValue()
            promise.resolve(obj.toJSValue())
        }
    }
    promise.toJSValue()
}

let EXPORT_MODULE = JSModule.registerModule {
    runtime, exports => 
        exports["latexStringToImage"] = runtime.function(latexStringToImage).toJSValue()
        exports["latexStringToImageWithError"] = runtime.function(latexStringToImageWithError).toJSValue()
}
