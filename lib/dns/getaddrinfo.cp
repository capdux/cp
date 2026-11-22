// Source: Absorbed from /data/dev/bun/src/dns/dns.cp | Synced: 2026-11-22T02:31:32Z
// Pure-CP zero-allocation RFC 3493 / Bun `GetAddrInfo` query options, address
// family/socket-type/protocol/backend parsers, AI_* flag bitmask validator,
// and deterministic case-insensitive cache-key hasher (zero std.posix import).

pub const MAX_DNS_HOSTNAME_LEN: usize = 253;

// Standard RFC 3493 / POSIX AI_* flag bit values (pure numeric constants)
pub const AI_FLAG_PASSIVE: u32 = 0x0001;
pub const AI_FLAG_CANONNAME: u32 = 0x0002;
pub const AI_FLAG_NUMERICHOST: u32 = 0x0004;
pub const AI_FLAG_V4MAPPED: u32 = 0x0008;
pub const AI_FLAG_ALL: u32 = 0x0010;
pub const AI_FLAG_ADDRCONFIG: u32 = 0x0020;
pub const AI_FLAG_NUMERICSERV: u32 = 0x0400;
pub const AI_FLAGS_KNOWN_MASK: u32 =
    AI_FLAG_PASSIVE | AI_FLAG_CANONNAME | AI_FLAG_NUMERICHOST |
    AI_FLAG_V4MAPPED | AI_FLAG_ALL | AI_FLAG_ADDRCONFIG | AI_FLAG_NUMERICSERV;

/// `GetAddrInfo` 选项与查询构建错误集
pub err GetAddrInfoError {
    EmptyHostname,
    HostnameTooLong,
    InvalidFamily,
    InvalidSocketType,
    InvalidProtocol,
    InvalidBackend,
    InvalidFlags,
    BufferTooSmall,
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn fnv1a64Byte(h: u64, b: u8): u64 {
    return (h ^ (b as u64)) *% 0x100000001b3;
}

fn fnv1a64U64(h_in: u64, val: u64): u64 {
    let h = h_in;
    h = fnv1a64Byte(h, (val & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 8) & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 16) & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 24) & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 32) & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 40) & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 48) & 0xff) as u8);
    h = fnv1a64Byte(h, ((val >> 56) & 0xff) as u8);
    return h;
}

fn appendBytes(buf: []u8, pos: usize, s: string): usize! {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

fn appendU16(buf: []u8, pos: usize, val: u16): usize! {
    let tmp: [8]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ('0' as u8) + ((v % 10) as u8);
            v /= 10;
            n += 1;
        }
    }
    if (pos + n > buf.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < n) {
        buf[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// 地址族枚举（对齐 Bun `GetAddrInfo.Family`）
pub enum AddrInfoFamily: u8 {
    unspecified = 0,
    inet = 1,
    inet6 = 2,
    unix = 3,

    static fn parse(s: string): AddrInfoFamily! {
        if (eqlIgnoreCaseAscii(s, "ipv4") || std.mem.eql(u8, s, "4") || eqlIgnoreCaseAscii(s, "inet")) {
            return .inet;
        }
        if (eqlIgnoreCaseAscii(s, "ipv6") || std.mem.eql(u8, s, "6") || eqlIgnoreCaseAscii(s, "inet6")) {
            return .inet6;
        }
        if (eqlIgnoreCaseAscii(s, "any") || std.mem.eql(u8, s, "0") || eqlIgnoreCaseAscii(s, "unspecified")) {
            return .unspecified;
        }
        if (eqlIgnoreCaseAscii(s, "unix")) {
            return .unix;
        }
        return err.InvalidFamily;
    }

    fn asStr(): string {
        return switch (_) {
            .unspecified => "any",
            .inet => "IPv4",
            .inet6 => "IPv6",
            .unix => "unix",
        };
    }

    /// 映射至 Linux/POSIX `AF_*` 标准数值（0=AF_UNSPEC, 1=AF_UNIX, 2=AF_INET, 10=AF_INET6）
    fn toPosixAf(): i32 {
        return switch (_) {
            .unspecified => 0,
            .unix => 1,
            .inet => 2,
            .inet6 => 10,
        };
    }
}

/// 套接字类型枚举（对齐 Bun `GetAddrInfo.SocketType`）
pub enum AddrInfoSocketType: u8 {
    unspecified = 0,
    stream = 1,
    dgram = 2,

    static fn parse(s: string): AddrInfoSocketType! {
        if (eqlIgnoreCaseAscii(s, "stream")) { return .stream; }
        if (eqlIgnoreCaseAscii(s, "dgram")) { return .dgram; }
        if (eqlIgnoreCaseAscii(s, "any") || eqlIgnoreCaseAscii(s, "unspecified")) { return .unspecified; }
        return err.InvalidSocketType;
    }

    fn asStr(): string {
        return switch (_) {
            .unspecified => "any",
            .stream => "stream",
            .dgram => "dgram",
        };
    }

    fn toPosixSockType(): i32 {
        return switch (_) {
            .unspecified => 0,
            .stream => 1,
            .dgram => 2,
        };
    }
}

/// 传输层协议枚举（对齐 Bun `GetAddrInfo.Protocol`）
pub enum AddrInfoProtocol: u8 {
    unspecified = 0,
    tcp = 1,
    udp = 2,

    static fn parse(s: string): AddrInfoProtocol! {
        if (eqlIgnoreCaseAscii(s, "tcp")) { return .tcp; }
        if (eqlIgnoreCaseAscii(s, "udp")) { return .udp; }
        if (eqlIgnoreCaseAscii(s, "any") || eqlIgnoreCaseAscii(s, "unspecified")) { return .unspecified; }
        return err.InvalidProtocol;
    }

    fn asStr(): string {
        return switch (_) {
            .unspecified => "any",
            .tcp => "tcp",
            .udp => "udp",
        };
    }

    /// 映射至 IANA/POSIX `IPPROTO_*` 标准数值（0=UNSPEC, 6=TCP, 17=UDP）
    fn toPosixProto(): i32 {
        return switch (_) {
            .unspecified => 0,
            .tcp => 6,
            .udp => 17,
        };
    }
}

/// DNS 解析后端枚举（对齐 Bun `GetAddrInfo.Backend` 全部别名映射）
pub enum AddrInfoBackend: u8 {
    c_ares = 0,
    system = 1,
    libc = 2,

    static fn parse(s: string): AddrInfoBackend! {
        if (eqlIgnoreCaseAscii(s, "c-ares") || eqlIgnoreCaseAscii(s, "c_ares") || eqlIgnoreCaseAscii(s, "cares") || eqlIgnoreCaseAscii(s, "async")) {
            return .c_ares;
        }
        if (eqlIgnoreCaseAscii(s, "system")) {
            return .system;
        }
        if (eqlIgnoreCaseAscii(s, "libc") || eqlIgnoreCaseAscii(s, "getaddrinfo")) {
            return .libc;
        }
        return err.InvalidBackend;
    }

    fn asStr(): string {
        return switch (_) {
            .c_ares => "c-ares",
            .system => "system",
            .libc => "libc",
        };
    }
}

/// RFC 3493 `AI_*` 标志位集合
pub struct AddrInfoFlags {
    bits: u32 = 0,

    static fn empty(): AddrInfoFlags {
        return AddrInfoFlags{ bits: 0 };
    }

    static fn fromBits(raw_bits: u32): AddrInfoFlags! {
        if ((raw_bits & (~AI_FLAGS_KNOWN_MASK)) != 0) {
            return err.InvalidFlags;
        }
        return AddrInfoFlags{ bits: raw_bits };
    }

    fn has(mask: u32): bool {
        return (_.bits & mask) == mask;
    }

    fn withFlag(mask: u32): AddrInfoFlags {
        return AddrInfoFlags{ bits: _.bits | (mask & AI_FLAGS_KNOWN_MASK) };
    }

    fn eql(other: AddrInfoFlags): bool {
        return _.bits == other.bits;
    }
}

/// `GetAddrInfo` 完整查询选项（可压缩打包为确定性 `u64`）
pub struct GetAddrInfoOptions {
    family: AddrInfoFamily = .unspecified,
    socktype: AddrInfoSocketType = .stream,
    protocol: AddrInfoProtocol = .unspecified,
    backend: AddrInfoBackend = .c_ares,
    flags: AddrInfoFlags = AddrInfoFlags{},

    /// 检查是否无显式 hints（对齐 Bun `Options.toLibC()` null-hints 优化判定）
    fn hasNoHints(): bool {
        return _.family == .unspecified &&
            _.socktype == .unspecified &&
            _.protocol == .unspecified &&
            _.flags.bits == 0;
    }

    /// 打包为确定性 64 位整数（用于快速哈希与等价比较）
    fn packU64(): u64 {
        const f = (@intFromEnum(_.family) as u64) & 0x3;
        const s = (@intFromEnum(_.socktype) as u64) & 0x3;
        const p = (@intFromEnum(_.protocol) as u64) & 0x3;
        const b = (@intFromEnum(_.backend) as u64) & 0x3;
        return f | (s << 2) | (p << 4) | (b << 6) | ((_.flags.bits as u64) << 8);
    }

    fn eql(other: GetAddrInfoOptions): bool {
        return _.packU64() == other.packU64();
    }
}

/// 零堆分配 `GetAddrInfo` 查询描述符（支持大小写无关主机名哈希与摘要格式化）
pub struct GetAddrInfoQuery {
    name: string = "",
    port: u16 = 0,
    options: GetAddrInfoOptions = GetAddrInfoOptions{},

    static fn init(name: string, port: u16, options: GetAddrInfoOptions): GetAddrInfoQuery! {
        if (name.len == 0) {
            return err.EmptyHostname;
        }
        if (name.len > MAX_DNS_HOSTNAME_LEN) {
            return err.HostnameTooLong;
        }
        return GetAddrInfoQuery{
            name: name,
            port: port,
            options: options,
        };
    }

    /// 计算确定性 64 位查询缓存键哈希（对齐 Bun `GetAddrInfo.hash()`，并对主机名执行 ASCII 小写归一化）
    fn hash(): u64 {
        let h: u64 = 0xcbf29ce484222325;
        h = fnv1a64U64(h, _.options.packU64());
        h = fnv1a64Byte(h, (_.port & 0xff) as u8);
        h = fnv1a64Byte(h, ((_.port >> 8) & 0xff) as u8);
        let i: usize = 0;
        while (i < _.name.len) {
            h = fnv1a64Byte(h, toLowerAscii(_.name[i]));
            i += 1;
        }
        return h;
    }

    fn isLocalhost(): bool {
        return eqlIgnoreCaseAscii(_.name, "localhost") ||
            eqlIgnoreCaseAscii(_.name, "localhost.") ||
            std.mem.eql(u8, _.name, "127.0.0.1") ||
            std.mem.eql(u8, _.name, "::1");
    }

    fn eql(other: GetAddrInfoQuery): bool {
        return _.port == other.port &&
            _.options.eql(other.options) &&
            eqlIgnoreCaseAscii(_.name, other.name);
    }

    /// 格式化查询摘要，如 `"example.com:443 [IPv4/stream/tcp via c-ares]"`
    fn formatSummary(buf: []u8): string! {
        let pos: usize = 0;
        pos = appendBytes(buf, pos, _.name)?;
        pos = appendBytes(buf, pos, ":")?;
        pos = appendU16(buf, pos, _.port)?;
        pos = appendBytes(buf, pos, " [")?;
        pos = appendBytes(buf, pos, _.options.family.asStr())?;
        pos = appendBytes(buf, pos, "/")?;
        pos = appendBytes(buf, pos, _.options.socktype.asStr())?;
        pos = appendBytes(buf, pos, "/")?;
        pos = appendBytes(buf, pos, _.options.protocol.asStr())?;
        pos = appendBytes(buf, pos, " via ")?;
        pos = appendBytes(buf, pos, _.options.backend.asStr())?;
        pos = appendBytes(buf, pos, "]")?;
        return buf[0..pos];
    }
}
