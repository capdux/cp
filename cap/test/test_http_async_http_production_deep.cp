import { test, expect } from "cap:test";
import {
    ASYNC_HTTP_DEFAULT_MAX_SIMULTANEOUS,
    AsyncHttpState,
    AsyncHttpOptions,
    AsyncHttpConcurrencyLimiter,
    AsyncHttpTask,
} from "cap:http";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("http async_http: happy-path task lifecycle state transitions") {
    let task = AsyncHttpTask.init(1, "https://api.example.com/v1/chat", AsyncHttpOptions.defaults())?;
    expect(task.isHttps());
    expect(task.state == .pending);
    expect(!task.state.isActive() && !task.state.isTerminal());
    expect(eqBytes(task.state.asLabel(), "pending"));

    expect((task.schedule()?) == .scheduled);
    expect(task.state.isActive());

    expect((task.beginSend()?) == .sending);
    expect(task.state.isActive());
    expect(eqBytes(task.state.asLabel(), "sending"));

    expect((task.completeSuccess(200)?) == .success);
    expect(task.state.isTerminal() && !task.state.isActive());
    expect(task.status_code == 200);
    expect(eqBytes(task.state.asLabel(), "success"));
}

test("http async_http: failure and bounded retry exhaustion") {
    let opts = AsyncHttpOptions.defaults();
    opts.max_retries = 2;
    let task = AsyncHttpTask.init(42, "http://127.0.0.1:8080/health", opts)?;
    expect(!task.isHttps());

    // 第 1 次尝试在 scheduled 阶段失败并重试
    * = task.schedule()?;
    * = task.completeFail()?;
    expect(task.state == .fail && task.state.isTerminal());
    * = task.resetForRetry()?;
    expect(task.state == .pending && task.attempt == 1);

    // 第 2 次尝试在 sending 阶段失败并重试
    * = task.schedule()?;
    * = task.beginSend()?;
    * = task.completeFail()?;
    * = task.resetForRetry()?;
    expect(task.state == .pending && task.attempt == 2);

    // 第 3 次失败后达到 max_retries=2，再次 resetForRetry 返回 RetryExhausted
    * = task.schedule()?;
    * = task.beginSend()?;
    * = task.completeFail()?;
    let exhausted: usize = 0;
    * = task.resetForRetry() catch { exhausted += 1; AsyncHttpState.fail; };
    expect(exhausted == 1);
}

test("http async_http: invalid state transition guards") {
    let task = AsyncHttpTask.init(7, "https://cp.dev", AsyncHttpOptions.defaults())?;
    let blocked: usize = 0;

    // pending 不能直接 beginSend / completeSuccess / completeFail / resetForRetry
    * = task.beginSend() catch { blocked += 1; AsyncHttpState.pending; };
    * = task.completeSuccess(200) catch { blocked += 1; AsyncHttpState.pending; };
    * = task.completeFail() catch { blocked += 1; AsyncHttpState.pending; };
    * = task.resetForRetry() catch { blocked += 1; AsyncHttpState.pending; };

    // 正常走到 success 后不能再次 schedule 或 resetForRetry
    * = task.schedule()?;
    * = task.beginSend()?;
    * = task.completeSuccess(204)?;
    * = task.schedule() catch { blocked += 1; AsyncHttpState.success; };
    * = task.resetForRetry() catch { blocked += 1; AsyncHttpState.success; };

    expect(blocked == 6);
}

test("http async_http: options helpers and conflicting transport guard") {
    let opts = AsyncHttpOptions.defaults();
    expect(opts.shouldKeepAlive());
    expect(opts.shouldDecompress());
    expect(!opts.shouldUseProxy());
    expect(!opts.shouldUseUnixSocket());
    expect(opts.reject_unauthorized);

    opts.http_proxy = "http://proxy.internal:3128";
    opts.disable_decompression = true;
    expect(opts.shouldUseProxy());
    expect(!opts.shouldDecompress());
    expect(opts.validate()?);

    // 同时设置 http_proxy 和 unix_socket_path 触发 ConflictingTransport
    opts.unix_socket_path = "/var/run/docker.sock";
    let conflict_err: usize = 0;
    * = opts.validate() catch { conflict_err += 1; false; };
    * = AsyncHttpTask.init(9, "http://localhost/", opts) catch { conflict_err += 1; AsyncHttpTask.init(9, "http://localhost/", AsyncHttpOptions.defaults())?; };
    expect(conflict_err == 2);
}

test("http async_http: concurrency limiter quota, peak tracking and underflow guard") {
    const def_lim = AsyncHttpConcurrencyLimiter.initDefault();
    expect(def_lim.max_simultaneous == ASYNC_HTTP_DEFAULT_MAX_SIMULTANEOUS);
    expect(def_lim.remainingHeadroom() == 256);

    let lim = AsyncHttpConcurrencyLimiter.init(2)?;
    expect(lim.canDispatchNow());
    expect(lim.tryAcquire()?);
    expect(lim.tryAcquire()?);
    expect(!lim.canDispatchNow());
    expect(lim.remainingHeadroom() == 0);
    expect(lim.peak_active == 2);
    expect(lim.total_dispatched == 2);

    // 满载时触发 ConcurrencyLimitReached 并计入 queued_count
    let limit_err: usize = 0;
    * = lim.tryAcquire() catch { limit_err += 1; false; };
    expect(limit_err == 1);
    expect(lim.queued_count == 1);

    // 释放槽位并防下溢
    expect((lim.release()?) == 1);
    expect(lim.queued_count == 0);
    expect((lim.release()?) == 0);

    let underflow_err: usize = 0;
    * = lim.release() catch { underflow_err += 1; 0; };
    expect(underflow_err == 1);
}

test("http async_http: BUN_CONFIG_MAX_HTTP_REQUESTS env parser and URL guards") {
    expect((AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("256")?) == 256);
    expect((AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("  1024\n")?) == 1024);
    expect((AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("65536")?) == 65536);

    let rejected: usize = 0;
    * = AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("") catch { rejected += 1; 0; };
    * = AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("0") catch { rejected += 1; 0; };
    * = AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("-10") catch { rejected += 1; 0; };
    * = AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("abc") catch { rejected += 1; 0; };
    * = AsyncHttpConcurrencyLimiter.parseMaxRequestsEnv("65537") catch { rejected += 1; 0; };
    * = AsyncHttpConcurrencyLimiter.init(0) catch { rejected += 1; AsyncHttpConcurrencyLimiter.initDefault(); };

    // 非法 URL 拒绝
    const def_opts = AsyncHttpOptions.defaults();
    * = AsyncHttpTask.init(1, "", def_opts) catch { rejected += 1; AsyncHttpTask.init(1, "http://ok", def_opts)?; };
    * = AsyncHttpTask.init(2, "ftp://example.com/file", def_opts) catch { rejected += 1; AsyncHttpTask.init(1, "http://ok", def_opts)?; };
    * = AsyncHttpTask.init(3, "http://", def_opts) catch { rejected += 1; AsyncHttpTask.init(1, "http://ok", def_opts)?; };
    * = AsyncHttpTask.init(4, "https://", def_opts) catch { rejected += 1; AsyncHttpTask.init(1, "http://ok", def_opts)?; };

    expect(rejected == 10);
}
