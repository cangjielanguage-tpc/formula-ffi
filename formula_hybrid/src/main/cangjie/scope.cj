package formula

import ohos.ark_interop.*

@C
struct JSHClassPrivate {}

@C
struct JSScopePrivate {}

type JSEnv = IntNative

type JSScope = CPointer<JSScopePrivate>

type JSHClass = CPointer<JSHClassPrivate>

type JSAsyncCallback = CFunc<(JSEnv, Int64) -> Unit>

/**
 * Alias for type which is `CPointer<Unit>`.
 */
public type napi_env = CPointer<Unit>

/**
 * Alias for type which is `CPointer<Unit>`.
 */
public type napi_value = CPointer<Unit>

@FastNative
foreign func openScope(env: JSEnv): JSScope

@FastNative
foreign func closeScope(env: JSEnv, scope: JSScope): Unit

/**
* @param runtime {JSContext} in this context you want to create values and call arkts functions/methods
* @param callback {(} -> T) the lambda function holds the logic
*/
public func newScope<T>(runtime: JSContext, callback: () -> T): T {
    unsafe {
        let scope = openScope(runtime.env)
        let result = try {
            callback()
        } catch (e: Exception) {
            throw e
        } catch (e: Error) {
            throw e
        } finally {
            closeScope(runtime.env, scope)
        }
        return result
    }
}