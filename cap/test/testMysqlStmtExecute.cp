import { test, expect } from "cap:test";
import {
    STMT_EXECUTE_CMD,
    STMT_EXECUTE_HEADER_LEN,
    STMT_EXECUTE_DEFAULT_ITERATION_COUNT,
    STMT_EXECUTE_MAX_PARAMS,
    STMT_PARAM_UNSIGNED_FLAG,
    STMT_NEW_PARAMS_BIND_NONE,
    STMT_NEW_PARAMS_BIND_YES,
    STMT_CURSOR_TYPE_NO_CURSOR,
    STMT_CURSOR_TYPE_READ_ONLY,
    STMT_CURSOR_TYPE_FOR_UPDATE,
    STMT_CURSOR_TYPE_SCROLLABLE,
    STMT_PARAMETER_COUNT_AVAILABLE,
    STMT_FTYPE_TINY,
    STMT_FTYPE_LONG,
    STMT_FTYPE_DOUBLE,
    STMT_FTYPE_NULL,
    STMT_FTYPE_LONGLONG,
    STMT_FTYPE_BLOB,
    STMT_FTYPE_VAR_STRING,
    StmtExecuteError,
    StmtCursorType,
    isStmtBinaryFixedType,
    stmtNullBitmapLen,
    isStmtNullInBitmap,
    StmtExecParam,
    StmtExecuteHeader,
    StmtExecutePacket,
} from "cap:mysql";

test("zero-param COM_STMT_EXECUTE frame roundtrip & cursor flags", () => {
    let pkt = StmtExecutePacket.initWithFlags(
        0x11223344,
        STMT_CURSOR_TYPE_READ_ONLY | STMT_PARAMETER_COUNT_AVAILABLE,
        true,
    );
    expect(pkt.nullBitmapLen()).toBe(0);
    expect(pkt.payloadLen()).toBe(STMT_EXECUTE_HEADER_LEN);
    expect(pkt.frameLen()).toBe(14);

    let buf: [32]u8 = [0; 32];
    const n = pkt.encodeFrame(3, buf[0..])?;
    expect(n).toBe(14);
    expect(buf[0]).toBe(10);
    expect(buf[1]).toBe(0);
    expect(buf[2]).toBe(0);
    expect(buf[3]).toBe(3);
    expect(buf[4]).toBe(STMT_EXECUTE_CMD);
    expect(buf[5]).toBe(0x44);
    expect(buf[6]).toBe(0x33);
    expect(buf[7]).toBe(0x22);
    expect(buf[8]).toBe(0x11);
    expect(buf[9]).toBe(STMT_CURSOR_TYPE_READ_ONLY | STMT_PARAMETER_COUNT_AVAILABLE);
    expect(buf[10]).toBe(1);

    const hdr = StmtExecutePacket.decodeFrameHeader(buf[0..n], 0)?;
    expect(hdr.statement_id).toBe(0x11223344);
    expect(hdr.iteration_count).toBe(STMT_EXECUTE_DEFAULT_ITERATION_COUNT);
    expect(hdr.sequence_id).toBe(3);
    expect(hdr.param_count).toBe(0);
    expect(hdr.cursorType() == StmtCursorType.ReadOnly).toBe(true);
    expect(hdr.hasParamCountAvailable()).toBe(true);
    expect(hdr.isParamNull(0)).toBe(false);

    expect(StmtCursorType.NoCursor.toByte()).toBe(STMT_CURSOR_TYPE_NO_CURSOR);
    expect(StmtCursorType.ForUpdate.toByte()).toBe(STMT_CURSOR_TYPE_FOR_UPDATE);
    expect(StmtCursorType.Scrollable.toByte()).toBe(STMT_CURSOR_TYPE_SCROLLABLE);
    expect(StmtCursorType.fromFlags(STMT_CURSOR_TYPE_FOR_UPDATE) == StmtCursorType.ForUpdate).toBe(true);
    expect(StmtCursorType.fromFlags(STMT_CURSOR_TYPE_SCROLLABLE) == StmtCursorType.Scrollable).toBe(true);
});

test("mixed typed parameters wire layout & NULL-bitmap verification", () => {
    let pkt = StmtExecutePacket.init(42);
    * = pkt.addParam(StmtExecParam.uint32(0x04030201))?;
    * = pkt.addParam(StmtExecParam.nullParam())?;
    * = pkt.addParam(StmtExecParam.varString("hello"))?;
    * = pkt.addParam(StmtExecParam.int8(-5))?;
    * = pkt.addParam(StmtExecParam.blob("AB"))?;

    expect(pkt.param_count).toBe(5);
    expect(pkt.nullBitmapLen()).toBe(1);

    let buf: [128]u8 = [0; 128];
    const n = pkt.encodeFrame(0, buf[0..])?;

    const hdr = StmtExecutePacket.decodeFrameHeader(buf[0..n], 5)?;
    expect(hdr.statement_id).toBe(42);
    expect(hdr.new_params_bind_flag).toBe(true);
    expect(hdr.null_bitmap.len).toBe(1);
    expect(hdr.null_bitmap[0]).toBe(0x02); // bit 1 set
    expect(hdr.isParamNull(0)).toBe(false);
    expect(hdr.isParamNull(1)).toBe(true);
    expect(hdr.isParamNull(2)).toBe(false);
    expect(hdr.isParamNull(3)).toBe(false);
    expect(hdr.isParamNull(4)).toBe(false);

    // Check parameter types array (starts at payload offset 12 => frame offset 16)
    const types_off = 4 + hdr.params_offset;
    expect(buf[types_off]).toBe(STMT_FTYPE_LONG);
    expect(buf[types_off + 1]).toBe(STMT_PARAM_UNSIGNED_FLAG);
    expect(buf[types_off + 2]).toBe(STMT_FTYPE_NULL);
    expect(buf[types_off + 3]).toBe(0);
    expect(buf[types_off + 4]).toBe(STMT_FTYPE_VAR_STRING);
    expect(buf[types_off + 5]).toBe(0);
    expect(buf[types_off + 6]).toBe(STMT_FTYPE_TINY);
    expect(buf[types_off + 7]).toBe(0);
    expect(buf[types_off + 8]).toBe(STMT_FTYPE_BLOB);
    expect(buf[types_off + 9]).toBe(0);

    // Check serialized values right after 5*2=10 type bytes
    const val_off = types_off + 10;
    // Param 0: uint32 LE 0x04030201
    expect(buf[val_off]).toBe(0x01);
    expect(buf[val_off + 1]).toBe(0x02);
    expect(buf[val_off + 2]).toBe(0x03);
    expect(buf[val_off + 3]).toBe(0x04);
    // Param 1: NULL -> 0 bytes
    // Param 2: lenenc(5) + "hello"
    expect(buf[val_off + 4]).toBe(5);
    expect(buf[val_off + 5]).toBe('h');
    expect(buf[val_off + 9]).toBe('o');
    // Param 3: int8(-5) => 0xFB
    expect(buf[val_off + 10]).toBe(0xFB);
    // Param 4: lenenc(2) + "AB"
    expect(buf[val_off + 11]).toBe(2);
    expect(buf[val_off + 12]).toBe('A');
    expect(buf[val_off + 13]).toBe('B');
    expect(n).toBe(val_off + 14);
});

test("multi-byte NULL-bitmap across 10 parameters", () => {
    expect(stmtNullBitmapLen(0)).toBe(0);
    expect(stmtNullBitmapLen(1)).toBe(1);
    expect(stmtNullBitmapLen(8)).toBe(1);
    expect(stmtNullBitmapLen(9)).toBe(2);
    expect(stmtNullBitmapLen(16)).toBe(2);

    let pkt = StmtExecutePacket.init(99);
    let i: usize = 0;
    while (i < 10) {
        if (i == 0 || i == 7 || i == 8 || i == 9) {
            * = pkt.addParam(StmtExecParam.nullParam())?;
        } else {
            * = pkt.addParam(StmtExecParam.int32(@as(i32, @intCast(i))))?;
        }
        i += 1;
    }

    let bm: [4]u8 = [0; 4];
    const bm_n = pkt.writeNullBitmap(bm[0..])?;
    expect(bm_n).toBe(2);
    expect(bm[0]).toBe(0x81); // bits 0 and 7
    expect(bm[1]).toBe(0x03); // bits 8 and 9
    expect(isStmtNullInBitmap(bm[0..bm_n], 0)).toBe(true);
    expect(isStmtNullInBitmap(bm[0..bm_n], 1)).toBe(false);
    expect(isStmtNullInBitmap(bm[0..bm_n], 7)).toBe(true);
    expect(isStmtNullInBitmap(bm[0..bm_n], 8)).toBe(true);
    expect(isStmtNullInBitmap(bm[0..bm_n], 9)).toBe(true);
    expect(isStmtNullInBitmap(bm[0..bm_n], 16)).toBe(false);
});

test("re-execute mode with new_params_bind_flag = false omits type pairs", () => {
    let pkt = StmtExecutePacket.initWithFlags(5, STMT_CURSOR_TYPE_NO_CURSOR, false);
    * = pkt.addParam(StmtExecParam.int64(123456789))?;
    * = pkt.addParam(StmtExecParam.uint64(987654321))?;
    * = pkt.addParam(StmtExecParam.float64(3.5))?;
    * = pkt.addParam(StmtExecParam.uint8(255))?;

    // Header(10) + bitmap(1) + bind_flag(1) + values(8+8+8+1 = 25) = 37
    expect(pkt.payloadLen()).toBe(37);

    let buf: [64]u8 = [0; 64];
    const n = pkt.encodeFrame(1, buf[0..])?;
    expect(n).toBe(41);

    const hdr = StmtExecutePacket.decodeFrameHeader(buf[0..n], 4)?;
    expect(hdr.new_params_bind_flag).toBe(false);
    expect(hdr.params_offset).toBe(12);
    // Last byte is uint8(255)
    expect(buf[n - 1]).toBe(255);
});

test("binary fixed type predicate & custom fixedBinary param", () => {
    expect(isStmtBinaryFixedType(STMT_FTYPE_TINY)).toBe(true);
    expect(isStmtBinaryFixedType(STMT_FTYPE_LONG)).toBe(true);
    expect(isStmtBinaryFixedType(STMT_FTYPE_DOUBLE)).toBe(true);
    expect(isStmtBinaryFixedType(STMT_FTYPE_LONGLONG)).toBe(true);
    expect(isStmtBinaryFixedType(STMT_FTYPE_VAR_STRING)).toBe(false);
    expect(isStmtBinaryFixedType(STMT_FTYPE_BLOB)).toBe(false);
    expect(isStmtBinaryFixedType(STMT_FTYPE_NULL)).toBe(false);

    const raw_short: [2]u8 = [0x34, 0x12];
    let pkt = StmtExecutePacket.init(11);
    * = pkt.addParam(StmtExecParam.fixedBinary(0x02, true, raw_short[0..]))?;

    let buf: [64]u8 = [0; 64];
    const n = pkt.encodePayload(buf[0..])?;
    // 10 header + 1 bitmap + 1 bind + 2 type + 2 raw = 16
    expect(n).toBe(16);
    expect(buf[14]).toBe(0x34);
    expect(buf[15]).toBe(0x12);
});

test("error guards: BufferTooSmall, TooManyParams, Truncated, InvalidCommand, InvalidBindFlag", () => {
    // TooManyParams (> 64)
    let full_pkt = StmtExecutePacket.init(1);
    let i: usize = 0;
    while (i < STMT_EXECUTE_MAX_PARAMS) {
        * = full_pkt.addParam(StmtExecParam.uint8(1))?;
        i += 1;
    }
    const over_err = full_pkt.addParam(StmtExecParam.uint8(2)) catch |e| {
        expect(e == StmtExecuteError.TooManyParams).toBe(true);
        return;
    };
    * = over_err;
    expect(false).toBe(true);
});

test("decode error guards: BufferTooSmall, Truncated, InvalidCommand, InvalidBindFlag", () => {
    let pkt = StmtExecutePacket.init(1);
    * = pkt.addParam(StmtExecParam.int32(7))?;

    let tiny: [8]u8 = [0; 8];
    let got_buf_small = false;
    * = pkt.encodeFrame(0, tiny[0..]) catch |e| {
        got_buf_small = (e == StmtExecuteError.BufferTooSmall);
        0
    };
    expect(got_buf_small).toBe(true);

    // Truncated payload (< 10 bytes)
    const short_in: [5]u8 = [STMT_EXECUTE_CMD, 1, 0, 0, 0];
    let got_trunc = false;
    const trunc_res = StmtExecutePacket.decodePayloadHeader(short_in[0..], 0);
    * = trunc_res catch |e| {
        got_trunc = (e == StmtExecuteError.Truncated);
        StmtExecuteHeader{}
    };
    expect(got_trunc).toBe(true);

    // InvalidCommand
    let bad_cmd: [10]u8 = [0x03, 1, 0, 0, 0, 0, 1, 0, 0, 0];
    let got_bad_cmd = false;
    const cmd_res = StmtExecutePacket.decodePayloadHeader(bad_cmd[0..], 0);
    * = cmd_res catch |e| {
        got_bad_cmd = (e == StmtExecuteError.InvalidCommand);
        StmtExecuteHeader{}
    };
    expect(got_bad_cmd).toBe(true);

    // InvalidBindFlag (0x02)
    let bad_bind: [12]u8 = [STMT_EXECUTE_CMD, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0x02];
    let got_bad_bind = false;
    const bind_res = StmtExecutePacket.decodePayloadHeader(bad_bind[0..], 1);
    * = bind_res catch |e| {
        got_bad_bind = (e == StmtExecuteError.InvalidBindFlag);
        StmtExecuteHeader{}
    };
    expect(got_bad_bind).toBe(true);
});
