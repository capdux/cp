// Source: Absorbed from /data/dev/bun/src/js_parser/ast/Op.cp | Synced: 2026-11-21T17:06:09Z
// Complete JS/TS AST operator codes, precedence levels, associativity, assignment targets, and text metadata.

pub enum AssignTarget: u8 {
    none = 0,
    replace = 1,
    update = 2,
}

pub enum Code: u8 {
    // Prefix
    un_pos = 0,
    un_neg = 1,
    un_cpl = 2,
    un_not = 3,
    un_void = 4,
    un_typeof = 5,
    un_delete = 6,

    // Prefix update
    un_pre_dec = 7,
    un_pre_inc = 8,

    // Postfix update
    un_post_dec = 9,
    un_post_inc = 10,

    // Left-associative binary
    bin_add = 11,
    bin_sub = 12,
    bin_mul = 13,
    bin_div = 14,
    bin_rem = 15,
    bin_pow = 16,
    bin_lt = 17,
    bin_le = 18,
    bin_gt = 19,
    bin_ge = 20,
    bin_in = 21,
    bin_instanceof = 22,
    bin_shl = 23,
    bin_shr = 24,
    bin_u_shr = 25,
    bin_loose_eq = 26,
    bin_loose_ne = 27,
    bin_strict_eq = 28,
    bin_strict_ne = 29,
    bin_nullish_coalescing = 30,
    bin_logical_or = 31,
    bin_logical_and = 32,
    bin_bitwise_or = 33,
    bin_bitwise_and = 34,
    bin_bitwise_xor = 35,

    // Non-associative
    bin_comma = 36,

    // Right-associative assignments
    bin_assign = 37,
    bin_add_assign = 38,
    bin_sub_assign = 39,
    bin_mul_assign = 40,
    bin_div_assign = 41,
    bin_rem_assign = 42,
    bin_pow_assign = 43,
    bin_shl_assign = 44,
    bin_shr_assign = 45,
    bin_u_shr_assign = 46,
    bin_bitwise_or_assign = 47,
    bin_bitwise_and_assign = 48,
    bin_bitwise_xor_assign = 49,
    bin_nullish_coalescing_assign = 50,
    bin_logical_or_assign = 51,
    bin_logical_and_assign = 52,

    fn unaryAssignTarget(): AssignTarget {
        const v = @intFromEnum(_);
        if (v >= @intFromEnum(Code.un_pre_dec) && v <= @intFromEnum(Code.un_post_inc)) {
            return .update;
        }
        return .none;
    }

    fn isLeftAssociative(): bool {
        const v = @intFromEnum(_);
        return v >= @intFromEnum(Code.bin_add) && v < @intFromEnum(Code.bin_comma) && _ != .bin_pow;
    }

    fn isRightAssociative(): bool {
        return @intFromEnum(_) >= @intFromEnum(Code.bin_assign) || _ == .bin_pow;
    }

    fn binaryAssignTarget(): AssignTarget {
        if (_ == .bin_assign) return .replace;
        if (@intFromEnum(_) > @intFromEnum(Code.bin_assign)) return .update;
        return .none;
    }

    fn isPrefix(): bool {
        return @intFromEnum(_) < @intFromEnum(Code.un_post_dec);
    }

    fn isPostfix(): bool {
        return _ == .un_post_dec || _ == .un_post_inc;
    }

    fn isUnary(): bool {
        return @intFromEnum(_) <= @intFromEnum(Code.un_post_inc);
    }

    fn isBinary(): bool {
        return @intFromEnum(_) >= @intFromEnum(Code.bin_add);
    }

    fn info(): OpInfo {
        return opInfo(_);
    }
}

pub enum Level: u8 {
    lowest = 0,
    comma = 1,
    spread = 2,
    yield_level = 3,
    assign = 4,
    conditional = 5,
    nullish_coalescing = 6,
    logical_or = 7,
    logical_and = 8,
    bitwise_or = 9,
    bitwise_xor = 10,
    bitwise_and = 11,
    equals = 12,
    compare = 13,
    shift = 14,
    add = 15,
    multiply = 16,
    exponentiation = 17,
    prefix = 18,
    postfix = 19,
    new_level = 20,
    call = 21,
    member = 22,

    fn lt(b: Level): bool {
        return @intFromEnum(_) < @intFromEnum(b);
    }

    fn gt(b: Level): bool {
        return @intFromEnum(_) > @intFromEnum(b);
    }

    fn lte(b: Level): bool {
        return @intFromEnum(_) <= @intFromEnum(b);
    }

    fn gte(b: Level): bool {
        return @intFromEnum(_) >= @intFromEnum(b);
    }

    fn eql(b: Level): bool {
        return @intFromEnum(_) == @intFromEnum(b);
    }
}

pub struct OpInfo {
    text: string,
    level: Level,
    is_keyword: bool = false,
}

pub fn opInfo(code: Code): OpInfo {
    return switch (code) {
        .un_pos => OpInfo{ text: "+", level: .prefix, is_keyword: false },
        .un_neg => OpInfo{ text: "-", level: .prefix, is_keyword: false },
        .un_cpl => OpInfo{ text: "~", level: .prefix, is_keyword: false },
        .un_not => OpInfo{ text: "!", level: .prefix, is_keyword: false },
        .un_void => OpInfo{ text: "void", level: .prefix, is_keyword: true },
        .un_typeof => OpInfo{ text: "typeof", level: .prefix, is_keyword: true },
        .un_delete => OpInfo{ text: "delete", level: .prefix, is_keyword: true },
        .un_pre_dec => OpInfo{ text: "--", level: .prefix, is_keyword: false },
        .un_pre_inc => OpInfo{ text: "++", level: .prefix, is_keyword: false },
        .un_post_dec => OpInfo{ text: "--", level: .postfix, is_keyword: false },
        .un_post_inc => OpInfo{ text: "++", level: .postfix, is_keyword: false },

        .bin_add => OpInfo{ text: "+", level: .add, is_keyword: false },
        .bin_sub => OpInfo{ text: "-", level: .add, is_keyword: false },
        .bin_mul => OpInfo{ text: "*", level: .multiply, is_keyword: false },
        .bin_div => OpInfo{ text: "/", level: .multiply, is_keyword: false },
        .bin_rem => OpInfo{ text: "%", level: .multiply, is_keyword: false },
        .bin_pow => OpInfo{ text: "**", level: .exponentiation, is_keyword: false },
        .bin_lt => OpInfo{ text: "<", level: .compare, is_keyword: false },
        .bin_le => OpInfo{ text: "<=", level: .compare, is_keyword: false },
        .bin_gt => OpInfo{ text: ">", level: .compare, is_keyword: false },
        .bin_ge => OpInfo{ text: ">=", level: .compare, is_keyword: false },
        .bin_in => OpInfo{ text: "in", level: .compare, is_keyword: true },
        .bin_instanceof => OpInfo{ text: "instanceof", level: .compare, is_keyword: true },
        .bin_shl => OpInfo{ text: "<<", level: .shift, is_keyword: false },
        .bin_shr => OpInfo{ text: ">>", level: .shift, is_keyword: false },
        .bin_u_shr => OpInfo{ text: ">>>", level: .shift, is_keyword: false },
        .bin_loose_eq => OpInfo{ text: "==", level: .equals, is_keyword: false },
        .bin_loose_ne => OpInfo{ text: "!=", level: .equals, is_keyword: false },
        .bin_strict_eq => OpInfo{ text: "===", level: .equals, is_keyword: false },
        .bin_strict_ne => OpInfo{ text: "!==", level: .equals, is_keyword: false },
        .bin_nullish_coalescing => OpInfo{ text: "??", level: .nullish_coalescing, is_keyword: false },
        .bin_logical_or => OpInfo{ text: "||", level: .logical_or, is_keyword: false },
        .bin_logical_and => OpInfo{ text: "&&", level: .logical_and, is_keyword: false },
        .bin_bitwise_or => OpInfo{ text: "|", level: .bitwise_or, is_keyword: false },
        .bin_bitwise_and => OpInfo{ text: "&", level: .bitwise_and, is_keyword: false },
        .bin_bitwise_xor => OpInfo{ text: "^", level: .bitwise_xor, is_keyword: false },

        .bin_comma => OpInfo{ text: ",", level: .comma, is_keyword: false },

        .bin_assign => OpInfo{ text: "=", level: .assign, is_keyword: false },
        .bin_add_assign => OpInfo{ text: "+=", level: .assign, is_keyword: false },
        .bin_sub_assign => OpInfo{ text: "-=", level: .assign, is_keyword: false },
        .bin_mul_assign => OpInfo{ text: "*=", level: .assign, is_keyword: false },
        .bin_div_assign => OpInfo{ text: "/=", level: .assign, is_keyword: false },
        .bin_rem_assign => OpInfo{ text: "%=", level: .assign, is_keyword: false },
        .bin_pow_assign => OpInfo{ text: "**=", level: .assign, is_keyword: false },
        .bin_shl_assign => OpInfo{ text: "<<=", level: .assign, is_keyword: false },
        .bin_shr_assign => OpInfo{ text: ">>=", level: .assign, is_keyword: false },
        .bin_u_shr_assign => OpInfo{ text: ">>>=", level: .assign, is_keyword: false },
        .bin_bitwise_or_assign => OpInfo{ text: "|=", level: .assign, is_keyword: false },
        .bin_bitwise_and_assign => OpInfo{ text: "&=", level: .assign, is_keyword: false },
        .bin_bitwise_xor_assign => OpInfo{ text: "^=", level: .assign, is_keyword: false },
        .bin_nullish_coalescing_assign => OpInfo{ text: "??=", level: .assign, is_keyword: false },
        .bin_logical_or_assign => OpInfo{ text: "||=", level: .assign, is_keyword: false },
        .bin_logical_and_assign => OpInfo{ text: "&&=", level: .assign, is_keyword: false },
    };
}
