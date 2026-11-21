import {
    FetchSite,
    FetchMode,
    FetchDest,
    FetchMetadata,
} from "cap:http_sec_fetch";
import { test, expect } from "cap:test";

test("FetchMetadata parses headers and enforces W3C Resource Isolation Policy", () => {
    // Same-origin API POST allowed
    const m1 = FetchMetadata.parse("same-origin", "cors", "empty", null);
    expect(m1.site.? == FetchSite.same_origin);
    expect(m1.mode.? == FetchMode.cors);
    expect(m1.dest.? == FetchDest.empty);
    expect(m1.allowByResourceIsolation(false));
    expect(m1.allowSameOriginApi(false));

    // Cross-site top-level GET navigation allowed
    const m2 = FetchMetadata.parse("cross-site", "navigate", "document", "?1");
    expect(m2.user_activated);
    expect(m2.allowByResourceIsolation(true));
    expect(!m2.allowByResourceIsolation(false)); // cross-site POST blocked
    expect(!m2.allowSameOriginApi(true));

    // Cross-site <object>/<embed> navigation blocked even on GET
    const m3 = FetchMetadata.parse("cross-site", "navigate", "object", null);
    expect(!m3.allowByResourceIsolation(true));

    // Cross-site XHR/fetch blocked
    const m4 = FetchMetadata.parse("cross-site", "cors", "empty", null);
    expect(!m4.allowByResourceIsolation(true));

    // Legacy client (no Sec-Fetch-Site)
    const m5 = FetchMetadata.parse(null, null, null, null);
    expect(m5.allowByResourceIsolation(false));
    expect(m5.allowSameOriginApi(true));
    expect(!m5.allowSameOriginApi(false));
});
