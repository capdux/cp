// @cap/zod-to-json-schema — Zero-heap JSON Schema builder & serializer in pure CP
// Clean-room MIT implementation of npm `zod-to-json-schema` (#580, 98.7M/wk).

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

fn zjsWriteStr(s: string, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn zjsWriteJsonStr(s: string, out: []u8, pos: usize): usize {
    let p = zjsWriteStr("\"", out, pos);
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == 34) {
            p = zjsWriteStr("\\\"", out, p);
        } else if (b == 92) {
            p = zjsWriteStr("\\\\", out, p);
        } else if (b == 10) {
            p = zjsWriteStr("\\n", out, p);
        } else if (b == 13) {
            p = zjsWriteStr("\\r", out, p);
        } else if (b == 9) {
            p = zjsWriteStr("\\t", out, p);
        } else {
            if (p < out.len) {
                out[p] = b;
                p += 1;
            }
        }
        i += 1;
    }
    return zjsWriteStr("\"", out, p);
}

fn zjsWriteI64(val: i64, out: []u8, pos: usize): usize {
    let p = pos;
    let mag: u64 = 0;
    if (val < 0) {
        p = zjsWriteStr("-", out, p);
        mag = (0 - val) as u64;
    } else {
        mag = val as u64;
    }
    let digits: [24]u8 = [0; 24];
    let n: usize = 0;
    if (mag == 0) {
        digits[0] = 48;
        n = 1;
    } else {
        while (mag > 0 && n < 24) {
            digits[n] = ((mag % 10) as u8) + 48;
            mag = mag / 10;
            n += 1;
        }
    }
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p;
}

/// Emits a Draft-07 JSON Schema for an object with `props` into `out`; returns bytes written.
pub fn toJsonSchema(
    title: string,
    props: []const SchemaProp,
    additional_properties: bool,
    out: []u8,
): usize {
    let p: usize = 0;
    p = zjsWriteStr("{\"$schema\":\"http://json-schema.org/draft-07/schema#\",", out, p);
    if (title.len > 0) {
        p = zjsWriteStr("\"title\":", out, p);
        p = zjsWriteJsonStr(title, out, p);
        p = zjsWriteStr(",", out, p);
    }
    p = zjsWriteStr("\"type\":\"object\",\"properties\":{", out, p);

    let i: usize = 0;
    while (i < props.len) {
        if (i > 0) {
            p = zjsWriteStr(",", out, p);
        }
        const pr = props[i];
        p = zjsWriteJsonStr(pr.name, out, p);
        p = zjsWriteStr(":{\"type\":", out, p);
        p = zjsWriteJsonStr(schemaTypeName(pr.kind), out, p);
        if (pr.description.len > 0) {
            p = zjsWriteStr(",\"description\":", out, p);
            p = zjsWriteJsonStr(pr.description, out, p);
        }
        if (pr.format.len > 0) {
            p = zjsWriteStr(",\"format\":", out, p);
            p = zjsWriteJsonStr(pr.format, out, p);
        }
        if (pr.kind == .Array) {
            p = zjsWriteStr(",\"items\":{\"type\":", out, p);
            p = zjsWriteJsonStr(schemaTypeName(pr.item_kind), out, p);
            p = zjsWriteStr("}", out, p);
        }
        if (pr.has_min) {
            if (pr.kind == .String) {
                p = zjsWriteStr(",\"minLength\":", out, p);
            } else {
                p = zjsWriteStr(",\"minimum\":", out, p);
            }
            p = zjsWriteI64(pr.min_val, out, p);
        }
        if (pr.has_max) {
            if (pr.kind == .String) {
                p = zjsWriteStr(",\"maxLength\":", out, p);
            } else {
                p = zjsWriteStr(",\"maximum\":", out, p);
            }
            p = zjsWriteI64(pr.max_val, out, p);
        }
        p = zjsWriteStr("}", out, p);
        i += 1;
    }

    p = zjsWriteStr("}", out, p);

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
        p = zjsWriteStr(",\"required\":[", out, p);
        let written: usize = 0;
        let k: usize = 0;
        while (k < props.len) {
            if (props[k].required) {
                if (written > 0) {
                    p = zjsWriteStr(",", out, p);
                }
                p = zjsWriteJsonStr(props[k].name, out, p);
                written += 1;
            }
            k += 1;
        }
        p = zjsWriteStr("]", out, p);
    }

    if (!additional_properties) {
        p = zjsWriteStr(",\"additionalProperties\":false", out, p);
    }
    p = zjsWriteStr("}", out, p);
    return p;
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
