// Source: native-cp-stdlib (OpenMetrics / Prometheus Text Format)

pub const om = @import("./openmetrics.cp");

pub const MetricType = om.MetricType;
pub const Label = om.Label;
pub const Sample = om.Sample;
pub const MetricFamily = om.MetricFamily;
pub const MetricsDocument = om.MetricsDocument;

pub fn parse(alloc: std.mem.Allocator, text: []const u8): MetricsDocument! {
    return MetricsDocument.parse(alloc, text);
}
