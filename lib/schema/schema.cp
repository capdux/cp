// Source: cp/lib/schema/schema.cp (JSON Schema Draft 2020-12 / AI Tool Parameter Schema AST)
// Pure CP JSON Schema AST, fluent builder & JSON Schema Draft 2020-12 / OpenAI / MCP parser.

const std = @import("std");
import { JsonValue, JsonTag, JsonField, JsonParser } from "cap:json";

type Str = []const u8;

pub err SchemaParseError {
    InvalidSchema,
    UnsupportedSchemaType,
    MaxDepthExceeded,
}

pub enum SchemaType: u8 {
    Any = 0,
    Null = 1,
    Bool = 2,
    Integer = 3,
    Number = 4,
    String = 5,
    Array = 6,
    Object = 7,

    pub fn toString(): string {
        return switch (_) {
            .Any => "any",
            .Null => "null",
            .Bool => "boolean",
            .Integer => "integer",
            .Number => "number",
            .String => "string",
            .Array => "array",
            .Object => "object",
        };
    }

    pub static fn fromString(s: []const u8): SchemaType? {
        if (std.mem.eql(u8, s, "null")) return .Null;
        if (std.mem.eql(u8, s, "boolean")) return .Bool;
        if (std.mem.eql(u8, s, "integer")) return .Integer;
        if (std.mem.eql(u8, s, "number")) return .Number;
        if (std.mem.eql(u8, s, "string")) return .String;
        if (std.mem.eql(u8, s, "array")) return .Array;
        if (std.mem.eql(u8, s, "object")) return .Object;
        if (std.mem.eql(u8, s, "any")) return .Any;
        return null;
    }
}

pub enum StringFormat: u8 {
    None = 0,
    Email = 1,
    Uri = 2,
    Uuid = 3,
    Ipv4 = 4,
    DateTime = 5,

    pub fn toString(): string {
        return switch (_) {
            .None => "none",
            .Email => "email",
            .Uri => "uri",
            .Uuid => "uuid",
            .Ipv4 => "ipv4",
            .DateTime => "date-time",
        };
    }

    pub static fn fromString(s: []const u8): StringFormat? {
        if (std.mem.eql(u8, s, "email")) return .Email;
        if (std.mem.eql(u8, s, "uri") || std.mem.eql(u8, s, "url")) return .Uri;
        if (std.mem.eql(u8, s, "uuid")) return .Uuid;
        if (std.mem.eql(u8, s, "ipv4")) return .Ipv4;
        if (std.mem.eql(u8, s, "date-time") || std.mem.eql(u8, s, "datetime")) return .DateTime;
        return null;
    }
}

pub struct PropertySchema {
    name: []const u8 = "",
    schema: Schema = Schema{},
    required: bool = true,

    pub static fn init(name: []const u8, schema: Schema, required: bool): PropertySchema {
        return PropertySchema{
            name: name,
            schema: schema,
            required: required,
        };
    }

    pub static fn req(name: []const u8, schema: Schema): PropertySchema {
        return PropertySchema{
            name: name,
            schema: schema,
            required: true,
        };
    }

    pub static fn opt(name: []const u8, schema: Schema): PropertySchema {
        return PropertySchema{
            name: name,
            schema: schema,
            required: false,
        };
    }
}

pub struct Schema {
    schema_type: SchemaType = .Any,
    nullable: bool = false,

    // String constraints
    min_len: usize? = null,
    max_len: usize? = null,
    format: StringFormat = .None,

    // Number / Integer constraints
    min_val: f64? = null,
    max_val: f64? = null,
    exclusive_min: f64? = null,
    exclusive_max: f64? = null,
    multiple_of: f64? = null,

    // Array constraints
    min_items: usize? = null,
    max_items: usize? = null,
    unique_items: bool = false,
    item_schema: (*const Schema)? = null,

    // Object constraints
    properties: []const PropertySchema = [],
    extra_required: []const []const u8 = [],
    additional_properties: bool = true,

    // Enumerations & metadata
    enum_strings: []const []const u8 = [],
    title: []const u8 = "",
    description: []const u8 = "",

    // Owned arena for schemas parsed via Schema.fromJson / Schema.fromJsonValue
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    pub static const MAX_PARSE_DEPTH: usize = 64;

    pub static fn any(): Schema {
        return Schema{ schema_type: .Any };
    }

    pub static fn nullType(): Schema {
        return Schema{ schema_type: .Null, nullable: true };
    }

    pub static fn boolean(): Schema {
        return Schema{ schema_type: .Bool };
    }

    pub static fn integer(): Schema {
        return Schema{ schema_type: .Integer };
    }

    pub static fn number(): Schema {
        return Schema{ schema_type: .Number };
    }

    pub static fn string(): Schema {
        return Schema{ schema_type: .String };
    }

    pub static fn array(item_schema: *const Schema): Schema {
        return Schema{
            schema_type: .Array,
            item_schema: item_schema,
        };
    }

    pub static fn object(props: []const PropertySchema): Schema {
        return Schema{
            schema_type: .Object,
            properties: props,
        };
    }

    pub static fn enumStrings(vals: []const []const u8): Schema {
        return Schema{
            schema_type: .String,
            enum_strings: vals,
        };
    }

    fn copyBase(): Schema {
        return Schema{
            schema_type: _.schema_type,
            nullable: _.nullable,
            min_len: _.min_len,
            max_len: _.max_len,
            format: _.format,
            min_val: _.min_val,
            max_val: _.max_val,
            exclusive_min: _.exclusive_min,
            exclusive_max: _.exclusive_max,
            multiple_of: _.multiple_of,
            min_items: _.min_items,
            max_items: _.max_items,
            unique_items: _.unique_items,
            item_schema: _.item_schema,
            properties: _.properties,
            extra_required: _.extra_required,
            additional_properties: _.additional_properties,
            enum_strings: _.enum_strings,
            title: _.title,
            description: _.description,
            owned_arena: _.owned_arena,
            parent_alloc: _.parent_alloc,
        };
    }

    pub fn minLen(n: usize): Schema {
        let copy = _.copyBase();
        copy.min_len = n;
        return copy;
    }

    pub fn maxLen(n: usize): Schema {
        let copy = _.copyBase();
        copy.max_len = n;
        return copy;
    }

    pub fn withFormat(fmt: StringFormat): Schema {
        let copy = _.copyBase();
        copy.format = fmt;
        return copy;
    }

    pub fn minVal(v: f64): Schema {
        let copy = _.copyBase();
        copy.min_val = v;
        return copy;
    }

    pub fn maxVal(v: f64): Schema {
        let copy = _.copyBase();
        copy.max_val = v;
        return copy;
    }

    pub fn exclusiveMin(v: f64): Schema {
        let copy = _.copyBase();
        copy.exclusive_min = v;
        return copy;
    }

    pub fn exclusiveMax(v: f64): Schema {
        let copy = _.copyBase();
        copy.exclusive_max = v;
        return copy;
    }

    pub fn multipleOf(v: f64): Schema {
        let copy = _.copyBase();
        copy.multiple_of = v;
        return copy;
    }

    pub fn minItems(n: usize): Schema {
        let copy = _.copyBase();
        copy.min_items = n;
        return copy;
    }

    pub fn maxItems(n: usize): Schema {
        let copy = _.copyBase();
        copy.max_items = n;
        return copy;
    }

    pub fn withUniqueItems(unique: bool): Schema {
        let copy = _.copyBase();
        copy.unique_items = unique;
        return copy;
    }

    pub fn withAdditionalProperties(allow: bool): Schema {
        let copy = _.copyBase();
        copy.additional_properties = allow;
        return copy;
    }

    pub fn withNullable(allow: bool): Schema {
        let copy = _.copyBase();
        copy.nullable = allow;
        return copy;
    }

    pub fn withEnumStrings(vals: []const []const u8): Schema {
        let copy = _.copyBase();
        copy.enum_strings = vals;
        return copy;
    }

    pub fn withTitle(t: []const u8): Schema {
        let copy = _.copyBase();
        copy.title = t;
        return copy;
    }

    pub fn withDescription(d: []const u8): Schema {
        let copy = _.copyBase();
        copy.description = d;
        return copy;
    }

    pub fn deinit(*_) {
        if (ar := _.owned_arena) {
            ar.deinit();
            if (palloc := _.parent_alloc) {
                palloc.destroy(ar);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
    }

    static fn parseNonNegUsize(val: JsonValue): usize! {
        if (!val.isNumber()) return err.InvalidSchema;
        const n = val.num_val;
        if (std.math.isNan(n) || std.math.isInf(n) || n < 0.0) {
            return err.InvalidSchema;
        }
        if (std.math.floor(n) != n) {
            return err.InvalidSchema;
        }
        return (n as usize);
    }

    static fn parseFiniteNumber(val: JsonValue): f64! {
        if (!val.isNumber()) return err.InvalidSchema;
        const n = val.num_val;
        if (std.math.isNan(n) || std.math.isInf(n)) {
            return err.InvalidSchema;
        }
        return n;
    }

    static fn dupeStr(arena_alloc: std.mem.Allocator, s: []const u8): ([]const u8)! {
        if (s.len == 0) return "";
        return arena_alloc.dupe(u8, s);
    }

    static fn containsString(list: []const []const u8, needle: []const u8): bool {
        for (item of list) {
            if (std.mem.eql(u8, item, needle)) return true;
        }
        return false;
    }

    static fn parseNode(arena_alloc: std.mem.Allocator, node: JsonValue, depth: usize): Schema! {
        if (depth > Schema.MAX_PARSE_DEPTH) {
            return err.MaxDepthExceeded;
        }
        if (node.isBool()) {
            // JSON Schema Draft 2020-12 boolean schema: `true` accepts any, `false` is unsupported/rejects
            if (node.bool_val) return Schema.any();
            return err.InvalidSchema;
        }
        if (!node.isObject()) {
            return err.InvalidSchema;
        }

        let s = Schema{};
        let has_explicit_type = false;

        if (type_val := node.get("type")) {
            has_explicit_type = true;
            if (type_str := type_val.asString()) {
                const parsed_t = SchemaType.fromString(type_str) ?? return err.UnsupportedSchemaType;
                s.schema_type = parsed_t;
                if (parsed_t == .Null) {
                    s.nullable = true;
                }
            } else if (type_arr := type_val.asArray()) {
                let primary_type: SchemaType? = null;
                let saw_null = false;
                for (elem of type_arr) {
                    const elem_str = elem.asString() ?? return err.InvalidSchema;
                    const parsed_t = SchemaType.fromString(elem_str) ?? return err.UnsupportedSchemaType;
                    if (parsed_t == .Null) {
                        saw_null = true;
                    } else if (primary_type == null) {
                        primary_type = parsed_t;
                    } else if (primary_type.? != parsed_t) {
                        return err.UnsupportedSchemaType;
                    }
                }
                if (pt := primary_type) {
                    s.schema_type = pt;
                    s.nullable = saw_null;
                } else if (saw_null) {
                    s.schema_type = .Null;
                    s.nullable = true;
                } else {
                    return err.InvalidSchema;
                }
            } else {
                return err.InvalidSchema;
            }
        }

        if (nullable_val := node.get("nullable")) {
            const nb = nullable_val.asBool() ?? return err.InvalidSchema;
            if (nb) s.nullable = true;
        }

        if (title_val := node.get("title")) {
            if (ts := title_val.asString()) {
                s.title = Schema.dupeStr(arena_alloc, ts)?;
            }
        }

        if (desc_val := node.get("description")) {
            if (ds := desc_val.asString()) {
                s.description = Schema.dupeStr(arena_alloc, ds)?;
            }
        }

        // String constraints
        if (v := node.get("minLength")) {
            s.min_len = Schema.parseNonNegUsize(v)?;
        }
        if (v := node.get("maxLength")) {
            s.max_len = Schema.parseNonNegUsize(v)?;
        }
        if (v := node.get("format")) {
            const fmt_str = v.asString() ?? return err.InvalidSchema;
            if (fmt := StringFormat.fromString(fmt_str)) {
                s.format = fmt;
            }
        }

        // Number / Integer constraints
        if (v := node.get("minimum")) {
            s.min_val = Schema.parseFiniteNumber(v)?;
        }
        if (v := node.get("maximum")) {
            s.max_val = Schema.parseFiniteNumber(v)?;
        }
        if (v := node.get("exclusiveMinimum")) {
            s.exclusive_min = Schema.parseFiniteNumber(v)?;
        }
        if (v := node.get("exclusiveMaximum")) {
            s.exclusive_max = Schema.parseFiniteNumber(v)?;
        }
        if (v := node.get("multipleOf")) {
            const mo = Schema.parseFiniteNumber(v)?;
            if (mo <= 0.0) return err.InvalidSchema;
            s.multiple_of = mo;
        }

        // Enum & Const
        if (enum_val := node.get("enum")) {
            const arr = enum_val.asArray() ?? return err.InvalidSchema;
            let enum_list: std.ArrayList(Str) = .empty;
            for (item of arr) {
                const str_item = item.asString() ?? return err.UnsupportedSchemaType;
                enum_list.append(arena_alloc, Schema.dupeStr(arena_alloc, str_item)?)?;
            }
            s.enum_strings = enum_list.toOwnedSlice(arena_alloc)?;
            if (!has_explicit_type) {
                s.schema_type = .String;
            }
        } else if (const_val := node.get("const")) {
            if (cs := const_val.asString()) {
                let const_list: std.ArrayList(Str) = .empty;
                const_list.append(arena_alloc, Schema.dupeStr(arena_alloc, cs)?)?;
                s.enum_strings = const_list.toOwnedSlice(arena_alloc)?;
                if (!has_explicit_type) {
                    s.schema_type = .String;
                }
            }
        }

        // Array constraints
        if (v := node.get("minItems")) {
            s.min_items = Schema.parseNonNegUsize(v)?;
        }
        if (v := node.get("maxItems")) {
            s.max_items = Schema.parseNonNegUsize(v)?;
        }
        if (v := node.get("uniqueItems")) {
            const ui = v.asBool() ?? return err.InvalidSchema;
            s.unique_items = ui;
        }
        if (items_val := node.get("items")) {
            const child = Schema.parseNode(arena_alloc, items_val, depth + 1)?;
            let child_ptr = arena_alloc.create(Schema)?;
            unsafe {
                child_ptr.* = child;
            }
            s.item_schema = child_ptr;
            if (!has_explicit_type) {
                s.schema_type = .Array;
            }
        }

        // Object constraints
        let req_keys: std.ArrayList(Str) = .empty;
        if (req_val := node.get("required")) {
            const req_arr = req_val.asArray() ?? return err.InvalidSchema;
            for (r of req_arr) {
                const rname = r.asString() ?? return err.InvalidSchema;
                req_keys.append(arena_alloc, Schema.dupeStr(arena_alloc, rname)?)?;
            }
        }
        const req_slice = req_keys.toOwnedSlice(arena_alloc)?;

        if (props_val := node.get("properties")) {
            const props_fields = props_val.asObject() ?? return err.InvalidSchema;
            let prop_list: std.ArrayList(PropertySchema) = .empty;
            for (f of props_fields) {
                const pname = Schema.dupeStr(arena_alloc, f.key)?;
                const pschema = Schema.parseNode(arena_alloc, f.value, depth + 1)?;
                const is_req = Schema.containsString(req_slice, pname);
                prop_list.append(arena_alloc, PropertySchema{
                    name: pname,
                    schema: pschema,
                    required: is_req,
                })?;
            }
            s.properties = prop_list.toOwnedSlice(arena_alloc)?;
            if (!has_explicit_type) {
                s.schema_type = .Object;
            }
        }

        if (req_slice.len > 0) {
            let missing_req: std.ArrayList(Str) = .empty;
            for (rk of req_slice) {
                let in_props = false;
                for (p of s.properties) {
                    if (std.mem.eql(u8, p.name, rk)) {
                        in_props = true;
                        break;
                    }
                }
                if (!in_props) {
                    missing_req.append(arena_alloc, rk)?;
                }
            }
            s.extra_required = missing_req.toOwnedSlice(arena_alloc)?;
            if (!has_explicit_type) {
                s.schema_type = .Object;
            }
        }

        if (add_val := node.get("additionalProperties")) {
            if (ab := add_val.asBool()) {
                s.additional_properties = ab;
            } else if (add_val.isObject()) {
                // Treat object-valued additionalProperties as allowed
                s.additional_properties = true;
            } else {
                return err.InvalidSchema;
            }
        }

        return s;
    }

    /// Parses a `Schema` directly from an already-parsed `JsonValue`.
    /// Call `schema.deinit()` when finished to free internal schema allocations.
    pub static fn fromJsonValue(alloc: std.mem.Allocator, root: JsonValue): Schema! {
        let schema_arena = alloc.create(std.heap.ArenaAllocator)?;
        unsafe {
            schema_arena.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            schema_arena.deinit();
            alloc.destroy(schema_arena);
        }

        let s = Schema.parseNode(schema_arena.allocator(), root, 0)?;
        s.owned_arena = schema_arena;
        s.parent_alloc = alloc;
        return s;
    }

    /// Parses a `Schema` from standard JSON Schema Draft 2020-12 / OpenAI / MCP JSON text.
    /// Call `schema.deinit()` when finished to free internal schema allocations.
    pub static fn fromJson(alloc: std.mem.Allocator, json_str: []const u8): Schema! {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();

        const root = JsonParser.parse(tmp_arena.allocator(), json_str)?;
        return Schema.fromJsonValue(alloc, root);
    }
}
