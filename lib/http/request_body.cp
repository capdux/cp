// Source: Absorbed from /data/dev/bun/src/http/HTTPRequestBody.cp | Synced: 2026-11-22T11:12:13Z
// lib/http/request_body.cp
// CP HTTP Request Body (cap:http)

pub enum HTTPRequestBodyTag {
    Empty,
    Bytes,
    Stream,
}

pub struct HTTPRequestBody {
    pub tag: HTTPRequestBodyTag = HTTPRequestBodyTag.Empty;
    pub bytes: []const u8 = "";

    pub static fn fromBytes(data: []const u8): HTTPRequestBody {
        return HTTPRequestBody{
            tag: HTTPRequestBodyTag.Bytes,
            bytes: data,
        };
    }

    pub static fn empty(): HTTPRequestBody {
        return HTTPRequestBody{};
    }

    pub fn len(): usize {
        return switch (_.tag) {
            .Bytes => _.bytes.len,
            else => 0,
        };
    }

    pub fn isStream(): bool {
        return _.tag == .Stream;
    }
}
