// Source: Custom implementation by Antigravity AI
// Zero-copy JSON value representation in CP
// String keys and string values borrow directly from source slice without heap duplication

pub enum JsonTag {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object,
}

pub struct JsonField {
    key: []const u8 = "",
    value: JsonValue = .{},
}

pub struct JsonValue {
    tag: JsonTag = .Null,
    bool_val: bool = false,
    num_val: f64 = 0,
    str_val: []const u8 = "",
    arr_val: []JsonValue = [],
    obj_val: []JsonField = [],

    pub fn isNull(*_): bool {
        return _.tag == .Null;
    }

    pub fn asBool(*_): bool? {
        if (_.tag == .Bool) return _.bool_val;
        return null;
    }

    pub fn asNumber(*_): f64? {
        if (_.tag == .Number) return _.num_val;
        return null;
    }

    pub fn asString(*_): ([]const u8)? {
        if (_.tag == .String) return _.str_val;
        return null;
    }

    pub fn asArray(*_): ([]JsonValue)? {
        if (_.tag == .Array) return _.arr_val;
        return null;
    }

    pub fn asObject(*_): ([]JsonField)? {
        if (_.tag == .Object) return _.obj_val;
        return null;
    }

    pub fn len(*_): usize {
        return switch (_.tag) {
            .Array => _.arr_val.len,
            .Object => _.obj_val.len,
            .String => _.str_val.len,
            else => 0,
        };
    }

    pub fn get(*_, key: []const u8): JsonValue? {
        if (_.tag != .Object) return null;
        for (field of _.obj_val) {
            if (field.key.len == key.len) {
                let match_key = true;
                let i: usize = 0;
                while (i < key.len) {
                    if (field.key[i] != key[i]) {
                        match_key = false;
                        break;
                    }
                    i += 1;
                }
                if (match_key) return field.value;
            }
        }
        return null;
    }

    pub fn getIndex(*_, idx: usize): JsonValue? {
        if (_.tag != .Array or idx >= _.arr_val.len) return null;
        return _.arr_val[idx];
    }
}
