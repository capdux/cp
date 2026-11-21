import { test, expect_eq } from "cap:test";
import { ErrnoOs, SystemErrno } from "cap:errno_map";

test("1. Cross-platform Linux vs Darwin divergent numeric code mapping", () => {
    // EAGAIN: Linux 11 vs Darwin 35
    expect_eq(SystemErrno.fromLinuxCode(11).? == SystemErrno.EAGAIN, true);
    expect_eq(SystemErrno.fromDarwinCode(35).? == SystemErrno.EAGAIN, true);
    expect_eq(SystemErrno.EAGAIN.toOsCode(ErrnoOs.linux), i32(11));
    expect_eq(SystemErrno.EAGAIN.toOsCode(ErrnoOs.darwin), i32(35));

    // ETIMEDOUT: Linux 110 vs Darwin 60
    expect_eq(SystemErrno.fromLinuxCode(110).? == SystemErrno.ETIMEDOUT, true);
    expect_eq(SystemErrno.fromDarwinCode(60).? == SystemErrno.ETIMEDOUT, true);
    expect_eq(SystemErrno.ETIMEDOUT.toOsCode(ErrnoOs.linux), i32(110));
    expect_eq(SystemErrno.ETIMEDOUT.toOsCode(ErrnoOs.darwin), i32(60));

    // ECONNREFUSED: Linux 111 vs Darwin 61
    expect_eq(SystemErrno.fromLinuxCode(111).? == SystemErrno.ECONNREFUSED, true);
    expect_eq(SystemErrno.fromDarwinCode(61).? == SystemErrno.ECONNREFUSED, true);

    // ENOTEMPTY: Linux 39 vs Darwin 66
    expect_eq(SystemErrno.fromLinuxCode(39).? == SystemErrno.ENOTEMPTY, true);
    expect_eq(SystemErrno.fromDarwinCode(66).? == SystemErrno.ENOTEMPTY, true);

    // Negative kernel return value (-ENOENT = -2) supported
    expect_eq(SystemErrno.fromLinuxCode(-2).? == SystemErrno.ENOENT, true);
    expect_eq(SystemErrno.fromDarwinCode(-60).? == SystemErrno.ETIMEDOUT, true);
});

test("2. Full round-trip bijective consistency across all 69 SystemErrno variants", () => {
    comptime for (field of std.meta.fields(SystemErrno)) {
        const val = @field(SystemErrno, field.name);
        const lcode = val.toOsCode(ErrnoOs.linux);
        const dcode = val.toOsCode(ErrnoOs.darwin);
        expect_eq(SystemErrno.fromLinuxCode(lcode).? == val, true);
        expect_eq(SystemErrno.fromDarwinCode(dcode).? == val, true);
        expect_eq(std.mem.eql(u8, val.name(), field.name), true);
        expect_eq(SystemErrno.fromName(field.name).? == val, true);
    }
});

test("3. Name lookup & POSIX aliases (EWOULDBLOCK -> EAGAIN, ENOTSUP -> EOPNOTSUPP)", () => {
    expect_eq(SystemErrno.fromName("ENOENT").? == SystemErrno.ENOENT, true);
    expect_eq(SystemErrno.fromName("ECANCELED").? == SystemErrno.ECANCELED, true);
    expect_eq(SystemErrno.fromName("EWOULDBLOCK").? == SystemErrno.EAGAIN, true);
    expect_eq(SystemErrno.fromName("ENOTSUP").? == SystemErrno.EOPNOTSUPP, true);
    expect_eq(SystemErrno.fromName("enoent") == null, true);
    expect_eq(SystemErrno.fromName("") == null, true);
    expect_eq(SystemErrno.fromName("EUNKNOWN") == null, true);
});

test("4. OS-specific error messages (Linux vs Darwin differences)", () => {
    expect_eq(std.mem.eql(u8, SystemErrno.EBUSY.message(ErrnoOs.linux), "Device or resource busy"), true);
    expect_eq(std.mem.eql(u8, SystemErrno.EBUSY.message(ErrnoOs.darwin), "Resource busy"), true);

    expect_eq(std.mem.eql(u8, SystemErrno.ETIMEDOUT.message(ErrnoOs.linux), "Connection timed out"), true);
    expect_eq(std.mem.eql(u8, SystemErrno.ETIMEDOUT.message(ErrnoOs.darwin), "Operation timed out"), true);

    expect_eq(std.mem.eql(u8, SystemErrno.ENODEV.message(ErrnoOs.linux), "No such device"), true);
    expect_eq(std.mem.eql(u8, SystemErrno.ENODEV.message(ErrnoOs.darwin), "Operation not supported by device"), true);

    expect_eq(std.mem.eql(u8, SystemErrno.ENOENT.message(ErrnoOs.linux), "No such file or directory"), true);
    expect_eq(std.mem.eql(u8, SystemErrno.EACCES.message(ErrnoOs.darwin), "Permission denied"), true);
});

test("5. Transient retryable classification & unknown/out-of-range code guards", () => {
    expect_eq(SystemErrno.EINTR.isTransient(), true);
    expect_eq(SystemErrno.EAGAIN.isTransient(), true);
    expect_eq(SystemErrno.ETIMEDOUT.isTransient(), true);
    expect_eq(SystemErrno.ECONNRESET.isTransient(), true);
    expect_eq(SystemErrno.EBUSY.isTransient(), true);

    expect_eq(SystemErrno.ENOENT.isTransient(), false);
    expect_eq(SystemErrno.EACCES.isTransient(), false);
    expect_eq(SystemErrno.EINVAL.isTransient(), false);
    expect_eq(SystemErrno.ENOSYS.isTransient(), false);

    // Unknown / 0 / out-of-range codes return null
    expect_eq(SystemErrno.fromLinuxCode(0) == null, true);
    expect_eq(SystemErrno.fromLinuxCode(999) == null, true);
    expect_eq(SystemErrno.fromLinuxCode(-999) == null, true);
    expect_eq(SystemErrno.fromDarwinCode(0) == null, true);
    expect_eq(SystemErrno.fromDarwinCode(150) == null, true);
});
