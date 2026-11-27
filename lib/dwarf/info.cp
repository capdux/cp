// Source: cp/lib/dwarf/info.cp (DWARF v4/v5 .debug_abbrev and .debug_info DIE parser)

const std = @import("std");
const types = @import("./types.cp");
const DwarfError = types.DwarfError;
const DwForm = types.DwForm;
const Cursor = types.Cursor;

pub const MAX_DIE_NODES: u32 = 500000;

pub struct AttrSpec {
    name: u32,
    form: u32,
}

pub struct AbbrevEntry {
    code: u64,
    tag: u32,
    has_children: bool,
    attr_specs: []AttrSpec,
}

pub struct AbbrevTable {
    entries: []AbbrevEntry,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (e of _.entries) {
            if (e.attr_specs.len > 0) {
                alloc.free(e.attr_specs);
            }
        }
        if (_.entries.len > 0) {
            alloc.free(_.entries);
        }
    }

    pub fn find(code: u64): (*const AbbrevEntry)? {
        const entries = _.entries;
        let i: usize = 0;
        while (i < entries.len) {
            if (entries[i].code == code) {
                return &entries[i];
            }
            i += 1;
        }
        return null;
    }
}

pub enum AttrValueKind : u8 {
    addr = 0,
    uint = 1,
    sint = 2,
    str = 3,
    flag = 4,
    sec_offset = 5,
    ref = 6,
    bytes = 7,
}

pub struct AttrValue {
    kind: AttrValueKind,
    uint_val: u64,
    sint_val: i64,
    str_val: []const u8,
    flag_val: bool,
    bytes_val: []const u8,
}

pub struct DieAttr {
    name: u32,
    form: u32,
    value: AttrValue,
}

pub struct DieNode {
    offset: u64,
    abbrev_code: u64,
    tag: u32,
    has_children: bool,
    depth: u32,
    attrs: []DieAttr,

    pub fn getAttr(name: u32): (*const AttrValue)? {
        const attrs = _.attrs;
        let i: usize = 0;
        while (i < attrs.len) {
            if (attrs[i].name == name) {
                return &attrs[i].value;
            }
            i += 1;
        }
        return null;
    }
}

pub struct CompileUnitHeader {
    unit_length: u64,
    is_64bit: bool,
    version: u16,
    unit_type: u8,
    debug_abbrev_offset: u64,
    address_size: u8,
}

pub struct CompileUnit {
    header: CompileUnitHeader,
    nodes: []DieNode,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        for (n of _.nodes) {
            if (n.attrs.len > 0) {
                alloc.free(n.attrs);
            }
        }
        if (_.nodes.len > 0) {
            alloc.free(_.nodes);
        }
    }
}

pub fn parseAbbrevTable(alloc: std.mem.Allocator, debug_abbrev: []const u8, offset: u64): !AbbrevTable {
    if (offset >= debug_abbrev.len) {
        return DwarfError.TruncatedData;
    }
    let cur = Cursor.init(debug_abbrev[@as(usize, @intCast(offset))..]);
    let entries = std.ArrayList(AbbrevEntry).empty;
    errdefer {
        for (e of entries.items) {
            if (e.attr_specs.len > 0) {
                alloc.free(e.attr_specs);
            }
        }
        entries.deinit(alloc);
    }

    while (cur.remaining() > 0) {
        const code = try cur.readUleb128();
        if (code == 0) {
            break;
        }
        const tag: u32 = @intCast(try cur.readUleb128());
        const has_children = (try cur.readU8()) != 0;

        let specs = std.ArrayList(AttrSpec).empty;
        errdefer specs.deinit(alloc);

        while (true) {
            const attr_name: u32 = @intCast(try cur.readUleb128());
            const attr_form: u32 = @intCast(try cur.readUleb128());
            if (attr_name == 0 && attr_form == 0) {
                break;
            }
            try specs.append(alloc, AttrSpec{ .name = attr_name, .form = attr_form });
        }

        const specs_slice = try specs.toOwnedSlice(alloc);
        try entries.append(alloc, AbbrevEntry{
            .code = code,
            .tag = tag,
            .has_children = has_children,
            .attr_specs = specs_slice,
        });
    }

    return AbbrevTable{ .entries = try entries.toOwnedSlice(alloc) };
}

fn readCStringAt(buf: []const u8, offset: usize): ![]const u8 {
    if (offset >= buf.len) {
        return DwarfError.TruncatedData;
    }
    let i = offset;
    while (i < buf.len) {
        if (buf[i] == 0) {
            return buf[offset..i];
        }
        i += 1;
    }
    return DwarfError.TruncatedData;
}

fn decodeFormValue(
    cur: *Cursor,
    form: u32,
    addr_size: u8,
    is_64bit: bool,
    debug_str: []const u8,
): !AttrValue {
    let actual_form = form;
    if (actual_form == DwForm.indirect) {
        actual_form = @intCast(try cur.readUleb128());
    }

    if (actual_form == DwForm.addr) {
        const v = try cur.readAddr(addr_size);
        return AttrValue{
            .kind = AttrValueKind.addr,
            .uint_val = v,
            .sint_val = 0,
            .str_val = "",
            .flag_val = false,
            .bytes_val = "",
        };
    }
    if (actual_form == DwForm.data1) {
        const v: u64 = @intCast(try cur.readU8());
        return AttrValue{ .kind = AttrValueKind.uint, .uint_val = v, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.data2) {
        const v: u64 = @intCast(try cur.readU16());
        return AttrValue{ .kind = AttrValueKind.uint, .uint_val = v, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.data4) {
        const v: u64 = @intCast(try cur.readU32());
        return AttrValue{ .kind = AttrValueKind.uint, .uint_val = v, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.data8) {
        const v = try cur.readU64();
        return AttrValue{ .kind = AttrValueKind.uint, .uint_val = v, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.udata) {
        const v = try cur.readUleb128();
        return AttrValue{ .kind = AttrValueKind.uint, .uint_val = v, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.sdata) {
        const v = try cur.readSleb128();
        return AttrValue{ .kind = AttrValueKind.sint, .uint_val = 0, .sint_val = v, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.string) {
        const s = try cur.readCString();
        return AttrValue{ .kind = AttrValueKind.str, .uint_val = 0, .sint_val = 0, .str_val = s, .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.strp) {
        const off: usize = if (is_64bit) @intCast(try cur.readU64()) else @intCast(try cur.readU32());
        const s = try readCStringAt(debug_str, off);
        return AttrValue{ .kind = AttrValueKind.str, .uint_val = 0, .sint_val = 0, .str_val = s, .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.flag) {
        const b = (try cur.readU8()) != 0;
        return AttrValue{ .kind = AttrValueKind.flag, .uint_val = 0, .sint_val = 0, .str_val = "", .flag_val = b, .bytes_val = "" };
    }
    if (actual_form == DwForm.flag_present) {
        return AttrValue{ .kind = AttrValueKind.flag, .uint_val = 0, .sint_val = 0, .str_val = "", .flag_val = true, .bytes_val = "" };
    }
    if (actual_form == DwForm.sec_offset || actual_form == DwForm.line_strp) {
        const off: u64 = if (is_64bit) try cur.readU64() else @intCast(try cur.readU32());
        return AttrValue{ .kind = AttrValueKind.sec_offset, .uint_val = off, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.ref1) {
        const r: u64 = @intCast(try cur.readU8());
        return AttrValue{ .kind = AttrValueKind.ref, .uint_val = r, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.ref2) {
        const r: u64 = @intCast(try cur.readU16());
        return AttrValue{ .kind = AttrValueKind.ref, .uint_val = r, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.ref4) {
        const r: u64 = @intCast(try cur.readU32());
        return AttrValue{ .kind = AttrValueKind.ref, .uint_val = r, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.ref8) {
        const r = try cur.readU64();
        return AttrValue{ .kind = AttrValueKind.ref, .uint_val = r, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.ref_udata) {
        const r = try cur.readUleb128();
        return AttrValue{ .kind = AttrValueKind.ref, .uint_val = r, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.ref_addr) {
        const r: u64 = if (is_64bit) try cur.readU64() else @intCast(try cur.readU32());
        return AttrValue{ .kind = AttrValueKind.ref, .uint_val = r, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = "" };
    }
    if (actual_form == DwForm.block1) {
        const len: usize = @intCast(try cur.readU8());
        const b = try cur.readBytes(len);
        return AttrValue{ .kind = AttrValueKind.bytes, .uint_val = 0, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = b };
    }
    if (actual_form == DwForm.block2) {
        const len: usize = @intCast(try cur.readU16());
        const b = try cur.readBytes(len);
        return AttrValue{ .kind = AttrValueKind.bytes, .uint_val = 0, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = b };
    }
    if (actual_form == DwForm.block4) {
        const len: usize = @intCast(try cur.readU32());
        const b = try cur.readBytes(len);
        return AttrValue{ .kind = AttrValueKind.bytes, .uint_val = 0, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = b };
    }
    if (actual_form == DwForm.block || actual_form == DwForm.exprloc) {
        const len: usize = @intCast(try cur.readUleb128());
        const b = try cur.readBytes(len);
        return AttrValue{ .kind = AttrValueKind.bytes, .uint_val = 0, .sint_val = 0, .str_val = "", .flag_val = false, .bytes_val = b };
    }

    return DwarfError.UnsupportedForm;
}

pub fn parseDebugInfoCu(
    alloc: std.mem.Allocator,
    debug_info: []const u8,
    debug_abbrev: []const u8,
    debug_str: []const u8,
    cu_offset: u64,
): !CompileUnit {
    if (cu_offset >= debug_info.len) {
        return DwarfError.TruncatedData;
    }

    let cur = Cursor.init(debug_info);
    cur.pos = @intCast(cu_offset);

    const len32 = try cur.readU32();
    let is_64bit = false;
    let unit_length: u64 = @intCast(len32);
    if (len32 == 0xffffffff) {
        is_64bit = true;
        unit_length = try cur.readU64();
    } else if (len32 >= 0xfffffff0) {
        return DwarfError.InvalidHeader;
    }

    const unit_start = cur.pos;
    if (cur.remaining() < unit_length) {
        return DwarfError.TruncatedData;
    }
    const unit_end = unit_start + @as(usize, @intCast(unit_length));

    const version = try cur.readU16();
    if (version != 4 && version != 5) {
        return DwarfError.UnsupportedVersion;
    }

    let unit_type: u8 = 1; // DW_UT_compile
    let abbrev_offset: u64 = 0;
    let addr_size: u8 = 8;

    if (version == 5) {
        unit_type = try cur.readU8();
        addr_size = try cur.readU8();
        abbrev_offset = if (is_64bit) try cur.readU64() else @intCast(try cur.readU32());
    } else {
        abbrev_offset = if (is_64bit) try cur.readU64() else @intCast(try cur.readU32());
        addr_size = try cur.readU8();
    }

    if (addr_size != 1 && addr_size != 2 && addr_size != 4 && addr_size != 8) {
        return DwarfError.InvalidHeader;
    }

    let abbrev_table = try parseAbbrevTable(alloc, debug_abbrev, abbrev_offset);
    defer abbrev_table.deinit(alloc);

    let nodes = std.ArrayList(DieNode).empty;
    errdefer {
        for (n of nodes.items) {
            if (n.attrs.len > 0) {
                alloc.free(n.attrs);
            }
        }
        nodes.deinit(alloc);
    }

    let depth: u32 = 0;
    let node_count: u32 = 0;

    while (cur.pos < unit_end) {
        node_count += 1;
        if (node_count > MAX_DIE_NODES) {
            return DwarfError.LoopLimitExceeded;
        }

        const die_offset: u64 = @intCast(cur.pos);
        const abbrev_code = try cur.readUleb128();
        if (abbrev_code == 0) {
            if (depth == 0) {
                break;
            }
            depth -= 1;
            continue;
        }

        const abbrev_opt = abbrev_table.find(abbrev_code);
        if (abbrev_opt == null) {
            return DwarfError.MissingAbbrev;
        }
        const abbrev = abbrev_opt.?;

        let attrs = std.ArrayList(DieAttr).empty;
        errdefer attrs.deinit(alloc);

        for (spec of abbrev.attr_specs) {
            const val = try decodeFormValue(&cur, spec.form, addr_size, is_64bit, debug_str);
            try attrs.append(alloc, DieAttr{
                .name = spec.name,
                .form = spec.form,
                .value = val,
            });
        }

        try nodes.append(alloc, DieNode{
            .offset = die_offset,
            .abbrev_code = abbrev_code,
            .tag = abbrev.tag,
            .has_children = abbrev.has_children,
            .depth = depth,
            .attrs = try attrs.toOwnedSlice(alloc),
        });

        if (abbrev.has_children) {
            depth += 1;
        }
    }

    return CompileUnit{
        .header = CompileUnitHeader{
            .unit_length = unit_length,
            .is_64bit = is_64bit,
            .version = version,
            .unit_type = unit_type,
            .debug_abbrev_offset = abbrev_offset,
            .address_size = addr_size,
        },
        .nodes = try nodes.toOwnedSlice(alloc),
    };
}
