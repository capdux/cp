// Source: cp/lib/dwarf/line.cp (DWARF v4/v5 .debug_line state machine, lookup, and v4 encoder)

const std = @import("std");
const types = @import("./types.cp");
const DwarfError = types.DwarfError;
const Cursor = types.Cursor;

// Standard opcodes (DWARF v4/v5)
pub const DW_LNS_copy: u8 = 1;
pub const DW_LNS_advance_pc: u8 = 2;
pub const DW_LNS_advance_line: u8 = 3;
pub const DW_LNS_set_file: u8 = 4;
pub const DW_LNS_set_column: u8 = 5;
pub const DW_LNS_negate_stmt: u8 = 6;
pub const DW_LNS_set_basic_block: u8 = 7;
pub const DW_LNS_const_add_pc: u8 = 8;
pub const DW_LNS_fixed_advance_pc: u8 = 9;
pub const DW_LNS_set_prologue_end: u8 = 10;
pub const DW_LNS_set_epilogue_begin: u8 = 11;
pub const DW_LNS_set_isa: u8 = 12;

// Extended opcodes
pub const DW_LNE_end_sequence: u8 = 1;
pub const DW_LNE_set_address: u8 = 2;
pub const DW_LNE_define_file: u8 = 3;
pub const DW_LNE_set_discriminator: u8 = 4;

// DWARF v5 line number header entry content types
pub const DW_LNCT_path: u64 = 0x1;
pub const DW_LNCT_directory_index: u64 = 0x2;
pub const DW_LNCT_timestamp: u64 = 0x3;
pub const DW_LNCT_size: u64 = 0x4;
pub const DW_LNCT_MD5: u64 = 0x5;

pub const MAX_LINE_PROGRAM_STEPS: u32 = 1000000;

pub struct FileEntry {
    name: []const u8,
    dir_index: u32,
    mod_time: u64,
    length: u64,
}

pub struct LineProgramHeader {
    unit_length: u64,
    is_64bit: bool,
    version: u16,
    address_size: u8,
    segment_selector_size: u8,
    header_length: u64,
    minimum_instruction_length: u8,
    maximum_operations_per_instruction: u8,
    default_is_stmt: bool,
    line_base: i8,
    line_range: u8,
    opcode_base: u8,
    standard_opcode_lengths: []const u8,
    include_directories: []const []const u8,
    file_names: []const FileEntry,
}

pub struct LineRow {
    address: u64,
    file_index: u32,
    line: i64,
    column: u32,
    is_stmt: bool,
    basic_block: bool,
    end_sequence: bool,
    prologue_end: bool,
    epilogue_begin: bool,
    isa: u32,
    discriminator: u32,
}

pub struct LineLocation {
    file: []const u8,
    directory: []const u8,
    line: i64,
    column: u32,
    address: u64,
}

pub struct LineEncodeEntry {
    address: u64,
    file_index: u32,
    line: i64,
    column: u32,
    is_stmt: bool,
    end_sequence: bool,
}

pub struct LineTable {
    header: LineProgramHeader,
    rows: []LineRow,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.header.standard_opcode_lengths.len > 0) {
            alloc.free(_.header.standard_opcode_lengths);
        }
        if (_.header.include_directories.len > 0) {
            alloc.free(_.header.include_directories);
        }
        if (_.header.file_names.len > 0) {
            alloc.free(_.header.file_names);
        }
        if (_.rows.len > 0) {
            alloc.free(_.rows);
        }
    }

    pub fn lookupAddress(pc: u64): LineLocation? {
        return lookupAddressImpl(&_, pc);
    }
}

pub fn lookupAddress(table: *const LineTable, pc: u64): LineLocation? {
    return lookupAddressImpl(table, pc);
}

fn lookupAddressImpl(table: *const LineTable, pc: u64): LineLocation? {
    if (table.rows.len == 0) {
        return null;
    }

    // Binary search for the last row with row.address <= pc
    let lo: usize = 0;
    let hi: usize = table.rows.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        if (table.rows[mid].address <= pc) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        return null;
    }

    const row = table.rows[lo - 1];
    if (row.end_sequence && row.address < pc) {
        return null;
    }

    let file_name: []const u8 = "";
    let dir_name: []const u8 = "";

    if (table.header.version >= 5) {
        if (row.file_index < table.header.file_names.len) {
            const fe = table.header.file_names[row.file_index];
            file_name = fe.name;
            if (fe.dir_index < table.header.include_directories.len) {
                dir_name = table.header.include_directories[fe.dir_index];
            }
        }
    } else {
        if (row.file_index >= 1 && row.file_index <= table.header.file_names.len) {
            const fe = table.header.file_names[row.file_index - 1];
            file_name = fe.name;
            if (fe.dir_index >= 1 && fe.dir_index <= table.header.include_directories.len) {
                dir_name = table.header.include_directories[fe.dir_index - 1];
            }
        }
    }

    return LineLocation{
        .file = file_name,
        .directory = dir_name,
        .line = row.line,
        .column = row.column,
        .address = row.address,
    };
}

struct LineState {
    address: u64,
    op_index: u32,
    file: u32,
    line: i64,
    column: u32,
    is_stmt: bool,
    basic_block: bool,
    end_sequence: bool,
    prologue_end: bool,
    epilogue_begin: bool,
    isa: u32,
    discriminator: u32,

    static fn reset(default_is_stmt: bool): LineState {
        return LineState{
            .address = 0,
            .op_index = 0,
            .file = 1,
            .line = 1,
            .column = 0,
            .is_stmt = default_is_stmt,
            .basic_block = false,
            .end_sequence = false,
            .prologue_end = false,
            .epilogue_begin = false,
            .isa = 0,
            .discriminator = 0,
        };
    }

    fn toRow(): LineRow {
        return LineRow{
            .address = _.address,
            .file_index = _.file,
            .line = _.line,
            .column = _.column,
            .is_stmt = _.is_stmt,
            .basic_block = _.basic_block,
            .end_sequence = _.end_sequence,
            .prologue_end = _.prologue_end,
            .epilogue_begin = _.epilogue_begin,
            .isa = _.isa,
            .discriminator = _.discriminator,
        };
    }
}

fn validateFileIndex(header: *const LineProgramHeader, file_index: u32): !void {
    if (header.version >= 5) {
        if (file_index >= header.file_names.len) {
            return DwarfError.InvalidFileIndex;
        }
    } else {
        if (file_index == 0 || file_index > header.file_names.len) {
            return DwarfError.InvalidFileIndex;
        }
    }
}

struct V5FormatEntry {
    content_type: u64,
    form: u64,
}

fn readV5FormatList(alloc: std.mem.Allocator, cur: *Cursor, is_64bit: bool): ![]V5FormatEntry {
    const count = try cur.readU8();
    let list = std.ArrayList(V5FormatEntry).empty;
    errdefer list.deinit(alloc);
    let i: u8 = 0;
    while (i < count) {
        const content_type = try cur.readUleb128();
        const form = try cur.readUleb128();
        try list.append(alloc, V5FormatEntry{ .content_type = content_type, .form = form });
        i += 1;
    }
    * = is_64bit;
    return try list.toOwnedSlice(alloc);
}

fn skipOrReadV5String(cur: *Cursor, form: u64, is_64bit: bool): ![]const u8 {
    if (form == 0x08) { // DW_FORM_string
        return try cur.readCString();
    }
    if (form == 0x0e || form == 0x1f) { // DW_FORM_strp / DW_FORM_line_strp
        if (is_64bit) {
            * = try cur.readU64();
        } else {
            * = try cur.readU32();
        }
        return "";
    }
    return DwarfError.UnsupportedForm;
}

fn readV5Uint(cur: *Cursor, form: u64): !u64 {
    if (form == 0x0b) { // DW_FORM_data1
        return @intCast(try cur.readU8());
    }
    if (form == 0x05) { // DW_FORM_data2
        return @intCast(try cur.readU16());
    }
    if (form == 0x06) { // DW_FORM_data4
        return @intCast(try cur.readU32());
    }
    if (form == 0x07) { // DW_FORM_data8
        return try cur.readU64();
    }
    if (form == 0x0f) { // DW_FORM_udata
        return try cur.readUleb128();
    }
    return DwarfError.UnsupportedForm;
}

pub fn parseDebugLine(alloc: std.mem.Allocator, data: []const u8, default_addr_size: u8): !LineTable {
    let cur = Cursor.init(data);

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

    let addr_size: u8 = default_addr_size;
    let seg_size: u8 = 0;
    if (version == 5) {
        addr_size = try cur.readU8();
        seg_size = try cur.readU8();
    }

    const header_length: u64 = if (is_64bit) try cur.readU64() else @intCast(try cur.readU32());
    const header_start = cur.pos;
    const header_end = header_start + @as(usize, @intCast(header_length));
    if (header_end > unit_end) {
        return DwarfError.TruncatedData;
    }

    const min_inst_len = try cur.readU8();
    if (min_inst_len == 0) {
        return DwarfError.InvalidHeader;
    }
    const max_ops_per_inst = try cur.readU8();
    if (max_ops_per_inst == 0) {
        return DwarfError.InvalidHeader;
    }

    const default_is_stmt = (try cur.readU8()) != 0;
    const line_base = try cur.readI8();
    const line_range = try cur.readU8();
    if (line_range == 0) {
        return DwarfError.InvalidHeader;
    }
    const opcode_base = try cur.readU8();
    if (opcode_base == 0) {
        return DwarfError.InvalidHeader;
    }

    let std_op_lens = std.ArrayList(u8).empty;
    errdefer std_op_lens.deinit(alloc);
    let op_idx: u8 = 1;
    while (op_idx < opcode_base) {
        try std_op_lens.append(alloc, try cur.readU8());
        op_idx += 1;
    }
    const std_op_lens_slice = try std_op_lens.toOwnedSlice(alloc);
    errdefer alloc.free(std_op_lens_slice);

    let dirs = std.ArrayList(string).empty;
    errdefer dirs.deinit(alloc);
    let files = std.ArrayList(FileEntry).empty;
    errdefer files.deinit(alloc);

    if (version == 4) {
        while (true) {
            if (cur.pos >= header_end) {
                return DwarfError.TruncatedData;
            }
            if (cur.data[cur.pos] == 0) {
                cur.pos += 1;
                break;
            }
            const d = try cur.readCString();
            try dirs.append(alloc, d);
        }

        while (true) {
            if (cur.pos >= header_end) {
                return DwarfError.TruncatedData;
            }
            if (cur.data[cur.pos] == 0) {
                cur.pos += 1;
                break;
            }
            const name = try cur.readCString();
            const dir_idx: u32 = @intCast(try cur.readUleb128());
            const mod_time = try cur.readUleb128();
            const flen = try cur.readUleb128();
            try files.append(alloc, FileEntry{
                .name = name,
                .dir_index = dir_idx,
                .mod_time = mod_time,
                .length = flen,
            });
        }
    } else {
        // DWARF v5 directory and file name tables
        const dir_formats = try readV5FormatList(alloc, &cur, is_64bit);
        defer alloc.free(dir_formats);
        const dir_count = try cur.readUleb128();
        let di: u64 = 0;
        while (di < dir_count) {
            let dir_path: []const u8 = "";
            for (fmt of dir_formats) {
                if (fmt.content_type == DW_LNCT_path) {
                    dir_path = try skipOrReadV5String(&cur, fmt.form, is_64bit);
                } else {
                    * = try readV5Uint(&cur, fmt.form);
                }
            }
            try dirs.append(alloc, dir_path);
            di += 1;
        }

        const file_formats = try readV5FormatList(alloc, &cur, is_64bit);
        defer alloc.free(file_formats);
        const file_count = try cur.readUleb128();
        let fi: u64 = 0;
        while (fi < file_count) {
            let fname: []const u8 = "";
            let fdir: u32 = 0;
            let fmod: u64 = 0;
            let flen: u64 = 0;
            for (fmt of file_formats) {
                if (fmt.content_type == DW_LNCT_path) {
                    fname = try skipOrReadV5String(&cur, fmt.form, is_64bit);
                } else if (fmt.content_type == DW_LNCT_directory_index) {
                    fdir = @intCast(try readV5Uint(&cur, fmt.form));
                } else if (fmt.content_type == DW_LNCT_timestamp) {
                    fmod = try readV5Uint(&cur, fmt.form);
                } else if (fmt.content_type == DW_LNCT_size) {
                    flen = try readV5Uint(&cur, fmt.form);
                } else if (fmt.content_type == DW_LNCT_MD5 && fmt.form == 0x1e) {
                    * = try cur.readBytes(16);
                } else {
                    * = try readV5Uint(&cur, fmt.form);
                }
            }
            try files.append(alloc, FileEntry{
                .name = fname,
                .dir_index = fdir,
                .mod_time = fmod,
                .length = flen,
            });
            fi += 1;
        }
    }

    const dirs_slice = try dirs.toOwnedSlice(alloc);
    errdefer alloc.free(dirs_slice);
    const files_slice = try files.toOwnedSlice(alloc);
    errdefer alloc.free(files_slice);

    const header = LineProgramHeader{
        .unit_length = unit_length,
        .is_64bit = is_64bit,
        .version = version,
        .address_size = addr_size,
        .segment_selector_size = seg_size,
        .header_length = header_length,
        .minimum_instruction_length = min_inst_len,
        .maximum_operations_per_instruction = max_ops_per_inst,
        .default_is_stmt = default_is_stmt,
        .line_base = line_base,
        .line_range = line_range,
        .opcode_base = opcode_base,
        .standard_opcode_lengths = std_op_lens_slice,
        .include_directories = dirs_slice,
        .file_names = files_slice,
    };

    cur.pos = header_end;
    let rows = std.ArrayList(LineRow).empty;
    errdefer rows.deinit(alloc);

    let state = LineState.reset(default_is_stmt);
    let step_count: u32 = 0;

    while (cur.pos < unit_end) {
        step_count += 1;
        if (step_count > MAX_LINE_PROGRAM_STEPS) {
            return DwarfError.LoopLimitExceeded;
        }

        const opcode = try cur.readU8();
        if (opcode == 0) {
            // Extended opcode
            const ext_len = try cur.readUleb128();
            if (ext_len == 0) {
                return DwarfError.InvalidOpcode;
            }
            const ext_end = cur.pos + @as(usize, @intCast(ext_len));
            if (ext_end > unit_end) {
                return DwarfError.TruncatedData;
            }
            const ext_op = try cur.readU8();
            if (ext_op == DW_LNE_end_sequence) {
                state.end_sequence = true;
                try rows.append(alloc, state.toRow());
                state = LineState.reset(default_is_stmt);
            } else if (ext_op == DW_LNE_set_address) {
                const target_addr_size: u8 = if (ext_end - cur.pos == 4) 4 else if (ext_end - cur.pos == 8) 8 else addr_size;
                state.address = try cur.readAddr(target_addr_size);
                state.op_index = 0;
            } else if (ext_op == DW_LNE_set_discriminator) {
                state.discriminator = @intCast(try cur.readUleb128());
            } else if (ext_op == DW_LNE_define_file) {
                * = try cur.readCString();
                * = try cur.readUleb128();
                * = try cur.readUleb128();
                * = try cur.readUleb128();
            }
            cur.pos = ext_end;
        } else if (opcode < opcode_base) {
            // Standard opcode
            if (opcode == DW_LNS_copy) {
                try validateFileIndex(&header, state.file);
                try rows.append(alloc, state.toRow());
                state.discriminator = 0;
                state.basic_block = false;
                state.prologue_end = false;
                state.epilogue_begin = false;
            } else if (opcode == DW_LNS_advance_pc) {
                const adv = try cur.readUleb128();
                state.address += adv * @as(u64, @intCast(min_inst_len));
            } else if (opcode == DW_LNS_advance_line) {
                const adv = try cur.readSleb128();
                state.line += adv;
            } else if (opcode == DW_LNS_set_file) {
                state.file = @intCast(try cur.readUleb128());
            } else if (opcode == DW_LNS_set_column) {
                state.column = @intCast(try cur.readUleb128());
            } else if (opcode == DW_LNS_negate_stmt) {
                state.is_stmt = !state.is_stmt;
            } else if (opcode == DW_LNS_set_basic_block) {
                state.basic_block = true;
            } else if (opcode == DW_LNS_const_add_pc) {
                const adj_op: u64 = @intCast(255 - opcode_base);
                const addr_inc = (adj_op / @as(u64, @intCast(line_range))) * @as(u64, @intCast(min_inst_len));
                state.address += addr_inc;
            } else if (opcode == DW_LNS_fixed_advance_pc) {
                const adv = try cur.readU16();
                state.address += @as(u64, @intCast(adv));
                state.op_index = 0;
            } else if (opcode == DW_LNS_set_prologue_end) {
                state.prologue_end = true;
            } else if (opcode == DW_LNS_set_epilogue_begin) {
                state.epilogue_begin = true;
            } else if (opcode == DW_LNS_set_isa) {
                state.isa = @intCast(try cur.readUleb128());
            } else {
                const arg_count = std_op_lens_slice[opcode - 1];
                let ai: u8 = 0;
                while (ai < arg_count) {
                    * = try cur.readUleb128();
                    ai += 1;
                }
            }
        } else {
            // Special opcode
            const adj_op: i64 = @intCast(opcode - opcode_base);
            const lr: i64 = @intCast(line_range);
            const lb: i64 = @intCast(line_base);
            const addr_inc: u64 = @intCast(@divTrunc(adj_op, lr) * @as(i64, @intCast(min_inst_len)));
            const line_inc: i64 = lb + @rem(adj_op, lr);
            state.address += addr_inc;
            state.line += line_inc;

            try validateFileIndex(&header, state.file);
            try rows.append(alloc, state.toRow());
            state.basic_block = false;
            state.prologue_end = false;
            state.epilogue_begin = false;
            state.discriminator = 0;
        }
    }

    const rows_slice = try rows.toOwnedSlice(alloc);
    return LineTable{
        .header = header,
        .rows = rows_slice,
    };
}

pub fn encodeDebugLineV4(
    alloc: std.mem.Allocator,
    dirs: []const []const u8,
    files: []const FileEntry,
    entries: []const LineEncodeEntry,
    addr_size: u8,
): ![]u8 {
    let hdr = std.ArrayList(u8).empty;
    defer hdr.deinit(alloc);

    const opcode_base: u8 = 13;
    const line_base: i8 = -5;
    const line_range: u8 = 14;
    const min_inst_len: u8 = 1;

    try hdr.append(alloc, min_inst_len);
    try hdr.append(alloc, 1); // maximum_operations_per_instruction
    try hdr.append(alloc, 1); // default_is_stmt
    try hdr.append(alloc, @bitCast(line_base));
    try hdr.append(alloc, line_range);
    try hdr.append(alloc, opcode_base);

    // standard_opcode_lengths for opcodes 1..12
    const std_lens: [12]u8 = [ 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1 ];
    for (l of std_lens) {
        try hdr.append(alloc, l);
    }

    // include_directories
    for (d of dirs) {
        try hdr.appendSlice(alloc, d);
        try hdr.append(alloc, 0);
    }
    try hdr.append(alloc, 0);

    // file_names
    for (f of files) {
        try hdr.appendSlice(alloc, f.name);
        try hdr.append(alloc, 0);
        try types.writeUleb128(alloc, &hdr, @intCast(f.dir_index));
        try types.writeUleb128(alloc, &hdr, f.mod_time);
        try types.writeUleb128(alloc, &hdr, f.length);
    }
    try hdr.append(alloc, 0);

    let prog = std.ArrayList(u8).empty;
    defer prog.deinit(alloc);

    let cur_addr: u64 = 0;
    let cur_line: i64 = 1;
    let cur_file: u32 = 1;
    let cur_col: u32 = 0;
    let cur_stmt: bool = true;
    let has_addr = false;

    for (e of entries) {
        if (!has_addr) {
            // DW_LNE_set_address
            try prog.append(alloc, 0);
            try types.writeUleb128(alloc, &prog, @intCast(1 + addr_size));
            try prog.append(alloc, DW_LNE_set_address);
            if (addr_size == 4) {
                try types.writeU32(alloc, &prog, @intCast(e.address));
            } else {
                try types.writeU64(alloc, &prog, e.address);
            }
            cur_addr = e.address;
            has_addr = true;
        }

        if (e.file_index != cur_file) {
            try prog.append(alloc, DW_LNS_set_file);
            try types.writeUleb128(alloc, &prog, @intCast(e.file_index));
            cur_file = e.file_index;
        }
        if (e.column != cur_col) {
            try prog.append(alloc, DW_LNS_set_column);
            try types.writeUleb128(alloc, &prog, @intCast(e.column));
            cur_col = e.column;
        }
        if (e.is_stmt != cur_stmt) {
            try prog.append(alloc, DW_LNS_negate_stmt);
            cur_stmt = e.is_stmt;
        }

        const addr_diff = e.address - cur_addr;
        const line_diff = e.line - cur_line;

        if (e.end_sequence) {
            if (addr_diff > 0) {
                try prog.append(alloc, DW_LNS_advance_pc);
                try types.writeUleb128(alloc, &prog, addr_diff);
            }
            try prog.append(alloc, 0);
            try types.writeUleb128(alloc, &prog, 1);
            try prog.append(alloc, DW_LNE_end_sequence);
            cur_addr = 0;
            cur_line = 1;
            cur_file = 1;
            cur_col = 0;
            cur_stmt = true;
            has_addr = false;
            continue;
        }

        // Try special opcode if in range
        const lb: i64 = @intCast(line_base);
        const lr: i64 = @intCast(line_range);
        if (line_diff >= lb && line_diff < lb + lr && addr_diff < 16) {
            const special: i64 = (line_diff - lb) + (lr * @as(i64, @intCast(addr_diff))) + @as(i64, @intCast(opcode_base));
            if (special >= @as(i64, @intCast(opcode_base)) && special <= 255) {
                try prog.append(alloc, @intCast(special));
                cur_addr = e.address;
                cur_line = e.line;
                continue;
            }
        }

        if (addr_diff > 0) {
            try prog.append(alloc, DW_LNS_advance_pc);
            try types.writeUleb128(alloc, &prog, addr_diff);
            cur_addr = e.address;
        }
        if (line_diff != 0) {
            try prog.append(alloc, DW_LNS_advance_line);
            try types.writeSleb128(alloc, &prog, line_diff);
            cur_line = e.line;
        }
        try prog.append(alloc, DW_LNS_copy);
    }

    // Assemble full unit: unit_length (4) + version (2) + header_length (4) + hdr + prog
    const header_len: u32 = @intCast(hdr.items.len);
    const unit_len: u32 = @intCast(2 + 4 + hdr.items.len + prog.items.len);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try types.writeU32(alloc, &out, unit_len);
    try types.writeU16(alloc, &out, 4);
    try types.writeU32(alloc, &out, header_len);
    try out.appendSlice(alloc, hdr.items);
    try out.appendSlice(alloc, prog.items);

    return try out.toOwnedSlice(alloc);
}
