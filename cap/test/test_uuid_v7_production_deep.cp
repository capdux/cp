// Source: cp/cap/test/test_uuid_v7_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { UuidError, Uuid, NAMESPACE_DNS } from "cap:uuid_v7";

test "uuid_v7: RFC 9562 UUID v7 timestamp ordering & format roundtrip" {
    const u1 = Uuid.v7At(1700000000000);
    const u2 = Uuid.v7At(1700000001000);
    try expect_eq(u1.version(), 7);
    try expect_eq(u1.variantRfc9562(), true);
    try expect_eq(u1.timestampMsV7(), 1700000000000);
    try expect_eq(u1.compare(u2), -1);

    const formatted = u1.format();
    const parsed = try Uuid.parse(&formatted);
    try expect_eq(u1.equals(parsed), true);
}

test "uuid_v7: RFC 9562 UUID v5 official DNS vector (python.org)" {
    // Well-known RFC 4122 / 9562 vector: v5(DNS, "python.org") = 886313e1-3b8a-5372-9b90-0c9aee199e5d
    const u = Uuid.v5(NAMESPACE_DNS, "python.org");
    const s = u.format();
    try expect_eq(u.version(), 5);
    try expect_eq(u.variantRfc9562(), true);
    try expect_eq(std.mem.eql(u8, &s, "886313e1-3b8a-5372-9b90-0c9aee199e5d"), true);
}

test "uuid_v7: UUID v4 randomness & version/variant bits" {
    const a = Uuid.v4();
    const b = Uuid.v4();
    try expect_eq(a.version(), 4);
    try expect_eq(a.variantRfc9562(), true);
    try expect_eq(a.equals(b), false);
}

test "uuid_v7: parse error guards" {
    try expect_eq(Uuid.parse("short") == UuidError.InvalidLength, true);
    try expect_eq(Uuid.parse("886313e1_3b8a-5372-9b90-0c9aee199e5d") == UuidError.InvalidFormat, true);
    try expect_eq(Uuid.parse("886313e1-3b8a-5372-9b90-0c9aee199e5z") == UuidError.InvalidHex, true);
}

test "uuid_v7: cap.uuid_v7 dynamic mount" {
    const id = cap.uuid_v7.Uuid.v7();
    try expect_eq(id.version(), 7);
    try expect_eq(id.variantRfc9562(), true);
}
