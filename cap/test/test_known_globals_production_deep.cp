import { test, expect_eq } from "cap:test";
import {
    GlobalNamespace,
    lookupGlobal,
    isKnownGlobal,
    isPureZeroArgConstructor,
    canDropNewKeyword,
    isPurePropertyAccess,
    isPureGlobalCall,
} from "cap:known_globals";

test("1. Core ECMAScript globals recognition & namespace classification", () => {
    const core_es: [10]string = [
        "Math", "Object", "Array", "Symbol", "Promise",
        "Reflect", "JSON", "Number", "String", "Error",
    ];
    for (name of core_es) {
        const info = lookupGlobal(name).?;
        expect_eq(info.isEcmaScript(), true);
        expect_eq(info.isWebApi(), false);
        expect_eq(isKnownGlobal(name), true);
    }

    expect_eq(lookupGlobal("Map").?.namespace == GlobalNamespace.ecmascript, true);
    expect_eq(lookupGlobal("WeakSet").?.isEcmaScript(), true);
});

test("2. Pure member property access detection for Tree-Shaking", () => {
    expect_eq(isPurePropertyAccess("Math", "PI"), true);
    expect_eq(isPurePropertyAccess("Math", "floor"), true);
    expect_eq(isPurePropertyAccess("Symbol", "iterator"), true);
    expect_eq(isPurePropertyAccess("Symbol", "toStringTag"), true);
    expect_eq(isPurePropertyAccess("Object", "keys"), true);
    expect_eq(isPurePropertyAccess("Object", "freeze"), true);
    expect_eq(isPurePropertyAccess("Array", "isArray"), true);
    expect_eq(isPurePropertyAccess("Number", "MAX_SAFE_INTEGER"), true);
    expect_eq(isPurePropertyAccess("JSON", "stringify"), true);
    expect_eq(isPurePropertyAccess("Promise", "all"), true);
    expect_eq(isPurePropertyAccess("Reflect", "ownKeys"), true);
    expect_eq(isPurePropertyAccess("Date", "now"), true);

    // Unknown or non-pure property access returns false
    expect_eq(isPurePropertyAccess("Math", "notAProp"), false);
    expect_eq(isPurePropertyAccess("Object", "customGetter"), false);
    expect_eq(isPurePropertyAccess("MyClass", "PI"), false);
});

test("3. Web API, Node, Bun runtime globals vs unknown user identifiers", () => {
    expect_eq(lookupGlobal("Headers").?.isWebApi(), true);
    expect_eq(lookupGlobal("Response").?.isWebApi(), true);
    expect_eq(lookupGlobal("TextEncoder").?.isWebApi(), true);
    expect_eq(lookupGlobal("TextDecoder").?.isWebApi(), true);
    expect_eq(lookupGlobal("URL").?.isWebApi(), true);

    expect_eq(lookupGlobal("Buffer").?.isNodeRuntime(), true);
    expect_eq(lookupGlobal("process").?.isNodeRuntime(), true);
    expect_eq(lookupGlobal("Bun").?.isBunRuntime(), true);

    // Unknown user identifiers
    expect_eq(lookupGlobal("MyService") == null, true);
    expect_eq(isKnownGlobal("fooBar"), false);
    expect_eq(isKnownGlobal("__webpack_require__"), false);
});

test("4. Pure zero-arg constructor, drop-new optimization & pure static calls", () => {
    // Error & Object/Array/Function can drop `new`; RegExp cannot!
    expect_eq(canDropNewKeyword("Error"), true);
    expect_eq(canDropNewKeyword("TypeError"), true);
    expect_eq(canDropNewKeyword("Object"), true);
    expect_eq(canDropNewKeyword("Array"), true);
    expect_eq(canDropNewKeyword("Function"), true);
    expect_eq(canDropNewKeyword("RegExp"), false);
    expect_eq(canDropNewKeyword("Map"), false);

    // Pure zero-arg constructors (absorbed from Bun KnownGlobal.cp)
    expect_eq(isPureZeroArgConstructor("Set"), true);
    expect_eq(isPureZeroArgConstructor("Map"), true);
    expect_eq(isPureZeroArgConstructor("WeakSet"), true);
    expect_eq(isPureZeroArgConstructor("WeakMap"), true);
    expect_eq(isPureZeroArgConstructor("Date"), true);
    expect_eq(isPureZeroArgConstructor("Headers"), true);
    expect_eq(isPureZeroArgConstructor("Response"), true);
    expect_eq(isPureZeroArgConstructor("TextEncoder"), true);
    expect_eq(isPureZeroArgConstructor("TextDecoder"), true);
    expect_eq(isPureZeroArgConstructor("Function"), false);
    expect_eq(isPureZeroArgConstructor("UnknownCtor"), false);

    // Pure static calls
    expect_eq(isPureGlobalCall("Math", "sqrt"), true);
    expect_eq(isPureGlobalCall("Number", "isFinite"), true);
    expect_eq(isPureGlobalCall("Array", "isArray"), true);
    expect_eq(isPureGlobalCall("Object", "is"), true);
    expect_eq(isPureGlobalCall("String", "fromCodePoint"), true);
    expect_eq(isPureGlobalCall("JSON", "parse"), false); // can throw SyntaxError
    expect_eq(isPureGlobalCall("Object", "assign"), false); // mutates target
});

test("5. Empty strings & strict case-sensitivity boundary checks", () => {
    expect_eq(lookupGlobal("") == null, true);
    expect_eq(isKnownGlobal(""), false);
    expect_eq(isPurePropertyAccess("", "PI"), false);
    expect_eq(isPurePropertyAccess("Math", ""), false);
    expect_eq(isPureGlobalCall("", "abs"), false);

    // Strict case sensitivity (JS identifiers are case-sensitive)
    expect_eq(isKnownGlobal("math"), false);
    expect_eq(isKnownGlobal("OBJECT"), false);
    expect_eq(isKnownGlobal("bun"), false);
    expect_eq(isPurePropertyAccess("Math", "pi"), false);
    expect_eq(isPureGlobalCall("Math", "FLOOR"), false);
});
