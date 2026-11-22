import { test, expect } from "cap:test";
import {
    JSON_LINE_DEFAULT_COMPACTION_THRESHOLD,
    JsonLineBufferError,
    JsonLineNext,
    JsonLineBuffer,
    stripUtf8Bom,
} from "cap:json";

test("JsonLineBuffer basic multi-line LF and CRLF consumption", () => {
    let storage: [128]u8 = [0; 128];
    let buf = JsonLineBuffer.init(storage[0..]);

    expect(JSON_LINE_DEFAULT_COMPACTION_THRESHOLD).toBe(16 * 1024 * 1024);
    expect(buf.isEmpty()).toBe(true);
    expect(buf.hasCompleteLine()).toBe(false);
    expect(buf.capacity()).toBe(128);
    expect(buf.unusedCapacity()).toBe(128);

    const n = buf.append("{\"id\":1}\n{\"id\":2}\r\n{\"id\":3}")?;
    expect(n).toBe(28);
    expect(buf.isEmpty()).toBe(false);
    expect(buf.hasCompleteLine()).toBe(true);

    if (line1 := buf.next()) {
        expect(line1.data).toBe("{\"id\":1}\n");
        expect(line1.payload()).toBe("{\"id\":1}");
        expect(line1.newline_pos).toBe(8);
        expect(line1.consumedBytes()).toBe(9);
        buf.consume(line1.consumedBytes());
    } else {
        expect(false).toBe(true);
    }

    if (line2 := buf.next()) {
        expect(line2.data).toBe("{\"id\":2}\r\n");
        expect(line2.payload()).toBe("{\"id\":2}");
        expect(line2.newline_pos).toBe(9);
        buf.consume(line2.consumedBytes());
    } else {
        expect(false).toBe(true);
    }

    // Trailing incomplete record stays buffered with no complete line
    expect(buf.hasCompleteLine()).toBe(false);
    expect(buf.next() == null).toBe(true);
    expect(buf.activeSlice()).toBe("{\"id\":3}");

    // Complete the third record
    * = buf.append("\n")?;
    if (line3 := buf.next()) {
        expect(line3.payload()).toBe("{\"id\":3}");
        buf.consume(line3.consumedBytes());
    } else {
        expect(false).toBe(true);
    }

    expect(buf.isEmpty()).toBe(true);
    expect(buf.head).toBe(0);
    expect(buf.len).toBe(0);
    expect(buf.scanned_pos).toBe(0);
});

test("JsonLineBuffer strict O(n) single-pass scanning across fragmented chunks", () => {
    let storage: [64]u8 = [0; 64];
    let buf = JsonLineBuffer.init(storage[0..]);

    // Feed a 20-byte JSON line in 5 tiny chunks without newline until the last chunk
    * = buf.append("{\"k\":")?;
    expect(buf.next() == null).toBe(true);
    expect(buf.total_bytes_scanned).toBe(5);

    * = buf.append("\"abcd")?;
    expect(buf.next() == null).toBe(true);
    expect(buf.total_bytes_scanned).toBe(10);

    * = buf.append("efgh\"")?;
    expect(buf.next() == null).toBe(true);
    expect(buf.total_bytes_scanned).toBe(15);

    * = buf.append(",\"v\":")?;
    expect(buf.next() == null).toBe(true);
    expect(buf.total_bytes_scanned).toBe(20);

    * = buf.append("9}\n{\"next\":1}")?;
    // Only stops at '\n' (byte 22 -> 23 bytes scanned total), does NOT scan bytes after '\n' yet
    expect(buf.total_bytes_scanned).toBe(23);

    if (line := buf.next()) {
        expect(line.payload()).toBe("{\"k\":\"abcdefgh\",\"v\":9}");
        buf.consume(line.consumedBytes());
    } else {
        expect(false).toBe(true);
    }

    // After consuming past '\n', remaining 10 bytes "{\"next\":1}" are scanned once
    expect(buf.total_bytes_scanned).toBe(33);
    expect(buf.activeSlice()).toBe("{\"next\":1}");
});

test("JsonLineBuffer lazy head advancement, threshold compaction, and on-demand compaction", () => {
    let storage: [32]u8 = [0; 32];
    let buf = JsonLineBuffer.initWithThreshold(storage[0..], 16);

    * = buf.append("aaaa\nbbbb\ncccc\ndddd")?;
    expect(buf.compaction_count).toBe(0);

    // Consume 5 bytes ("aaaa\n"): head=5 < 16, no compaction
    if (l1 := buf.next()) {
        expect(l1.payload()).toBe("aaaa");
        buf.consume(l1.consumedBytes());
    } else {
        expect(false).toBe(true);
    }
    expect(buf.head).toBe(5);
    expect(buf.compaction_count).toBe(0);

    // Consume 5 bytes ("bbbb\n"): head=10 < 16, still no compaction
    if (l2 := buf.next()) {
        expect(l2.payload()).toBe("bbbb");
        buf.consume(l2.consumedBytes());
    } else {
        expect(false).toBe(true);
    }
    expect(buf.head).toBe(10);
    expect(buf.compaction_count).toBe(0);

    // Now tail unusedCapacity is 32 - 19 = 13, but availableSpace is 32 - 9 = 23.
    // Appending 16 bytes triggers on-demand compaction without overflow!
    const active_after = buf.append("0123456789abcdef")?;
    expect(active_after).toBe(25);
    expect(buf.head).toBe(0);
    expect(buf.compaction_count).toBe(1);

    // Consume "cccc\n" (5 bytes), then append "\n" and consume 21 bytes to trigger threshold compaction
    expect(buf.skipLine()).toBe(true);
    expect(buf.head).toBe(5);

    // Append another line so buffer won't become empty when we cross threshold
    * = buf.append("\nend")?;
    if (l4 := buf.next()) {
        expect(l4.payload()).toBe("dddd0123456789abcdef");
        buf.consume(l4.consumedBytes());
    } else {
        expect(false).toBe(true);
    }
    // head became 5 + 21 = 26 >= 16 while "end" remained -> threshold compaction triggered
    expect(buf.head).toBe(0);
    expect(buf.compaction_count).toBe(2);
    expect(buf.activeSlice()).toBe("end");
});

test("JsonLineBuffer direct unusedCapacitySlice write, notifyWritten, and overflow guards", () => {
    let storage: [16]u8 = [0; 16];
    let buf = JsonLineBuffer.init(storage[0..]);

    // Write directly into unusedCapacitySlice() like a zero-copy socket read
    let dst = buf.unusedCapacitySlice();
    expect(dst.len).toBe(16);
    const msg = "{\"x\":1}\n";
    let i: usize = 0;
    while (i < msg.len) {
        dst[i] = msg[i];
        i += 1;
    }

    const active = buf.notifyWritten(msg.len)?;
    expect(active).toBe(8);
    expect(buf.hasCompleteLine()).toBe(true);

    if (line := buf.next()) {
        expect(line.payload()).toBe("{\"x\":1}");
    } else {
        expect(false).toBe(true);
    }

    // notifyWritten beyond remaining unusedCapacity returns WrittenExceedsCapacity
    let got_written_err = false;
    * = buf.notifyWritten(9) catch |e| {
        if (e == JsonLineBufferError.WrittenExceedsCapacity) {
            got_written_err = true;
        }
        0
    };
    expect(got_written_err).toBe(true);

    // append beyond total availableSpace returns BufferOverflow without corrupting state
    let got_overflow_err = false;
    * = buf.append("123456789") catch |e| {
        if (e == JsonLineBufferError.BufferOverflow) {
            got_overflow_err = true;
        }
        0
    };
    expect(got_overflow_err).toBe(true);
    expect(buf.activeLen()).toBe(8);
});

test("JsonLineBuffer partial consume, empty lines, skipLine, reset, and stripUtf8Bom", () => {
    let storage: [32]u8 = [0; 32];
    let buf = JsonLineBuffer.init(storage[0..]);

    // Partial consume before newline adjusts newline_pos in O(1) without rescanning
    * = buf.append("abcdef\n\n")?;
    expect(buf.newline_pos ?? 999).toBe(6);
    buf.consume(2);
    expect(buf.activeSlice()).toBe("cdef\n\n");
    expect(buf.newline_pos ?? 999).toBe(4);

    if (l1 := buf.next()) {
        expect(l1.payload()).toBe("cdef");
        buf.consume(l1.consumedBytes());
    } else {
        expect(false).toBe(true);
    }

    // Second line is empty "\n"
    if (l2 := buf.next()) {
        expect(l2.data).toBe("\n");
        expect(l2.payload()).toBe("");
        expect(l2.newline_pos).toBe(0);
        buf.consume(l2.consumedBytes());
    } else {
        expect(false).toBe(true);
    }
    expect(buf.isEmpty()).toBe(true);
    expect(buf.skipLine()).toBe(false);

    // Over-consume clamps cleanly to empty
    * = buf.append("tail")?;
    buf.consume(100);
    expect(buf.isEmpty()).toBe(true);

    // reset clears active data
    * = buf.append("abc\n")?;
    buf.reset();
    expect(buf.isEmpty()).toBe(true);
    expect(buf.hasCompleteLine()).toBe(false);

    // stripUtf8Bom handles BOM, non-BOM, and short slices
    const bom_bytes: [9]u8 = [0xEF, 0xBB, 0xBF, '{', '"', 'a', '"', ':', '}'];
    expect(stripUtf8Bom(bom_bytes[0..])).toBe("{\"a\":}");
    expect(stripUtf8Bom("{\"a\":1}")).toBe("{\"a\":1}");
    expect(stripUtf8Bom("ab")).toBe("ab");
    expect(stripUtf8Bom("")).toBe("");
});
