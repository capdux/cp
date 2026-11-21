// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Exponential Smoothing: SES, Holt Linear/Damped & Holt-Winters Seasonal)

export {
    SeasonalityKind,
    SesResult,
    HoltConfig,
    HoltResult,
    HoltWintersConfig,
    HoltWintersResult,
    ForecastMetrics,
    sesForecast,
    holtForecast,
    holtWintersForecast,
    evaluateForecast,
} from "./holt.cp";
