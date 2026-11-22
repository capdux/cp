// cap/test/test_http_h3_stream_dispatch_production_deep.cp
// 深度生产级回归测试：cap:http HTTP/3 QUIC 客户端回调事件调度器 (lib/http/h3/stream_dispatch.cp)

import { test } from "cap:test";
import {
    H3_DISPATCH_MAX_STREAMS,
    H3ConnCloseReason,
    H3HeaderEventKind,
    H3RawHeader,
    H3CallbackDispatcher,
} from "cap:http";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("h3_dispatch: full happy-path lifecycle handshake -> open -> headers -> data -> close") {
    let d = H3CallbackDispatcher.init();
    * = d.enqueueRequest(101)?;
    * = d.enqueueRequest(102)?;

    // 握手成功返回 2 个待建流数量
    const to_open = d.onHandshakeDone(true);
    if (to_open != 2 || !d.handshake_done) { return err.HandshakeCountMismatch; }

    // 打开两条客户端流依次绑定 101 和 102
    const r1 = d.onStreamOpen(4, true)?;
    const r2 = d.onStreamOpen(8, true)?;
    if (r1 != 101 || r2 != 102) { return err.StreamBindOrderMismatch; }

    // 投递最终响应头（含伪头与普通头）
    const hdrs: [3]H3RawHeader = [
        H3RawHeader{ name: ":status", value: "200" },
        H3RawHeader{ name: ":ignored-pseudo", value: "x" },
        H3RawHeader{ name: "content-type", value: "application/json" },
    ];
    const hr = d.onStreamHeaders(4, hdrs[0..])?;
    if (hr.kind != H3HeaderEventKind.DeliveredFinalHeaders || hr.status_code != 200 || hr.regular_header_count != 1) {
        return err.FinalHeadersMismatch;
    }

    // 投递两段数据并结束
    if (d.onStreamData(4, 64, false)?) { return err.FinShouldBeFalse; }
    if (!(d.onStreamData(4, 36, true)?)) { return err.FinShouldBeTrue; }

    if (slot := d.findSlotByQuicStream(4)) {
        if (d.streams[slot].received_bytes != 100 || !bytesEq(d.streams[slot].headersSlice()[0].name, "content-type")) {
            return err.SlotPayloadMismatch;
        }
    } else { return err.SlotNotFound; }

    // 关闭流 4，O(1) swap-remove 后仅剩 102
    const closed_req = d.onStreamClose(4)?;
    if (closed_req != 101 || d.activeStreamCount() != 1) { return err.CloseReclaimMismatch; }
    if (d.findSlotByQuicStream(8) == null) { return err.RemainingStreamLost; }
}

test("h3_dispatch: ignores 1xx informational responses and post-status trailers, flags missing :status") {
    let d = H3CallbackDispatcher.init();
    * = d.enqueueRequest(201)?;
    * = d.enqueueRequest(202)?;
    * = d.onHandshakeDone(true);
    * = d.onStreamOpen(4, true)?;
    * = d.onStreamOpen(8, true)?;

    // 流 4 先收到 103 Early Hints -> IgnoredInformational1xx
    const early: [2]H3RawHeader = [
        H3RawHeader{ name: ":status", value: "103" },
        H3RawHeader{ name: "link", value: "</app.css>; rel=preload" },
    ];
    const r_103 = d.onStreamHeaders(4, early[0..])?;
    if (r_103.kind != H3HeaderEventKind.IgnoredInformational1xx || r_103.status_code != 103) {
        return err.EarlyHintsNotIgnored;
    }

    // 流 4 随后收到 200 OK -> DeliveredFinalHeaders
    const ok200: [1]H3RawHeader = [H3RawHeader{ name: ":status", value: "200" }];
    const r_200 = d.onStreamHeaders(4, ok200[0..])?;
    if (r_200.kind != H3HeaderEventKind.DeliveredFinalHeaders || r_200.status_code != 200) {
        return err.FinalAfter1xxFailed;
    }

    // 流 4 再次收到无 :status 的尾部 Trailer -> IgnoredTrailingHeaders
    const trailer: [1]H3RawHeader = [H3RawHeader{ name: "grpc-status", value: "0" }];
    const r_tr = d.onStreamHeaders(4, trailer[0..])?;
    if (r_tr.kind != H3HeaderEventKind.IgnoredTrailingHeaders || r_tr.status_code != 200) {
        return err.TrailerNotIgnored;
    }

    // 流 8 首次头帧缺失 :status -> ProtocolErrorMissingStatus
    const no_status: [1]H3RawHeader = [H3RawHeader{ name: "server", value: "test" }];
    const r_err = d.onStreamHeaders(8, no_status[0..])?;
    if (r_err.kind != H3HeaderEventKind.ProtocolErrorMissingStatus || r_err.status_code != 0) {
        return err.MissingStatusNotFlagged;
    }
}

test("h3_dispatch: pre-handshake close reports HandshakeFailed and drains pending requests") {
    let d = H3CallbackDispatcher.init();
    * = d.enqueueRequest(301)?;
    * = d.enqueueRequest(302)?;

    // 握手失败
    const opened = d.onHandshakeDone(false);
    if (opened != 0 || !d.closed) { return err.FailedHandshakeStateMismatch; }

    const summary = d.onConnClose();
    if (summary.reason != H3ConnCloseReason.HandshakeFailed || summary.failed_count != 2 || d.activeStreamCount() != 0) {
        return err.PreHandshakeDrainMismatch;
    }
    const ids = summary.failedIdsSlice();
    if (ids[0] != 301 || ids[1] != 302) { return err.DrainedIdsMismatch; }
}

test("h3_dispatch: post-handshake close and GOAWAY close reason classification") {
    let d1 = H3CallbackDispatcher.init();
    * = d1.enqueueRequest(401)?;
    * = d1.onHandshakeDone(true);
    * = d1.onStreamOpen(4, true)?;
    const s1 = d1.onConnClose();
    if (s1.reason != H3ConnCloseReason.ConnectionClosed || s1.failed_count != 1) {
        return err.MidSessionCloseReasonMismatch;
    }

    let d2 = H3CallbackDispatcher.init();
    * = d2.enqueueRequest(501)?;
    * = d2.onHandshakeDone(true);
    d2.onGoaway();

    // GOAWAY 后拒绝新请求入队
    let rejected_new = false;
    d2.enqueueRequest(502) catch {
        rejected_new = true;
    };
    if (!rejected_new) { return err.GoawayShouldRejectNewRequest; }

    const s2 = d2.onConnClose();
    if (s2.reason != H3ConnCloseReason.GoawayReceived || s2.failed_count != 1) {
        return err.GoawayCloseReasonMismatch;
    }
}

test("h3_dispatch: rejects server-initiated streams, zero stream id and unbound open") {
    let d = H3CallbackDispatcher.init();
    * = d.enqueueRequest(601)?;
    * = d.onHandshakeDone(true);

    // 服务端主动发起流（is_client=false）拒绝
    let srv_rejected = false;
    d.onStreamOpen(3, false) catch {
        srv_rejected = true;
    };
    if (!srv_rejected) { return err.ServerStreamNotRejected; }

    // quic_stream_id == 0 拒绝
    let zero_id_rejected = false;
    d.onStreamOpen(0, true) catch {
        zero_id_rejected = true;
    };
    if (!zero_id_rejected) { return err.ZeroStreamIdNotRejected; }

    // 绑定唯一挂起请求后，再次打开流应报 NoPendingUnboundStream
    * = d.onStreamOpen(4, true)?;
    let no_pending = false;
    d.onStreamOpen(8, true) catch {
        no_pending = true;
    };
    if (!no_pending) { return err.UnboundOverflowNotRejected; }
}

test("h3_dispatch: duplicate request id, unknown stream id and max streams capacity guards") {
    let d = H3CallbackDispatcher.init();
    * = d.enqueueRequest(701)?;

    // 重复 request_id 拒绝
    let dup_err = false;
    d.enqueueRequest(701) catch {
        dup_err = true;
    };
    if (!dup_err) { return err.DupReqIdNotRejected; }

    // 未知流 ID 触发 StreamNotFound
    let unknown_stream = false;
    d.onStreamData(999, 10, false) catch {
        unknown_stream = true;
    };
    if (!unknown_stream) { return err.UnknownStreamNotRejected; }

    // 容量上限保护
    let i: usize = 1;
    while (i < H3_DISPATCH_MAX_STREAMS) {
        * = d.enqueueRequest(@as(u64, @intCast(701 + i)))?;
        i += 1;
    }
    let cap_err = false;
    d.enqueueRequest(99999) catch {
        cap_err = true;
    };
    if (!cap_err) { return err.StreamCapOverflowNotRejected; }
}
