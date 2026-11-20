// Source: cp/lib/gossip/index.cp (Public module entry for cap:gossip / cap.gossip)

export {
    GossipError,
    MemberState,
    MemberInfo,
    GossipUpdate,
    SwimConfig,
    shouldSupersede,
    retransmitLimit,
} from "./types.cp";

export {
    WIRE_MAGIC_0,
    WIRE_MAGIC_1,
    WIRE_VERSION,
    MessageKind,
    GossipMessage,
    OutboundMessage,
    encodeMessage,
    decodeMessage,
    freeMessage,
} from "./wire.cp";

export {
    SwimClusterNode,
    freeOutbound,
} from "./node.cp";
