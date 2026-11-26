import {
    MediaType, parseMediaType, formatMediaType, mediaTypeEq, mtPushLower
} from "../../media-typer/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";

pub const MAX_MT_PARAMS: usize = 8;

pub struct MediaParam {
    pub name: string,
    pub value: string,
}

fn mtyTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == 32 || s[start] == 9)) {
        start = start + 1;
    }
    while (end > start && (s[end - 1] == 32 || s[end - 1] == 9)) {
        end = end - 1;
    }
    return s[start..end];
}

fn mtyPushStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

pub struct ParsedMediaType {
    pub valid: bool,
    pub top_level: string,
    pub sub_level: string,
    pub suffix: string,
    pub params: [8]MediaParam,
    pub param_count: usize,

    pub static fn invalid(): ParsedMediaType {
        let blank = MediaParam { name: "", value: "" };
        let arr: [8]MediaParam = [blank; 8];
        return ParsedMediaType {
            valid: false,
            top_level: "",
            sub_level: "",
            suffix: "",
            params: arr,
            param_count: 0,
        };
    }

    pub static fn parse(raw: string): ParsedMediaType {
        let semi: usize = 0;
        while (semi < raw.len && raw[semi] != 59) { // ';'
            semi = semi + 1;
        }
        let essence_raw = mtyTrim(raw[0..semi]);
        let core: MediaType = parseMediaType(essence_raw);
        if (!core.valid) {
            return ParsedMediaType.invalid();
        }
        let res = ParsedMediaType.invalid();
        res.valid = true;
        res.top_level = core.main_type;
        res.sub_level = core.subtype;
        res.suffix = core.suffix;

        let pos = semi;
        while (pos < raw.len) {
            if (raw[pos] == 59) { pos = pos + 1; }
            let seg_start = pos;
            let in_quotes = false;
            while (pos < raw.len) {
                let c = raw[pos];
                if (c == 34) { in_quotes = !in_quotes; }
                else if (c == 59 && !in_quotes) { break; }
                pos = pos + 1;
            }
            let seg = mtyTrim(raw[seg_start..pos]);
            if (seg.len == 0) { continue; }
            let eq: usize = 0;
            let found = false;
            while (eq < seg.len) {
                if (seg[eq] == 61) { found = true; break; }
                eq = eq + 1;
            }
            if (!found || eq == 0) { return ParsedMediaType.invalid(); }
            let k = mtyTrim(seg[0..eq]);
            let v = mtyTrim(seg[(eq + 1)..seg.len]);
            if (v.len >= 2 && v[0] == 34 && v[v.len - 1] == 34) {
                v = v[1..(v.len - 1)];
            }
            if (res.param_count < MAX_MT_PARAMS) {
                res.params[res.param_count] = MediaParam { name: k, value: v };
                res.param_count = res.param_count + 1;
            }
        }
        return res;
    }

    pub fn getParam(key: string): string? {
        let i: usize = 0;
        while (i < _.param_count) {
            if (eqAscii(_.params[i].name, key)) {
                return _.params[i].value;
            }
            i = i + 1;
        }
        return null;
    }

    pub fn hasSuffix(suf: string): bool {
        if (_.suffix.len == 0) { return false; }
        return eqAscii(_.suffix, suf);
    }

    pub fn formatEssence(out: []u8): usize {
        return formatMediaType(_.top_level, _.sub_level, _.suffix, out);
    }

    pub fn formatFull(out: []u8): usize {
        let p = formatMediaType(_.top_level, _.sub_level, _.suffix, out);
        let i: usize = 0;
        while (i < _.param_count && p < out.len) {
            p = mtyPushStr(out, p, "; ");
            p = mtPushLower(out, p, _.params[i].name);
            p = mtyPushStr(out, p, "=");
            p = mtyPushStr(out, p, _.params[i].value);
            i = i + 1;
        }
        return p;
    }

    pub fn isJson(): bool {
        if (eqAscii(_.top_level, "application") && eqAscii(_.sub_level, "json")) {
            return true;
        }
        return _.hasSuffix("json");
    }

    pub fn isText(): bool {
        return eqAscii(_.top_level, "text");
    }
}

pub fn essenceEq(a: string, b: string): bool {
    let pa = ParsedMediaType.parse(a);
    let pb = ParsedMediaType.parse(b);
    if (!pa.valid || !pb.valid) { return false; }
    let ba: [128]u8 = [0; 128];
    let bb: [128]u8 = [0; 128];
    let na = pa.formatEssence(ba[0..128]);
    let nb = pb.formatEssence(bb[0..128]);
    return mediaTypeEq(ba[0..na] as string, bb[0..nb] as string);
}
