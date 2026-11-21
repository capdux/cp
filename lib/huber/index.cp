// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Robust Statistics, IRLS Huber/Tukey M-Estimation Regression & Theil-Sen 1D)

export {
    RobustKind,
    HuberRegConfig,
    HuberRegResult,
    HuberRegWorkspace,
    TheilSenResult,
    median,
    madScale,
    trimmedMean,
    huberLocation,
    huberLoss,
    huberWeight,
    tukeyBiweight,
    huberRegress,
    theilSen1D,
} from "./huber.cp";
