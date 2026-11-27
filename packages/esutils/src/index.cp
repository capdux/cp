// @cap/esutils — Zero-allocation ECMAScript code/keyword/identifier utilities in pure CP
// Clean-room MIT implementation of npm `esutils` (#201, 215M/wk).

fn esuStrEq(a: string, b: string): bool {
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

/// Returns `true` if `cp` is an ECMAScript decimal digit (`0`-`9`).
pub fn isDecimalDigit(cp: u32): bool {
    return cp >= 0x30 && cp <= 0x39;
}

/// Returns `true` if `cp` is an ECMAScript hex digit (`0-9`, `a-f`, `A-F`).
pub fn isHexDigit(cp: u32): bool {
    return (cp >= 0x30 && cp <= 0x39)
        || (cp >= 0x41 && cp <= 0x46)
        || (cp >= 0x61 && cp <= 0x66);
}

/// Returns `true` if `cp` is an ECMAScript octal digit (`0`-`7`).
pub fn isOctalDigit(cp: u32): bool {
    return cp >= 0x30 && cp <= 0x37;
}

/// Returns `true` if `cp` is an ECMAScript WhiteSpace character (ES2024 §12.2).
pub fn isWhiteSpace(cp: u32): bool {
    if (cp == 0x20 || cp == 0x09 || cp == 0x0B || cp == 0x0C || cp == 0xA0 || cp == 0xFEFF) {
        return true;
    }
    if (cp == 0x1680 || (cp >= 0x2000 && cp <= 0x200A) || cp == 0x202F || cp == 0x205F || cp == 0x3000) {
        return true;
    }
    return false;
}

/// Returns `true` if `cp` is an ECMAScript LineTerminator (`\n`, `\r`, U+2028, U+2029).
pub fn isLineTerminator(cp: u32): bool {
    return cp == 0x0A || cp == 0x0D || cp == 0x2028 || cp == 0x2029;
}

/// Returns `true` if `cp` can start an ES6 IdentifierName (`$`, `_`, ASCII letters, non-ASCII letters).
pub fn isIdentifierStartES6(cp: u32): bool {
    if (cp < 0x80) {
        return (cp >= 0x41 && cp <= 0x5A) || (cp >= 0x61 && cp <= 0x7A) || cp == 0x24 || cp == 0x5F;
    }
    if ((cp >= 0x00C0 && cp <= 0x02AF && cp != 0x00D7 && cp != 0x00F7)
        || (cp >= 0x0370 && cp <= 0x052F)
        || (cp >= 0x3040 && cp <= 0x30FF)
        || (cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0xAC00 && cp <= 0xD7A3)) {
        return true;
    }
    return false;
}

/// Returns `true` if `cp` can continue an ES6 IdentifierName.
pub fn isIdentifierPartES6(cp: u32): bool {
    if (isIdentifierStartES6(cp)) {
        return true;
    }
    if (isDecimalDigit(cp)) {
        return true;
    }
    return cp == 0x200C || cp == 0x200D; // ZWNJ / ZWJ
}

/// Returns `true` if `id` is a strict-mode ES6 keyword.
pub fn isKeywordES6(id: string, strict: bool): bool {
    if (strict) {
        if (esuStrEq(id, "let") || esuStrEq(id, "static") || esuStrEq(id, "yield")
            || esuStrEq(id, "implements") || esuStrEq(id, "interface") || esuStrEq(id, "package")
            || esuStrEq(id, "private") || esuStrEq(id, "protected") || esuStrEq(id, "public")) {
            return true;
        }
    }
    if (esuStrEq(id, "if") || esuStrEq(id, "in") || esuStrEq(id, "do") || esuStrEq(id, "var")
        || esuStrEq(id, "for") || esuStrEq(id, "new") || esuStrEq(id, "try") || esuStrEq(id, "this")
        || esuStrEq(id, "else") || esuStrEq(id, "case") || esuStrEq(id, "void") || esuStrEq(id, "with")
        || esuStrEq(id, "enum") || esuStrEq(id, "while") || esuStrEq(id, "break") || esuStrEq(id, "catch")
        || esuStrEq(id, "throw") || esuStrEq(id, "const") || esuStrEq(id, "class") || esuStrEq(id, "super")
        || esuStrEq(id, "return") || esuStrEq(id, "typeof") || esuStrEq(id, "delete") || esuStrEq(id, "switch")
        || esuStrEq(id, "export") || esuStrEq(id, "import") || esuStrEq(id, "default") || esuStrEq(id, "finally")
        || esuStrEq(id, "extends") || esuStrEq(id, "function") || esuStrEq(id, "continue") || esuStrEq(id, "debugger")
        || esuStrEq(id, "instanceof") || esuStrEq(id, "await")) {
        return true;
    }
    return false;
}

/// Returns `true` if `id` is a reserved word in ES6 (keywords + `null`, `true`, `false`).
pub fn isReservedWordES6(id: string, strict: bool): bool {
    if (esuStrEq(id, "null") || esuStrEq(id, "true") || esuStrEq(id, "false")) {
        return true;
    }
    return isKeywordES6(id, strict);
}

/// Returns `true` if `id` is a restricted word in strict mode (`eval` or `arguments`).
pub fn isRestrictedWord(id: string): bool {
    return esuStrEq(id, "eval") || esuStrEq(id, "arguments");
}

/// Returns `true` if `id` is a valid ES6 IdentifierName (allows keywords, e.g. property names).
pub fn isIdentifierNameES6(id: string): bool {
    if (id.len == 0) {
        return false;
    }
    let i: usize = 0;
    let first = true;
    while (i < id.len) {
        const b0 = id[i];
        let cp: u32 = 0;
        let step: usize = 1;
        if (b0 < 0x80) {
            cp = b0 as u32;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < id.len) {
            cp = (((b0 & 0x1F) as u32) << 6) | ((id[i + 1] & 0x3F) as u32);
            step = 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < id.len) {
            cp = (((b0 & 0x0F) as u32) << 12) | (((id[i + 1] & 0x3F) as u32) << 6) | ((id[i + 2] & 0x3F) as u32);
            step = 3;
        } else {
            return false;
        }
        if (first) {
            if (!isIdentifierStartES6(cp)) { return false; }
            first = false;
        } else {
            if (!isIdentifierPartES6(cp)) { return false; }
        }
        i += step;
    }
    return true;
}

/// Returns `true` if `id` is a valid ES6 Identifier (valid IdentifierName and not reserved).
pub fn isIdentifierES6(id: string, strict: bool): bool {
    if (!isIdentifierNameES6(id)) {
        return false;
    }
    return !isReservedWordES6(id, strict);
}

export {
    isDecimalDigit,
    isHexDigit,
    isOctalDigit,
    isWhiteSpace,
    isLineTerminator,
    isIdentifierStartES6,
    isIdentifierPartES6,
    isKeywordES6,
    isReservedWordES6,
    isRestrictedWord,
    isIdentifierNameES6,
    isIdentifierES6,
};
