import { test, expect } from "cap:test";
import {
    SQL_DATA_CELL_MAX_ROW_COLS,
    SqlDataCellError,
    SqlDataCellTag,
    SqlDataCellFlags,
    SqlDataCell,
    SqlDataRow,
} from "cap:sql";

test("SqlDataCell typed constructors and zero-copy accessors", () => {
    const c_null = SqlDataCell.nullCell();
    expect(c_null.isNull()).toBe(true);

    const c_i4 = SqlDataCell.fromInt4(-42);
    expect(c_i4.asInt64()?).toBe(-42);

    const c_i8 = SqlDataCell.fromInt8(-9000000000000);
    expect(c_i8.asInt64()?).toBe(-9000000000000);

    const c_u4 = SqlDataCell.fromUint4(4000000000);
    expect(c_u4.asUint64()?).toBe(4000000000);

    const c_u8 = SqlDataCell.fromUint8(18000000000000000000);
    expect(c_u8.asUint64()?).toBe(18000000000000000000);

    const c_f8 = SqlDataCell.fromFloat8(3.5);
    expect(c_f8.asFloat64()? == 3.5).toBe(true);

    const c_bool = SqlDataCell.fromBool(true);
    expect(c_bool.asBool()?).toBe(true);

    const c_str = SqlDataCell.fromString("hello");
    expect(c_str.asSlice()?).toBe("hello");

    const c_json = SqlDataCell.fromJson("{\"a\":1}");
    expect(c_json.asSlice()?).toBe("{\"a\":1}");

    const raw_bytes: [3]u8 = [0xDE, 0xAD, 0xBE];
    const c_bytea = SqlDataCell.fromBytea(raw_bytes[0..]);
    expect(c_bytea.asSlice()?.len).toBe(3);

    const c_raw = SqlDataCell.fromRaw(raw_bytes[0..]);
    expect(c_raw.tag == SqlDataCellTag.Raw).toBe(true);

    const c_date = SqlDataCell.fromDateMs(946684800000.0);
    expect(c_date.asFloat64()? == 946684800000.0).toBe(true);

    const c_datetz = SqlDataCell.fromDateTzMs(1704067200000.0);
    expect(c_datetz.asFloat64()? == 1704067200000.0).toBe(true);
});

test("SqlDataCellTag ABI discriminants, tagName, and category predicates", () => {
    expect(@intFromEnum(SqlDataCellTag.Null)).toBe(0);
    expect(@intFromEnum(SqlDataCellTag.String)).toBe(1);
    expect(@intFromEnum(SqlDataCellTag.Uint8)).toBe(14);

    expect(SqlDataCellTag.Null.tagName()).toBe("null");
    expect(SqlDataCellTag.DateWithTimeZone.tagName()).toBe("date_with_time_zone");
    expect(SqlDataCellTag.TypedArray.tagName()).toBe("typed_array");
    expect(SqlDataCellTag.Uint8.tagName()).toBe("uint8");

    expect(SqlDataCellTag.Int4.isNumeric()).toBe(true);
    expect(SqlDataCellTag.Uint8.isNumeric()).toBe(true);
    expect(SqlDataCellTag.Float8.isNumeric()).toBe(true);
    expect(SqlDataCellTag.String.isNumeric()).toBe(false);

    expect(SqlDataCellTag.Date.isTemporal()).toBe(true);
    expect(SqlDataCellTag.DateWithTimeZone.isTemporal()).toBe(true);
    expect(SqlDataCellTag.Int8.isTemporal()).toBe(false);

    expect(SqlDataCellTag.Bytea.isBinaryLike()).toBe(true);
    expect(SqlDataCellTag.Raw.isBinaryLike()).toBe(true);
    expect(SqlDataCellTag.TypedArray.isBinaryLike()).toBe(true);
    expect(SqlDataCellTag.Json.isBinaryLike()).toBe(false);
});

test("SqlDataCellFlags bitmask round-trip and fast object layout predicate", () => {
    const fast = SqlDataCellFlags{
        has_indexed_columns: false,
        has_named_columns: true,
        has_duplicate_columns: false,
    };
    expect(fast.canUseFastObjectLayout()).toBe(true);
    expect(fast.toMask()).toBe(0x02);

    const dup = SqlDataCellFlags.fromMask(0x07);
    expect(dup.has_indexed_columns).toBe(true);
    expect(dup.has_named_columns).toBe(true);
    expect(dup.has_duplicate_columns).toBe(true);
    expect(dup.canUseFastObjectLayout()).toBe(false);
});

test("SqlDataRow push, get, set, nullCount, clear, and cell equality", () => {
    let row = SqlDataRow.init();
    * = row.push(SqlDataCell.fromInt4(10))?;
    * = row.push(SqlDataCell.nullCell())?;
    * = row.push(SqlDataCell.fromString("cp"))?;
    expect(row.len).toBe(3);
    expect(row.nullCount()).toBe(1);

    if (c0 := row.get(0)) {
        expect(c0.eql(SqlDataCell.fromInt4(10))).toBe(true);
        expect(c0.eql(SqlDataCell.fromInt8(10))).toBe(false);
    } else {
        expect(false).toBe(true);
    }
    expect(row.get(3) == null).toBe(true);

    // Overwrite NULL cell with Bool -> nullCount becomes 0
    expect(row.set(1, SqlDataCell.fromBool(false))?).toBe(true);
    expect(row.nullCount()).toBe(0);

    row.clear();
    expect(row.len).toBe(0);
});

test("SqlDataCell and SqlDataRow enforce TypeMismatch, RowFull, and IndexOutOfBounds guards", () => {
    expect(SQL_DATA_CELL_MAX_ROW_COLS).toBe(64);

    const s = SqlDataCell.fromString("abc");
    let got_mismatch = false;
    * = s.asInt64() catch |e| {
        if (e == SqlDataCellError.TypeMismatch) {
            got_mismatch = true;
        }
        0
    };
    expect(got_mismatch).toBe(true);

    let row = SqlDataRow.init();
    let got_oob = false;
    * = row.set(0, SqlDataCell.nullCell()) catch |e| {
        if (e == SqlDataCellError.IndexOutOfBounds) {
            got_oob = true;
        }
        false
    };
    expect(got_oob).toBe(true);

    let i: usize = 0;
    while (i < SQL_DATA_CELL_MAX_ROW_COLS) {
        * = row.push(SqlDataCell.fromInt4(@as(i32, @intCast(i))))?;
        i += 1;
    }
    let got_full = false;
    * = row.push(SqlDataCell.nullCell()) catch |e| {
        if (e == SqlDataCellError.RowFull) {
            got_full = true;
        }
        0
    };
    expect(got_full).toBe(true);
});
