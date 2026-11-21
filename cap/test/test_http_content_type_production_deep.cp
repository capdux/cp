import { ContentType } from "cap:http";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("ContentType parses essence, charset, boundary, and classifiers", () => {
    const ct1 = ContentType.parse("Application/Problem+JSON; Charset=\"UTF-8\"; version=1").?;
    expect(ct1.isJson());
    expect(!ct1.isXml());
    expect(strEq(ct1.charset().?, "UTF-8"));
    expect(strEq(ct1.param("version").?, "1"));

    const ct2 = ContentType.parse("multipart/form-data; boundary=\"----WebKit;123\"").?;
    expect(ct2.isMultipart());
    expect(strEq(ct2.boundary().?, "----WebKit;123"));

    const ct3 = ContentType.parse("application/x-www-form-urlencoded").?;
    expect(ct3.isFormUrlEncoded());

    const ct4 = ContentType.parse("text/event-stream; charset=utf-8").?;
    expect(ct4.isText());
    expect(ct4.isEventStream());

    const ct5 = ContentType.parse("application/atom+xml").?;
    expect(ct5.isXml());

    expect(ContentType.parse("") == null);
    expect(ContentType.parse("text") == null);
    expect(ContentType.parse("a/b/c") == null);
});
