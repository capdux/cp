// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Support Vector Machine: Platt SMO Kernel SVM & Pegasos Linear SVM)

export {
    KernelKind,
    KernelParams,
    SmoConfig,
    SmoResult,
    LinearSvmResult,
    evalKernel,
    svmFitSmo,
    svmDecision,
    svmPredict,
    linearSvmFit,
    linearSvmDecision,
} from "./svm.cp";
