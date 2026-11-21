// Source: Absorbed from /data/dev/bun/src/cli.zig (Dockerfile Instruction Parser)

/// 单条解析后的 Dockerfile 指令
pub struct DockerInstruction {
    keyword: []const u8 = "",
    args: []const u8 = "",
    line_no: usize = 0,
}

/// `FROM` 指令解析结果（如 `--platform=linux/amd64 node:22-alpine AS builder`）
pub struct FromSpec {
    platform: ([]const u8)? = null,
    image: []const u8 = "",
    stage_alias: ([]const u8)? = null,
}

/// `EXPOSE` 端口与传输协议解析结果
pub struct ExposePort {
    port: u16 = 0,
    is_udp: bool = false,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isHorizSpace(s[start])) { start += 1; }
    while (end > start && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn trimLeftHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isHorizSpace(s[start])) { start += 1; }
    return s[start..s.len];
}

fn trimRightHoriz(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[0..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

fn parseEscapeDirective(comment_line: []const u8): u8? {
    // 格式: # escape=\ 或 # escape=`
    const body = trimHoriz(comment_line[1..comment_line.len]);
    let eq_idx: usize? = null;
    let i: usize = 0;
    while (i < body.len) {
        if (body[i] == '=') {
            eq_idx = i;
            break;
        }
        i += 1;
    }
    if (ei := eq_idx) {
        const key = trimHoriz(body[0..ei]);
        const val = trimHoriz(body[(ei + 1)..body.len]);
        if (eqlIgnoreCaseAscii(key, "escape") && val.len == 1) {
            if (val[0] == '\\' || val[0] == '`') {
                return val[0];
            }
        }
    }
    return null;
}

fn appendBytes(buf: []u8, pos: *usize, s: []const u8) {
    let i: usize = 0;
    while (i < s.len && pos.* < buf.len) {
        buf[pos.*] = s[i];
        pos.* += 1;
        i += 1;
    }
}

/// 零堆分配 Dockerfile 指令流式迭代器
pub struct DockerfileIter {
    src: []const u8,
    pos: usize = 0,
    line_no: usize = 1,
    escape_char: u8 = '\\',
    header_done: bool = false,

    /// 初始化迭代器，自动剥离可选 UTF-8 BOM
    static fn init(src: []const u8): DockerfileIter {
        let start_pos: usize = 0;
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            start_pos = 3;
        }
        return DockerfileIter{
            src: src,
            pos: start_pos,
            line_no: 1,
            escape_char: '\\',
            header_done: false,
        };
    }

    fn readRawLine(*_): ([]const u8)? {
        if (_.pos >= _.src.len) { return null; }
        const start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != '\n') {
            _.pos += 1;
        }
        let end = _.pos;
        if (end > start && _.src[end - 1] == '\r') {
            end -= 1;
        }
        if (_.pos < _.src.len && _.src[_.pos] == '\n') {
            _.pos += 1;
        }
        _.line_no += 1;
        return _.src[start..end];
    }

    /// 读取下一条完整指令，将续行折叠后的参数写入 `args_buf`
    fn next(*_, args_buf: []u8): DockerInstruction? {
        while (true) {
            const cur_line_no = _.line_no;
            const raw = _.readRawLine() ?? return null;
            const trimmed = trimHoriz(raw);

            if (trimmed.len == 0) {
                _.header_done = true;
                continue;
            }

            if (trimmed[0] == '#') {
                if (!_.header_done) {
                    if (esc := parseEscapeDirective(trimmed)) {
                        _.escape_char = esc;
                    } else {
                        _.header_done = true;
                    }
                }
                continue;
            }

            _.header_done = true;

            // 提取指令关键字
            let kw_end: usize = 0;
            while (kw_end < trimmed.len && !isHorizSpace(trimmed[kw_end])) {
                kw_end += 1;
            }
            const keyword = trimmed[0..kw_end];
            const first_seg = trimRightHoriz(trimLeftHoriz(trimmed[kw_end..trimmed.len]));

            let wpos: usize = 0;
            let has_cont = (first_seg.len > 0 && first_seg[first_seg.len - 1] == _.escape_char);
            const first_piece = if (has_cont) trimRightHoriz(first_seg[0..(first_seg.len - 1)]) else first_seg;
            if (first_piece.len > 0) {
                appendBytes(args_buf, *wpos, first_piece);
            }

            while (has_cont) {
                const next_raw = _.readRawLine() ?? break;
                const nt = trimHoriz(next_raw);
                if (nt.len == 0 || nt[0] == '#') {
                    continue;
                }
                has_cont = (nt.len > 0 && nt[nt.len - 1] == _.escape_char);
                const piece = if (has_cont) trimRightHoriz(nt[0..(nt.len - 1)]) else nt;
                if (piece.len > 0) {
                    if (wpos > 0 && wpos < args_buf.len && args_buf[wpos - 1] != ' ') {
                        args_buf[wpos] = ' ';
                        wpos += 1;
                    }
                    appendBytes(args_buf, *wpos, piece);
                }
            }

            return DockerInstruction{
                keyword: keyword,
                args: trimHoriz(args_buf[0..wpos]),
                line_no: cur_line_no,
            };
        }
        return null;
    }
}

/// 解析 `FROM` 指令参数（支持 `--platform=...`、镜像名与 `AS <stage>`）
pub fn parseFromArgs(args: []const u8): FromSpec? {
    let tokens: [8][]const u8 = [""; 8];
    let count: usize = 0;
    let i: usize = 0;
    while (i < args.len && count < 8) {
        while (i < args.len && isHorizSpace(args[i])) { i += 1; }
        if (i >= args.len) { break; }
        const start = i;
        while (i < args.len && !isHorizSpace(args[i])) { i += 1; }
        tokens[count] = args[start..i];
        count += 1;
    }
    if (count == 0) { return null; }

    let idx: usize = 0;
    let platform: ([]const u8)? = null;
    if (startsWithIgnoreCaseAscii(tokens[idx], "--platform=")) {
        const pval = tokens[idx][11..tokens[idx].len];
        if (pval.len == 0) { return null; }
        platform = pval;
        idx += 1;
    }

    if (idx >= count) { return null; }
    const image = tokens[idx];
    if (image.len == 0 || (image.len >= 2 && image[0] == '-' && image[1] == '-')) {
        return null;
    }
    idx += 1;

    let stage_alias: ([]const u8)? = null;
    if (idx < count) {
        if (!eqlIgnoreCaseAscii(tokens[idx], "as") || idx + 2 != count) {
            return null;
        }
        stage_alias = tokens[idx + 1];
    }

    return FromSpec{
        platform: platform,
        image: image,
        stage_alias: stage_alias,
    };
}

/// 解析 `EXPOSE` 单个端口标记（如 `8080`、`8080/tcp`、`53/udp`）
pub fn parseExposeToken(tok: []const u8): ExposePort? {
    const s = trimHoriz(tok);
    if (s.len == 0) { return null; }

    let slash_idx: usize? = null;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') {
            slash_idx = i;
            break;
        }
        i += 1;
    }

    let port_str = s;
    let is_udp = false;
    if (si := slash_idx) {
        port_str = s[0..si];
        const proto = s[(si + 1)..s.len];
        if (eqlIgnoreCaseAscii(proto, "tcp")) {
            is_udp = false;
        } else if (eqlIgnoreCaseAscii(proto, "udp")) {
            is_udp = true;
        } else {
            return null;
        }
    }

    if (port_str.len == 0 || port_str.len > 5) { return null; }
    let acc: u32 = 0;
    let k: usize = 0;
    while (k < port_str.len) {
        const c = port_str[k];
        if (c < '0' || c > '9') { return null; }
        acc = acc * 10 + @as(u32, c - '0');
        k += 1;
    }
    if (acc == 0 || acc > 65535) {
        return null;
    }

    return ExposePort{
        port: @as(u16, @intCast(acc)),
        is_udp: is_udp,
    };
}
