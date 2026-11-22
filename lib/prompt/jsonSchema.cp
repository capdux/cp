// Origin: Pure-CP Stdlib (AI Structured Outputs JSON Schema Builder & OpenAI/Anthropic Tool Spec Emitter) | Created: 2026-11-22
// Zero-heap-allocation JSON Schema Draft 2020-12 builder and OpenAI / Anthropic tool declaration emitter.

/// JSON Schema primitive and container types supported for LLM structured outputs and tool parameters.
pub enum JsonSchemaType {
    String,
    Integer,
    Number,
    Boolean,
    Array,
    Object,
}

/// Descriptor for a single property in a JSON Schema object.
pub struct SchemaProp {
    name: string = "";
    kind: JsonSchemaType = JsonSchemaType.String;
    description: string = "";
    required: bool = true;
    enum_csv: string = "";
    item_kind: JsonSchemaType = JsonSchemaType.String;
}

/// Tool specification combining metadata with parameter schema properties.
pub struct ToolSchemaSpec {
    tool_name: string = "";
    description: string = "";
    strict: bool = true;
    props: []const SchemaProp;
}

fn schemaTypeStr(kind: JsonSchemaType): string {
    return switch (kind) {
        JsonSchemaType.String => "string",
        JsonSchemaType.Integer => "integer",
        JsonSchemaType.Number => "number",
        JsonSchemaType.Boolean => "boolean",
        JsonSchemaType.Array => "array",
        JsonSchemaType.Object => "object",
    };
}

fn writeByte(out_buf: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= out_buf.len) return false;
    out_buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeRaw(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn hexDigitLower(n: u8): u8 {
    return if (n < 10) ('0' + n) else ('a' + (n - 10));
}

/// Writes an RFC 8259 escaped JSON string literal including surrounding quotes.
fn writeJsonString(out_buf: *[]u8, pos: *usize, s: string): bool {
    if (!writeByte(out_buf, pos, '"')) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            if (!writeRaw(out_buf, pos, "\\\"")) return false;
        } else if (b == '\\') {
            if (!writeRaw(out_buf, pos, "\\\\")) return false;
        } else if (b == '\n') {
            if (!writeRaw(out_buf, pos, "\\n")) return false;
        } else if (b == '\r') {
            if (!writeRaw(out_buf, pos, "\\r")) return false;
        } else if (b == '\t') {
            if (!writeRaw(out_buf, pos, "\\t")) return false;
        } else if (b == 0x08) {
            if (!writeRaw(out_buf, pos, "\\b")) return false;
        } else if (b == 0x0C) {
            if (!writeRaw(out_buf, pos, "\\f")) return false;
        } else if (b < 0x20) {
            if (!writeRaw(out_buf, pos, "\\u00")) return false;
            if (!writeByte(out_buf, pos, hexDigitLower((b >> 4) & 0x0F))) return false;
            if (!writeByte(out_buf, pos, hexDigitLower(b & 0x0F))) return false;
        } else {
            if (!writeByte(out_buf, pos, b)) return false;
        }
    }
    return writeByte(out_buf, pos, '"');
}

fn writeEnumCsv(out_buf: *[]u8, pos: *usize, enum_csv: string): bool {
    // Check if there is at least one non-empty trimmed token
    let has_any = false;
    let p: usize = 0;
    while (p < enum_csv.len) {
        let seg_end = p;
        while (seg_end < enum_csv.len && enum_csv[seg_end] != ',') : (seg_end += 1) {}
        let s = p;
        let e = seg_end;
        while (s < e && (enum_csv[s] == ' ' || enum_csv[s] == '\t')) : (s += 1) {}
        while (e > s && (enum_csv[e - 1] == ' ' || enum_csv[e - 1] == '\t')) : (e -= 1) {}
        if (e > s) {
            has_any = true;
            break;
        }
        p = if (seg_end < enum_csv.len) seg_end + 1 else seg_end;
    }
    if (!has_any) return true;

    if (!writeRaw(out_buf, pos, ",\"enum\":[")) return false;
    let first = true;
    p = 0;
    while (p < enum_csv.len) {
        let seg_end = p;
        while (seg_end < enum_csv.len && enum_csv[seg_end] != ',') : (seg_end += 1) {}
        let s = p;
        let e = seg_end;
        while (s < e && (enum_csv[s] == ' ' || enum_csv[s] == '\t')) : (s += 1) {}
        while (e > s && (enum_csv[e - 1] == ' ' || enum_csv[e - 1] == '\t')) : (e -= 1) {}
        if (e > s) {
            if (!first) {
                if (!writeByte(out_buf, pos, ',')) return false;
            }
            if (!writeJsonString(out_buf, pos, enum_csv[s..e])) return false;
            first = false;
        }
        p = if (seg_end < enum_csv.len) seg_end + 1 else seg_end;
    }
    return writeByte(out_buf, pos, ']');
}

fn emitSchemaObjectAt(props: []const SchemaProp, strict: bool, out_buf: *[]u8, pos: *usize): bool {
    if (!writeRaw(out_buf, pos, "{\"type\":\"object\",\"properties\":{")) return false;

    let i: usize = 0;
    while (i < props.len) : (i += 1) {
        const prop = props[i];
        if (prop.name.len == 0) return false;
        if (i > 0) {
            if (!writeByte(out_buf, pos, ',')) return false;
        }
        if (!writeJsonString(out_buf, pos, prop.name)) return false;
        if (!writeRaw(out_buf, pos, ":{\"type\":\"")) return false;
        if (!writeRaw(out_buf, pos, schemaTypeStr(prop.kind))) return false;
        if (!writeByte(out_buf, pos, '"')) return false;

        if (prop.description.len > 0) {
            if (!writeRaw(out_buf, pos, ",\"description\":")) return false;
            if (!writeJsonString(out_buf, pos, prop.description)) return false;
        }
        if (prop.kind == JsonSchemaType.Array) {
            if (!writeRaw(out_buf, pos, ",\"items\":{\"type\":\"")) return false;
            if (!writeRaw(out_buf, pos, schemaTypeStr(prop.item_kind))) return false;
            if (!writeRaw(out_buf, pos, "\"}")) return false;
        }
        if (prop.enum_csv.len > 0) {
            if (!writeEnumCsv(out_buf, pos, prop.enum_csv)) return false;
        }
        if (!writeByte(out_buf, pos, '}')) return false;
    }

    if (!writeRaw(out_buf, pos, "},\"required\":[")) return false;
    let first_req = true;
    let j: usize = 0;
    while (j < props.len) : (j += 1) {
        if (!props[j].required) continue;
        if (!first_req) {
            if (!writeByte(out_buf, pos, ',')) return false;
        }
        if (!writeJsonString(out_buf, pos, props[j].name)) return false;
        first_req = false;
    }
    if (!writeByte(out_buf, pos, ']')) return false;

    if (strict) {
        if (!writeRaw(out_buf, pos, ",\"additionalProperties\":false")) return false;
    }
    return writeByte(out_buf, pos, '}');
}

/// Zero-heap-allocation emitter for a JSON Schema Draft 2020-12 object definition.
/// Writes `type`, `properties`, `required`, and (when `strict == true`) `additionalProperties: false`.
/// Returns bytes written to `out_buf`, or `null` on empty property name or insufficient buffer.
pub fn emitJsonSchemaObject(
    props: []const SchemaProp,
    strict: bool,
    out_buf: *[]u8,
): usize? {
    let pos: usize = 0;
    if (!emitSchemaObjectAt(props, strict, out_buf, *pos)) {
        return null;
    }
    return pos;
}

/// Zero-heap-allocation emitter for an OpenAI Function Calling tool declaration:
/// `{"type":"function","function":{"name":...,"description":...,"strict":...,"parameters":{...}}}`.
/// Returns bytes written to `out_buf`, or `null` if `spec.tool_name` is empty or buffer is too small.
pub fn emitOpenAiToolJson(spec: ToolSchemaSpec, out_buf: *[]u8): usize? {
    if (spec.tool_name.len == 0) return null;
    let pos: usize = 0;

    if (!writeRaw(out_buf, *pos, "{\"type\":\"function\",\"function\":{\"name\":")) return null;
    if (!writeJsonString(out_buf, *pos, spec.tool_name)) return null;
    if (!writeRaw(out_buf, *pos, ",\"description\":")) return null;
    if (!writeJsonString(out_buf, *pos, spec.description)) return null;
    if (!writeRaw(out_buf, *pos, ",\"strict\":")) return null;
    if (!writeRaw(out_buf, *pos, if (spec.strict) "true" else "false")) return null;
    if (!writeRaw(out_buf, *pos, ",\"parameters\":")) return null;
    if (!emitSchemaObjectAt(spec.props, spec.strict, out_buf, *pos)) return null;
    if (!writeRaw(out_buf, *pos, "}}")) return null;

    return pos;
}

/// Zero-heap-allocation emitter for an Anthropic tool declaration:
/// `{"name":...,"description":...,"input_schema":{...}}`.
/// Returns bytes written to `out_buf`, or `null` if `spec.tool_name` is empty or buffer is too small.
pub fn emitAnthropicToolJson(spec: ToolSchemaSpec, out_buf: *[]u8): usize? {
    if (spec.tool_name.len == 0) return null;
    let pos: usize = 0;

    if (!writeRaw(out_buf, *pos, "{\"name\":")) return null;
    if (!writeJsonString(out_buf, *pos, spec.tool_name)) return null;
    if (!writeRaw(out_buf, *pos, ",\"description\":")) return null;
    if (!writeJsonString(out_buf, *pos, spec.description)) return null;
    if (!writeRaw(out_buf, *pos, ",\"input_schema\":")) return null;
    if (!emitSchemaObjectAt(spec.props, spec.strict, out_buf, *pos)) return null;
    if (!writeByte(out_buf, *pos, '}')) return null;

    return pos;
}
