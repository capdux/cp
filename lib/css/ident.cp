// Source: Absorbed from /data/dev/bun/src/css/values/ident.cp | Synced: 2026-11-23T21:44:18Z
// Pure-CP zero-allocation W3C CSS Values Level 4 `<ident>`, `<custom-ident>`, `<dashed-ident>`,
// CSS Modules `DashedIdentReference` (`--var from "..." | global`), and CSSOM identifier serializer.

type CssIdentVec16 = @Vector(16, u8);

const CSS_HEX_LOWER: []const u8 = "0123456789abcdef";

fn makeCssSafeIdentLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9') ||
            i == '-' || i == '_' || i >= 0x80)
        {
            lut[i] = 1;
        }
    }
    return lut;
}

fn makeCssWsLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[usize(' ')] = 1;
    lut[usize('\t')] = 1;
    lut[usize('\r')] = 1;
    lut[usize('\n')] = 1;
    return lut;
}

fn makeCssLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const CSS_SAFE_IDENT_LUT: [u8; 256] = makeCssSafeIdentLut();
const CSS_WS_LUT: [u8; 256] = makeCssWsLut();
const CSS_LOWER_LUT: [u8; 256] = makeCssLowerLut();

/// CSS 标识符解析与序列化错误集
pub err CssIdentError {
    EmptyIdent,
    InvalidIdent,
    ReservedKeyword,
    MissingDoubleDash,
    InvalidFromClause,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return CSS_WS_LUT[usize(c)] != 0;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isSafeIdentByte(c: u8): bool {
    return CSS_SAFE_IDENT_LUT[usize(c)] != 0;
}

fn scanSafeIdentEndSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_a: CssIdentVec16 = @splat('a' as u8);
    const v_z: CssIdentVec16 = @splat('z' as u8);
    const v_A: CssIdentVec16 = @splat('A' as u8);
    const v_Z: CssIdentVec16 = @splat('Z' as u8);
    const v_0: CssIdentVec16 = @splat('0' as u8);
    const v_9: CssIdentVec16 = @splat('9' as u8);
    const v_dash: CssIdentVec16 = @splat('-' as u8);
    const v_us: CssIdentVec16 = @splat('_' as u8);
    const v_hi: CssIdentVec16 = @splat(0x80 as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: CssIdentVec16 = s[i .. i + 16][0..16].*;
        const ok = ((chunk >= v_a) & (chunk <= v_z)) |
            ((chunk >= v_A) & (chunk <= v_Z)) |
            ((chunk >= v_0) & (chunk <= v_9)) |
            (chunk == v_dash) | (chunk == v_us) | (chunk >= v_hi);
        const mask: u16 = @bitCast(ok);
        const bad: u16 = ~mask;
        if (bad != 0) {
            return i + usize(@ctz(bad));
        }
    }
    while (i < s.len && CSS_SAFE_IDENT_LUT[usize(s[i])] != 0) : (i += 1) {}
    return i;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: CssIdentVec16 = a[i .. i + 16][0..16].*;
        const vb: CssIdentVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: CssIdentVec16 = @splat(65 as u8);
    const hi_Z: CssIdentVec16 = @splat(90 as u8);
    const case_bit: CssIdentVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: CssIdentVec16 = a[i .. i + 16][0..16].*;
        const vb: CssIdentVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (CSS_LOWER_LUT[usize(a[i])] != CSS_LOWER_LUT[usize(b[i])]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    if (src.len > 0) {
        @memcpy(out[pos .. pos + src.len], src);
    }
    return pos + src.len;
}

fn hexDigitLower(nibble: u8): u8 {
    return CSS_HEX_LOWER[usize(nibble & 0x0F)];
}

fn appendHexEscape(out: []u8, pos: usize, byte: u8): usize! {
    let cur = appendChar(out, pos, '\\')?;
    const hi = (byte >> 4) & 0x0F;
    const lo = byte & 0x0F;
    if (hi != 0) {
        cur = appendChar(out, cur, hexDigitLower(hi))?;
    }
    cur = appendChar(out, cur, hexDigitLower(lo))?;
    cur = appendChar(out, cur, ' ')?;
    return cur;
}

/// 校验是否为合法未转义 CSS 标识符词法（遵循 CSS Syntax Level 3 `would start an ident sequence`）
fn isValidRawCssIdent(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    if (s[0] == '-') {
        if (s.len == 1) {
            return false;
        }
        const c1 = s[1];
        if (c1 != '-' && !isAsciiAlpha(c1) && c1 != '_' && c1 < 0x80) {
            return false;
        }
    } else if (!isAsciiAlpha(s[0]) && s[0] != '_' && s[0] < 0x80) {
        return false;
    }
    return scanSafeIdentEndSimd(s, 1) == s.len;
}

/// 判断是否为 `<custom-ident>` 禁用的 6 大 CSS-wide 全局关键字（大小写不敏感，对齐 Bun `CustomIdent.parse`）
pub fn isCssWideKeyword(s: []const u8): bool {
    return eqlAsciiIgnoreCase(s, "initial") ||
        eqlAsciiIgnoreCase(s, "inherit") ||
        eqlAsciiIgnoreCase(s, "unset") ||
        eqlAsciiIgnoreCase(s, "default") ||
        eqlAsciiIgnoreCase(s, "revert") ||
        eqlAsciiIgnoreCase(s, "revert-layer");
}

/// 按 W3C CSSOM `serialize an identifier` 规范将任意字符串转义序列化为合法 CSS 标识符
pub fn serializeCssIdentifier(ident: []const u8, out: []u8): usize! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < ident.len) {
        if (i >= 2) {
            const safe_end = scanSafeIdentEndSimd(ident, i);
            if (safe_end > i) {
                pos = appendBytes(out, pos, ident[i..safe_end])?;
                i = safe_end;
                if (i >= ident.len) break;
            }
        }
        const c = ident[i];
        if (c == 0) {
            // U+0000 -> U+FFFD (UTF-8: EF BF BD)
            const repl: [3]u8 = [0xEF, 0xBF, 0xBD];
            pos = appendBytes(out, pos, repl[0..])?;
        } else if ((c >= 0x01 && c <= 0x1F) || c == 0x7F) {
            pos = appendHexEscape(out, pos, c)?;
        } else if (i == 0 && isDigit(c)) {
            pos = appendHexEscape(out, pos, c)?;
        } else if (i == 1 && isDigit(c) && ident[0] == '-') {
            pos = appendHexEscape(out, pos, c)?;
        } else if (i == 0 && ident.len == 1 && c == '-') {
            pos = appendBytes(out, pos, "\\-")?;
        } else if (isSafeIdentByte(c)) {
            pos = appendChar(out, pos, c)?;
        } else {
            pos = appendChar(out, pos, '\\')?;
            pos = appendChar(out, pos, c)?;
        }
        i += 1;
    }
    return pos;
}

/// CSS `<ident>` 值（对齐 Bun `Ident`）
pub struct CssIdent {
    value: []const u8 = "",

    static fn parse(raw: []const u8): CssIdent! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return err.EmptyIdent;
        }
        if (!isValidRawCssIdent(s)) {
            return err.InvalidIdent;
        }
        return CssIdent{ value: s };
    }

    fn eql(other: CssIdent): bool {
        return bytesEql(_.value, other.value);
    }

    fn toCss(out: []u8): usize! {
        return serializeCssIdentifier(_.value, out);
    }
}

/// W3C CSS `<custom-ident>` 值（自动拒绝 `initial` / `inherit` / `unset` / `default` / `revert` / `revert-layer`）
pub struct CustomIdent {
    value: []const u8 = "",

    static fn parse(raw: []const u8): CustomIdent! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return err.EmptyIdent;
        }
        if (!isValidRawCssIdent(s)) {
            return err.InvalidIdent;
        }
        if (isCssWideKeyword(s)) {
            return err.ReservedKeyword;
        }
        return CustomIdent{ value: s };
    }

    fn eql(other: CustomIdent): bool {
        return bytesEql(_.value, other.value);
    }

    fn toCss(out: []u8): usize! {
        return serializeCssIdentifier(_.value, out);
    }
}

/// W3C CSS `<dashed-ident>` 值（必须以 `--` 开头，例如 `--brand-primary`）
pub struct DashedIdent {
    value: []const u8 = "",

    static fn parse(raw: []const u8): DashedIdent! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return err.EmptyIdent;
        }
        if (s.len < 2 || s[0] != '-' || s[1] != '-') {
            return err.MissingDoubleDash;
        }
        if (s.len == 2) {
            return err.InvalidIdent;
        }
        if (scanSafeIdentEndSimd(s, 2) != s.len) {
            return err.InvalidIdent;
        }
        return DashedIdent{ value: s };
    }

    /// 返回去除前导 `--` 后的名称部分
    fn nameWithoutPrefix(): []const u8 {
        if (_.value.len <= 2) {
            return "";
        }
        return _.value[2..];
    }

    fn eql(other: DashedIdent): bool {
        return bytesEql(_.value, other.value);
    }

    fn toCss(out: []u8): usize! {
        return serializeCssIdentifier(_.value, out);
    }
}

/// CSS Modules `DashedIdentReference` 的 `from` 来源类型
pub enum CssIdentFromKind: u8 {
    None = 0,
    Global = 1,
    File = 2,
}

/// CSS `<dashed-ident>` 引用（支持 CSS Modules `--ident from global` 与 `--ident from "./file.css"`）
pub struct DashedIdentReference {
    ident: DashedIdent = DashedIdent{},
    from_kind: CssIdentFromKind = CssIdentFromKind.None,
    from_file: []const u8 = "",

    /// 解析 `--name`、`--name from global` 或 `--name from "./tokens.css"`
    static fn parse(raw: []const u8): DashedIdentReference! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return err.EmptyIdent;
        }
        // 提取首段 `<dashed-ident>` 令牌
        let tok_end: usize = 0;
        while (tok_end < s.len && !isWs(s[tok_end])) {
            tok_end += 1;
        }
        const ident = DashedIdent.parse(s[0..tok_end])?;
        const rest = trimWs(s[tok_end..]);
        if (rest.len == 0) {
            return DashedIdentReference{
                ident: ident,
                from_kind: CssIdentFromKind.None,
                from_file: "",
            };
        }

        // 必须紧跟 `from` 关键字 + 空白 + (`global` | 引号文件路径)
        let kw_end: usize = 0;
        while (kw_end < rest.len && !isWs(rest[kw_end])) {
            kw_end += 1;
        }
        if (!eqlAsciiIgnoreCase(rest[0..kw_end], "from")) {
            return err.InvalidFromClause;
        }
        const spec = trimWs(rest[kw_end..]);
        if (spec.len == 0) {
            return err.InvalidFromClause;
        }
        if (eqlAsciiIgnoreCase(spec, "global")) {
            return DashedIdentReference{
                ident: ident,
                from_kind: CssIdentFromKind.Global,
                from_file: "",
            };
        }
        if (spec.len >= 3) {
            const q = spec[0];
            if ((q == '"' || q == '\'') && spec[spec.len - 1] == q) {
                const path = spec[1 .. spec.len - 1];
                if (path.len == 0) {
                    return err.InvalidFromClause;
                }
                return DashedIdentReference{
                    ident: ident,
                    from_kind: CssIdentFromKind.File,
                    from_file: path,
                };
            }
        }
        return err.InvalidFromClause;
    }

    fn hasFrom(): bool {
        return _.from_kind != CssIdentFromKind.None;
    }

    fn isFromGlobal(): bool {
        return _.from_kind == CssIdentFromKind.Global;
    }

    fn eql(other: DashedIdentReference): bool {
        if (!_.ident.eql(other.ident) || _.from_kind != other.from_kind) {
            return false;
        }
        if (_.from_kind == CssIdentFromKind.File) {
            return bytesEql(_.from_file, other.from_file);
        }
        return true;
    }

    fn toCss(out: []u8): usize! {
        let pos = _.ident.toCss(out)?;
        if (_.from_kind == CssIdentFromKind.Global) {
            pos = appendBytes(out, pos, " from global")?;
        } else if (_.from_kind == CssIdentFromKind.File) {
            pos = appendBytes(out, pos, " from \"")?;
            pos = appendBytes(out, pos, _.from_file)?;
            pos = appendChar(out, pos, '"')?;
        }
        return pos;
    }
}
