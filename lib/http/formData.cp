// Source: Absorbed from /data/dev/bun/src/runtime/webcore/FormData.cp | Synced: 2026-11-23T20:50:09Z
// lib/http/form_data.cp
// RFC 7578 multipart/form-data & URL-Encoded FormData Parser

const std = @import("std");

pub struct FormField {
    pub name: []const u8 = "";
    pub value: []const u8 = "";
    pub filename: []const u8 = "";
    pub content_type: []const u8 = "text/plain";
}

pub struct FormData {
    pub fields: [64]FormField = undefined;
    pub count: usize = 0;

    pub static fn init(): FormData {
        return FormData {
            fields: undefined,
            count: 0,
        };
    }

    /// Parses an `application/x-www-form-urlencoded` body (`a=1&b=hello`).
    pub static fn parseUrlEncoded(body: []const u8): FormData {
        let form = FormData.init();
        let iter = std.mem.splitScalar(u8, body, '&');
        while (pair := iter.next()) {
            if (pair.len == 0) {
                continue;
            }
            if (eq_idx := std.mem.indexOfScalar(u8, pair, '=')) {
                form.append(pair[0..eq_idx], pair[eq_idx + 1 ..]);
            } else {
                form.append(pair, "");
            }
        }
        return form;
    }

    /// Extracts the `boundary=...` parameter from a `Content-Type: multipart/form-data; boundary=...` header.
    pub static fn extractBoundary(content_type: []const u8): ([]const u8)? {
        if (idx := std.mem.indexOf(u8, content_type, "boundary=")) {
            const rest = std.mem.trim(u8, content_type[idx + 9 ..], " \t\r\n\"'");
            if (semi := std.mem.indexOfScalar(u8, rest, ';')) {
                return std.mem.trim(u8, rest[0..semi], " \t\"'");
            }
            return rest;
        }
        return null;
    }

    pub fn append(*_, name: []const u8, value: []const u8) {
        if (_.count < 64) {
            _.fields[_.count] = FormField {
                name: name,
                value: value,
            };
            _.count += 1;
        }
    }

    pub fn get(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.fields[i].name, name)) {
                return _.fields[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn has(name: []const u8): bool {
        return _.get(name) != null;
    }
}
