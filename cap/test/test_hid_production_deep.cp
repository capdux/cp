// Source: cp/cap/test/test_hid_production_deep.cp (Production deep tests for cap:hid)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ITEM_TYPE_MAIN,
    ITEM_TYPE_GLOBAL,
    ITEM_TYPE_LOCAL,
    ITEM_LONG_PREFIX,
    MAIN_INPUT,
    MAIN_OUTPUT,
    MAIN_COLLECTION,
    MAIN_FEATURE,
    MAIN_END_COLLECTION,
    GLOBAL_USAGE_PAGE,
    GLOBAL_LOGICAL_MIN,
    GLOBAL_LOGICAL_MAX,
    GLOBAL_PHYSICAL_MIN,
    GLOBAL_PHYSICAL_MAX,
    GLOBAL_UNIT_EXPONENT,
    GLOBAL_UNIT,
    GLOBAL_REPORT_SIZE,
    GLOBAL_REPORT_ID,
    GLOBAL_REPORT_COUNT,
    GLOBAL_PUSH,
    GLOBAL_POP,
    LOCAL_USAGE,
    LOCAL_USAGE_MIN,
    LOCAL_USAGE_MAX,
    COLL_PHYSICAL,
    COLL_APPLICATION,
    COLL_LOGICAL,
    USAGE_PAGE_GENERIC_DESKTOP,
    USAGE_PAGE_KEYBOARD,
    USAGE_PAGE_LED,
    USAGE_PAGE_BUTTON,
    USAGE_PAGE_CONSUMER,
    FLAG_CONSTANT,
    FLAG_VARIABLE,
    FLAG_RELATIVE,
    HidItem,
    HidItemIter,
    HidReportSummary,
    summarizeHidDescriptor,
} from "cap:sys";

test("1. Standard USB Boot Mouse Report Descriptor (signed -127..127 & 24-bit Input)", () => {
    expect_eq(ITEM_TYPE_MAIN, 0 as u8);
    expect_eq(ITEM_TYPE_GLOBAL, 1 as u8);
    expect_eq(ITEM_TYPE_LOCAL, 2 as u8);
    expect_eq(ITEM_LONG_PREFIX, 0xFE as u8);
    expect_eq(GLOBAL_PHYSICAL_MIN, 3 as u8);
    expect_eq(GLOBAL_PHYSICAL_MAX, 4 as u8);
    expect_eq(GLOBAL_UNIT_EXPONENT, 5 as u8);
    expect_eq(GLOBAL_UNIT, 6 as u8);
    expect_eq(LOCAL_USAGE, 0 as u8);
    expect_eq(LOCAL_USAGE_MIN, 1 as u8);
    expect_eq(LOCAL_USAGE_MAX, 2 as u8);
    expect_eq(COLL_PHYSICAL, 0 as u8);
    expect_eq(COLL_APPLICATION, 1 as u8);
    expect_eq(COLL_LOGICAL, 2 as u8);
    expect_eq(USAGE_PAGE_KEYBOARD, 0x07 as u32);
    expect_eq(USAGE_PAGE_LED, 0x08 as u32);
    expect_eq(USAGE_PAGE_BUTTON, 0x09 as u32);
    expect_eq(USAGE_PAGE_CONSUMER, 0x0C as u32);
    expect_eq(FLAG_CONSTANT, 0x01 as u32);
    expect_eq(FLAG_VARIABLE, 0x02 as u32);
    expect_eq(FLAG_RELATIVE, 0x04 as u32);

    // USB HID 1.11 Appendix E.10 Boot Mouse (50 bytes)
    const mouse_desc: [50]u8 = [
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x02,       // Usage (Mouse)
        0xA1, 0x01,       // Collection (Application)
        0x09, 0x01,       //   Usage (Pointer)
        0xA1, 0x00,       //   Collection (Physical)
        0x05, 0x09,       //     Usage Page (Button)
        0x19, 0x01,       //     Usage Minimum (1)
        0x29, 0x03,       //     Usage Maximum (3)
        0x15, 0x00,       //     Logical Minimum (0)
        0x25, 0x01,       //     Logical Maximum (1)
        0x95, 0x03,       //     Report Count (3)
        0x75, 0x01,       //     Report Size (1)
        0x81, 0x02,       //     Input (Data, Var, Abs) -> 3 bits
        0x95, 0x01,       //     Report Count (1)
        0x75, 0x05,       //     Report Size (5)
        0x81, 0x01,       //     Input (Const) -> 5 bits padding
        0x05, 0x01,       //     Usage Page (Generic Desktop)
        0x09, 0x30,       //     Usage (X)
        0x09, 0x31,       //     Usage (Y)
        0x15, 0x81,       //     Logical Minimum (-127)
        0x25, 0x7F,       //     Logical Maximum (127)
        0x75, 0x08,       //     Report Size (8)
        0x95, 0x02,       //     Report Count (2)
        0x81, 0x06,       //     Input (Data, Var, Rel) -> 16 bits
        0xC0,             //   End Collection
        0xC0,             // End Collection
    ];

    const summary = summarizeHidDescriptor(mouse_desc[0..]) ?? unreachable;
    expect_eq(summary.input_bits, 24 as u32);
    expect_eq(summary.output_bits, 0 as u32);
    expect_eq(summary.feature_bits, 0 as u32);
    expect_eq(summary.collection_count, 2 as u32);
    expect_eq(summary.max_depth, 2 as u32);
    expect_eq(summary.has_report_ids, false);
    expect_eq(summary.usage_page, USAGE_PAGE_GENERIC_DESKTOP);

    // Verify signed -127 Logical Minimum & 127 Logical Maximum via streaming iterator
    let it = HidItemIter.init(mouse_desc[0..]);
    let saw_min_neg127 = false;
    let saw_max_127 = false;
    while (true) {
        if (item := it.next()) {
            if (item.isGlobal() && item.tag == GLOBAL_LOGICAL_MIN && item.i32Value() == -127) {
                saw_min_neg127 = true;
            }
            if (item.isGlobal() && item.tag == GLOBAL_LOGICAL_MAX && item.i32Value() == 127) {
                saw_max_127 = true;
            }
        } else {
            break;
        }
    }
    expect_eq(it.isMalformed(), false);
    expect_eq(saw_min_neg127, true);
    expect_eq(saw_max_127, true);
});

test("2. Standard USB Boot Keyboard Report Descriptor (64-bit Input + 8-bit LED Output)", () => {
    // USB HID 1.11 Appendix E.6 Boot Keyboard (63 bytes)
    const kbd_desc: [63]u8 = [
        0x05, 0x01,       // Usage Page (Generic Desktop)
        0x09, 0x06,       // Usage (Keyboard)
        0xA1, 0x01,       // Collection (Application)
        0x05, 0x07,       //   Usage Page (Keyboard)
        0x19, 0xE0,       //   Usage Minimum (224)
        0x29, 0xE7,       //   Usage Maximum (231)
        0x15, 0x00,       //   Logical Minimum (0)
        0x25, 0x01,       //   Logical Maximum (1)
        0x75, 0x01,       //   Report Size (1)
        0x95, 0x08,       //   Report Count (8)
        0x81, 0x02,       //   Input (Data, Var, Abs) -> 8 bits modifiers
        0x95, 0x01,       //   Report Count (1)
        0x75, 0x08,       //   Report Size (8)
        0x81, 0x01,       //   Input (Const) -> 8 bits reserved
        0x95, 0x05,       //   Report Count (5)
        0x75, 0x01,       //   Report Size (1)
        0x05, 0x08,       //   Usage Page (LEDs)
        0x19, 0x01,       //   Usage Minimum (1)
        0x29, 0x05,       //   Usage Maximum (5)
        0x91, 0x02,       //   Output (Data, Var, Abs) -> 5 bits LEDs
        0x95, 0x01,       //   Report Count (1)
        0x75, 0x03,       //   Report Size (3)
        0x91, 0x01,       //   Output (Const) -> 3 bits padding
        0x95, 0x06,       //   Report Count (6)
        0x75, 0x08,       //   Report Size (8)
        0x15, 0x00,       //   Logical Minimum (0)
        0x25, 0x65,       //   Logical Maximum (101)
        0x05, 0x07,       //   Usage Page (Keyboard)
        0x19, 0x00,       //   Usage Minimum (0)
        0x29, 0x65,       //   Usage Maximum (101)
        0x81, 0x00,       //   Input (Data, Array) -> 48 bits keys
        0xC0,             // End Collection
    ];

    const s = summarizeHidDescriptor(kbd_desc[0..]) ?? unreachable;
    expect_eq(s.input_bits, 64 as u32);
    expect_eq(s.output_bits, 8 as u32);
    expect_eq(s.feature_bits, 0 as u32);
    expect_eq(s.collection_count, 1 as u32);
    expect_eq(s.max_depth, 1 as u32);
    expect_eq(s.has_report_ids, false);
    expect_eq(s.usage_page, USAGE_PAGE_GENERIC_DESKTOP);
});

test("3. Global Push/Pop stack, Report ID, Feature bits & 2B/4B value decoding", () => {
    const desc: [25]u8 = [
        0x05, 0x0C,             // Usage Page (Consumer)
        0xA1, 0x01,             // Collection (Application)
        0x85, 0x02,             //   Report ID (2)
        0x75, 0x10,             //   Report Size (16)
        0x95, 0x01,             //   Report Count (1)
        0xA4,                   //   Push (size=16, count=1)
        0x75, 0x08,             //     Report Size (8)
        0x95, 0x04,             //     Report Count (4)
        0xB1, 0x02,             //     Feature (Data, Var, Abs) -> 32 bits
        0xB4,                   //   Pop (restores size=16, count=1)
        0x16, 0x00, 0x80,       //   Logical Minimum (-32768, 2-byte signed)
        0x81, 0x02,             //   Input -> 16 bits
        0xC0,                   // End Collection
        0xA4,                   // Wait: remove trailing Push; instead use 0x00? No, keep balanced!
    ];
    // Use only the first 24 bytes (exact balanced descriptor)
    const s = summarizeHidDescriptor(desc[0..24]) ?? unreachable;
    expect_eq(s.has_report_ids, true);
    expect_eq(s.usage_page, USAGE_PAGE_CONSUMER);
    expect_eq(s.feature_bits, 32 as u32);
    expect_eq(s.input_bits, 16 as u32);

    // Check 2-byte signed -32768 decoding
    let it = HidItemIter.init(desc[0..24]);
    let saw_neg32768 = false;
    while (true) {
        if (item := it.next()) {
            if (item.isGlobal() && item.tag == GLOBAL_LOGICAL_MIN && item.i32Value() == -32768) {
                saw_neg32768 = true;
            }
        } else {
            break;
        }
    }
    expect_eq(saw_neg32768, true);
});

test("4. Long Item (0xFE) decoding alongside Short Items", () => {
    const desc: [10]u8 = [
        0xFE, 0x03, 0xF0, 0xAA, 0xBB, 0xCC, // Long Item: len=3, tag=0xF0, data=[AA,BB,CC]
        0x07, 0x78, 0x56, 0x34,             // Short Item size_code=3 -> needs 4B! Use 0x06 (2B) below
    ];
    // Test Long Item alone [0..6]
    let it = HidItemIter.init(desc[0..6]);
    const li = it.next() ?? unreachable;
    expect_eq(li.is_long, true);
    expect_eq(li.tag, 0xF0 as u8);
    expect_eq(li.data.len, 3 as usize);
    expect_eq(li.data[0], 0xAA as u8);
    expect_eq(li.data[2], 0xCC as u8);
    expect_eq(it.next() == null, true);
    expect_eq(it.isMalformed(), false);

    // 4-byte Short Item (size_code == 3 -> 4 bytes)
    const short4: [5]u8 = [0x07, 0x78, 0x56, 0x34, 0x12]; // Global Usage Page (4B LE: 0x12345678)
    let it4 = HidItemIter.init(short4[0..]);
    const item4 = it4.next() ?? unreachable;
    expect_eq(item4.isGlobal(), true);
    expect_eq(item4.tag, GLOBAL_USAGE_PAGE);
    expect_eq(item4.u32Value(), 0x12345678 as u32);
});

test("5. Reserved item_type == 3, unbalanced collections, bad Push/Pop & truncation rejection", () => {
    // Empty descriptor
    expect_eq(summarizeHidDescriptor("") == null, true);

    // Reserved short item type == 3 (0x0C: type bits = 0b11, not 0xFE)
    const bad_type: [2]u8 = [0x0C, 0x00];
    let it1 = HidItemIter.init(bad_type[0..]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.isMalformed(), true);
    expect_eq(summarizeHidDescriptor(bad_type[0..]) == null, true);

    // Unclosed Collection
    const unclosed: [4]u8 = [0x05, 0x01, 0xA1, 0x01];
    expect_eq(summarizeHidDescriptor(unclosed[0..]) == null, true);

    // Extra End Collection
    const extra_end: [3]u8 = [0x05, 0x01, 0xC0];
    expect_eq(summarizeHidDescriptor(extra_end[0..]) == null, true);

    // Pop on empty stack
    const bad_pop: [1]u8 = [0xB4];
    expect_eq(summarizeHidDescriptor(bad_pop[0..]) == null, true);

    // Truncated Short Item (claims 4 bytes, only 2 present)
    const trunc_short: [3]u8 = [0x07, 0x01, 0x02];
    expect_eq(summarizeHidDescriptor(trunc_short[0..]) == null, true);

    // Truncated Long Item (claims 4 bytes, only 1 present)
    const trunc_long: [4]u8 = [0xFE, 0x04, 0xF1, 0xAA];
    let it2 = HidItemIter.init(trunc_long[0..]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.isMalformed(), true);
});
