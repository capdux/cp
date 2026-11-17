// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
/**
 * CP Standard JSON & Serde (cap:json / cap:serde)
 * 工业级零拷贝序列化与反序列化体系
 */

export { JsonTag, JsonField, JsonValue } from "./value";
export { JsonParser, JsonError } from "./parser";
export { JsonBufferPool } from "./pool";

pub class JSON {
    pub static fn parse(allocator: std.mem.Allocator, src: []const u8): !JsonValue {
        return JsonParser.parse(allocator, src);
    }

    pub fn stringify(obj: auto): string {
        return JSON.stringify(obj);
    }
}

pub const json = new JSON();
