import { ClearSiteData } from "cap:http_clear_site_data";
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

test("ClearSiteData parses quoted directives, wildcard *, and formats canonically", () => {
    let buf: [128]u8 = [0; 128];

    const c1 = ClearSiteData.parse("\"cache\", \"cookies\", \"storage\"");
    expect(c1.cache && c1.cookies && c1.storage && !c1.execution_contexts);
    expect(strEq(c1.format(buf[0..]).?, "\"cache\", \"cookies\", \"storage\""));

    const c2 = ClearSiteData.parse("\"*\"");
    expect(c2.isAll());
    expect(strEq(c2.format(buf[0..]).?, "\"*\""));

    // Unquoted or wrong-case tokens ignored per W3C spec
    const c3 = ClearSiteData.parse("cache, \"CACHE\", \"executionContexts\"");
    expect(!c3.cache && c3.execution_contexts);
    expect(strEq(c3.format(buf[0..]).?, "\"executionContexts\""));

    expect(ClearSiteData.parse("").isEmpty());
});
