// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/Auth.cp | Synced: 2026-11-22T03:19:48Z

pub const NATIVE_SCRAMBLE_LEN: usize = 20;
pub const CACHING_SHA2_SCRAMBLE_LEN: usize = 32;
pub const MIN_SALT_LEN: usize = 20;
pub const SSL_REQUEST_PAYLOAD_LEN: usize = 32;
pub const SSL_REQUEST_FRAME_LEN: usize = 36;
pub const CAP_CLIENT_SSL: u32 = 1 << 11;

pub err Error {
    MissingSalt,
    BufferTooSmall,
    TruncatedFrame,
    InvalidFrameLength,
}

/// MySQL 认证插件类型枚举
pub enum AuthMethod: u8 {
    mysql_native_password = 0,
    caching_sha2_password = 1,
    mysql_clear_password = 2,
    sha256_password = 3,

    static fn fromPluginName(plugin_name: string): AuthMethod? {
        if (std.mem.eql(u8, plugin_name, "mysql_native_password")) { return .mysql_native_password; }
        if (std.mem.eql(u8, plugin_name, "caching_sha2_password")) { return .caching_sha2_password; }
        if (std.mem.eql(u8, plugin_name, "mysql_clear_password")) { return .mysql_clear_password; }
        if (std.mem.eql(u8, plugin_name, "sha256_password")) { return .sha256_password; }
        return null;
    }

    fn pluginName(): string {
        return switch (_) {
            .mysql_native_password => "mysql_native_password",
            .caching_sha2_password => "caching_sha2_password",
            .mysql_clear_password => "mysql_clear_password",
            .sha256_password => "sha256_password",
        };
    }

    fn requiresTls(): bool {
        return _ == .mysql_clear_password;
    }
}

/// MySQL caching_sha2_password 快速认证状态字节
pub enum AuthFastStatus: u8 {
    success = 0x03,
    continue_auth = 0x04,

    static fn fromByte(b: u8): AuthFastStatus? {
        if (b == 0x03) { return .success; }
        if (b == 0x04) { return .continue_auth; }
        return null;
    }
}

/// MySQL `mysql_native_password` SHA-1 握手加密（零堆分配）：
/// `SHA1(password) XOR SHA1(salt[0..20] || SHA1(SHA1(password)))`
/// 空密码返回 0 字节空切片
pub fn scrambleNativePassword(password: string, salt: []const u8, out: []u8): ([]const u8)! {
    if (password.len == 0) {
        return out[0..0];
    }
    if (salt.len < MIN_SALT_LEN) {
        return err.MissingSalt;
    }
    if (out.len < NATIVE_SCRAMBLE_LEN) {
        return err.BufferTooSmall;
    }

    let stage1: [20]u8 = [0; 20];
    let stage2: [20]u8 = [0; 20];
    let stage3: [20]u8 = [0; 20];

    std.crypto.hash.Sha1.hash(password, &stage1, {});
    std.crypto.hash.Sha1.hash(stage1[0..20], &stage2, {});

    let h = std.crypto.hash.Sha1.init({});
    h.update(salt[0..20]);
    h.update(stage2[0..20]);
    h.final(&stage3);

    let i: usize = 0;
    while (i < NATIVE_SCRAMBLE_LEN) {
        out[i] = stage1[i] ^ stage3[i];
        i += 1;
    }
    return out[0..NATIVE_SCRAMBLE_LEN];
}

/// MySQL 8.0 `caching_sha2_password` SHA-256 握手加密（零堆分配）：
/// `SHA256(password) XOR SHA256(SHA256(SHA256(password)) || salt)`
/// 空密码返回 0 字节空切片
pub fn scrambleCachingSha2Password(password: string, salt: []const u8, out: []u8): ([]const u8)! {
    if (password.len == 0) {
        return out[0..0];
    }
    if (salt.len < MIN_SALT_LEN) {
        return err.MissingSalt;
    }
    if (out.len < CACHING_SHA2_SCRAMBLE_LEN) {
        return err.BufferTooSmall;
    }

    let stage1: [32]u8 = [0; 32];
    let stage2: [32]u8 = [0; 32];
    let stage3: [32]u8 = [0; 32];

    std.crypto.hash.sha2.Sha256.hash(password, &stage1, {});
    std.crypto.hash.sha2.Sha256.hash(stage1[0..32], &stage2, {});

    let h = std.crypto.hash.sha2.Sha256.init({});
    h.update(stage2[0..32]);
    h.update(salt[0..20]);
    h.final(&stage3);

    let i: usize = 0;
    while (i < CACHING_SHA2_SCRAMBLE_LEN) {
        out[i] = stage1[i] ^ stage3[i];
        i += 1;
    }
    return out[0..CACHING_SHA2_SCRAMBLE_LEN];
}

/// 编码 `mysql_clear_password` NUL 结尾明文密码（零堆分配）
pub fn encodeCleartextPassword(password: string, out: []u8): ([]const u8)! {
    const need = password.len + 1;
    if (out.len < need) {
        return err.BufferTooSmall;
    }
    if (password.len > 0) {
        @memcpy(out[0..password.len], password);
    }
    out[password.len] = 0;
    return out[0..need];
}

/// 对 NUL 结尾密码与盐值执行循环异或（用于 RSA-OAEP 公钥认证前预处理）
pub fn xorPasswordWithNonce(password: string, nonce: []const u8, out: []u8): ([]const u8)! {
    if (nonce.len == 0) {
        return err.MissingSalt;
    }
    const need = password.len + 1;
    if (out.len < need) {
        return err.BufferTooSmall;
    }
    if (password.len > 0) {
        @memcpy(out[0..password.len], password);
    }
    out[password.len] = 0;

    let i: usize = 0;
    while (i < need) {
        out[i] ^= nonce[i % nonce.len];
        i += 1;
    }
    return out[0..need];
}

/// MySQL 32 字节 SSLRequest 报文（含 4 字节包头共 36 字节帧）
pub struct SSLRequest {
    capability_flags: u32,
    max_packet_size: u32 = 0xFFFFFF,
    character_set: u8 = 45, // utf8mb4_general_ci
    sequence_id: u8 = 1,

    /// 编码为 36 字节完整 MySQL SSLRequest 帧（4B 包头 + 32B 负载）
    fn encode(out: []u8): ([]const u8)! {
        if (out.len < SSL_REQUEST_FRAME_LEN) {
            return err.BufferTooSmall;
        }
        @memset(out[0..SSL_REQUEST_FRAME_LEN], 0);

        // 4 字节 MySQL 包头：3B LE 长度(32) + 1B sequence_id
        out[0] = SSL_REQUEST_PAYLOAD_LEN as u8;
        out[1] = 0;
        out[2] = 0;
        out[3] = _.sequence_id;

        // 4 字节 capability_flags (LE)
        const caps = _.capability_flags;
        out[4] = (caps & 0xFF) as u8;
        out[5] = ((caps >> 8) & 0xFF) as u8;
        out[6] = ((caps >> 16) & 0xFF) as u8;
        out[7] = ((caps >> 24) & 0xFF) as u8;

        // 4 字节 max_packet_size (LE)
        const mps = _.max_packet_size;
        out[8] = (mps & 0xFF) as u8;
        out[9] = ((mps >> 8) & 0xFF) as u8;
        out[10] = ((mps >> 16) & 0xFF) as u8;
        out[11] = ((mps >> 24) & 0xFF) as u8;

        // 1 字节 character_set，后接 23 字节零填充
        out[12] = _.character_set;
        return out[0..SSL_REQUEST_FRAME_LEN];
    }

    /// 从 36 字节帧解码 SSLRequest
    static fn decode(frame: []const u8): SSLRequest! {
        if (frame.len < SSL_REQUEST_FRAME_LEN) {
            return err.TruncatedFrame;
        }
        const payload_len: u32 = (frame[0] as u32) |
            ((frame[1] as u32) << 8) |
            ((frame[2] as u32) << 16);
        if (payload_len != SSL_REQUEST_PAYLOAD_LEN) {
            return err.InvalidFrameLength;
        }
        const seq_id = frame[3];
        const caps: u32 = (frame[4] as u32) |
            ((frame[5] as u32) << 8) |
            ((frame[6] as u32) << 16) |
            ((frame[7] as u32) << 24);
        const mps: u32 = (frame[8] as u32) |
            ((frame[9] as u32) << 8) |
            ((frame[10] as u32) << 16) |
            ((frame[11] as u32) << 24);
        const charset = frame[12];

        return SSLRequest{
            capability_flags: caps,
            max_packet_size: mps,
            character_set: charset,
            sequence_id: seq_id,
        };
    }

    fn hasSslCapability(): bool {
        return (_.capability_flags & CAP_CLIENT_SSL) != 0;
    }

    fn eql(other: SSLRequest): bool {
        return _.capability_flags == other.capability_flags &&
            _.max_packet_size == other.max_packet_size &&
            _.character_set == other.character_set &&
            _.sequence_id == other.sequence_id;
    }
}
