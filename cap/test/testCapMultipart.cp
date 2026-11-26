import { test, expect_eq } from "cap:test";
import {
    MultipartPart,
    MultipartReader,
    MultipartWriter,
    extractBoundary,
    formatContentTypeHeader
} from "../../packages/@cap/multipart/src/index.cp";

fn mpTestStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn mpTestBufEq(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn mpTestDummyPart(): MultipartPart {
    return MultipartPart {
        name: "",
        filename: "",
        content_type: "",
        data: "",
        is_file: false,
    };
}

fn mpTestZeroBuf512(): [512]u8 {
    let b: [512]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ];
    return b;
}

fn main() {
    test("1. @cap/multipart — extractBoundary via @cap/mediatype & formatContentTypeHeader", () => {
        let b1 = extractBoundary("multipart/form-data; boundary=----CapBoundary7MA4");
        expect_eq(b1 == null, false);
        expect_eq(mpTestStrEq(b1 ?? "", "----CapBoundary7MA4"), true);

        let b2 = extractBoundary("MULTIPART/MIXED; BOUNDARY=\"quoted-bnd-42\"");
        expect_eq(b2 == null, false);
        expect_eq(mpTestStrEq(b2 ?? "", "quoted-bnd-42"), true);

        expect_eq(extractBoundary("application/json") == null, true);
        expect_eq(extractBoundary("multipart/form-data") == null, true);
        expect_eq(extractBoundary("multipart/form-data; boundary=") == null, true);

        let hbuf: [512]u8 = mpTestZeroBuf512();
        let hlen: usize = formatContentTypeHeader("abc123", hbuf[0..512]);
        expect_eq(mpTestBufEq(hbuf[0..512], hlen, "multipart/form-data; boundary=abc123"), true);
    });

    test("2. @cap/multipart — MultipartReader decodes text fields & file upload", () => {
        let body: string = "--bnd\r\nContent-Disposition: form-data; name=\"user\"\r\n\r\nalice\r\n--bnd\r\ncontent-disposition: form-data; name=\"avatar\"; filename=\"pic.png\"\r\ncontent-type: image/png\r\n\r\nPNGDATA\r\n--bnd--\r\n";
        let reader: MultipartReader = MultipartReader.new("bnd", body);
        expect_eq(reader.countParts(), 2);

        let p1 = reader.nextPart();
        expect_eq(p1 == null, false);
        let part1: MultipartPart = p1 ?? mpTestDummyPart();
        expect_eq(part1.is_file, false);
        expect_eq(mpTestStrEq(part1.name, "user"), true);
        expect_eq(mpTestStrEq(part1.data, "alice"), true);

        let p2 = reader.nextPart();
        expect_eq(p2 == null, false);
        let part2: MultipartPart = p2 ?? mpTestDummyPart();
        expect_eq(part2.is_file, true);
        expect_eq(mpTestStrEq(part2.name, "avatar"), true);
        expect_eq(mpTestStrEq(part2.filename, "pic.png"), true);
        expect_eq(mpTestStrEq(part2.content_type, "image/png"), true);
        expect_eq(mpTestStrEq(part2.data, "PNGDATA"), true);

        expect_eq(reader.nextPart() == null, true);
    });

    test("3. @cap/multipart — MultipartWriter + MultipartReader full round-trip", () => {
        let buf: [512]u8 = mpTestZeroBuf512();
        let writer: MultipartWriter = MultipartWriter.new("CapFormBoundary99");
        let pos: usize = 0;
        pos = writer.writeField("title", "hello world", buf[0..512], pos);
        pos = writer.writeFile("doc", "notes.txt", "text/plain", "line1\r\nline2", buf[0..512], pos);
        pos = writer.finish(buf[0..512], pos);

        let payload: string = buf[0..pos];
        let reader: MultipartReader = MultipartReader.new("CapFormBoundary99", payload);
        expect_eq(reader.countParts(), 2);

        let part1: MultipartPart = reader.nextPart() ?? mpTestDummyPart();
        expect_eq(mpTestStrEq(part1.name, "title"), true);
        expect_eq(mpTestStrEq(part1.data, "hello world"), true);
        expect_eq(part1.is_file, false);

        let part2: MultipartPart = reader.nextPart() ?? mpTestDummyPart();
        expect_eq(mpTestStrEq(part2.name, "doc"), true);
        expect_eq(mpTestStrEq(part2.filename, "notes.txt"), true);
        expect_eq(mpTestStrEq(part2.content_type, "text/plain"), true);
        expect_eq(mpTestStrEq(part2.data, "line1\r\nline2"), true);
        expect_eq(part2.is_file, true);
    });

    test("4. @cap/multipart — preamble skipping & LF-only line endings", () => {
        let body: string = "Ignored preamble text\n--sep\nContent-Disposition: form-data; name=\"k\"\n\nv1\n--sep--\n";
        let reader: MultipartReader = MultipartReader.new("sep", body);
        expect_eq(reader.countParts(), 1);
        let p: MultipartPart = reader.nextPart() ?? mpTestDummyPart();
        expect_eq(mpTestStrEq(p.name, "k"), true);
        expect_eq(mpTestStrEq(p.data, "v1"), true);
        expect_eq(reader.nextPart() == null, true);
    });

    test("5. @cap/multipart — default content-type for writeFile & empty field value", () => {
        let buf: [512]u8 = mpTestZeroBuf512();
        let writer: MultipartWriter = MultipartWriter.new("B1");
        let pos: usize = 0;
        pos = writer.writeField("empty", "", buf[0..512], pos);
        pos = writer.writeFile("bin", "a.bin", "", "RAW", buf[0..512], pos);
        pos = writer.finish(buf[0..512], pos);

        let reader: MultipartReader = MultipartReader.new("B1", buf[0..pos]);
        let p1: MultipartPart = reader.nextPart() ?? mpTestDummyPart();
        expect_eq(mpTestStrEq(p1.name, "empty"), true);
        expect_eq(p1.data.len, 0);

        let p2: MultipartPart = reader.nextPart() ?? mpTestDummyPart();
        expect_eq(mpTestStrEq(p2.content_type, "application/octet-stream"), true);
        expect_eq(mpTestStrEq(p2.data, "RAW"), true);
    });

    test("6. @cap/multipart — graceful handling of empty / truncated / missing boundary", () => {
        let r1: MultipartReader = MultipartReader.new("bnd", "");
        expect_eq(r1.countParts(), 0);
        expect_eq(r1.nextPart() == null, true);

        let r2: MultipartReader = MultipartReader.new("", "--bnd\r\n\r\nhi\r\n--bnd--");
        expect_eq(r2.countParts(), 0);

        let r3: MultipartReader = MultipartReader.new("bnd", "--bnd\r\nContent-Disposition: form-data; name=\"x\"\r\n\r\ntruncated");
        expect_eq(r3.nextPart() == null, true);
    });
}
