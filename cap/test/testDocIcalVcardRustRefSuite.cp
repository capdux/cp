// RFC 5545 (iCalendar) & RFC 6350 (vCard 4.0) (MIT OR Apache-2.0) Reference-Hardened Test Suite
// Covers:
//   - RFC 5545 line unfolding (CRLF+SP/HTAB), text escaping (\n, \,, \;, \\), nested VEVENT/VTODO/VALARM, round-trip
//   - RFC 6350 line unfolding, VERSION 3.0/4.0, quoted TYPE params, group prefixes, N/ADR escaping, round-trip
//   - Strict zero-heap validators (validateIcalStrict, validateVcardStrict) + unclosed/mismatched/control-byte/bare-CR/injection/depth rejection
//   - 16-lane @Vector(16, u8) SIMD vs scalar 0-discrepancy parity across lengths 0..129
//   - 1,800-iteration deterministic PRNG fuzzing (900 iCalendar + 900 vCard)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ICAL_MAX_COMPONENT_DEPTH,
    scalarCalFindLineDelim,
    simdCalFindLineDelim,
    scalarCalHasControlByte,
    simdCalHasControlByte,
    validateIcalStrict,
    IcalCalendar,
} from "../../lib/doc/ical.cp";
import {
    validateVcardStrict,
    VCardVersion,
    TypedValue,
    VCardContact,
    VCardCodec,
} from "../../lib/doc/vcard.cp";

fn xorshift64(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn rejectIcalStrict(src: []const u8): bool {
    const ok = validateIcalStrict(src) catch return true;
    return !ok;
}

fn rejectIcalParse(alloc: std.mem.Allocator, src: []const u8): bool {
    let cal = IcalCalendar.parse(alloc, src) catch return true;
    cal.deinit();
    return false;
}

fn rejectVcardStrict(src: string): bool {
    const ok = validateVcardStrict(src) catch return true;
    return !ok;
}

fn rejectVcardParse(alloc: std.mem.Allocator, src: string): bool {
    const list = VCardCodec.parseAll(alloc, src) catch return true;
    VCardCodec.freeContacts(alloc, list);
    return false;
}

fn runTest01IcalUnfoldEscapeNestedRoundtrip(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    const ics =
        "BEGIN:VCALENDAR\r\n" ++
        "VERSION:2.0\r\n" ++
        "PRODID:-//CP//Wave38//EN\r\n" ++
        "BEGIN:VEVENT\r\n" ++
        "UID:ev-001@example.com\r\n" ++
        "DTSTART:20261124T090000Z\r\n" ++
        "DTEND:20261124T103000Z\r\n" ++
        "SUMMARY;LANGUAGE=\"en-US;fmt:1\":Quarterly\\, Review\\; Part 1\r\n" ++
        "  & Follow-up\\\\Notes\r\n" ++
        "DESCRIPTION:Line 1\\nLine 2\r\n" ++
        "\t continued\r\n" ++
        "BEGIN:VALARM\r\n" ++
        "ACTION:DISPLAY\r\n" ++
        "TRIGGER:-PT15M\r\n" ++
        "END:VALARM\r\n" ++
        "END:VEVENT\r\n" ++
        "BEGIN:VTODO\r\n" ++
        "UID:todo-1\r\n" ++
        "SUMMARY:Ship R7\r\n" ++
        "END:VTODO\r\n" ++
        "END:VCALENDAR\r\n";

    if (!(validateIcalStrict(ics) catch return false)) return false;

    let cal = IcalCalendar.parse(alloc, ics) catch return false;
    defer cal.deinit();
    if (cal.events.items.len != 1) return false;
    const ev = cal.events.items[0];
    if (!std.mem.eql(u8, ev.uid, "ev-001@example.com")) return false;
    if (!std.mem.eql(u8, ev.summary, "Quarterly, Review; Part 1 & Follow-up\\Notes")) return false;
    if (!std.mem.eql(u8, ev.description, "Line 1\nLine 2 continued")) return false;

    // Round-trip serialize -> strict validate -> re-parse
    const out = cal.toIcs(alloc) catch return false;
    defer alloc.free(out);
    if (!(validateIcalStrict(out) catch return false)) return false;
    let cal2 = IcalCalendar.parse(alloc, out) catch return false;
    defer cal2.deinit();
    if (cal2.events.items.len != 1) return false;
    return std.mem.eql(u8, cal2.events.items[0].summary, ev.summary) and
        std.mem.eql(u8, cal2.events.items[0].description, ev.description);
}

fn runTest02VcardUnfoldEscapeParamsRoundtrip(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    const vcf =
        "BEGIN:VCARD\r\n" ++
        "VERSION:4.0\r\n" ++
        "UID:urn:uuid:1234\r\n" ++
        "FN:Dr. Jane\\, Doe\\; Jr.\r\n" ++
        "N:Doe;Jane;M.;Dr.;Jr.\r\n" ++
        "item1.TEL;TYPE=\"work,voice\":+1-555-0100\r\n" ++
        "EMAIL;TYPE=home:jane@example.com\r\n" ++
        "ADR;TYPE=work:;;123 Main\\, St;Metropolis;NY;10001;USA\r\n" ++
        "NOTE:First line\\nSecond \r\n" ++
        " line\\\\end\r\n" ++
        "END:VCARD\r\n";

    if (!(validateVcardStrict(vcf) catch return false)) return false;

    let contact = VCardCodec.parseOne(alloc, vcf) catch return false;
    defer contact.deinit();
    if (contact.version != .V4_0) return false;
    if (!std.mem.eql(u8, contact.fn_name, "Dr. Jane, Doe; Jr.")) return false;
    if (!std.mem.eql(u8, contact.note, "First line\nSecond line\\end")) return false;
    if (contact.phones.len != 1 or contact.phones[0].types.len != 2) return false;
    if (contact.addresses.len != 1 or !std.mem.eql(u8, contact.addresses[0].street, "123 Main, St")) return false;

    // Round-trip generate with 75-octet folding -> validate -> re-parse
    const arr: [1]VCardContact = [contact];
    const generated = VCardCodec.toVcf(alloc, arr[0..1]) catch return false;
    defer alloc.free(generated);
    if (!(validateVcardStrict(generated) catch return false)) return false;
    let c2 = VCardCodec.parseOne(alloc, generated) catch return false;
    defer c2.deinit();
    return std.mem.eql(u8, c2.fn_name, contact.fn_name) and
        std.mem.eql(u8, c2.note, contact.note);
}

fn runTest03StrictSecurityRejectionGuards(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;

    // iCalendar rejections: empty, unclosed, mismatched, bare CR, NUL/control byte, unclosed quote, depth overflow
    if (!rejectIcalStrict("")) return false;
    if (!rejectIcalStrict("BEGIN:VCALENDAR\r\nVERSION:2.0\r\n")) return false;
    if (!rejectIcalStrict("BEGIN:VCALENDAR\r\nBEGIN:VEVENT\r\nEND:VTODO\r\nEND:VCALENDAR\r\n")) return false;
    if (!rejectIcalParse(alloc, "BEGIN:VCALENDAR\r\nBEGIN:VEVENT\r\nEND:VTODO\r\nEND:VCALENDAR\r\n")) return false;
    if (!rejectIcalStrict("BEGIN:VCALENDAR\rVERSION:2.0\rEND:VCALENDAR\r")) return false;
    if (!rejectIcalStrict("BEGIN:VCALENDAR\r\nSUMMARY:hi\x00there\r\nEND:VCALENDAR\r\n")) return false;
    if (!rejectIcalStrict("BEGIN:VCALENDAR\r\nSUMMARY;PARAM=\"unclosed:val\r\nEND:VCALENDAR\r\n")) return false;
    if (!rejectIcalParse(alloc, "BEGIN:VCALENDAR\r\nSUMMARY;PARAM=\"unclosed:val\r\nEND:VCALENDAR\r\n")) return false;

    // Depth overflow > ICAL_MAX_COMPONENT_DEPTH (16)
    let deep: std.ArrayList(u8) = .empty;
    defer deep.deinit(alloc);
    deep.appendSlice(alloc, "BEGIN:VCALENDAR\r\n") catch return false;
    let d: usize = 0;
    while (d < ICAL_MAX_COMPONENT_DEPTH + 1) : (d += 1) {
        deep.appendSlice(alloc, "BEGIN:VTODO\r\n") catch return false;
    }
    d = 0;
    while (d < ICAL_MAX_COMPONENT_DEPTH + 1) : (d += 1) {
        deep.appendSlice(alloc, "END:VTODO\r\n") catch return false;
    }
    deep.appendSlice(alloc, "END:VCALENDAR\r\n") catch return false;
    if (!rejectIcalStrict(deep.items)) return false;
    if (!rejectIcalParse(alloc, deep.items)) return false;

    // vCard rejections: empty, unclosed, unsupported VERSION 2.1, missing FN, bare CR, control byte, unclosed quote, CRLF injection in TYPE
    if (!rejectVcardStrict("")) return false;
    if (!rejectVcardStrict("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Alice\r\n")) return false;
    if (!rejectVcardStrict("BEGIN:VCARD\r\nVERSION:2.1\r\nFN:Alice\r\nEND:VCARD\r\n")) return false;
    if (!rejectVcardParse(alloc, "BEGIN:VCARD\r\nVERSION:2.1\r\nFN:Alice\r\nEND:VCARD\r\n")) return false;
    if (!rejectVcardStrict("BEGIN:VCARD\r\nVERSION:4.0\r\nEND:VCARD\r\n")) return false;
    if (!rejectVcardStrict("BEGIN:VCARD\rVERSION:4.0\rFN:Alice\rEND:VCARD\r")) return false;
    if (!rejectVcardStrict("BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Ali\x07ce\r\nEND:VCARD\r\n")) return false;
    if (!rejectVcardStrict("BEGIN:VCARD\r\nVERSION:4.0\r\nFN;TYPE=\"bad:Alice\r\nEND:VCARD\r\n")) return false;

    // CRLF injection attempt in TypedValue.types during toVcf generation
    let bad_types: [1]string = ["work\r\nFN:Injected"];
    let bad_phones: [1]TypedValue = [TypedValue{ types: bad_types[0..1], value: "123" }];
    let bad_c = VCardContact{ version: .V4_0, fn_name: "Safe", phones: bad_phones[0..1] };
    const bad_arr: [1]VCardContact = [bad_c];
    let inj_rejected: bool = false;
    const maybe_vcf = VCardCodec.toVcf(alloc, bad_arr[0..1]) catch {
        inj_rejected = true;
        return true;
    };
    alloc.free(maybe_vcf);
    return inj_rejected;
}

fn runTest04Simd16ZeroDiscrepancy(): bool {
    const lens: [14]usize = [0, 1, 7, 15, 16, 17, 31, 32, 33, 48, 63, 64, 95, 129];
    let buf: [129]u8 = [0; 129];
    let rng: u64 = 0x51AD_CA10_0000_0016;

    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        let trial: usize = 0;
        while (trial < 12) : (trial += 1) {
            let i: usize = 0;
            while (i < n) : (i += 1) {
                const r = xorshift64(*rng) % 100;
                if (r < 3) {
                    buf[i] = '\r';
                } else if (r < 6) {
                    buf[i] = '\n';
                } else if (r < 9) {
                    buf[i] = ':';
                } else if (r < 12) {
                    buf[i] = ';';
                } else if (r == 13 and (trial % 3) == 0) {
                    buf[i] = @intCast(xorshift64(*rng) % 8); // control byte 0x00..0x07
                } else if (r == 14 and (trial % 4) == 0) {
                    buf[i] = 0x7F; // DEL
                } else {
                    buf[i] = @as(u8, 'A') + @as(u8, @intCast(xorshift64(*rng) % 26));
                }
            }
            const slice = buf[0..n];
            if (simdCalHasControlByte(slice) != scalarCalHasControlByte(slice)) return false;

            let st: usize = 0;
            while (st <= n) : (st += 7) {
                if (simdCalFindLineDelim(slice, st) != scalarCalFindLineDelim(slice, st)) return false;
            }
        }
    }
    return true;
}

fn runTest05IcalFuzz900(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let rng: u64 = 0x1CA1_F022_0000_0900;
    let buf: std.ArrayList(u8) = .empty;
    defer buf.deinit(alloc);

    let iter: usize = 0;
    while (iter < 900) : (iter += 1) {
        buf.clearRetainingCapacity();
        const mode = xorshift64(*rng) % 6;

        if (mode < 3) {
            // Valid folded/escaped iCalendar with 1..3 events
            buf.appendSlice(alloc, "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//Fuzz//EN\r\n") catch return false;
            const n_ev_off: usize = @intCast(xorshift64(*rng) % 3);
            const n_ev: usize = n_ev_off + 1;
            let e: usize = 0;
            while (e < n_ev) : (e += 1) {
                buf.appendSlice(alloc, "BEGIN:VEVENT\r\nUID:id-") catch return false;
                buf.append(alloc, @as(u8, '0') + @as(u8, @intCast(e))) catch return false;
                buf.appendSlice(alloc, "\r\nDTSTART:20261124T120000Z\r\nSUMMARY:PartA\\,") catch return false;
                if ((iter % 2) == 0) {
                    buf.appendSlice(alloc, "\r\n PartB\\;\\\\End\r\n") catch return false;
                } else {
                    buf.appendSlice(alloc, "\r\n\tPartB\\nEnd\r\n") catch return false;
                }
                buf.appendSlice(alloc, "END:VEVENT\r\n") catch return false;
            }
            buf.appendSlice(alloc, "END:VCALENDAR\r\n") catch return false;

            if (!(validateIcalStrict(buf.items) catch return false)) return false;
            let cal = IcalCalendar.parse(alloc, buf.items) catch return false;
            defer cal.deinit();
            if (cal.events.items.len != n_ev) return false;
        } else if (mode == 3) {
            // Control byte injection -> must be rejected by both strict & parse
            buf.appendSlice(alloc, "BEGIN:VCALENDAR\r\nSUMMARY:bad") catch return false;
            buf.append(alloc, @as(u8, @intCast(1 + (xorshift64(*rng) % 7)))) catch return false;
            buf.appendSlice(alloc, "\r\nEND:VCALENDAR\r\n") catch return false;
            if (!rejectIcalStrict(buf.items)) return false;
            if (!rejectIcalParse(alloc, buf.items)) return false;
        } else if (mode == 4) {
            // Unclosed or mismatched component -> must be rejected
            buf.appendSlice(alloc, "BEGIN:VCALENDAR\r\nBEGIN:VEVENT\r\nDTSTART:20261124\r\nEND:VCALENDAR\r\n") catch return false;
            if (!rejectIcalStrict(buf.items)) return false;
            if (!rejectIcalParse(alloc, buf.items)) return false;
        } else {
            // Random garbage bytes -> must never crash; SIMD & scalar must agree
            const glen_off: usize = @intCast(xorshift64(*rng) % 60);
            const glen: usize = glen_off + 4;
            let g: usize = 0;
            while (g < glen) : (g += 1) {
                buf.append(alloc, @as(u8, @intCast(xorshift64(*rng) & 0x7F))) catch return false;
            }
            if (simdCalHasControlByte(buf.items) != scalarCalHasControlByte(buf.items)) return false;
            if (simdCalFindLineDelim(buf.items, 0) != scalarCalFindLineDelim(buf.items, 0)) return false;
            * = validateIcalStrict(buf.items) catch false;
        }
    }
    return true;
}

fn runTest06VcardFuzz900(): bool {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    let rng: u64 = 0x5CA9_F022_0000_0900;
    let buf: std.ArrayList(u8) = .empty;
    defer buf.deinit(alloc);

    let iter: usize = 0;
    while (iter < 900) : (iter += 1) {
        buf.clearRetainingCapacity();
        const mode = xorshift64(*rng) % 6;

        if (mode < 3) {
            // Valid vCard 3.0 or 4.0 with folding & escaping
            buf.appendSlice(alloc, "BEGIN:VCARD\r\nVERSION:") catch return false;
            if ((iter % 2) == 0) {
                buf.appendSlice(alloc, "4.0\r\n") catch return false;
            } else {
                buf.appendSlice(alloc, "3.0\r\n") catch return false;
            }
            buf.appendSlice(alloc, "FN:User\\, ") catch return false;
            buf.append(alloc, @as(u8, 'A') + @as(u8, @intCast(iter % 26))) catch return false;
            buf.appendSlice(alloc, "\r\n Tail\r\nTEL;TYPE=\"work,cell\":+1555000") catch return false;
            buf.append(alloc, @as(u8, '0') + @as(u8, @intCast(iter % 10))) catch return false;
            buf.appendSlice(alloc, "\r\nEND:VCARD\r\n") catch return false;

            if (!(validateVcardStrict(buf.items) catch return false)) return false;
            let c = VCardCodec.parseOne(alloc, buf.items) catch return false;
            defer c.deinit();
            if (c.phones.len != 1) return false;
        } else if (mode == 3) {
            // Control byte injection -> must be rejected
            buf.appendSlice(alloc, "BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Bad") catch return false;
            buf.append(alloc, @as(u8, @intCast(xorshift64(*rng) % 8))) catch return false;
            buf.appendSlice(alloc, "\r\nEND:VCARD\r\n") catch return false;
            if (!rejectVcardStrict(buf.items)) return false;
            if (!rejectVcardParse(alloc, buf.items)) return false;
        } else if (mode == 4) {
            // Unsupported version or missing FN -> must be rejected
            if ((iter % 2) == 0) {
                buf.appendSlice(alloc, "BEGIN:VCARD\r\nVERSION:9.9\r\nFN:X\r\nEND:VCARD\r\n") catch return false;
            } else {
                buf.appendSlice(alloc, "BEGIN:VCARD\r\nVERSION:4.0\r\nORG:NoFn\r\nEND:VCARD\r\n") catch return false;
            }
            if (!rejectVcardStrict(buf.items)) return false;
            if (!rejectVcardParse(alloc, buf.items)) return false;
        } else {
            // Random garbage -> never crash, SIMD parity holds
            const glen_off: usize = @intCast(xorshift64(*rng) % 60);
            const glen: usize = glen_off + 4;
            let g: usize = 0;
            while (g < glen) : (g += 1) {
                buf.append(alloc, @as(u8, @intCast(xorshift64(*rng) & 0x7F))) catch return false;
            }
            if (simdCalHasControlByte(buf.items) != scalarCalHasControlByte(buf.items)) return false;
            * = validateVcardStrict(buf.items) catch false;
        }
    }
    return true;
}

fn main() {
    test("doc_ref_01_ical_unfold_escape_nested_roundtrip", () => {
        expect_eq(runTest01IcalUnfoldEscapeNestedRoundtrip(), true);
    });

    test("doc_ref_02_vcard_unfold_escape_params_roundtrip", () => {
        expect_eq(runTest02VcardUnfoldEscapeParamsRoundtrip(), true);
    });

    test("doc_ref_03_strict_security_rejection_guards", () => {
        expect_eq(runTest03StrictSecurityRejectionGuards(), true);
    });

    test("doc_ref_04_simd16_zero_discrepancy_parity", () => {
        expect_eq(runTest04Simd16ZeroDiscrepancy(), true);
    });

    test("doc_ref_05_ical_prng_fuzz_900_iters", () => {
        expect_eq(runTest05IcalFuzz900(), true);
    });

    test("doc_ref_06_vcard_prng_fuzz_900_iters", () => {
        expect_eq(runTest06VcardFuzz900(), true);
    });
}
