// Source: Absorbed from /data/dev/bun/src/http_jsc/websocket_client/ | Synced: 2026-11-20T11:39:17Z
// ============================================================================
// Absorbed from Bun: bun/src/ws/
// Upstream Sync: Refer to bun/src/ws/ for future upstream updates.
// ============================================================================
// lib/ws/index.cp
export { Opcode, CloseCode } from "./opcode";
export { Mask } from "./mask";
export { FrameHeader, FrameBuilder, ClosePayload } from "./frame";
export {
    WS_MAGIC_GUID,
    WebSocketUpgradeRequest,
    WebSocketUpgradeResponse,
    WebSocketHandshake,
} from "./handshake";
export {
    WebSocketMessageKind,
    WebSocketMessage,
    AssembleResult,
    WebSocketMessageAssembler,
} from "./assembler";
export {
    SubprotocolList,
    SubprotocolNegotiator,
    WsExtensionParam,
    WsExtensionHeader,
    WsExtensionList,
    CloseCodeValidator,
} from "./subprotocol";
export {
    PMD_SYNC_TRAILER,
    PerMessageDeflateConfig,
    PerMessageDeflateNegotiation,
    PerMessageDeflate,
} from "./permessage_deflate";
export {
    WsSessionState,
    WsSessionConfig,
    WsSessionResult,
    WsSession,
} from "./session";
export {
    WS_PUBSUB_MAX_TOPICS,
    WS_PUBSUB_MAX_SUBSCRIBERS_PER_TOPIC,
    WsPubSubError,
    WsPubSubConfig,
    WsPublishResult,
    WsPubSubBroker,
} from "./pubsub";
export {
    WS_CLIENT_MAX_CUSTOM_HEADERS,
    WS_CLIENT_MAX_PROTOCOLS,
    WsClientUpgradeError,
    WsUrlScheme,
    WsUpgradePhase,
    WsClientUrl,
    WsClientHeader,
    WsClientUpgradeConfig,
    WsClientUpgradeMachine,
} from "./client_upgrade";

import { FrameBuilder } from "./frame";
import { Mask } from "./mask";
import { WebSocketHandshake } from "./handshake";

pub const ws = struct {
    pub const buildText = FrameBuilder.buildText;
    pub const buildBinary = FrameBuilder.buildBinary;
    pub const buildContinue = FrameBuilder.buildContinue;
    pub const buildFragment = FrameBuilder.buildFragment;
    pub const buildClose = FrameBuilder.buildClose;
    pub const buildCloseCode = FrameBuilder.buildCloseCode;
    pub const buildPing = FrameBuilder.buildPing;
    pub const buildPong = FrameBuilder.buildPong;
    pub const buildPongReply = FrameBuilder.buildPongReply;
    pub const applyMask = Mask.apply;
    pub const computeAcceptKey = WebSocketHandshake.computeAcceptKey;
};
