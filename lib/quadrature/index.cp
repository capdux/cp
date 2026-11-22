// Origin: Pure-CP Stdlib (Zero-Alloc Numerical Quadrature: Gauss-Legendre 5/10, Composite Simpson, Adaptive G7-K15 & Romberg) | Created: 2026-11-22

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
