// Regression test for Task 3C: Unused-Import / On-Demand Link Tree-Shaking
// Case 1: `@cap/picohttp` is imported and referenced (`phr_parse_request`),
//         `@cap/zstd` is imported (`Compressor`) but never referenced.
// Expected: generated `.zig` contains `// @cap-used-pkg: @cap/picohttp`,
//           does NOT contain `// @cap-used-pkg: @cap/zstd`,
//           and does NOT pull in any `ZSTD_` symbols.

import { phr_parse_request } from "@cap/picohttp";
import { Compressor } from "zstd";

pub fn checkPicoSymbolRef(): bool {
    const fn_ptr = phr_parse_request;
    return unsafe { @intFromPtr(&fn_ptr) != 0 };
}

pub fn main() {
    const ok = checkPicoSymbolRef();
    if (!ok) {
        @panic("phr_parse_request symbol missing");
    }
}
