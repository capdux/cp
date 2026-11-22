import {
    TAG_PARAMETER_DESCRIPTION,
    PARAM_DESC_MAX_PARAMS,
    ParamDescError,
    isBinaryFriendlyParamOid,
    ParameterDescriptionView,
    ParameterDescription,
} from "cap:pg";
import { expect, test } from "cap:test";

test("pg paramDesc: encodeFrame and decodeFrame roundtrip with multiple OIDs", fn () {
    let pd = ParameterDescription.init();
    * = pd.push(23) catch 0;   // INT4
    * = pd.push(25) catch 0;   // TEXT
    * = pd.push(2950) catch 0; // UUID
    * = pd.push(3802) catch 0; // JSONB
    expect(pd.len).toBe(4);
    expect(pd.encodedFrameLen()).toBe(1 + 4 + 2 + 16);

    let wire: [64]u8 = [0; 64];
    const n = pd.encodeFrame(wire[0..]) catch 0;
    expect(n).toBe(23);
    expect(wire[0]).toBe(TAG_PARAMETER_DESCRIPTION);
    // BE length = 4 + 2 + 16 = 22
    expect(wire[1]).toBe(0);
    expect(wire[2]).toBe(0);
    expect(wire[3]).toBe(0);
    expect(wire[4]).toBe(22);
    // BE count = 4
    expect(wire[5]).toBe(0);
    expect(wire[6]).toBe(4);

    const view = ParameterDescriptionView.decodeFrame(wire[0..n]) catch ParameterDescriptionView{};
    expect(view.count).toBe(4);
    expect(view.getOid(0) ?? 0).toBe(23);
    expect(view.getOid(1) ?? 0).toBe(25);
    expect(view.getOid(2) ?? 0).toBe(2950);
    expect(view.getOid(3) ?? 0).toBe(3802);
    expect(view.getOid(4) == null).toBe(true);
    expect(view.hasUnspecifiedTypes()).toBe(false);
    expect(view.allBinaryFriendly()).toBe(true);

    const decoded = ParameterDescription.decodeFrame(wire[0..n]) catch ParameterDescription.init();
    expect(decoded.eql(pd)).toBe(true);
    expect(decoded.slice().len).toBe(4);
    expect(decoded.get(2) ?? 0).toBe(2950);
});

test("pg paramDesc: zero-parameter statement description", fn () {
    const pd = ParameterDescription.init();
    expect(pd.encodedFrameLen()).toBe(7);

    let wire: [16]u8 = [0; 16];
    const n = pd.encodeFrame(wire[0..]) catch 0;
    expect(n).toBe(7);
    expect(wire[0]).toBe('t');
    expect(wire[4]).toBe(6); // length = 4 + 2
    expect(wire[5]).toBe(0);
    expect(wire[6]).toBe(0);

    const view = ParameterDescriptionView.decodeFrame(wire[0..n]) catch ParameterDescriptionView{};
    expect(view.count).toBe(0);
    expect(view.getOid(0) == null).toBe(true);
    expect(view.hasUnspecifiedTypes()).toBe(false);
    expect(view.allBinaryFriendly()).toBe(true);
});

test("pg paramDesc: unspecified OID (0) and custom non-binary OID classification", fn () {
    expect(isBinaryFriendlyParamOid(23)).toBe(true);
    expect(isBinaryFriendlyParamOid(2950)).toBe(true);
    expect(isBinaryFriendlyParamOid(0)).toBe(false);
    expect(isBinaryFriendlyParamOid(1700)).toBe(false); // NUMERIC

    let pd = ParameterDescription.init();
    * = pd.push(23) catch 0;
    * = pd.push(0) catch 0; // UNSPECIFIED
    let wire: [32]u8 = [0; 32];
    const n = pd.encodeFrame(wire[0..]) catch 0;

    const view = ParameterDescriptionView.decodeFrame(wire[0..n]) catch ParameterDescriptionView{};
    expect(view.hasUnspecifiedTypes()).toBe(true);
    expect(view.allBinaryFriendly()).toBe(false);
});

test("pg paramDesc: rejects invalid tag, negative count, length mismatch and truncation", fn () {
    // Wrong tag ('T' RowDescription instead of 't')
    const bad_tag: [7]u8 = ['T', 0, 0, 0, 6, 0, 0];
    let got_bad_tag = false;
    * = ParameterDescriptionView.decodeFrame(bad_tag[0..]) catch |e| {
        got_bad_tag = e == ParamDescError.InvalidTag;
    };
    expect(got_bad_tag).toBe(true);

    // Negative signed 16-bit parameter count (-1 = 0xFFFF)
    const neg_count: [7]u8 = ['t', 0, 0, 0, 6, 0xFF, 0xFF];
    let got_neg = false;
    * = ParameterDescriptionView.decodeFrame(neg_count[0..]) catch |e| {
        got_neg = e == ParamDescError.NegativeCount;
    };
    expect(got_neg).toBe(true);

    // Length < 6
    const bad_len: [5]u8 = ['t', 0, 0, 0, 4];
    let got_bad_len = false;
    * = ParameterDescriptionView.decodeFrame(bad_len[0..]) catch |e| {
        got_bad_len = e == ParamDescError.InvalidLength;
    };
    expect(got_bad_len).toBe(true);

    // Truncated frame (claims 1 parameter = 10B len, but only 8B provided)
    const trunc_frame: [8]u8 = ['t', 0, 0, 0, 10, 0, 1, 0];
    let got_trunc = false;
    * = ParameterDescriptionView.decodeFrame(trunc_frame[0..]) catch |e| {
        got_trunc = e == ParamDescError.Truncated;
    };
    expect(got_trunc).toBe(true);

    // Trailing extra bytes inside declared payload length (count=0 but payload=6B)
    const trailing_payload: [6]u8 = [0, 0, 0, 0, 0, 23];
    let got_mismatch = false;
    * = ParameterDescriptionView.decodePayload(trailing_payload[0..]) catch |e| {
        got_mismatch = e == ParamDescError.InvalidLength;
    };
    expect(got_mismatch).toBe(true);
});

test("pg paramDesc: capacity limit and encode buffer guard", fn () {
    let pd = ParameterDescription.init();
    let i: usize = 0;
    while (i < PARAM_DESC_MAX_PARAMS) {
        * = pd.push(23) catch 0;
        i += 1;
    }
    expect(pd.len).toBe(PARAM_DESC_MAX_PARAMS);

    // 257th push rejected with TooManyParameters
    let got_too_many = false;
    * = pd.push(25) catch |e| {
        got_too_many = e == ParamDescError.TooManyParameters;
    };
    expect(got_too_many).toBe(true);

    // Buffer too small on encodeFrame
    let tiny: [16]u8 = [0; 16];
    let got_small = false;
    * = pd.encodeFrame(tiny[0..]) catch |e| {
        got_small = e == ParamDescError.BufferTooSmall;
    };
    expect(got_small).toBe(true);
});

test("pg paramDesc: decodeFrame handles trailing subsequent wire messages in stream", fn () {
    // Frame 1: ParameterDescription with 1 OID (20 = INT8), total 11B
    // Followed immediately by ReadyForQuery ('Z', len=5, 'I') in the same stream buffer
    const stream: [17]u8 = [
        't', 0, 0, 0, 10, 0, 1, 0, 0, 0, 20,
        'Z', 0, 0, 0, 5, 'I',
    ];
    const pd = ParameterDescription.decodeFrame(stream[0..]) catch ParameterDescription.init();
    expect(pd.len).toBe(1);
    expect(pd.get(0) ?? 0).toBe(20);
    expect(pd.encodedFrameLen()).toBe(11);
});
