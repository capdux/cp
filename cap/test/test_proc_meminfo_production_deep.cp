import { MemInfo } from "cap:sys";
import { test, expect } from "cap:test";

const SAMPLE_MEMINFO =
    "MemTotal:       16384000 kB\n" ++
    "MemFree:         2048000 kB\n" ++
    "MemAvailable:    8192000 kB\n" ++
    "Buffers:          512000 kB\n" ++
    "Cached:          4096000 kB\n" ++
    "SwapTotal:       4096000 kB\n" ++
    "SwapFree:        3072000 kB\n" ++
    "Dirty:              1024 kB\n" ++
    "HugePages_Total:       0\n";

test("MemInfo parses kB fields to bytes and computes used/swap/percent", () => {
    const m = MemInfo.parse(SAMPLE_MEMINFO);
    expect(m.mem_total == 16384000 * 1024);
    expect(m.availableBytes() == 8192000 * 1024);
    expect(m.usedBytes() == 8192000 * 1024);
    expect(m.usedPercent() == 50);
    expect(m.swapUsedBytes() == 1024000 * 1024);
    expect(m.dirty == 1024 * 1024);

    // Fallback when MemAvailable is absent (older kernels)
    const legacy = "MemTotal: 1000 kB\nMemFree: 200 kB\nBuffers: 100 kB\nCached: 300 kB\n";
    const ml = MemInfo.parse(legacy);
    expect(!ml.has_mem_available);
    expect(ml.availableBytes() == 600 * 1024);
    expect(ml.usedBytes() == 400 * 1024);
    expect(ml.usedPercent() == 40);
});
