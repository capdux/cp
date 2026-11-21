// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Numerical Quadrature: Gauss-Legendre 5/10, Composite Simpson, Adaptive G7-K15 & Romberg)

export {
    IntegrandFn,
    Gk15Estimate,
    QuadResult,
    gaussLegendre5,
    gaussLegendre10,
    compositeSimpson,
    gaussKronrod15,
    adaptiveGk15,
    romberg,
} from "./quadrature.cp";
