import { test, expect } from "cap:test";
import {
    MYSQL_STMT_SIG_MAX_PARAMS,
    MYSQL_STMT_SIG_MAX_NAME_LEN,
    MySqlStmtSignatureError,
    MySqlSigFieldType,
    MySqlSigParam,
    MySqlStmtSignature,
} from "cap:mysql";

test("MySqlStmtSignature builds signed, unsigned, and null parameter signature keys", () => {
    const params: [4]MySqlSigParam = [
        MySqlSigParam.unsigned(MySqlSigFieldType.Long),
        MySqlSigParam.nullParam(),
        MySqlSigParam.signed(MySqlSigFieldType.Double),
        MySqlSigParam.signed(MySqlSigFieldType.VarString),
    ];

    const sig = MySqlStmtSignature.build("SELECT * FROM items WHERE id=? AND x=? AND p=? AND s=?", params[0..])?;
    expect(sig.isEmpty()).toBe(false);
    expect(sig.querySlice()).toBe("SELECT * FROM items WHERE id=? AND x=? AND p=? AND s=?");
    expect(sig.fieldCount()).toBe(4);
    expect(sig.nameSlice()).toBe(
        "SELECT * FROM items WHERE id=? AND x=? AND p=? AND s=?UMYSQL_TYPE_LONG.nullMYSQL_TYPE_DOUBLEMYSQL_TYPE_VAR_STRING",
    );

    const fields = sig.fields();
    expect(fields.len).toBe(4);
    expect(fields[0].field_type == MySqlSigFieldType.Long).toBe(true);
    expect(fields[0].is_unsigned).toBe(true);
    expect(fields[0].wireFlagByte()).toBe(0x80);

    expect(fields[1].field_type == MySqlSigFieldType.Null).toBe(true);
    expect(fields[1].is_unsigned).toBe(false);
    expect(fields[1].wireFlagByte()).toBe(0x00);

    expect(fields[2].field_type == MySqlSigFieldType.Double).toBe(true);
    expect(fields[2].is_unsigned).toBe(false);

    expect(fields[3].field_type == MySqlSigFieldType.VarString).toBe(true);
});

test("MySqlStmtSignature distinguishes signed vs unsigned parameters in hash and equality", () => {
    const p_signed: [1]MySqlSigParam = [MySqlSigParam.signed(MySqlSigFieldType.LongLong)];
    const p_unsigned: [1]MySqlSigParam = [MySqlSigParam.unsigned(MySqlSigFieldType.LongLong)];

    const sig_s1 = MySqlStmtSignature.build("SELECT ?", p_signed[0..])?;
    const sig_s2 = MySqlStmtSignature.build("SELECT ?", p_signed[0..])?;
    const sig_u = MySqlStmtSignature.build("SELECT ?", p_unsigned[0..])?;

    expect(sig_s1.eql(sig_s2)).toBe(true);
    expect(sig_s1.hash()).toBe(sig_s2.hash());

    expect(sig_s1.eql(sig_u)).toBe(false);
    expect(sig_s1.hash() != sig_u.hash()).toBe(true);
    expect(sig_s1.nameSlice()).toBe("SELECT ?MYSQL_TYPE_LONGLONG");
    expect(sig_u.nameSlice()).toBe("SELECT ?UMYSQL_TYPE_LONGLONG");
});

test("MySqlSigFieldType tagName coverage and unsigned Null normalization", () => {
    expect(MySqlSigFieldType.Decimal.tagName()).toBe("MYSQL_TYPE_DECIMAL");
    expect(MySqlSigFieldType.Tiny.tagName()).toBe("MYSQL_TYPE_TINY");
    expect(MySqlSigFieldType.Short.tagName()).toBe("MYSQL_TYPE_SHORT");
    expect(MySqlSigFieldType.Float.tagName()).toBe("MYSQL_TYPE_FLOAT");
    expect(MySqlSigFieldType.Timestamp.tagName()).toBe("MYSQL_TYPE_TIMESTAMP");
    expect(MySqlSigFieldType.Date.tagName()).toBe("MYSQL_TYPE_DATE");
    expect(MySqlSigFieldType.Time.tagName()).toBe("MYSQL_TYPE_TIME");
    expect(MySqlSigFieldType.DateTime.tagName()).toBe("MYSQL_TYPE_DATETIME");
    expect(MySqlSigFieldType.Json.tagName()).toBe("MYSQL_TYPE_JSON");
    expect(MySqlSigFieldType.NewDecimal.tagName()).toBe("MYSQL_TYPE_NEWDECIMAL");
    expect(MySqlSigFieldType.Blob.tagName()).toBe("MYSQL_TYPE_BLOB");
    expect(MySqlSigFieldType.String.tagName()).toBe("MYSQL_TYPE_STRING");

    // unsigned(Null) normalizes to signed Null with ".null" suffix
    const norm = MySqlSigParam.unsigned(MySqlSigFieldType.Null);
    expect(norm.is_unsigned).toBe(false);
    expect(norm.wireFlagByte()).toBe(0x00);

    const arr: [1]MySqlSigParam = [norm];
    const sig = MySqlStmtSignature.build("SELECT ?", arr[0..])?;
    expect(sig.nameSlice()).toBe("SELECT ?.null");
});

test("MySqlStmtSignature empty constructor and zero-parameter query", () => {
    const empty = MySqlStmtSignature.empty();
    expect(empty.isEmpty()).toBe(true);
    expect(empty.fieldCount()).toBe(0);
    expect(empty.nameSlice()).toBe("");
    expect(empty.querySlice()).toBe("");

    const no_params: [0]MySqlSigParam = [];
    const sig = MySqlStmtSignature.build("SELECT 1", no_params[0..])?;
    expect(sig.isEmpty()).toBe(false);
    expect(sig.fieldCount()).toBe(0);
    expect(sig.nameSlice()).toBe("SELECT 1");
    expect(sig.querySlice()).toBe("SELECT 1");
});

test("MySqlStmtSignature enforces TooManyParams and SignatureTooLong guards", () => {
    expect(MYSQL_STMT_SIG_MAX_PARAMS).toBe(32);
    expect(MYSQL_STMT_SIG_MAX_NAME_LEN).toBe(512);

    let too_many: [33]MySqlSigParam = [MySqlSigParam.signed(MySqlSigFieldType.Long); 33];
    let got_too_many = false;
    * = MySqlStmtSignature.build("SELECT 1", too_many[0..]) catch |e| {
        if (e == MySqlStmtSignatureError.TooManyParams) {
            got_too_many = true;
        }
        MySqlStmtSignature.empty()
    };
    expect(got_too_many).toBe(true);

    // 505-byte query + "MYSQL_TYPE_VAR_STRING" (21 bytes) > 512 bytes
    let long_q: [505]u8 = ['a'; 505];
    const one_param: [1]MySqlSigParam = [MySqlSigParam.signed(MySqlSigFieldType.VarString)];
    let got_too_long = false;
    * = MySqlStmtSignature.build(long_q[0..], one_param[0..]) catch |e| {
        if (e == MySqlStmtSignatureError.SignatureTooLong) {
            got_too_long = true;
        }
        MySqlStmtSignature.empty()
    };
    expect(got_too_long).toBe(true);
});
