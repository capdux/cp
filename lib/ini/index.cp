// Source: cp/lib/ini/index.cp (Unified Exports for cap:ini)
// Industrial-grade pure CP INI / Systemd / Gitconfig parser, lossless incremental editor & JSON exporter.

export {
    IniError,
    IniOptions,
    IniLineKind,
    IniLine,
    IniDoc,
} from "./ini.cp";

// 向后兼容旧 cap:conf/ini 快捷接口
export { Ini, ini } from "../conf/ini.cp";
