// Origin: Pure-CP Stdlib (Zero-Alloc Discrete HMM: Log-Space Viterbi, Rabiner Scaled Forward-Backward & Baum-Welch EM) | Created: 2026-11-22

export {
    HmmModel,
    ViterbiResult,
    viterbiDecode,
    forwardScaled,
    backwardScaled,
    posteriorStates,
    baumWelchStep,
} from "./hmm.cp";
