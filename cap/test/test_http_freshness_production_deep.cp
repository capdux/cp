import {
    CacheFreshness,
    FreshnessInput,
} from "cap:http_freshness";
import { test, expect } from "cap:test";

test("FreshnessInput RFC 9111 precedence, age correction, and SWR/SIE windows", () => {
    // Shared cache prefers s-maxage (120) over max-age (60)
    let f1 = FreshnessInput.empty(true, 1000, 1002);
    f1.max_age_sec = 60;
    f1.s_maxage_sec = 120;
    f1.stale_while_revalidate_sec = 30;
    f1.stale_if_error_sec = 60;
    f1.age_header_sec = 10;
    // response_delay = 2, corrected_age_value = 12, resident at t=1100 = 98 -> age = 110 < 120 (fresh)
    expect(f1.freshnessLifetimeSec() == 120);
    expect(f1.currentAgeSec(1100) == 110);
    expect(f1.remainingFreshSec(1100) == 10);
    expect(f1.evaluate(1100) == CacheFreshness.fresh);

    // At t=1120 -> age=130 in [120, 150) -> stale_while_revalidate
    expect(f1.evaluate(1120) == CacheFreshness.stale_while_revalidate);
    // At t=1150 -> age=160 in [150, 180) -> stale, but stale-if-error still true
    expect(f1.evaluate(1150) == CacheFreshness.stale);
    expect(f1.canServeStaleOnError(1150));
    expect(!f1.canServeStaleOnError(1200));

    // Private cache ignores s-maxage
    f1.is_shared_cache = false;
    expect(f1.freshnessLifetimeSec() == 60);
});

test("FreshnessInput Expires and 10% Last-Modified heuristic fallback", () => {
    let f2 = FreshnessInput.empty(false, 2000, 2000);
    f2.date_unix_sec = 2000;
    f2.expires_unix_sec = 2300;
    expect(f2.freshnessLifetimeSec() == 300);

    let f3 = FreshnessInput.empty(false, 5000, 5000);
    f3.date_unix_sec = 5000;
    f3.last_modified_unix_sec = 4000;
    expect(f3.freshnessLifetimeSec() == 100); // 10% of 1000s
});
