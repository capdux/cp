// Source: cp/lib/onnx/index.cp (Public exports for cap:onnx & cap.onnx)
export {
    MAX_PROTO_DEPTH,
    OnnxError,
    PbWireType,
    PbTag,
    PbWireReader,
    PbWireWriter,
} from "./wire.cp";

export {
    OnnxDataType,
    OnnxDim,
    OnnxValueInfo,
    OnnxAttrType,
    OnnxAttrValue,
    OnnxAttribute,
    OnnxNode,
    OnnxTensor,
    OnnxOpsetImport,
    OnnxGraph,
    OnnxModel,
} from "./ir.cp";

export {
    encodeOnnxModel,
    decodeOnnxModel,
    validateGraphAcyclic,
} from "./codec.cp";
