// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc USB HID 1.11 Report Descriptor Parser & Summarizer)

// HID Item Types & Long Item Prefix (USB HID 1.11 §6.2.2)
pub const ITEM_TYPE_MAIN: u8 = 0;
pub const ITEM_TYPE_GLOBAL: u8 = 1;
pub const ITEM_TYPE_LOCAL: u8 = 2;
pub const ITEM_LONG_PREFIX: u8 = 0xFE;

// Main Item Tags
pub const MAIN_INPUT: u8 = 8;
pub const MAIN_OUTPUT: u8 = 9;
pub const MAIN_COLLECTION: u8 = 10;
pub const MAIN_FEATURE: u8 = 11;
pub const MAIN_END_COLLECTION: u8 = 12;

// Global Item Tags
pub const GLOBAL_USAGE_PAGE: u8 = 0;
pub const GLOBAL_LOGICAL_MIN: u8 = 1;
pub const GLOBAL_LOGICAL_MAX: u8 = 2;
pub const GLOBAL_PHYSICAL_MIN: u8 = 3;
pub const GLOBAL_PHYSICAL_MAX: u8 = 4;
pub const GLOBAL_UNIT_EXPONENT: u8 = 5;
pub const GLOBAL_UNIT: u8 = 6;
pub const GLOBAL_REPORT_SIZE: u8 = 7;
pub const GLOBAL_REPORT_ID: u8 = 8;
pub const GLOBAL_REPORT_COUNT: u8 = 9;
pub const GLOBAL_PUSH: u8 = 10;
pub const GLOBAL_POP: u8 = 11;

// Local Item Tags
pub const LOCAL_USAGE: u8 = 0;
pub const LOCAL_USAGE_MIN: u8 = 1;
pub const LOCAL_USAGE_MAX: u8 = 2;

// Collection Types
pub const COLL_PHYSICAL: u8 = 0;
pub const COLL_APPLICATION: u8 = 1;
pub const COLL_LOGICAL: u8 = 2;

// Common HID Usage Pages
pub const USAGE_PAGE_GENERIC_DESKTOP: u32 = 0x01;
pub const USAGE_PAGE_KEYBOARD: u32 = 0x07;
pub const USAGE_PAGE_LED: u32 = 0x08;
pub const USAGE_PAGE_BUTTON: u32 = 0x09;
pub const USAGE_PAGE_CONSUMER: u32 = 0x0C;

// Main Item Data Flags (Input / Output / Feature)
pub const FLAG_CONSTANT: u32 = 0x01;
pub const FLAG_VARIABLE: u32 = 0x02;
pub const FLAG_RELATIVE: u32 = 0x04;

/// Decoded USB HID 1.11 Short or Long Report Descriptor item.
pub struct HidItem {
    is_long: bool,
    item_type: u8,
    tag: u8,
    data: []const u8,

    fn isMain(): bool {
        return !_.is_long && _.item_type == ITEM_TYPE_MAIN;
    }

    fn isGlobal(): bool {
        return !_.is_long && _.item_type == ITEM_TYPE_GLOBAL;
    }

    fn isLocal(): bool {
        return !_.is_long && _.item_type == ITEM_TYPE_LOCAL;
    }

    /// Zero-extended little-endian unsigned 32-bit value (0 / 1 / 2 / 4 bytes).
    fn u32Value(): u32 {
        if (_.data.len == 0) {
            return 0;
        }
        if (_.data.len == 1) {
            return _.data[0] as u32;
        }
        if (_.data.len == 2) {
            return (_.data[0] as u32) | ((_.data[1] as u32) << 8);
        }
        if (_.data.len >= 4) {
            return (_.data[0] as u32)
                | ((_.data[1] as u32) << 8)
                | ((_.data[2] as u32) << 16)
                | ((_.data[3] as u32) << 24);
        }
        return (_.data[0] as u32)
            | ((_.data[1] as u32) << 8)
            | ((_.data[2] as u32) << 16);
    }

    /// Sign-extended little-endian signed 32-bit value (1 / 2 / 4 bytes).
    fn i32Value(): i32 {
        if (_.data.len == 0) {
            return 0;
        }
        if (_.data.len == 1) {
            const b = _.data[0] as u32;
            if ((b & 0x80) != 0) {
                return (b | 0xFFFFFF00) as i32;
            }
            return b as i32;
        }
        if (_.data.len == 2) {
            const w = (_.data[0] as u32) | ((_.data[1] as u32) << 8);
            if ((w & 0x8000) != 0) {
                return (w | 0xFFFF0000) as i32;
            }
            return w as i32;
        }
        return _.u32Value() as i32;
    }
}

/// Zero-copy streaming iterator over USB HID 1.11 Short and Long Report Descriptor items.
pub struct HidItemIter {
    desc: []const u8,
    pos: usize,
    malformed: bool,

    pub fn init(desc: []const u8): HidItemIter {
        return HidItemIter{
            .desc = desc,
            .pos = 0,
            .malformed = false,
        };
    }

    fn next(*_): HidItem? {
        if (_.malformed || _.pos >= _.desc.len) {
            return null;
        }
        const prefix = _.desc[_.pos];
        if (prefix == ITEM_LONG_PREFIX) {
            if (_.desc.len - _.pos < 3) {
                _.malformed = true;
                return null;
            }
            const dlen = _.desc[_.pos + 1] as usize;
            const long_tag = _.desc[_.pos + 2];
            const data_start = _.pos + 3;
            if (dlen > _.desc.len - data_start) {
                _.malformed = true;
                return null;
            }
            _.pos = data_start + dlen;
            return HidItem{
                .is_long = true,
                .item_type = 3,
                .tag = long_tag,
                .data = _.desc[data_start..(data_start + dlen)],
            };
        }

        const item_type = (prefix >> 2) & 0x03;
        if (item_type == 3) {
            // Reserved short item type (only 0xFE Long Item prefix is valid)
            _.malformed = true;
            return null;
        }
        const tag = (prefix >> 4) & 0x0F;
        const size_code = prefix & 0x03;
        const dlen: usize = if (size_code == 3) 4 else (size_code as usize);
        const data_start = _.pos + 1;
        if (dlen > _.desc.len - data_start) {
            _.malformed = true;
            return null;
        }
        _.pos = data_start + dlen;
        return HidItem{
            .is_long = false,
            .item_type = item_type,
            .tag = tag,
            .data = _.desc[data_start..(data_start + dlen)],
        };
    }

    fn isMalformed(): bool {
        return _.malformed;
    }
}

/// Summary of report bit layouts and collection nesting extracted from a HID Report Descriptor.
pub struct HidReportSummary {
    input_bits: u32,
    output_bits: u32,
    feature_bits: u32,
    collection_count: u32,
    max_depth: u32,
    has_report_ids: bool,
    usage_page: u32,
}

/// Single-pass zero-alloc summarizer for a USB HID 1.11 Report Descriptor.
/// Tracks `report_size`, `report_count`, `Push/Pop` global state stack, and `Collection` nesting.
/// Returns `null` on empty descriptor, malformed/truncated items, stack imbalance, or unbalanced collections.
pub fn summarizeHidDescriptor(desc: []const u8): HidReportSummary? {
    if (desc.len == 0) {
        return null;
    }
    let it = HidItemIter.init(desc);

    let report_size: u32 = 0;
    let report_count: u32 = 0;
    let cur_usage_page: u32 = 0;
    let first_usage_page: u32 = 0;
    let has_first_up = false;

    let stack_size: [16]u32 = [0; 16];
    let stack_count: [16]u32 = [0; 16];
    let stack_up: [16]u32 = [0; 16];
    let sp: usize = 0;

    let input_bits: u32 = 0;
    let output_bits: u32 = 0;
    let feature_bits: u32 = 0;
    let collection_count: u32 = 0;
    let depth: u32 = 0;
    let max_depth: u32 = 0;
    let has_report_ids = false;

    while (true) {
        if (item := it.next()) {
            if (item.isGlobal()) {
                if (item.tag == GLOBAL_USAGE_PAGE) {
                    cur_usage_page = item.u32Value();
                    if (!has_first_up) {
                        first_usage_page = cur_usage_page;
                        has_first_up = true;
                    }
                } else if (item.tag == GLOBAL_REPORT_SIZE) {
                    report_size = item.u32Value();
                } else if (item.tag == GLOBAL_REPORT_COUNT) {
                    report_count = item.u32Value();
                } else if (item.tag == GLOBAL_REPORT_ID) {
                    if (item.u32Value() != 0) {
                        has_report_ids = true;
                    }
                } else if (item.tag == GLOBAL_PUSH) {
                    if (sp >= 16) {
                        return null;
                    }
                    stack_size[sp] = report_size;
                    stack_count[sp] = report_count;
                    stack_up[sp] = cur_usage_page;
                    sp += 1;
                } else if (item.tag == GLOBAL_POP) {
                    if (sp == 0) {
                        return null;
                    }
                    sp -= 1;
                    report_size = stack_size[sp];
                    report_count = stack_count[sp];
                    cur_usage_page = stack_up[sp];
                }
            } else if (item.isMain()) {
                if (item.tag == MAIN_INPUT) {
                    input_bits += report_size * report_count;
                } else if (item.tag == MAIN_OUTPUT) {
                    output_bits += report_size * report_count;
                } else if (item.tag == MAIN_FEATURE) {
                    feature_bits += report_size * report_count;
                } else if (item.tag == MAIN_COLLECTION) {
                    collection_count += 1;
                    depth += 1;
                    if (depth > max_depth) {
                        max_depth = depth;
                    }
                } else if (item.tag == MAIN_END_COLLECTION) {
                    if (depth == 0) {
                        return null;
                    }
                    depth -= 1;
                }
            }
        } else {
            break;
        }
    }

    if (it.isMalformed() || depth != 0 || sp != 0) {
        return null;
    }

    return HidReportSummary{
        .input_bits = input_bits,
        .output_bits = output_bits,
        .feature_bits = feature_bits,
        .collection_count = collection_count,
        .max_depth = max_depth,
        .has_report_ids = has_report_ids,
        .usage_page = first_usage_page,
    };
}
