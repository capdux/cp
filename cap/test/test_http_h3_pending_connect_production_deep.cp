// cap/test/test_http_h3_pending_connect_production_deep.cp
// 深度生产级回归测试：cap:http HTTP/3 DNS-Pending QUIC 连接协调队列 (lib/http/h3/pending_connect.cp)

import { test } from "cap:test";
import {
    H3_DNS_PENDING_MAX_CONNECTS,
    H3_DNS_DEFAULT_TTL_SECS,
    H3_DNS_MAX_TTL_SECS,
    H3DnsConnectState,
    H3DnsDrainAction,
    H3SessionFailReason,
    H3DrainOutcome,
    H3PendingConnectQueue,
    parseH3DnsTtlSeconds,
} from "cap:http";

test("h3_pending_connect: register, case-insensitive inflight dedup, wakeup edge & socket attach") {
    let q = H3PendingConnectQueue.init();
    if (q.dnsTtlSeconds() != H3_DNS_DEFAULT_TTL_SECS) return err.UnexpectedDefaultTtl;

    const r1 = q.register(101, "quic.example.com", 443, 2)?;
    if (r1.deduped_inflight) return err.FirstShouldNotDedup;

    const r2 = q.register(102, "QUIC.Example.COM", 443, 1)?;
    if (!r2.deduped_inflight) return err.SecondShouldDedup;

    const r3 = q.register(103, "quic.example.com", 8443, 3)?;
    if (r3.deduped_inflight) return err.DifferentPortShouldNotDedup;

    if (q.activeCount() != 3 || q.readyCount() != 0) return err.UnexpectedCounts;

    if (e1 := q.getEntry(r1.connect_id)) {
        if (e1.state != H3DnsConnectState.Resolving || e1.pending_streams != 2) {
            return err.UnexpectedEntrySnapshot;
        }
    } else {
        return err.MissingEntry1;
    }

    // 首条 DNS 回调需唤醒事件循环，后续已非空不再重复唤醒
    const wake1 = q.onDnsResolvedThreadsafe(r1.connect_id, true, 7001)?;
    const wake2 = q.onDnsResolvedThreadsafe(r2.connect_id, true, 7002)?;
    if (!wake1 || wake2) return err.UnexpectedWakeupEdge;
    if (q.readyCount() != 2) return err.ExpectedTwoReady;

    let outcomes: [4]H3DrainOutcome = [H3DrainOutcome{}; 4];
    const n = q.drainResolved(outcomes[0..])?;
    if (n != 2) return err.ExpectedTwoDrained;
    if (outcomes[0].action != H3DnsDrainAction.AttachedSocket ||
        outcomes[0].session_id != 101 ||
        outcomes[0].quic_socket_id != 7001 ||
        outcomes[0].should_cancel_pc ||
        outcomes[0].should_fail_session ||
        outcomes[0].should_evict_dns_cache ||
        !outcomes[0].released_session_ref) {
        return err.UnexpectedOutcome0;
    }
    if (outcomes[1].action != H3DnsDrainAction.AttachedSocket ||
        outcomes[1].session_id != 102 ||
        outcomes[1].quic_socket_id != 7002) {
        return err.UnexpectedOutcome1;
    }

    // 仍剩余 r3 在途
    if (q.activeCount() != 1 || q.readyCount() != 0) return err.ExpectedOneRemaining;
    const st = q.stats();
    if (st.total_registered != 3 || st.inflight_dedup_hits != 1 || st.attached_count != 2) {
        return err.StatsMismatch;
    }
    * = r3;
}

test("h3_pending_connect: session closed vs zero pending streams abort semantics") {
    let q = H3PendingConnectQueue.init();
    const r_closed = q.register(201, "api.bun.sh", 443, 2)?;
    const r_empty = q.register(202, "cdn.bun.sh", 443, 1)?;

    if (!q.markSessionClosed(201)) return err.FailedMarkClosed;
    if (q.markSessionClosed(9999)) return err.UnknownSessionShouldReturnFalse;
    if (!q.updatePendingStreams(202, 0)) return err.FailedUpdatePending;

    * = q.onDnsResolvedThreadsafe(r_closed.connect_id, true, 8001)?;
    * = q.onDnsResolvedThreadsafe(r_empty.connect_id, true, 8002)?;

    let out: [4]H3DrainOutcome = [H3DrainOutcome{}; 4];
    const n = q.drainResolved(out[0..])?;
    if (n != 2) return err.ExpectedTwoOutcomes;

    // 已关闭会话：只取消 pending connect，不重复触发 failSession
    if (out[0].action != H3DnsDrainAction.CancelledSessionClosed ||
        !out[0].should_cancel_pc ||
        out[0].should_fail_session ||
        out[0].fail_reason != H3SessionFailReason.None) {
        return err.UnexpectedClosedOutcome;
    }

    // 未关闭但 pending_streams == 0：取消 pending connect 且触发 Aborted
    if (out[1].action != H3DnsDrainAction.AbortedNoPendingStreams ||
        !out[1].should_cancel_pc ||
        !out[1].should_fail_session ||
        out[1].fail_reason != H3SessionFailReason.Aborted) {
        return err.UnexpectedAbortedOutcome;
    }

    const st = q.stats();
    if (st.cancelled_closed_count != 1 || st.aborted_empty_count != 1 || st.active_count != 0) {
        return err.CancelAbortStatsMismatch;
    }
}

test("h3_pending_connect: DNS resolution failure & null socket trigger cache eviction") {
    let q = H3PendingConnectQueue.init();
    const r_err = q.register(301, "nxdomain.invalid", 443, 1)?;
    const r_null_sock = q.register(302, "zero-sock.invalid", 443, 2)?;

    * = q.onDnsResolvedThreadsafe(r_err.connect_id, false, 0)?;
    // resolved_ok=true 但 quic_socket_id==0 同样视为解析失败
    * = q.onDnsResolvedThreadsafe(r_null_sock.connect_id, true, 0)?;

    let out: [4]H3DrainOutcome = [H3DrainOutcome{}; 4];
    const n = q.drainResolved(out[0..])?;
    if (n != 2) return err.ExpectedTwoFailed;

    let i: usize = 0;
    while (i < 2) {
        if (out[i].action != H3DnsDrainAction.FailedDnsResolution ||
            out[i].fail_reason != H3SessionFailReason.DnsResolutionFailed ||
            !out[i].should_fail_session ||
            !out[i].should_evict_dns_cache ||
            out[i].should_cancel_pc) {
            return err.UnexpectedDnsFailOutcome;
        }
        i += 1;
    }

    if (q.stats().dns_failed_count != 2 || q.activeCount() != 0) {
        return err.DnsFailStatsMismatch;
    }
}

test("h3_pending_connect: cancelBeforeResolve, double-resolve guard & BufferTooSmall safety") {
    let q = H3PendingConnectQueue.init();
    const r1 = q.register(401, "one.example.com", 443, 1)?;
    const r2 = q.register(402, "two.example.com", 443, 1)?;
    const r3 = q.register(403, "three.example.com", 443, 1)?;

    // 解析前主动取消 r2，槽位立即释放
    const cancelled = q.cancelBeforeResolve(r2.connect_id)?;
    if (!cancelled || q.activeCount() != 2) return err.CancelBeforeResolveFailed;

    * = q.onDnsResolvedThreadsafe(r1.connect_id, true, 9101)?;
    * = q.onDnsResolvedThreadsafe(r3.connect_id, true, 9103)?;

    // 重复回调拦截
    let dup_blocked = false;
    * = q.onDnsResolvedThreadsafe(r1.connect_id, true, 9999) catch {
        dup_blocked = true;
        false;
    };
    if (!dup_blocked) return err.ExpectedAlreadyResolvedError;

    // 已入队不能再 cancelBeforeResolve
    let cancel_after_blocked = false;
    * = q.cancelBeforeResolve(r1.connect_id) catch {
        cancel_after_blocked = true;
        false;
    };
    if (!cancel_after_blocked) return err.ExpectedCancelAfterResolveError;

    // 输出缓冲过小返回 BufferTooSmall 且不丢失待排空数据
    let tiny: [1]H3DrainOutcome = [H3DrainOutcome{}; 1];
    let too_small = false;
    * = q.drainResolved(tiny[0..]) catch {
        too_small = true;
        0;
    };
    if (!too_small || q.readyCount() != 2 || q.activeCount() != 2) {
        return err.BufferTooSmallMustPreserveState;
    }

    let ok_buf: [2]H3DrainOutcome = [H3DrainOutcome{}; 2];
    const n = q.drainResolved(ok_buf[0..])?;
    if (n != 2 || q.activeCount() != 0 || q.readyCount() != 0) {
        return err.DrainAfterResizeFailed;
    }
}

test("h3_pending_connect: DNS TTL parser & initWithTtl boundaries") {
    const def_ttl = parseH3DnsTtlSeconds("")?;
    if (def_ttl != H3_DNS_DEFAULT_TTL_SECS) return err.ExpectedDefault30;

    const aws_ttl = parseH3DnsTtlSeconds("5")?;
    if (aws_ttl != 5) return err.Expected5;

    const max_ttl = parseH3DnsTtlSeconds("86400")?;
    if (max_ttl != H3_DNS_MAX_TTL_SECS) return err.ExpectedMax86400;

    const q5 = H3PendingConnectQueue.initWithTtl(aws_ttl)?;
    if (q5.dnsTtlSeconds() != 5) return err.ExpectedQueueTtl5;

    let zero_rejected = false;
    * = parseH3DnsTtlSeconds("0") catch {
        zero_rejected = true;
        0;
    };
    if (!zero_rejected) return err.ZeroTtlMustFail;

    let over_rejected = false;
    * = parseH3DnsTtlSeconds("86401") catch {
        over_rejected = true;
        0;
    };
    if (!over_rejected) return err.OverflowTtlMustFail;

    let alpha_rejected = false;
    * = parseH3DnsTtlSeconds("30s") catch {
        alpha_rejected = true;
        0;
    };
    if (!alpha_rejected) return err.NonDigitTtlMustFail;
}

test("h3_pending_connect: input validation & fixed-capacity saturation + reuse") {
    let q = H3PendingConnectQueue.init();

    // 非法 session_id / port / host
    let bad_sid = false;
    * = q.register(0, "ok.example.com", 443, 1) catch { bad_sid = true; };
    if (!bad_sid) return err.ZeroSessionMustFail;

    let bad_port = false;
    * = q.register(1, "ok.example.com", 0, 1) catch { bad_port = true; };
    if (!bad_port) return err.ZeroPortMustFail;

    let bad_host = false;
    * = q.register(1, "bad/host.com", 443, 1) catch { bad_host = true; };
    if (!bad_host) return err.SlashHostMustFail;

    let long_host: [260]u8 = ['a'; 260];
    let too_long = false;
    * = q.register(1, long_host[0..], 443, 1) catch { too_long = true; };
    if (!too_long) return err.HostTooLongMustFail;

    // 填满全部 32 槽位
    let ids: [H3_DNS_PENDING_MAX_CONNECTS]u64 = [0; H3_DNS_PENDING_MAX_CONNECTS];
    let i: usize = 0;
    while (i < H3_DNS_PENDING_MAX_CONNECTS) {
        const res = q.register((i as u64) + 1000, "pool.example.com", 443, 1)?;
        ids[i] = res.connect_id;
        i += 1;
    }

    // 重复 session_id 与满槽位均应拦截
    let dup_sid = false;
    * = q.register(1000, "other.example.com", 443, 1) catch { dup_sid = true; };
    if (!dup_sid) return err.DupSessionMustFail;

    let full_err = false;
    * = q.register(9999, "overflow.example.com", 443, 1) catch { full_err = true; };
    if (!full_err) return err.FullQueueMustFail;

    // 全部回调并排空后，槽位可完整复用
    i = 0;
    while (i < H3_DNS_PENDING_MAX_CONNECTS) {
        * = q.onDnsResolvedThreadsafe(ids[i], true, 5000 + (i as u64))?;
        i += 1;
    }
    let out: [H3_DNS_PENDING_MAX_CONNECTS]H3DrainOutcome = [H3DrainOutcome{}; H3_DNS_PENDING_MAX_CONNECTS];
    const drained = q.drainResolved(out[0..])?;
    if (drained != H3_DNS_PENDING_MAX_CONNECTS || q.activeCount() != 0) {
        return err.FullDrainMismatch;
    }

    const reused = q.register(2000, "reused.example.com", 443, 1)?;
    if (reused.connect_id == 0 || q.activeCount() != 1) return err.ReuseFailed;
}
