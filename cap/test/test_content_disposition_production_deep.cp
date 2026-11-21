import { test, expect } from "cap:test";
import {
    DispositionType,
    ContentDisposition,
    sanitizeFilename,
    formatAttachment,
} from "cap:mime";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("content_disposition: parse attachment, form-data, and RFC 5987 filename*", () => {
    let buf: [128]u8 = [0; 128];

    const d1 = ContentDisposition.parse("attachment; filename=\"fallback.txt\"; filename*=UTF-8''hello%20world%21.txt").?;
    expect(d1.kind == DispositionType.attachment);
    expect(strEq(d1.filename, "fallback.txt"));
    expect(strEq(d1.decodedFilename(buf[0..]).?, "hello world!.txt"));

    const d2 = ContentDisposition.parse("form-data; name=\"avatar\"; filename=\"pic;1.png\"").?;
    expect(d2.kind == DispositionType.form_data);
    expect(strEq(d2.name, "avatar") && strEq(d2.filename, "pic;1.png"));

    expect(ContentDisposition.parse("bogus; name=\"x\"") == null);
});

test("content_disposition: sanitizeFilename and formatAttachment", () => {
    let buf: [256]u8 = [0; 256];

    expect(strEq(sanitizeFilename("../../etc/passwd", buf[0..]).?, "passwd"));
    expect(strEq(sanitizeFilename("C:\\Users\\a\\doc:1.pdf", buf[0..]).?, "doc_1.pdf"));
    expect(sanitizeFilename("..", buf[0..]) == null);

    const h1 = formatAttachment("report.pdf", buf[0..]).?;
    expect(strEq(h1, "attachment; filename=\"report.pdf\""));

    // Non-ASCII filename emits both ASCII fallback and UTF-8 filename*
    const h2 = formatAttachment("报告.txt", buf[0..]).?;
    const reparsed = ContentDisposition.parse(h2).?;
    let dec_buf: [64]u8 = [0; 64];
    expect(strEq(reparsed.decodedFilename(dec_buf[0..]).?, "报告.txt"));
});
