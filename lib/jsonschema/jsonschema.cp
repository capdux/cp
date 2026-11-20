// Source: cp/lib/jsonschema/jsonschema.cp (JSON Schema Draft-07/2020-12 Validator & Path Diagnostics)
// Pure CP JSON Schema validator supporting type, numeric/string/array/object constraints,
// enum/const, allOf/anyOf/oneOf/not combinators, and local $ref resolution with depth guard.

const std = @import("std");
import { JsonValue, JsonTag, JsonField, JsonParser } from "cap:json";

pub const JSONSCHEMA_MAX_DEPTH: usize = 64;

pub err JsonSchemaError {
    InvalidSchema,
    InvalidJson,
    InvalidReference,
    MaxDepthExceeded,
    OutOfMemory,
}

pub struct ValidationIssue {
    path: []const u8 = "$",
    keyword: []const u8 = "",
    message: []const u8 = "",
}

pub struct ValidationResult {
    valid: bool = true,
    issues: []ValidationIssue = [],

    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (issue of _.issues) {
            allocator.free(issue.path);
            allocator.free(issue.keyword);
            allocator.free(issue.message);
        }
        if (_.issues.len > 0) {
            allocator.free(_.issues);
        }
        _.issues = [];
        _.valid = true;
    }
}

pub struct SchemaValidator {
    allocator: std.mem.Allocator = undefined,
    schema_arena: *std.heap.ArenaAllocator = undefined,
    root_schema: JsonValue = .{},
    schema_src: []const u8 = "",
    max_depth: usize = JSONSCHEMA_MAX_DEPTH,

    pub static fn compile(allocator: std.mem.Allocator, schema_json: []const u8): SchemaValidator! {
        return SchemaValidator.compileWithMaxDepth(allocator, schema_json, JSONSCHEMA_MAX_DEPTH);
    }

    pub static fn compileWithMaxDepth(
        allocator: std.mem.Allocator,
        schema_json: []const u8,
        max_depth: usize,
    ): SchemaValidator! {
        let alloc_arena = allocator.create(std.heap.ArenaAllocator) catch return err.OutOfMemory;
        errdefer allocator.destroy(alloc_arena);
        alloc_arena.* = std.heap.ArenaAllocator.init(allocator);
        errdefer alloc_arena.deinit();

        const arena_alloc = alloc_arena.allocator();
        const src_copy = arena_alloc.dupe(u8, schema_json) catch return err.OutOfMemory;

        const parsed = JsonParser.parse(arena_alloc, src_copy) catch {
            return err.InvalidSchema;
        };

        validateSchemaSyntax(parsed, 0, max_depth)?;

        return SchemaValidator{
            allocator: allocator,
            schema_arena: alloc_arena,
            root_schema: parsed,
            schema_src: src_copy,
            max_depth: max_depth,
        };
    }

    pub fn deinit(*_) {
        _.schema_arena.deinit();
        _.allocator.destroy(_.schema_arena);
    }

    pub fn validate(allocator: std.mem.Allocator, instance_json: []const u8): ValidationResult! {
        let inst_arena = std.heap.ArenaAllocator.init(allocator);
        defer inst_arena.deinit();

        const instance_val = JsonParser.parse(inst_arena.allocator(), instance_json) catch {
            return err.InvalidJson;
        };

        return _.validateValue(allocator, instance_val);
    }

    pub fn validateValue(allocator: std.mem.Allocator, value: JsonValue): ValidationResult! {
        let issues: std.ArrayList(ValidationIssue) = .empty;
        errdefer freeIssueList(allocator, &issues);

        _.validateNode(allocator, _.root_schema, value, "$", &issues, 0)?;

        const valid = issues.items.len == 0;
        const owned = issues.toOwnedSlice(allocator) catch return err.OutOfMemory;
        return ValidationResult{
            valid: valid,
            issues: owned,
        };
    }

    fn validateNode(
        allocator: std.mem.Allocator,
        schema: JsonValue,
        value: JsonValue,
        path: []const u8,
        issues: *std.ArrayList(ValidationIssue),
        depth: usize,
    ): ! {
        if (depth > _.max_depth) {
            return err.MaxDepthExceeded;
        }

        if (schema.tag == .Bool) {
            if (!schema.bool_val) {
                addIssue(allocator, issues, path, "false", "value is disallowed by false schema")?;
            }
            return;
        }

        if (schema.tag != .Object) {
            return err.InvalidSchema;
        }

        // 1. $ref resolution
        if (ref_val := schema.get("$ref")) {
            if (ref_val.tag != .String) {
                return err.InvalidSchema;
            }
            const target = _.resolveLocalRef(ref_val.str_val)?;
            _.validateNode(allocator, target, value, path, issues, depth + 1)?;
        }

        // 2. type
        if (type_val := schema.get("type")) {
            let matched = false;
            if (type_val.tag == .String) {
                matched = matchesJsonType(value, type_val.str_val);
                if (!matched) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "expected type '{s}', got '{s}'",
                        .{ type_val.str_val, jsonTypeName(value) },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "type", msg)?;
                }
            } else if (type_val.tag == .Array) {
                for (item of type_val.arr_val) {
                    if (item.tag == .String && matchesJsonType(value, item.str_val)) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "value of type '{s}' does not match any allowed type",
                        .{jsonTypeName(value)},
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "type", msg)?;
                }
            } else {
                return err.InvalidSchema;
            }
        }

        // 3. const
        if (const_val := schema.get("const")) {
            if (!value.equals(const_val)) {
                addIssue(allocator, issues, path, "const", "value does not match const constraint")?;
            }
        }

        // 4. enum
        if (enum_val := schema.get("enum")) {
            if (enum_val.tag != .Array) {
                return err.InvalidSchema;
            }
            let in_enum = false;
            for (candidate of enum_val.arr_val) {
                if (value.equals(candidate)) {
                    in_enum = true;
                    break;
                }
            }
            if (!in_enum) {
                addIssue(allocator, issues, path, "enum", "value is not one of the allowed enum values")?;
            }
        }

        // 5. Numeric constraints
        if (value.tag == .Number) {
            const num = value.num_val;
            if (min_val := schema.get("minimum")) {
                if (min_val.tag != .Number) return err.InvalidSchema;
                if (num < min_val.num_val) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "value {d} is less than minimum {d}",
                        .{ num, min_val.num_val },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "minimum", msg)?;
                }
            }
            if (max_val := schema.get("maximum")) {
                if (max_val.tag != .Number) return err.InvalidSchema;
                if (num > max_val.num_val) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "value {d} is greater than maximum {d}",
                        .{ num, max_val.num_val },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "maximum", msg)?;
                }
            }
            if (emin_val := schema.get("exclusiveMinimum")) {
                if (emin_val.tag != .Number) return err.InvalidSchema;
                if (num <= emin_val.num_val) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "value {d} must be strictly greater than {d}",
                        .{ num, emin_val.num_val },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "exclusiveMinimum", msg)?;
                }
            }
            if (emax_val := schema.get("exclusiveMaximum")) {
                if (emax_val.tag != .Number) return err.InvalidSchema;
                if (num >= emax_val.num_val) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "value {d} must be strictly less than {d}",
                        .{ num, emax_val.num_val },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "exclusiveMaximum", msg)?;
                }
            }
            if (mult_val := schema.get("multipleOf")) {
                if (mult_val.tag != .Number || mult_val.num_val <= 0.0) return err.InvalidSchema;
                if (!isMultipleOf(num, mult_val.num_val)) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "value {d} is not a multiple of {d}",
                        .{ num, mult_val.num_val },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "multipleOf", msg)?;
                }
            }
        }

        // 6. String constraints (UTF-8 codepoint length)
        if (value.tag == .String) {
            const cp_len = countUtf8Codepoints(value.str_val);
            if (min_len_val := schema.get("minLength")) {
                const min_len = parseNonNegInt(min_len_val) ?? return err.InvalidSchema;
                if (cp_len < min_len) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "string length {d} is less than minLength {d}",
                        .{ cp_len, min_len },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "minLength", msg)?;
                }
            }
            if (max_len_val := schema.get("maxLength")) {
                const max_len = parseNonNegInt(max_len_val) ?? return err.InvalidSchema;
                if (cp_len > max_len) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "string length {d} exceeds maxLength {d}",
                        .{ cp_len, max_len },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "maxLength", msg)?;
                }
            }
        }

        // 7. Array constraints
        if (value.tag == .Array) {
            const arr = value.arr_val;
            if (min_items_val := schema.get("minItems")) {
                const min_items = parseNonNegInt(min_items_val) ?? return err.InvalidSchema;
                if (arr.len < min_items) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "array has {d} items, minimum is {d}",
                        .{ arr.len, min_items },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "minItems", msg)?;
                }
            }
            if (max_items_val := schema.get("maxItems")) {
                const max_items = parseNonNegInt(max_items_val) ?? return err.InvalidSchema;
                if (arr.len > max_items) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "array has {d} items, maximum is {d}",
                        .{ arr.len, max_items },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "maxItems", msg)?;
                }
            }
            if (uniq_val := schema.get("uniqueItems")) {
                if (uniq_val.tag != .Bool) return err.InvalidSchema;
                if (uniq_val.bool_val) {
                    let i: usize = 0;
                    let found_dup = false;
                    while (i < arr.len && !found_dup) : (i += 1) {
                        let j: usize = i + 1;
                        while (j < arr.len) : (j += 1) {
                            if (arr[i].equals(arr[j])) {
                                const elem_path = std.fmt.allocPrint(allocator, "{s}[{d}]", .{ path, j }) catch return err.OutOfMemory;
                                defer allocator.free(elem_path);
                                const msg = std.fmt.allocPrint(
                                    allocator,
                                    "duplicate array item at index {d} (matches index {d})",
                                    .{ j, i },
                                ) catch return err.OutOfMemory;
                                defer allocator.free(msg);
                                addIssue(allocator, issues, elem_path, "uniqueItems", msg)?;
                                found_dup = true;
                                break;
                            }
                        }
                    }
                }
            }
            if (items_val := schema.get("items")) {
                if (items_val.tag == .Array) {
                    const limit = @min(arr.len, items_val.arr_val.len);
                    let idx: usize = 0;
                    while (idx < limit) : (idx += 1) {
                        const elem_path = std.fmt.allocPrint(allocator, "{s}[{d}]", .{ path, idx }) catch return err.OutOfMemory;
                        defer allocator.free(elem_path);
                        _.validateNode(allocator, items_val.arr_val[idx], arr[idx], elem_path, issues, depth + 1)?;
                    }
                } else {
                    let idx: usize = 0;
                    while (idx < arr.len) : (idx += 1) {
                        const elem_path = std.fmt.allocPrint(allocator, "{s}[{d}]", .{ path, idx }) catch return err.OutOfMemory;
                        defer allocator.free(elem_path);
                        _.validateNode(allocator, items_val, arr[idx], elem_path, issues, depth + 1)?;
                    }
                }
            }
        }

        // 8. Object constraints
        if (value.tag == .Object) {
            const obj = value.obj_val;
            if (min_props_val := schema.get("minProperties")) {
                const min_props = parseNonNegInt(min_props_val) ?? return err.InvalidSchema;
                if (obj.len < min_props) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "object has {d} properties, minimum is {d}",
                        .{ obj.len, min_props },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "minProperties", msg)?;
                }
            }
            if (max_props_val := schema.get("maxProperties")) {
                const max_props = parseNonNegInt(max_props_val) ?? return err.InvalidSchema;
                if (obj.len > max_props) {
                    const msg = std.fmt.allocPrint(
                        allocator,
                        "object has {d} properties, maximum is {d}",
                        .{ obj.len, max_props },
                    ) catch return err.OutOfMemory;
                    defer allocator.free(msg);
                    addIssue(allocator, issues, path, "maxProperties", msg)?;
                }
            }
            if (req_val := schema.get("required")) {
                if (req_val.tag != .Array) return err.InvalidSchema;
                for (req_item of req_val.arr_val) {
                    if (req_item.tag != .String) return err.InvalidSchema;
                    if (value.get(req_item.str_val) == null) {
                        const req_path = std.fmt.allocPrint(allocator, "{s}.{s}", .{ path, req_item.str_val }) catch return err.OutOfMemory;
                        defer allocator.free(req_path);
                        const msg = std.fmt.allocPrint(
                            allocator,
                            "missing required property '{s}'",
                            .{req_item.str_val},
                        ) catch return err.OutOfMemory;
                        defer allocator.free(msg);
                        addIssue(allocator, issues, req_path, "required", msg)?;
                    }
                }
            }

            const props_opt = schema.get("properties");
            if (props_val := props_opt) {
                if (props_val.tag != .Object) return err.InvalidSchema;
                for (prop_field of props_val.obj_val) {
                    if (inst_prop := value.get(prop_field.key)) {
                        const prop_path = std.fmt.allocPrint(allocator, "{s}.{s}", .{ path, prop_field.key }) catch return err.OutOfMemory;
                        defer allocator.free(prop_path);
                        _.validateNode(allocator, prop_field.value, inst_prop, prop_path, issues, depth + 1)?;
                    }
                }
            }

            if (add_props_val := schema.get("additionalProperties")) {
                for (inst_field of obj) {
                    let is_declared = false;
                    if (props_val := props_opt) {
                        if (props_val.get(inst_field.key) != null) {
                            is_declared = true;
                        }
                    }
                    if (!is_declared) {
                        const add_path = std.fmt.allocPrint(allocator, "{s}.{s}", .{ path, inst_field.key }) catch return err.OutOfMemory;
                        defer allocator.free(add_path);
                        if (add_props_val.tag == .Bool) {
                            if (!add_props_val.bool_val) {
                                const msg = std.fmt.allocPrint(
                                    allocator,
                                    "additional property '{s}' is not allowed",
                                    .{inst_field.key},
                                ) catch return err.OutOfMemory;
                                defer allocator.free(msg);
                                addIssue(allocator, issues, add_path, "additionalProperties", msg)?;
                            }
                        } else if (add_props_val.tag == .Object) {
                            _.validateNode(allocator, add_props_val, inst_field.value, add_path, issues, depth + 1)?;
                        } else {
                            return err.InvalidSchema;
                        }
                    }
                }
            }
        }

        // 9. Combinators: allOf, anyOf, oneOf, not
        if (all_of := schema.get("allOf")) {
            if (all_of.tag != .Array) return err.InvalidSchema;
            for (sub of all_of.arr_val) {
                _.validateNode(allocator, sub, value, path, issues, depth + 1)?;
            }
        }

        if (any_of := schema.get("anyOf")) {
            if (any_of.tag != .Array) return err.InvalidSchema;
            let any_matched = false;
            for (sub of any_of.arr_val) {
                let sub_issues: std.ArrayList(ValidationIssue) = .empty;
                defer freeIssueList(allocator, &sub_issues);
                _.validateNode(allocator, sub, value, path, &sub_issues, depth + 1)?;
                if (sub_issues.items.len == 0) {
                    any_matched = true;
                    break;
                }
            }
            if (!any_matched) {
                addIssue(allocator, issues, path, "anyOf", "value does not match any schema in anyOf")?;
            }
        }

        if (one_of := schema.get("oneOf")) {
            if (one_of.tag != .Array) return err.InvalidSchema;
            let match_count: usize = 0;
            for (sub of one_of.arr_val) {
                let sub_issues: std.ArrayList(ValidationIssue) = .empty;
                defer freeIssueList(allocator, &sub_issues);
                _.validateNode(allocator, sub, value, path, &sub_issues, depth + 1)?;
                if (sub_issues.items.len == 0) {
                    match_count += 1;
                }
            }
            if (match_count != 1) {
                const msg = std.fmt.allocPrint(
                    allocator,
                    "value matched {d} schemas in oneOf, expected exactly 1",
                    .{match_count},
                ) catch return err.OutOfMemory;
                defer allocator.free(msg);
                addIssue(allocator, issues, path, "oneOf", msg)?;
            }
        }

        if (not_schema := schema.get("not")) {
            let sub_issues: std.ArrayList(ValidationIssue) = .empty;
            defer freeIssueList(allocator, &sub_issues);
            _.validateNode(allocator, not_schema, value, path, &sub_issues, depth + 1)?;
            if (sub_issues.items.len == 0) {
                addIssue(allocator, issues, path, "not", "value must not match the schema in 'not'")?;
            }
        }
    }

    fn resolveLocalRef(ref_str: []const u8): JsonValue! {
        if (ref_str.len == 0 || ref_str[0] != '#') {
            return err.InvalidReference;
        }
        if (ref_str.len == 1) {
            return _.root_schema;
        }
        if (ref_str[1] != '/') {
            return err.InvalidReference;
        }
        if (target := _.root_schema.pointer(ref_str[1..])) {
            return target;
        }
        return err.InvalidReference;
    }
}

fn freeIssueList(allocator: std.mem.Allocator, list: *std.ArrayList(ValidationIssue)) {
    for (issue of list.items) {
        allocator.free(issue.path);
        allocator.free(issue.keyword);
        allocator.free(issue.message);
    }
    list.deinit(allocator);
}

fn addIssue(
    allocator: std.mem.Allocator,
    list: *std.ArrayList(ValidationIssue),
    path: []const u8,
    keyword: []const u8,
    message: []const u8,
): ! {
    const p = allocator.dupe(u8, path) catch return err.OutOfMemory;
    errdefer allocator.free(p);
    const k = allocator.dupe(u8, keyword) catch return err.OutOfMemory;
    errdefer allocator.free(k);
    const m = allocator.dupe(u8, message) catch return err.OutOfMemory;
    errdefer allocator.free(m);

    list.append(allocator, ValidationIssue{
        path: p,
        keyword: k,
        message: m,
    }) catch return err.OutOfMemory;
}

fn isValidTypeName(t: []const u8): bool {
    return std.mem.eql(u8, t, "null") ||
        std.mem.eql(u8, t, "boolean") ||
        std.mem.eql(u8, t, "object") ||
        std.mem.eql(u8, t, "array") ||
        std.mem.eql(u8, t, "number") ||
        std.mem.eql(u8, t, "integer") ||
        std.mem.eql(u8, t, "string");
}

fn matchesJsonType(val: JsonValue, type_name: []const u8): bool {
    if (std.mem.eql(u8, type_name, "null")) return val.tag == .Null;
    if (std.mem.eql(u8, type_name, "boolean")) return val.tag == .Bool;
    if (std.mem.eql(u8, type_name, "string")) return val.tag == .String;
    if (std.mem.eql(u8, type_name, "array")) return val.tag == .Array;
    if (std.mem.eql(u8, type_name, "object")) return val.tag == .Object;
    if (std.mem.eql(u8, type_name, "number")) return val.tag == .Number;
    if (std.mem.eql(u8, type_name, "integer")) {
        if (val.tag != .Number) return false;
        return std.math.isFinite(val.num_val) && @floor(val.num_val) == val.num_val;
    }
    return false;
}

fn jsonTypeName(val: JsonValue): []const u8 {
    return switch (val.tag) {
        .Null => "null",
        .Bool => "boolean",
        .Number => {
            if (std.math.isFinite(val.num_val) && @floor(val.num_val) == val.num_val) {
                return "integer";
            }
            return "number";
        },
        .String => "string",
        .Array => "array",
        .Object => "object",
    };
}

fn isMultipleOf(val: f64, divisor: f64): bool {
    if (!std.math.isFinite(val) || !std.math.isFinite(divisor) || divisor <= 0.0) {
        return false;
    }
    const ratio = val / divisor;
    if (!std.math.isFinite(ratio)) return false;
    const rounded = @round(ratio);
    const diff = @abs(ratio - rounded);
    const tol = 0.000000001 * @max(1.0, @abs(rounded));
    return diff <= tol;
}

fn countUtf8Codepoints(s: []const u8): usize {
    let count: usize = 0;
    for (b of s) {
        if ((b & 0xC0) != 0x80) {
            count += 1;
        }
    }
    return count;
}

fn parseNonNegInt(val: JsonValue): usize? {
    if (val.tag != .Number) return null;
    if (!std.math.isFinite(val.num_val) || val.num_val < 0.0) return null;
    if (@floor(val.num_val) != val.num_val) return null;
    return val.num_val as usize;
}

fn validateSchemaSyntax(schema: JsonValue, depth: usize, max_depth: usize): ! {
    if (depth > max_depth) {
        return err.MaxDepthExceeded;
    }
    if (schema.tag == .Bool) {
        return;
    }
    if (schema.tag != .Object) {
        return err.InvalidSchema;
    }

    if (t := schema.get("type")) {
        if (t.tag == .String) {
            if (!isValidTypeName(t.str_val)) return err.InvalidSchema;
        } else if (t.tag == .Array) {
            if (t.arr_val.len == 0) return err.InvalidSchema;
            for (item of t.arr_val) {
                if (item.tag != .String || !isValidTypeName(item.str_val)) {
                    return err.InvalidSchema;
                }
            }
        } else {
            return err.InvalidSchema;
        }
    }

    if (v := schema.get("minimum")) {
        if (v.tag != .Number || !std.math.isFinite(v.num_val)) return err.InvalidSchema;
    }
    if (v := schema.get("maximum")) {
        if (v.tag != .Number || !std.math.isFinite(v.num_val)) return err.InvalidSchema;
    }
    if (v := schema.get("exclusiveMinimum")) {
        if (v.tag != .Number || !std.math.isFinite(v.num_val)) return err.InvalidSchema;
    }
    if (v := schema.get("exclusiveMaximum")) {
        if (v.tag != .Number || !std.math.isFinite(v.num_val)) return err.InvalidSchema;
    }
    if (v := schema.get("multipleOf")) {
        if (v.tag != .Number || !std.math.isFinite(v.num_val) || v.num_val <= 0.0) {
            return err.InvalidSchema;
        }
    }

    if (v := schema.get("minLength")) {
        if (parseNonNegInt(v) == null) return err.InvalidSchema;
    }
    if (v := schema.get("maxLength")) {
        if (parseNonNegInt(v) == null) return err.InvalidSchema;
    }
    if (v := schema.get("minItems")) {
        if (parseNonNegInt(v) == null) return err.InvalidSchema;
    }
    if (v := schema.get("maxItems")) {
        if (parseNonNegInt(v) == null) return err.InvalidSchema;
    }
    if (v := schema.get("minProperties")) {
        if (parseNonNegInt(v) == null) return err.InvalidSchema;
    }
    if (v := schema.get("maxProperties")) {
        if (parseNonNegInt(v) == null) return err.InvalidSchema;
    }
    if (v := schema.get("uniqueItems")) {
        if (v.tag != .Bool) return err.InvalidSchema;
    }

    if (v := schema.get("required")) {
        if (v.tag != .Array) return err.InvalidSchema;
        for (item of v.arr_val) {
            if (item.tag != .String) return err.InvalidSchema;
        }
    }

    if (v := schema.get("properties")) {
        if (v.tag != .Object) return err.InvalidSchema;
        for (f of v.obj_val) {
            validateSchemaSyntax(f.value, depth + 1, max_depth)?;
        }
    }

    if (v := schema.get("additionalProperties")) {
        validateSchemaSyntax(v, depth + 1, max_depth)?;
    }

    if (v := schema.get("items")) {
        if (v.tag == .Array) {
            for (sub of v.arr_val) {
                validateSchemaSyntax(sub, depth + 1, max_depth)?;
            }
        } else {
            validateSchemaSyntax(v, depth + 1, max_depth)?;
        }
    }

    if (v := schema.get("enum")) {
        if (v.tag != .Array || v.arr_val.len == 0) return err.InvalidSchema;
    }

    if (v := schema.get("allOf")) {
        if (v.tag != .Array || v.arr_val.len == 0) return err.InvalidSchema;
        for (sub of v.arr_val) {
            validateSchemaSyntax(sub, depth + 1, max_depth)?;
        }
    }
    if (v := schema.get("anyOf")) {
        if (v.tag != .Array || v.arr_val.len == 0) return err.InvalidSchema;
        for (sub of v.arr_val) {
            validateSchemaSyntax(sub, depth + 1, max_depth)?;
        }
    }
    if (v := schema.get("oneOf")) {
        if (v.tag != .Array || v.arr_val.len == 0) return err.InvalidSchema;
        for (sub of v.arr_val) {
            validateSchemaSyntax(sub, depth + 1, max_depth)?;
        }
    }
    if (v := schema.get("not")) {
        validateSchemaSyntax(v, depth + 1, max_depth)?;
    }

    if (v := schema.get("$ref")) {
        if (v.tag != .String) return err.InvalidSchema;
    }

    if (v := schema.get("$defs")) {
        if (v.tag != .Object) return err.InvalidSchema;
        for (f of v.obj_val) {
            validateSchemaSyntax(f.value, depth + 1, max_depth)?;
        }
    }
    if (v := schema.get("definitions")) {
        if (v.tag != .Object) return err.InvalidSchema;
        for (f of v.obj_val) {
            validateSchemaSyntax(f.value, depth + 1, max_depth)?;
        }
    }
}
