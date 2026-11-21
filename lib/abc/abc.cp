// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ABC Music Notation v2.1 Parser & Event Lexer)

/// ABC 分数（拍号 `M:6/8`、默认音长 `L:1/8` 与音符时值倍率 `3/2`）
pub struct AbcFraction {
    num: u32 = 1,
    den: u32 = 1,
}

/// ABC v2.1 乐谱头零拷贝视图（`X:` 起始，`K:` 结束，`body_offset` 指向正文起始）
pub struct AbcHeader {
    reference_num: u32 = 0,
    title: []const u8 = "",
    composer: []const u8 = "",
    meter: AbcFraction = AbcFraction { num: 4, den: 4 },
    unit_note_len: AbcFraction = AbcFraction { num: 1, den: 8 },
    tempo_bpm: u32 = 0,
    key: []const u8 = "",
    body_offset: usize = 0,
}

/// ABC 乐谱正文词法事件类型
pub enum AbcEventKind: u8 {
    note = 0,
    rest = 1,
    barline = 2,
    chord_start = 3,
    chord_end = 4,
    tie = 5,
    slur_start = 6,
    slur_end = 7,
}

/// 零拷贝 ABC 音符 / 休止符 / 小节线 / 和弦事件
pub struct AbcEvent {
    kind: AbcEventKind,
    pitch_letter: u8 = 0,
    accidental: i8 = 0,
    octave_shift: i8 = 0,
    midi_note: u8? = null,
    dur_num: u32 = 1,
    dur_den: u32 = 1,
    bar_text: []const u8 = "",
}

/// ABC 乐谱统计汇总
pub struct AbcStats {
    note_count: usize = 0,
    rest_count: usize = 0,
    bar_count: usize = 0,
    chord_count: usize = 0,
    min_midi: u8 = 0,
    max_midi: u8 = 0,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiLetter(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isPitchLetter(c: u8): bool {
    return (c >= 'A' && c <= 'G') || (c >= 'a' && c <= 'g');
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimLine(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && (isSpace(s[end - 1]) || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

fn stripLineComment(line: []const u8): []const u8 {
    let i: usize = 0;
    let in_quote = false;
    while (i < line.len) {
        if (line[i] == '"') {
            in_quote = !in_quote;
        } else if (!in_quote && line[i] == '%') {
            return trimLine(line[0..i]);
        }
        i += 1;
    }
    return trimLine(line);
}

fn parseU32Prefix(s: []const u8): u32? {
    if (s.len == 0 || !isDigit(s[0])) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        val = val * 10 + u64(s[i] - '0');
        if (val > 4294967295) return null;
        i += 1;
    }
    return val as u32;
}

fn parseFractionStr(s: []const u8): AbcFraction? {
    let slash: usize = 0;
    let found = false;
    while (slash < s.len) {
        if (s[slash] == '/') {
            found = true;
            break;
        }
        slash += 1;
    }
    if (!found) return null;
    const n = parseU32Prefix(trimLine(s[0..slash])) ?? return null;
    const d = parseU32Prefix(trimLine(s[slash + 1 .. s.len])) ?? return null;
    if (d == 0) return null;
    return AbcFraction { num: n, den: d };
}

fn parseMeter(s: []const u8): AbcFraction? {
    if (strEql(s, "C")) return AbcFraction { num: 4, den: 4 };
    if (strEql(s, "C|")) return AbcFraction { num: 2, den: 2 };
    return parseFractionStr(s);
}

fn parseTempo(s: []const u8): u32? {
    // 支持 `Q:120` 与 `Q:1/4=120`（或 `Q:"Allegro" 1/4=120`）
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '=') {
            return parseU32Prefix(trimLine(s[i + 1 .. s.len]));
        }
        i += 1;
    }
    return parseU32Prefix(s);
}

fn pitchToMidi(letter: u8, accidental: i8, octave_shift: i8): u8? {
    let base: i32 = 60;
    if (letter == 'C') base = 60;
    else if (letter == 'D') base = 62;
    else if (letter == 'E') base = 64;
    else if (letter == 'F') base = 65;
    else if (letter == 'G') base = 67;
    else if (letter == 'A') base = 69;
    else if (letter == 'B') base = 71;
    else if (letter == 'c') base = 72;
    else if (letter == 'd') base = 74;
    else if (letter == 'e') base = 76;
    else if (letter == 'f') base = 77;
    else if (letter == 'g') base = 79;
    else if (letter == 'a') base = 81;
    else if (letter == 'b') base = 83;
    else return null;

    const midi: i32 = base + i32(accidental) + i32(octave_shift) * 12;
    if (midi < 0 || midi > 127) return null;
    return midi as u8;
}

/// 解析 ABC v2.1 乐谱头（要求包含 `X:` 与 `K:` 字段，返回 `body_offset` 指向正文起始）
pub fn parseAbcHeader(data: []const u8): AbcHeader? {
    if (data.len == 0) return null;

    let hdr = AbcHeader {};
    let has_x = false;
    let has_k = false;
    let pos: usize = 0;

    // 跳过可选 UTF-8 BOM
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        pos = 3;
    }

    while (pos < data.len) {
        const line_start = pos;
        while (pos < data.len && data[pos] != '\n') {
            pos += 1;
        }
        const line_end = pos;
        if (pos < data.len && data[pos] == '\n') {
            pos += 1;
        }

        const raw_line = trimLine(data[line_start..line_end]);
        if (raw_line.len == 0) {
            continue;
        }
        // 跳过 `%` 注释与 `%%` 指令
        if (raw_line[0] == '%') {
            continue;
        }

        const clean = stripLineComment(raw_line);
        if (clean.len < 2 || clean[1] != ':' || !isAsciiLetter(clean[0])) {
            // 尚未见到 `X:` 就遇到非头字段行 -> 非法 ABC
            if (!has_x) return null;
            continue;
        }

        const tag = clean[0];
        const val = trimLine(clean[2..clean.len]);

        if (tag == 'X') {
            const ref_num = parseU32Prefix(val) ?? return null;
            hdr.reference_num = ref_num;
            has_x = true;
        } else if (tag == 'T') {
            if (hdr.title.len == 0) {
                hdr.title = val;
            }
        } else if (tag == 'C') {
            if (hdr.composer.len == 0) {
                hdr.composer = val;
            }
        } else if (tag == 'M') {
            if (m := parseMeter(val)) {
                hdr.meter = m;
            }
        } else if (tag == 'L') {
            if (l := parseFractionStr(val)) {
                hdr.unit_note_len = l;
            }
        } else if (tag == 'Q') {
            if (q := parseTempo(val)) {
                hdr.tempo_bpm = q;
            }
        } else if (tag == 'K') {
            if (!has_x) return null;
            hdr.key = val;
            hdr.body_offset = pos;
            has_k = true;
            break;
        }
    }

    if (!has_x || !has_k) return null;
    return hdr;
}

/// 判定输入是否为合法 ABC v2.1 乐谱（包含完整 `X:` .. `K:` 头）
pub fn isAbc(data: []const u8): bool {
    return parseAbcHeader(data) != null;
}

/// 零堆分配 ABC 乐谱正文事件词法迭代器
pub struct AbcEventIter {
    data: []const u8,
    pos: usize,
    at_line_start: bool = true,

    /// 从完整 ABC 文档初始化（自动定位 `K:` 之后的 `body_offset`；非法头部返回 `null`）
    pub static fn init(data: []const u8): AbcEventIter? {
        const hdr = parseAbcHeader(data) ?? return null;
        return AbcEventIter {
            data: data,
            pos: hdr.body_offset,
            at_line_start: true,
        };
    }

    /// 直接从乐谱正文切片初始化
    pub static fn fromBody(body: []const u8): AbcEventIter {
        return AbcEventIter {
            data: body,
            pos: 0,
            at_line_start: true,
        };
    }

    fn parseDuration(*_): AbcFraction {
        let num: u32 = 1;
        let den: u32 = 1;

        if (_.pos < _.data.len && isDigit(_.data[_.pos])) {
            let n: u64 = 0;
            while (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                if (n < 1000000) {
                    n = n * 10 + u64(_.data[_.pos] - '0');
                }
                _.pos += 1;
            }
            if (n > 0) num = n as u32;
        }

        if (_.pos < _.data.len && _.data[_.pos] == '/') {
            _.pos += 1;
            if (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                let d: u64 = 0;
                while (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                    if (d < 1000000) {
                        d = d * 10 + u64(_.data[_.pos] - '0');
                    }
                    _.pos += 1;
                }
                if (d > 0) den = d as u32;
            } else {
                den = 2;
                while (_.pos < _.data.len && _.data[_.pos] == '/') {
                    if (den < 1000000) {
                        den = den * 2;
                    }
                    _.pos += 1;
                }
            }
        }

        return AbcFraction { num: num, den: den };
    }

    pub fn next(*_): AbcEvent? {
        while (_.pos < _.data.len) {
            const c = _.data[_.pos];

            // 空白与换行
            if (c == '\n') {
                _.pos += 1;
                _.at_line_start = true;
                continue;
            }
            if (c == ' ' || c == '\t' || c == '\r' || c == '\\') {
                _.pos += 1;
                continue;
            }

            // `%` 注释跳过至行尾
            if (c == '%') {
                while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }

            // 行首独立信息字段（如 `V:1`, `w:lyrics`, `L:1/16`）跳过整行
            if (_.at_line_start && isAsciiLetter(c) && _.pos + 1 < _.data.len && _.data[_.pos + 1] == ':') {
                while (_.pos < _.data.len && _.data[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            _.at_line_start = false;

            // `"..."` 吉他和弦 / 文本标注跳过
            if (c == '"') {
                _.pos += 1;
                while (_.pos < _.data.len && _.data[_.pos] != '"' && _.data[_.pos] != '\n') {
                    _.pos += 1;
                }
                if (_.pos < _.data.len && _.data[_.pos] == '"') {
                    _.pos += 1;
                }
                continue;
            }

            // `!trill!` 或 `+fermata+` 装饰符跳过
            if (c == '!' || c == '+') {
                const delim = c;
                _.pos += 1;
                while (_.pos < _.data.len && _.data[_.pos] != delim && _.data[_.pos] != '\n') {
                    _.pos += 1;
                }
                if (_.pos < _.data.len && _.data[_.pos] == delim) {
                    _.pos += 1;
                }
                continue;
            }

            // `[`：区分行内字段 `[M:3/4]`、小节线 `[|` 与和弦起始 `[`
            if (c == '[') {
                // 行内信息字段 `[A:...]`
                if (_.pos + 2 < _.data.len && isAsciiLetter(_.data[_.pos + 1]) && _.data[_.pos + 2] == ':') {
                    _.pos += 3;
                    while (_.pos < _.data.len && _.data[_.pos] != ']' && _.data[_.pos] != '\n') {
                        _.pos += 1;
                    }
                    if (_.pos < _.data.len && _.data[_.pos] == ']') {
                        _.pos += 1;
                    }
                    continue;
                }
                // `[|` 小节线
                if (_.pos + 1 < _.data.len && _.data[_.pos + 1] == '|') {
                    const bstart = _.pos;
                    _.pos += 2;
                    return AbcEvent {
                        kind: AbcEventKind.barline,
                        bar_text: _.data[bstart.._.pos],
                    };
                }
                _.pos += 1;
                return AbcEvent { kind: AbcEventKind.chord_start };
            }

            // `]` 和弦结束（附可选和弦整体时值倍率如 `[CEG]2`）
            if (c == ']') {
                _.pos += 1;
                const dur = _.parseDuration();
                return AbcEvent {
                    kind: AbcEventKind.chord_end,
                    dur_num: dur.num,
                    dur_den: dur.den,
                };
            }

            // 小节线 `|`, `||`, `|:`, `|]`, `:|`, `::`
            if (c == '|' || (c == ':' && _.pos + 1 < _.data.len && (_.data[_.pos + 1] == '|' || _.data[_.pos + 1] == ':'))) {
                const bstart = _.pos;
                _.pos += 1;
                while (_.pos < _.data.len) {
                    const nc = _.data[_.pos];
                    if (nc == '|' || nc == ':' || nc == ']') {
                        _.pos += 1;
                    } else {
                        break;
                    }
                }
                return AbcEvent {
                    kind: AbcEventKind.barline,
                    bar_text: _.data[bstart.._.pos],
                };
            }

            // 连音线 `-`
            if (c == '-') {
                _.pos += 1;
                return AbcEvent { kind: AbcEventKind.tie };
            }

            // 圆滑线 `(` / `)`（跳过连音符前缀 `(3`）
            if (c == '(') {
                _.pos += 1;
                if (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                    while (_.pos < _.data.len && (isDigit(_.data[_.pos]) || _.data[_.pos] == ':')) {
                        _.pos += 1;
                    }
                    continue;
                }
                return AbcEvent { kind: AbcEventKind.slur_start };
            }
            if (c == ')') {
                _.pos += 1;
                return AbcEvent { kind: AbcEventKind.slur_end };
            }

            // 休止符 `z`, `Z`, `x`, `X`
            if (c == 'z' || c == 'Z' || c == 'x' || c == 'X') {
                _.pos += 1;
                const dur = _.parseDuration();
                return AbcEvent {
                    kind: AbcEventKind.rest,
                    pitch_letter: c,
                    dur_num: dur.num,
                    dur_den: dur.den,
                };
            }

            // 音符：可选临时升降号 (`^^`, `^`, `__`, `_`, `=`) + 音名 (`A..G` / `a..g`) + 八度 (`'` / `,`) + 时值
            if (c == '^' || c == '_' || c == '=' || isPitchLetter(c)) {
                const saved = _.pos;
                let acc: i8 = 0;
                if (c == '^') {
                    _.pos += 1;
                    acc = 1;
                    if (_.pos < _.data.len && _.data[_.pos] == '^') {
                        _.pos += 1;
                        acc = 2;
                    }
                } else if (c == '_') {
                    _.pos += 1;
                    acc = -1;
                    if (_.pos < _.data.len && _.data[_.pos] == '_') {
                        _.pos += 1;
                        acc = -2;
                    }
                } else if (c == '=') {
                    _.pos += 1;
                    acc = 0;
                }

                if (_.pos < _.data.len && isPitchLetter(_.data[_.pos])) {
                    const letter = _.data[_.pos];
                    _.pos += 1;

                    let oct: i8 = 0;
                    while (_.pos < _.data.len) {
                        if (_.data[_.pos] == '\'') {
                            if (oct < 10) oct += 1;
                            _.pos += 1;
                        } else if (_.data[_.pos] == ',') {
                            if (oct > -10) oct -= 1;
                            _.pos += 1;
                        } else {
                            break;
                        }
                    }

                    const dur = _.parseDuration();
                    const midi = pitchToMidi(letter, acc, oct);
                    return AbcEvent {
                        kind: AbcEventKind.note,
                        pitch_letter: letter,
                        accidental: acc,
                        octave_shift: oct,
                        midi_note: midi,
                        dur_num: dur.num,
                        dur_den: dur.den,
                    };
                }

                // 孤立升降号字符，安全跳过 1 字节继续
                _.pos = saved + 1;
                continue;
            }

            // 其余装饰/节奏符号（如 `>`, `<`, `{`, `}`, `~`, `.`）直接跳过
            _.pos += 1;
        }
        return null;
    }
}

/// 统计 ABC 乐谱的音符数、休止符数、小节线数、和弦数与 MIDI 音域 `[min_midi, max_midi]`
pub fn summarizeAbc(data: []const u8): AbcStats {
    let stats = AbcStats {};
    let it = AbcEventIter.init(data) ?? return stats;
    let has_midi = false;

    while (ev := it.next()) {
        if (ev.kind == AbcEventKind.note) {
            stats.note_count += 1;
            if (m := ev.midi_note) {
                if (!has_midi) {
                    stats.min_midi = m;
                    stats.max_midi = m;
                    has_midi = true;
                } else {
                    if (m < stats.min_midi) stats.min_midi = m;
                    if (m > stats.max_midi) stats.max_midi = m;
                }
            }
        } else if (ev.kind == AbcEventKind.rest) {
            stats.rest_count += 1;
        } else if (ev.kind == AbcEventKind.barline) {
            stats.bar_count += 1;
        } else if (ev.kind == AbcEventKind.chord_start) {
            stats.chord_count += 1;
        }
    }

    return stats;
}
