// Source: cp/lib/perf/types.cp (PerfPhase, PerfEvent, PerfSpan, PerfError, PerfRecorderOptions)

pub const PerfError = error{
    EmptyName,
    InvalidSpan,
    UnpairedEnd,
    CapacityExceeded,
    OutOfMemory,
};

pub enum PerfPhase {
    Begin,
    End,
    Complete,
    Instant,
    Counter,
    Metadata,

    pub fn toChar(): u8 {
        return switch (_) {
            .Begin => 'B',
            .End => 'E',
            .Complete => 'X',
            .Instant => 'i',
            .Counter => 'C',
            .Metadata => 'M',
        };
    }
}

pub struct PerfEvent {
    name: []const u8,
    category: []const u8,
    phase: PerfPhase,
    ts_ns: u64,
    dur_ns: u64,
    pid: u32,
    tid: u32,
    value: i64,
}

pub struct PerfSpan {
    name: []const u8,
    category: []const u8,
    start_ns: u64,
    pid: u32,
    tid: u32,
    depth: u32,
    active: bool,
}

pub struct PerfRecorderOptions {
    max_events: usize = 0,
    max_open_spans: usize = 256,
    default_pid: u32 = 1,
    default_tid: u32 = 1,
    default_category: []const u8 = "perf",
}
