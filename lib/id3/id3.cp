// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ID3v1 / ID3v2 Audio Metadata Parser)

/// ID3v2 10 字节文件头（`"ID3"`）元数据
pub struct Id3v2Header {
    version_major: u8,
    version_minor: u8,
    flags: u8,
    /// 28-bit syncsafe 标签主体字节长度（不含 10 字节头部与可选 10 字节尾部）
    tag_size: u32,

    /// 返回 ID3v2 标签总字节数（`10 + tag_size`，若存在 v2.4 Footer 标志 `0x10` 再加 10）
    pub fn totalBytes(): u32 {
        let total: u32 = _.tag_size + 10;
        if ((_.flags & 0x10) != 0) {
            total += 10;
        }
        return total;
    }
}

/// 零拷贝 ID3v2.3 / ID3v2.4 帧视图
pub struct Id3v2Frame {
    id: [4]u8,
    size: u32,
    flags: u16,
    payload: []const u8,

    /// 比较 4 字节帧 ID 是否等于给定切片（如 `"TIT2"`、`"TPE1"`、`"TALB"`）
    pub fn idEquals(other: []const u8): bool {
        if (other.len != 4) return false;
        return _.id[0] == other[0]
            && _.id[1] == other[1]
            && _.id[2] == other[2]
            && _.id[3] == other[3];
    }

    /// 对 `T***` 文本帧跳过编码字节（`0x00` ISO-8859-1 / `0x03` UTF-8）并剥离尾部 `\0` 返回零拷贝文本切片
    pub fn textContent(): ([]const u8)? {
        if (_.id[0] != 'T' || _.payload.len == 0) return null;
        const enc = _.payload[0];
        if (enc != 0x00 && enc != 0x03) return null;

        let end = _.payload.len;
        while (end > 1 && _.payload[end - 1] == 0) {
            end -= 1;
        }
        return _.payload[1..end];
    }
}

/// 零拷贝 ID3v1 / ID3v1.1 尾部 128 字节标签元数据
pub struct Id3v1Tag {
    title: []const u8,
    artist: []const u8,
    album: []const u8,
    year: []const u8,
    genre: u8,
    /// ID3v1.1 音轨号（当倒数第 3 字节为 `0` 且倒数第 2 字节非 `0` 时有效）
    track: u8?,
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn isValidFrameIdByte(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn trimId3v1Field(raw: []const u8): []const u8 {
    let end = raw.len;
    while (end > 0 && (raw[end - 1] == 0 || raw[end - 1] == ' ')) {
        end -= 1;
    }
    return raw[0..end];
}

/// 解码 ID3v2 4 字节 28-bit Syncsafe 整数（校验长度 `>= 4` 且每字节最高位为 0）
pub fn decodeSyncsafeU32(bytes: []const u8): u32? {
    if (bytes.len < 4) return null;
    if ((bytes[0] & 0x80) != 0
        || (bytes[1] & 0x80) != 0
        || (bytes[2] & 0x80) != 0
        || (bytes[3] & 0x80) != 0) {
        return null;
    }
    return ((bytes[0] as u32) << 21)
        | ((bytes[1] as u32) << 14)
        | ((bytes[2] as u32) << 7)
        | (bytes[3] as u32);
}

/// 解析 10 字节 ID3v2 头部（识别 `"ID3"` 魔数与 syncsafe 长度）
pub fn parseId3v2Header(data: []const u8): Id3v2Header? {
    if (data.len < 10) return null;
    if (data[0] != 'I' || data[1] != 'D' || data[2] != '3') return null;

    const major = data[3];
    const minor = data[4];
    if (major == 0xFF || minor == 0xFF) return null;

    const tag_size = decodeSyncsafeU32(data[6..10]) ?? return null;

    return Id3v2Header {
        version_major: major,
        version_minor: minor,
        flags: data[5],
        tag_size: tag_size,
    };
}

/// 零堆分配 ID3v2.3 / ID3v2.4 帧流式迭代器（遇零填充字节自动停止）
pub struct Id3v2FrameIter {
    data: []const u8,
    pos: usize = 0,
    limit: usize = 0,
    version_major: u8 = 0,

    pub static fn init(data: []const u8): Id3v2FrameIter {
        if (hdr := parseId3v2Header(data)) {
            if (hdr.version_major != 3 && hdr.version_major != 4) {
                return Id3v2FrameIter {
                    data: data,
                    pos: 0,
                    limit: 0,
                    version_major: 0,
                };
            }
            const avail: usize = data.len - 10;
            const declared: usize = hdr.tag_size as usize;
            const bounded: usize = if (declared <= avail) declared else avail;
            const limit: usize = bounded + 10;
            let pos: usize = 10;

            // Skip optional extended header (flag 0x40)
            if ((hdr.flags & 0x40) != 0) {
                if (limit - pos < 4) {
                    return Id3v2FrameIter {
                        data: data,
                        pos: limit,
                        limit: limit,
                        version_major: hdr.version_major,
                    };
                }
                if (hdr.version_major == 4) {
                    if (ext_size := decodeSyncsafeU32(data[pos .. pos + 4])) {
                        const ext_usize = ext_size as usize;
                        if (ext_usize >= 6 && ext_usize <= limit - pos) {
                            pos += ext_usize;
                        } else {
                            pos = limit;
                        }
                    } else {
                        pos = limit;
                    }
                } else {
                    const ext_usize = (readU32BE(data, pos) as usize) + 4;
                    if (ext_usize <= limit - pos) {
                        pos += ext_usize;
                    } else {
                        pos = limit;
                    }
                }
            }

            return Id3v2FrameIter {
                data: data,
                pos: pos,
                limit: limit,
                version_major: hdr.version_major,
            };
        }

        return Id3v2FrameIter {
            data: data,
            pos: 0,
            limit: 0,
            version_major: 0,
        };
    }

    /// 读取下一帧；到达尾部、零填充区或非法帧头时安全返回 `null`
    pub fn next(*_): Id3v2Frame? {
        if (_.version_major != 3 && _.version_major != 4) return null;
        if (_.limit - _.pos < 10) {
            _.pos = _.limit;
            return null;
        }

        // Stop on zero-padding
        if (_.data[_.pos] == 0) {
            _.pos = _.limit;
            return null;
        }

        // Validate 4-byte ASCII frame ID
        if (!isValidFrameIdByte(_.data[_.pos])
            || !isValidFrameIdByte(_.data[_.pos + 1])
            || !isValidFrameIdByte(_.data[_.pos + 2])
            || !isValidFrameIdByte(_.data[_.pos + 3])) {
            _.pos = _.limit;
            return null;
        }

        const id: [4]u8 = [
            _.data[_.pos],
            _.data[_.pos + 1],
            _.data[_.pos + 2],
            _.data[_.pos + 3],
        ];

        let frame_size: u32 = 0;
        if (_.version_major == 4) {
            if (sz := decodeSyncsafeU32(_.data[_.pos + 4 .. _.pos + 8])) {
                frame_size = sz;
            } else {
                _.pos = _.limit;
                return null;
            }
        } else {
            frame_size = readU32BE(_.data, _.pos + 4);
        }

        const frame_flags = readU16BE(_.data, _.pos + 8);
        const size_usize = frame_size as usize;

        if (size_usize > _.limit - (_.pos + 10)) {
            _.pos = _.limit;
            return null;
        }

        const payload = _.data[_.pos + 10 .. _.pos + 10 + size_usize];
        _.pos += 10 + size_usize;

        return Id3v2Frame {
            id: id,
            size: frame_size,
            flags: frame_flags,
            payload: payload,
        };
    }
}

/// 解析文件末尾 128 字节 ID3v1 / ID3v1.1 `"TAG"` 音频标签
pub fn parseId3v1Tail(data: []const u8): Id3v1Tag? {
    if (data.len < 128) return null;
    const tail = data[data.len - 128 .. data.len];
    if (tail[0] != 'T' || tail[1] != 'A' || tail[2] != 'G') return null;

    let track: u8? = null;
    if (tail[125] == 0 && tail[126] != 0) {
        track = tail[126];
    }

    return Id3v1Tag {
        title: trimId3v1Field(tail[3..33]),
        artist: trimId3v1Field(tail[33..63]),
        album: trimId3v1Field(tail[63..93]),
        year: trimId3v1Field(tail[93..97]),
        genre: tail[127],
        track: track,
    };
}
