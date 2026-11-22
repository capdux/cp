import { test, expect } from "cap:test";
import {
    PureGlobalFlags,
    lookupPureGlobalIdentifier,
    isPureGlobalIdentifier,
    isConstantFoldableGlobal,
    isPureGlobalPropertyAccess,
    isPureGlobalPropertyChain,
    isPureGlobalCallSafeForString,
    classifyGlobalExpr,
} from "cap:bundler";

test("bundler pure_globals: constant-foldable and built-in global identifier classification") {
    expect(lookupPureGlobalIdentifier("NaN") == .nan_literal);
    expect(lookupPureGlobalIdentifier("Infinity") == .infinity_literal);
    expect(lookupPureGlobalIdentifier("undefined") == .strict_undefined);
    expect(isConstantFoldableGlobal("NaN"));
    expect(isConstantFoldableGlobal("Infinity"));
    expect(isConstantFoldableGlobal("undefined"));

    expect(lookupPureGlobalIdentifier("Array") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("Uint8Array") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("Float16Array") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("AggregateError") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("AbortController") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("TextEncoder") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("URLSearchParams") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("WebSocket") == .pure_identifier);
    expect(lookupPureGlobalIdentifier("structuredClone") == .pure_identifier);
    expect(!isConstantFoldableGlobal("Math"));
}

test("bundler pure_globals: eval and unknown globals strictly rejected") {
    // direct eval 绝不可视作纯全局标识符
    expect(lookupPureGlobalIdentifier("eval") == .unknown);
    expect(!isPureGlobalIdentifier("eval"));
    expect(classifyGlobalExpr("eval").eql(PureGlobalFlags.none()));

    // 未知全局标识符可能抛 ReferenceError 或触发 getter
    expect(!isPureGlobalIdentifier("window"));
    expect(!isPureGlobalIdentifier("process"));
    expect(!isPureGlobalIdentifier("customGlobal"));
    expect(!isPureGlobalIdentifier(""));
    expect(classifyGlobalExpr("customGlobal").eql(PureGlobalFlags.none()));
}

test("bundler pure_globals: 2-segment static property accesses across built-in namespaces") {
    expect(isPureGlobalPropertyAccess("Array", "isArray"));
    expect(isPureGlobalPropertyAccess("JSON", "stringify"));
    expect(isPureGlobalPropertyAccess("Math", "PI"));
    expect(isPureGlobalPropertyAccess("Math", "hypot"));
    expect(isPureGlobalPropertyAccess("Number", "MAX_SAFE_INTEGER"));
    expect(isPureGlobalPropertyAccess("Object", "defineProperty"));
    expect(isPureGlobalPropertyAccess("Object", "hasOwn"));
    expect(isPureGlobalPropertyAccess("Reflect", "construct"));
    expect(isPureGlobalPropertyAccess("String", "fromCodePoint"));
    expect(isPureGlobalPropertyAccess("Symbol", "iterator"));
    expect(isPureGlobalPropertyAccess("Promise", "withResolvers"));
    expect(isPureGlobalPropertyAccess("Date", "now"));
    expect(isPureGlobalPropertyAccess("console", "log"));

    // 未知属性或未知命名空间拒绝
    expect(!isPureGlobalPropertyAccess("Math", "nonExistent"));
    expect(!isPureGlobalPropertyAccess("console", "explode"));
    expect(!isPureGlobalPropertyAccess("Unknown", "log"));
    expect(!isPureGlobalPropertyAccess("", "log"));
    expect(!isPureGlobalPropertyAccess("Math", ""));
}

test("bundler pure_globals: 3-segment prototype chains and over-deep chain rejection") {
    expect(isPureGlobalPropertyChain("Object.prototype.hasOwnProperty"));
    expect(isPureGlobalPropertyChain("Object.prototype.toString"));
    expect(isPureGlobalPropertyChain("Array.prototype.slice"));
    expect(isPureGlobalPropertyChain("Array.prototype.map"));
    expect(isPureGlobalPropertyChain("Function.prototype.call"));
    expect(isPureGlobalPropertyChain("Function.prototype.bind"));

    // 未知原型方法或超过 3 段链拒绝
    expect(!isPureGlobalPropertyChain("Object.prototype.unknownMethod"));
    expect(!isPureGlobalPropertyChain("Array.prototype.push"));
    expect(!isPureGlobalPropertyChain("Object.prototype.hasOwnProperty.call"));
}

test("bundler pure_globals: Symbol.for pure string call and classifyGlobalExpr matrix") {
    expect(isPureGlobalCallSafeForString("Symbol", "for"));
    expect(!isPureGlobalCallSafeForString("Symbol", "keyFor"));
    expect(!isPureGlobalCallSafeForString("JSON", "parse"));

    const f_sym = classifyGlobalExpr("Symbol.for");
    expect(f_sym.can_be_removed_if_unused);
    expect(!f_sym.is_constant_foldable);
    expect(f_sym.safe_to_reorder_read);
    expect(f_sym.call_is_pure_for_string_arg);

    const f_undef = classifyGlobalExpr("undefined");
    expect(f_undef.can_be_removed_if_unused);
    expect(f_undef.is_constant_foldable);
    expect(f_undef.safe_to_reorder_read);
    expect(!f_undef.call_is_pure_for_string_arg);

    const f_log = classifyGlobalExpr("console.log");
    expect(f_log.can_be_removed_if_unused);
    expect(f_log.safe_to_reorder_read);
    expect(!f_log.call_is_pure_for_string_arg);
}

test("bundler pure_globals: whitespace trimming, case sensitivity and malformed chains") {
    // 前后空白容错
    expect(isPureGlobalPropertyChain("  Math.PI  "));
    expect(classifyGlobalExpr("\tSymbol.for\n").call_is_pure_for_string_arg);

    // 大小写严格区分
    expect(!isPureGlobalIdentifier("math"));
    expect(!isPureGlobalIdentifier("nan"));
    expect(!isPureGlobalPropertyChain("math.PI"));
    expect(!isPureGlobalPropertyChain("Math.pi"));

    // 畸形链拒绝
    expect(!isPureGlobalPropertyChain(""));
    expect(!isPureGlobalPropertyChain("   "));
    expect(!isPureGlobalPropertyChain(".PI"));
    expect(!isPureGlobalPropertyChain("Math."));
    expect(!isPureGlobalPropertyChain("Math..PI"));
    expect(classifyGlobalExpr("Math..PI").eql(PureGlobalFlags.none()));
}
