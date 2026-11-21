// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc IEEE 1364 VCD Waveform Parser)

/// VCD 时间单位枚举
pub enum VcdTimeUnit: u8 {
    unknown = 0,
    s = 1,
    ms = 2,
    us = 3,
    ns = 4,
    ps = 5,
    fs = 6,
}

/// VCD 信号变量类型枚举
pub enum VcdVarType: u8 {
    unknown = 0,
    wire = 1,
    reg = 2,
    integer = 3,
    real = 4,
    parameter = 5,
    event = 6,
    supply0 = 7,
    supply1 = 8,
    tri = 9,
}

/// VCD 波形体值变流事件类型
pub enum VcdEventKind: u8 {
    timestamp = 0,
    scalar_change = 1,
    vector_change = 2,
    real_change = 3,
    dumpvars = 4,
    dumpon = 5,
    dumpoff = 6,
    dumpall = 7,
    end = 8,
}

/// VCD 时间精度（`magnitude` 仅允许 1 / 10 / 100）
pub struct VcdTimescale {
    magnitude: u32,
    unit: VcdTimeUnit,
}

/// VCD 信号定义（`$var <type> <size> <id> <ref> $end`）
pub struct VcdVarDef {
    var_type: VcdVarType,
    width: u32,
    id_code: []const u8,
    reference: []const u8,
}

/// VCD 文件头摘要与波形体起始偏移
pub struct VcdHeader {
    date: []const u8,
    version: []const u8,
    timescale: VcdTimescale,
    var_count: u32,
    body_offset: usize,
}

/// VCD 波形体值变事件
pub struct VcdEvent {
    kind: VcdEventKind,
    timestamp: u64,
    scalar_val: u8,
    id_code: []const u8,
    value: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
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

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = (c - '0') as u64;
        if (acc > 1844674407370955161) return null;
        if (acc == 1844674407370955161 && d > 5) return null;
        acc = acc * 10 + d;
        i += 1;
    }
    return acc;
}

fn parseU32Dec(s: []const u8): u32? {
    if (v := parseU64Dec(s)) {
        if (v > 0xFFFFFFFF) return null;
        return v as u32;
    }
    return null;
}

fn decodeVarType(tok: []const u8): VcdVarType {
    if (strEq(tok, "wire")) return VcdVarType.wire;
    if (strEq(tok, "reg")) return VcdVarType.reg;
    if (strEq(tok, "integer")) return VcdVarType.integer;
    if (strEq(tok, "real")) return VcdVarType.real;
    if (strEq(tok, "parameter")) return VcdVarType.parameter;
    if (strEq(tok, "event")) return VcdVarType.event;
    if (strEq(tok, "supply0")) return VcdVarType.supply0;
    if (strEq(tok, "supply1")) return VcdVarType.supply1;
    if (strEq(tok, "tri")) return VcdVarType.tri;
    return VcdVarType.unknown;
}

fn nextToken(data: []const u8, pos: *usize): ([]const u8)? {
    let p = pos.*;
    while (p < data.len && isSpace(data[p])) {
        p += 1;
    }
    if (p >= data.len) {
        pos.* = p;
        return null;
    }
    const start = p;
    while (p < data.len && !isSpace(data[p])) {
        p += 1;
    }
    pos.* = p;
    return data[start..p];
}

/// 读取直到 `$end` 为止的原始内容切片（已去首尾空白）
fn readUntilEnd(data: []const u8, pos: *usize): ([]const u8)? {
    const content_start = pos.*;
    while (true) {
        const before_tok = pos.*;
        if (tok := nextToken(data, pos)) {
            if (strEq(tok, "$end")) {
                return trimSpaces(data[content_start..before_tok]);
            }
        } else {
            return null;
        }
    }
}

/// 解析 VCD `$timescale` 文本（如 `"1ns"`, `"10 ps"`, `"100fs"`）
pub fn parseVcdTimescale(text: []const u8): VcdTimescale? {
    const s = trimSpaces(text);
    if (s.len == 0) return null;

    let i: usize = 0;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        i += 1;
    }
    if (i == 0) return null;

    let mag: u32 = 0;
    if (m := parseU32Dec(s[0..i])) {
        if (m != 1 && m != 10 && m != 100) return null;
        mag = m;
    } else {
        return null;
    }

    const unit_str = trimSpaces(s[i..s.len]);
    let unit = VcdTimeUnit.unknown;
    if (strEq(unit_str, "s")) {
        unit = VcdTimeUnit.s;
    } else if (strEq(unit_str, "ms")) {
        unit = VcdTimeUnit.ms;
    } else if (strEq(unit_str, "us")) {
        unit = VcdTimeUnit.us;
    } else if (strEq(unit_str, "ns")) {
        unit = VcdTimeUnit.ns;
    } else if (strEq(unit_str, "ps")) {
        unit = VcdTimeUnit.ps;
    } else if (strEq(unit_str, "fs")) {
        unit = VcdTimeUnit.fs;
    } else {
        return null;
    }

    return VcdTimescale { magnitude: mag, unit: unit };
}

fn parseVarCommand(data: []const u8, pos: *usize): VcdVarDef? {
    let type_tok: []const u8 = "";
    if (t := nextToken(data, pos)) {
        type_tok = t;
    } else {
        return null;
    }

    let size_tok: []const u8 = "";
    if (t := nextToken(data, pos)) {
        size_tok = t;
    } else {
        return null;
    }

    let id_tok: []const u8 = "";
    if (t := nextToken(data, pos)) {
        id_tok = t;
    } else {
        return null;
    }

    let width: u32 = 0;
    if (w := parseU32Dec(size_tok)) {
        width = w;
    } else {
        return null;
    }

    let ref_str: []const u8 = "";
    if (r := readUntilEnd(data, pos)) {
        if (r.len == 0) return null;
        ref_str = r;
    } else {
        return null;
    }

    return VcdVarDef {
        var_type: decodeVarType(type_tok),
        width: width,
        id_code: id_tok,
        reference: ref_str,
    };
}

/// 解析 VCD 文件头（`$date`/`$version`/`$timescale`/`$scope`/`$var` 直至 `$enddefinitions $end`）
pub fn parseVcdHeader(data: []const u8): VcdHeader? {
    let pos: usize = 0;
    let date: []const u8 = "";
    let version: []const u8 = "";
    let timescale = VcdTimescale { magnitude: 0, unit: VcdTimeUnit.unknown };
    let var_count: u32 = 0;

    while (tok := nextToken(data, *pos)) {
        if (strEq(tok, "$date")) {
            if (txt := readUntilEnd(data, *pos)) {
                date = txt;
            } else {
                return null;
            }
        } else if (strEq(tok, "$version")) {
            if (txt := readUntilEnd(data, *pos)) {
                version = txt;
            } else {
                return null;
            }
        } else if (strEq(tok, "$timescale")) {
            if (txt := readUntilEnd(data, *pos)) {
                if (ts := parseVcdTimescale(txt)) {
                    timescale = ts;
                } else {
                    return null;
                }
            } else {
                return null;
            }
        } else if (strEq(tok, "$var")) {
            if (parseVarCommand(data, *pos) == null) return null;
            var_count += 1;
        } else if (strEq(tok, "$scope") || strEq(tok, "$upscope") || strEq(tok, "$comment")) {
            if (readUntilEnd(data, *pos) == null) return null;
        } else if (strEq(tok, "$enddefinitions")) {
            if (readUntilEnd(data, *pos) == null) return null;
            return VcdHeader {
                date: date,
                version: version,
                timescale: timescale,
                var_count: var_count,
                body_offset: pos,
            };
        } else {
            return null;
        }
    }

    // 缺失 $enddefinitions
    return null;
}

/// 零堆分配 VCD 信号定义（`$var`）迭代器
pub struct VcdVarIter {
    data: []const u8,
    pos: usize = 0,
    done: bool = false,
    had_error: bool = false,

    static fn init(data: []const u8): VcdVarIter {
        return VcdVarIter {
            data: data,
            pos: 0,
            done: false,
            had_error: false,
        };
    }

    fn next(*_): VcdVarDef? {
        if (_.done || _.had_error) return null;

        let p = _.pos;
        while (tok := nextToken(_.data, *p)) {
            if (strEq(tok, "$var")) {
                if (v := parseVarCommand(_.data, *p)) {
                    _.pos = p;
                    return v;
                }
                _.pos = p;
                _.had_error = true;
                return null;
            } else if (strEq(tok, "$enddefinitions")) {
                if (readUntilEnd(_.data, *p) == null) {
                    _.had_error = true;
                }
                _.pos = p;
                _.done = true;
                return null;
            } else if (tok.len > 0 && tok[0] == '$') {
                if (readUntilEnd(_.data, *p) == null) {
                    _.pos = p;
                    _.had_error = true;
                    return null;
                }
            } else {
                _.pos = p;
                _.had_error = true;
                return null;
            }
        }

        _.pos = p;
        _.done = true;
        return null;
    }
}

fn isScalarChar(c: u8): bool {
    return c == '0' || c == '1' || c == 'x' || c == 'X' || c == 'z' || c == 'Z';
}

/// 零堆分配 VCD 波形值变流迭代器
pub struct VcdValueIter {
    data: []const u8,
    pos: usize,
    had_error: bool = false,

    static fn init(data: []const u8, body_offset: usize): VcdValueIter {
        if (body_offset > data.len) {
            return VcdValueIter {
                data: data,
                pos: data.len,
                had_error: true,
            };
        }
        return VcdValueIter {
            data: data,
            pos: body_offset,
            had_error: false,
        };
    }

    static fn fromHeader(data: []const u8, header: VcdHeader): VcdValueIter {
        return VcdValueIter.init(data, header.body_offset);
    }

    fn next(*_): VcdEvent? {
        if (_.had_error) return null;

        let p = _.pos;
        while (tok := nextToken(_.data, *p)) {
            const first = tok[0];

            // 1. Timestamp: #<u64>
            if (first == '#') {
                if (ts := parseU64Dec(tok[1..tok.len])) {
                    _.pos = p;
                    return VcdEvent {
                        kind: VcdEventKind.timestamp,
                        timestamp: ts,
                        scalar_val: 0,
                        id_code: "",
                        value: "",
                    };
                }
                _.pos = p;
                _.had_error = true;
                return null;
            }

            // 2. Simulation control commands
            if (first == '$') {
                if (strEq(tok, "$dumpvars")) {
                    _.pos = p;
                    return VcdEvent { kind: VcdEventKind.dumpvars, timestamp: 0, scalar_val: 0, id_code: "", value: "" };
                }
                if (strEq(tok, "$dumpon")) {
                    _.pos = p;
                    return VcdEvent { kind: VcdEventKind.dumpon, timestamp: 0, scalar_val: 0, id_code: "", value: "" };
                }
                if (strEq(tok, "$dumpoff")) {
                    _.pos = p;
                    return VcdEvent { kind: VcdEventKind.dumpoff, timestamp: 0, scalar_val: 0, id_code: "", value: "" };
                }
                if (strEq(tok, "$dumpall")) {
                    _.pos = p;
                    return VcdEvent { kind: VcdEventKind.dumpall, timestamp: 0, scalar_val: 0, id_code: "", value: "" };
                }
                if (strEq(tok, "$end")) {
                    _.pos = p;
                    return VcdEvent { kind: VcdEventKind.end, timestamp: 0, scalar_val: 0, id_code: "", value: "" };
                }
                if (strEq(tok, "$comment")) {
                    if (readUntilEnd(_.data, *p) == null) {
                        _.pos = p;
                        _.had_error = true;
                        return null;
                    }
                    _.pos = p;
                    continue;
                }
                _.pos = p;
                _.had_error = true;
                return null;
            }

            // 3. Vector change: b<bits> <id_code>
            if (first == 'b' || first == 'B') {
                if (tok.len < 2) {
                    _.pos = p;
                    _.had_error = true;
                    return null;
                }
                if (id_tok := nextToken(_.data, *p)) {
                    _.pos = p;
                    return VcdEvent {
                        kind: VcdEventKind.vector_change,
                        timestamp: 0,
                        scalar_val: 0,
                        id_code: id_tok,
                        value: tok[1..tok.len],
                    };
                }
                _.pos = p;
                _.had_error = true;
                return null;
            }

            // 4. Real change: r<float_text> <id_code>
            if (first == 'r' || first == 'R') {
                if (tok.len < 2) {
                    _.pos = p;
                    _.had_error = true;
                    return null;
                }
                if (id_tok := nextToken(_.data, *p)) {
                    _.pos = p;
                    return VcdEvent {
                        kind: VcdEventKind.real_change,
                        timestamp: 0,
                        scalar_val: 0,
                        id_code: id_tok,
                        value: tok[1..tok.len],
                    };
                }
                _.pos = p;
                _.had_error = true;
                return null;
            }

            // 5. Scalar change: [01xXzZ]<id_code>
            if (isScalarChar(first)) {
                if (tok.len < 2) {
                    _.pos = p;
                    _.had_error = true;
                    return null;
                }
                _.pos = p;
                return VcdEvent {
                    kind: VcdEventKind.scalar_change,
                    timestamp: 0,
                    scalar_val: first,
                    id_code: tok[1..tok.len],
                    value: "",
                };
            }

            _.pos = p;
            _.had_error = true;
            return null;
        }

        _.pos = p;
        return null;
    }
}
