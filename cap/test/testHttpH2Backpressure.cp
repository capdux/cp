import { test, expect } from "cap:test";
import {
    H2_LOCAL_INITIAL_WINDOW_SIZE,
    H2_LOCAL_MAX_HEADER_LIST_SIZE,
    H2_WRITE_BUFFER_HIGH_WATER,
    H2_WRITE_BUFFER_CONTROL_LIMIT,
    H2BackpressureError,
    H2WriteAdmission,
    H2WriteBufferBudget,
    H2ClientMetrics,
} from "cap:http";

test("H2WriteBufferBudget default thresholds & hysteresis backpressure resume", () => {
    expect(H2_LOCAL_INITIAL_WINDOW_SIZE).toBe(16 * 1024 * 1024);
    expect(H2_LOCAL_MAX_HEADER_LIST_SIZE).toBe(256 * 1024);
    expect(H2_WRITE_BUFFER_HIGH_WATER).toBe(256 * 1024);
    expect(H2_WRITE_BUFFER_CONTROL_LIMIT).toBe(1024 * 1024);

    let b = H2WriteBufferBudget.initDefault();
    expect(b.canAcceptData()).toBe(true);
    expect(b.remainingDataBudget()).toBe(256 * 1024);
    expect(b.utilizationPercent()).toBe(0);

    // Enqueue below high_water
    const a1 = b.queueData(100 * 1024);
    expect(a1 == H2WriteAdmission.Allow).toBe(true);
    expect(b.canAcceptData()).toBe(true);

    // Cross high_water -> ApplyBackpressure
    const a2 = b.queueData(200 * 1024);
    expect(a2 == H2WriteAdmission.ApplyBackpressure).toBe(true);
    expect(b.backpressure_active).toBe(true);
    expect(b.backpressure_events).toBe(1);
    expect(b.canAcceptData()).toBe(false);
    expect(b.remainingDataBudget()).toBe(0);
    expect(b.peak_queued_bytes).toBe(300 * 1024);

    // Partial flush still above low_water (128 KiB) -> stays paused
    const r1 = b.onFlushed(100 * 1024);
    expect(r1).toBe(false);
    expect(b.backpressure_active).toBe(true);
    expect(b.queued_bytes).toBe(200 * 1024);

    // Flush down to <= 128 KiB -> hysteresis resumes
    const r2 = b.onFlushed(100 * 1024);
    expect(r2).toBe(true);
    expect(b.backpressure_active).toBe(false);
    expect(b.canAcceptData()).toBe(true);
});

test("H2WriteBufferBudget control frame privilege & hard limit enforcement", () => {
    let b = H2WriteBufferBudget.init(1000, 2000)?;

    // DATA fills to high_water (1000)
    expect(b.queueData(1000) == H2WriteAdmission.ApplyBackpressure).toBe(true);

    // Control frame (e.g. RST_STREAM / PING) still allowed between 1000..2000
    const ctrl_adm = b.queueControl(800)?;
    expect(ctrl_adm == H2WriteAdmission.ApplyBackpressure).toBe(true);
    expect(b.queued_bytes).toBe(1800);
    expect(b.remainingControlBudget()).toBe(200);
    expect(b.utilizationPercent()).toBe(90);

    // Control frame exceeding hard limit -> ControlBufferExceeded
    let got_ctrl_err = false;
    * = b.queueControl(201) catch |e| {
        got_ctrl_err = (e == H2BackpressureError.ControlBufferExceeded);
        H2WriteAdmission.DropConnection
    };
    expect(got_ctrl_err).toBe(true);

    // DATA exceeding hard limit -> DropConnection
    expect(b.queueData(201) == H2WriteAdmission.DropConnection).toBe(true);
});

test("H2WriteBufferBudget invalid thresholds guard", () => {
    let got_zero = false;
    * = H2WriteBufferBudget.init(0, 1024) catch |e| {
        got_zero = (e == H2BackpressureError.InvalidThresholds);
        H2WriteBufferBudget.initDefault()
    };
    expect(got_zero).toBe(true);

    let got_inv = false;
    * = H2WriteBufferBudget.init(2048, 1024) catch |e| {
        got_inv = (e == H2BackpressureError.InvalidThresholds);
        H2WriteBufferBudget.initDefault()
    };
    expect(got_inv).toBe(true);
});

test("H2ClientMetrics session/stream tracking, peaks & underflow guard", () => {
    let m = H2ClientMetrics.init();
    expect(m.isIdle()).toBe(true);

    expect(m.onSessionOpened()).toBe(1);
    expect(m.onSessionOpened()).toBe(2);
    expect(m.onStreamOpened()).toBe(1);
    expect(m.onStreamOpened()).toBe(2);
    expect(m.onStreamOpened()).toBe(3);
    expect(m.isIdle()).toBe(false);
    expect(m.peak_sessions).toBe(2);
    expect(m.peak_streams).toBe(3);

    expect(m.onStreamClosed()?).toBe(2);
    expect(m.onStreamClosed()?).toBe(1);
    expect(m.onStreamClosed()?).toBe(0);
    expect(m.onSessionClosed()?).toBe(1);
    expect(m.onSessionClosed()?).toBe(0);
    expect(m.isIdle()).toBe(true);
    expect(m.total_sessions_opened).toBe(2);
    expect(m.total_streams_opened).toBe(3);

    // Underflow guards
    let got_sess_uf = false;
    * = m.onSessionClosed() catch |e| {
        got_sess_uf = (e == H2BackpressureError.CounterUnderflow);
        0
    };
    expect(got_sess_uf).toBe(true);

    let got_stream_uf = false;
    * = m.onStreamClosed() catch |e| {
        got_stream_uf = (e == H2BackpressureError.CounterUnderflow);
        0
    };
    expect(got_stream_uf).toBe(true);
});

test("H2WriteBufferBudget saturating flush clamps cleanly to zero", () => {
    let b = H2WriteBufferBudget.init(512, 1024)?;
    * = b.queueData(600);
    expect(b.backpressure_active).toBe(true);
    const resumed = b.onFlushed(99999);
    expect(resumed).toBe(true);
    expect(b.queued_bytes).toBe(0);
    expect(b.backpressure_active).toBe(false);
    expect(b.utilizationPercent()).toBe(0);
});
