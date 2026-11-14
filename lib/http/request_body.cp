/**
 * CP HTTP Request Body (cap:http)
 * Ported from Bun.js http/HTTPRequestBody.cp
 */

pub enum HTTPRequestBodyTag {
    Empty,
    Bytes,
    Stream,
}

pub const HTTPRequestBody = struct {
    tag: HTTPRequestBodyTag = HTTPRequestBodyTag.Empty,
    bytes: []const u8 = "",

    pub fn fromBytes(data: []const u8) HTTPRequestBody {
        let body = HTTPRequestBody{};
        body.tag = HTTPRequestBodyTag.Bytes;
        body.bytes = data;
        return body;
    }

    pub fn empty() HTTPRequestBody {
        return HTTPRequestBody{};
    }

    pub fn len(*_) usize {
        return switch (_.tag) {
            .Bytes => _.bytes.len,
            else => 0,
        };
    }

    pub fn isStream(*_): bool {
        return _.tag == .Stream;
    }
};
