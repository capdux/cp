// Origin: Pure-CP Stdlib (Zero-Alloc Classical IIR Filter Design: Butterworth, Chebyshev-I Bilinear SOS Cascade & Zero-Phase filtfilt) | Created: 2026-11-22

export {
    SosSection,
    SosState,
    designButterworthLowpass,
    designButterworthHighpass,
    designChebyshev1Lowpass,
    sosFilter,
    sosFiltFilt,
    sosMagnitudeDb,
} from "./iir.cp";
