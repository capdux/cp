// Source: Absorbed from /data/dev/bun/src/bundler/defines-table.cp | Synced: 2026-11-22T08:11:00Z
// Pure-CP zero-allocation ECMAScript/Web global identifier purity, constant-foldability,
// and no-side-effect property-access classification tables for bundler tree-shaking (DCE)
// and expression reordering optimization.

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[start..end];
}

/// 全局标识符的纯净度与常量折叠分类（对应上游 `PureGlobalIdentifierValue`）
pub enum PureGlobalKind: u8 {
    unknown = 0,
    nan_literal = 1,
    infinity_literal = 2,
    strict_undefined = 3,
    pure_identifier = 4,
}

/// 针对单个全局表达式的 DCE/重排优化标志位
pub struct PureGlobalFlags {
    can_be_removed_if_unused: bool,
    is_constant_foldable: bool,
    safe_to_reorder_read: bool,
    call_is_pure_for_string_arg: bool,

    static fn none(): PureGlobalFlags {
        return PureGlobalFlags{
            can_be_removed_if_unused: false,
            is_constant_foldable: false,
            safe_to_reorder_read: false,
            call_is_pure_for_string_arg: false,
        };
    }

    fn eql(other: PureGlobalFlags): bool {
        return _.can_be_removed_if_unused == other.can_be_removed_if_unused &&
            _.is_constant_foldable == other.is_constant_foldable &&
            _.safe_to_reorder_read == other.safe_to_reorder_read &&
            _.call_is_pure_for_string_arg == other.call_is_pure_for_string_arg;
    }
}

/// 查询全局标识符的纯净度分类（明确排除 `eval` 等特殊语义标识符）
pub fn lookupPureGlobalIdentifier(name: []const u8): PureGlobalKind {
    if (name.len == 0) return .unknown;

    // 可常量折叠的全局字面量
    if (bytesEql(name, "NaN")) return .nan_literal;
    if (bytesEql(name, "Infinity")) return .infinity_literal;
    if (bytesEql(name, "undefined")) return .strict_undefined;

    // 显式排除 eval（direct eval 具有词法作用域捕获特殊语义）
    if (bytesEql(name, "eval")) return .unknown;

    // 核心 ECMAScript 内建全局对象
    if (bytesEql(name, "Array") || bytesEql(name, "Boolean") || bytesEql(name, "Function") ||
        bytesEql(name, "Math") || bytesEql(name, "Number") || bytesEql(name, "Object") ||
        bytesEql(name, "RegExp") || bytesEql(name, "String") || bytesEql(name, "BigInt") ||
        bytesEql(name, "Symbol") || bytesEql(name, "Date") || bytesEql(name, "JSON") ||
        bytesEql(name, "Promise") || bytesEql(name, "Proxy") || bytesEql(name, "Reflect") ||
        bytesEql(name, "Map") || bytesEql(name, "Set") || bytesEql(name, "WeakMap") ||
        bytesEql(name, "WeakSet") || bytesEql(name, "WeakRef") || bytesEql(name, "Intl") ||
        bytesEql(name, "globalThis"))
    {
        return .pure_identifier;
    }

    // 二进制与 TypedArray 全局构造器
    if (bytesEql(name, "ArrayBuffer") || bytesEql(name, "SharedArrayBuffer") || bytesEql(name, "DataView") ||
        bytesEql(name, "Int8Array") || bytesEql(name, "Uint8Array") || bytesEql(name, "Uint8ClampedArray") ||
        bytesEql(name, "Int16Array") || bytesEql(name, "Uint16Array") ||
        bytesEql(name, "Int32Array") || bytesEql(name, "Uint32Array") ||
        bytesEql(name, "BigInt64Array") || bytesEql(name, "BigUint64Array") ||
        bytesEql(name, "Float16Array") || bytesEql(name, "Float32Array") || bytesEql(name, "Float64Array"))
    {
        return .pure_identifier;
    }

    // 标准 Error 家族
    if (bytesEql(name, "Error") || bytesEql(name, "AggregateError") || bytesEql(name, "EvalError") ||
        bytesEql(name, "RangeError") || bytesEql(name, "ReferenceError") ||
        bytesEql(name, "SyntaxError") || bytesEql(name, "TypeError") || bytesEql(name, "URIError"))
    {
        return .pure_identifier;
    }

    // 浏览器与服务端通用 Web/WinterCG 全局对象
    if (bytesEql(name, "AbortController") || bytesEql(name, "AbortSignal") ||
        bytesEql(name, "Event") || bytesEql(name, "EventTarget") ||
        bytesEql(name, "MessageChannel") || bytesEql(name, "MessageEvent") || bytesEql(name, "MessagePort") ||
        bytesEql(name, "TextDecoder") || bytesEql(name, "TextEncoder") ||
        bytesEql(name, "URL") || bytesEql(name, "URLSearchParams") ||
        bytesEql(name, "Blob") || bytesEql(name, "Headers") || bytesEql(name, "Request") ||
        bytesEql(name, "Response") || bytesEql(name, "FormData") || bytesEql(name, "ReadableStream") ||
        bytesEql(name, "WritableStream") || bytesEql(name, "TransformStream") ||
        bytesEql(name, "WebAssembly") || bytesEql(name, "WebSocket") ||
        bytesEql(name, "console") || bytesEql(name, "crypto") || bytesEql(name, "fetch") ||
        bytesEql(name, "isFinite") || bytesEql(name, "isNaN") || bytesEql(name, "parseFloat") ||
        bytesEql(name, "parseInt") || bytesEql(name, "decodeURI") || bytesEql(name, "decodeURIComponent") ||
        bytesEql(name, "encodeURI") || bytesEql(name, "encodeURIComponent") ||
        bytesEql(name, "atob") || bytesEql(name, "btoa") || bytesEql(name, "structuredClone") ||
        bytesEql(name, "queueMicrotask") || bytesEql(name, "setTimeout") || bytesEql(name, "clearTimeout") ||
        bytesEql(name, "setInterval") || bytesEql(name, "clearInterval"))
    {
        return .pure_identifier;
    }

    return .unknown;
}

/// 判断全局标识符是否读取无副作用（不会抛 `ReferenceError`）
pub fn isPureGlobalIdentifier(name: []const u8): bool {
    return lookupPureGlobalIdentifier(name) != .unknown;
}

/// 判断全局标识符是否可直接常量折叠（`NaN` / `Infinity` / `undefined`）
pub fn isConstantFoldableGlobal(name: []const u8): bool {
    const k = lookupPureGlobalIdentifier(name);
    return k == .nan_literal || k == .infinity_literal || k == .strict_undefined;
}

/// 判断 `root.prop` 静态属性读取是否无副作用（对应上游 `global_no_side_effect_property_accesses`）
/// 注意：此判定仅表示“读取该属性/方法引用本身无副作用”（允许未用消除与重排），不代表调用它无副作用。
pub fn isPureGlobalPropertyAccess(root: []const u8, prop: []const u8): bool {
    if (root.len == 0 || prop.len == 0) {
        return false;
    }
    if (bytesEql(root, "Array")) {
        return bytesEql(prop, "from") || bytesEql(prop, "isArray") || bytesEql(prop, "of") || bytesEql(prop, "fromAsync");
    }
    if (bytesEql(root, "JSON")) {
        return bytesEql(prop, "parse") || bytesEql(prop, "stringify");
    }
    if (bytesEql(root, "Math")) {
        return bytesEql(prop, "E") || bytesEql(prop, "LN10") || bytesEql(prop, "LN2") ||
            bytesEql(prop, "LOG10E") || bytesEql(prop, "LOG2E") || bytesEql(prop, "PI") ||
            bytesEql(prop, "SQRT1_2") || bytesEql(prop, "SQRT2") ||
            bytesEql(prop, "abs") || bytesEql(prop, "acos") || bytesEql(prop, "acosh") ||
            bytesEql(prop, "asin") || bytesEql(prop, "asinh") || bytesEql(prop, "atan") ||
            bytesEql(prop, "atan2") || bytesEql(prop, "atanh") || bytesEql(prop, "cbrt") ||
            bytesEql(prop, "ceil") || bytesEql(prop, "clz32") || bytesEql(prop, "cos") ||
            bytesEql(prop, "cosh") || bytesEql(prop, "exp") || bytesEql(prop, "expm1") ||
            bytesEql(prop, "floor") || bytesEql(prop, "fround") || bytesEql(prop, "hypot") ||
            bytesEql(prop, "imul") || bytesEql(prop, "log") || bytesEql(prop, "log10") ||
            bytesEql(prop, "log1p") || bytesEql(prop, "log2") || bytesEql(prop, "max") ||
            bytesEql(prop, "min") || bytesEql(prop, "pow") || bytesEql(prop, "random") ||
            bytesEql(prop, "round") || bytesEql(prop, "sign") || bytesEql(prop, "sin") ||
            bytesEql(prop, "sinh") || bytesEql(prop, "sqrt") || bytesEql(prop, "tan") ||
            bytesEql(prop, "tanh") || bytesEql(prop, "trunc");
    }
    if (bytesEql(root, "Number")) {
        return bytesEql(prop, "EPSILON") || bytesEql(prop, "MAX_SAFE_INTEGER") ||
            bytesEql(prop, "MAX_VALUE") || bytesEql(prop, "MIN_SAFE_INTEGER") ||
            bytesEql(prop, "MIN_VALUE") || bytesEql(prop, "NaN") ||
            bytesEql(prop, "NEGATIVE_INFINITY") || bytesEql(prop, "POSITIVE_INFINITY") ||
            bytesEql(prop, "isFinite") || bytesEql(prop, "isInteger") ||
            bytesEql(prop, "isNaN") || bytesEql(prop, "isSafeInteger") ||
            bytesEql(prop, "parseFloat") || bytesEql(prop, "parseInt");
    }
    if (bytesEql(root, "Object")) {
        return bytesEql(prop, "assign") || bytesEql(prop, "create") || bytesEql(prop, "defineProperties") ||
            bytesEql(prop, "defineProperty") || bytesEql(prop, "entries") || bytesEql(prop, "freeze") ||
            bytesEql(prop, "fromEntries") || bytesEql(prop, "getOwnPropertyDescriptor") ||
            bytesEql(prop, "getOwnPropertyDescriptors") || bytesEql(prop, "getOwnPropertyNames") ||
            bytesEql(prop, "getOwnPropertySymbols") || bytesEql(prop, "getPrototypeOf") ||
            bytesEql(prop, "hasOwn") || bytesEql(prop, "is") || bytesEql(prop, "isExtensible") ||
            bytesEql(prop, "isFrozen") || bytesEql(prop, "isSealed") || bytesEql(prop, "keys") ||
            bytesEql(prop, "preventExtensions") || bytesEql(prop, "seal") ||
            bytesEql(prop, "setPrototypeOf") || bytesEql(prop, "values") || bytesEql(prop, "prototype");
    }
    if (bytesEql(root, "Reflect")) {
        return bytesEql(prop, "apply") || bytesEql(prop, "construct") || bytesEql(prop, "defineProperty") ||
            bytesEql(prop, "deleteProperty") || bytesEql(prop, "get") ||
            bytesEql(prop, "getOwnPropertyDescriptor") || bytesEql(prop, "getPrototypeOf") ||
            bytesEql(prop, "has") || bytesEql(prop, "isExtensible") || bytesEql(prop, "ownKeys") ||
            bytesEql(prop, "preventExtensions") || bytesEql(prop, "set") || bytesEql(prop, "setPrototypeOf");
    }
    if (bytesEql(root, "String")) {
        return bytesEql(prop, "fromCharCode") || bytesEql(prop, "fromCodePoint") || bytesEql(prop, "raw");
    }
    if (bytesEql(root, "Symbol")) {
        return bytesEql(prop, "asyncIterator") || bytesEql(prop, "for") || bytesEql(prop, "hasInstance") ||
            bytesEql(prop, "isConcatSpreadable") || bytesEql(prop, "iterator") || bytesEql(prop, "keyFor") ||
            bytesEql(prop, "match") || bytesEql(prop, "matchAll") || bytesEql(prop, "replace") ||
            bytesEql(prop, "search") || bytesEql(prop, "species") || bytesEql(prop, "split") ||
            bytesEql(prop, "toPrimitive") || bytesEql(prop, "toStringTag") || bytesEql(prop, "unscopables");
    }
    if (bytesEql(root, "Promise")) {
        return bytesEql(prop, "all") || bytesEql(prop, "allSettled") || bytesEql(prop, "any") ||
            bytesEql(prop, "race") || bytesEql(prop, "reject") || bytesEql(prop, "resolve") ||
            bytesEql(prop, "withResolvers");
    }
    if (bytesEql(root, "Date")) {
        return bytesEql(prop, "now") || bytesEql(prop, "parse") || bytesEql(prop, "UTC");
    }
    if (bytesEql(root, "console")) {
        return bytesEql(prop, "log") || bytesEql(prop, "warn") || bytesEql(prop, "error") ||
            bytesEql(prop, "info") || bytesEql(prop, "debug") || bytesEql(prop, "trace") ||
            bytesEql(prop, "assert") || bytesEql(prop, "time") || bytesEql(prop, "timeEnd");
    }
    return false;
}

/// 判断点分属性访问链是否无副作用（支持 2 段如 `Math.PI` 与 3 段如 `Object.prototype.hasOwnProperty`）
pub fn isPureGlobalPropertyChain(chain_raw: []const u8): bool {
    const chain = trimWs(chain_raw);
    if (chain.len == 0) {
        return false;
    }

    // 拆分点分链（最多 3 段）
    let dot1: usize = chain.len;
    let dot2: usize = chain.len;
    let i: usize = 0;
    while (i < chain.len) {
        if (chain[i] == '.') {
            if (dot1 == chain.len) {
                dot1 = i;
            } else if (dot2 == chain.len) {
                dot2 = i;
            } else {
                // 超过 3 段不纳入保守白名单
                return false;
            }
        }
        i += 1;
    }

    if (dot1 == chain.len) {
        return isPureGlobalIdentifier(chain);
    }

    const seg1 = chain[0..dot1];
    if (dot2 == chain.len) {
        const seg2 = chain[dot1 + 1..];
        return isPureGlobalPropertyAccess(seg1, seg2);
    }

    const seg2 = chain[dot1 + 1..dot2];
    const seg3 = chain[dot2 + 1..];
    if (bytesEql(seg1, "Object") && bytesEql(seg2, "prototype")) {
        return bytesEql(seg3, "hasOwnProperty") || bytesEql(seg3, "isPrototypeOf") ||
            bytesEql(seg3, "propertyIsEnumerable") || bytesEql(seg3, "toString") || bytesEql(seg3, "valueOf");
    }
    if (bytesEql(seg1, "Array") && bytesEql(seg2, "prototype")) {
        return bytesEql(seg3, "slice") || bytesEql(seg3, "concat") || bytesEql(seg3, "map") ||
            bytesEql(seg3, "filter") || bytesEql(seg3, "forEach") || bytesEql(seg3, "join");
    }
    if (bytesEql(seg1, "Function") && bytesEql(seg2, "prototype")) {
        return bytesEql(seg3, "apply") || bytesEql(seg3, "bind") || bytesEql(seg3, "call");
    }
    return false;
}

/// 判断 `root.prop("...")` 函数调用在参数为字符串字面量时是否保证无副作用且不抛异常
/// （对应上游 `global_no_side_effect_function_calls_safe_for_to_string`，如 React 常用 `Symbol.for`）
pub fn isPureGlobalCallSafeForString(root: []const u8, prop: []const u8): bool {
    return bytesEql(root, "Symbol") && bytesEql(prop, "for");
}

/// 综合分析全局标识符或属性链表达式，返回 Bundler DCE/常量折叠/重排标志位
pub fn classifyGlobalExpr(expr_raw: []const u8): PureGlobalFlags {
    const expr = trimWs(expr_raw);
    if (expr.len == 0) {
        return PureGlobalFlags.none();
    }

    // 检查是否含 '.'
    let dot_pos: usize = expr.len;
    let i: usize = 0;
    while (i < expr.len) {
        if (expr[i] == '.') {
            dot_pos = i;
            break;
        }
        i += 1;
    }

    if (dot_pos == expr.len) {
        const k = lookupPureGlobalIdentifier(expr);
        if (k == .unknown) {
            return PureGlobalFlags.none();
        }
        const foldable = k == .nan_literal || k == .infinity_literal || k == .strict_undefined;
        return PureGlobalFlags{
            can_be_removed_if_unused: true,
            is_constant_foldable: foldable,
            safe_to_reorder_read: true,
            call_is_pure_for_string_arg: false,
        };
    }

    if (!isPureGlobalPropertyChain(expr)) {
        return PureGlobalFlags.none();
    }

    const root = expr[0..dot_pos];
    const rest = expr[dot_pos + 1..];
    const call_pure = isPureGlobalCallSafeForString(root, rest);
    return PureGlobalFlags{
        can_be_removed_if_unused: true,
        is_constant_foldable: false,
        safe_to_reorder_read: true,
        call_is_pure_for_string_arg: call_pure,
    };
}
