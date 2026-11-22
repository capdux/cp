// Origin: Pure-CP Stdlib (Zero-Alloc JVM HPROF 1.0.1/1.0.2 Heap Dump Parser) | Created: 2026-11-22

/// JVM HPROF 顶层 Record Tag 常量
pub const TAG_UTF8: u8 = 0x01;
pub const TAG_LOAD_CLASS: u8 = 0x02;
pub const TAG_UNLOAD_CLASS: u8 = 0x03;
pub const TAG_STACK_FRAME: u8 = 0x04;
pub const TAG_STACK_TRACE: u8 = 0x05;
pub const TAG_ALLOC_SITES: u8 = 0x06;
pub const TAG_HEAP_SUMMARY: u8 = 0x07;
pub const TAG_START_THREAD: u8 = 0x0A;
pub const TAG_END_THREAD: u8 = 0x0B;
pub const TAG_HEAP_DUMP: u8 = 0x0C;
pub const TAG_CPU_SAMPLES: u8 = 0x0D;
pub const TAG_CONTROL_SETTINGS: u8 = 0x0E;
pub const TAG_HEAP_DUMP_SEGMENT: u8 = 0x1C;
pub const TAG_HEAP_DUMP_END: u8 = 0x2C;

/// JVM HPROF Heap Dump 子记录 Tag 常量
pub const SUB_ROOT_UNKNOWN: u8 = 0xFF;
pub const SUB_ROOT_JNI_GLOBAL: u8 = 0x01;
pub const SUB_ROOT_JNI_LOCAL: u8 = 0x02;
pub const SUB_ROOT_JAVA_FRAME: u8 = 0x03;
pub const SUB_ROOT_STICKY_CLASS: u8 = 0x05;
pub const SUB_ROOT_THREAD_OBJ: u8 = 0x08;
pub const SUB_CLASS_DUMP: u8 = 0x20;
pub const SUB_INSTANCE_DUMP: u8 = 0x21;
pub const SUB_OBJ_ARRAY_DUMP: u8 = 0x22;
pub const SUB_PRIM_ARRAY_DUMP: u8 = 0x23;

/// Java 基本类型枚举
pub enum HprofBasicType: u8 {
    unknown = 0,
    object = 2,
    boolean = 4,
    char = 5,
    float = 6,
    double = 7,
    byte = 8,
    short = 9,
    int = 10,
    long = 11,
}

/// 将原始字节解码为 `HprofBasicType`
pub fn decodeHprofBasicType(code: u8): HprofBasicType {
    if (code == 2) return HprofBasicType.object;
    if (code == 4) return HprofBasicType.boolean;
    if (code == 5) return HprofBasicType.char;
    if (code == 6) return HprofBasicType.float;
    if (code == 7) return HprofBasicType.double;
    if (code == 8) return HprofBasicType.byte;
    if (code == 9) return HprofBasicType.short;
    if (code == 10) return HprofBasicType.int;
    if (code == 11) return HprofBasicType.long;
    return HprofBasicType.unknown;
}

/// 返回指定 Java 基本类型的字节宽度（`object` 依赖 `id_size` 仅允许 4 或 8）
pub fn hprofBasicTypeSize(t: HprofBasicType, id_size: u32): u8? {
    if (t == HprofBasicType.object) {
        if (id_size == 4 || id_size == 8) return id_size as u8;
        return null;
    }
    if (t == HprofBasicType.boolean || t == HprofBasicType.byte) return 1;
    if (t == HprofBasicType.char || t == HprofBasicType.short) return 2;
    if (t == HprofBasicType.float || t == HprofBasicType.int) return 4;
    if (t == HprofBasicType.double || t == HprofBasicType.long) return 8;
    return null;
}

/// HPROF 文件头解析结果
pub struct HprofHeader {
    format: []const u8,
    id_size: u32,
    timestamp_ms: u64,
    records_offset: usize,
}

/// HPROF 顶层记录零拷贝视图
pub struct HprofRecord {
    tag: u8,
    time_offset_us: u32,
    length: u32,
    body: []const u8,
}

/// `TAG_UTF8` (0x01) 记录解析结果
pub struct HprofUtf8 {
    id: u64,
    text: []const u8,
}

/// `TAG_LOAD_CLASS` (0x02) 记录解析结果
pub struct HprofLoadClass {
    class_serial: u32,
    class_obj_id: u64,
    stack_trace_serial: u32,
    class_name_id: u64,
}

/// `TAG_HEAP_SUMMARY` (0x07) 记录解析结果
pub struct HprofHeapSummary {
    total_live_bytes: u32,
    total_live_instances: u32,
    total_bytes_allocated: u64,
    total_instances_allocated: u64,
}

/// HPROF 单遍扫描统计结果
pub struct HprofStats {
    total_records: u64,
    utf8_count: u64,
    load_class_count: u64,
    heap_dump_segments: u64,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readU64BE(data: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(data, off) as u64;
    const lo: u64 = readU32BE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

fn readIdBE(data: []const u8, off: usize, id_size: u32): u64 {
    if (id_size == 4) return readU32BE(data, off) as u64;
    return readU64BE(data, off);
}

/// 校验字节切片是否以合法 JVM HPROF 1.0.1 / 1.0.2 文件头开头
pub fn isHprof(data: []const u8): bool {
    return parseHprofHeader(data) != null;
}

/// 解析 HPROF 文件头：`\0` 结尾 `"JAVA PROFILE 1.0.1"` / `"JAVA PROFILE 1.0.2"` + 4B `id_size` (4/8) + 8B `timestamp_ms`
pub fn parseHprofHeader(data: []const u8): HprofHeader? {
    // 最多在前 32 字节内查找 '\0' 终止符
    let nul_pos: usize = 0;
    let found = false;
    const scan_limit = if (data.len < 32) data.len else 32;
    while (nul_pos < scan_limit) {
        if (data[nul_pos] == 0) {
            found = true;
            break;
        }
        nul_pos += 1;
    }
    if (!found) return null;

    const fmt = data[0..nul_pos];
    if (!strEq(fmt, "JAVA PROFILE 1.0.1") && !strEq(fmt, "JAVA PROFILE 1.0.2")) {
        return null;
    }

    const after_nul = nul_pos + 1;
    if (data.len - after_nul < 12) return null;

    const id_size = readU32BE(data, after_nul);
    if (id_size != 4 && id_size != 8) return null;

    const timestamp_ms = readU64BE(data, after_nul + 4);
    return HprofHeader {
        format: fmt,
        id_size: id_size,
        timestamp_ms: timestamp_ms,
        records_offset: after_nul + 12,
    };
}

/// 零堆分配 HPROF 顶层记录流迭代器
pub struct HprofRecordIter {
    data: []const u8,
    pos: usize,
    had_error: bool = false,

    static fn init(data: []const u8, header: HprofHeader): HprofRecordIter {
        if (header.records_offset > data.len) {
            return HprofRecordIter {
                data: data,
                pos: data.len,
                had_error: true,
            };
        }
        return HprofRecordIter {
            data: data,
            pos: header.records_offset,
            had_error: false,
        };
    }

    static fn fromData(data: []const u8): HprofRecordIter {
        if (hdr := parseHprofHeader(data)) {
            return HprofRecordIter.init(data, hdr);
        }
        return HprofRecordIter {
            data: data,
            pos: data.len,
            had_error: true,
        };
    }

    fn next(*_): HprofRecord? {
        if (_.had_error || _.pos >= _.data.len) return null;
        if (_.data.len - _.pos < 9) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const off = _.pos;
        const tag = _.data[off];
        const time_offset_us = readU32BE(_.data, off + 1);
        const length = readU32BE(_.data, off + 5);

        const body_start = off + 9;
        if ((length as u64) > ((_.data.len - body_start) as u64)) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const len_usize = length as usize;
        const body = _.data[body_start .. body_start + len_usize];
        _.pos = body_start + len_usize;

        return HprofRecord {
            tag: tag,
            time_offset_us: time_offset_us,
            length: length,
            body: body,
        };
    }
}

/// 解析 `TAG_UTF8` (0x01) 记录体
pub fn parseHprofUtf8(body: []const u8, id_size: u32): HprofUtf8? {
    if (id_size != 4 && id_size != 8) return null;
    const id_len = id_size as usize;
    if (body.len < id_len) return null;

    return HprofUtf8 {
        id: readIdBE(body, 0, id_size),
        text: body[id_len..body.len],
    };
}

/// 解析 `TAG_LOAD_CLASS` (0x02) 记录体
pub fn parseHprofLoadClass(body: []const u8, id_size: u32): HprofLoadClass? {
    if (id_size != 4 && id_size != 8) return null;
    const id_len = id_size as usize;
    const need: usize = (8 as usize) + (2 as usize) * id_len;
    if (body.len < need) return null;

    const class_serial = readU32BE(body, 0);
    const class_obj_id = readIdBE(body, 4, id_size);
    const stack_trace_serial = readU32BE(body, 4 + id_len);
    const class_name_id = readIdBE(body, 8 + id_len, id_size);

    return HprofLoadClass {
        class_serial: class_serial,
        class_obj_id: class_obj_id,
        stack_trace_serial: stack_trace_serial,
        class_name_id: class_name_id,
    };
}

/// 解析 `TAG_HEAP_SUMMARY` (0x07) 记录体（固定 24 字节）
pub fn parseHprofHeapSummary(body: []const u8): HprofHeapSummary? {
    if (body.len < 24) return null;

    return HprofHeapSummary {
        total_live_bytes: readU32BE(body, 0),
        total_live_instances: readU32BE(body, 4),
        total_bytes_allocated: readU64BE(body, 8),
        total_instances_allocated: readU64BE(body, 16),
    };
}

/// 单遍扫描统计 HPROF 字符串数、类加载数、Heap Dump 段数与总记录数
pub fn summarizeHprof(data: []const u8): HprofStats? {
    let hdr = HprofHeader { format: "", id_size: 0, timestamp_ms: 0, records_offset: 0 };
    if (h := parseHprofHeader(data)) {
        hdr = h;
    } else {
        return null;
    }

    let it = HprofRecordIter.init(data, hdr);
    let stats = HprofStats {
        total_records: 0,
        utf8_count: 0,
        load_class_count: 0,
        heap_dump_segments: 0,
    };

    while (rec := it.next()) {
        stats.total_records += 1;
        if (rec.tag == TAG_UTF8) {
            stats.utf8_count += 1;
        } else if (rec.tag == TAG_LOAD_CLASS) {
            stats.load_class_count += 1;
        } else if (rec.tag == TAG_HEAP_DUMP || rec.tag == TAG_HEAP_DUMP_SEGMENT) {
            stats.heap_dump_segments += 1;
        }
    }

    if (it.had_error) return null;
    return stats;
}
