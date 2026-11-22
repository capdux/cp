import { test, expect } from "cap:test";
import {
    PG_STMT_SIG_MAX_PARAMS,
    PG_STMT_SIG_MAX_NAME_LEN,
    PG_STMT_SIG_PREFIX_MAX,
    PgStmtSignatureError,
    PgParamTypeTag,
    PgStmtSignature,
} from "cap:pgwire";

test("PgStmtSignature builds canonical signature key and wire OIDs across all PgParamTypeTag variants", () => {
    const tags: [14]PgParamTypeTag = [
        PgParamTypeTag.Null,
        PgParamTypeTag.Bool,
        PgParamTypeTag.Bytea,
        PgParamTypeTag.Int8,
        PgParamTypeTag.Int2,
        PgParamTypeTag.Int4,
        PgParamTypeTag.Text,
        PgParamTypeTag.Json,
        PgParamTypeTag.Float4,
        PgParamTypeTag.Float8,
        PgParamTypeTag.Timestamp,
        PgParamTypeTag.Timestamptz,
        PgParamTypeTag.Numeric,
        PgParamTypeTag.Jsonb,
    ];

    const sig = PgStmtSignature.build("Q", tags[0..], 42, false)?;
    expect(sig.querySlice()).toBe("Q");
    expect(sig.paramCount()).toBe(14);
    expect(sig.nameSlice()).toBe(
        "Q.null.bool.bytea.int8.int2.int4.string.json.float4.float8.timestamp.timestamptz.numeric.json",
    );

    const oids = sig.paramOids();
    expect(oids.len).toBe(14);
    expect(oids[0]).toBe(0);    // Null -> 0
    expect(oids[1]).toBe(16);   // Bool -> 16
    expect(oids[2]).toBe(17);   // Bytea -> 17
    expect(oids[3]).toBe(20);   // Int8 -> 20
    expect(oids[4]).toBe(21);   // Int2 -> 21
    expect(oids[5]).toBe(23);   // Int4 -> 23
    expect(oids[6]).toBe(0);    // Text -> 0
    expect(oids[7]).toBe(0);    // Json -> 0
    expect(oids[8]).toBe(700);  // Float4 -> 700
    expect(oids[9]).toBe(701);  // Float8 -> 701
    expect(oids[10]).toBe(0);   // Timestamp -> 0
    expect(oids[11]).toBe(0);   // Timestamptz -> 0
    expect(oids[12]).toBe(1700);// Numeric -> 1700
    expect(oids[13]).toBe(0);   // Jsonb -> 0
});

test("PgStmtSignature prepared statement naming, 40-byte prefix cap, and unnamed mode", () => {
    expect(PG_STMT_SIG_PREFIX_MAX).toBe(40);

    // Short query: full name used as prefix
    const t1: [1]PgParamTypeTag = [PgParamTypeTag.Int4];
    const short_sig = PgStmtSignature.build("SELECT $1", t1[0..], 0, false)?;
    expect(short_sig.isUnnamed()).toBe(false);
    expect(short_sig.preparedNameSlice()).toBe("PSELECT $1.int4$0");

    // Long query (> 40 bytes): prefix capped at 40 bytes
    const long_q = "SELECT id, email, created_at FROM accounts WHERE id = $1 AND active = $2";
    const t2: [2]PgParamTypeTag = [PgParamTypeTag.Int8, PgParamTypeTag.Bool];
    const long_sig = PgStmtSignature.build(long_q, t2[0..], 9876543210, false)?;
    expect(long_sig.preparedNameSlice()).toBe(
        "PSELECT id, email, created_at FROM accoun$9876543210",
    );

    // Unnamed statement leaves preparedNameSlice empty
    const unnamed_sig = PgStmtSignature.build("SELECT $1", t1[0..], 99, true)?;
    expect(unnamed_sig.isUnnamed()).toBe(true);
    expect(unnamed_sig.preparedNameSlice()).toBe("");
    expect(unnamed_sig.nameSlice()).toBe("SELECT $1.int4");
});

test("PgStmtSignature deterministic hash and equality for statement cache lookup", () => {
    const t_a: [2]PgParamTypeTag = [PgParamTypeTag.Int4, PgParamTypeTag.Text];
    const t_b: [2]PgParamTypeTag = [PgParamTypeTag.Int8, PgParamTypeTag.Text];

    const sig1 = PgStmtSignature.build("SELECT * FROM t WHERE a=$1 AND b=$2", t_a[0..], 1, false)?;
    const sig2 = PgStmtSignature.build("SELECT * FROM t WHERE a=$1 AND b=$2", t_a[0..], 999, true)?;
    const sig3 = PgStmtSignature.build("SELECT * FROM t WHERE a=$1 AND b=$2", t_b[0..], 1, false)?;

    // Different prepared_statement_id / unnamed flag still match as the same cache key
    expect(sig1.eql(sig2)).toBe(true);
    expect(sig1.hash()).toBe(sig2.hash());

    // Different parameter types produce distinct cache keys and hashes
    expect(sig1.eql(sig3)).toBe(false);
    expect(sig1.hash() != sig3.hash()).toBe(true);

    // Json and Jsonb map to the same ".json" suffix and OID 0 (matching Bun)
    const t_json: [1]PgParamTypeTag = [PgParamTypeTag.Json];
    const t_jsonb: [1]PgParamTypeTag = [PgParamTypeTag.Jsonb];
    const sj = PgStmtSignature.build("SELECT $1::jsonb", t_json[0..], 1, false)?;
    const sjb = PgStmtSignature.build("SELECT $1::jsonb", t_jsonb[0..], 2, false)?;
    expect(sj.eql(sjb)).toBe(true);
    expect(sj.hash()).toBe(sjb.hash());
});

test("PgStmtSignature empty constructor and zero-parameter query", () => {
    const empty = PgStmtSignature.empty();
    expect(empty.isUnnamed()).toBe(true);
    expect(empty.paramCount()).toBe(0);
    expect(empty.nameSlice()).toBe("");
    expect(empty.querySlice()).toBe("");
    expect(empty.preparedNameSlice()).toBe("");

    const no_params: [0]PgParamTypeTag = [];
    const s = PgStmtSignature.build("SELECT 1", no_params[0..], 5, false)?;
    expect(s.paramCount()).toBe(0);
    expect(s.nameSlice()).toBe("SELECT 1");
    expect(s.querySlice()).toBe("SELECT 1");
    expect(s.preparedNameSlice()).toBe("PSELECT 1$5");
});

test("PgStmtSignature enforces TooManyParams and SignatureTooLong guards", () => {
    expect(PG_STMT_SIG_MAX_PARAMS).toBe(32);
    expect(PG_STMT_SIG_MAX_NAME_LEN).toBe(512);

    // 33 parameters exceeds PG_STMT_SIG_MAX_PARAMS (32)
    let too_many: [33]PgParamTypeTag = [PgParamTypeTag.Int4; 33];
    let got_too_many = false;
    * = PgStmtSignature.build("SELECT 1", too_many[0..], 1, false) catch |e| {
        if (e == PgStmtSignatureError.TooManyParams) {
            got_too_many = true;
        }
        PgStmtSignature.empty()
    };
    expect(got_too_many).toBe(true);

    // Query > 512 bytes exceeds PG_STMT_SIG_MAX_NAME_LEN
    let huge_query: [520]u8 = ['x'; 520];
    const empty_params: [0]PgParamTypeTag = [];
    let got_too_long = false;
    * = PgStmtSignature.build(huge_query[0..], empty_params[0..], 1, false) catch |e| {
        if (e == PgStmtSignatureError.SignatureTooLong) {
            got_too_long = true;
        }
        PgStmtSignature.empty()
    };
    expect(got_too_long).toBe(true);
});
