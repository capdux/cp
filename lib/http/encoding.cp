/**
 * CP HTTP Content & Transfer Encoding (cap:http)
 * Ported from Bun.js http_types/Encoding.cp
 */

pub enum Encoding {
    Identity,
    Gzip,
    Deflate,
    Brotli,
    Zstd,
    Chunked,

    fn canUseLibDeflate(): bool {
        return switch (_) {
            .Gzip, .Deflate => true,
            else => false,
        };
    }

    fn isCompressed(): bool {
        return switch (_) {
            .Gzip, .Deflate, .Brotli, .Zstd => true,
            else => false,
        };
    }
}
