// cap/test/test_blob.cp
// E2E Verification for cap:blob and cap:file (Binary Large Objects)
// Absorbed from Bun: bun/src/runtime/webcore/Blob.cp & bun/src/runtime/webcore/blob/Store.cp

import { test, expect_eq } from "cap:test";
import { Blob, File, BlobStore, BlobKind } from "cap:blob";

fn main() {
    test("1. Blob fromBytes & init properties", () => {
        let b = Blob.init("Hello, Antigravity Blob!", "text/plain");
        expect_eq(b.size, 24 as usize);
        expect_eq(b.type, "text/plain");
        expect_eq(b.text(), "Hello, Antigravity Blob!");
        expect_eq(b.bytes().len, 24 as usize);
        expect_eq(b.offset, 0 as usize);
        expect_eq(b.store.ref_count, 1 as usize);
        b.release();
    });

    test("2. Blob fromString & default mime type", () => {
        let b = Blob.fromString("Binary Data", "");
        expect_eq(b.size, 11 as usize);
        expect_eq(b.type, "text/plain;charset=utf-8");
        expect_eq(b.text(), "Binary Data");
        b.release();
    });

    test("3. Blob zero-copy slice and reference counting", () => {
        let orig = Blob.fromBytes("0123456789ABCDEF", "application/octet-stream");
        expect_eq(orig.size, 16 as usize);

        // Slice middle: "456789"
        let sub = orig.slice(4, 10, "text/custom");
        expect_eq(sub.size, 6 as usize);
        expect_eq(sub.offset, 4 as usize);
        expect_eq(sub.type, "text/custom");
        expect_eq(sub.text(), "456789");
        expect_eq(orig.store.ref_count, 2 as usize);

        // Slice sub again: "67"
        let sub2 = sub.slice(2, 4, "");
        expect_eq(sub2.size, 2 as usize);
        expect_eq(sub2.offset, 6 as usize);
        expect_eq(sub2.text(), "67");
        expect_eq(sub2.type, "text/custom");
        expect_eq(orig.store.ref_count, 3 as usize);

        sub2.release();
        sub.release();
        orig.release();
    });

    test("4. Blob arrayBuffer heap allocation", () => {
        let b = Blob.fromBytes("DeepMind", "text/plain");
        let ab = b.arrayBuffer();
        expect_eq(ab.len, 8 as usize);
        expect_eq(ab[0], 68 as u8); // 'D'
        expect_eq(ab[7], 100 as u8); // 'd'
        b.release();
    });

    test("5. File abstraction, metadata & slice", () => {
        let f = File.init("File Payload Content", "data.txt", "text/plain", 1700000000 as i64);
        expect_eq(f.name, "data.txt");
        expect_eq(f.lastModified, 1700000000 as i64);
        expect_eq(f.size, 20 as usize);
        expect_eq(f.type, "text/plain");
        expect_eq(f.text(), "File Payload Content");

        let f_slice = f.slice(5, 12, "application/sub");
        expect_eq(f_slice.name, "data.txt");
        expect_eq(f_slice.size, 7 as usize);
        expect_eq(f_slice.text(), "Payload");
        expect_eq(f_slice.type, "application/sub");

        f_slice.release();
        f.release();
    });

    test("6. File writeTo/save disk roundtrip & File.fromPath", () => {
        let sample = "Persistent Large Object Data Storage Test";
        let f = File.init(sample, "out_test.txt", "text/plain", 123456 as i64);
        let dest = "temp/test_blob_file_io.txt";
        let saved = f.writeTo(dest);
        expect_eq(saved, true);

        // Read back from path
        let loaded = File.fromPath(dest, "");
        expect_eq(loaded != null, true);
        let loaded_file = loaded.?;
        expect_eq(loaded_file.name, "test_blob_file_io.txt");
        expect_eq(loaded_file.text(), sample);
        expect_eq(loaded_file.size, 41 as usize);
        expect_eq(loaded_file.type, "text/plain");

        loaded_file.release();
        f.release();
    });
}
