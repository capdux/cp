// Source: cp/lib/schema/validator.cp (JSON Schema Runtime Validator with JSON Pointer Diagnostics)
// Pure CP JSON Schema runtime validation engine with RFC 6901 JSON Pointer error paths,
// format validation (email/uri/uuid/ipv4/date-time), and max recursion depth (64) guard.

const std = @import("std");
import { JsonValue, JsonTag, JsonField, JsonParser } from "cap:json";
import { Schema, SchemaType, StringFormat, PropertySchema } from "./schema.cp";

pub struct ValidationIssue {
    path: []const u8 = "",
    code: []const u8 = "",
    message: []const u8 = "",
}

pub struct ValidationResult {
    allocator: std.mem.Allocator = undefined,
    ok: bool = true,
    issues: []ValidationIssue = [],

    pub fn count(): usize {
        return _.issues.len;
    }

    pub fn first(): ValidationIssue? {
        if (_.issues.len == 0) return null;
        return _.issues[0];
    }

    pub fn deinit(*_) {
        for (iss of _.issues) {
            if (iss.path.len > 0) {
                _.allocator.free(iss.path);
            }
            if (iss.message.len > 0) {
                _.allocator.free(iss.message);
            }
        }
        if (_.issues.len > 0) {
            _.allocator.free(_.issues);
        }
        _.issues = [];
    }
}

pub struct SchemaValidator {
    pub static const MAX_DEPTH: usize = 64;

    static fn buildPointer(alloc: std.mem.Allocator, segments: []const []const u8): ([]const u8)! {
        if (segments.len == 0) {
            return "";
        }
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        for (seg of segments) {
            out.append(alloc, '/')?;
            for (c of seg) {
                if (c == '~') {
                    out.appendSlice(alloc, "~0")?;
                } else if (c == '/') {
                    out.appendSlice(alloc, "~1")?;
                } else {
                    out.append(alloc, c)?;
                }
            }
        }
        return out.toOwnedSlice(alloc);
    }

    static fn pushIssue(
        alloc: std.mem.Allocator,
        issues: *std.ArrayList(ValidationIssue),
        segments: []const []const u8,
        code: []const u8,
        msg: []const u8,
    ): ! {
        const ptr_str = SchemaValidator.buildPointer(alloc, segments)?;
        errdefer if (ptr_str.len > 0) alloc.free(ptr_str);
        const msg_copy = alloc.dupe(u8, msg)?;
        errdefer alloc.free(msg_copy);
        issues.append(alloc, ValidationIssue{
            path: ptr_str,
            code: code,
            message: msg_copy,
        })?;
    }

    static fn countCodepoints(s: []const u8): usize {
        return std.unicode.utf8CountCodepoints(s) catch s.len;
    }

    static fn isAsciiAlpha(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static fn isAsciiDigit(c: u8): bool {
        return c >= '0' && c <= '9';
    }

    static fn isAsciiAlnum(c: u8): bool {
        return SchemaValidator.isAsciiAlpha(c) || SchemaValidator.isAsciiDigit(c);
    }

    static fn isHexDigit(c: u8): bool {
        return SchemaValidator.isAsciiDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static fn isEmailLocalChar(c: u8): bool {
        if (SchemaValidator.isAsciiAlnum(c)) return true;
        return c == '.' || c == '!' || c == '#' || c == '$' || c == '%' ||
            c == '&' || c == '\'' || c == '*' || c == '+' || c == '-' ||
            c == '/' || c == '=' || c == '?' || c == '^' || c == '_' ||
            c == '`' || c == '{' || c == '|' || c == '}' || c == '~';
    }

    pub static fn isValidEmail(s: []const u8): bool {
        if (s.len < 3 || s.len > 320) return false;
        let at_idx: usize? = null;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c <= 0x20 || c >= 0x7F) return false;
            if (c == '@') {
                if (at_idx != null) return false;
                at_idx = i;
            }
        }
        const at = at_idx ?? return false;
        if (at == 0 || at > 64 || at + 1 >= s.len) return false;

        const local = s[0..at];
        const domain = s[at + 1 ..];
        if (domain.len == 0 || domain.len > 253) return false;

        // Validate local part
        if (local[0] == '.' || local[local.len - 1] == '.') return false;
        let prev_dot = false;
        for (c of local) {
            if (!SchemaValidator.isEmailLocalChar(c)) return false;
            if (c == '.') {
                if (prev_dot) return false;
                prev_dot = true;
            } else {
                prev_dot = false;
            }
        }

        // Validate domain part: must contain at least one '.', valid DNS labels
        if (domain[0] == '.' || domain[domain.len - 1] == '.') return false;
        let dot_count: usize = 0;
        let label_start: usize = 0;
        let j: usize = 0;
        while (j <= domain.len) : (j += 1) {
            if (j == domain.len || domain[j] == '.') {
                const label = domain[label_start..j];
                if (label.len == 0 || label.len > 63) return false;
                if (label[0] == '-' || label[label.len - 1] == '-') return false;
                for (lc of label) {
                    if (!SchemaValidator.isAsciiAlnum(lc) && lc != '-') return false;
                }
                if (j < domain.len) {
                    dot_count += 1;
                }
                label_start = j + 1;
            }
        }
        return dot_count >= 1;
    }

    pub static fn isValidUri(s: []const u8): bool {
        if (s.len < 3) return false;
        let colon_idx: usize? = null;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (c <= 0x20 || c >= 0x7F) return false;
            if (c == ':' && colon_idx == null) {
                colon_idx = i;
            }
        }
        const colon = colon_idx ?? return false;
        if (colon == 0 || colon + 1 >= s.len) return false;

        // Scheme: ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        if (!SchemaValidator.isAsciiAlpha(s[0])) return false;
        let k: usize = 1;
        while (k < colon) : (k += 1) {
            const sc = s[k];
            if (!SchemaValidator.isAsciiAlnum(sc) && sc != '+' && sc != '-' && sc != '.') {
                return false;
            }
        }

        const rest = s[colon + 1 ..];
        if (rest.len >= 2 && rest[0] == '/' && rest[1] == '/') {
            // Authority must be non-empty before path/query/fragment
            if (rest.len == 2) return false;
            const after_slashes = rest[2];
            if (after_slashes == '/' || after_slashes == '?' || after_slashes == '#') return false;
        }
        return true;
    }

    pub static fn isValidUuid(s: []const u8): bool {
        if (s.len != 36) return false;
        let i: usize = 0;
        while (i < 36) : (i += 1) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                if (s[i] != '-') return false;
            } else {
                if (!SchemaValidator.isHexDigit(s[i])) return false;
            }
        }
        return true;
    }

    pub static fn isValidIpv4(s: []const u8): bool {
        if (s.len < 7 || s.len > 15) return false;
        let octets: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            const start = i;
            let val: u32 = 0;
            let digits: usize = 0;
            while (i < s.len && s[i] != '.') : (i += 1) {
                const c = s[i];
                if (!SchemaValidator.isAsciiDigit(c)) return false;
                digits += 1;
                if (digits > 3) return false;
                val = val * 10 + ((c - '0') as u32);
            }
            if (digits == 0 || val > 255) return false;
            // Reject leading zeros like "01" or "00"
            if (digits > 1 && s[start] == '0') return false;
            octets += 1;
            if (i < s.len) {
                if (s[i] != '.' || octets >= 4) return false;
                i += 1;
                if (i == s.len) return false; // trailing dot
            }
        }
        return octets == 4;
    }

    static fn parseTwoDigits(s: []const u8): u32? {
        if (s.len < 2) return null;
        if (!SchemaValidator.isAsciiDigit(s[0]) || !SchemaValidator.isAsciiDigit(s[1])) return null;
        return ((s[0] - '0') as u32) * 10 + ((s[1] - '0') as u32);
    }

    static fn parseFourDigits(s: []const u8): u32? {
        if (s.len < 4) return null;
        let i: usize = 0;
        let val: u32 = 0;
        while (i < 4) : (i += 1) {
            if (!SchemaValidator.isAsciiDigit(s[i])) return null;
            val = val * 10 + ((s[i] - '0') as u32);
        }
        return val;
    }

    static fn maxDaysInMonth(year: u32, month: u32): u32 {
        if (month == 2) {
            const is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return if (is_leap) 29 else 28;
        }
        return switch (month) {
            1, 3, 5, 7, 8, 10, 12 => 31,
            4, 6, 9, 11 => 30,
            else => 0,
        };
    }

    pub static fn isValidDateTime(s: []const u8): bool {
        // RFC 3339: YYYY-MM-DD[T|t]HH:MM:SS(.frac)?(Z|z|[+-]HH:MM)
        if (s.len < 20) return false;
        const year = SchemaValidator.parseFourDigits(s[0..4]) ?? return false;
        if (s[4] != '-') return false;
        const month = SchemaValidator.parseTwoDigits(s[5..7]) ?? return false;
        if (s[7] != '-') return false;
        const day = SchemaValidator.parseTwoDigits(s[8..10]) ?? return false;
        if (month < 1 || month > 12) return false;
        const max_day = SchemaValidator.maxDaysInMonth(year, month);
        if (day < 1 || day > max_day) return false;

        if (s[10] != 'T' && s[10] != 't') return false;
        const hour = SchemaValidator.parseTwoDigits(s[11..13]) ?? return false;
        if (s[13] != ':') return false;
        const min = SchemaValidator.parseTwoDigits(s[14..16]) ?? return false;
        if (s[16] != ':') return false;
        const sec = SchemaValidator.parseTwoDigits(s[17..19]) ?? return false;
        if (hour > 23 || min > 59 || sec > 60) return false;

        let pos: usize = 19;
        if (pos < s.len && s[pos] == '.') {
            pos += 1;
            let frac_digits: usize = 0;
            while (pos < s.len && SchemaValidator.isAsciiDigit(s[pos])) : (pos += 1) {
                frac_digits += 1;
            }
            if (frac_digits == 0) return false;
        }

        if (pos >= s.len) return false;
        if (s[pos] == 'Z' || s[pos] == 'z') {
            return pos + 1 == s.len;
        }
        if (s[pos] == '+' || s[pos] == '-') {
            if (pos + 6 != s.len) return false;
            const tz_h = SchemaValidator.parseTwoDigits(s[pos + 1 .. pos + 3]) ?? return false;
            if (s[pos + 3] != ':') return false;
            const tz_m = SchemaValidator.parseTwoDigits(s[pos + 4 .. pos + 6]) ?? return false;
            return tz_h <= 23 && tz_m <= 59;
        }
        return false;
    }

    static fn checkFormat(fmt: StringFormat, s: []const u8): bool {
        return switch (fmt) {
            .None => true,
            .Email => SchemaValidator.isValidEmail(s),
            .Uri => SchemaValidator.isValidUri(s),
            .Uuid => SchemaValidator.isValidUuid(s),
            .Ipv4 => SchemaValidator.isValidIpv4(s),
            .DateTime => SchemaValidator.isValidDateTime(s),
        };
    }

    static fn jsonTagName(val: JsonValue): string {
        return switch (val.tag) {
            .Null => "null",
            .Bool => "boolean",
            .Number => "number",
            .String => "string",
            .Array => "array",
            .Object => "object",
        };
    }

    static fn validateNode(
        alloc: std.mem.Allocator,
        schema: *const Schema,
        value: JsonValue,
        path_stack: *std.ArrayList(string),
        issues: *std.ArrayList(ValidationIssue),
        depth: usize,
    ): ! {
        if (depth > SchemaValidator.MAX_DEPTH) {
            SchemaValidator.pushIssue(
                alloc,
                issues,
                path_stack.items,
                "max_depth",
                "Maximum schema validation depth (64) exceeded",
            )?;
            return;
        }

        // Handle null values
        if (value.isNull()) {
            if (schema.schema_type == .Null || schema.schema_type == .Any || schema.nullable) {
                return;
            }
            let msg_buf: [128]u8 = undefined;
            const msg = std.fmt.bufPrint(&msg_buf, "Expected {s}, got null", .{schema.schema_type.toString()}) catch "Type mismatch: got null";
            SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
            return;
        }

        if (schema.schema_type == .Null) {
            let msg_buf: [128]u8 = undefined;
            const msg = std.fmt.bufPrint(&msg_buf, "Expected null, got {s}", .{SchemaValidator.jsonTagName(value)}) catch "Expected null";
            SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
            return;
        }

        if (schema.schema_type == .Any) {
            if (schema.enum_strings.len > 0) {
                if (s := value.asString()) {
                    let matched = false;
                    for (allowed of schema.enum_strings) {
                        if (std.mem.eql(u8, s, allowed)) {
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) {
                        SchemaValidator.pushIssue(alloc, issues, path_stack.items, "enum", "Value is not in allowed enum set")?;
                    }
                } else {
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "enum", "Value is not in allowed string enum set")?;
                }
            }
            return;
        }

        // Boolean check
        if (schema.schema_type == .Bool) {
            if (!value.isBool()) {
                let msg_buf: [128]u8 = undefined;
                const msg = std.fmt.bufPrint(&msg_buf, "Expected boolean, got {s}", .{SchemaValidator.jsonTagName(value)}) catch "Expected boolean";
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
            }
            return;
        }

        // Integer & Number checks
        if (schema.schema_type == .Integer || schema.schema_type == .Number) {
            if (!value.isNumber()) {
                let msg_buf: [128]u8 = undefined;
                const msg = std.fmt.bufPrint(&msg_buf, "Expected {s}, got {s}", .{ schema.schema_type.toString(), SchemaValidator.jsonTagName(value) }) catch "Expected number";
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
                return;
            }
            const n = value.num_val;
            if (std.math.isNan(n) || std.math.isInf(n)) {
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", "Number must be finite")?;
                return;
            }
            if (schema.schema_type == .Integer && std.math.floor(n) != n) {
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", "Expected integer, got non-integer number")?;
                return;
            }
            if (mv := schema.min_val) {
                if (n < mv) {
                    const msg = std.fmt.allocPrint(alloc, "Value {d} is less than minimum {d}", .{ n, mv })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "minimum", msg)?;
                }
            }
            if (mx := schema.max_val) {
                if (n > mx) {
                    const msg = std.fmt.allocPrint(alloc, "Value {d} exceeds maximum {d}", .{ n, mx })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "maximum", msg)?;
                }
            }
            if (em := schema.exclusive_min) {
                if (n <= em) {
                    const msg = std.fmt.allocPrint(alloc, "Value {d} must be strictly greater than {d}", .{ n, em })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "exclusive_minimum", msg)?;
                }
            }
            if (ex := schema.exclusive_max) {
                if (n >= ex) {
                    const msg = std.fmt.allocPrint(alloc, "Value {d} must be strictly less than {d}", .{ n, ex })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "exclusive_maximum", msg)?;
                }
            }
            if (mo := schema.multiple_of) {
                if (mo > 0.0) {
                    const ratio = n / mo;
                    const diff = @abs(ratio - @round(ratio));
                    if (diff > 0.000000001) {
                        const msg = std.fmt.allocPrint(alloc, "Value {d} is not a multiple of {d}", .{ n, mo })?;
                        defer alloc.free(msg);
                        SchemaValidator.pushIssue(alloc, issues, path_stack.items, "multiple_of", msg)?;
                    }
                }
            }
            return;
        }

        // String checks
        if (schema.schema_type == .String) {
            const s = value.asString() ?? {
                let msg_buf: [128]u8 = undefined;
                const msg = std.fmt.bufPrint(&msg_buf, "Expected string, got {s}", .{SchemaValidator.jsonTagName(value)}) catch "Expected string";
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
                return;
            };

            const cp_len = SchemaValidator.countCodepoints(s);
            if (ml := schema.min_len) {
                if (cp_len < ml) {
                    const msg = std.fmt.allocPrint(alloc, "String length {d} is less than minLength {d}", .{ cp_len, ml })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "min_length", msg)?;
                }
            }
            if (xl := schema.max_len) {
                if (cp_len > xl) {
                    const msg = std.fmt.allocPrint(alloc, "String length {d} exceeds maxLength {d}", .{ cp_len, xl })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "max_length", msg)?;
                }
            }
            if (schema.format != .None) {
                if (!SchemaValidator.checkFormat(schema.format, s)) {
                    const msg = std.fmt.allocPrint(alloc, "String does not match format '{s}'", .{schema.format.toString()})?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "format", msg)?;
                }
            }
            if (schema.enum_strings.len > 0) {
                let matched = false;
                for (allowed of schema.enum_strings) {
                    if (std.mem.eql(u8, s, allowed)) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "enum", "String value is not in allowed enum set")?;
                }
            }
            return;
        }

        // Array checks
        if (schema.schema_type == .Array) {
            const arr = value.asArray() ?? {
                let msg_buf: [128]u8 = undefined;
                const msg = std.fmt.bufPrint(&msg_buf, "Expected array, got {s}", .{SchemaValidator.jsonTagName(value)}) catch "Expected array";
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
                return;
            };

            if (mi := schema.min_items) {
                if (arr.len < mi) {
                    const msg = std.fmt.allocPrint(alloc, "Array length {d} is less than minItems {d}", .{ arr.len, mi })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "min_items", msg)?;
                }
            }
            if (xi := schema.max_items) {
                if (arr.len > xi) {
                    const msg = std.fmt.allocPrint(alloc, "Array length {d} exceeds maxItems {d}", .{ arr.len, xi })?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "max_items", msg)?;
                }
            }

            if (schema.unique_items && arr.len > 1) {
                let j: usize = 1;
                while (j < arr.len) : (j += 1) {
                    let k: usize = 0;
                    let dup = false;
                    while (k < j) : (k += 1) {
                        if (arr[j].equals(arr[k])) {
                            dup = true;
                            break;
                        }
                    }
                    if (dup) {
                        let idx_buf: [32]u8 = undefined;
                        const idx_str = std.fmt.bufPrint(&idx_buf, "{d}", .{j}) catch "0";
                        path_stack.append(alloc, idx_str)?;
                        SchemaValidator.pushIssue(alloc, issues, path_stack.items, "unique_items", "Duplicate item violates uniqueItems constraint")?;
                        * = path_stack.pop();
                    }
                }
            }

            if (item_s := schema.item_schema) {
                let i: usize = 0;
                while (i < arr.len) : (i += 1) {
                    let idx_buf: [32]u8 = undefined;
                    const idx_str = std.fmt.bufPrint(&idx_buf, "{d}", .{i}) catch "0";
                    path_stack.append(alloc, idx_str)?;
                    SchemaValidator.validateNode(alloc, item_s, arr[i], path_stack, issues, depth + 1)?;
                    * = path_stack.pop();
                }
            }
            return;
        }

        // Object checks
        if (schema.schema_type == .Object) {
            const fields = value.asObject() ?? {
                let msg_buf: [128]u8 = undefined;
                const msg = std.fmt.bufPrint(&msg_buf, "Expected object, got {s}", .{SchemaValidator.jsonTagName(value)}) catch "Expected object";
                SchemaValidator.pushIssue(alloc, issues, path_stack.items, "type_mismatch", msg)?;
                return;
            };

            // 1. Check required properties declared in `properties`
            for (prop of schema.properties) {
                if (prop.required && value.get(prop.name) == null) {
                    path_stack.append(alloc, prop.name)?;
                    const msg = std.fmt.allocPrint(alloc, "Missing required property '{s}'", .{prop.name})?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "required", msg)?;
                    * = path_stack.pop();
                }
            }

            // 2. Check extra_required properties (from JSON Schema `required` array not in `properties`)
            for (req_name of schema.extra_required) {
                if (value.get(req_name) == null) {
                    path_stack.append(alloc, req_name)?;
                    const msg = std.fmt.allocPrint(alloc, "Missing required property '{s}'", .{req_name})?;
                    defer alloc.free(msg);
                    SchemaValidator.pushIssue(alloc, issues, path_stack.items, "required", msg)?;
                    * = path_stack.pop();
                }
            }

            // 3. Validate each present field & enforce additionalProperties
            for (field of fields) {
                let matched_idx: usize? = null;
                let pi: usize = 0;
                while (pi < schema.properties.len) : (pi += 1) {
                    if (std.mem.eql(u8, field.key, schema.properties[pi].name)) {
                        matched_idx = pi;
                        break;
                    }
                }
                if (mi := matched_idx) {
                    path_stack.append(alloc, field.key)?;
                    SchemaValidator.validateNode(alloc, &schema.properties[mi].schema, field.value, path_stack, issues, depth + 1)?;
                    * = path_stack.pop();
                } else {
                    let is_extra_req = false;
                    for (rk of schema.extra_required) {
                        if (std.mem.eql(u8, field.key, rk)) {
                            is_extra_req = true;
                            break;
                        }
                    }
                    if (!is_extra_req && !schema.additional_properties) {
                        path_stack.append(alloc, field.key)?;
                        const msg = std.fmt.allocPrint(alloc, "Additional property '{s}' is not allowed", .{field.key})?;
                        defer alloc.free(msg);
                        SchemaValidator.pushIssue(alloc, issues, path_stack.items, "additional_property", msg)?;
                        * = path_stack.pop();
                    }
                }
            }
            return;
        }
    }

    pub static fn validate(alloc: std.mem.Allocator, schema: Schema, value: JsonValue): ValidationResult! {
        let path_stack: std.ArrayList(string) = .empty;
        defer path_stack.deinit(alloc);

        let issues: std.ArrayList(ValidationIssue) = .empty;
        errdefer {
            for (iss of issues.items) {
                if (iss.path.len > 0) alloc.free(iss.path);
                if (iss.message.len > 0) alloc.free(iss.message);
            }
            issues.deinit(alloc);
        }

        SchemaValidator.validateNode(alloc, &schema, value, &path_stack, &issues, 0)?;

        if (issues.items.len == 0) {
            issues.deinit(alloc);
            return ValidationResult{
                allocator: alloc,
                ok: true,
                issues: [],
            };
        }

        const owned = issues.toOwnedSlice(alloc)?;
        return ValidationResult{
            allocator: alloc,
            ok: false,
            issues: owned,
        };
    }

    pub static fn validateJson(alloc: std.mem.Allocator, schema: Schema, json_str: []const u8): ValidationResult! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();

        const val = JsonParser.parse(tmp_arena.allocator(), json_str)?;
        return SchemaValidator.validate(alloc, schema, val);
    }
}
