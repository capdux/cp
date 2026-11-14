// cap/test/test_col_pack_md_html.cp
// E2E Regression Suite for deepened modules in cap:col, cap:pack, cap:md, and cap:html

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { ObjectPool } from "cap:col";
import {
    ZIP_LOCAL_HEADER_SIG,
    ZIP_CENTRAL_DIR_SIG,
    ZIP_EOCD_SIG,
    ZIP_CENTRAL_DIR_SIZE,
    ZIP_EOCD_SIZE,
    ZipLocalHeader,
    ZipCentralHeader,
    ZipEocd,
    ZipReader
} from "cap:pack";
import { MdTableAlign, MdTable } from "cap:md";
import { HtmlEscape } from "cap:html";

fn main() {
    test("1. cap:col — ObjectPool<T, CAP> zero-heap free-list lifecycle", () => {
        let pool = ObjectPool<i64, 4>.init();
        expect_eq(pool.capacity(), 4 as usize);
        expect_eq(pool.usedCount(), 0 as usize);
        expect_eq(pool.availableCount(), 4 as usize);
        expect_eq(pool.isEmpty(), true);

        const s0 = pool.acquire(100).?;
        const s1 = pool.acquire(200).?;
        const s2 = pool.acquire(300).?;
        expect_eq(s0, 0 as usize);
        expect_eq(s1, 1 as usize);
        expect_eq(s2, 2 as usize);
        expect_eq(pool.usedCount(), 3 as usize);
        expect_eq(pool.availableCount(), 1 as usize);
        expect_eq(pool.get(s1).?, 200 as i64);

        // Release slot 1 and verify O(1) free-list head reuse
        expect_eq(pool.releaseSlot(s1), true);
        expect_eq(pool.releaseSlot(s1), false);
        expect_eq(pool.get(s1) == null, true);
        expect_eq(pool.usedCount(), 2 as usize);
        expect_eq(pool.availableCount(), 2 as usize);

        const s1_reused = pool.acquire(250).?;
        expect_eq(s1_reused, s1);
        expect_eq(pool.get(s1_reused).?, 250 as i64);

        const s3 = pool.acquire(400).?;
        expect_eq(s3, 3 as usize);
        expect_eq(pool.isFull(), true);
        expect_eq(pool.acquire(500) == null, true);

        pool.reset();
        expect_eq(pool.usedCount(), 0 as usize);
        expect_eq(pool.availableCount(), 4 as usize);
        expect_eq(pool.isEmpty(), true);
    });

    test("2. cap:pack — ZipLocalHeader, ZipCentralHeader, ZipEocd & zero-copy ZipReader", () => {
        let zip_buf: [512]u8 = [0; 512];
        let offset: usize = 0;

        // Encode 2 stored local file entries
        const n1 = ZipLocalHeader.encodeEntry(zip_buf[offset..], "hello.txt", "Hello ZIP!");
        expect_eq(n1 > 30, true);
        offset += n1;

        const n2 = ZipLocalHeader.encodeEntry(zip_buf[offset..], "sub/data.bin", "ABCD1234");
        expect_eq(n2 > 30, true);
        offset += n2;

        // Append Central Directory Header (0x02014b50)
        const cd_offset = offset;
        let cd_hdr = ZipCentralHeader {
            version_made_by: 20,
            version_needed: 20,
            compression_method: 0,
            compressed_size: 10,
            uncompressed_size: 10,
            name_len: 9,
            local_header_offset: 0,
        };
        const cd_written = cd_hdr.write(zip_buf[offset..]);
        expect_eq(cd_written, ZIP_CENTRAL_DIR_SIZE);
        const cd_parsed = ZipCentralHeader.parse(zip_buf[offset..]).?;
        expect_eq(cd_parsed.name_len, 9 as u16);
        offset += cd_written;

        // Append End of Central Directory (0x06054b50)
        let eocd = ZipEocd {
            total_entries: 2,
            entries_on_disk: 2,
            central_dir_size: cd_written as u32,
            central_dir_offset: cd_offset as u32,
        };
        const eocd_written = eocd.write(zip_buf[offset..]);
        expect_eq(eocd_written, ZIP_EOCD_SIZE);
        offset += eocd_written;

        // Locate EOCD backward from buffer end
        const found_eocd = ZipEocd.findInBuffer(zip_buf[0..offset]).?;
        expect_eq(found_eocd.total_entries, 2 as u16);
        expect_eq(found_eocd.central_dir_offset, cd_offset as u32);

        // Scan entries zero-copy with ZipReader
        let reader = ZipReader.init(zip_buf[0..offset]);
        const entry1 = reader.next().?;
        expect_eq(std.mem.eql(u8, entry1.name, "hello.txt"), true);
        expect_eq(std.mem.eql(u8, entry1.data, "Hello ZIP!"), true);
        expect_eq(entry1.isStored(), true);
        expect_eq(entry1.verifyCrc(), true);

        const entry2 = reader.next().?;
        expect_eq(std.mem.eql(u8, entry2.name, "sub/data.bin"), true);
        expect_eq(std.mem.eql(u8, entry2.data, "ABCD1234"), true);
        expect_eq(entry2.verifyCrc(), true);

        // Next encounters Central Directory signature and terminates local entry iteration
        expect_eq(reader.next() == null, true);
    });

    test("3. cap:md — MdTableAlign & MdTable GFM HTML <table> renderer", () => {
        expect_eq(MdTable.parseAlignSpec(":---").? == MdTableAlign.Left, true);
        expect_eq(MdTable.parseAlignSpec(":---:").? == MdTableAlign.Center, true);
        expect_eq(MdTable.parseAlignSpec("---:").? == MdTableAlign.Right, true);
        expect_eq(MdTable.parseAlignSpec("---").? == MdTableAlign.None, true);
        expect_eq(MdTable.parseAlignSpec("::") == null, true);
        expect_eq(MdTable.parseAlignSpec("abc") == null, true);

        const md_src = "| Name | Score | Rank |\n| :--- | :---: | ---: |\n| Alice | 98 | 1 |\n| Bob | 91 | 2 |\n";
        let html_buf: [512]u8 = [0; 512];
        const rendered = MdTable.renderHtml(md_src, html_buf[0..512]);
        const expected = "<table><thead><tr><th align=\"left\">Name</th><th align=\"center\">Score</th><th align=\"right\">Rank</th></tr></thead><tbody><tr><td align=\"left\">Alice</td><td align=\"center\">98</td><td align=\"right\">1</td></tr><tr><td align=\"left\">Bob</td><td align=\"center\">91</td><td align=\"right\">2</td></tr></tbody></table>";
        expect_eq(std.mem.eql(u8, rendered, expected), true);
    });

    test("4. cap:html — HtmlEscape fast-path entity escaping & unescaping", () => {
        expect_eq(HtmlEscape.needsEscape("Plain ASCII text 123"), false);
        expect_eq(HtmlEscape.needsEscape("<script>&\"'"), true);

        let esc_buf: [256]u8 = [0; 256];
        const escaped = HtmlEscape.escapeToBuf("<div class=\"box\">Tom & 'Jerry'</div>", esc_buf[0..256]);
        expect_eq(std.mem.eql(u8, escaped, "&lt;div class=&quot;box&quot;&gt;Tom &amp; &#39;Jerry&#39;&lt;/div&gt;"), true);

        let unesc_buf: [256]u8 = [0; 256];
        const unescaped = HtmlEscape.unescapeToBuf(escaped, unesc_buf[0..256]);
        expect_eq(std.mem.eql(u8, unescaped, "<div class=\"box\">Tom & 'Jerry'</div>"), true);
    });
}
