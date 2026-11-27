// @cap/s3-uri — Zero-heap AWS S3 / R2 / MinIO URI parser & bucket validator
// Clean-room MIT implementation for CP (compatible with npm amazon-s3-uri).

import { strEq, startsWith, endsWith, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const STYLE_S3: u8 = 1;
pub const STYLE_VIRTUAL_HOSTED: u8 = 2;
pub const STYLE_PATH: u8 = 3;

pub struct S3Uri {
    pub style: u8,
    pub bucket: string,
    pub key: string,
    pub region: string,
    pub valid: bool,
}

fn indexOfByte(s: string, ch: u8): i64 {
    if (idx := findByte(s, ch, 0)) {
        return idx as i64;
    }
    return -1;
}

/// Validate AWS S3 bucket naming rules (3..63 chars, lowercase letters, digits, '.', '-',
/// must start and end with letter/digit, no consecutive '..', not formatted as IPv4).
pub fn isValidBucketName(name: string): bool {
    if (name.len < 3 || name.len > 63) {
        return false;
    }
    let first: u8 = name[0];
    let last: u8 = name[name.len - 1];
    let first_ok: bool = (first >= 'a' && first <= 'z') || (first >= '0' && first <= '9');
    let last_ok: bool = (last >= 'a' && last <= 'z') || (last >= '0' && last <= '9');
    if (!first_ok || !last_ok) {
        return false;
    }
    let prev_dot: bool = false;
    let all_digits_or_dots: bool = true;
    let dot_count: usize = 0;
    let i: usize = 0;
    while (i < name.len) {
        let c: u8 = name[i];
        let is_alnum: bool = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (!is_alnum && c != '-' && c != '.') {
            return false;
        }
        if (c == '.') {
            if (prev_dot) { return false; }
            prev_dot = true;
            dot_count += 1;
        } else {
            prev_dot = false;
        }
        if (!(c >= '0' && c <= '9') && c != '.') {
            all_digits_or_dots = false;
        }
        i += 1;
    }
    if (all_digits_or_dots && dot_count == 3) {
        return false;
    }
    return true;
}

fn stripQueryAndFrag(s: string): string {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '?' || s[i] == '#') {
            return s[0..i];
        }
        i += 1;
    }
    return s;
}

/// Parse an S3 URI in any of the 3 standard forms:
/// 1. `s3://bucket/path/to/key`
/// 2. Virtual-hosted: `https://bucket.s3.us-east-1.amazonaws.com/path/to/key` or `https://bucket.s3.amazonaws.com/key`
/// 3. Path-style: `https://s3.us-east-1.amazonaws.com/bucket/path/to/key` or `https://s3.amazonaws.com/bucket/key`
pub fn parseS3Uri(input: string): S3Uri {
    let invalid: S3Uri = S3Uri {
        style: 0,
        bucket: "",
        key: "",
        region: "",
        valid: false,
    };

    let clean: string = stripQueryAndFrag(input);
    if (startsWith(clean, "s3://")) {
        let rest: string = clean[5..clean.len];
        if (rest.len == 0) { return invalid; }
        let slash: i64 = indexOfByte(rest, '/');
        let bucket: string = rest;
        let key: string = "";
        if (slash >= 0) {
            bucket = rest[0..(slash as usize)];
            key = rest[((slash as usize) + 1)..rest.len];
        }
        if (!isValidBucketName(bucket)) { return invalid; }
        return S3Uri {
            style: STYLE_S3,
            bucket: bucket,
            key: key,
            region: "",
            valid: true,
        };
    }

    let after_scheme: string = "";
    if (startsWith(clean, "https://")) {
        after_scheme = clean[8..clean.len];
    } else if (startsWith(clean, "http://")) {
        after_scheme = clean[7..clean.len];
    } else {
        return invalid;
    }

    let slash: i64 = indexOfByte(after_scheme, '/');
    let host: string = after_scheme;
    let path: string = "";
    if (slash >= 0) {
        host = after_scheme[0..(slash as usize)];
        path = after_scheme[((slash as usize) + 1)..after_scheme.len];
    }

    if (!endsWith(host, ".amazonaws.com")) {
        return invalid;
    }
    let prefix_host: string = host[0..(host.len - 14)]; // strip ".amazonaws.com"

    // Path-style: host is `s3` or `s3.<region>` or `s3-<region>`
    if (strEq(prefix_host, "s3") || startsWith(prefix_host, "s3.") || startsWith(prefix_host, "s3-")) {
        let region: string = "";
        if (prefix_host.len > 3) {
            region = prefix_host[3..prefix_host.len];
        }
        if (path.len == 0) { return invalid; }
        let pslash: i64 = indexOfByte(path, '/');
        let bucket: string = path;
        let key: string = "";
        if (pslash >= 0) {
            bucket = path[0..(pslash as usize)];
            key = path[((pslash as usize) + 1)..path.len];
        }
        if (!isValidBucketName(bucket)) { return invalid; }
        return S3Uri {
            style: STYLE_PATH,
            bucket: bucket,
            key: key,
            region: region,
            valid: true,
        };
    }

    // Virtual-hosted-style: `<bucket>.s3` or `<bucket>.s3.<region>` or `<bucket>.s3-<region>`
    let marker_pos: i64 = -1;
    let i: usize = 0;
    while (i + 3 <= prefix_host.len) {
        if (prefix_host[i] == '.' && prefix_host[i + 1] == 's' && prefix_host[i + 2] == '3') {
            if (i + 3 == prefix_host.len || prefix_host[i + 3] == '.' || prefix_host[i + 3] == '-') {
                marker_pos = i as i64;
                break;
            }
        }
        i += 1;
    }
    if (marker_pos <= 0) {
        return invalid;
    }
    let bucket: string = prefix_host[0..(marker_pos as usize)];
    if (!isValidBucketName(bucket)) { return invalid; }
    let region: string = "";
    let reg_start: usize = (marker_pos as usize) + 3;
    if (reg_start + 1 < prefix_host.len) {
        region = prefix_host[(reg_start + 1)..prefix_host.len];
    }
    return S3Uri {
        style: STYLE_VIRTUAL_HOSTED,
        bucket: bucket,
        key: path,
        region: region,
        valid: true,
    };
}

/// Format `(bucket, key)` into canonical `s3://<bucket>/<key>` URI.
pub fn formatS3Uri(bucket: string, key: string, out: []u8): usize {
    if (!isValidBucketName(bucket)) { return 0; }
    let need: usize = (5 as usize) + bucket.len;
    if (key.len > 0) {
        need += (1 as usize) + key.len;
    }
    if (need > out.len) { return 0; }
    let pos: usize = writeStr("s3://", out, 0);
    pos = writeStr(bucket, out, pos);
    if (key.len > 0) {
        out[pos] = '/';
        pos = writeStr(key, out, pos + 1);
    }
    return pos;
}
