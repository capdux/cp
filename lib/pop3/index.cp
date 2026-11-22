// Origin: Pure-CP Stdlib (Public Unified Export for cap:pop3) | Created: 2026-11-22

export {
    POP3_TCP_PORT,
    POP3S_TCP_PORT,
    POP3_MAX_CMD_LEN,
    POP3_MAX_LINE_LEN,
    isPop3ResponsePrefix,
    Pop3Response,
    parsePop3Response,
    Pop3Stat,
    parsePop3Stat,
    Pop3ListItem,
    parsePop3ListLine,
    Pop3UidlItem,
    parsePop3UidlLine,
    Pop3Command,
    parsePop3Command,
    Pop3MultiLineIter,
    encodePop3Command,
    pop3DotStuff,
} from "./pop3.cp";
