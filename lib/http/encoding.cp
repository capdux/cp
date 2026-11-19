// Source: /data/dev/bun/src/http_types/Encoding.cp
// lib/http/encoding.cp
// CP HTTP Content & Transfer Encoding (cap:http)

pub enum Encoding {
    Identity,
    Gzip,
    Deflate,
    Brotli,
    Zstd,
    Chunked,

    pub fn canUseLibDeflate(): bool {
        return switch (_) {
            .Gzip, .Deflate => true,
            else => false,
        };
    }

    pub fn isCompressed(): bool {
        return switch (_) {
            .Gzip, .Deflate, .Brotli, .Zstd => true,
            else => false,
        };
    }
}
