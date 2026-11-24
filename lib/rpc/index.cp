// Source: cp/lib/rpc/index.cp (Unified Exports for cap:rpc)
// Pure CP JSON-RPC 2.0, Content-Length/NDJSON Framing & MCP Core Server Protocol Engine.

export {
    ERR_PARSE,
    ERR_INVALID_REQUEST,
    ERR_METHOD_NOT_FOUND,
    ERR_INVALID_PARAMS,
    ERR_INTERNAL,
    JsonRpcCodecError,
    JsonRpcId,
    JsonRpcError,
    JsonRpcRequest,
    JsonRpcResponse,
    JsonRpcCodec,
} from "./jsonrpc.cp";

export {
    RpcVec16u8,
    DEFAULT_MAX_FRAME_BYTES,
    DEFAULT_MAX_HEADER_BYTES,
    simdRpcFindCrlf,
    scalarRpcFindCrlf,
    simdRpcAsciiLowerEql,
    scalarRpcAsciiLowerEql,
    FramingError,
    ContentLengthFramer,
    LineRpcFramer,
} from "./framing.cp";

export {
    MCP_MAX_TOOLS,
    MCP_MAX_RESOURCES,
    McpError,
    McpToolHandler,
    McpToolSpec,
    McpResourceSpec,
    McpServerInfo,
    McpServer,
} from "./mcp.cp";
