// Source: cp/lib/safetensors/dtype.cp (Safetensors data types and error definitions)
const std = @import("std");

pub const SafetensorsError = error{
    InvalidHeaderLength,
    HeaderTooLarge,
    InvalidJson,
    InvalidDtype,
    InvalidShape,
    InvalidOffsets,
    OutOfBounds,
    ShapeSizeMismatch,
    OverlappingTensors,
    TensorNotFound,
    UnsupportedConversion,
    DuplicateTensorName,
    OutOfMemory,
};

pub enum Dtype {
    F32,
    F64,
    F16,
    BF16,
    I8,
    U8,
    I16,
    U16,
    I32,
    U32,
    I64,
    U64,
    BOOL,
}

pub fn dtypeSize(dt: Dtype): usize {
    return switch (dt) {
        .BOOL, .I8, .U8 => 1,
        .F16, .BF16, .I16, .U16 => 2,
        .F32, .I32, .U32 => 4,
        .F64, .I64, .U64 => 8,
    };
}

pub fn dtypeName(dt: Dtype): []const u8 {
    return switch (dt) {
        .F32 => "F32",
        .F64 => "F64",
        .F16 => "F16",
        .BF16 => "BF16",
        .I8 => "I8",
        .U8 => "U8",
        .I16 => "I16",
        .U16 => "U16",
        .I32 => "I32",
        .U32 => "U32",
        .I64 => "I64",
        .U64 => "U64",
        .BOOL => "BOOL",
    };
}

pub fn parseDtype(name: []const u8): Dtype? {
    if (std.mem.eql(u8, name, "F32")) { return .F32; }
    if (std.mem.eql(u8, name, "F64")) { return .F64; }
    if (std.mem.eql(u8, name, "F16")) { return .F16; }
    if (std.mem.eql(u8, name, "BF16")) { return .BF16; }
    if (std.mem.eql(u8, name, "I8")) { return .I8; }
    if (std.mem.eql(u8, name, "U8")) { return .U8; }
    if (std.mem.eql(u8, name, "I16")) { return .I16; }
    if (std.mem.eql(u8, name, "U16")) { return .U16; }
    if (std.mem.eql(u8, name, "I32")) { return .I32; }
    if (std.mem.eql(u8, name, "U32")) { return .U32; }
    if (std.mem.eql(u8, name, "I64")) { return .I64; }
    if (std.mem.eql(u8, name, "U64")) { return .U64; }
    if (std.mem.eql(u8, name, "BOOL")) { return .BOOL; }
    return null;
}

pub fn computeShapeElements(shape: []const usize): !usize {
    let total: usize = 1;
    for (dim of shape) {
        const res = @mulWithOverflow(total, dim);
        if (res[1] != 0) {
            return error.InvalidShape;
        }
        total = res[0];
    }
    return total;
}
