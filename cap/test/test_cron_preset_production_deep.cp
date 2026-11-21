import {
    CronPreset,
    parseCronPreset,
    presetToCron5,
    normalizeCron5,
} from "cap:cron";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("CronPreset parsing, expansion, and normalizeCron5", () => {
    expect(parseCronPreset("@annually").? == CronPreset.yearly);
    expect(parseCronPreset("@MIDNIGHT").? == CronPreset.daily);
    expect(parseCronPreset("@reboot").? == CronPreset.reboot);
    expect(presetToCron5(CronPreset.reboot) == null);

    expect(strEq(normalizeCron5("@hourly").?, "0 * * * *"));
    expect(strEq(normalizeCron5("@daily").?, "0 0 * * *"));
    expect(strEq(normalizeCron5("@weekly").?, "0 0 * * 0"));
    expect(strEq(normalizeCron5("@monthly").?, "0 0 1 * *"));
    expect(strEq(normalizeCron5("@yearly").?, "0 0 1 1 *"));
    expect(strEq(normalizeCron5("  */5 * * * *  ").?, "*/5 * * * *"));

    expect(normalizeCron5("@reboot") == null);
    expect(normalizeCron5("@unknown") == null);
    expect(normalizeCron5("* * *") == null);
});
