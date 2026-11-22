import { test, expect } from "cap:test";
import {
    IPC_CHANNEL_BUF_CAP,
    IpcChannel,
    IpcChannelCloseReason,
    IpcChannelPollStatus,
    IpcFrameKind,
    IpcFrameWriter,
    IpcPayloadReader,
} from "cap:test_runner";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("test_runner ipc_channel: fragmented reads and coalesced multi-frame decoding", () => {
    // Encode two frames back-to-back into `wire`
    let wire1: [128]u8 = [0; 128];
    let w1 = IpcFrameWriter.init(wire1[0..]);
    let ok1 = true;
    w1.begin(IpcFrameKind.ready) catch { ok1 = false; };
    w1.writeU32(1001) catch { ok1 = false; };
    const f1 = w1.finish() catch "";
    expect(ok1).toBe(true);
    expect(f1.len).toBe(9);

    let wire2: [128]u8 = [0; 128];
    let w2 = IpcFrameWriter.init(wire2[0..]);
    let ok2 = true;
    w2.begin(IpcFrameKind.file_start) catch { ok2 = false; };
    w2.writeStr("src/math.test.cp") catch { ok2 = false; };
    const f2 = w2.finish() catch "";
    expect(ok2).toBe(true);

    let ch = IpcChannel.init(true);
    let payload: [128]u8 = [0; 128];

    // Feed first 3 bytes (incomplete 5-byte header)
    * = ch.ingest(f1[0..3]) catch 0;
    const p0 = ch.pollFrame(payload[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p0.status == IpcChannelPollStatus.NeedMoreData).toBe(true);
    expect(ch.rxBufferedLen()).toBe(3);

    // Feed rest of f1 + first 4 bytes of f2 (coalesced stream)
    * = ch.ingest(f1[3..]) catch 0;
    * = ch.ingest(f2[0..4]) catch 0;

    const p1 = ch.pollFrame(payload[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p1.status == IpcChannelPollStatus.Frame).toBe(true);
    expect(p1.kind == IpcFrameKind.ready).toBe(true);
    expect(p1.payload_len).toBe(4);
    let rd1 = IpcPayloadReader.init(payload[0..p1.payload_len]);
    expect(rd1.readU32() ?? 0).toBe(1001);

    // Second frame still incomplete
    const p1b = ch.pollFrame(payload[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p1b.status == IpcChannelPollStatus.NeedMoreData).toBe(true);

    // Feed remainder of f2
    * = ch.ingest(f2[4..]) catch 0;
    const p2 = ch.pollFrame(payload[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(p2.status == IpcChannelPollStatus.Frame).toBe(true);
    expect(p2.kind == IpcFrameKind.file_start).toBe(true);
    let rd2 = IpcPayloadReader.init(payload[0..p2.payload_len]);
    const fname = rd2.readStr() ?? "";
    expect(sliceEq(fname, "src/math.test.cp")).toBe(true);
    expect(ch.frames_decoded).toBe(2);
    expect(ch.rxBufferedLen()).toBe(0);
});

test("test_runner ipc_channel: forward-compatible skip of unknown frame kind", () => {
    let ch = IpcChannel.init(true);
    // Craft an unknown kind=99 frame with 3-byte payload [0xAA, 0xBB, 0xCC]
    const unknown_frame: [8]u8 = [3, 0, 0, 0, 99, 0xAA, 0xBB, 0xCC];
    // Followed by a valid shutdown frame (0-byte payload, kind=8)
    const shutdown_frame: [5]u8 = [0, 0, 0, 0, 8];

    * = ch.ingest(unknown_frame[0..]) catch 0;
    * = ch.ingest(shutdown_frame[0..]) catch 0;

    let payload: [32]u8 = [0; 32];
    const res = ch.pollFrame(payload[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(res.status == IpcChannelPollStatus.Frame).toBe(true);
    expect(res.kind == IpcFrameKind.shutdown).toBe(true);
    expect(res.payload_len).toBe(0);
    expect(ch.unknown_frames_skipped).toBe(1);
    expect(ch.frames_decoded).toBe(1);
});

test("test_runner ipc_channel: POSIX partial-write backpressure queue and advanceTx", () => {
    let ch = IpcChannel.init(true);
    expect(ch.hasPendingWrites()).toBe(false);

    const msg1: []const u8 = "FRAME_ONE_";
    const msg2: []const u8 = "FRAME_TWO";
    expect(ch.queueSend(msg1) catch 0).toBe(10);
    expect(ch.queueSend(msg2) catch 0).toBe(19);
    expect(ch.hasPendingWrites()).toBe(true);

    // Kernel accepts only 6 bytes
    const c1 = ch.peekTxChunk(6);
    expect(sliceEq(c1, "FRAME_")).toBe(true);
    ch.advanceTx(6);
    expect(ch.txQueuedLen()).toBe(13);
    expect(ch.bytes_sent).toBe(6);

    // Kernel accepts remaining 13 bytes
    const c2 = ch.peekTxChunk(64);
    expect(sliceEq(c2, "ONE_FRAME_TWO")).toBe(true);
    ch.advanceTx(13);
    expect(ch.txQueuedLen()).toBe(0);
    expect(ch.hasPendingWrites()).toBe(false);
    expect(ch.bytes_sent).toBe(19);
});

test("test_runner ipc_channel: Windows double-buffered inflight swap and write error", () => {
    let ch = IpcChannel.init(true);
    * = ch.queueSend("BATCH_A") catch 0;

    const inflight1 = ch.submitInflight();
    expect(sliceEq(inflight1, "BATCH_A")).toBe(true);
    expect(ch.inflightLen()).toBe(7);
    expect(ch.txQueuedLen()).toBe(0);
    expect(ch.hasPendingWrites()).toBe(true);

    // New writes arriving while inflight is active queue in tx_buf
    * = ch.queueSend("BATCH_B") catch 0;
    expect(ch.txQueuedLen()).toBe(7);
    // Second submitInflight while active returns empty slice
    expect(ch.submitInflight().len).toBe(0);

    // Complete first write cleanly -> submit second batch
    ch.completeInflight(true);
    expect(ch.inflightLen()).toBe(0);
    expect(ch.bytes_sent).toBe(7);

    const inflight2 = ch.submitInflight();
    expect(sliceEq(inflight2, "BATCH_B")).toBe(true);

    // Simulate OS write error on second batch
    ch.completeInflight(false);
    expect(ch.isDone()).toBe(true);
    expect(ch.close_reason == IpcChannelCloseReason.WriteError).toBe(true);
    expect(ch.hasPendingWrites()).toBe(false);
});

test("test_runner ipc_channel: corrupt >64MiB frame sets ProtocolError while staying attached", () => {
    let ch = IpcChannel.init(true);
    // payload_len = 0x0400_0001 (64 MiB + 1 byte), kind = ready (0)
    const corrupt_hdr: [5]u8 = [0x01, 0x00, 0x00, 0x04, 0];
    * = ch.ingest(corrupt_hdr[0..]) catch 0;

    let payload: [32]u8 = [0; 32];
    let got_corrupt = false;
    * = ch.pollFrame(payload[0..]) catch {
        got_corrupt = true;
    };
    expect(got_corrupt).toBe(true);
    expect(ch.isDone()).toBe(true);
    expect(ch.isAttached()).toBe(true);
    expect(ch.isProtocolError()).toBe(true);

    // Subsequent ingest/queueSend rejected with ChannelClosed
    let send_closed = false;
    * = ch.queueSend("x") catch {
        send_closed = true;
    };
    expect(send_closed).toBe(true);
});

test("test_runner ipc_channel: peer EOF vs clean close and buffer overflow guards", () => {
    let ch1 = IpcChannel.init(true);
    ch1.onPeerEof();
    expect(ch1.isDone()).toBe(true);
    expect(ch1.isAttached()).toBe(false);
    expect(ch1.isProtocolError()).toBe(false);
    expect(ch1.close_reason == IpcChannelCloseReason.PeerEof).toBe(true);

    let ch2 = IpcChannel.init(true);
    ch2.close();
    expect(ch2.isDone()).toBe(true);
    expect(ch2.isAttached()).toBe(false);
    expect(ch2.close_reason == IpcChannelCloseReason.CleanClose).toBe(true);

    // PayloadBufferTooSmall preserves frame in rx_buf for retry with larger buffer
    let ch3 = IpcChannel.init(true);
    const frame: [10]u8 = [5, 0, 0, 0, 0, 'h', 'e', 'l', 'l', 'o'];
    * = ch3.ingest(frame[0..]) catch 0;

    let tiny: [2]u8 = [0; 2];
    let small_err = false;
    * = ch3.pollFrame(tiny[0..]) catch {
        small_err = true;
    };
    expect(small_err).toBe(true);
    expect(ch3.rxBufferedLen()).toBe(10);

    let big: [16]u8 = [0; 16];
    const retry = ch3.pollFrame(big[0..]) catch {
        expect(false).toBe(true);
        return;
    };
    expect(retry.status == IpcChannelPollStatus.Frame).toBe(true);
    expect(sliceEq(big[0..retry.payload_len], "hello")).toBe(true);
    expect(IPC_CHANNEL_BUF_CAP).toBe(8192);
});
