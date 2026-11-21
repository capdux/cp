import {
    LoadAvg,
    SystemUptime,
} from "cap:sys";
import { test, expect } from "cap:test";

test("LoadAvg parses /proc/loadavg and checks CPU overload", () => {
    const la = LoadAvg.parse("4.25 2.10 0.95 3/512 48123\n").?;
    expect(la.load1_centi == 425);
    expect(la.load5_centi == 210);
    expect(la.load15_centi == 95);
    expect(la.running_procs == 3);
    expect(la.total_procs == 512);
    expect(la.last_pid == 48123);

    expect(la.isOverloaded(4));
    expect(!la.isOverloaded(8));

    expect(LoadAvg.parse("0.10 0.20") == null);
    expect(LoadAvg.parse("0.10 0.20 0.30 badprocs 123") == null);
});

test("SystemUptime parses /proc/uptime into milliseconds", () => {
    const up = SystemUptime.parse("12345.67 98765.40\n").?;
    expect(up.uptime_ms == 12345670);
    expect(up.idle_ms == 98765400);
    expect(up.uptimeSeconds() == 12345);

    expect(SystemUptime.parse("only_one") == null);
});
