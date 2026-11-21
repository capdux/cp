// Source: cp/cap/test/test_pg_oids_production_deep.cp (Production deep tests for cap:pg_oids)

import { test, expect_eq } from "cap:test";
import {
    PgOid,
    PgTypeCategory,
} from "cap:pg_oids";

test("pg_oids: bidirectional scalar <-> array OID round-trip across all 15 array pairs") {
    const pairs_scalar: [15]PgOid = [
        .BOOL,
        .BYTEA,
        .INT2,
        .INT4,
        .INT8,
        .TEXT,
        .VARCHAR,
        .FLOAT4,
        .FLOAT8,
        .UUID,
        .JSON,
        .JSONB,
        .TIMESTAMP,
        .TIMESTAMPTZ,
        .NUMERIC,
    ];
    const pairs_array: [15]PgOid = [
        .BOOL_ARRAY,
        .BYTEA_ARRAY,
        .INT2_ARRAY,
        .INT4_ARRAY,
        .INT8_ARRAY,
        .TEXT_ARRAY,
        .VARCHAR_ARRAY,
        .FLOAT4_ARRAY,
        .FLOAT8_ARRAY,
        .UUID_ARRAY,
        .JSON_ARRAY,
        .JSONB_ARRAY,
        .TIMESTAMP_ARRAY,
        .TIMESTAMPTZ_ARRAY,
        .NUMERIC_ARRAY,
    ];

    let i: usize = 0;
    while (i < 15) {
        const sc = pairs_scalar[i];
        const ar = pairs_array[i];

        expect_eq(sc.isArray(), false);
        expect_eq(ar.isArray(), true);
        expect_eq(ar.category() == .array, true);

        expect_eq(sc.arrayOid().? == ar, true);
        expect_eq(ar.elementOid().? == sc, true);
        expect_eq(sc.elementOid() == null, true);
        expect_eq(ar.arrayOid() == null, true);

        // fromU32 <-> toU32 精确往返
        expect_eq(PgOid.fromU32(sc.toU32()).? == sc, true);
        expect_eq(PgOid.fromU32(ar.toU32()).? == ar, true);
        i += 1;
    }
}

test("pg_oids: type categorization across all PgTypeCategory variants") {
    expect_eq(PgOid.BOOL.category() == .boolean, true);
    expect_eq(PgOid.INT2.category() == .integer, true);
    expect_eq(PgOid.INT4.category() == .integer, true);
    expect_eq(PgOid.INT8.category() == .integer, true);
    expect_eq(PgOid.OID.category() == .integer, true);
    expect_eq(PgOid.FLOAT4.category() == .floating, true);
    expect_eq(PgOid.FLOAT8.category() == .floating, true);
    expect_eq(PgOid.NUMERIC.category() == .numeric, true);
    expect_eq(PgOid.MONEY.category() == .numeric, true);
    expect_eq(PgOid.TEXT.category() == .text, true);
    expect_eq(PgOid.VARCHAR.category() == .text, true);
    expect_eq(PgOid.BPCHAR.category() == .text, true);
    expect_eq(PgOid.XML.category() == .text, true);
    expect_eq(PgOid.BYTEA.category() == .binary, true);
    expect_eq(PgOid.BIT.category() == .binary, true);
    expect_eq(PgOid.VARBIT.category() == .binary, true);
    expect_eq(PgOid.JSON.category() == .json, true);
    expect_eq(PgOid.JSONB.category() == .json, true);
    expect_eq(PgOid.DATE.category() == .datetime, true);
    expect_eq(PgOid.TIME.category() == .datetime, true);
    expect_eq(PgOid.TIMETZ.category() == .datetime, true);
    expect_eq(PgOid.TIMESTAMP.category() == .datetime, true);
    expect_eq(PgOid.TIMESTAMPTZ.category() == .datetime, true);
    expect_eq(PgOid.INTERVAL.category() == .datetime, true);
    expect_eq(PgOid.CIDR.category() == .network, true);
    expect_eq(PgOid.INET.category() == .network, true);
    expect_eq(PgOid.MACADDR.category() == .network, true);
    expect_eq(PgOid.UUID.category() == .uuid, true);
    expect_eq(PgOid.UUID_ARRAY.category() == .array, true);
    expect_eq(std.mem.eql(u8, PgTypeCategory.datetime.asStr(), "datetime"), true);
}

test("pg_oids: canonical PostgreSQL typeName formatting") {
    expect_eq(std.mem.eql(u8, PgOid.BOOL.typeName(), "bool"), true);
    expect_eq(std.mem.eql(u8, PgOid.INT4.typeName(), "int4"), true);
    expect_eq(std.mem.eql(u8, PgOid.INT4_ARRAY.typeName(), "int4[]"), true);
    expect_eq(std.mem.eql(u8, PgOid.TIMESTAMPTZ.typeName(), "timestamptz"), true);
    expect_eq(std.mem.eql(u8, PgOid.TIMESTAMPTZ_ARRAY.typeName(), "timestamptz[]"), true);
    expect_eq(std.mem.eql(u8, PgOid.UUID.typeName(), "uuid"), true);
    expect_eq(std.mem.eql(u8, PgOid.JSONB.typeName(), "jsonb"), true);
    expect_eq(std.mem.eql(u8, PgOid.JSONB_ARRAY.typeName(), "jsonb[]"), true);
    expect_eq(std.mem.eql(u8, PgOid.NUMERIC_ARRAY.typeName(), "numeric[]"), true);
}

test("pg_oids: prefersBinary wire encoding preference") {
    // 固定长度或原生二进制类型优先二进制编码
    expect_eq(PgOid.BOOL.prefersBinary(), true);
    expect_eq(PgOid.BYTEA.prefersBinary(), true);
    expect_eq(PgOid.INT2.prefersBinary(), true);
    expect_eq(PgOid.INT4.prefersBinary(), true);
    expect_eq(PgOid.INT8.prefersBinary(), true);
    expect_eq(PgOid.FLOAT4.prefersBinary(), true);
    expect_eq(PgOid.FLOAT8.prefersBinary(), true);
    expect_eq(PgOid.UUID.prefersBinary(), true);
    expect_eq(PgOid.TIMESTAMP.prefersBinary(), true);
    expect_eq(PgOid.TIMESTAMPTZ.prefersBinary(), true);
    expect_eq(PgOid.JSONB.prefersBinary(), true);
    expect_eq(PgOid.INT4_ARRAY.prefersBinary(), true);

    // 文本、高精度小数、原始 JSON、网络与区间类型优先文本格式
    expect_eq(PgOid.TEXT.prefersBinary(), false);
    expect_eq(PgOid.VARCHAR.prefersBinary(), false);
    expect_eq(PgOid.NUMERIC.prefersBinary(), false);
    expect_eq(PgOid.JSON.prefersBinary(), false);
    expect_eq(PgOid.INET.prefersBinary(), false);
    expect_eq(PgOid.INTERVAL.prefersBinary(), false);
    expect_eq(PgOid.TEXT_ARRAY.prefersBinary(), false);
}

test("pg_oids: unknown OID guards and boundary values") {
    expect_eq(PgOid.fromU32(0) == null, true);
    expect_eq(PgOid.fromU32(1) == null, true);
    expect_eq(PgOid.fromU32(999999) == null, true);
    expect_eq(PgOid.fromU32(4294967295) == null, true);

    // 确认关键 OID 数值与 PostgreSQL pg_type 一致
    expect_eq(PgOid.fromU32(16).? == .BOOL, true);
    expect_eq(PgOid.fromU32(23).? == .INT4, true);
    expect_eq(PgOid.fromU32(1700).? == .NUMERIC, true);
    expect_eq(PgOid.fromU32(2950).? == .UUID, true);
    expect_eq(PgOid.fromU32(3802).? == .JSONB, true);
    expect_eq(PgOid.fromU32(3807).? == .JSONB_ARRAY, true);
}
