// Source: Absorbed from /data/dev/bun/src/js_parser/ast/KnownGlobal.cp | Synced: 2026-11-21T13:03:52Z

/// 全局符号所属运行时/规范命名空间
pub enum GlobalNamespace: u8 {
    ecmascript = 0,
    web_api = 1,
    node_runtime = 2,
    bun_runtime = 3,
}

/// 已知全局符号元数据（用于 Tree-Shaking、纯构造判定与 `new` 关键字化简）
pub struct GlobalInfo {
    name: string,
    namespace: GlobalNamespace,
    can_drop_new: bool = false,
    pure_zero_arg_new: bool = false,
    pure_read: bool = true,

    fn isEcmaScript(): bool {
        return _.namespace == GlobalNamespace.ecmascript;
    }

    fn isWebApi(): bool {
        return _.namespace == GlobalNamespace.web_api;
    }

    fn isNodeRuntime(): bool {
        return _.namespace == GlobalNamespace.node_runtime;
    }

    fn isBunRuntime(): bool {
        return _.namespace == GlobalNamespace.bun_runtime;
    }
}

const KNOWN_GLOBALS: [69]GlobalInfo = [
    // ECMAScript 核心对象与构造器
    GlobalInfo{ name: "Object", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "Array", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "Function", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: false },
    GlobalInfo{ name: "Error", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "TypeError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "SyntaxError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "RangeError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "ReferenceError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "EvalError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "URIError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: true },
    GlobalInfo{ name: "AggregateError", namespace: GlobalNamespace.ecmascript, can_drop_new: true, pure_zero_arg_new: false },
    GlobalInfo{ name: "Map", namespace: GlobalNamespace.ecmascript, can_drop_new: false, pure_zero_arg_new: true },
    GlobalInfo{ name: "Set", namespace: GlobalNamespace.ecmascript, can_drop_new: false, pure_zero_arg_new: true },
    GlobalInfo{ name: "WeakMap", namespace: GlobalNamespace.ecmascript, can_drop_new: false, pure_zero_arg_new: true },
    GlobalInfo{ name: "WeakSet", namespace: GlobalNamespace.ecmascript, can_drop_new: false, pure_zero_arg_new: true },
    GlobalInfo{ name: "Date", namespace: GlobalNamespace.ecmascript, can_drop_new: false, pure_zero_arg_new: true },
    GlobalInfo{ name: "RegExp", namespace: GlobalNamespace.ecmascript, can_drop_new: false, pure_zero_arg_new: true },
    GlobalInfo{ name: "Math", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "JSON", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Reflect", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Symbol", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Promise", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Number", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "String", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Boolean", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "BigInt", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Proxy", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "ArrayBuffer", namespace: GlobalNamespace.ecmascript, pure_zero_arg_new: true },
    GlobalInfo{ name: "DataView", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Uint8Array", namespace: GlobalNamespace.ecmascript, pure_zero_arg_new: true },
    GlobalInfo{ name: "Int32Array", namespace: GlobalNamespace.ecmascript, pure_zero_arg_new: true },
    GlobalInfo{ name: "Float64Array", namespace: GlobalNamespace.ecmascript, pure_zero_arg_new: true },
    GlobalInfo{ name: "WeakRef", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "FinalizationRegistry", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Atomics", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Intl", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "globalThis", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "Infinity", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "NaN", namespace: GlobalNamespace.ecmascript },
    GlobalInfo{ name: "undefined", namespace: GlobalNamespace.ecmascript },
    // Web / WHATWG API 全局符号
    GlobalInfo{ name: "Headers", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "Response", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "Request", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "TextEncoder", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "TextDecoder", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "URL", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "URLSearchParams", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "AbortController", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "AbortSignal", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "Blob", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "File", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "FormData", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "ReadableStream", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "WritableStream", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "TransformStream", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "Event", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "EventTarget", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "CustomEvent", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "DOMException", namespace: GlobalNamespace.web_api, pure_zero_arg_new: true },
    GlobalInfo{ name: "console", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "crypto", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "performance", namespace: GlobalNamespace.web_api },
    GlobalInfo{ name: "fetch", namespace: GlobalNamespace.web_api },
    // Node.js & Bun 运行时全局符号
    GlobalInfo{ name: "Buffer", namespace: GlobalNamespace.node_runtime },
    GlobalInfo{ name: "process", namespace: GlobalNamespace.node_runtime },
    GlobalInfo{ name: "global", namespace: GlobalNamespace.node_runtime },
    GlobalInfo{ name: "setImmediate", namespace: GlobalNamespace.node_runtime },
    GlobalInfo{ name: "clearImmediate", namespace: GlobalNamespace.node_runtime },
    GlobalInfo{ name: "Bun", namespace: GlobalNamespace.bun_runtime },
];

/// 按精确大小写查询已知全局符号元数据，未知用户标识符返回 `null`。
pub fn lookupGlobal(name: string): GlobalInfo? {
    if (name.len == 0) {
        return null;
    }
    for (entry of KNOWN_GLOBALS) {
        if (std.mem.eql(u8, name, entry.name)) {
            return entry;
        }
    }
    return null;
}

/// 判断标识符是否为已知全局符号（大小写敏感）
pub fn isKnownGlobal(name: string): bool {
    return lookupGlobal(name) != null;
}

/// 判断 `new Name()` 无参构造是否无副作用（可在结果未使用时被 Tree-Shaking 移除）
pub fn isPureZeroArgConstructor(name: string): bool {
    if (info := lookupGlobal(name)) {
        return info.pure_zero_arg_new;
    }
    return false;
}

/// 判断 `new Name(...)` 是否可安全化简去掉 `new` 关键字（如 `Error`/`Object`/`Array`/`Function`，而 `RegExp` 不可）
pub fn canDropNewKeyword(name: string): bool {
    if (info := lookupGlobal(name)) {
        return info.can_drop_new;
    }
    return false;
}

fn inList(target: string, list: []const string): bool {
    for (item of list) {
        if (std.mem.eql(u8, target, item)) {
            return true;
        }
    }
    return false;
}

const MATH_PROPS: [24]string = [
    "E", "LN10", "LN2", "LOG10E", "LOG2E", "PI", "SQRT1_2", "SQRT2",
    "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "exp",
    "floor", "log", "max", "min", "pow", "round", "sin", "sqrt",
];

const SYMBOL_PROPS: [12]string = [
    "asyncIterator", "for", "hasInstance", "isConcatSpreadable", "iterator",
    "keyFor", "match", "replace", "search", "species", "split", "toStringTag",
];

const OBJECT_PROPS: [14]string = [
    "assign", "create", "defineProperty", "entries", "freeze", "fromEntries",
    "getOwnPropertyDescriptor", "getOwnPropertyNames", "getPrototypeOf",
    "hasOwn", "is", "keys", "prototype", "values",
];

const ARRAY_PROPS: [4]string = ["from", "isArray", "of", "prototype"];

const NUMBER_PROPS: [11]string = [
    "EPSILON", "MAX_SAFE_INTEGER", "MAX_VALUE", "MIN_SAFE_INTEGER", "MIN_VALUE",
    "NaN", "NEGATIVE_INFINITY", "POSITIVE_INFINITY", "isFinite", "isInteger", "isNaN",
];

const STRING_PROPS: [4]string = ["fromCharCode", "fromCodePoint", "prototype", "raw"];
const JSON_PROPS: [2]string = ["parse", "stringify"];
const PROMISE_PROPS: [7]string = ["all", "allSettled", "any", "prototype", "race", "reject", "resolve"];
const REFLECT_PROPS: [6]string = ["apply", "construct", "get", "has", "ownKeys", "set"];
const DATE_PROPS: [4]string = ["UTC", "now", "parse", "prototype"];

/// 判断全局对象静态成员访问（如 `Math.PI`、`Symbol.iterator`、`Object.keys`）是否无副作用。
pub fn isPurePropertyAccess(global_name: string, prop: string): bool {
    if (global_name.len == 0 || prop.len == 0) {
        return false;
    }
    if (std.mem.eql(u8, global_name, "Math")) { return inList(prop, MATH_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Symbol")) { return inList(prop, SYMBOL_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Object")) { return inList(prop, OBJECT_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Array")) { return inList(prop, ARRAY_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Number")) { return inList(prop, NUMBER_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "String")) { return inList(prop, STRING_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "JSON")) { return inList(prop, JSON_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Promise")) { return inList(prop, PROMISE_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Reflect")) { return inList(prop, REFLECT_PROPS[0..]); }
    if (std.mem.eql(u8, global_name, "Date")) { return inList(prop, DATE_PROPS[0..]); }
    return false;
}

const PURE_MATH_CALLS: [16]string = [
    "abs", "acos", "asin", "atan", "atan2", "ceil", "cos", "exp",
    "floor", "log", "max", "min", "pow", "round", "sin", "sqrt",
];

const PURE_NUMBER_CALLS: [3]string = ["isFinite", "isInteger", "isNaN"];

/// 判断全局静态函数调用（假设实参为纯原始值）是否无副作用，可用于死代码消除（DCE）。
pub fn isPureGlobalCall(global_name: string, prop: string): bool {
    if (global_name.len == 0 || prop.len == 0) {
        return false;
    }
    if (std.mem.eql(u8, global_name, "Math")) { return inList(prop, PURE_MATH_CALLS[0..]); }
    if (std.mem.eql(u8, global_name, "Number")) { return inList(prop, PURE_NUMBER_CALLS[0..]); }
    if (std.mem.eql(u8, global_name, "Array")) { return std.mem.eql(u8, prop, "isArray"); }
    if (std.mem.eql(u8, global_name, "Object")) { return std.mem.eql(u8, prop, "is"); }
    if (std.mem.eql(u8, global_name, "String")) {
        return std.mem.eql(u8, prop, "fromCharCode") || std.mem.eql(u8, prop, "fromCodePoint");
    }
    if (std.mem.eql(u8, global_name, "Date")) {
        return std.mem.eql(u8, prop, "now") || std.mem.eql(u8, prop, "UTC");
    }
    if (std.mem.eql(u8, global_name, "Symbol")) {
        return std.mem.eql(u8, prop, "for");
    }
    return false;
}
