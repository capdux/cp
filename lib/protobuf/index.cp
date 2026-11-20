// Source: cp/lib/protobuf/index.cp (Unified Exports for cap:protobuf)
// Unified Public Exports for Protocol Buffers Wire Format & gRPC Framing Engine (cap:protobuf & cap.protobuf)

export {
    MAX_FIELD_NUMBER,
    DEFAULT_MAX_DEPTH,
    ProtoError,
    WireType,
    zigzagEncode32,
    zigzagDecode32,
    zigzagEncode64,
    zigzagDecode64,
    ProtoField,
    ProtoWriter,
    ProtoReader,
} from "./protobuf.cp";

export {
    DEFAULT_MAX_MESSAGE_BYTES,
    GRPC_FRAME_HEADER_SIZE,
    GrpcError,
    GrpcStatus,
    GrpcFrame,
    GrpcFrameHeader,
    GrpcFrameCodec,
    GrpcFrameStream,
    GrpcTimeout,
} from "./grpc.cp";
