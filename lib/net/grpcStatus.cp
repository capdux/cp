// Reference-hardened against hyperium/tonic (MIT)
// Source: cp/lib/grpc_wire/status.cp (gRPC status codes, errors, and service/method path utilities)

const std = @import("std");

pub const GrpcWireError = error{
    InvalidStatusCode,
    InvalidServicePath,
    InvalidTimeout,
    TimeoutOverflow,
    InvalidPercentEncoding,
    InvalidBinHeader,
    InvalidCompressedFlag,
    MessageTooLarge,
    TruncatedFrame,
    OutOfMemory,
};

pub enum GrpcStatus: u32 {
    OK = 0,
    CANCELLED = 1,
    UNKNOWN = 2,
    INVALID_ARGUMENT = 3,
    DEADLINE_EXCEEDED = 4,
    NOT_FOUND = 5,
    ALREADY_EXISTS = 6,
    PERMISSION_DENIED = 7,
    RESOURCE_EXHAUSTED = 8,
    FAILED_PRECONDITION = 9,
    ABORTED = 10,
    OUT_OF_RANGE = 11,
    UNIMPLEMENTED = 12,
    INTERNAL = 13,
    UNAVAILABLE = 14,
    DATA_LOSS = 15,
    UNAUTHENTICATED = 16,
}

pub struct GrpcError {
    pub status: GrpcStatus,
    pub message: []const u8,
}

pub struct ServiceMethodPath {
    pub package_service: []const u8,
    pub method: []const u8,
}

pub fn statusName(status: GrpcStatus): []const u8 {
    return switch (status) {
        .OK => "OK",
        .CANCELLED => "CANCELLED",
        .UNKNOWN => "UNKNOWN",
        .INVALID_ARGUMENT => "INVALID_ARGUMENT",
        .DEADLINE_EXCEEDED => "DEADLINE_EXCEEDED",
        .NOT_FOUND => "NOT_FOUND",
        .ALREADY_EXISTS => "ALREADY_EXISTS",
        .PERMISSION_DENIED => "PERMISSION_DENIED",
        .RESOURCE_EXHAUSTED => "RESOURCE_EXHAUSTED",
        .FAILED_PRECONDITION => "FAILED_PRECONDITION",
        .ABORTED => "ABORTED",
        .OUT_OF_RANGE => "OUT_OF_RANGE",
        .UNIMPLEMENTED => "UNIMPLEMENTED",
        .INTERNAL => "INTERNAL",
        .UNAVAILABLE => "UNAVAILABLE",
        .DATA_LOSS => "DATA_LOSS",
        .UNAUTHENTICATED => "UNAUTHENTICATED",
    };
}

pub fn statusFromCode(code: u32): GrpcStatus? {
    return switch (code) {
        0 => .OK,
        1 => .CANCELLED,
        2 => .UNKNOWN,
        3 => .INVALID_ARGUMENT,
        4 => .DEADLINE_EXCEEDED,
        5 => .NOT_FOUND,
        6 => .ALREADY_EXISTS,
        7 => .PERMISSION_DENIED,
        8 => .RESOURCE_EXHAUSTED,
        9 => .FAILED_PRECONDITION,
        10 => .ABORTED,
        11 => .OUT_OF_RANGE,
        12 => .UNIMPLEMENTED,
        13 => .INTERNAL,
        14 => .UNAVAILABLE,
        15 => .DATA_LOSS,
        16 => .UNAUTHENTICATED,
        else => null,
    };
}

/// Parses a canonical uppercase gRPC status name into `GrpcStatus` (all 17 codes).
pub fn statusFromName(name: []const u8): GrpcStatus? {
    if (std.mem.eql(u8, name, "OK")) return .OK;
    if (std.mem.eql(u8, name, "CANCELLED")) return .CANCELLED;
    if (std.mem.eql(u8, name, "UNKNOWN")) return .UNKNOWN;
    if (std.mem.eql(u8, name, "INVALID_ARGUMENT")) return .INVALID_ARGUMENT;
    if (std.mem.eql(u8, name, "DEADLINE_EXCEEDED")) return .DEADLINE_EXCEEDED;
    if (std.mem.eql(u8, name, "NOT_FOUND")) return .NOT_FOUND;
    if (std.mem.eql(u8, name, "ALREADY_EXISTS")) return .ALREADY_EXISTS;
    if (std.mem.eql(u8, name, "PERMISSION_DENIED")) return .PERMISSION_DENIED;
    if (std.mem.eql(u8, name, "RESOURCE_EXHAUSTED")) return .RESOURCE_EXHAUSTED;
    if (std.mem.eql(u8, name, "FAILED_PRECONDITION")) return .FAILED_PRECONDITION;
    if (std.mem.eql(u8, name, "ABORTED")) return .ABORTED;
    if (std.mem.eql(u8, name, "OUT_OF_RANGE")) return .OUT_OF_RANGE;
    if (std.mem.eql(u8, name, "UNIMPLEMENTED")) return .UNIMPLEMENTED;
    if (std.mem.eql(u8, name, "INTERNAL")) return .INTERNAL;
    if (std.mem.eql(u8, name, "UNAVAILABLE")) return .UNAVAILABLE;
    if (std.mem.eql(u8, name, "DATA_LOSS")) return .DATA_LOSS;
    if (std.mem.eql(u8, name, "UNAUTHENTICATED")) return .UNAUTHENTICATED;
    return null;
}

pub fn parseServiceMethodPath(path: []const u8): !ServiceMethodPath {
    if (path.len < 4) {
        return error.InvalidServicePath;
    }
    if (path[0] != '/') {
        return error.InvalidServicePath;
    }
    const rest = path[1..];
    let slash_idx: usize? = null;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == '/') {
            if (slash_idx != null) {
                return error.InvalidServicePath;
            }
            slash_idx = i;
        }
        i += 1;
    }
    if (slash_idx == null) {
        return error.InvalidServicePath;
    }
    const idx = slash_idx.?;
    if (idx == 0 or idx + 1 >= rest.len) {
        return error.InvalidServicePath;
    }
    return ServiceMethodPath{
        .package_service = rest[0..idx],
        .method = rest[idx + 1 ..],
    };
}

pub fn formatServiceMethodPath(alloc: std.mem.Allocator, package_service: []const u8, method: []const u8): ![]u8 {
    if (package_service.len == 0 or method.len == 0) {
        return error.InvalidServicePath;
    }
    let i: usize = 0;
    while (i < package_service.len) {
        if (package_service[i] == '/') {
            return error.InvalidServicePath;
        }
        i += 1;
    }
    i = 0;
    while (i < method.len) {
        if (method[i] == '/') {
            return error.InvalidServicePath;
        }
        i += 1;
    }
    return std.fmt.allocPrint(alloc, "/{s}/{s}", .{ package_service, method });
}
