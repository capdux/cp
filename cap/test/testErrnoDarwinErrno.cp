// Deep production regression tests for cap:errno darwinErrno.cp
// Covers all 107 Darwin <sys/errno.h> entries, negative syscall decoding,
// Darwin-specific error classification, name lookup (incl. EWOULDBLOCK alias),
// and bidirectional DarwinErrno <-> SystemErrno cross-platform translation.

import { test, expect } from "cap:test";
import {
    DARWIN_MAX_ERRNO,
    DarwinErrno,
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

test("darwinErrno: full 107-entry table alignment and name round-trip", () => {
    expect(DARWIN_MAX_ERRNO == 107);

    let code: i32 = 0;
    while (code < 107) {
        const d_opt = DarwinErrno.fromCode(code);
        expect(d_opt != null);
        if (d := d_opt) {
            expect(d.toCode() == code);
            const name = d.codeName();
            expect(name.len > 0);

            const by_name = DarwinErrno.fromName(name);
            expect(by_name != null);
            if (bn := by_name) {
                expect(bn == d);
            }

            // Negative syscall encoding (-code) resolves to the same variant
            if (code > 0) {
                const neg = DarwinErrno.fromCode(-code);
                expect(neg != null && neg.? == d);
            }
        }
        code += 1;
    }
});

test("darwinErrno: out-of-range and extreme i32 values safely return null", () => {
    expect(DarwinErrno.fromCode(107) == null);
    expect(DarwinErrno.fromCode(9999) == null);
    expect(DarwinErrno.fromCode(-107) == null);
    expect(DarwinErrno.fromCode(-2147483648) == null);
    expect(DarwinErrno.fromCode(2147483647) == null);

    expect(DarwinErrno.fromName("") == null);
    expect(DarwinErrno.fromName("ENOTREAL") == null);
    // EWOULDBLOCK aliases to EAGAIN (35) on Darwin
    expect(DarwinErrno.fromName("EWOULDBLOCK").? == .EAGAIN);
});

test("darwinErrno: Linux vs Darwin divergent errno numbers translate accurately", () => {
    // EAGAIN: Darwin 35 <-> Linux 11
    expect(DarwinErrno.EAGAIN.toCode() == 35);
    expect(DarwinErrno.EAGAIN.toSystemErrno() == .EAGAIN);
    expect(SystemErrno.EAGAIN.toCode() == 11);
    expect(DarwinErrno.fromSystemErrno(.EAGAIN).? == .EAGAIN);

    // EADDRINUSE: Darwin 48 <-> Linux 98
    expect(DarwinErrno.EADDRINUSE.toCode() == 48);
    expect(DarwinErrno.EADDRINUSE.toSystemErrno() == .EADDRINUSE);
    expect(DarwinErrno.fromSystemErrno(.EADDRINUSE).? == .EADDRINUSE);

    // ETIMEDOUT: Darwin 60 <-> Linux 110
    expect(DarwinErrno.ETIMEDOUT.toCode() == 60);
    expect(DarwinErrno.ETIMEDOUT.toSystemErrno() == .ETIMEDOUT);
    expect(DarwinErrno.fromSystemErrno(.ETIMEDOUT).? == .ETIMEDOUT);

    // ECONNREFUSED: Darwin 61 <-> Linux 111
    expect(DarwinErrno.ECONNREFUSED.toCode() == 61);
    expect(DarwinErrno.ECONNREFUSED.toSystemErrno() == .ECONNREFUSED);
    expect(DarwinErrno.fromSystemErrno(.ECONNREFUSED).? == .ECONNREFUSED);

    // ENOTEMPTY: Darwin 66 <-> Linux 39
    expect(DarwinErrno.ENOTEMPTY.toCode() == 66);
    expect(DarwinErrno.ENOTEMPTY.toSystemErrno() == .ENOTEMPTY);
    expect(DarwinErrno.fromSystemErrno(.ENOTEMPTY).? == .ENOTEMPTY);

    // ENOSYS: Darwin 78 <-> Linux 38
    expect(DarwinErrno.ENOSYS.toCode() == 78);
    expect(DarwinErrno.ENOSYS.toSystemErrno() == .ENOSYS);
    expect(DarwinErrno.fromSystemErrno(.ENOSYS).? == .ENOSYS);
});

test("darwinErrno: Darwin-specific Mach-O, RPC, and extended-attribute codes", () => {
    expect(DarwinErrno.EBADMACHO.toCode() == 88);
    expect(DarwinErrno.EBADMACHO.isDarwinSpecific());
    expect(DarwinErrno.EBADMACHO.toSystemErrno() == .UNKNOWN);
    expect(strEq(DarwinErrno.EBADMACHO.codeName(), "EBADMACHO"));

    expect(DarwinErrno.EBADARCH.isDarwinSpecific());
    expect(DarwinErrno.ENOATTR.isDarwinSpecific());
    expect(DarwinErrno.EPWROFF.isDarwinSpecific());
    expect(DarwinErrno.EQFULL.isDarwinSpecific());
    expect(DarwinErrno.EQFULL.toCode() == 106);

    expect(!DarwinErrno.ENOENT.isDarwinSpecific());
    expect(!DarwinErrno.ECONNRESET.isDarwinSpecific());
    expect(!DarwinErrno.SUCCESS.isDarwinSpecific());
});

test("darwinErrno: isOk, isRetryable, and Linux-only SystemErrno reverse lookup", () => {
    expect(DarwinErrno.SUCCESS.isOk());
    expect(!DarwinErrno.ENOENT.isOk());

    expect(DarwinErrno.EINTR.isRetryable());
    expect(DarwinErrno.EAGAIN.isRetryable());
    expect(DarwinErrno.EBUSY.isRetryable());
    expect(DarwinErrno.ETIMEDOUT.isRetryable());
    expect(!DarwinErrno.EACCES.isRetryable());
    expect(!DarwinErrno.ENOENT.isRetryable());

    // EWOULDBLOCK maps to Darwin EAGAIN
    expect(DarwinErrno.fromSystemErrno(.EWOULDBLOCK).? == .EAGAIN);
    // Linux-only EHWPOISON / UNKNOWN have no Darwin equivalent -> null
    expect(DarwinErrno.fromSystemErrno(.UNKNOWN) == null);
    expect(DarwinErrno.fromSystemErrno(.EHWPOISON) == null);
});

test("darwinErrno: EOPNOTSUPP (102) and ENOTSUP (45) both map to SystemErrno.ENOTSUP", () => {
    expect(DarwinErrno.ENOTSUP.toCode() == 45);
    expect(DarwinErrno.EOPNOTSUPP.toCode() == 102);
    expect(DarwinErrno.ENOTSUP.toSystemErrno() == .ENOTSUP);
    expect(DarwinErrno.EOPNOTSUPP.toSystemErrno() == .ENOTSUP);
});
