// Origin: Pure-CP Stdlib (Zero-Alloc Java KeyStore .jks v1/v2 Binary Parser) | Created: 2026-11-22

pub const JKS_MAGIC: u32 = 0xFEEDFEED;
pub const JKS_VERSION_1: u32 = 1;
pub const JKS_VERSION_2: u32 = 2;
pub const JKS_TAG_PRIVATE_KEY: u32 = 1;
pub const JKS_TAG_TRUSTED_CERT: u32 = 2;
pub const JKS_DIGEST_SIZE: usize = 20;

const DEFAULT_V1_CERT_TYPE: []const u8 = "X.509";

/// Java KeyStore 12B 头部与尾部 20B SHA-1 完整性摘要切片（零拷贝）
pub struct JksHeader {
    version: u32,
    entry_count: u32,
    entries_offset: usize,
    digest: []const u8,
    signed_body: []const u8,
}

/// JKS 证书链中的单张证书（零拷贝 DER 切片）
pub struct JksCert {
    cert_type: []const u8,
    der: []const u8,
}

/// JKS 单个条目（PrivateKeyEntry 或 TrustedCertEntry，零拷贝切片）
pub struct JksEntry {
    tag: u32,
    alias: []const u8,
    timestamp_ms: i64,
    protected_key_der: []const u8,
    cert_count: u32,
    certs_slice: []const u8,
    cert_type: []const u8,
    cert_der: []const u8,

    /// 返回 PrivateKeyEntry 附带证书链的零分配迭代器
    fn certs(version: u32): JksCertIter {
        return JksCertIter.init(_.certs_slice, _.cert_count, version);
    }

    fn isPrivateKey(): bool {
        return _.tag == JKS_TAG_PRIVATE_KEY;
    }

    fn isTrustedCert(): bool {
        return _.tag == JKS_TAG_TRUSTED_CERT;
    }
}

/// JKS 单遍结构统计汇总
pub struct JksStats {
    version: u32,
    entry_count: u32,
    private_key_count: u32,
    trusted_cert_count: u32,
    total_certs: u32,
}

// ---------------------------------------------------------------------------
// 内部大端序安全读取与边界校验辅助函数
// ---------------------------------------------------------------------------

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn readU16Be(buf: []const u8, pos: *usize, limit: usize): u16? {
    if (pos.* > limit || limit - pos.* < 2) {
        return null;
    }
    const p = pos.*;
    const v = (u16(buf[p]) << 8) | u16(buf[p + 1]);
    pos.* = p + 2;
    return v;
}

fn readU32Be(buf: []const u8, pos: *usize, limit: usize): u32? {
    if (pos.* > limit || limit - pos.* < 4) {
        return null;
    }
    const p = pos.*;
    const v = (u32(buf[p]) << 24) |
        (u32(buf[p + 1]) << 16) |
        (u32(buf[p + 2]) << 8) |
        u32(buf[p + 3]);
    pos.* = p + 4;
    return v;
}

fn readI64Be(buf: []const u8, pos: *usize, limit: usize): i64? {
    if (pos.* > limit || limit - pos.* < 8) {
        return null;
    }
    const p = pos.*;
    const hi = (u64(buf[p]) << 56) |
        (u64(buf[p + 1]) << 48) |
        (u64(buf[p + 2]) << 40) |
        (u64(buf[p + 3]) << 32);
    const lo = (u64(buf[p + 4]) << 24) |
        (u64(buf[p + 5]) << 16) |
        (u64(buf[p + 6]) << 8) |
        u64(buf[p + 7]);
    pos.* = p + 8;
    const raw = hi | lo;
    return @as(i64, @bitCast(raw));
}

fn readBytes(buf: []const u8, pos: *usize, limit: usize, len: usize): ([]const u8)? {
    if (pos.* > limit || limit - pos.* < len) {
        return null;
    }
    const start = pos.*;
    pos.* = start + len;
    return buf[start..start + len];
}

fn readUtf8Be(buf: []const u8, pos: *usize, limit: usize): ([]const u8)? {
    const slen = readU16Be(buf, pos, limit) ?? return null;
    return readBytes(buf, pos, limit, usize(slen));
}

fn readCertOne(buf: []const u8, pos: *usize, limit: usize, version: u32): JksCert? {
    let ctype = DEFAULT_V1_CERT_TYPE;
    if (version == JKS_VERSION_2) {
        ctype = readUtf8Be(buf, pos, limit) ?? return null;
    }
    const der_len = readU32Be(buf, pos, limit) ?? return null;
    const der = readBytes(buf, pos, limit, usize(der_len)) ?? return null;
    return JksCert{
        cert_type: ctype,
        der: der,
    };
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 快速检测二进制缓冲区是否为合法 JKS v1/v2 密钥库（>= 32B，0xFEEDFEED 魔数，version 1 或 2）
pub fn isJks(data: []const u8): bool {
    if (data.len < 12 + JKS_DIGEST_SIZE) {
        return false;
    }
    let pos: usize = 0;
    const magic = readU32Be(data, *pos, 12) ?? return false;
    const version = readU32Be(data, *pos, 12) ?? return false;
    return magic == JKS_MAGIC && (version == JKS_VERSION_1 || version == JKS_VERSION_2);
}

/// 解析 JKS 12B 头部与尾部 20B SHA-1 完整性摘要切片
pub fn parseJksHeader(data: []const u8): JksHeader? {
    if (data.len < 12 + JKS_DIGEST_SIZE) {
        return null;
    }
    let pos: usize = 0;
    const magic = readU32Be(data, *pos, 12) ?? return null;
    if (magic != JKS_MAGIC) {
        return null;
    }
    const version = readU32Be(data, *pos, 12) ?? return null;
    if (version != JKS_VERSION_1 && version != JKS_VERSION_2) {
        return null;
    }
    const entry_count = readU32Be(data, *pos, 12) ?? return null;

    const body_end = data.len - JKS_DIGEST_SIZE;
    return JksHeader{
        version: version,
        entry_count: entry_count,
        entries_offset: 12,
        digest: data[body_end..data.len],
        signed_body: data[0..body_end],
    };
}

/// 零分配迭代 PrivateKeyEntry 附带的证书链（自动适配 v1 无 type 与 v2 带 type 格式）
pub struct JksCertIter {
    data: []const u8,
    pos: usize,
    remaining: u32,
    version: u32,
    malformed: bool,

    fn init(certs_slice: []const u8, cert_count: u32, version: u32): JksCertIter {
        return JksCertIter{
            data: certs_slice,
            pos: 0,
            remaining: cert_count,
            version: version,
            malformed: false,
        };
    }

    fn next(*_): JksCert? {
        if (_.remaining == 0 || _.malformed) {
            return null;
        }
        if (cert := readCertOne(_.data, *_.pos, _.data.len, _.version)) {
            _.remaining -= 1;
            return cert;
        }
        _.malformed = true;
        _.remaining = 0;
        return null;
    }
}

/// 零分配逐条迭代 JKS 中的全部条目（严格不越过 data.len - 20 摘要边界）
pub struct JksEntryIter {
    data: []const u8,
    pos: usize,
    body_end: usize,
    remaining: u32,
    version: u32,
    malformed: bool,

    fn init(data: []const u8): JksEntryIter {
        if (hdr := parseJksHeader(data)) {
            return JksEntryIter{
                data: data,
                pos: hdr.entries_offset,
                body_end: hdr.signed_body.len,
                remaining: hdr.entry_count,
                version: hdr.version,
                malformed: false,
            };
        }
        return JksEntryIter{
            data: data,
            pos: 0,
            body_end: 0,
            remaining: 0,
            version: 0,
            malformed: true,
        };
    }

    fn next(*_): JksEntry? {
        if (_.remaining == 0 || _.malformed) {
            return null;
        }

        const tag_opt = readU32Be(_.data, *_.pos, _.body_end);
        if (tag_opt == null) {
            _.malformed = true;
            _.remaining = 0;
            return null;
        }
        const tag = tag_opt ?? unreachable;

        const alias_opt = readUtf8Be(_.data, *_.pos, _.body_end);
        if (alias_opt == null) {
            _.malformed = true;
            _.remaining = 0;
            return null;
        }
        const alias = alias_opt ?? unreachable;

        const ts_opt = readI64Be(_.data, *_.pos, _.body_end);
        if (ts_opt == null) {
            _.malformed = true;
            _.remaining = 0;
            return null;
        }
        const timestamp_ms = ts_opt ?? unreachable;

        const empty = _.data[0..0];

        if (tag == JKS_TAG_PRIVATE_KEY) {
            const klen_opt = readU32Be(_.data, *_.pos, _.body_end);
            if (klen_opt == null) {
                _.malformed = true;
                _.remaining = 0;
                return null;
            }
            const klen = klen_opt ?? unreachable;
            const key_der_opt = readBytes(_.data, *_.pos, _.body_end, usize(klen));
            if (key_der_opt == null) {
                _.malformed = true;
                _.remaining = 0;
                return null;
            }
            const protected_key_der = key_der_opt ?? unreachable;

            const cc_opt = readU32Be(_.data, *_.pos, _.body_end);
            if (cc_opt == null) {
                _.malformed = true;
                _.remaining = 0;
                return null;
            }
            const cert_count = cc_opt ?? unreachable;

            const certs_start = _.pos;
            let ci: u32 = 0;
            while (ci < cert_count) {
                if (readCertOne(_.data, *_.pos, _.body_end, _.version) == null) {
                    _.malformed = true;
                    _.remaining = 0;
                    return null;
                }
                ci += 1;
            }
            const certs_slice = _.data[certs_start.._.pos];

            _.remaining -= 1;
            return JksEntry{
                tag: tag,
                alias: alias,
                timestamp_ms: timestamp_ms,
                protected_key_der: protected_key_der,
                cert_count: cert_count,
                certs_slice: certs_slice,
                cert_type: empty,
                cert_der: empty,
            };
        }

        if (tag == JKS_TAG_TRUSTED_CERT) {
            const cert_opt = readCertOne(_.data, *_.pos, _.body_end, _.version);
            if (cert_opt == null) {
                _.malformed = true;
                _.remaining = 0;
                return null;
            }
            const cert = cert_opt ?? unreachable;

            _.remaining -= 1;
            return JksEntry{
                tag: tag,
                alias: alias,
                timestamp_ms: timestamp_ms,
                protected_key_der: empty,
                cert_count: 1,
                certs_slice: empty,
                cert_type: cert.cert_type,
                cert_der: cert.der,
            };
        }

        // 未知 tag
        _.malformed = true;
        _.remaining = 0;
        return null;
    }
}

/// 按别名零分配查找 JKS 条目
pub fn findJksEntry(data: []const u8, alias: []const u8): JksEntry? {
    let it = JksEntryIter.init(data);
    while (entry := it.next()) {
        if (eqlBytes(entry.alias, alias)) {
            return entry;
        }
    }
    return null;
}

/// 单遍校验并统计 JKS 密钥库摘要（若条目截断或剩余字节不为精确 20B 尾摘要则返回 null）
pub fn summarizeJks(data: []const u8): JksStats? {
    const hdr = parseJksHeader(data) ?? return null;
    let stats = JksStats{
        version: hdr.version,
        entry_count: 0,
        private_key_count: 0,
        trusted_cert_count: 0,
        total_certs: 0,
    };

    let it = JksEntryIter.init(data);
    while (entry := it.next()) {
        stats.entry_count += 1;
        if (entry.tag == JKS_TAG_PRIVATE_KEY) {
            stats.private_key_count += 1;
            stats.total_certs += entry.cert_count;
        } else if (entry.tag == JKS_TAG_TRUSTED_CERT) {
            stats.trusted_cert_count += 1;
            stats.total_certs += 1;
        }
    }

    // 严格校验：无畸形、完整消费 entry_count 条、且恰好停在 data.len - 20 摘要边界
    if (it.malformed || stats.entry_count != hdr.entry_count || it.pos != hdr.signed_body.len) {
        return null;
    }

    return stats;
}
