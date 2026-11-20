// Source: bun/src/valkey/valkey_protocol.cp -> cp/lib/valkey/valkey.cp (Pure CP RESP2/RESP3 Valkey/Redis Protocol Codec & Incremental Stream Parser)
// Zero-C-dependency RESP2/RESP3 serializer, command encoder, inline parser, and incremental stream parser.

const std = @import("std");

/// Default maximum nesting depth for recursive RESP collections (Array, Map, Set, Push, Attribute).
pub const DEFAULT_MAX_DEPTH: usize = 32;

/// Default maximum byte length for a single BulkString, BulkError, or VerbatimString (64 MiB).
pub const DEFAULT_MAX_BULK_LEN: usize = 64 * 1024 * 1024;

/// Default maximum element count for a single RESP collection (Array, Map, Set, Push, Attribute).
pub const DEFAULT_MAX_COLLECTION_LEN: usize = 1024 * 1024;

/// Unified error set for `cap:valkey` RESP2/RESP3 parsing and encoding.
pub err ProtocolError {
    Incomplete,
    DepthLimitExceeded,
    BulkTooLarge,
    CollectionTooLarge,
    InvalidTypeByte,
    InvalidInteger,
    InvalidDouble,
    InvalidBoolean,
    InvalidBulkLength,
    InvalidCollectionLength,
    InvalidVerbatimString,
    InvalidPush,
    InvalidInlineCommand,
    MissingCrLf,
    OutOfMemory,
}

/// Resource-exhaustion protection limits for RESP parsing.
pub struct RespLimits {
    pub max_depth: usize = DEFAULT_MAX_DEPTH;
    pub max_bulk_len: usize = DEFAULT_MAX_BULK_LEN;
    pub max_collection_len: usize = DEFAULT_MAX_COLLECTION_LEN;
}

/// RESP2 and RESP3 wire type prefix bytes.
pub enum RespType: u8 {
    // RESP2 types
    SimpleString = '+',
    SimpleError = '-',
    Integer = ':',
    BulkString = '$',
    Array = '*',

    // RESP3 types
    Null = '_',
    Double = ',',
    Boolean = '#',
    BulkError = '!',
    VerbatimString = '=',
    Map = '%',
    Set = '~',
    Push = '>',
    BigNumber = '(',
    Attribute = '|',

    /// Converts a leading wire byte into a `RespType`, or `null` if unrecognized.
    pub static fn fromByte(byte: u8): RespType? {
        return switch (byte) {
            '+' => .SimpleString,
            '-' => .SimpleError,
            ':' => .Integer,
            '$' => .BulkString,
            '*' => .Array,
            '_' => .Null,
            ',' => .Double,
            '#' => .Boolean,
            '!' => .BulkError,
            '=' => .VerbatimString,
            '%' => .Map,
            '~' => .Set,
            '>' => .Push,
            '(' => .BigNumber,
            '|' => .Attribute,
            else => null,
        };
    }
}

/// RESP3 VerbatimString (`=`) payload with a 3-byte format tag (e.g. `"txt"`, `"mkd"`) and content.
pub struct VerbatimString {
    pub format: []const u8 = "";
    pub content: []const u8 = "";

    /// Frees owned `format` and `content` slices.
    pub fn deinit(alloc: std.mem.Allocator) {
        if (_.format.len > 0) {
            alloc.free(_.format);
        }
        if (_.content.len > 0) {
            alloc.free(_.content);
        }
    }

    /// Compares two `VerbatimString` values for byte-for-byte equality.
    pub fn eql(other: VerbatimString): bool {
        return std.mem.eql(u8, _.format, other.format) && std.mem.eql(u8, _.content, other.content);
    }
}

/// Key-value pair stored in a RESP3 `Map` (`%`).
pub struct MapEntry {
    pub key: RespValue = .Null;
    pub value: RespValue = .Null;

    /// Recursively frees `key` and `value`.
    pub fn deinit(alloc: std.mem.Allocator) {
        _.key.deinit(alloc);
        _.value.deinit(alloc);
    }

    /// Compares two `MapEntry` values for structural equality.
    pub fn eql(other: MapEntry): bool {
        return _.key.eql(other.key) && _.value.eql(other.value);
    }
}

/// Tagged union representing all RESP2 and RESP3 values.
pub enum RespValue {
    SimpleString: []const u8,
    SimpleError: []const u8,
    Integer: i64,
    BulkString: []const u8,
    Null,
    Array: []RespValue,
    Boolean: bool,
    Double: f64,
    BigNumber: []const u8,
    BulkError: []const u8,
    VerbatimString: VerbatimString,
    Map: []MapEntry,
    Set: []RespValue,
    Push: []RespValue,

    /// Recursively frees all heap-allocated memory owned by this `RespValue`.
    pub fn deinit(alloc: std.mem.Allocator) {
        switch (_) {
            .SimpleString s => {
                if (s.len > 0) alloc.free(s);
            },
            .SimpleError s => {
                if (s.len > 0) alloc.free(s);
            },
            .Integer _ => {},
            .BulkString s => {
                if (s.len > 0) alloc.free(s);
            },
            .Null => {},
            .Array items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                if (items.len > 0) alloc.free(items);
            },
            .Boolean _ => {},
            .Double _ => {},
            .BigNumber s => {
                if (s.len > 0) alloc.free(s);
            },
            .BulkError s => {
                if (s.len > 0) alloc.free(s);
            },
            .VerbatimString v => {
                v.deinit(alloc);
            },
            .Map entries => {
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    entries[i].deinit(alloc);
                }
                if (entries.len > 0) alloc.free(entries);
            },
            .Set items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                if (items.len > 0) alloc.free(items);
            },
            .Push items => {
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    items[i].deinit(alloc);
                }
                if (items.len > 0) alloc.free(items);
            },
        }
    }

    /// Returns `true` if this value is `.Null`.
    pub fn isNull(): bool {
        return switch (_) {
            .Null => true,
            else => false,
        };
    }

    /// Returns the underlying string slice for string-like variants.
    pub fn asStr(): []const u8? {
        return switch (_) {
            .SimpleString s => s,
            .SimpleError s => s,
            .BulkString s => s,
            .BigNumber s => s,
            .BulkError s => s,
            else => null,
        };
    }

    /// Returns the integer value if this is `.Integer`.
    pub fn asInt(): i64? {
        return switch (_) {
            .Integer n => n,
            else => null,
        };
    }

    /// Returns the boolean value if this is `.Boolean`.
    pub fn asBool(): bool? {
        return switch (_) {
            .Boolean b => b,
            else => null,
        };
    }

    /// Returns the floating-point value if this is `.Double`.
    pub fn asDouble(): f64? {
        return switch (_) {
            .Double d => d,
            else => null,
        };
    }

    /// Deep structural equality check (treats NaN == NaN for `.Double`).
    pub fn eql(other: RespValue): bool {
        return switch (_) {
            .SimpleString a => switch (other) {
                .SimpleString b => std.mem.eql(u8, a, b),
                else => false,
            },
            .SimpleError a => switch (other) {
                .SimpleError b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Integer a => switch (other) {
                .Integer b => a == b,
                else => false,
            },
            .BulkString a => switch (other) {
                .BulkString b => std.mem.eql(u8, a, b),
                else => false,
            },
            .Null => switch (other) {
                .Null => true,
                else => false,
            },
            .Array a => switch (other) {
                .Array b => eqlValueSlice(a, b),
                else => false,
            },
            .Boolean a => switch (other) {
                .Boolean b => a == b,
                else => false,
            },
            .Double a => switch (other) {
                .Double b => {
                    if (std.math.isNan(a) && std.math.isNan(b)) return true;
                    return a == b;
                },
                else => false,
            },
            .BigNumber a => switch (other) {
                .BigNumber b => std.mem.eql(u8, a, b),
                else => false,
            },
            .BulkError a => switch (other) {
                .BulkError b => std.mem.eql(u8, a, b),
                else => false,
            },
            .VerbatimString a => switch (other) {
                .VerbatimString b => a.eql(b),
                else => false,
            },
            .Map a => switch (other) {
                .Map b => eqlMapSlice(a, b),
                else => false,
            },
            .Set a => switch (other) {
                .Set b => eqlValueSlice(a, b),
                else => false,
            },
            .Push a => switch (other) {
                .Push b => eqlValueSlice(a, b),
                else => false,
            },
        };
    }
}

fn eqlValueSlice(a: []const RespValue, b: []const RespValue): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!a[i].eql(b[i])) return false;
    }
    return true;
}

fn eqlMapSlice(a: []const MapEntry, b: []const MapEntry): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!a[i].eql(b[i])) return false;
    }
    return true;
}

/// Result returned by `parseOne` / `parseOneWithLimits`.
pub struct ParseResult {
    pub value: RespValue;
    pub consumed: usize;
}

fn findCrLf(data: []const u8, start: usize): usize? {
    if (data.len < 2 || start >= data.len - 1) return null;
    let i: usize = start;
    while (i + 1 < data.len) : (i += 1) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    return null;
}

fn isValidBigNumber(s: []const u8): bool {
    if (s.len == 0) return false;
    let start: usize = 0;
    if (s[0] == '+' || s[0] == '-') {
        if (s.len == 1) return false;
        start = 1;
    }
    let i: usize = start;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

fn dupeOwned(alloc: std.mem.Allocator, s: []const u8): []const u8 ! ProtocolError {
    if (s.len == 0) return "";
    return alloc.dupe(u8, s) catch return ProtocolError.OutOfMemory;
}

struct Cursor {
    data: []const u8,
    pos: usize = 0,

    fn readByte(*_): u8 ! ProtocolError {
        if (this.pos >= this.data.len) return ProtocolError.Incomplete;
        const b = this.data[this.pos];
        this.pos += 1;
        return b;
    }

    fn readLine(*_): []const u8 ! ProtocolError {
        const idx_opt = findCrLf(this.data, this.pos);
        if (idx_opt == null) return ProtocolError.Incomplete;
        const idx = idx_opt.?;
        const line = this.data[this.pos..idx];
        this.pos = idx + 2;
        return line;
    }

    fn readInt(*_, comptime err_kind: ProtocolError): i64 ! ProtocolError {
        const line = this.readLine()?;
        if (line.len == 0) return err_kind;
        return std.fmt.parseInt(i64, line, 10) catch return err_kind;
    }
}

fn parseValueDepth(alloc: std.mem.Allocator, cur: *Cursor, limits: RespLimits, depth: usize): RespValue ! ProtocolError {
    const type_byte = cur.readByte()?;
    const resp_type_opt = RespType.fromByte(type_byte);
    if (resp_type_opt == null) return ProtocolError.InvalidTypeByte;
    const resp_type = resp_type_opt.?;

    return switch (resp_type) {
        .SimpleString => {
            const line = cur.readLine()?;
            const owned = dupeOwned(alloc, line)?;
            return RespValue{ SimpleString: owned };
        },
        .SimpleError => {
            const line = cur.readLine()?;
            const owned = dupeOwned(alloc, line)?;
            return RespValue{ SimpleError: owned };
        },
        .Integer => {
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidInteger;
            const n = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidInteger;
            return RespValue{ Integer: n };
        },
        .BigNumber => {
            const line = cur.readLine()?;
            if (!isValidBigNumber(line)) return ProtocolError.InvalidInteger;
            const owned = dupeOwned(alloc, line)?;
            return RespValue{ BigNumber: owned };
        },
        .Boolean => {
            const line = cur.readLine()?;
            if (line.len != 1) return ProtocolError.InvalidBoolean;
            if (line[0] == 't') return RespValue{ Boolean: true };
            if (line[0] == 'f') return RespValue{ Boolean: false };
            return ProtocolError.InvalidBoolean;
        },
        .Double => {
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidDouble;
            if (std.mem.eql(u8, line, "inf") || std.mem.eql(u8, line, "+inf")) {
                return RespValue{ Double: std.math.inf(f64) };
            }
            if (std.mem.eql(u8, line, "-inf")) {
                return RespValue{ Double: -std.math.inf(f64) };
            }
            if (std.mem.eql(u8, line, "nan")) {
                return RespValue{ Double: std.math.nan(f64) };
            }
            const d = std.fmt.parseFloat(f64, line) catch return ProtocolError.InvalidDouble;
            return RespValue{ Double: d };
        },
        .Null => {
            const line = cur.readLine()?;
            if (line.len != 0) return ProtocolError.MissingCrLf;
            return RespValue.Null;
        },
        .BulkString => {
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidBulkLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidBulkLength;
            if (len == -1) return RespValue.Null;
            if (len < -1) return ProtocolError.InvalidBulkLength;
            const ulen = len as usize;
            if (ulen > limits.max_bulk_len) return ProtocolError.BulkTooLarge;
            if (cur.pos + ulen + 2 > cur.data.len) return ProtocolError.Incomplete;
            if (cur.data[cur.pos + ulen] != '\r' || cur.data[cur.pos + ulen + 1] != '\n') {
                return ProtocolError.MissingCrLf;
            }
            const payload = cur.data[cur.pos .. cur.pos + ulen];
            cur.pos += ulen + 2;
            const owned = dupeOwned(alloc, payload)?;
            return RespValue{ BulkString: owned };
        },
        .BulkError => {
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidBulkLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidBulkLength;
            if (len < 0) return ProtocolError.InvalidBulkLength;
            const ulen = len as usize;
            if (ulen > limits.max_bulk_len) return ProtocolError.BulkTooLarge;
            if (cur.pos + ulen + 2 > cur.data.len) return ProtocolError.Incomplete;
            if (cur.data[cur.pos + ulen] != '\r' || cur.data[cur.pos + ulen + 1] != '\n') {
                return ProtocolError.MissingCrLf;
            }
            const payload = cur.data[cur.pos .. cur.pos + ulen];
            cur.pos += ulen + 2;
            const owned = dupeOwned(alloc, payload)?;
            return RespValue{ BulkError: owned };
        },
        .VerbatimString => {
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidVerbatimString;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidVerbatimString;
            if (len < 4) return ProtocolError.InvalidVerbatimString;
            const ulen = len as usize;
            if (ulen > limits.max_bulk_len) return ProtocolError.BulkTooLarge;
            if (cur.pos + ulen + 2 > cur.data.len) return ProtocolError.Incomplete;
            if (cur.data[cur.pos + ulen] != '\r' || cur.data[cur.pos + ulen + 1] != '\n') {
                return ProtocolError.MissingCrLf;
            }
            const payload = cur.data[cur.pos .. cur.pos + ulen];
            cur.pos += ulen + 2;
            if (payload[3] != ':') return ProtocolError.InvalidVerbatimString;
            const fmt = dupeOwned(alloc, payload[0..3])?;
            errdefer {
                if (fmt.len > 0) alloc.free(fmt);
            }
            const content = dupeOwned(alloc, payload[4..])?;
            return RespValue{ VerbatimString: VerbatimString{
                format: fmt,
                content: content,
            } };
        },
        .Array => {
            if (depth >= limits.max_depth) return ProtocolError.DepthLimitExceeded;
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidCollectionLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidCollectionLength;
            if (len == -1) return RespValue.Null;
            if (len < -1) return ProtocolError.InvalidCollectionLength;
            const ulen = len as usize;
            if (ulen > limits.max_collection_len) return ProtocolError.CollectionTooLarge;
            const items = parseValueArray(alloc, cur, limits, depth + 1, ulen)?;
            return RespValue{ Array: items };
        },
        .Set => {
            if (depth >= limits.max_depth) return ProtocolError.DepthLimitExceeded;
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidCollectionLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidCollectionLength;
            if (len < 0) return ProtocolError.InvalidCollectionLength;
            const ulen = len as usize;
            if (ulen > limits.max_collection_len) return ProtocolError.CollectionTooLarge;
            const items = parseValueArray(alloc, cur, limits, depth + 1, ulen)?;
            return RespValue{ Set: items };
        },
        .Push => {
            if (depth >= limits.max_depth) return ProtocolError.DepthLimitExceeded;
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidCollectionLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidCollectionLength;
            if (len < 0) return ProtocolError.InvalidCollectionLength;
            const ulen = len as usize;
            if (ulen > limits.max_collection_len) return ProtocolError.CollectionTooLarge;
            const items = parseValueArray(alloc, cur, limits, depth + 1, ulen)?;
            return RespValue{ Push: items };
        },
        .Map => {
            if (depth >= limits.max_depth) return ProtocolError.DepthLimitExceeded;
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidCollectionLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidCollectionLength;
            if (len < 0) return ProtocolError.InvalidCollectionLength;
            const ulen = len as usize;
            if (ulen > limits.max_collection_len) return ProtocolError.CollectionTooLarge;
            const entries = parseMapEntries(alloc, cur, limits, depth + 1, ulen)?;
            return RespValue{ Map: entries };
        },
        .Attribute => {
            if (depth >= limits.max_depth) return ProtocolError.DepthLimitExceeded;
            const line = cur.readLine()?;
            if (line.len == 0) return ProtocolError.InvalidCollectionLength;
            const len = std.fmt.parseInt(i64, line, 10) catch return ProtocolError.InvalidCollectionLength;
            if (len < 0) return ProtocolError.InvalidCollectionLength;
            const ulen = len as usize;
            if (ulen > limits.max_collection_len) return ProtocolError.CollectionTooLarge;
            const attrs = parseMapEntries(alloc, cur, limits, depth + 1, ulen)?;
            let k: usize = 0;
            while (k < attrs.len) : (k += 1) {
                attrs[k].deinit(alloc);
            }
            if (attrs.len > 0) alloc.free(attrs);
            return parseValueDepth(alloc, cur, limits, depth + 1);
        },
    };
}

fn parseValueArray(alloc: std.mem.Allocator, cur: *Cursor, limits: RespLimits, next_depth: usize, ulen: usize): []RespValue ! ProtocolError {
    if (ulen == 0) {
        return alloc.alloc(RespValue, 0) catch return ProtocolError.OutOfMemory;
    }
    let items = alloc.alloc(RespValue, ulen) catch return ProtocolError.OutOfMemory;
    let i: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < i) : (j += 1) {
            items[j].deinit(alloc);
        }
        alloc.free(items);
    }
    while (i < ulen) : (i += 1) {
        items[i] = parseValueDepth(alloc, cur, limits, next_depth)?;
    }
    return items;
}

fn parseMapEntries(alloc: std.mem.Allocator, cur: *Cursor, limits: RespLimits, next_depth: usize, ulen: usize): []MapEntry ! ProtocolError {
    if (ulen == 0) {
        return alloc.alloc(MapEntry, 0) catch return ProtocolError.OutOfMemory;
    }
    let entries = alloc.alloc(MapEntry, ulen) catch return ProtocolError.OutOfMemory;
    let i: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < i) : (j += 1) {
            entries[j].deinit(alloc);
        }
        alloc.free(entries);
    }
    while (i < ulen) : (i += 1) {
        const key = parseValueDepth(alloc, cur, limits, next_depth)?;
        errdefer key.deinit(alloc);
        const val = parseValueDepth(alloc, cur, limits, next_depth)?;
        entries[i] = MapEntry{ key: key, value: val };
    }
    return entries;
}

/// Parses a single RESP2/RESP3 value from `data` using default limits.
/// Returns `ProtocolError.Incomplete` if more bytes are required.
pub fn parseOne(alloc: std.mem.Allocator, data: []const u8): ParseResult ! ProtocolError {
    return parseOneWithLimits(alloc, data, RespLimits{});
}

/// Parses a single RESP2/RESP3 value from `data` with explicit resource-exhaustion limits.
pub fn parseOneWithLimits(alloc: std.mem.Allocator, data: []const u8, limits: RespLimits): ParseResult ! ProtocolError {
    let cur = Cursor{ data: data, pos: 0 };
    const val = parseValueDepth(alloc, &cur, limits, 0)?;
    return ParseResult{ value: val, consumed: cur.pos };
}

/// Parses a single Redis/Valkey inline command line (e.g. `"PING\r\n"` or `"SET k v\r\n"`).
/// Returns a `RespValue.Array` of `.BulkString` arguments.
pub fn parseInlineCommand(alloc: std.mem.Allocator, data: []const u8): ParseResult ! ProtocolError {
    if (data.len == 0) return ProtocolError.Incomplete;
    let end_idx: usize = 0;
    let consumed: usize = 0;
    let found = false;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (data[i] == '\n') {
            end_idx = i;
            if (end_idx > 0 && data[end_idx - 1] == '\r') {
                end_idx -= 1;
            }
            consumed = i + 1;
            found = true;
            break;
        }
    }
    if (!found) return ProtocolError.Incomplete;

    const line = data[0..end_idx];
    let args: std.ArrayListUnmanaged(RespValue) = .empty;
    errdefer {
        let j: usize = 0;
        while (j < args.items.len) : (j += 1) {
            args.items[j].deinit(alloc);
        }
        args.deinit(alloc);
    }

    let pos: usize = 0;
    while (pos < line.len) {
        while (pos < line.len && (line[pos] == ' ' || line[pos] == '\t')) : (pos += 1) {}
        if (pos >= line.len) break;
        const start = pos;
        while (pos < line.len && line[pos] != ' ' && line[pos] != '\t') : (pos += 1) {}
        const owned = dupeOwned(alloc, line[start..pos])?;
        args.append(alloc, RespValue{ BulkString: owned }) catch return ProtocolError.OutOfMemory;
    }

    if (args.items.len == 0) return ProtocolError.InvalidInlineCommand;

    const slice = args.toOwnedSlice(alloc) catch return ProtocolError.OutOfMemory;
    return ParseResult{
        value: RespValue{ Array: slice },
        consumed: consumed,
    };
}

/// Incremental streaming parser for RESP2/RESP3 frames.
/// Handles arbitrary chunk boundaries (1..N byte half-packets and sticky multi-frame packets).
pub struct RespStreamParser {
    alloc: std.mem.Allocator,
    buf: std.ArrayListUnmanaged(u8),
    pos: usize = 0,
    limits: RespLimits = RespLimits{},
    allow_inline: bool = false,

    /// Initializes a stream parser with default resource limits.
    pub static fn init(alloc: std.mem.Allocator): RespStreamParser {
        return RespStreamParser{
            alloc: alloc,
            buf: .empty,
        };
    }

    /// Initializes a stream parser with custom `RespLimits`.
    pub static fn initWithLimits(alloc: std.mem.Allocator, limits: RespLimits): RespStreamParser {
        return RespStreamParser{
            alloc: alloc,
            buf: .empty,
            limits: limits,
        };
    }

    /// Releases all internal buffer memory.
    pub fn deinit(*_) {
        this.buf.deinit(this.alloc);
    }

    /// Enables or disables inline command fallback when a frame does not start with a RESP type byte.
    pub fn setAllowInline(*_, allow: bool) {
        this.allow_inline = allow;
    }

    /// Returns the number of unconsumed bytes currently buffered.
    pub fn bufferedLen(): usize {
        return _.buf.items.len - _.pos;
    }

    /// Clears all buffered bytes and resets the read cursor.
    pub fn reset(*_) {
        this.buf.clearRetainingCapacity();
        this.pos = 0;
    }

    /// Appends a chunk of incoming bytes to the internal buffer.
    pub fn feed(*_, data: []const u8): void ! ProtocolError {
        if (data.len == 0) return;
        this.compactIfNeeded();
        this.buf.appendSlice(this.alloc, data) catch return ProtocolError.OutOfMemory;
    }

    fn compactIfNeeded(*_) {
        if (this.pos == 0) return;
        if (this.pos >= this.buf.items.len) {
            this.buf.clearRetainingCapacity();
            this.pos = 0;
            return;
        }
        if (this.pos >= 4096) {
            const rem = this.buf.items.len - this.pos;
            std.mem.copyForwards(u8, this.buf.items[0..rem], this.buf.items[this.pos..]);
            this.buf.items.len = rem;
            this.pos = 0;
        }
    }

    /// Attempts to decode the next complete `RespValue` from the buffer.
    /// Returns `null` when more bytes are needed (`Incomplete`), or a `ProtocolError` on malformed input.
    pub fn next(*_): RespValue? ! ProtocolError {
        if (this.pos >= this.buf.items.len) return null;
        const view = this.buf.items[this.pos..];

        const use_inline = this.allow_inline && RespType.fromByte(view[0]) == null;
        if (use_inline) {
            const res = parseInlineCommand(this.alloc, view) catch |err_val| {
                if (err_val == ProtocolError.Incomplete) return null;
                return err_val;
            };
            this.pos += res.consumed;
            this.compactIfNeeded();
            return res.value;
        }

        const res = parseOneWithLimits(this.alloc, view, this.limits) catch |err_val| {
            if (err_val == ProtocolError.Incomplete) return null;
            return err_val;
        };
        this.pos += res.consumed;
        this.compactIfNeeded();
        return res.value;
    }
}

fn appendInt(out: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: i64): void ! ProtocolError {
    let tmp: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&tmp, "{d}", [val]) catch return ProtocolError.OutOfMemory;
    out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
}

fn appendUsize(out: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: usize): void ! ProtocolError {
    let tmp: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&tmp, "{d}", [val]) catch return ProtocolError.OutOfMemory;
    out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
}

fn appendDouble(out: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: f64): void ! ProtocolError {
    if (std.math.isNan(val)) {
        out.appendSlice(alloc, "nan") catch return ProtocolError.OutOfMemory;
        return;
    }
    if (std.math.isInf(val)) {
        if (val < 0) {
            out.appendSlice(alloc, "-inf") catch return ProtocolError.OutOfMemory;
        } else {
            out.appendSlice(alloc, "inf") catch return ProtocolError.OutOfMemory;
        }
        return;
    }
    let tmp: [64]u8 = undefined;
    const s = std.fmt.bufPrint(&tmp, "{d}", [val]) catch return ProtocolError.OutOfMemory;
    out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
}

fn appendBulk(out: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, prefix: u8, data: []const u8): void ! ProtocolError {
    out.append(alloc, prefix) catch return ProtocolError.OutOfMemory;
    appendUsize(out, alloc, data.len)?;
    out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
    out.appendSlice(alloc, data) catch return ProtocolError.OutOfMemory;
    out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
}

fn encodeValueInto(out: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: RespValue, resp3: bool): void ! ProtocolError {
    switch (val) {
        .SimpleString s => {
            out.append(alloc, '+') catch return ProtocolError.OutOfMemory;
            out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
            out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
        },
        .SimpleError s => {
            out.append(alloc, '-') catch return ProtocolError.OutOfMemory;
            out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
            out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
        },
        .Integer n => {
            out.append(alloc, ':') catch return ProtocolError.OutOfMemory;
            appendInt(out, alloc, n)?;
            out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
        },
        .BulkString s => {
            appendBulk(out, alloc, '$', s)?;
        },
        .Null => {
            if (resp3) {
                out.appendSlice(alloc, "_\r\n") catch return ProtocolError.OutOfMemory;
            } else {
                out.appendSlice(alloc, "$-1\r\n") catch return ProtocolError.OutOfMemory;
            }
        },
        .Array items => {
            out.append(alloc, '*') catch return ProtocolError.OutOfMemory;
            appendUsize(out, alloc, items.len)?;
            out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            let i: usize = 0;
            while (i < items.len) : (i += 1) {
                encodeValueInto(out, alloc, items[i], resp3)?;
            }
        },
        .Boolean b => {
            if (resp3) {
                out.appendSlice(alloc, if (b) "#t\r\n" else "#f\r\n") catch return ProtocolError.OutOfMemory;
            } else {
                out.appendSlice(alloc, if (b) ":1\r\n" else ":0\r\n") catch return ProtocolError.OutOfMemory;
            }
        },
        .Double d => {
            if (resp3) {
                out.append(alloc, ',') catch return ProtocolError.OutOfMemory;
                appendDouble(out, alloc, d)?;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            } else {
                let tmp: std.ArrayListUnmanaged(u8) = .empty;
                defer tmp.deinit(alloc);
                appendDouble(&tmp, alloc, d)?;
                appendBulk(out, alloc, '$', tmp.items)?;
            }
        },
        .BigNumber s => {
            if (resp3) {
                out.append(alloc, '(') catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            } else {
                appendBulk(out, alloc, '$', s)?;
            }
        },
        .BulkError s => {
            if (resp3) {
                appendBulk(out, alloc, '!', s)?;
            } else {
                out.append(alloc, '-') catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, s) catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            }
        },
        .VerbatimString v => {
            if (v.format.len != 3) return ProtocolError.InvalidVerbatimString;
            if (resp3) {
                out.append(alloc, '=') catch return ProtocolError.OutOfMemory;
                appendUsize(out, alloc, 4 + v.content.len)?;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, v.format) catch return ProtocolError.OutOfMemory;
                out.append(alloc, ':') catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, v.content) catch return ProtocolError.OutOfMemory;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            } else {
                appendBulk(out, alloc, '$', v.content)?;
            }
        },
        .Map entries => {
            if (resp3) {
                out.append(alloc, '%') catch return ProtocolError.OutOfMemory;
                appendUsize(out, alloc, entries.len)?;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    encodeValueInto(out, alloc, entries[i].key, resp3)?;
                    encodeValueInto(out, alloc, entries[i].value, resp3)?;
                }
            } else {
                out.append(alloc, '*') catch return ProtocolError.OutOfMemory;
                appendUsize(out, alloc, entries.len * 2)?;
                out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
                let i: usize = 0;
                while (i < entries.len) : (i += 1) {
                    encodeValueInto(out, alloc, entries[i].key, resp3)?;
                    encodeValueInto(out, alloc, entries[i].value, resp3)?;
                }
            }
        },
        .Set items => {
            out.append(alloc, if (resp3) '~' else '*') catch return ProtocolError.OutOfMemory;
            appendUsize(out, alloc, items.len)?;
            out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            let i: usize = 0;
            while (i < items.len) : (i += 1) {
                encodeValueInto(out, alloc, items[i], resp3)?;
            }
        },
        .Push items => {
            out.append(alloc, if (resp3) '>' else '*') catch return ProtocolError.OutOfMemory;
            appendUsize(out, alloc, items.len)?;
            out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
            let i: usize = 0;
            while (i < items.len) : (i += 1) {
                encodeValueInto(out, alloc, items[i], resp3)?;
            }
        },
    }
}

/// Encodes a `RespValue` into wire bytes in RESP3 (`resp3 == true`) or RESP2 (`resp3 == false`) format.
pub fn encodeValue(alloc: std.mem.Allocator, val: RespValue, resp3: bool): []u8 ! ProtocolError {
    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(alloc);
    encodeValueInto(&out, alloc, val, resp3)?;
    return out.toOwnedSlice(alloc) catch return ProtocolError.OutOfMemory;
}

/// Encodes a Redis/Valkey command argument list into a standard RESP2 Array (`*N\r\n$len\r\narg\r\n`).
pub fn encodeCommand(alloc: std.mem.Allocator, args: []const []const u8): []u8 ! ProtocolError {
    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(alloc);
    out.append(alloc, '*') catch return ProtocolError.OutOfMemory;
    appendUsize(&out, alloc, args.len)?;
    out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
    let i: usize = 0;
    while (i < args.len) : (i += 1) {
        appendBulk(&out, alloc, '$', args[i])?;
    }
    return out.toOwnedSlice(alloc) catch return ProtocolError.OutOfMemory;
}

/// Encodes a Redis/Valkey command argument list into an inline command line (`arg0 arg1 ...\r\n`).
pub fn encodeInlineCommand(alloc: std.mem.Allocator, args: []const []const u8): []u8 ! ProtocolError {
    if (args.len == 0) return ProtocolError.InvalidInlineCommand;
    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(alloc);
    let i: usize = 0;
    while (i < args.len) : (i += 1) {
        if (i > 0) {
            out.append(alloc, ' ') catch return ProtocolError.OutOfMemory;
        }
        out.appendSlice(alloc, args[i]) catch return ProtocolError.OutOfMemory;
    }
    out.appendSlice(alloc, "\r\n") catch return ProtocolError.OutOfMemory;
    return out.toOwnedSlice(alloc) catch return ProtocolError.OutOfMemory;
}
