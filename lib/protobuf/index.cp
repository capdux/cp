// Source: cp/lib/protobuf/index.cp (Unified Exports for cap:protobuf)
// Unified Public Exports for Protocol Buffers v3 Wire Format & gRPC Framing Engine (cap:protobuf & cap.protobuf)

export {
    ProtoError,
    WireType,
    ZigZag,
    MAX_FIELD_NUMBER,
    RESERVED_FIELD_START,
    RESERVED_FIELD_END,
    ProtoTag,
    ProtoWriter,
    ProtoReader,
} from "./wire.cp";

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
