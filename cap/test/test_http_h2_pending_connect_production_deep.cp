// cap/test/test_http_h2_pending_connect_production_deep.cp
// 深度生产级回归测试：cap:http HTTP/2 在途 TLS 握手合并器 (lib/http/h2/pending_connect.cp)

import { test } from "cap:test";
import {
    H2_PENDING_MAX_CONNECTS,
    H2_PENDING_MAX_WAITERS,
    H2ConnectJoinKind,
    H2PendingConnectTable,
} from "cap:http";

test("h2_pending_connect: case-insensitive singleflight coalescing & FIFO waiter drain") {
    let tbl = H2PendingConnectTable.init();

    const j1 = tbl.joinOrStart("api.example.com", 443, 0, 101)?;
    if (j1.kind != H2ConnectJoinKind.InitiatedNew || j1.waiter_count != 1) {
        return err.ExpectedInitiatedNew;
    }

    const j2 = tbl.joinOrStart("API.Example.COM", 443, 0, 102)?;
    if (j2.kind != H2ConnectJoinKind.CoalescedWaiter || j2.connect_id != j1.connect_id || j2.waiter_count != 2) {
        return err.ExpectedCoalesced2;
    }

    const j3 = tbl.joinOrStart("api.example.com", 443, 0, 103)?;
    if (j3.kind != H2ConnectJoinKind.CoalescedWaiter || j3.connect_id != j1.connect_id || j3.waiter_count != 3) {
        return err.ExpectedCoalesced3;
    }
    if (tbl.activeCount() != 1) return err.ExpectedSinglePendingEntry;

    let drained: [8]u64 = [0; 8];
    const n = tbl.resolveAndDrain(j1.connect_id, drained[0..])?;
    if (n != 3 || drained[0] != 101 || drained[1] != 102 || drained[2] != 103) {
        return err.DrainFifoMismatch;
    }
    if (tbl.activeCount() != 0) return err.ExpectedEmptyAfterDrain;
}

test("h2_pending_connect: strict port & TLS ssl_config_id isolation") {
    let tbl = H2PendingConnectTable.init();

    // 同域名不同端口必须隔离
    const c_443 = tbl.joinOrStart("cdn.example.com", 443, 0, 1)?;
    const c_8443 = tbl.joinOrStart("cdn.example.com", 8443, 0, 2)?;
    if (c_443.connect_id == c_8443.connect_id || c_8443.kind != H2ConnectJoinKind.InitiatedNew) {
        return err.PortIsolationFailed;
    }

    // 同域名同端口但不同 TLS SSLConfig (CA / mTLS / rejectUnauthorized) 必须隔离
    const c_custom_tls = tbl.joinOrStart("cdn.example.com", 443, 0xCAFE, 3)?;
    if (c_custom_tls.connect_id == c_443.connect_id || c_custom_tls.kind != H2ConnectJoinKind.InitiatedNew) {
        return err.TlsConfigIsolationFailed;
    }
    if (tbl.activeCount() != 3) return err.Expected3IsolatedEntries;

    // 匹配自定义 TLS 配置的后续请求正确合并
    const c_custom_tls2 = tbl.joinOrStart("CDN.EXAMPLE.COM", 443, 0xCAFE, 4)?;
    if (c_custom_tls2.kind != H2ConnectJoinKind.CoalescedWaiter || c_custom_tls2.connect_id != c_custom_tls.connect_id) {
        return err.CustomTlsCoalesceFailed;
    }
}

test("h2_pending_connect: waiter cancellation preserves FIFO order & reclaims empty slot") {
    let tbl = H2PendingConnectTable.init();

    const j1 = tbl.joinOrStart("s1.example.com", 443, 0, 10)?;
    * = tbl.joinOrStart("s1.example.com", 443, 0, 20)?;
    * = tbl.joinOrStart("s1.example.com", 443, 0, 30)?;

    // 取消中间等待者 20，剩余 10, 30 保持顺序
    if (!tbl.cancelWaiter(20)) return err.CancelMiddleFailed;
    if (tbl.cancelWaiter(999)) return err.CancelUnknownShouldBeFalse;

    let out: [4]u64 = [0; 4];
    const n = tbl.resolveAndDrain(j1.connect_id, out[0..])?;
    if (n != 2 || out[0] != 10 || out[1] != 30) return err.CancelOrderMismatch;

    // 单等待者取消后自动回收整个槽位
    * = tbl.joinOrStart("s2.example.com", 443, 0, 50)?;
    if (tbl.activeCount() != 1) return err.Expected1Active;
    if (!tbl.cancelWaiter(50)) return err.CancelLastFailed;
    if (tbl.activeCount() != 0) return err.EmptySlotNotReclaimed;
}

test("h2_pending_connect: O(1) swap-remove unregistration keeps remaining entries intact") {
    let tbl = H2PendingConnectTable.init();

    const a = tbl.joinOrStart("a.example.com", 443, 0, 1)?;
    const b = tbl.joinOrStart("b.example.com", 443, 0, 2)?;
    const c = tbl.joinOrStart("c.example.com", 443, 0, 3)?;

    // 先移除首条 a（触发 c swap 到 index 0）
    let buf: [4]u64 = [0; 4];
    if (tbl.resolveAndDrain(a.connect_id, buf[0..])? != 1 || buf[0] != 1) return err.DrainAFailed;
    if (tbl.activeCount() != 2) return err.CountAfterAFailed;

    // 确认 c 仍可合并新 waiter 且 b 仍可正常 resolve
    const c2 = tbl.joinOrStart("c.example.com", 443, 0, 33)?;
    if (c2.kind != H2ConnectJoinKind.CoalescedWaiter || c2.connect_id != c.connect_id) {
        return err.SwappedEntryCoalesceFailed;
    }

    if (tbl.resolveAndDrain(b.connect_id, buf[0..])? != 1 || buf[0] != 2) return err.DrainBFailed;
    if (tbl.resolveAndDrain(c.connect_id, buf[0..])? != 2 || buf[0] != 3 || buf[1] != 33) {
        return err.DrainCFailed;
    }
    if (tbl.activeCount() != 0) return err.Expected0AfterAllDrained;
}

test("h2_pending_connect: malformed host, port=0, duplicate waiter & missing connect_id") {
    let tbl = H2PendingConnectTable.init();

    // 1. 非法主机名（空、含空格、含路径斜杠、含 '@'）
    const bad_hosts: [4]([]const u8) = ["", "bad host", "a.com/path", "user@a.com"];
    let i: usize = 0;
    while (i < bad_hosts.len) {
        let caught = false;
        * = tbl.joinOrStart(bad_hosts[i], 443, 0, 1) catch {
            caught = true;
        };
        if (!caught) return err.ExpectedInvalidHost;
        i += 1;
    }

    // 2. port == 0 拒绝
    let caught_port = false;
    * = tbl.joinOrStart("ok.example.com", 0, 0, 1) catch {
        caught_port = true;
    };
    if (!caught_port) return err.ExpectedInvalidPort;

    // 3. 同一 waiter_id 重复加入同一在途连接拒绝
    const j = tbl.joinOrStart("ok.example.com", 443, 0, 777)?;
    let caught_dup = false;
    * = tbl.joinOrStart("ok.example.com", 443, 0, 777) catch {
        caught_dup = true;
    };
    if (!caught_dup) return err.ExpectedDuplicateWaiter;

    // 4. 未知 connect_id 拒绝 drain
    let out: [4]u64 = [0; 4];
    let caught_missing = false;
    * = tbl.resolveAndDrain(j.connect_id + 999, out[0..]) catch {
        caught_missing = true;
    };
    if (!caught_missing) return err.ExpectedPendingConnectNotFound;
}

test("h2_pending_connect: waiter & connect capacity limits and BufferTooSmall protection") {
    let tbl = H2PendingConnectTable.init();

    // 1. 填满单个连接的 H2_PENDING_MAX_WAITERS (32)
    let w: u64 = 1;
    let first_id: u64 = 0;
    while (w <= (H2_PENDING_MAX_WAITERS as u64)) {
        const res = tbl.joinOrStart("busy.example.com", 443, 0, w)?;
        if (w == 1) first_id = res.connect_id;
        w += 1;
    }
    let caught_waiters = false;
    * = tbl.joinOrStart("busy.example.com", 443, 0, 9999) catch {
        caught_waiters = true;
    };
    if (!caught_waiters) return err.ExpectedTooManyWaiters;

    // 2. drain 输出缓冲过小安全报错且不破坏表内状态
    let tiny: [4]u64 = [0; 4];
    let caught_small = false;
    * = tbl.resolveAndDrain(first_id, tiny[0..]) catch {
        caught_small = true;
    };
    if (!caught_small) return err.ExpectedBufferTooSmall;
    if (tbl.activeCount() != 1) return err.SmallBufShouldNotDropEntry;

    // 3. 填满 H2_PENDING_MAX_CONNECTS (32) 个不同端口的在途连接
    let p: u16 = 1000;
    while (tbl.activeCount() < H2_PENDING_MAX_CONNECTS) {
        * = tbl.joinOrStart("pool.example.com", p, 0, (p as u64) + 10000)?;
        p += 1;
    }
    let caught_connects = false;
    * = tbl.joinOrStart("pool.example.com", 9999, 0, 88888) catch {
        caught_connects = true;
    };
    if (!caught_connects) return err.ExpectedTooManyPendingConnects;
}

test("h2_pending_connect: full lifecycle round-trip after drain allows fresh handshake") {
    let tbl = H2PendingConnectTable.init();
    const r1 = tbl.joinOrStart("reopen.example.com", 443, 1, 10)?;
    let out: [2]u64 = [0; 2];
    * = tbl.resolveAndDrain(r1.connect_id, out[0..])?;

    // 上一轮握手完成后，同目标新请求重新分配新的 connect_id
    const r2 = tbl.joinOrStart("reopen.example.com", 443, 1, 11)?;
    if (r2.kind != H2ConnectJoinKind.InitiatedNew || r2.connect_id <= r1.connect_id) {
        return err.FreshReconnectFailed;
    }
}
