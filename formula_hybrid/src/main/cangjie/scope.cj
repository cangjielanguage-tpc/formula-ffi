package formula
internal import ohos.ark_interop.JSContext

@FastNative
foreign func napi_open_handle_scope(env: CPointer<Unit>, scope: CPointer<CPointer<Unit>>): Int32

@FastNative
foreign func napi_close_handle_scope(env: CPointer<Unit>, scope: CPointer<Unit>): Int32

public func openScope(env:CPointer<Unit>):CPointer<Unit>{
    var scopePtr:CPointer<Unit> = CPointer<Unit>()
    let res = unsafe {
        napi_open_handle_scope(env,inout scopePtr)
    }
    if(res != 0){
        throw Exception("napi_open_handle_scope failed")
    }
    return scopePtr
}

public func closeScope(env:CPointer<Unit>,scope:CPointer<Unit>):Int32{
    return unsafe {napi_close_handle_scope(env,scope)}
}

func newScope<T>(runtime: JSContext, callback: () -> T): T {
    unsafe {
        let scope = openScope(runtime.getNapiEnv())
        let result = try {
            callback()
        } catch (e: Exception) {
            throw e
        } catch (e: Error) {
            throw e
        } finally {
            closeScope(runtime.getNapiEnv(), scope)
        }
        return result
    }
}