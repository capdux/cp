// Deep production regression tests for cap:errno freebsdErrno.cp
// Covers all 98 FreeBSD <sys/errno.h> entries, negative syscall decoding,
// Capsicum sandbox errors (ENOTCAPABLE / ECAPMODE), BSD-specific codes,
// alias lookups (EWOULDBLOCK / ENOTSUP), and bidirectional SystemErrno translation.

import { test, expect } from "cap:test";
import {
    FREEBSD_MAX_ERRNO,
    FreebsdErrno,
    SystemErrno,
} from "cap:errno";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("freebsdErrno: full 98-entry table alignment and name round-trip", () => {
    expect(FREEBSD_MAX_ERRNO == 98);

    let code: i32 = 0;
    while (code < 98) {
        const fb_opt = FreebsdErrno.fromCode(code);
        expect(fb_opt != null);
        if (fb := fb_opt) {
            expect(fb.toCode() == code);
            const sym = fb.codeName();
            expect(sym.len > 0);

            const by_name = FreebsdErrno.fromName(sym);
            expect(by_name != null && by_name.? == fb);

            if (code > 0) {
                const neg = FreebsdErrno.fromCode(-code);
                expect(neg != null && neg.? == fb);
            }
        }
        code += 1;
    }
});

test("freebsdErrno: out-of-range codes and alias lookups", () => {
    expect(FreebsdErrno.fromCode(98) == null);
    expect(FreebsdErrno.fromCode(-98) == null);
    expect(FreebsdErrno.fromCode(-2147483648) == null);
    expect(FreebsdErrno.fromCode(2147483647) == null);

    expect(FreebsdErrno.fromName("") == null);
    expect(FreebsdErrno.fromName("EBADMACHO") == null);
    expect(FreebsdErrno.fromName("EWOULDBLOCK").? == .EAGAIN);
    expect(FreebsdErrno.fromName("ENOTSUP").? == .EOPNOTSUPP);
});

test("freebsdErrno: Capsicum sandbox and FreeBSD-specific error classification", () => {
    expect(FreebsdErrno.ENOTCAPABLE.toCode() == 93);
    expect(FreebsdErrno.ECAPMODE.toCode() == 94);
    expect(FreebsdErrno.ENOTCAPABLE.isCapsicumError());
    expect(FreebsdErrno.ECAPMODE.isCapsicumError());
    expect(!FreebsdErrno.EPERM.isCapsicumError());
    expect(!FreebsdErrno.EACCES.isCapsicumError());

    expect(FreebsdErrno.ENOTCAPABLE.isFreebsdSpecific());
    expect(FreebsdErrno.ECAPMODE.isFreebsdSpecific());
    expect(FreebsdErrno.EDOOFUS.isFreebsdSpecific());
    expect(FreebsdErrno.EINTEGRITY.isFreebsdSpecific());
    expect(FreebsdErrno.EINTEGRITY.toCode() == 97);
    expect(strEq(FreebsdErrno.EINTEGRITY.codeName(), "EINTEGRITY"));
    expect(FreebsdErrno.EINTEGRITY.toSystemErrno() == .UNKNOWN);

    expect(!FreebsdErrno.ENOENT.isFreebsdSpecific());
    expect(!FreebsdErrno.ECONNREFUSED.isFreebsdSpecific());
});

test("freebsdErrno: FreeBSD vs Linux divergent errno bidirectional translation", () => {
    // ECANCELED: FreeBSD 85 <-> Linux 125
    expect(FreebsdErrno.ECANCELED.toCode() == 85);
    expect(FreebsdErrno.ECANCELED.toSystemErrno() == .ECANCELED);
    expect(FreebsdErrno.fromSystemErrno(.ECANCELED).? == .ECANCELED);

    // EIDRM: FreeBSD 82 <-> Linux 43
    expect(FreebsdErrno.EIDRM.toCode() == 82);
    expect(FreebsdErrno.EIDRM.toSystemErrno() == .EIDRM);
    expect(FreebsdErrno.fromSystemErrno(.EIDRM).? == .EIDRM);

    // EPROTO: FreeBSD 92 <-> Linux 71
    expect(FreebsdErrno.EPROTO.toCode() == 92);
    expect(FreebsdErrno.EPROTO.toSystemErrno() == .EPROTO);
    expect(FreebsdErrno.fromSystemErrno(.EPROTO).? == .EPROTO);

    // EOPNOTSUPP (45) <-> SystemErrno.ENOTSUP
    expect(FreebsdErrno.EOPNOTSUPP.toCode() == 45);
    expect(FreebsdErrno.EOPNOTSUPP.toSystemErrno() == .ENOTSUP);
    expect(FreebsdErrno.fromSystemErrno(.ENOTSUP).? == .EOPNOTSUPP);
});

test("freebsdErrno: isOk, isRetryable, and Linux-only SystemErrno reverse lookup", () => {
    expect(FreebsdErrno.SUCCESS.isOk());
    expect(!FreebsdErrno.EIO.isOk());

    expect(FreebsdErrno.EINTR.isRetryable());
    expect(FreebsdErrno.EAGAIN.isRetryable());
    expect(FreebsdErrno.EBUSY.isRetryable());
    expect(FreebsdErrno.ETIMEDOUT.isRetryable());
    expect(!FreebsdErrno.ENOTCAPABLE.isRetryable());
    expect(!FreebsdErrno.EINVAL.isRetryable());

    expect(FreebsdErrno.fromSystemErrno(.EWOULDBLOCK).? == .EAGAIN);
    expect(FreebsdErrno.fromSystemErrno(.UNKNOWN) == null);
    expect(FreebsdErrno.fromSystemErrno(.ENODATA) == null);
});

test("freebsdErrno: EOWNERDEAD (96) and ENOTRECOVERABLE (95) mutex recovery codes", () => {
    expect(FreebsdErrno.ENOTRECOVERABLE.toCode() == 95);
    expect(FreebsdErrno.EOWNERDEAD.toCode() == 96);
    expect(FreebsdErrno.ENOTRECOVERABLE.toSystemErrno() == .ENOTRECOVERABLE);
    expect(FreebsdErrno.EOWNERDEAD.toSystemErrno() == .EOWNERDEAD);
    expect(FreebsdErrno.fromSystemErrno(.EOWNERDEAD).? == .EOWNERDEAD);
    expect(FreebsdErrno.fromSystemErrno(.ENOTRECOVERABLE).? == .ENOTRECOVERABLE);
});
