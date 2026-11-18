// Source: Custom (agy-generated export index)
/**
 * CP Standard JSON & Serde (cap:json / cap:serde)
 * 工业级 SIMD 零拷贝流式与编译期强类型序列化与反序列化体系
 */

export { JsonTag, JsonField, JsonValue } from "./value";
export { JsonParser, JsonError, FastJsonScanner } from "./parser";
export { JsonBufferPool } from "./pool";

const std = @import("std");
import { JsonParser } from "./parser";
import { JsonValue } from "./value";

pub class JSON {
    /// High-performance zero-copy dynamic DOM JSON parser
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): !JsonValue {
        return JsonParser.parse(allocator, src);
    }

    /// High-performance SIMD compiled-in typed JSON parser (replaces std.json.parseFromSliceLeaky)
    pub static fn parseTyped(comptime T: type, allocator: std.mem.Allocator, src: []const u8): !T {
        return JsonParser.parseTyped(T, allocator, src);
    }

    pub fn stringify(obj: auto): string {
        return JSON.stringify(obj);
    }
}

pub const json = new JSON();
