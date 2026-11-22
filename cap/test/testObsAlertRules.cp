// Origin: Pure-CP Stdlib Test Suite (lib/obs/alertRules.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Prometheus-Style Alert Rules Evaluator & `for` Hysteresis FSM:
//   1. 4-state hysteresis FSM (`inactive -> pending -> firing -> keep_firing -> resolved -> inactive`)
//   2. `for_duration_ms` transient spike debounce & `keep_firing_for_ms` re-spike hold reset
//   3. All 5 comparison operators (`gt | ge | lt | le | eq`) & zero-duration immediate firing
//   4. Time-bounded silence windows (by `rule_id` and by `group_hash`) with boundary precision
//   5. Parent->child alert inhibition (cluster-down suppresses downstream node alerts) & `collectFiringAlerts`
//   6. 32-rule capacity saturation, NaN/Inf sample guards, telemetry stats & `cap.obs` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    ALERT_MAX_RULES,
    AlertCompareOp,
    AlertSeverity,
    AlertState,
    AlertNotifyKind,
    AlertRuleSpec,
    FiringAlertSnapshot,
    AlertRuleEngine,
} from "cap:obs";

test("AlertRules: 4-state hysteresis FSM with for_duration_ms & keep_firing_for_ms") {
    let eng = AlertRuleEngine.init();
    expect_true(eng.addRule(AlertRuleSpec{
        rule_id: 10,
        name: "HighLatencyP99",
        op: AlertCompareOp.gt,
        threshold: 250.0,
        for_duration_ms: 5000,
        keep_firing_for_ms: 3000,
        severity: AlertSeverity.critical,
        group_hash: 0xAA,
    }) != null);

    // t=1000: healthy -> stays inactive
    const e0 = eng.evaluate(10, 120.0, 1000);
    expect_eq(e0.new_state, AlertState.inactive);
    expect_eq(e0.should_notify, false);

    // t=2000: first breach -> pending
    const e1 = eng.evaluate(10, 300.0, 2000);
    expect_eq(e1.prev_state, AlertState.inactive);
    expect_eq(e1.new_state, AlertState.pending);
    expect_eq(e1.should_notify, false);

    // t=6999: 4999ms < 5000ms -> still pending
    const e2 = eng.evaluate(10, 310.0, 6999);
    expect_eq(e2.new_state, AlertState.pending);
    expect_eq(e2.should_notify, false);

    // t=7000: 5000ms elapsed -> upgrades to firing with one-shot fire notification!
    const e3 = eng.evaluate(10, 320.0, 7000);
    expect_eq(e3.prev_state, AlertState.pending);
    expect_eq(e3.new_state, AlertState.firing);
    expect_eq(e3.notify_kind, AlertNotifyKind.fire);
    expect_true(e3.should_notify);

    // t=8000: still breaching -> stays firing, no duplicate fire notification
    const e4 = eng.evaluate(10, 330.0, 8000);
    expect_eq(e4.new_state, AlertState.firing);
    expect_eq(e4.notify_kind, AlertNotifyKind.none);
    expect_eq(e4.should_notify, false);

    // t=10000: drops below threshold -> enters keep_firing hold (still firing)
    const e5 = eng.evaluate(10, 100.0, 10000);
    expect_eq(e5.new_state, AlertState.firing);
    expect_eq(e5.notify_kind, AlertNotifyKind.none);

    // t=12999: 2999ms < 3000ms -> still held firing
    const e6 = eng.evaluate(10, 95.0, 12999);
    expect_eq(e6.new_state, AlertState.firing);

    // t=13000: 3000ms hold elapsed -> transitions to resolved with one-shot resolve notification!
    const e7 = eng.evaluate(10, 90.0, 13000);
    expect_eq(e7.prev_state, AlertState.firing);
    expect_eq(e7.new_state, AlertState.resolved);
    expect_eq(e7.notify_kind, AlertNotifyKind.resolve);
    expect_true(e7.should_notify);

    // t=14000: next healthy sample quietly returns to inactive without duplicate resolve
    const e8 = eng.evaluate(10, 85.0, 14000);
    expect_eq(e8.prev_state, AlertState.resolved);
    expect_eq(e8.new_state, AlertState.inactive);
    expect_eq(e8.notify_kind, AlertNotifyKind.none);
    expect_eq(e8.should_notify, false);
}

test("AlertRules: transient spike debounce & keep_firing_for_ms re-spike reset") {
    let eng = AlertRuleEngine.init();
    * = eng.addRule(AlertRuleSpec{
        rule_id: 20,
        name: "SpikeGuard",
        op: AlertCompareOp.ge,
        threshold: 80.0,
        for_duration_ms: 4000,
        keep_firing_for_ms: 2000,
        severity: AlertSeverity.warning,
    });

    // Transient spike clears before 4000ms -> debounced cleanly to inactive
    expect_eq(eng.evaluate(20, 80.0, 1000).new_state, AlertState.pending);
    expect_eq(eng.evaluate(20, 79.9, 3000).new_state, AlertState.inactive);
    expect_eq(eng.stats().total_fires, 0 as u64);

    // Sustained breach -> firing
    * = eng.evaluate(20, 85.0, 5000);
    expect_eq(eng.evaluate(20, 85.0, 9000).new_state, AlertState.firing);

    // Brief dip starts keep_firing hold at t=10000, then re-spikes at t=11000 resetting hold
    expect_eq(eng.evaluate(20, 50.0, 10000).new_state, AlertState.firing);
    expect_eq(eng.evaluate(20, 90.0, 11000).new_state, AlertState.firing);
    // Next dip at t=12000 needs a fresh 2000ms (resolves at t=14000, NOT t=12000)
    expect_eq(eng.evaluate(20, 50.0, 12000).new_state, AlertState.firing);
    expect_eq(eng.evaluate(20, 50.0, 13500).new_state, AlertState.firing);
    expect_eq(eng.evaluate(20, 50.0, 14000).new_state, AlertState.resolved);
}

test("AlertRules: all 5 comparison operators & zero-duration immediate firing") {
    let eng = AlertRuleEngine.init();
    * = eng.addRule(AlertRuleSpec{ rule_id: 1, name: "OpGt", op: AlertCompareOp.gt, threshold: 10.0 });
    * = eng.addRule(AlertRuleSpec{ rule_id: 2, name: "OpGe", op: AlertCompareOp.ge, threshold: 10.0 });
    * = eng.addRule(AlertRuleSpec{ rule_id: 3, name: "OpLt", op: AlertCompareOp.lt, threshold: 10.0 });
    * = eng.addRule(AlertRuleSpec{ rule_id: 4, name: "OpLe", op: AlertCompareOp.le, threshold: 10.0 });
    * = eng.addRule(AlertRuleSpec{ rule_id: 5, name: "OpEq", op: AlertCompareOp.eq, threshold: 10.0 });

    expect_eq(eng.evaluate(1, 10.0, 100).new_state, AlertState.inactive);
    expect_eq(eng.evaluate(1, 10.1, 200).new_state, AlertState.firing);
    expect_eq(eng.evaluate(2, 10.0, 100).new_state, AlertState.firing);
    expect_eq(eng.evaluate(3, 10.0, 100).new_state, AlertState.inactive);
    expect_eq(eng.evaluate(3, 9.9, 200).new_state, AlertState.firing);
    expect_eq(eng.evaluate(4, 10.0, 100).new_state, AlertState.firing);
    expect_eq(eng.evaluate(5, 10.0, 100).new_state, AlertState.firing);
    expect_eq(eng.evaluate(5, 11.0, 200).new_state, AlertState.resolved);
}

test("AlertRules: time-bounded silence windows (by rule_id and by group_hash)") {
    const GRP_DB: u64 = 0xDB01;
    let eng = AlertRuleEngine.init();
    * = eng.addRule(AlertRuleSpec{ rule_id: 1, name: "DbDisk", op: AlertCompareOp.gt, threshold: 80.0, group_hash: GRP_DB });
    * = eng.addRule(AlertRuleSpec{ rule_id: 2, name: "ApiErr", op: AlertCompareOp.gt, threshold: 5.0, group_hash: 0xAB02 });

    // Silence group GRP_DB during [1000, 5000) and rule 2 during [2000, 3000)
    const s1 = eng.addSilence(GRP_DB, true, 1000, 5000) ?? 99;
    expect_true(s1 < 32);
    expect_true(eng.addSilence(2, false, 2000, 3000) != null);
    expect_true(eng.addSilence(1, false, 5000, 5000) == null); // invalid empty interval

    // Inside silence window: rule 1 fires internally but notification is suppressed
    const r1 = eng.evaluate(1, 95.0, 2500);
    expect_eq(r1.new_state, AlertState.firing);
    expect_true(r1.silenced);
    expect_eq(r1.should_notify, false);

    // At exact end_ms=5000, silence window has expired -> resolve notifies normally
    const r1_res = eng.evaluate(1, 40.0, 5000);
    expect_eq(r1_res.new_state, AlertState.resolved);
    expect_eq(r1_res.silenced, false);
    expect_true(r1_res.should_notify);

    // Rule 2 silenced at t=2500
    const r2 = eng.evaluate(2, 10.0, 2500);
    expect_true(r2.silenced);
    expect_eq(r2.should_notify, false);
    expect_eq(eng.stats().total_suppressed, 2 as u64);
}

test("AlertRules: parent->child inhibition & collectFiringAlerts severity ordering") {
    let eng = AlertRuleEngine.init();
    * = eng.addRule(AlertRuleSpec{ rule_id: 100, name: "ClusterDown", op: AlertCompareOp.eq, threshold: 0.0, severity: AlertSeverity.critical });
    * = eng.addRule(AlertRuleSpec{ rule_id: 101, name: "NodeCpuHigh", op: AlertCompareOp.gt, threshold: 90.0, severity: AlertSeverity.warning });
    * = eng.addRule(AlertRuleSpec{ rule_id: 102, name: "PodRestart", op: AlertCompareOp.gt, threshold: 3.0, severity: AlertSeverity.info });

    // ClusterDown (100) inhibits NodeCpuHigh (101)
    expect_true(eng.addInhibition(100, 101) != null);
    expect_true(eng.addInhibition(100, 100) == null); // self-inhibition rejected
    expect_true(eng.addInhibition(100, 101) == null); // duplicate rejected

    // Trigger all 3 alerts
    expect_true(eng.evaluate(100, 0.0, 1000).should_notify);
    const child = eng.evaluate(101, 98.0, 1000);
    expect_eq(child.new_state, AlertState.firing);
    expect_true(child.inhibited);
    expect_eq(child.should_notify, false);
    expect_true(eng.evaluate(102, 5.0, 1000).should_notify);

    // collectFiringAlerts(include_suppressed=false) returns [ClusterDown(critical), PodRestart(info)]
    let out: [4]FiringAlertSnapshot = [FiringAlertSnapshot{}; 4];
    let out_s: []FiringAlertSnapshot = out[0..];
    const n_unsup = eng.collectFiringAlerts(1000, false, *out_s);
    expect_eq(n_unsup, 2 as usize);
    expect_eq(out[0].rule_id, 100 as u32);
    expect_eq(out[1].rule_id, 102 as u32);

    // collectFiringAlerts(include_suppressed=true) returns all 3 sorted: critical(100) > warning(101) > info(102)
    const n_all = eng.collectFiringAlerts(1000, true, *out_s);
    expect_eq(n_all, 3 as usize);
    expect_eq(out[0].rule_id, 100 as u32);
    expect_eq(out[1].rule_id, 101 as u32);
    expect_true(out[1].inhibited);
    expect_eq(out[2].rule_id, 102 as u32);

    // Once ClusterDown resolves, NodeCpuHigh is no longer inhibited
    * = eng.evaluate(100, 1.0, 2000);
    expect_eq(eng.isInhibited(101), false);
}

test("AlertRules: 32-rule capacity saturation, NaN guard & cap.obs facade parity") {
    let eng = cap.obs.AlertRuleEngine.init();
    expect_true(eng.addRule(AlertRuleSpec{ rule_id: 0, name: "Zero", op: AlertCompareOp.gt, threshold: 1.0 }) == null);
    expect_true(eng.addRule(AlertRuleSpec{ rule_id: 1, name: "", op: AlertCompareOp.gt, threshold: 1.0 }) == null);

    let i: u32 = 1;
    while (i <= (ALERT_MAX_RULES as u32)) : (i += 1) {
        expect_true(eng.addRule(AlertRuleSpec{
            rule_id: i,
            name: "Rule",
            op: AlertCompareOp.gt,
            threshold: 10.0,
        }) != null);
    }
    expect_eq(eng.stats().rule_count, ALERT_MAX_RULES);
    expect_true(eng.addRule(AlertRuleSpec{ rule_id: 99, name: "Overflow", op: AlertCompareOp.gt, threshold: 10.0 }) == null);

    // NaN sample rejected safely without changing state
    const nan_val: f64 = 0.0 / 0.0;
    const bad = eng.evaluate(1, nan_val, 1000);
    expect_true(bad.found);
    expect_eq(bad.valid_sample, false);
    expect_eq(bad.new_state, AlertState.inactive);

    // Unknown rule returns found == false
    expect_eq(eng.evaluate(999, 50.0, 1000).found, false);

    // Remove rule and reuse slot
    expect_true(eng.removeRule(1));
    expect_eq(eng.stats().rule_count, ALERT_MAX_RULES - 1);
    expect_true(eng.addRule(AlertRuleSpec{ rule_id: 100, name: "Reused", op: AlertCompareOp.gt, threshold: 5.0 }) != null);
}
