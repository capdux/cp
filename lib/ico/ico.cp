// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ICO / CUR Icon Container Parser)

/// Windows ICO / CUR 容器类型
pub enum IcoKind {
    icon,
    cursor,
    unknown,
}

/// 6 字节小端 `ICONDIR` 文件头
pub struct IcoHeader {
    kind: IcoKind,
    count: u16,
}

/// 16 字节 `ICONDIRENTRY` 目录项（宽/高字节值 `0` 已规范解码为 `256`）
pub struct IcoEntry {
    width: u16,
    height: u16,
    color_count: u8,
    planes_or_hotspot_x: u16,
    bpp_or_hotspot_y: u16,
    bytes_in_res: u32,
    image_offset: u32,
    is_png: bool,

    /// 带溢出与边界检查返回该条目对应的原始 PNG/BMP 图像负载切片
    fn payload(data: []const u8): ([]const u8)? {
        const off = usize(_.image_offset);
        const size = usize(_.bytes_in_res);
        if (off > data.len || size > data.len - off) {
            return null;
        }
        return data[off .. off + size];
    }
}

fn readU16Le(data: []const u8, pos: usize): u16 {
    return u16(data[pos]) | (u16(data[pos + 1]) << 8);
}

fn readU32Le(data: []const u8, pos: usize): u32 {
    return u32(data[pos])
        | (u32(data[pos + 1]) << 8)
        | (u32(data[pos + 2]) << 16)
        | (u32(data[pos + 3]) << 24);
}

fn hasPngSignature(slice: []const u8): bool {
    if (slice.len < 8) return false;
    return slice[0] == 0x89
        && slice[1] == 0x50
        && slice[2] == 0x4E
        && slice[3] == 0x47
        && slice[4] == 0x0D
        && slice[5] == 0x0A
        && slice[6] == 0x1A
        && slice[7] == 0x0A;
}

fn absDiffU16(a: u16, b: u16): u16 {
    return if (a >= b) a - b else b - a;
}

/// 解析 6 字节小端 `ICONDIR` 头（要求 `reserved == 0` 且 `type` 为 `1=icon` 或 `2=cursor`）
pub fn parseIcoHeader(data: []const u8): IcoHeader? {
    if (data.len < 6) {
        return null;
    }
    const reserved = readU16Le(data, 0);
    if (reserved != 0) {
        return null;
    }
    const raw_type = readU16Le(data, 2);
    let kind = IcoKind.unknown;
    if (raw_type == 1) {
        kind = IcoKind.icon;
    } else if (raw_type == 2) {
        kind = IcoKind.cursor;
    } else {
        return null;
    }
    const count = readU16Le(data, 4);
    return IcoHeader {
        kind: kind,
        count: count,
    };
}

/// 快速魔数检测输入是否为合法 Windows ICO / CUR 容器头
pub fn isIcoOrCur(data: []const u8): bool {
    return parseIcoHeader(data) != null;
}

/// 零堆分配 ICO / CUR 目录项流式迭代器（严格校验目录表与负载边界不越界）
pub struct IcoEntryIter {
    data: []const u8,
    count: u16 = 0,
    index: u16 = 0,

    static fn init(data: []const u8): IcoEntryIter {
        if (hdr := parseIcoHeader(data)) {
            const dir_bytes = usize(hdr.count) * 16;
            if (dir_bytes <= data.len - 6) {
                return IcoEntryIter {
                    data: data,
                    count: hdr.count,
                    index: 0,
                };
            }
        }
        return IcoEntryIter {
            data: data,
            count: 0,
            index: 0,
        };
    }

    /// 读取下一个合法 `IcoEntry`；遍历结束或遇到越界负载时安全终止并返回 `null`
    fn next(*_): IcoEntry? {
        if (_.index >= _.count) {
            return null;
        }
        const dir_off = 6 + usize(_.index) * 16;
        if (dir_off + 16 > _.data.len) {
            _.index = _.count;
            return null;
        }

        const raw_w = _.data[dir_off];
        const raw_h = _.data[dir_off + 1];
        const width: u16 = if (raw_w == 0) 256 else u16(raw_w);
        const height: u16 = if (raw_h == 0) 256 else u16(raw_h);
        const color_count = _.data[dir_off + 2];
        const planes_or_hotspot_x = readU16Le(_.data, dir_off + 4);
        const bpp_or_hotspot_y = readU16Le(_.data, dir_off + 6);
        const bytes_in_res = readU32Le(_.data, dir_off + 8);
        const image_offset = readU32Le(_.data, dir_off + 12);

        const off_usize = usize(image_offset);
        const len_usize = usize(bytes_in_res);
        if (off_usize > _.data.len || len_usize > _.data.len - off_usize) {
            _.index = _.count;
            return null;
        }

        const slice = _.data[off_usize .. off_usize + len_usize];
        const is_png = hasPngSignature(slice);
        _.index += 1;

        return IcoEntry {
            width: width,
            height: height,
            color_count: color_count,
            planes_or_hotspot_x: planes_or_hotspot_x,
            bpp_or_hotspot_y: bpp_or_hotspot_y,
            bytes_in_res: bytes_in_res,
            image_offset: image_offset,
            is_png: is_png,
        };
    }
}

fn isBetterCandidate(cand: IcoEntry, cur: IcoEntry, target_size: u16): bool {
    const c_size: u16 = if (cand.width >= cand.height) cand.width else cand.height;
    const cur_size: u16 = if (cur.width >= cur.height) cur.width else cur.height;
    const c_dist = absDiffU16(c_size, target_size);
    const cur_dist = absDiffU16(cur_size, target_size);

    if (c_dist != cur_dist) {
        return c_dist < cur_dist;
    }
    // 等距时优先尺寸更大（降采样清晰度优于放大）
    if (c_size != cur_size) {
        return c_size > cur_size;
    }
    // 同尺寸优先 PNG 负载
    if (cand.is_png != cur.is_png) {
        return cand.is_png;
    }
    // 再优先更高色深（bpp）
    if (cand.bpp_or_hotspot_y != cur.bpp_or_hotspot_y) {
        return cand.bpp_or_hotspot_y > cur.bpp_or_hotspot_y;
    }
    return cand.bytes_in_res > cur.bytes_in_res;
}

/// 按最接近 `target_size` 且同尺寸优先 PNG / 高色深选出最佳图标条目
pub fn findBestIcoEntry(data: []const u8, target_size: u16): IcoEntry? {
    let it = IcoEntryIter.init(data);
    let best: IcoEntry? = null;
    while (entry := it.next()) {
        if (cur := best) {
            if (isBetterCandidate(entry, cur, target_size)) {
                best = entry;
            }
        } else {
            best = entry;
        }
    }
    return best;
}
