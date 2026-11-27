// @cap/property-information — Zero-allocation HTML/SVG attribute <-> DOM property mapping
// Clean-room CP implementation inspired by property-information (MIT).

import { eqIgnoreCase } from "cap:ascii";

pub struct PropInfo {
    property: string,
    attribute: string,
    is_boolean: bool,
    is_space_separated: bool,
    is_comma_separated: bool,
    is_numeric: bool,
    found: bool,
}

fn pinfMake(
    property: string,
    attribute: string,
    is_boolean: bool,
    is_space_separated: bool,
    is_comma_separated: bool,
    is_numeric: bool,
): PropInfo {
    return PropInfo{
        property: property,
        attribute: attribute,
        is_boolean: is_boolean,
        is_space_separated: is_space_separated,
        is_comma_separated: is_comma_separated,
        is_numeric: is_numeric,
        found: true,
    };
}

fn pinfNotFound(name: string): PropInfo {
    return PropInfo{
        property: name,
        attribute: name,
        is_boolean: false,
        is_space_separated: false,
        is_comma_separated: false,
        is_numeric: false,
        found: false,
    };
}

fn pinfMatchPair(name: string, prop: string, attr: string): bool {
    if (eqIgnoreCase(name, attr)) return true;
    if (eqIgnoreCase(name, prop)) return true;
    return false;
}

pub fn findHtmlProp(name: string): PropInfo {
    if (name.len == 0) return pinfNotFound(name);

    // Core HTML attribute <-> DOM property remappings
    if (pinfMatchPair(name, "className", "class")) return pinfMake("className", "class", false, true, false, false);
    if (pinfMatchPair(name, "htmlFor", "for")) return pinfMake("htmlFor", "for", false, true, false, false);
    if (pinfMatchPair(name, "tabIndex", "tabindex")) return pinfMake("tabIndex", "tabindex", false, false, false, true);
    if (pinfMatchPair(name, "readOnly", "readonly")) return pinfMake("readOnly", "readonly", true, false, false, false);
    if (pinfMatchPair(name, "maxLength", "maxlength")) return pinfMake("maxLength", "maxlength", false, false, false, true);
    if (pinfMatchPair(name, "minLength", "minlength")) return pinfMake("minLength", "minlength", false, false, false, true);
    if (pinfMatchPair(name, "colSpan", "colspan")) return pinfMake("colSpan", "colspan", false, false, false, true);
    if (pinfMatchPair(name, "rowSpan", "rowspan")) return pinfMake("rowSpan", "rowspan", false, false, false, true);
    if (pinfMatchPair(name, "autoFocus", "autofocus")) return pinfMake("autoFocus", "autofocus", true, false, false, false);
    if (pinfMatchPair(name, "autoPlay", "autoplay")) return pinfMake("autoPlay", "autoplay", true, false, false, false);
    if (pinfMatchPair(name, "noValidate", "novalidate")) return pinfMake("noValidate", "novalidate", true, false, false, false);
    if (pinfMatchPair(name, "formNoValidate", "formnovalidate")) return pinfMake("formNoValidate", "formnovalidate", true, false, false, false);
    if (pinfMatchPair(name, "allowFullScreen", "allowfullscreen")) return pinfMake("allowFullScreen", "allowfullscreen", true, false, false, false);
    if (pinfMatchPair(name, "playsInline", "playsinline")) return pinfMake("playsInline", "playsinline", true, false, false, false);
    if (pinfMatchPair(name, "isMap", "ismap")) return pinfMake("isMap", "ismap", true, false, false, false);
    if (pinfMatchPair(name, "noModule", "nomodule")) return pinfMake("noModule", "nomodule", true, false, false, false);
    if (pinfMatchPair(name, "itemScope", "itemscope")) return pinfMake("itemScope", "itemscope", true, false, false, false);
    if (pinfMatchPair(name, "contentEditable", "contenteditable")) return pinfMake("contentEditable", "contenteditable", false, false, false, false);
    if (pinfMatchPair(name, "crossOrigin", "crossorigin")) return pinfMake("crossOrigin", "crossorigin", false, false, false, false);
    if (pinfMatchPair(name, "autoComplete", "autocomplete")) return pinfMake("autoComplete", "autocomplete", false, true, false, false);
    if (pinfMatchPair(name, "accessKey", "accesskey")) return pinfMake("accessKey", "accesskey", false, true, false, false);
    if (pinfMatchPair(name, "acceptCharset", "accept-charset")) return pinfMake("acceptCharset", "accept-charset", false, true, false, false);
    if (pinfMatchPair(name, "httpEquiv", "http-equiv")) return pinfMake("httpEquiv", "http-equiv", false, true, false, false);
    if (pinfMatchPair(name, "srcSet", "srcset")) return pinfMake("srcSet", "srcset", false, false, true, false);
    if (pinfMatchPair(name, "imageSrcSet", "imagesrcset")) return pinfMake("imageSrcSet", "imagesrcset", false, false, true, false);
    if (pinfMatchPair(name, "srcDoc", "srcdoc")) return pinfMake("srcDoc", "srcdoc", false, false, false, false);
    if (pinfMatchPair(name, "useMap", "usemap")) return pinfMake("useMap", "usemap", false, false, false, false);
    if (pinfMatchPair(name, "dateTime", "datetime")) return pinfMake("dateTime", "datetime", false, false, false, false);
    if (pinfMatchPair(name, "encType", "enctype")) return pinfMake("encType", "enctype", false, false, false, false);
    if (pinfMatchPair(name, "spellCheck", "spellcheck")) return pinfMake("spellCheck", "spellcheck", false, false, false, false);
    if (pinfMatchPair(name, "referrerPolicy", "referrerpolicy")) return pinfMake("referrerPolicy", "referrerpolicy", false, false, false, false);
    if (pinfMatchPair(name, "viewBox", "viewBox")) return pinfMake("viewBox", "viewBox", false, false, false, false);
    if (pinfMatchPair(name, "preserveAspectRatio", "preserveAspectRatio")) return pinfMake("preserveAspectRatio", "preserveAspectRatio", false, false, false, false);

    // Boolean HTML attributes
    if (eqIgnoreCase(name, "disabled")) return pinfMake("disabled", "disabled", true, false, false, false);
    if (eqIgnoreCase(name, "checked")) return pinfMake("checked", "checked", true, false, false, false);
    if (eqIgnoreCase(name, "selected")) return pinfMake("selected", "selected", true, false, false, false);
    if (eqIgnoreCase(name, "multiple")) return pinfMake("multiple", "multiple", true, false, false, false);
    if (eqIgnoreCase(name, "hidden")) return pinfMake("hidden", "hidden", true, false, false, false);
    if (eqIgnoreCase(name, "required")) return pinfMake("required", "required", true, false, false, false);
    if (eqIgnoreCase(name, "controls")) return pinfMake("controls", "controls", true, false, false, false);
    if (eqIgnoreCase(name, "loop")) return pinfMake("loop", "loop", true, false, false, false);
    if (eqIgnoreCase(name, "muted")) return pinfMake("muted", "muted", true, false, false, false);
    if (eqIgnoreCase(name, "defer")) return pinfMake("defer", "defer", true, false, false, false);
    if (eqIgnoreCase(name, "async")) return pinfMake("async", "async", true, false, false, false);
    if (eqIgnoreCase(name, "open")) return pinfMake("open", "open", true, false, false, false);
    if (eqIgnoreCase(name, "reversed")) return pinfMake("reversed", "reversed", true, false, false, false);
    if (eqIgnoreCase(name, "inert")) return pinfMake("inert", "inert", true, false, false, false);
    if (eqIgnoreCase(name, "default")) return pinfMake("default", "default", true, false, false, false);

    // Comma-separated HTML attributes
    if (eqIgnoreCase(name, "accept")) return pinfMake("accept", "accept", false, false, true, false);
    if (eqIgnoreCase(name, "coords")) return pinfMake("coords", "coords", false, false, true, false);

    // Space-separated HTML attributes
    if (eqIgnoreCase(name, "rel")) return pinfMake("rel", "rel", false, true, false, false);
    if (eqIgnoreCase(name, "sandbox")) return pinfMake("sandbox", "sandbox", false, true, false, false);
    if (eqIgnoreCase(name, "headers")) return pinfMake("headers", "headers", false, true, false, false);
    if (eqIgnoreCase(name, "ping")) return pinfMake("ping", "ping", false, true, false, false);
    if (eqIgnoreCase(name, "itemProp") || eqIgnoreCase(name, "itemprop")) return pinfMake("itemProp", "itemprop", false, true, false, false);
    if (eqIgnoreCase(name, "itemRef") || eqIgnoreCase(name, "itemref")) return pinfMake("itemRef", "itemref", false, true, false, false);
    if (eqIgnoreCase(name, "itemType") || eqIgnoreCase(name, "itemtype")) return pinfMake("itemType", "itemtype", false, true, false, false);
    if (eqIgnoreCase(name, "sizes")) return pinfMake("sizes", "sizes", false, true, false, false);
    if (eqIgnoreCase(name, "blocking")) return pinfMake("blocking", "blocking", false, true, false, false);

    // Numeric HTML attributes
    if (eqIgnoreCase(name, "rows")) return pinfMake("rows", "rows", false, false, false, true);
    if (eqIgnoreCase(name, "cols")) return pinfMake("cols", "cols", false, false, false, true);
    if (eqIgnoreCase(name, "size")) return pinfMake("size", "size", false, false, false, true);
    if (eqIgnoreCase(name, "span")) return pinfMake("span", "span", false, false, false, true);
    if (eqIgnoreCase(name, "start")) return pinfMake("start", "start", false, false, false, true);
    if (eqIgnoreCase(name, "width")) return pinfMake("width", "width", false, false, false, true);
    if (eqIgnoreCase(name, "height")) return pinfMake("height", "height", false, false, false, true);

    // Common standard attributes
    if (eqIgnoreCase(name, "id")) return pinfMake("id", "id", false, false, false, false);
    if (eqIgnoreCase(name, "href")) return pinfMake("href", "href", false, false, false, false);
    if (eqIgnoreCase(name, "src")) return pinfMake("src", "src", false, false, false, false);
    if (eqIgnoreCase(name, "alt")) return pinfMake("alt", "alt", false, false, false, false);
    if (eqIgnoreCase(name, "title")) return pinfMake("title", "title", false, false, false, false);
    if (eqIgnoreCase(name, "style")) return pinfMake("style", "style", false, false, false, false);
    if (eqIgnoreCase(name, "type")) return pinfMake("type", "type", false, false, false, false);
    if (eqIgnoreCase(name, "name")) return pinfMake("name", "name", false, false, false, false);
    if (eqIgnoreCase(name, "value")) return pinfMake("value", "value", false, false, false, false);
    if (eqIgnoreCase(name, "role")) return pinfMake("role", "role", false, true, false, false);

    // ARIA attributes (aria-*)
    if (name.len > 5 && eqIgnoreCase(name[0..5], "aria-")) {
        const is_space: bool = eqIgnoreCase(name, "aria-labelledby") ||
            eqIgnoreCase(name, "aria-describedby") ||
            eqIgnoreCase(name, "aria-controls") ||
            eqIgnoreCase(name, "aria-owns") ||
            eqIgnoreCase(name, "aria-flowto") ||
            eqIgnoreCase(name, "aria-relevant");
        return pinfMake(name, name, false, is_space, false, false);
    }

    // Custom data attributes (data-*)
    if (name.len > 5 && eqIgnoreCase(name[0..5], "data-")) {
        return pinfMake(name, name, false, false, false, false);
    }

    return pinfNotFound(name);
}

pub fn isBooleanAttr(name: string): bool {
    const info = findHtmlProp(name);
    return info.found && info.is_boolean;
}

pub fn isSpaceSeparatedAttr(name: string): bool {
    const info = findHtmlProp(name);
    return info.found && info.is_space_separated;
}

pub fn isCommaSeparatedAttr(name: string): bool {
    const info = findHtmlProp(name);
    return info.found && info.is_comma_separated;
}

pub fn attrToDomProp(attr: string): string {
    const info = findHtmlProp(attr);
    if (info.found) return info.property;
    return attr;
}

pub fn domPropToAttr(prop: string): string {
    const info = findHtmlProp(prop);
    if (info.found) return info.attribute;
    return prop;
}
