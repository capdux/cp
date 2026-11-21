// Source: Absorbed from /data/dev/bun/src/http/http.cp (RFC 9110 Upgrade & Connection Header Parser)

export {
    UpgradeProtocol,
    UpgradeIter,
    ConnectionTokenIter,
    connectionHasToken,
    upgradeHasProtocol,
    isHopByHopHeader,
    isWebSocketUpgrade,
} from "./upgrade.cp";
