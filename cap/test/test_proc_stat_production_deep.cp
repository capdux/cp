import {
    ProcState,
    ProcStat,
} from "cap:proc_stat";
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

test("ProcStat parses standard line and comm containing spaces and parentheses", () => {
    const line = "4215 (Web Content (1)) S 1000 4215 1000 0 -1 4194304 1234 0 0 0 150 45 0 0 20 0 8 0 98765 104857600 2560 18446744073709551615\n";
    const st = ProcStat.parse(line).?;
    expect(st.pid == 4215);
    expect(strEq(st.comm, "Web Content (1)"));
    expect(st.state == ProcState.sleeping);
    expect(st.ppid == 1000);
    expect(st.pgrp == 4215);
    expect(st.session == 1000);
    expect(st.utime_ticks == 150);
    expect(st.stime_ticks == 45);
    expect(st.totalCpuTicks() == 195);
    expect(st.num_threads == 8);
    expect(st.starttime_ticks == 98765);
    expect(st.vsize_bytes == 104857600);
    expect(st.rss_pages == 2560);
    expect(st.rssBytes(4096) == 10485760);
    expect(!st.isZombie());

    const z = ProcStat.parse("99 (defunct) Z 1 99 1 0 0 0 0 0 0 0 0 0 0 0 20 0 1 0 100 0 0").?;
    expect(z.isZombie());

    expect(ProcStat.parse("truncated (a) S 1") == null);
    expect(ProcStat.parse("badpid (a) S 1 1 1") == null);
});
