// @cap/json-rpc — Zero-heap JSON-RPC 2.0 protocol encoder & envelope validator
// MIT License (clean-room CP implementation)

import { hexCharLower } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { writeStr, writeI64 } from "cap:fmt";

pub const ERR_PARSE: i32 = -32700;
pub const ERR_INVALID_REQUEST: i32 = -32600;
pub const ERR_METHOD_NOT_FOUND: i32 = -32601;
pub const ERR_INVALID_PARAMS: i32 = -32602;
pub const ERR_INTERNAL: i32 = -32603;

pub fn isServerError(code: i32) -> bool {
    return code >= -32099 && code <= -32000;
}

pub fn errorMessage(code: i32) -> string {
    if (code == ERR_PARSE) {
        return "Parse error";
    }
    if (code == ERR_INVALID_REQUEST) {
        return "Invalid Request";
    }
    if (code == ERR_METHOD_NOT_FOUND) {
        return "Method not found";
    }
    if (code == ERR_INVALID_PARAMS) {
        return "Invalid params";
    }
    if (code == ERR_INTERNAL) {
        return "Internal error";
    }
    if (isServerError(code)) {
        return "Server error";
    }
    return "";
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8 || c == '\n' as u8 || c == '\r' as u8;
}

fn appendByte(out: []u8, pos: usize, b: u8) -> i64 {
    if (pos >= out.len) {
        return -1;
    }
    out[pos] = b;
    return (pos + 1) as i64;
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    return writeStr(s, out, pos) as i64;
}

fn appendI32(out: []u8, pos: usize, v: i32) -> i64 {
    let digits: [16]u8 = undefined;
    const n: usize = writeI64(v as i64, digits[0..16], 0);
    return appendSlice(out, pos, digits[0..n]);
}

fn appendJsonString(out: []u8, pos: usize, s: string) -> i64 {
    let p: i64 = appendByte(out, pos, '"' as u8);
    if (p < 0) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c == '"' as u8) {
            p = appendSlice(out, p as usize, "\\\"");
        } else if (c == '\\' as u8) {
            p = appendSlice(out, p as usize, "\\\\");
        } else if (c == '\n' as u8) {
            p = appendSlice(out, p as usize, "\\n");
        } else if (c == '\r' as u8) {
            p = appendSlice(out, p as usize, "\\r");
        } else if (c == '\t' as u8) {
            p = appendSlice(out, p as usize, "\\t");
        } else if (c < 0x20) {
            p = appendSlice(out, p as usize, "\\u00");
            if (p < 0) {
                return -1;
            }
            p = appendByte(out, p as usize, hexCharLower((c >> 4) & 0x0F));
            if (p < 0) {
                return -1;
            }
            p = appendByte(out, p as usize, hexCharLower(c & 0x0F));
        } else {
            p = appendByte(out, p as usize, c);
        }
        if (p < 0) {
            return -1;
        }
        i += 1;
    }
    return appendByte(out, p as usize, '"' as u8);
}

pub fn encodeRequest(id_json: string, method: string, params_json: string, out: []u8) -> usize {
    if (method.len == 0) {
        return 0;
    }
    let p: i64 = appendSlice(out, 0, "{\"jsonrpc\":\"2.0\"");
    if (p < 0) {
        return 0;
    }
    if (id_json.len > 0) {
        p = appendSlice(out, p as usize, ",\"id\":");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, id_json);
        if (p < 0) {
            return 0;
        }
    }
    p = appendSlice(out, p as usize, ",\"method\":");
    if (p < 0) {
        return 0;
    }
    p = appendJsonString(out, p as usize, method);
    if (p < 0) {
        return 0;
    }
    if (params_json.len > 0) {
        p = appendSlice(out, p as usize, ",\"params\":");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, params_json);
        if (p < 0) {
            return 0;
        }
    }
    p = appendByte(out, p as usize, '}' as u8);
    if (p < 0) {
        return 0;
    }
    return p as usize;
}

pub fn encodeResult(id_json: string, result_json: string, out: []u8) -> usize {
    if (id_json.len == 0 || result_json.len == 0) {
        return 0;
    }
    let p: i64 = appendSlice(out, 0, "{\"jsonrpc\":\"2.0\",\"id\":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, id_json);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ",\"result\":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, result_json);
    if (p < 0) {
        return 0;
    }
    p = appendByte(out, p as usize, '}' as u8);
    if (p < 0) {
        return 0;
    }
    return p as usize;
}

pub fn encodeError(
    id_json: string,
    code: i32,
    message: string,
    data_json: string,
    out: []u8,
) -> usize {
    let p: i64 = appendSlice(out, 0, "{\"jsonrpc\":\"2.0\",\"id\":");
    if (p < 0) {
        return 0;
    }
    if (id_json.len > 0) {
        p = appendSlice(out, p as usize, id_json);
    } else {
        p = appendSlice(out, p as usize, "null");
    }
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ",\"error\":{\"code\":");
    if (p < 0) {
        return 0;
    }
    p = appendI32(out, p as usize, code);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ",\"message\":");
    if (p < 0) {
        return 0;
    }
    p = appendJsonString(out, p as usize, message);
    if (p < 0) {
        return 0;
    }
    if (data_json.len > 0) {
        p = appendSlice(out, p as usize, ",\"data\":");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, data_json);
        if (p < 0) {
            return 0;
        }
    }
    p = appendSlice(out, p as usize, "}}");
    if (p < 0) {
        return 0;
    }
    return p as usize;
}

pub fn isBatch(json: string) -> bool {
    let s: usize = 0;
    let e: usize = json.len;
    while (s < e && isWs(json[s])) {
        s += 1;
    }
    while (e > s && isWs(json[e - 1])) {
        e -= 1;
    }
    return (e - s) >= 2 && json[s] == '[' as u8 && json[e - 1] == ']' as u8;
}

pub fn isValidEnvelope(json: string) -> bool {
    let s: usize = 0;
    let e: usize = json.len;
    while (s < e && isWs(json[s])) {
        s += 1;
    }
    while (e > s && isWs(json[e - 1])) {
        e -= 1;
    }
    if ((e - s) < 17 || json[s] != '{' as u8 || json[e - 1] != '}' as u8) {
        return false;
    }

    // Scan top-level (depth == 1) object keys for "jsonrpc": "2.0"
    let depth: usize = 0;
    let i: usize = s;
    while (i < e) {
        const c: u8 = json[i];
        if (c == '{' as u8 || c == '[' as u8) {
            depth += 1;
            i += 1;
            continue;
        }
        if (c == '}' as u8 || c == ']' as u8) {
            if (depth == 0) {
                return false;
            }
            depth -= 1;
            i += 1;
            continue;
        }
        if (c == '"' as u8) {
            const str_start: usize = i + 1;
            i += 1;
            while (i < e) {
                if (json[i] == '\\' as u8 && i + 1 < e) {
                    i += 2;
                    continue;
                }
                if (json[i] == '"' as u8) {
                    break;
                }
                i += 1;
            }
            if (i >= e) {
                return false;
            }
            const str_end: usize = i;
            i += 1;

            if (depth == 1 && bytesEq(json[str_start..str_end], "jsonrpc")) {
                let k: usize = i;
                while (k < e && isWs(json[k])) {
                    k += 1;
                }
                if (k < e && json[k] == ':' as u8) {
                    k += 1;
                    while (k < e && isWs(json[k])) {
                        k += 1;
                    }
                    if (k + 5 <= e && bytesEq(json[k..(k + 5)], "\"2.0\"")) {
                        return true;
                    }
                }
            }
            continue;
        }
        i += 1;
    }
    return false;
}
