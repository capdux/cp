// Source: cp/lib/quant/index.cp (Public module entry point for cap:quant)

export {
    QuantError,
    ScalarQuantParams,
    Int8Quantized,
    AffineQuantized,
    BinaryVector,
    PqCodebook,
} from "./types.cp";

export {
    quantizeInt8Symmetric,
    quantizeInt8Affine,
    dequantizeInt8,
    dequantizeAffine,
    dotInt8,
} from "./scalar.cp";

export {
    quantizeBinary,
    hammingDistance,
    binaryCosineApprox,
} from "./binary.cp";

export {
    trainPqKMeans,
    encodePq,
    buildAdcDistanceTable,
    adcDistance,
} from "./pq.cp";
