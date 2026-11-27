// @cap/zod-to-json-schema — Zero-heap JSON Schema builder & serializer in pure CP
// Clean-room MIT implementation of npm `zod-to-json-schema` (#580, 98.7M/wk).

import { BufWriter } from "cap:fmt";

pub enum SchemaType {
    String,
    Number,
    Integer,
    Boolean,
    Array,
    Object,
    Null,
}

pub struct SchemaProp {
    pub name: string,
    pub kind: SchemaType,
    pub description: string,
    pub required: bool,
    pub format: string,       // optional e.g. "email", "uri", "date-time"
    pub item_kind: SchemaType, // used when kind == .Array
    pub has_min: bool,
    pub min_val: i64,
    pub has_max: bool,
    pub max_val: i64,
}

pub fn schemaTypeName(t: SchemaType): string {
    if (t == .String) { return "string"; }
    if (t == .Number) { return "number"; }
    if (t == .Integer) { return "integer"; }
    if (t == .Boolean) { return "boolean"; }
    if (t == .Array) { return "array"; }
    if (t == .Object) { return "object"; }
    return "null";
}

/// Convenience constructor for a simple typed property.
pub fn prop(name: string, kind: SchemaType, description: string, required: bool): SchemaProp {
    return SchemaProp {
        name: name,
        kind: kind,
        description: description,
        required: required,
        format: "",
        item_kind: .String,
        has_min: false,
        min_val: 0,
        has_max: false,
        max_val: 0,
    };
}

/// Convenience constructor for a string property with format (e.g. "email", "uuid", "uri").
pub fn stringProp(name: string, description: string, format: string, required: bool): SchemaProp {
    return SchemaProp {
        name: name,
        kind: .String,
        description: description,
        required: required,
        format: format,
        item_kind: .String,
        has_min: false,
        min_val: 0,
        has_max: false,
        max_val: 0,
    };
}

/// Convenience constructor for an integer property with optional min/max bounds.
pub fn intProp(name: string, description: string, required: bool, min_val: i64, max_val: i64): SchemaProp {
    return SchemaProp {
        name: name,
        kind: .Integer,
        description: description,
        required: required,
        format: "",
        item_kind: .String,
        has_min: true,
        min_val: min_val,
        has_max: true,
        max_val: max_val,
    };
}

/// Convenience constructor for a typed array property.
pub fn arrayProp(name: string, item_kind: SchemaType, description: string, required: bool): SchemaProp {
    return SchemaProp {
        name: name,
        kind: .Array,
        description: description,
        required: required,
        format: "",
        item_kind: item_kind,
        has_min: false,
        min_val: 0,
        has_max: false,
        max_val: 0,
    };
}

/// Emits a Draft-07 JSON Schema for an object with `props` into `out`; returns bytes written.
pub fn toJsonSchema(
    title: string,
    props: []const SchemaProp,
    additional_properties: bool,
    out: []u8,
): usize {
    let w = BufWriter.new(out);
    w.writeStr("{\"$schema\":\"http://json-schema.org/draft-07/schema#\",");
    if (title.len > 0) {
        w.writeStr("\"title\":");
        w.writeJsonStr(title);
        w.writeStr(",");
    }
    w.writeStr("\"type\":\"object\",\"properties\":{");

    let i: usize = 0;
    while (i < props.len) {
        if (i > 0) {
            w.writeStr(",");
        }
        const pr = props[i];
        w.writeJsonStr(pr.name);
        w.writeStr(":{\"type\":");
        w.writeJsonStr(schemaTypeName(pr.kind));
        if (pr.description.len > 0) {
            w.writeStr(",\"description\":");
            w.writeJsonStr(pr.description);
        }
        if (pr.format.len > 0) {
            w.writeStr(",\"format\":");
            w.writeJsonStr(pr.format);
        }
        if (pr.kind == .Array) {
            w.writeStr(",\"items\":{\"type\":");
            w.writeJsonStr(schemaTypeName(pr.item_kind));
            w.writeStr("}");
        }
        if (pr.has_min) {
            if (pr.kind == .String) {
                w.writeStr(",\"minLength\":");
            } else {
                w.writeStr(",\"minimum\":");
            }
            w.writeI64(pr.min_val);
        }
        if (pr.has_max) {
            if (pr.kind == .String) {
                w.writeStr(",\"maxLength\":");
            } else {
                w.writeStr(",\"maximum\":");
            }
            w.writeI64(pr.max_val);
        }
        w.writeStr("}");
        i += 1;
    }

    w.writeStr("}");

    // Emit "required" array if any property is required
    let req_count: usize = 0;
    let r: usize = 0;
    while (r < props.len) {
        if (props[r].required) {
            req_count += 1;
        }
        r += 1;
    }
    if (req_count > 0) {
        w.writeStr(",\"required\":[");
        let written: usize = 0;
        let k: usize = 0;
        while (k < props.len) {
            if (props[k].required) {
                if (written > 0) {
                    w.writeStr(",");
                }
                w.writeJsonStr(props[k].name);
                written += 1;
            }
            k += 1;
        }
        w.writeStr("]");
    }

    if (!additional_properties) {
        w.writeStr(",\"additionalProperties\":false");
    }
    w.writeStr("}");
    return w.written();
}

export {
    SchemaType,
    SchemaProp,
    schemaTypeName,
    prop,
    stringProp,
    intProp,
    arrayProp,
    toJsonSchema,
};
