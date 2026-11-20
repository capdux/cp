// Source: cp/lib/timeseries/index.cp (Unified Exports for cap:timeseries)
// Pure CP zero-C-dependency time-series storage, aligned rollup downsampling,
// rate with counter-reset handling, SMA/EMA smoothing, and Z-score anomaly detection.

export {
    TimeSeriesError,
    AggOp,
    DataPoint,
    AnomalyPoint,
    SeriesSummary,
    TimeSeries,
} from "./series.cp";
