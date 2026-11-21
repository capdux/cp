import {
    BrandListIter,
    findBrandVersion,
    parseMobileHint,
    parsePlatformHint,
} from "cap:http";
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

test("BrandListIter, GREASE detection, and Sec-CH-UA-* helpers", () => {
    const hdr = "\"Chromium\";v=\"128\", \"Not;A=Brand\";v=\"24\", \"Google Chrome\";v=\"128\"";
    let it = BrandListIter.init(hdr);

    const b1 = it.next().?;
    expect(strEq(b1.brand, "Chromium") && strEq(b1.version, "128") && !b1.isGrease());

    const b2 = it.next().?;
    expect(strEq(b2.brand, "Not;A=Brand") && strEq(b2.version, "24") && b2.isGrease());

    const b3 = it.next().?;
    expect(strEq(b3.brand, "Google Chrome") && strEq(b3.version, "128") && !b3.isGrease());
    expect(it.next() == null);

    expect(strEq(findBrandVersion(hdr, "google chrome").?, "128"));
    expect(findBrandVersion(hdr, "Firefox") == null);

    expect(parseMobileHint("?1").? == true);
    expect(parseMobileHint("?0").? == false);
    expect(parseMobileHint("1") == null);

    expect(strEq(parsePlatformHint("\"macOS\"").?, "macOS"));
    expect(parsePlatformHint("unquoted") == null);
});
