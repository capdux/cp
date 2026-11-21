import { test, expect } from "cap:test";
import {
    WebLink,
    LinkHeaderIterator,
    findLinkByRel,
    formatWebLink,
} from "cap:http_link_header";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("http_link_header: parse pagination and preload Link headers", () => {
    const hdr =
        "<https://api.example.com/items?page=2&limit=50>; rel=\"next\"; title=\"Page 2, Next\", " ++
        "</assets/app.css>; rel=preload; as=style; crossorigin; nopush, " ++
        "<https://api.example.com/items?page=10>; rel=\"last\"";

    let it = LinkHeaderIterator.init(hdr);
    const l1 = it.next().?;
    expect(strEq(l1.uri, "https://api.example.com/items?page=2&limit=50"));
    expect(strEq(l1.rel, "next") && strEq(l1.title, "Page 2, Next"));

    const l2 = it.next().?;
    expect(strEq(l2.uri, "/assets/app.css") && strEq(l2.rel, "preload") && strEq(l2.as_dest, "style"));
    expect(l2.crossorigin && l2.nopush);

    const l3 = it.next().?;
    expect(strEq(l3.rel, "last"));
    expect(it.next() == null);

    const last = findLinkByRel(hdr, "LAST").?;
    expect(strEq(last.uri, "https://api.example.com/items?page=10"));
    expect(findLinkByRel(hdr, "prev") == null);
});

test("http_link_header: formatWebLink roundtrip", () => {
    let buf: [256]u8 = [0; 256];
    const link = WebLink{
        uri: "/fonts/inter.woff2",
        rel: "preload",
        as_dest: "font",
        media_type: "font/woff2",
        title: "",
        crossorigin: true,
        nopush: false,
    };
    const formatted = formatWebLink(link, buf[0..]).?;
    expect(strEq(formatted, "</fonts/inter.woff2>; rel=\"preload\"; as=font; type=\"font/woff2\"; crossorigin"));
});
