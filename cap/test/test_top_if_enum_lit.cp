pub enum PT {
    u8,
    u16,
    i32,
    bool,
    string,
    void,
    f32,
}

pub const k_true: PT = if (true) .u16 else .u8;
pub const k_false: PT = if (false) .u16 else .u8;
pub const k_kw: PT = if (true) .f32 else .string;

fn pick_pt(flag: bool): PT {
    return if (flag) .i32 else .bool;
}

fn pt_code(v: PT): i32 {
    return switch (v) {
        .u8 => 8,
        .u16 => 16,
        .i32 => 32,
        .bool => 1,
        .string => 2,
        .void => 0,
        .f32 => 99,
    };
}

fn main() {
    if (pt_code(k_true) != 16) {
        @panic("k_true should be .u16");
    }
    if (pt_code(k_false) != 8) {
        @panic("k_false should be .u8");
    }
    if (pt_code(k_kw) != 99) {
        @panic("k_kw should be .f32");
    }
    if (pt_code(pick_pt(true)) != 32) {
        @panic("pick_pt(true) should be .i32");
    }
    if (pt_code(pick_pt(false)) != 1) {
        @panic("pick_pt(false) should be .bool");
    }
    const v: PT = .void;
    if (pt_code(v) != 0) {
        @panic("v should be .void");
    }
}
