// cap/test/test_phase_c2_http_interchange_port.cp
// Phase C-2 Industrial Port Verification Suite:
// 1. 200+ MIME database, RFC 9110 HTTPStatus, HeaderBuilder, CacheControl/ContentDisposition/Accept/Range/ContentRange,
//    and HttpRequestBuilder / HttpResponseParser (100-Continue, obs-fold, chunked+trailers, gzip auto-decompression)
// 2. Complete RFC 7541 HPACK (61-entry Static Table, 257-entry Canonical Huffman, Dynamic Table FIFO & Size Update)
//    & HTTP/2 Flow Control + Control/Continuation Frame State Machine
// 3. Industrial YAML 1.2 (multi-doc, anchors/aliases, merge keys `<<`, block scalars `|`/`>`, flow collections, tags, escapes, stringify)
// 4. Industrial TOML 1.0 (inline tables, dotted/quoted keys, RFC 3339 date/time + validation, nested arrays, `[[table_arrays]]`, stringify)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Method,
    Headers,
    Request,
    Response,
    json,
    Encoding,
    FetchRedirect,
    MIME_DATABASE,
    MimeCategory,
    MimeType,
    HTTPStatus,
    HeaderBuilder,
    CacheControl,
    ContentDisposition,
    DispositionType,
    AcceptItem,
    AcceptParser,
    RangeHeader,
    ContentRange,
    HttpRequestBuilder,
    HttpResponseParser,
    HttpConnectionPool,
    RedirectPolicy,
    RedirectAction,
    HPACK_STATIC_TABLE,
    HPACK_HUFFMAN_TABLE,
    HpackCodec,
    HpackIndexMode,
    HpackDynamicTable,
    HpackSessionCodec,
    CLIENT_PREFACE,
    FrameType,
    FrameHeader,
    ErrorCode,
    SettingsType,
    SettingsPayloadUnit,
    HeadersFrameFlags,
    H2FrameActionKind,
    H2ClientSession,
} from "cap:http";
import { gzip } from "../../lib/pack/gzip.cp";
import {
    ConfTag,
    ConfDateTimeKind,
    ConfDateTime,
    ConfValue,
    YamlParser,
    YAML,
    TomlParser,
    TOML,
} from "cap:conf";

fn testMimeStatusHeaderToolsAndWireHttp(): !void {
    const alloc = std.heap.page_allocator;

    // 1. 150+ MIME Database, bidirectional lookup, compression/text/charset & magic sniff
    expect_eq(MIME_DATABASE.len >= 150, true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension("html"), "text/html; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension(".json"), "application/json; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension("avif"), "image/avif"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromExtension("parquet"), "application/vnd.apache.parquet"), true);
    expect_eq(std.mem.eql(u8, MimeType.fromPath("/docs/spec.yaml"), "text/yaml; charset=utf-8"), true);

    expect_eq(std.mem.eql(u8, MimeType.extensionForMime("image/png").?, "png"), true);
    expect_eq(std.mem.eql(u8, MimeType.extensionForMime("application/json; charset=utf-8").?, "json"), true);
    expect_eq(MimeType.isCompressible("application/javascript"), true);
    expect_eq(MimeType.isCompressible("png"), false);
    expect_eq(MimeType.isText("svg"), true);
    expect_eq(MimeType.isText("wasm"), false);
    expect_eq(MimeType.isUtf8Charset("text/css"), true);
    expect_eq(MimeType.isUtf8Charset("application/octet-stream"), false);
    expect_eq(MimeType.categoryOf("woff2") == MimeCategory.Font, true);
    expect_eq(MimeType.sniffMagic("\x89PNG\r\n\x1A\nabc").?.category == MimeCategory.Image, true);
    expect_eq(std.mem.eql(u8, MimeType.sniffMagic("\x00asm\x01\x00\x00\x00").?.ext, "wasm"), true);
    expect_eq(std.mem.eql(u8, MimeType.sniffMagic("%PDF-1.7\n").?.ext, "pdf"), true);

    // 2. RFC 9110 HTTPStatus
    expect_eq(std.mem.eql(u8, HTTPStatus.reasonPhrase(200), "OK"), true);
    expect_eq(std.mem.eql(u8, HTTPStatus.reasonPhrase(418), "I'm a teapot"), true);
    expect_eq(std.mem.eql(u8, HTTPStatus.reasonPhrase(429), "Too Many Requests"), true);
    expect_eq(std.mem.eql(u8, HTTPStatus.reasonPhrase(503), "Service Unavailable"), true);
    expect_eq(HTTPStatus.isSuccess(204), true);
    expect_eq(HTTPStatus.isRedirection(308), true);
    expect_eq(HTTPStatus.isClientError(404), true);
    expect_eq(HTTPStatus.isServerError(502), true);
    expect_eq(HTTPStatus.allowsBody(204), false);
    expect_eq(HTTPStatus.allowsBody(304), false);
    expect_eq(HTTPStatus.allowsBody(200), true);
    expect_eq(HTTPStatus.isCacheableByDefault(200), true);
    expect_eq(HTTPStatus.isCacheableByDefault(500), false);

    // 3. HeaderBuilder zero-alloc wire serialization
    let hb_buf: [512]u8 = undefined;
    let hb = HeaderBuilder.init(&hb_buf);
    expect_eq(hb.writeRequestLine("POST", "/v2/upload"), true);
    expect_eq(hb.append("Host", "api.cp.dev"), true);
    expect_eq(hb.appendInt("Content-Length", 128), true);
    expect_eq(std.mem.eql(u8, hb.get("host").?, "api.cp.dev"), true);
    expect_eq(std.mem.eql(u8, hb.get("content-length").?, "128"), true);
    const built = try hb.finish();
    expect_eq(std.mem.startsWith(u8, built, "POST /v2/upload HTTP/1.1\r\nHost: api.cp.dev\r\nContent-Length: 128\r\n\r\n"), true);

    // 4. CacheControl, ContentDisposition, AcceptParser, RangeHeader, ContentRange
    const cc = CacheControl.parse("public, max-age=3600, s-maxage=7200, stale-while-revalidate=60, must-revalidate, immutable");
    expect_eq(cc.is_public, true);
    expect_eq(cc.max_age.?, 3600);
    expect_eq(cc.s_maxage.?, 7200);
    expect_eq(cc.stale_while_revalidate.?, 60);
    expect_eq(cc.must_revalidate, true);
    expect_eq(cc.immutable, true);
    let cc_out: [128]u8 = undefined;
    const cc_str = try cc.format(&cc_out);
    expect_eq(std.mem.indexOf(u8, cc_str, "max-age=3600") != null, true);
    expect_eq(std.mem.indexOf(u8, cc_str, "immutable") != null, true);

    const cd = ContentDisposition.parse("attachment; filename=\"report 2026.pdf\"; filename*=UTF-8''report%202026.pdf");
    expect_eq(cd.disposition == DispositionType.Attachment, true);
    expect_eq(std.mem.eql(u8, cd.filename.?, "report 2026.pdf"), true);
    expect_eq(std.mem.eql(u8, cd.filename_ext.?, "report%202026.pdf"), true);
    expect_eq(std.mem.eql(u8, cd.effectiveFilename().?, "report%202026.pdf"), true);

    let accept_items: [8]AcceptItem = undefined;
    const acnt = AcceptParser.parse("text/html;q=0.8, application/json, */*;q=0.1", &accept_items);
    expect_eq(acnt, 3);
    expect_eq(std.mem.eql(u8, accept_items[0].media_range, "application/json"), true);
    const sup_mimes: [2][]const u8 = ["text/html", "application/json"];
    expect_eq(std.mem.eql(u8, AcceptParser.negotiate("text/html;q=0.7, application/json;q=0.9", &sup_mimes).?, "application/json"), true);
    expect_eq(std.mem.eql(u8, AcceptParser.negotiate("application/json;q=0, */*;q=0.5", &sup_mimes).?, "text/html"), true);

    const rh = RangeHeader.parse("bytes=0-499, 1000-, -200");
    expect_eq(rh.is_valid, true);
    expect_eq(rh.count, 3);
    const r0 = rh.ranges[0].resolve(2000).?;
    expect_eq(r0.start, 0);
    expect_eq(r0.end, 499);
    expect_eq(r0.length(), 500);
    const r1 = rh.ranges[1].resolve(2000).?;
    expect_eq(r1.start, 1000);
    expect_eq(r1.end, 1999);
    const r2 = rh.ranges[2].resolve(2000).?;
    expect_eq(r2.start, 1800);
    expect_eq(r2.end, 1999);

    const cr = ContentRange.parse("bytes 0-499/2000");
    expect_eq(cr.is_valid, true);
    expect_eq(cr.start.?, 0);
    expect_eq(cr.end.?, 499);
    expect_eq(cr.total_length.?, 2000);
    let cr_buf: [64]u8 = undefined;
    expect_eq(std.mem.eql(u8, try cr.format(&cr_buf), "bytes 0-499/2000"), true);

    // 5. HttpRequestBuilder (Expect: 100-continue) & HttpResponseParser (100-Continue + obs-fold + multi-value + chunked + gzip + pool/redirect)
    let reqb = HttpRequestBuilder.init(alloc, Method.POST, "https://api.cp.dev/v1/submit");
    defer reqb.deinit();
    reqb.setHeader("Authorization", "Bearer top-secret");
    reqb.setBody("payload-data");
    reqb.enableExpectContinue(true);

    let req_wire_buf: [1024]u8 = undefined;
    const head_wire = try reqb.buildWire(&req_wire_buf);
    expect_eq(std.mem.indexOf(u8, head_wire, "Expect: 100-continue\r\n") != null, true);
    expect_eq(std.mem.indexOf(u8, head_wire, "payload-data") == null, true); // withheld until 100 Continue

    const plain_payload = "CP HTTP/1.1 Gzipped + Chunked + 100-Continue + Obs-Fold Response!";
    const gz_payload = gzip(plain_payload, 6);
    let chunked_wire_buf: [512]u8 = undefined;
    const chunked_hdr = try std.fmt.bufPrint(&chunked_wire_buf, "{x}\r\n", .{gz_payload.len});
    let rpos: usize = chunked_hdr.len;
    @memcpy(chunked_wire_buf[rpos .. rpos + gz_payload.len], gz_payload);
    rpos += gz_payload.len;
    const chunk_tail = "\r\n0\r\nX-Checksum: crc32-ok\r\n\r\n";
    @memcpy(chunked_wire_buf[rpos .. rpos + chunk_tail.len], chunk_tail);
    rpos += chunk_tail.len;

    const resp_prefix =
        "HTTP/1.1 100 Continue\r\n\r\n" +
        "HTTP/1.1 200 OK\r\n" +
        "Content-Encoding: gzip\r\n" +
        "Transfer-Encoding: chunked\r\n" +
        "Set-Cookie: a=1\r\n" +
        "Set-Cookie: b=2\r\n" +
        "X-Folded: hello\r\n" +
        "   world\r\n\r\n";
    let full_resp_buf: [1024]u8 = undefined;
    @memcpy(full_resp_buf[0..resp_prefix.len], resp_prefix);
    @memcpy(full_resp_buf[resp_prefix.len .. resp_prefix.len + rpos], chunked_wire_buf[0..rpos]);

    let parsed_resp = try HttpResponseParser.parse(alloc, full_resp_buf[0 .. resp_prefix.len + rpos], true);
    defer parsed_resp.deinit();
    expect_eq(parsed_resp.saw_100_continue, true);
    expect_eq(parsed_resp.status, 200);
    expect_eq(parsed_resp.is_chunked, true);
    expect_eq(parsed_resp.content_encoding == Encoding.Gzip, true);
    expect_eq(std.mem.eql(u8, parsed_resp.getHeader("X-Folded").?, "hello world"), true);
    expect_eq(std.mem.eql(u8, parsed_resp.getTrailer("X-Checksum").?, "crc32-ok"), true);
    expect_eq(std.mem.eql(u8, parsed_resp.body, plain_payload), true);

    let cookies_out: [4][]const u8 = undefined;
    expect_eq(parsed_resp.getHeaderAll("Set-Cookie", &cookies_out), 2);
    expect_eq(std.mem.eql(u8, cookies_out[0], "a=1"), true);
    expect_eq(std.mem.eql(u8, cookies_out[1], "b=2"), true);

    // Pool + Redirect integration
    let pool = HttpConnectionPool.init(4, 16, 30000, 100);
    expect_eq(pool.addConnection(77, "api.cp.dev", 443, true, 1000), true);
    let policy = RedirectPolicy.init(FetchRedirect.Follow, 5);
    let url_out: [256]u8 = undefined;
    let redir_resp = try HttpResponseParser.parse(
        alloc,
        "HTTP/1.1 302 Found\r\nLocation: https://other.cp.dev/next\r\nContent-Length: 0\r\n\r\n",
        true,
    );
    defer redir_resp.deinit();
    const r_dec = HttpResponseParser.completeTurn(&pool, 77, "api.cp.dev", 443, true, 1200, redir_resp, &policy, Method.POST, "https://api.cp.dev/v1/submit", &url_out);
    expect_eq(r_dec.action == RedirectAction.Follow, true);
    expect_eq(reqb.applyRedirect(r_dec), true);
    expect_eq(reqb.method == Method.GET, true);
    expect_eq(reqb.headers.has("Authorization"), false);
    expect_eq(pool.idleCountForHost("api.cp.dev", 443, true), 1);
}

fn testHpackCompleteAndH2FlowControl(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Complete 61-entry Static Table & 257-entry Huffman Table
    expect_eq(HPACK_STATIC_TABLE.len, 61);
    expect_eq(HPACK_HUFFMAN_TABLE.len, 257);
    expect_eq(std.mem.eql(u8, HpackCodec.lookupStatic(1).?.name, ":authority"), true);
    expect_eq(std.mem.eql(u8, HpackCodec.lookupStatic(46).?.name, "location"), true);
    expect_eq(std.mem.eql(u8, HpackCodec.lookupStatic(61).?.name, "www-authenticate"), true);
    expect_eq(HpackCodec.findStaticExact(":status", "200"), 8);
    expect_eq(HpackCodec.findStaticName("content-type"), 31);

    // 2. RFC 7541 Appendix C Huffman vectors
    let hbuf: [64]u8 = undefined;
    const wlen = HpackCodec.encodeHuffman(&hbuf, "www.example.com");
    expect_eq(wlen, 12);
    const expected_www: [12]u8 = [0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff];
    expect_eq(std.mem.eql(u8, hbuf[0..12], &expected_www), true);

    let dec_buf: [64]u8 = undefined;
    const dlen = try HpackCodec.decodeHuffman(&dec_buf, hbuf[0..wlen]);
    expect_eq(std.mem.eql(u8, dec_buf[0..dlen], "www.example.com"), true);

    const nclen = HpackCodec.encodeHuffman(&hbuf, "no-cache");
    expect_eq(nclen, 6);
    const expected_nc: [6]u8 = [0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf];
    expect_eq(std.mem.eql(u8, hbuf[0..6], &expected_nc), true);
    expect_eq(std.mem.eql(u8, dec_buf[0 .. try HpackCodec.decodeHuffman(&dec_buf, hbuf[0..6])], "no-cache"), true);

    // Full 0..255 byte alphabet Huffman roundtrip
    let all_bytes: [256]u8 = undefined;
    let bi: usize = 0;
    while (bi < 256) : (bi += 1) {
        all_bytes[bi] = bi as u8;
    }
    let big_huff: [1024]u8 = undefined;
    let big_dec: [512]u8 = undefined;
    const bh_len = HpackCodec.encodeHuffman(&big_huff, &all_bytes);
    const bd_len = try HpackCodec.decodeHuffman(&big_dec, big_huff[0..bh_len]);
    expect_eq(bd_len, 256);
    expect_eq(std.mem.eql(u8, big_dec[0..bd_len], &all_bytes), true);

    // Invalid Huffman EOS / non-1 padding rejected
    let bad_huff = false;
    const bad_pad: [1]u8 = [0x00]; // 5-bit '0' + 3 zero-bits (invalid padding, must be 111)
    * = HpackCodec.decodeHuffman(&dec_buf, &bad_pad) catch {
        bad_huff = true;
    };
    expect_eq(bad_huff, true);

    // 3. Dynamic Table FIFO Eviction & Stateful Session Codec with Huffman
    let dt = HpackDynamicTable.init(alloc, 128);
    defer dt.deinit();
    // "custom-key" (10) + "custom-val" (10) + 32 = 52 bytes
    expect_eq(dt.insert("custom-key", "custom-val"), true);
    expect_eq(dt.current_size, 52);
    expect_eq(dt.insert("second-hdr", "second-val"), true); // 52 + 52 = 104 <= 128
    expect_eq(dt.count, 2);
    expect_eq(std.mem.eql(u8, dt.lookup(62).?.name, "second-hdr"), true);
    expect_eq(std.mem.eql(u8, dt.lookup(63).?.name, "custom-key"), true);
    // Third 52-byte entry triggers FIFO eviction of oldest ("custom-key")
    expect_eq(dt.insert("third-hdr1", "third-val1"), true);
    expect_eq(dt.count, 2);
    expect_eq(dt.evictions, 1);
    expect_eq(std.mem.eql(u8, dt.lookup(63).?.name, "second-hdr"), true);
    // Shrink max size triggers eviction
    expect_eq(dt.setMaxSize(60), true);
    expect_eq(dt.count, 1);
    expect_eq(std.mem.eql(u8, dt.lookup(62).?.name, "third-hdr1"), true);

    let sc = HpackSessionCodec.init(alloc, 4096);
    defer sc.deinit();
    let wire_block: [512]u8 = undefined;
    let wpos: usize = 0;
    wpos += sc.encodeTableSizeUpdate(wire_block[wpos..], 1024);
    wpos += sc.encodeHeader(wire_block[wpos..], "x-tenant-id", "acme-corp", HpackIndexMode.Incremental, true);
    // Second identical header should encode as a single indexed byte (> 61)!
    const second_start = wpos;
    wpos += sc.encodeHeader(wire_block[wpos..], "x-tenant-id", "acme-corp", HpackIndexMode.Incremental, true);
    expect_eq(wpos - second_start, 1);

    let n_scr: [128]u8 = undefined;
    let v_scr: [128]u8 = undefined;
    let rpos: usize = 0;
    const upd = sc.decodeHeader(wire_block[rpos..wpos], &n_scr, &v_scr).?;
    expect_eq(upd.name.len, 0);
    expect_eq(sc.dec_table.max_size, 1024);
    rpos += upd.bytes_read;

    const h1 = sc.decodeHeader(wire_block[rpos..wpos], &n_scr, &v_scr).?;
    expect_eq(std.mem.eql(u8, h1.name, "x-tenant-id"), true);
    expect_eq(std.mem.eql(u8, h1.value, "acme-corp"), true);
    rpos += h1.bytes_read;

    const h2 = sc.decodeHeader(wire_block[rpos..wpos], &n_scr, &v_scr).?;
    expect_eq(std.mem.eql(u8, h2.name, "x-tenant-id"), true);
    expect_eq(std.mem.eql(u8, h2.value, "acme-corp"), true);
    rpos += h2.bytes_read;
    expect_eq(rpos, wpos);

    // 4. H2ClientSession Frame Codec & Flow Control State Machine
    let sess = H2ClientSession{};
    let preface_buf: [128]u8 = undefined;
    const plen = sess.buildPrefaceAndSettings(&preface_buf, false);
    expect_eq(std.mem.startsWith(u8, preface_buf[0..plen], CLIENT_PREFACE), true);

    let s1 = sess.newStream();
    expect_eq(s1.id, 1);
    expect_eq(sess.flow.consumeSend(&s1, 16384), true);
    expect_eq(sess.flow.availableSendWindow(s1), 65535 - 16384);

    // Incoming SETTINGS frame -> ReplyReady SETTINGS ACK
    let s_payload: [6]u8 = undefined;
    const su = SettingsPayloadUnit{ type: SettingsType.MaxFrameSize, value: 32768 };
    su.encode(&s_payload);
    let reply_buf: [64]u8 = undefined;
    const act_set = sess.processFrame(
        FrameHeader{ length: 6, type: FrameType.Settings, flags: 0, stream_id: 0 },
        &s_payload,
        &reply_buf,
        null,
    );
    expect_eq(act_set.kind == H2FrameActionKind.ReplyReady, true);
    expect_eq(act_set.reply_len, 9);
    expect_eq(sess.max_frame_size, 32768);
    expect_eq(sess.settings_received, true);

    // Incoming PING -> ReplyReady PING ACK with same opaque bytes
    const ping_data: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    const act_ping = sess.processFrame(
        FrameHeader{ length: 8, type: FrameType.Ping, flags: 0, stream_id: 0 },
        &ping_data,
        &reply_buf,
        null,
    );
    expect_eq(act_ping.kind == H2FrameActionKind.ReplyReady, true);
    expect_eq(act_ping.reply_len, 17);
    expect_eq(std.mem.eql(u8, reply_buf[9..17], &ping_data), true);

    // WINDOW_UPDATE credits stream send window
    let wu_wire: [13]u8 = undefined;
    expect_eq(H2ClientSession.buildWindowUpdate(&wu_wire, 1, 16384), 13);
    const act_wu = sess.processFrame(FrameHeader.decode(&wu_wire).?, wu_wire[9..13], &reply_buf, &s1);
    expect_eq(act_wu.kind == H2FrameActionKind.WindowUpdated, true);
    expect_eq(s1.window_size, 65535);

    // HEADERS + CONTINUATION state machine
    const act_hdr1 = sess.processFrame(
        FrameHeader{ length: 4, type: FrameType.Headers, flags: 0, stream_id: 1 },
        "abcd",
        &reply_buf,
        &s1,
    );
    expect_eq(act_hdr1.kind == H2FrameActionKind.None, true);
    expect_eq(sess.expecting_continuation, 1);

    const act_cont = sess.processFrame(
        FrameHeader{ length: 2, type: FrameType.Continuation, flags: @intFromEnum(HeadersFrameFlags.EndHeaders), stream_id: 1 },
        "ef",
        &reply_buf,
        &s1,
    );
    expect_eq(act_cont.kind == H2FrameActionKind.HeadersComplete, true);
    expect_eq(sess.expecting_continuation, 0);

    // GOAWAY frame roundtrip
    let ga_wire: [32]u8 = undefined;
    const ga_len = H2ClientSession.buildGoaway(&ga_wire, 1, ErrorCode.NoError, "bye");
    const ga_hdr = FrameHeader.decode(ga_wire[0..ga_len]).?;
    const act_ga = sess.processFrame(ga_hdr, ga_wire[9..ga_len], &reply_buf, null);
    expect_eq(act_ga.kind == H2FrameActionKind.GoawayReceived, true);
    expect_eq(sess.is_goaway, true);
    expect_eq(sess.goaway_last_stream_id, 1);
    expect_eq(sess.canCreateStream(), false);
}

fn testYaml12Complete(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Multi-document stream + Directives + Anchors/Aliases + Merge Keys + Block Scalars + Flow + Tags + Escapes
    const yaml_src =
        "%YAML 1.2\n" +
        "---\n" +
        "defaults: &base\n" +
        "  timeout: 30\n" +
        "  retries: 3\n" +
        "extra: &extra_cfg\n" +
        "  tls: true\n" +
        "  timeout: 99\n" +
        "service:\n" +
        "  <<: [*base, *extra_cfg]\n" +
        "  timeout: 60\n" +
        "  name: \"api\\n\\x2dgateway\\u0021\"\n" +
        "  raw_sq: 'it''s ok'\n" +
        "  literal_clip: |\n" +
        "    line1\n" +
        "    line2\n" +
        "  literal_strip: |-\n" +
        "    no-nl\n" +
        "  literal_keep: |+\n" +
        "    keep\n" +
        "\n" +
        "  folded: >-\n" +
        "    hello\n" +
        "    world\n" +
        "\n" +
        "    next\n" +
        "  flow_list: [10, { nested: \"ok\" }, !!str 42]\n" +
        "  tagged_int: !!int \"0x2A\"\n" +
        "  tagged_bool: !!bool \"yes\"\n" +
        "  tagged_str: !!str true\n" +
        "  tagged_null: !!null \"ignored\"\n" +
        "...\n" +
        "---\n" +
        "- item: first\n" +
        "  count: 1\n" +
        "- item: second\n" +
        "  count: 2\n" +
        "...\n" +
        "---\n" +
        "42\n";

    const stream = try YamlParser.parseStream(alloc, yaml_src);
    expect_eq(stream.count(), 3);

    const doc0 = stream.getDoc(0).?;
    expect_eq(doc0.getPath("service.timeout").?.asInt().?, 60);    // explicit overrides merge
    expect_eq(doc0.getPath("service.retries").?.asInt().?, 3);     // from *base
    expect_eq(doc0.getPath("service.tls").?.asBool().?, true);     // from *extra_cfg
    expect_eq(std.mem.eql(u8, doc0.getPath("service.name").?.asString().?, "api\n-gateway!"), true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.raw_sq").?.asString().?, "it's ok"), true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.literal_clip").?.asString().?, "line1\nline2\n"), true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.literal_strip").?.asString().?, "no-nl"), true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.literal_keep").?.asString().?, "keep\n\n"), true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.folded").?.asString().?, "hello world\nnext"), true);
    expect_eq(doc0.getPath("service.flow_list.0").?.asInt().?, 10);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.flow_list.1.nested").?.asString().?, "ok"), true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.flow_list.2").?.asString().?, "42"), true);
    expect_eq(doc0.getPath("service.tagged_int").?.asInt().?, 42);
    expect_eq(doc0.getPath("service.tagged_bool").?.asBool().?, true);
    expect_eq(std.mem.eql(u8, doc0.getPath("service.tagged_str").?.asString().?, "true"), true);
    expect_eq(doc0.getPath("service.tagged_null").?.isNull(), true);

    const doc1 = stream.getDoc(1).?;
    expect_eq(doc1.tag == ConfTag.Array, true);
    expect_eq(std.mem.eql(u8, doc1.getPath("1.item").?.asString().?, "second"), true);
    expect_eq(doc1.getPath("1.count").?.asInt().?, 2);

    const doc2 = stream.getDoc(2).?;
    expect_eq(doc2.asInt().?, 42);

    // Bidirectional YAML serialization & re-parse roundtrip
    const serialized = try YamlParser.stringify(alloc, doc0);
    const reparsed = try YamlParser.parse(alloc, serialized);
    expect_eq(reparsed.getPath("service.timeout").?.asInt().?, 60);
    expect_eq(reparsed.getPath("service.tls").?.asBool().?, true);
    expect_eq(reparsed.getPath("service.flow_list.0").?.asInt().?, 10);

    // Backward-compatible YAML.parse(src) works on nested keys
    const legacy = YAML.parse("server:\n  host: 127.0.0.1\n  port: 8080\n");
    expect_eq(std.mem.eql(u8, legacy.get("server", "host").?, "127.0.0.1"), true);
    expect_eq(legacy.getInt("server", "port", 0), 8080);
}

fn testToml10Complete(): !void {
    const alloc = std.heap.page_allocator;

    const toml_src =
        "point = { x = 10, y = 20 }\n" +
        "nested.dotted.val = \"deep\"\n" +
        "\"127.0.0.1\" = \"loopback\"\n" +
        "site.\"example.com\" = true\n" +
        "created_utc = 1979-05-27T07:32:00Z\n" +
        "created_off = 1979-05-27 07:32:00-07:00\n" +
        "local_dt = 2026-11-20T14:05:30.500\n" +
        "local_d = 2024-02-29\n" +
        "local_t = 08:15:30\n" +
        "matrix = [\n" +
        "  [1, 2],\n" +
        "  [3, 4],\n" +
        "]\n" +
        "ml_str = \"\"\"\n" +
        "lineA\n" +
        "lineB\"\"\"\n" +
        "\n" +
        "[[products]]\n" +
        "name = \"Hammer\"\n" +
        "sku = 738594937\n" +
        "\n" +
        "[[products]]\n" +
        "name = \"Nail\"\n" +
        "sku = 284758393\n" +
        "meta = { color = \"gray\", weight = 0.05 }\n";

    const root = try TomlParser.parse(alloc, toml_src);
    expect_eq(root.getPath("point.x").?.asInt().?, 10);
    expect_eq(root.getPath("point.y").?.asInt().?, 20);
    expect_eq(std.mem.eql(u8, root.getPath("nested.dotted.val").?.asString().?, "deep"), true);
    expect_eq(std.mem.eql(u8, root.get("127.0.0.1").?.asString().?, "loopback"), true);
    expect_eq(root.get("site").?.get("example.com").?.asBool().?, true);

    // RFC 3339 Date/Time assertions
    const dt_utc = root.get("created_utc").?.asDateTime().?;
    expect_eq(dt_utc.kind == ConfDateTimeKind.OffsetDateTime, true);
    expect_eq(dt_utc.year as i64, 1979);
    expect_eq(dt_utc.month as i64, 5);
    expect_eq(dt_utc.day as i64, 27);
    expect_eq(dt_utc.is_utc, true);

    const dt_off = root.get("created_off").?.asDateTime().?;
    expect_eq(dt_off.kind == ConfDateTimeKind.OffsetDateTime, true);
    expect_eq(dt_off.offset_minutes as i64, -420);

    const dt_local = root.get("local_dt").?.asDateTime().?;
    expect_eq(dt_local.kind == ConfDateTimeKind.LocalDateTime, true);
    expect_eq(dt_local.nanosecond as i64, 500000000);

    const d_only = root.get("local_d").?.asDateTime().?;
    expect_eq(d_only.kind == ConfDateTimeKind.LocalDate, true);
    expect_eq(d_only.day as i64, 29);

    const t_only = root.get("local_t").?.asDateTime().?;
    expect_eq(t_only.kind == ConfDateTimeKind.LocalTime, true);
    expect_eq(t_only.hour as i64, 8);
    expect_eq(t_only.minute as i64, 15);

    // Invalid calendar dates rejected
    expect_eq(ConfDateTime.parse("2025-02-29") == null, true); // 2025 not leap year
    expect_eq(ConfDateTime.parse("2026-13-01") == null, true); // invalid month
    expect_eq(ConfDateTime.parse("25:00:00") == null, true);   // invalid hour

    // Nested multi-dimensional arrays & multi-line strings
    expect_eq(root.getPath("matrix.0.1").?.asInt().?, 2);
    expect_eq(root.getPath("matrix.1.1").?.asInt().?, 4);
    expect_eq(std.mem.eql(u8, root.get("ml_str").?.asString().?, "lineA\nlineB"), true);

    // Array of tables [[products]]
    const products = root.get("products").?.asArray().?;
    expect_eq(products.len, 2);
    expect_eq(std.mem.eql(u8, products[0].get("name").?.asString().?, "Hammer"), true);
    expect_eq(std.mem.eql(u8, products[1].get("name").?.asString().?, "Nail"), true);
    expect_eq(std.mem.eql(u8, root.getPath("products.1.meta.color").?.asString().?, "gray"), true);

    // Bidirectional TOML serialization & re-parse
    const toml_out = try TomlParser.stringify(alloc, root);
    const reparsed = try TomlParser.parse(alloc, toml_out);
    expect_eq(reparsed.getPath("point.x").?.asInt().?, 10);
    expect_eq(std.mem.eql(u8, reparsed.getPath("products.1.name").?.asString().?, "Nail"), true);
    expect_eq(reparsed.getPath("matrix.1.0").?.asInt().?, 3);

    // Legacy TOML.parse compatibility on inline table + datetime
    const legacy_toml = TOML.parse(toml_src);
    expect_eq(legacy_toml.getInt("point", "x", 0), 10);
    expect_eq(legacy_toml.getDateTime("", "created_utc").?.year as i64, 1979);
}

fn main() {
    test("1. cap:http — 200+ MIME DB, HTTPStatus, HeaderBuilder, CacheControl/Disposition/Accept/Range & Wire HTTP", () => {
        try testMimeStatusHeaderToolsAndWireHttp();
    });
    test("2. cap:http/h2 — Complete 61-Entry Static Table, 257-Entry Huffman, Dynamic Table & H2 Flow Control", () => {
        try testHpackCompleteAndH2FlowControl();
    });
    test("3. cap:conf — Industrial YAML 1.2 (Multi-doc, Anchors/Aliases, Merge <<, Block |/>, Flow, Tags, Roundtrip)", () => {
        try testYaml12Complete();
    });
    test("4. cap:conf — Industrial TOML 1.0 (Inline Tables, Dotted/Quoted Keys, RFC 3339 Dates, [[Arrays]], Roundtrip)", () => {
        try testToml10Complete();
    });
}
