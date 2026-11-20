// Source: cp/lib/perf/index.cp (cap:perf & cap.perf public entry point)

export {
    ClockReadFn,
    ClockKind,
    PerfClock,
    PerfTimer,
    osMonotonicNs,
} from "./clock.cp";

export {
    PerfError,
    PerfPhase,
    PerfEvent,
    PerfSpan,
    PerfRecorderOptions,
} from "./types.cp";

export { PerfRecorder } from "./recorder.cp";

export {
    PerfStatSummary,
    summarizeByName,
    freeSummaries,
} from "./stats.cp";

export {
    exportChromeTraceJson,
    formatSummaryTable,
} from "./trace.cp";
