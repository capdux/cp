// Source: Absorbed from /data/dev/bun/src/s3_signing/credentials.cp | Synced: 2026-11-22T09:39:48Z
// Ported from Bun: /data/dev/bun/src/s3_signing/credentials.cp & /data/dev/bun/docs/runtime/s3.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation S3 Multi-Cloud Endpoint Parser, Region/Bucket Guesser,
// SigV4 Key Normalizer/Encoder, Presign Expiry Validator & CRLF Header Injection Guard:
// - Parses AWS S3, Cloudflare R2, DigitalOcean Spaces, Backblaze B2, MinIO/localhost & custom S3-compatible endpoints
// - Guesses bucket and region from virtual-hosted and provider endpoints with `us-east-1` / `auto` fallbacks
// - Normalizes object keys (trimming `/` and `\`) and percent-encodes per RFC 3986 with Windows `\` -> `/` mapping
// - Validates presigned URL expiration bounds (1..=604800 seconds) and blocks CRLF (`\r`/`\n`) header injection.

/// AWS SigV4 预签名 URL 最小有效期（1 秒）
pub const S3_PRESIGN_MIN_EXPIRES: u64 = 1;

/// AWS SigV4 预签名 URL 最大有效期（7 天 = 604800 秒）
pub const S3_PRESIGN_MAX_EXPIRES: u64 = 604800;

/// AWS SigV4 预签名 URL 默认有效期（24 小时 = 86400 秒，对齐 Bun `S3SignQueryOptions`）
pub const S3_PRESIGN_DEFAULT_EXPIRES: u64 = 86400;

/// S3 端点与凭证辅助错误集
pub err S3EndpointError {
    EmptyEndpoint,
    InvalidScheme,
    EmptyHost,
    InvalidPort,
    EmptyBucket,
    InvalidPresignExpiry,
    CrlfInjectionDetected,
    BufferTooSmall,
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') { return c + 32; }
    return c;
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    let i: usize = 0;
    while (i < prefix.len) {
        if (asciiLower(s[i]) != asciiLower(prefix[i])) { return false; }
        i += 1;
    }
    return true;
}

fn endsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) { return false; }
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (asciiLower(s[off + i]) != asciiLower(suffix[i])) { return false; }
        i += 1;
    }
    return true;
}

fn indexOfIgnoreCase(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) { return 0; }
    if (needle.len > haystack.len) { return null; }
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (asciiLower(haystack[i + j]) != asciiLower(needle[j])) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) { return i; }
        i += 1;
    }
    return null;
}

fn indexOfByte(s: []const u8, b: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == b) { return i; }
        i += 1;
    }
    return null;
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): S3EndpointError!bool {
    if (pos.* + s.len > buf.len) {
        return S3EndpointError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(buf: []u8, pos: *usize, b: u8): S3EndpointError!bool {
    if (pos.* + 1 > buf.len) {
        return S3EndpointError.BufferTooSmall;
    }
    buf[pos.*] = b;
    pos.* += 1;
    return true;
}

/// 剥离可选 `http://` / `https://` 协议头和路径后缀，返回纯 `host[:port]`
fn extractHostOnly(endpoint: []const u8): []const u8 {
    let s = endpoint;
    if (startsWithIgnoreCase(s, "https://")) {
        s = s[8..s.len];
    } else if (startsWithIgnoreCase(s, "http://")) {
        s = s[7..s.len];
    }
    if (slash := indexOfByte(s, '/')) {
        s = s[0..slash];
    }
    if (colon := indexOfByte(s, ':')) {
        s = s[0..colon];
    }
    return s;
}

fn isIpv4OrLocalhost(host: []const u8): bool {
    if (host.len == 0) { return false; }
    if (startsWithIgnoreCase(host, "localhost") && host.len == 9) { return true; }
    if (host[0] == '[') { return true; } // IPv6 literal
    let dots: usize = 0;
    let i: usize = 0;
    while (i < host.len) {
        const c = host[i];
        if (c == '.') {
            dots += 1;
        } else if (c < '0' || c > '9') {
            return false;
        }
        i += 1;
    }
    return dots == 3;
}

/// S3 兼容云存储服务商类别（对齐 `/data/dev/bun/docs/runtime/s3.mdx`）
pub enum S3EndpointProvider: u8 {
    AwsS3 = 0,
    CloudflareR2 = 1,
    DigitalOceanSpaces = 2,
    BackblazeB2 = 3,
    MinioLocal = 4,
    Custom = 5,

    fn asStr(): []const u8 {
        return switch (_) {
            .AwsS3 => "aws-s3",
            .CloudflareR2 => "cloudflare-r2",
            .DigitalOceanSpaces => "digitalocean-spaces",
            .BackblazeB2 => "backblaze-b2",
            .MinioLocal => "minio-local",
            .Custom => "custom",
        };
    }
}

/// 检测 S3 请求头值是否包含 `\r` 或 `\n`（防 CRLF 注入，对齐 Bun `containsNewlineOrCR`）
pub fn containsS3Crlf(value: []const u8): bool {
    let i: usize = 0;
    while (i < value.len) {
        if (value[i] == '\r' || value[i] == '\n') {
            return true;
        }
        i += 1;
    }
    return false;
}

/// 严格校验 S3 签名头值不含 CRLF
pub fn validateS3HeaderValue(value: []const u8): S3EndpointError!bool {
    if (containsS3Crlf(value)) {
        return S3EndpointError.CrlfInjectionDetected;
    }
    return true;
}

/// 校验 AWS SigV4 预签名 URL 有效期（1..=604800 秒）
pub fn validateS3PresignExpiry(expires_seconds: u64): S3EndpointError!u64 {
    if (expires_seconds < S3_PRESIGN_MIN_EXPIRES || expires_seconds > S3_PRESIGN_MAX_EXPIRES) {
        return S3EndpointError.InvalidPresignExpiry;
    }
    return expires_seconds;
}

/// 规范化 S3 对象键（剥离首尾 `/` 与 `\`，对齐 Bun `S3Credentials.normalizeName`）
pub fn normalizeS3ObjectKey(name: []const u8): []const u8 {
    let start: usize = 0;
    while (start < name.len && (name[start] == '/' || name[start] == '\\')) {
        start += 1;
    }
    let end = name.len;
    while (end > start && (name[end - 1] == '/' || name[end - 1] == '\\')) {
        end -= 1;
    }
    return name[start..end];
}

/// AWS SigV4 RFC 3986 百分比编码（`encode_slash=false` 时保留 `/` 并将 Windows `\` 规范化为 `/`）
pub fn encodeS3UriComponent(input: []const u8, buf: []u8, encode_slash: bool): S3EndpointError!usize {
    const HEX = "0123456789ABCDEF";
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        const is_unreserved = (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~';
        if (is_unreserved) {
            * = appendByte(buf, &pos, c)?;
        } else if (!encode_slash && (c == '/' || c == '\\')) {
            * = appendByte(buf, &pos, '/')?;
        } else {
            if (pos + 3 > buf.len) { return S3EndpointError.BufferTooSmall; }
            buf[pos] = '%';
            buf[pos + 1] = HEX[@as(usize, (c >> 4) & 0xF)];
            buf[pos + 2] = HEX[@as(usize, c & 0xF)];
            pos += 3;
        }
        i += 1;
    }
    return pos;
}

/// 从 S3 Virtual-Hosted 端点自动猜测 Bucket 名称（支持 AWS S3 与 Cloudflare R2，对齐 Bun `guessBucket`）
pub fn guessS3Bucket(endpoint: []const u8): ([]const u8)? {
    const host = extractHostOnly(endpoint);
    if (host.len == 0) { return null; }

    // AWS S3: <bucket>.s3.<region>.amazonaws.com 或 <bucket>.s3.amazonaws.com
    if (indexOfIgnoreCase(host, ".amazonaws.com") != null) {
        if (s3_idx := indexOfIgnoreCase(host, ".s3.")) {
            if (s3_idx > 0) {
                return host[0..s3_idx];
            }
        }
        return null;
    }

    // Cloudflare R2: <bucket>.<account-id>.r2.cloudflarestorage.com
    if (r2_idx := indexOfIgnoreCase(host, ".r2.cloudflarestorage.com")) {
        if (first_dot := indexOfByte(host, '.')) {
            if (first_dot > 0 && first_dot < r2_idx) {
                return host[0..first_dot];
            }
        }
        return null;
    }

    return null;
}

/// 从多云 S3 端点自动推导 Region（对齐 Bun `guessRegion`：空端点回退 `us-east-1`，R2/未知端点返回 `auto`）
pub fn guessS3Region(endpoint: []const u8): []const u8 {
    if (endpoint.len == 0) {
        return "us-east-1";
    }
    const host = extractHostOnly(endpoint);
    if (host.len == 0) {
        return "us-east-1";
    }
    if (endsWithIgnoreCase(host, ".r2.cloudflarestorage.com")) {
        return "auto";
    }
    if (aws_end := indexOfIgnoreCase(host, ".amazonaws.com")) {
        if (s3_pos := indexOfIgnoreCase(host, "s3.")) {
            const reg_start = s3_pos + 3;
            if (reg_start < aws_end) {
                return host[reg_start..aws_end];
            }
        }
        return "us-east-1";
    }
    if (do_end := indexOfIgnoreCase(host, ".digitaloceanspaces.com")) {
        // <region>.digitaloceanspaces.com 或 <bucket>.<region>.digitaloceanspaces.com
        let seg_start: usize = 0;
        let i: usize = 0;
        while (i < do_end) {
            if (host[i] == '.') { seg_start = i + 1; }
            i += 1;
        }
        if (seg_start < do_end) {
            return host[seg_start..do_end];
        }
    }
    if (b2_end := indexOfIgnoreCase(host, ".backblazeb2.com")) {
        if (s3_pos := indexOfIgnoreCase(host, "s3.")) {
            const reg_start = s3_pos + 3;
            if (reg_start < b2_end) {
                return host[reg_start..b2_end];
            }
        }
    }
    return "auto";
}

/// 解析后的 S3 端点元数据
pub struct S3ParsedEndpoint {
    is_https: bool = true,
    host: []const u8 = "",
    port: u16 = 0,
    provider: S3EndpointProvider = .Custom,
    guessed_bucket: ([]const u8)? = null,
    guessed_region: []const u8 = "us-east-1",
    requires_path_style: bool = false,

    /// 解析完整或裸主机 S3 Endpoint（校验协议头、端口与服务商类型）
    static fn parse(raw: []const u8): S3EndpointError!S3ParsedEndpoint {
        if (raw.len == 0) { return S3EndpointError.EmptyEndpoint; }
        if (containsS3Crlf(raw)) { return S3EndpointError.CrlfInjectionDetected; }

        let is_https = true;
        let rest = raw;
        if (startsWithIgnoreCase(raw, "https://")) {
            is_https = true;
            rest = raw[8..raw.len];
        } else if (startsWithIgnoreCase(raw, "http://")) {
            is_https = false;
            rest = raw[7..raw.len];
        } else if (indexOfIgnoreCase(raw, "://") != null) {
            return S3EndpointError.InvalidScheme;
        }

        if (slash := indexOfByte(rest, '/')) {
            rest = rest[0..slash];
        }
        if (rest.len == 0) { return S3EndpointError.EmptyHost; }

        let host = rest;
        let port: u16 = 0;
        if (rest[0] != '[') {
            if (colon := indexOfByte(rest, ':')) {
                host = rest[0..colon];
                const port_str = rest[(colon + 1)..rest.len];
                if (port_str.len == 0) { return S3EndpointError.InvalidPort; }
                let pv: u32 = 0;
                let pi: usize = 0;
                while (pi < port_str.len) {
                    const c = port_str[pi];
                    if (c < '0' || c > '9') { return S3EndpointError.InvalidPort; }
                    pv = pv * 10 + @as(u32, c - '0');
                    if (pv > 65535) { return S3EndpointError.InvalidPort; }
                    pi += 1;
                }
                if (pv == 0) { return S3EndpointError.InvalidPort; }
                port = @as(u16, @intCast(pv));
            }
        }
        if (host.len == 0) { return S3EndpointError.EmptyHost; }

        let provider = S3EndpointProvider.Custom;
        if (endsWithIgnoreCase(host, ".amazonaws.com")) {
            provider = .AwsS3;
        } else if (endsWithIgnoreCase(host, ".r2.cloudflarestorage.com")) {
            provider = .CloudflareR2;
        } else if (endsWithIgnoreCase(host, ".digitaloceanspaces.com")) {
            provider = .DigitalOceanSpaces;
        } else if (endsWithIgnoreCase(host, ".backblazeb2.com")) {
            provider = .BackblazeB2;
        } else if (isIpv4OrLocalhost(host)) {
            provider = .MinioLocal;
        }

        return S3ParsedEndpoint{
            is_https: is_https,
            host: host,
            port: port,
            provider: provider,
            guessed_bucket: guessS3Bucket(raw),
            guessed_region: guessS3Region(raw),
            requires_path_style: isIpv4OrLocalhost(host),
        };
    }

    /// 构造规范请求 URL（自动判定 Virtual-Hosted vs Path-Style；含 `.` 的 Bucket 名或 IP/localhost 强制 Path-Style）
    fn formatObjectUrl(bucket: []const u8, object_key: []const u8, virtual_hosted: bool, buf: []u8): S3EndpointError!usize {
        if (bucket.len == 0) { return S3EndpointError.EmptyBucket; }
        if (containsS3Crlf(bucket) || containsS3Crlf(object_key)) {
            return S3EndpointError.CrlfInjectionDetected;
        }

        const bucket_has_dot = indexOfByte(bucket, '.') != null;
        const use_vhost = virtual_hosted && !_.requires_path_style && !bucket_has_dot;

        let pos: usize = 0;
        * = appendSlice(buf, &pos, if (_.is_https) "https://" else "http://")?;
        if (use_vhost) {
            * = appendSlice(buf, &pos, bucket)?;
            * = appendByte(buf, &pos, '.')?;
        }
        * = appendSlice(buf, &pos, _.host)?;
        if (_.port > 0) {
            * = appendByte(buf, &pos, ':')?;
            let ptmp: [8]u8 = [0; 8];
            let plen: usize = 0;
            let pv = _.port;
            while (pv > 0 && plen < 8) {
                ptmp[plen] = @as(u8, @intCast(pv % 10)) + '0';
                pv = pv / 10;
                plen += 1;
            }
            let k = plen;
            while (k > 0) {
                k -= 1;
                * = appendByte(buf, &pos, ptmp[k])?;
            }
        }

        if (!use_vhost) {
            * = appendByte(buf, &pos, '/')?;
            * = appendSlice(buf, &pos, bucket)?;
        }

        const norm_key = normalizeS3ObjectKey(object_key);
        * = appendByte(buf, &pos, '/')?;
        if (norm_key.len > 0) {
            const enc_len = encodeS3UriComponent(norm_key, buf[pos..buf.len], false)?;
            pos += enc_len;
        }
        return pos;
    }
}
