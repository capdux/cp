// Source: cp/lib/metrics/primitives.cp (Prometheus Counter, Gauge & Label Validation)
// Pure CP atomic observability primitives & Prometheus/OpenMetrics name/label validators.

const std = @import("std");

/// Error set shared across `cap:metrics` primitives, histograms, and registry.
pub err MetricsError {
    InvalidMetricName,
    InvalidLabelName,
    DuplicateMetric,
    InvalidBounds,
    InvalidQuantile,
    InvalidValue,
    OutOfMemory,
}

/// Key-value label pair attached to a Prometheus metric time series.
pub struct MetricLabel {
    pub key: []const u8 = "";
    pub value: []const u8 = "";

    pub static fn init(key: []const u8, value: []const u8): MetricLabel {
        return MetricLabel{
            key: key,
            value: value,
        };
    }
}

/// Prometheus/OpenMetrics metric and label name validator.
pub struct MetricNameValidator {
    /// Validates Prometheus metric name against `[a-zA-Z_:][a-zA-Z0-9_:]*`.
    pub static fn isValidMetricName(name: []const u8): bool {
        if (name.len == 0) return false;
        const b0 = name[0];
        const first_ok = (b0 >= 'a' && b0 <= 'z') || (b0 >= 'A' && b0 <= 'Z') || b0 == '_' || b0 == ':';
        if (!first_ok) return false;

        let i: usize = 1;
        while (i < name.len) : (i += 1) {
            const b = name[i];
            const ok = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_' || b == ':';
            if (!ok) return false;
        }
        return true;
    }

    /// Validates Prometheus label name against `[a-zA-Z_][a-zA-Z0-9_]*`.
    pub static fn isValidLabelName(name: []const u8): bool {
        if (name.len == 0) return false;
        const b0 = name[0];
        const first_ok = (b0 >= 'a' && b0 <= 'z') || (b0 >= 'A' && b0 <= 'Z') || b0 == '_';
        if (!first_ok) return false;

        let i: usize = 1;
        while (i < name.len) : (i += 1) {
            const b = name[i];
            const ok = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_';
            if (!ok) return false;
        }
        return true;
    }

    /// Validates a metric name, returning `MetricsError.InvalidMetricName` on failure.
    pub static fn validateMetricName(name: []const u8): void ! MetricsError {
        if (!MetricNameValidator.isValidMetricName(name)) {
            return MetricsError.InvalidMetricName;
        }
    }

    /// Validates a label name, returning `MetricsError.InvalidLabelName` on failure.
    pub static fn validateLabelName(name: []const u8): void ! MetricsError {
        if (!MetricNameValidator.isValidLabelName(name)) {
            return MetricsError.InvalidLabelName;
        }
    }

    /// Validates all label keys in a slice and rejects duplicate label keys.
    pub static fn validateLabels(labels: []const MetricLabel): void ! MetricsError {
        let i: usize = 0;
        while (i < labels.len) : (i += 1) {
            if (!MetricNameValidator.isValidLabelName(labels[i].key)) {
                return MetricsError.InvalidLabelName;
            }
            let j: usize = i + 1;
            while (j < labels.len) : (j += 1) {
                if (std.mem.eql(u8, labels[i].key, labels[j].key)) {
                    return MetricsError.InvalidLabelName;
                }
            }
        }
    }
}

/// Lock-free atomic monotonically increasing counter (`u64`).
/// Works both standalone and as a handle bound to a `MetricsRegistry` entry.
pub struct Counter {
    local_val: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
    shared_ptr: (*std.atomic.Value(u64)!)? = null,

    /// Creates an initialized standalone counter starting at `0`.
    pub static fn init(): Counter {
        return Counter{
            local_val: std.atomic.Value(u64).init(0),
            shared_ptr: null,
        };
    }

    /// Creates a registry-backed counter view pointing to a stable atomic cell.
    pub static fn fromAtomicPtr(ptr: *std.atomic.Value(u64)!): Counter {
        return Counter{
            local_val: std.atomic.Value(u64).init(0),
            shared_ptr: ptr,
        };
    }

    /// Atomically increments the counter by `1`.
    pub fn inc(*_) {
        if (ptr := _.shared_ptr) {
            unsafe {
                * = ptr.fetchAdd(1, .monotonic);
            }
        } else {
            * = _.local_val.fetchAdd(1, .monotonic);
        }
    }

    /// Atomically adds `delta` to the counter.
    pub fn add(*_, delta: u64) {
        if (delta == 0) return;
        if (ptr := _.shared_ptr) {
            unsafe {
                * = ptr.fetchAdd(delta, .monotonic);
            }
        } else {
            * = _.local_val.fetchAdd(delta, .monotonic);
        }
    }

    /// Atomically loads the current counter value.
    pub fn get(): u64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.load(.monotonic);
            }
        }
        return _.local_val.load(.monotonic);
    }

    /// Atomically resets the counter value back to `0`.
    pub fn reset(*_) {
        if (ptr := _.shared_ptr) {
            unsafe {
                ptr.store(0, .monotonic);
            }
        } else {
            _.local_val.store(0, .monotonic);
        }
    }
}

/// Lock-free atomic numeric gauge (`f64` stored via IEEE-754 bitcast CAS on `u64`).
/// Works both standalone and as a handle bound to a `MetricsRegistry` entry.
pub struct Gauge {
    local_bits: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
    shared_ptr: (*std.atomic.Value(u64)!)? = null,

    /// Creates an initialized standalone gauge starting at `0.0`.
    pub static fn init(): Gauge {
        return Gauge{
            local_bits: std.atomic.Value(u64).init(0),
            shared_ptr: null,
        };
    }

    /// Creates an initialized standalone gauge with initial value `val`.
    pub static fn initWith(val: f64): Gauge {
        const init_bits: u64 = @as(u64, @bitCast(val));
        return Gauge{
            local_bits: std.atomic.Value(u64).init(init_bits),
            shared_ptr: null,
        };
    }

    /// Creates a registry-backed gauge view pointing to a stable atomic cell.
    pub static fn fromAtomicPtr(ptr: *std.atomic.Value(u64)!): Gauge {
        return Gauge{
            local_bits: std.atomic.Value(u64).init(0),
            shared_ptr: ptr,
        };
    }

    /// Atomically sets the gauge to `val`.
    pub fn set(*_, val: f64) {
        const bits: u64 = @as(u64, @bitCast(val));
        if (ptr := _.shared_ptr) {
            unsafe {
                ptr.store(bits, .release);
            }
        } else {
            _.local_bits.store(bits, .release);
        }
    }

    /// Atomically adds `delta` to the gauge using a CAS loop.
    pub fn add(*_, delta: f64) {
        if (ptr := _.shared_ptr) {
            unsafe {
                let cur_bits = ptr.load(.monotonic);
                while (true) {
                    const cur_val: f64 = @as(f64, @bitCast(cur_bits));
                    const next_val: f64 = cur_val + delta;
                    const next_bits: u64 = @as(u64, @bitCast(next_val));
                    if (actual := ptr.cmpxchgWeak(cur_bits, next_bits, .acq_rel, .monotonic)) {
                        cur_bits = actual;
                    } else {
                        break;
                    }
                }
            }
        } else {
            let cur_bits = _.local_bits.load(.monotonic);
            while (true) {
                const cur_val: f64 = @as(f64, @bitCast(cur_bits));
                const next_val: f64 = cur_val + delta;
                const next_bits: u64 = @as(u64, @bitCast(next_val));
                if (actual := _.local_bits.cmpxchgWeak(cur_bits, next_bits, .acq_rel, .monotonic)) {
                    cur_bits = actual;
                } else {
                    break;
                }
            }
        }
    }

    /// Atomically subtracts `delta` from the gauge.
    pub fn sub(*_, delta: f64) {
        _.add(-delta);
    }

    /// Atomically increments the gauge by `1.0`.
    pub fn inc(*_) {
        _.add(1.0);
    }

    /// Atomically decrements the gauge by `1.0`.
    pub fn dec(*_) {
        _.add(-1.0);
    }

    /// Atomically loads the current gauge value.
    pub fn get(): f64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                const bits = ptr.load(.acquire);
                return @as(f64, @bitCast(bits));
            }
        }
        const bits = _.local_bits.load(.acquire);
        return @as(f64, @bitCast(bits));
    }

    /// Atomically resets the gauge value to `0.0`.
    pub fn reset(*_) {
        _.set(0.0);
    }
}
