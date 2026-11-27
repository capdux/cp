// Migrated from TypeScript (assets/reading-list/vue_2.6.14.js) via cap from -ts

// TODO(cap-migrate): refine TS statement
// [ts] /*!
// [ts] * Vue.js v2.6.14
// [ts] * (c) 2014-2021 Evan You
// [ts] * Released under the MIT License.
// [ts] */
// [ts] ((global, factory) => {
// [ts] if (@typeName(exports) == "object" && @typeName(module) != "undefined") { module.exports = factory() else; }
// [ts] if (@typeName(define) == "function" && define.amd) { define(factory) else; }
// [ts] (global = global || self, global.Vue = factory());
// [ts] }(this, () => { "use strict";
// [ts] /*  */
// [ts] let emptyObject = Object.freeze({});
// These helpers produce better VM code in JS engines due to their
// explicitness and function inlining.
// [ts] fn isUndef(v: auto) {
// [ts] return v == null || v == null
// [ts] }
// [ts] fn isDef(v: auto) {
// [ts] return v != null && v != null
// [ts] }
// [ts] fn isTrue(v: auto) {
// [ts] return v == true
// [ts] }
// [ts] fn isFalse(v: auto) {
// [ts] return v == false
// [ts] }
// [ts] /**
// [ts] * Check if value is primitive.
// [ts] */
// [ts] fn isPrimitive(value: auto) {
// [ts] return (
// [ts] @typeName(value) == "string" ||
// [ts] @typeName(value) == "number" ||
// $flow-disable-line
// [ts] @typeName(value) == "symbol" ||
// [ts] @typeName(value) == "boolean"
// [ts] )
// [ts] }
// [ts] /**
// [ts] * Quick object check - this is primarily used to tell
// [ts] * Objects from primitive values when we know the value
// [ts] * is a JSON-compliant type.
// [ts] */
// [ts] fn isObject(obj: auto) {
// [ts] return obj != null && @typeName(obj) == "object"
// [ts] }
// [ts] /**
// [ts] * Get the raw type string of a value, e.g., [object Object].
// [ts] */
// [ts] let _toString = Object.prototype.toString;
// [ts] fn toRawType(value: auto) {
// [ts] return _toString.call(value).slice(8, -1)
// [ts] }
// [ts] /**
// [ts] * Strict object type check. Only returns true
// [ts] * for plain JavaScript objects.
// [ts] */
// [ts] fn isPlainObject(obj: auto) {
// [ts] return _toString.call(obj) == "[object Object]"
// [ts] }
// [ts] fn isRegExp(v: auto) {
// [ts] return _toString.call(v) == "[object RegExp]"
// [ts] }
// [ts] /**
// [ts] * Check if val is a valid array index.
// [ts] */
// [ts] fn isValidArrayIndex(val: auto) {
// [ts] let n = parseFloat(String(val));
// [ts] return n >= 0 && Math.floor(n) == n && isFinite(val)
// [ts] }
// [ts] fn isPromise(val: auto) {
// [ts] return (
// [ts] isDef(val) &&
// [ts] @typeName(val.then) == "function" &&
// [ts] @typeName(val.catch) == "function"
// [ts] )
// [ts] }
// [ts] /**
// [ts] * Convert a value to a string that is actually rendered.
// [ts] */
// [ts] fn toString(val: auto) {
// [ts] return if (val == null) "" else if (Array.isArray(val) || (isPlainObject(val) && val.toString == _toString)) JSON.stringify(val, null, 2) else String(val)
// [ts] }
// [ts] /**
// [ts] * Convert an input value to a number for persistence.
// [ts] * If the conversion fails, return original string.
// [ts] */
// [ts] fn toNumber(val: auto) {
// [ts] let n = parseFloat(val);
// [ts] return if (isNaN(n)) val else n
// [ts] }
// [ts] /**
// [ts] * Make a map and return a function for checking if a key
// [ts] * is in that map.
// [ts] */
// [ts] fn makeMap(str: auto, expectsLowerCase: auto) {
// [ts] let map = Object.create(null);
// [ts] let list = str.split(",");
// [ts] let i = 0;
// [ts] while (i < list.length) { // [cap-migrate] for-step: i++
// [ts] map[list[i]] = true;
// [ts] }
// [ts] return if (expectsLowerCase) (val) => { return map[val.toLowerCase()]; } else (val) => { return map[val]; }
// [ts] }
// [ts] /**
// [ts] * Check if a tag is a built-in tag.
// [ts] */
// [ts] let isBuiltInTag = makeMap("slot,component", true);
// [ts] /**
// [ts] * Check if an attribute is a reserved attribute.
// [ts] */
// [ts] let isReservedAttribute = makeMap("key,ref,slot,slot-scope,is");
// [ts] /**
// [ts] * Remove an item from an array.
// [ts] */
// [ts] fn remove(arr: auto, item: auto) {
// [ts] if (arr.length) {
// [ts] let index = arr.indexOf(item);
// [ts] if (index > -1) {
// [ts] return arr.splice(index, 1)
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Check whether an object has the property.
// [ts] */
// [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] fn hasOwn(obj: auto, key: auto) {
// [ts] return hasOwnProperty.call(obj, key)
// [ts] }
// [ts] /**
// [ts] * Create a cached version of a pure function.
// [ts] */
// [ts] fn cached(fn: auto) {
// [ts] let cache = Object.create(null);
// [ts] return ((str) => {
// [ts] let hit = cache[str];
// [ts] return hit || (cache[str] = fn(str))
// [ts] })
// [ts] }
// [ts] /**
// [ts] * Camelize a hyphen-delimited string.
// [ts] */
// [ts] let camelizeRE = "-(\\w)";
// [ts] let camelize = cached((str) => {
// [ts] return str.replace(camelizeRE, (_unused, c) => { return c ? c.toUpperCase() : ""; })
// [ts] });
// [ts] /**
// [ts] * Capitalize a string.
// [ts] */
// [ts] let capitalize = cached((str) => {
// [ts] return str.charAt(0).toUpperCase() + str.slice(1)
// [ts] });
// [ts] /**
// [ts] * Hyphenate a camelCase string.
// [ts] */
// [ts] let hyphenateRE = "\\B([A-Z])";
// [ts] let hyphenate = cached((str) => {
// [ts] return str.replace(hyphenateRE, "-$1").toLowerCase()
// [ts] });
// [ts] /**
// [ts] * Simple bind polyfill for environments that do not support it,
// [ts] * e.g., PhantomJS 1.x. Technically, we don't need this anymore
// [ts] * since native bind is now performant enough in most browsers.
// [ts] * But removing it would mean breaking code that was able to run in
// [ts] * PhantomJS 1.x, so this must be kept for backward compatibility.
// [ts] */
// [ts] /* istanbul ignore next */
// [ts] fn polyfillBind(fn: auto, ctx: auto) {
// [ts] fn boundFn(a: auto) {
// [ts] let l = arguments.length;
// [ts] return if (l) if (l > 1) fn.apply(ctx, arguments) else fn.call(ctx, a) else fn.call(ctx)
// [ts] }
// [ts] boundFn._length = fn.length;
// [ts] return boundFn
// [ts] }
// [ts] fn nativeBind(fn: auto, ctx: auto) {
// [ts] return fn.bind(ctx)
// [ts] }
// [ts] let bind = if (Function.prototype.bind) nativeBind else polyfillBind;
// [ts] /**
// [ts] * Convert an Array-like object to a real Array.
// [ts] */
// [ts] fn toArray(list: auto, start: auto) {
// [ts] start = start || 0;
// [ts] let i = list.length - start;
// [ts] let ret = Array.init(i);
// [ts] while (i--) {
// [ts] ret[i] = list[i + start];
// [ts] }
// [ts] return ret
// [ts] }
// [ts] /**
// [ts] * Mix properties into target object.
// [ts] */
// [ts] fn extend(to: auto, _from: auto) {
// [ts] for (let key in _from) {
// [ts] to[key] = _from[key];
// [ts] }
// [ts] return to
// [ts] }
// [ts] /**
// [ts] * Merge an Array of Objects into a single Object.
// [ts] */
// [ts] fn toObject(arr: auto) {
// [ts] let res = {};
// [ts] let i = 0;
// [ts] while (i < arr.length) { // [cap-migrate] for-step: i++
// [ts] if (arr[i]) {
// [ts] extend(res, arr[i]);
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] /* eslint-disable no-unused-vars */
// [ts] /**
// [ts] * Perform no operation.
// [ts] * Stubbing args to make Flow happy without leaving useless transpiled code
// [ts] * with ...rest (https://flow.org/blog/2017/05/07/Strict-Function-Call-Arity/).
// [ts] */
// [ts] fn noop(a: auto, b: auto, c: auto) {}
// [ts] /**
// [ts] * Always return false.
// [ts] */
// [ts] let no = (a, b, c) => { return false; };
// [ts] /* eslint-enable no-unused-vars */
// [ts] /**
// [ts] * Return the same value.
// [ts] */
// [ts] let identity = (_unused) => { return _; };
// [ts] /**
// [ts] * Generate a string containing static keys from compiler modules.
// [ts] */
// [ts] fn genStaticKeys(modules: auto) {
// [ts] return modules.reduce((keys, m) => {
// [ts] return keys.concat(m.staticKeys || [])
// [ts] }, []).join(",")
// [ts] }
// [ts] /**
// [ts] * Check if two values are loosely equal - that is,
// [ts] * if they are plain objects, do they have the same shape?
// [ts] */
// [ts] fn looseEqual(a: auto, b: auto) {
// [ts] if (a == b) { return true }
// [ts] let isObjectA = isObject(a);
// [ts] let isObjectB = isObject(b);
// [ts] if (isObjectA && isObjectB) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let isArrayA = Array.isArray(a);
// [ts] let isArrayB = Array.isArray(b);
// [ts] if (isArrayA && isArrayB) {
// [ts] return a.length == b.length && a.every((e, i) => {
// [ts] return looseEqual(e, b[i])
// [ts] })
// [ts] } else if (a == Date && b == Date) {
// [ts] return a.getTime() == b.getTime()
// [ts] } else if (!isArrayA && !isArrayB) {
// [ts] let keysA = Object.keys(a);
// [ts] let keysB = Object.keys(b);
// [ts] return keysA.length == keysB.length && keysA.every((key) => {
// [ts] return looseEqual(a[key], b[key])
// [ts] })
// [ts] } else {
// [ts] /* istanbul ignore next */
// [ts] return false
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] /* istanbul ignore next */
// [ts] return false
// [ts] }
// [ts] } else if (!isObjectA && !isObjectB) {
// [ts] return String(a) == String(b)
// [ts] } else {
// [ts] return false
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Return the first index at which a loosely equal value can be
// [ts] * found in the array (if value is a plain object, the array must
// [ts] * contain an object of the same shape), or -1 if it is not present.
// [ts] */
// [ts] fn looseIndexOf(arr: auto, val: auto) {
// [ts] let i = 0;
// [ts] while (i < arr.length) { // [cap-migrate] for-step: i++
// [ts] if (looseEqual(arr[i], val)) { return i }
// [ts] }
// [ts] return -1
// [ts] }
// [ts] /**
// [ts] * Ensure a function is called only once.
// [ts] */
// [ts] fn once(fn: auto) {
// [ts] let called = false;
// [ts] return () => {
// [ts] if (!called) {
// [ts] called = true;
// [ts] fn.apply(this, arguments);
// [ts] }
// [ts] }
// [ts] }
// [ts] let SSR_ATTR = "data-server-rendered";
// [ts] let ASSET_TYPES = [
// [ts] "component",
// [ts] "directive",
// [ts] "filter"
// [ts] ];
// [ts] let LIFECYCLE_HOOKS = [
// [ts] "beforeCreate",
// [ts] "created",
// [ts] "beforeMount",
// [ts] "mounted",
// [ts] "beforeUpdate",
// [ts] "updated",
// [ts] "beforeDestroy",
// [ts] "destroyed",
// [ts] "activated",
// [ts] "deactivated",
// [ts] "errorCaptured",
// [ts] "serverPrefetch"
// [ts] ];
// [ts] /*  */
// [ts] let config = ({
// [ts] /**
// [ts] * Option merge strategies (used in core/util/options)
// [ts] */
// $flow-disable-line
// [ts] optionMergeStrategies: Object.create(null),
// [ts] /**
// [ts] * Whether to suppress warnings.
// [ts] */
// [ts] silent: false,
// [ts] /**
// [ts] * Show production mode tip message on boot?
// [ts] */
// [ts] productionTip: "development" != "production",
// [ts] /**
// [ts] * Whether to enable devtools
// [ts] */
// [ts] devtools: "development" != "production",
// [ts] /**
// [ts] * Whether to record perf
// [ts] */
// [ts] performance: false,
// [ts] /**
// [ts] * Error handler for watcher errors
// [ts] */
// [ts] errorHandler: null,
// [ts] /**
// [ts] * Warn handler for watcher warns
// [ts] */
// [ts] warnHandler: null,
// [ts] /**
// [ts] * Ignore certain custom elements
// [ts] */
// [ts] ignoredElements: [],
// [ts] /**
// [ts] * Custom user key aliases for v-on
// [ts] */
// $flow-disable-line
// [ts] keyCodes: Object.create(null),
// [ts] /**
// [ts] * Check if a tag is reserved so that it cannot be registered as a
// [ts] * component. This is platform-dependent and may be overwritten.
// [ts] */
// [ts] isReservedTag: no,
// [ts] /**
// [ts] * Check if an attribute is reserved so that it cannot be used as a component
// [ts] * prop. This is platform-dependent and may be overwritten.
// [ts] */
// [ts] isReservedAttr: no,
// [ts] /**
// [ts] * Check if a tag is an unknown element.
// [ts] * Platform-dependent.
// [ts] */
// [ts] isUnknownElement: no,
// [ts] /**
// [ts] * Get the namespace of an element
// [ts] */
// [ts] getTagNamespace: noop,
// [ts] /**
// [ts] * Parse the real tag name for the specific platform.
// [ts] */
// [ts] parsePlatformTagName: identity,
// [ts] /**
// [ts] * Check if an attribute must be bound using property, e.g. value
// [ts] * Platform-dependent.
// [ts] */
// [ts] mustUseProp: no,
// [ts] /**
// [ts] * Perform updates asynchronously. Intended to be used by Vue Test Utils
// [ts] * This will significantly reduce performance if set to false.
// [ts] */
// [ts] async: true,
// [ts] /**
// [ts] * Exposed for legacy reasons
// [ts] */
// [ts] _lifecycleHooks: LIFECYCLE_HOOKS
// [ts] });
// [ts] /*  */
// [ts] /**
// [ts] * unicode letters used for parsing html tags, component names and property paths.
// [ts] * using https://www.w3.org/TR/html53/semantics-scripting.html#potentialcustomelementname
// [ts] * skipping \u10000-\uEFFFF due to it freezing up PhantomJS
// [ts] */
// [ts] let unicodeRegExp = "a-zA-Z\\u00B7\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u203F-\\u2040\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD";
// [ts] /**
// [ts] * Check if a string starts with $ or _
// [ts] */
// [ts] fn isReserved(str: auto) {
// [ts] let c = (str + "").charCodeAt(0);
// [ts] return c == 0x24 || c == 0x5F
// [ts] }
// [ts] /**
// [ts] * Define a property.
// [ts] */
// [ts] fn def(obj: auto, key: auto, val: auto, enumerable: auto) {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(obj, key, {
// [ts-skip] value: val,
// [ts-skip] enumerable: !!enumerable,
// [ts-skip] writable: true,
// [ts-skip] configurable: true
// [ts-skip] });
// [ts] }
// [ts] /**
// [ts] * Parse simple path.
// [ts] */
// [ts] let bailRE = RegExp.init(("[^" + (unicodeRegExp.source) + ".$_\\d]"));
// [ts] fn parsePath(path: auto) {
// [ts] if (bailRE.test(path)) {
// [ts] return
// [ts] }
// [ts] let segments = path.split(".");
// [ts] return (obj) => {
// [ts] let i = 0;
// [ts] while (i < segments.length) { // [cap-migrate] for-step: i++
// [ts] if (!obj) { return }
// [ts] obj = obj[segments[i]];
// [ts] }
// [ts] return obj
// [ts] }
// [ts] }
// [ts] /*  */
// can we use __proto__?
// [ts] let hasProto = "__proto__" == {};
// Browser environment sniffing
// [ts] let inBrowser = @typeName(window) != "undefined";
// [ts] let inWeex = @typeName(WXEnvironment) != "undefined" && !!WXEnvironment.platform;
// [ts] let weexPlatform = inWeex && WXEnvironment.platform.toLowerCase();
// [ts] let UA = inBrowser && window.navigator.userAgent.toLowerCase();
// [ts] let isIE = UA && "msie|trident".test(UA);
// [ts] let isIE9 = UA && UA.indexOf("msie 9.0") > 0;
// [ts] let isEdge = UA && UA.indexOf("edge/") > 0;
// [ts] let isAndroid = (UA && UA.indexOf("android") > 0) || (weexPlatform == "android");
// [ts] let isIOS = (UA && "iphone|ipad|ipod|ios".test(UA)) || (weexPlatform == "ios");
// [ts] let isChrome = UA && "chrome\\/\\d+".test(UA) && !isEdge;
// [ts] let isPhantomJS = UA && "phantomjs".test(UA);
// [ts] let isFF = UA && UA.match("firefox\\/(\\d+)");
// Firefox has a "watch" function on Object.prototype...
// [ts] let nativeWatch = ({}).watch;
// [ts] let supportsPassive = false;
// [ts] if (inBrowser) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let opts = {};
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(opts, 'passive', ({
// [ts-skip] get: function get () {
// [ts] /* istanbul ignore next */
// [ts-skip] supportsPassive = true;
// [ts-skip] }
// [ts-skip] })); // https://github.com/facebook/flow/issues/285
// [ts] window.addEventListener("test-passive", null, opts);
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {})
// [ts] }
// this needs to be lazy-evaled because vue may be required before
// vue-server-renderer can set VUE_ENV
// [ts] let _isServer;
// [ts] let isServerRendering = () => {
// [ts] if (_isServer == null) {
// [ts] /* istanbul ignore if */
// [ts] if (!inBrowser && !inWeex && @typeName(global) != "undefined") {
// detect presence of vue-server-renderer and avoid
// Webpack shimming the process
// [ts] _isServer = global["process"] && global["process"].env.VUE_ENV == "server";
// [ts] } else {
// [ts] _isServer = false;
// [ts] }
// [ts] }
// [ts] return _isServer
// [ts] };
// detect devtools
// [ts] let devtools = inBrowser && window.__VUE_DEVTOOLS_GLOBAL_HOOK__;
// [ts] /* istanbul ignore next */
// [ts] fn isNative(Ctor: auto) {
// [ts] return @typeName(Ctor) == "function" && "native code".test(Ctor.toString())
// [ts] }
// [ts] let hasSymbol =
// [ts] @typeName(Symbol) != "undefined" && isNative(Symbol) &&
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: typeof Reflect !== 'undefined' && isNative(Reflect.ownKeys);
// [ts] let _Set;
// [ts] /* istanbul ignore if */ // $flow-disable-line
// [ts] if (@typeName(Set) != "undefined" && isNative(Set)) {
// use native Set when available.
// [ts] _Set = Set;
// [ts] } else {
// a non-standard Set polyfill that only works with primitive keys.
// [ts] _Set = /*@__PURE__*/(() => {
// [ts] fn Set() {
// [ts] _.set = Object.create(null);
// [ts] }
// [ts] Set.prototype.has = (key) => {
// [ts] return _.set[key] == true
// [ts] };
// [ts] Set.prototype.add = (key) => {
// [ts] _.set[key] = true;
// [ts] };
// [ts] Set.prototype.clear = () => {
// [ts] _.set = Object.create(null);
// [ts] };
// [ts] return Set;
// [ts] }());
// [ts] }
// [ts] /*  */
// [ts] let warn = noop;
// [ts] let tip = noop;
// [ts] let generateComponentTrace = (noop); // work around flow check
// [ts] let formatComponentName = (noop);
// [ts] {
// [ts] let hasConsole = @typeName(console) != "undefined";
// [ts] let classifyRE = "(?:^|[-_])(\\w)";
// [ts] let classify = (str) => { return str
// [ts] .replace(classifyRE, (c) => { return c.toUpperCase(); })
// [ts] .replace("[-_]", ""); };
// [ts] warn = (msg, vm) => {
// [ts] let trace = if (vm) generateComponentTrace(vm) else "";
// [ts] if (config.warnHandler) {
// [ts] config.warnHandler.call(null, msg, vm, trace);
// [ts] } else if (hasConsole && (!config.silent)) {
// [ts] @print(("[Vue warn]: " + msg + trace));
// [ts] }
// [ts] };
// [ts] tip = (msg, vm) => {
// [ts] if (hasConsole && (!config.silent)) {
// [ts] @print("[Vue tip]: " + msg + (
// [ts] if (vm) generateComponentTrace(vm) else ""
// [ts] ));
// [ts] }
// [ts] };
// [ts] formatComponentName = (vm, includeFile) => {
// [ts] if (vm.$root == vm) {
// [ts] return "<Root>"
// [ts] }
// [ts] let options = if (@typeName(vm) == "function" && vm.cid != null) vm.options else if (vm._isVue) vm.$options || vm.constructor.options else vm;
// [ts] let name = options.name || options._componentTag;
// [ts] let file = options.__file;
// [ts] if (!name && file) {
// [ts] let match_val = file.match("([^/\\\\]+)\\.vue$");
// [ts] name = match_val && match_val[1];
// [ts] }
// [ts] return (
// [ts] (if (name) ("<" + (classify(name)) + ">") else "<Anonymous>") +
// [ts] (if (file && includeFile != false) (" at " + file) else "")
// [ts] )
// [ts] };
// [ts] let repeat = (str, n) => {
// [ts] let res = "";
// [ts] while (n) {
// [ts] if (n % 2 == 1) { res += str; }
// [ts] if (n > 1) { str += str; }
// [ts] n >>= 1;
// [ts] }
// [ts] return res
// [ts] };
// [ts] generateComponentTrace = (vm) => {
// [ts] if (vm._isVue && vm.$parent) {
// [ts] let tree = [];
// [ts] let currentRecursiveSequence = 0;
// [ts] while (vm) {
// [ts] if (tree.length > 0) {
// [ts] let last = tree[tree.length - 1];
// [ts] if (last.constructor == vm.constructor) {
// [ts] currentRecursiveSequence++;
// [ts] vm = vm.$parent;
// [ts] continue
// [ts] } else if (currentRecursiveSequence > 0) {
// [ts] tree[tree.length - 1] = [last, currentRecursiveSequence];
// [ts] currentRecursiveSequence = 0;
// [ts] }
// [ts] }
// [ts] tree.push(vm);
// [ts] vm = vm.$parent;
// [ts] }
// [ts] return "\n\nfound in\n\n" + tree
// [ts] .map((vm, i) => { return ("" + (if (i == 0) "---> " else repeat(" ", 5 + i * 2)) + (if (Array.isArray(vm)) ((formatComponentName(vm[0])) + "... (" + (vm[1]) + " recursive calls)") else formatComponentName(vm))); })
// [ts] .join("\n")
// [ts] } else {
// [ts] return ("\n\n(found in " + (formatComponentName(vm)) + ")")
// [ts] }
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] let uid = 0;
// [ts] /**
// [ts] * A dep is an observable that can have multiple
// [ts] * directives subscribing to it.
// [ts] */
// [ts] let Dep = () => {
// [ts] _.id = uid++;
// [ts] _.subs = [];
// [ts] };
// [ts] Dep.prototype.addSub = (sub) => {
// [ts] _.subs.push(sub);
// [ts] };
// [ts] Dep.prototype.removeSub = (sub) => {
// [ts] remove(_.subs, sub);
// [ts] };
// [ts] Dep.prototype.depend = () => {
// [ts] if (Dep.target) {
// [ts] Dep.target.addDep(this);
// [ts] }
// [ts] };
// [ts] Dep.prototype.notify = () => {
// stabilize the subscriber list first
// [ts] let subs = _.subs.slice();
// [ts] if (!config.async) {
// subs aren't sorted in scheduler if not running async
// we need to sort them now to make sure they fire in correct
// order
// [ts] subs.sort((a, b) => { return a.id - b.id; });
// [ts] }
// [ts] let i = 0, l = subs.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] subs[i].update();
// [ts] }
// [ts] };
// The current target watcher being evaluated.
// This is globally unique because only one watcher
// can be evaluated at a time.
// [ts] Dep.target = null;
// [ts] let targetStack = [];
// [ts] fn pushTarget(target: auto) {
// [ts] targetStack.push(target);
// [ts] Dep.target = target;
// [ts] }
// [ts] fn popTarget() {
// [ts] targetStack.pop();
// [ts] Dep.target = targetStack[targetStack.length - 1];
// [ts] }
// [ts] /*  */
// [ts] let VNode = function VNode (
// [ts] tag,
// [ts] data,
// [ts] children,
// [ts] text,
// [ts] elm,
// [ts] context,
// [ts] componentOptions,
// [ts] asyncFactory
// [ts] ) {
// [ts] _.tag = tag;
// [ts] _.data = data;
// [ts] _.children = children;
// [ts] _.text = text;
// [ts] _.elm = elm;
// [ts] _.ns = null;
// [ts] _.context = context;
// [ts] _.fnContext = null;
// [ts] _.fnOptions = null;
// [ts] _.fnScopeId = null;
// [ts] _.key = data && data.key;
// [ts] _.componentOptions = componentOptions;
// [ts] _.componentInstance = null;
// [ts] _.parent = null;
// [ts] _.raw = false;
// [ts] _.isStatic = false;
// [ts] _.isRootInsert = true;
// [ts] _.isComment = false;
// [ts] _.isCloned = false;
// [ts] _.isOnce = false;
// [ts] _.asyncFactory = asyncFactory;
// [ts] _.asyncMeta = null;
// [ts] _.isAsyncPlaceholder = false;
// [ts] };
// [ts] let prototypeAccessors = { child: { configurable: true } };
// DEPRECATED: alias for componentInstance for backwards compat.
// [ts] /* istanbul ignore next */
// [ts] prototypeAccessors.child.get = () => {
// [ts] return _.componentInstance
// [ts] };
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperties( VNode.prototype, prototypeAccessors );
// [ts] let createEmptyVNode = (text) => {
// [ts] if ( text == void 0 ) { text = ""; }
// [ts] let node = VNode.init();
// [ts] node.text = text;
// [ts] node.isComment = true;
// [ts] return node
// [ts] };
// [ts] fn createTextVNode(val: auto) {
// [ts] return VNode.init(null, null, null, String(val))
// [ts] }
// optimized shallow clone
// used for static nodes and slot nodes because they may be reused across
// multiple renders, cloning them avoids errors when DOM manipulations rely
// on their elm reference.
// [ts] fn cloneVNode(vnode: auto) {
// [ts] let cloned = VNode.init(
// [ts] vnode.tag,
// [ts] vnode.data,
// #7975
// clone children array to avoid mutating original in case of cloning
// a child.
// [ts] vnode.children && vnode.children.slice(),
// [ts] vnode.text,
// [ts] vnode.elm,
// [ts] vnode.context,
// [ts] vnode.componentOptions,
// [ts] vnode.asyncFactory
// [ts] );
// [ts] cloned.ns = vnode.ns;
// [ts] cloned.isStatic = vnode.isStatic;
// [ts] cloned.key = vnode.key;
// [ts] cloned.isComment = vnode.isComment;
// [ts] cloned.fnContext = vnode.fnContext;
// [ts] cloned.fnOptions = vnode.fnOptions;
// [ts] cloned.fnScopeId = vnode.fnScopeId;
// [ts] cloned.asyncMeta = vnode.asyncMeta;
// [ts] cloned.isCloned = true;
// [ts] return cloned
// [ts] }
// [ts] /*
// [ts] * not type checking this file because flow doesn't play well with
// [ts] * dynamically accessing methods on Array prototype
// [ts] */
// [ts] let arrayProto = Array.prototype;
// [ts] let arrayMethods = Object.create(arrayProto);
// [ts] let methodsToPatch = [
// [ts] "push",
// [ts] "pop",
// [ts] "shift",
// [ts] "unshift",
// [ts] "splice",
// [ts] "sort",
// [ts] "reverse"
// [ts] ];
// [ts] /**
// [ts] * Intercept mutating methods and emit events
// [ts] */
// [ts] methodsToPatch.forEach((method) => {
// cache original method
// [ts] let original = arrayProto[method];
// [ts] def(arrayMethods, method, () => {
// [ts] let args = [], len = arguments.length;
// [ts] while ( len-- ) args[ len ] = arguments[ len ];
// [ts] let result = original.apply(this, args);
// [ts] let ob = _.__ob__;
// [ts] let inserted;
// [ts] switch (method) {
// [ts] "push", "unshift" => {
// [ts] inserted = args;
// break;
// [ts] },
// [ts] "splice" => {
// [ts] inserted = args.slice(2);
// break;
// [ts] }
// [ts] }
// [ts] if (inserted) { ob.observeArray(inserted); }
// notify change
// [ts] ob.dep.notify();
// [ts] return result
// [ts] });
// [ts] });
// [ts] /*  */
// [ts] let arrayKeys = Object.getOwnPropertyNames(arrayMethods);
// [ts] /**
// [ts] * In some cases we may want to disable observation inside a component's
// [ts] * update computation.
// [ts] */
// [ts] let shouldObserve = true;
// [ts] fn toggleObserving(value: auto) {
// [ts] shouldObserve = value;
// [ts] }
// [ts] /**
// [ts] * Observer class that is attached to each observed
// [ts] * object. Once attached, the observer converts the target
// [ts] * object's property keys into getter/setters that
// [ts] * collect dependencies and dispatch updates.
// [ts] */
// [ts] let Observer = (value) => {
// [ts] _.value = value;
// [ts] _.dep = Dep.init();
// [ts] _.vmCount = 0;
// [ts] def(value, "__ob__", this);
// [ts] if (Array.isArray(value)) {
// [ts] if (hasProto) {
// [ts] protoAugment(value, arrayMethods);
// [ts] } else {
// [ts] copyAugment(value, arrayMethods, arrayKeys);
// [ts] }
// [ts] _.observeArray(value);
// [ts] } else {
// [ts] _.walk(value);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Walk through all properties and convert them into
// [ts] * getter/setters. This method should only be called when
// [ts] * value type is Object.
// [ts] */
// [ts] Observer.prototype.walk = (obj) => {
// [ts] let keys = Object.keys(obj);
// [ts] let i = 0;
// [ts] while (i < keys.length) { // [cap-migrate] for-step: i++
// [ts] defineReactive$$1(obj, keys[i]);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Observe a list of Array items.
// [ts] */
// [ts] Observer.prototype.observeArray = (items) => {
// [ts] let i = 0, l = items.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] observe(items[i]);
// [ts] }
// [ts] };
// helpers
// [ts] /**
// [ts] * Augment a target Object or Array by intercepting
// [ts] * the prototype chain using __proto__
// [ts] */
// [ts] fn protoAugment(target: auto, src: auto) {
// [ts] /* eslint-disable no-proto */
// [ts] target.__proto__ = src;
// [ts] /* eslint-enable no-proto */
// [ts] }
// [ts] /**
// [ts] * Augment a target Object or Array by defining
// [ts] * hidden properties.
// [ts] */
// [ts] /* istanbul ignore next */
// [ts] fn copyAugment(target: auto, src: auto, keys: auto) {
// [ts] let i = 0, l = keys.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] let key = keys[i];
// [ts] def(target, key, src[key]);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Attempt to create an observer instance for a value,
// [ts] * returns the new observer if successfully observed,
// [ts] * or the existing observer if the value already has one.
// [ts] */
// [ts] fn observe(value: auto, asRootData: auto) {
// [ts] if (!isObject(value) || value == VNode) {
// [ts] return
// [ts] }
// [ts] let ob;
// [ts] if (hasOwn(value, "__ob__") && value.__ob__ == Observer) {
// [ts] ob = value.__ob__;
// [ts] } else if (
// [ts] shouldObserve &&
// [ts] !isServerRendering() &&
// [ts] (Array.isArray(value) || isPlainObject(value)) &&
// [ts] Object.isExtensible(value) &&
// [ts] !value._isVue
// [ts] ) {
// [ts] ob = Observer.init(value);
// [ts] }
// [ts] if (asRootData && ob) {
// [ts] ob.vmCount++;
// [ts] }
// [ts] return ob
// [ts] }
// [ts] /**
// [ts] * Define a reactive property on an Object.
// [ts] */
// [ts] function defineReactive$$1 (
// [ts] obj,
// [ts] key,
// [ts] val,
// [ts] customSetter,
// [ts] shallow
// [ts] ) {
// [ts] let dep = Dep.init();
// [ts] let property = Object.getOwnPropertyDescriptor(obj, key);
// [ts] if (property && property.configurable == false) {
// [ts] return
// [ts] }
// cater for pre-defined getter/setters
// [ts] let getter = property && property.get;
// [ts] let setter = property && property.set;
// [ts] if ((!getter || setter) && arguments.length == 2) {
// [ts] val = obj[key];
// [ts] }
// [ts] let childOb = !shallow && observe(val);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(obj, key, {
// [ts-skip] enumerable: true,
// [ts-skip] configurable: true,
// [ts-skip] get: function reactiveGetter () {
// [ts-skip] var value = getter ? getter.call(obj) : val;
// [ts-skip] if (Dep.target) {
// [ts-skip] dep.depend();
// [ts-skip] if (childOb) {
// [ts-skip] childOb.dep.depend();
// [ts-skip] if (Array.isArray(value)) {
// [ts-skip] dependArray(value);
// [ts-skip] }
// [ts-skip] }
// [ts-skip] }
// [ts-skip] return value
// [ts-skip] },
// [ts-skip] set: function reactiveSetter (newVal) {
// [ts-skip] var value = getter ? getter.call(obj) : val;
// [ts] /* eslint-disable no-self-compare */
// [ts-skip] if (newVal === value || (newVal !== newVal && value !== value)) {
// [ts-skip] return
// [ts-skip] }
// [ts] /* eslint-enable no-self-compare */
// [ts-skip] if (customSetter) {
// [ts-skip] customSetter();
// [ts-skip] }
// [ts-skip] // #7981: for accessor properties without setter
// [ts-skip] if (getter && !setter) { return }
// [ts-skip] if (setter) {
// [ts-skip] setter.call(obj, newVal);
// [ts-skip] } else {
// [ts-skip] val = newVal;
// [ts-skip] }
// [ts-skip] childOb = !shallow && observe(newVal);
// [ts-skip] dep.notify();
// [ts-skip] }
// [ts-skip] });
// [ts] }
// [ts] /**
// [ts] * Set a property on an object. Adds the new property and
// [ts] * triggers change notification if the property doesn't
// [ts] * already exist.
// [ts] */
// [ts] fn set(target: auto, key: auto, val: auto) {
// [ts] if (isUndef(target) || isPrimitive(target)
// [ts] ) {
// [ts] warn(("Cannot set reactive property on undefined, null, or primitive value: " + ((target))));
// [ts] }
// [ts] if (Array.isArray(target) && isValidArrayIndex(key)) {
// [ts] target.length = Math.max(target.length, key);
// [ts] target.splice(key, 1, val);
// [ts] return val
// [ts] }
// [ts] if (key == target && !(key == Object.prototype)) {
// [ts] target[key] = val;
// [ts] return val
// [ts] }
// [ts] let ob = (target).__ob__;
// [ts] if (target._isVue || (ob && ob.vmCount)) {
// [ts] warn(
// [ts] "Avoid adding reactive properties to a Vue instance or its root $data " +
// [ts] "at runtime - declare it upfront in the data option."
// [ts] );
// [ts] return val
// [ts] }
// [ts] if (!ob) {
// [ts] target[key] = val;
// [ts] return val
// [ts] }
// [ts] defineReactive$$1(ob.value, key, val);
// [ts] ob.dep.notify();
// [ts] return val
// [ts] }
// [ts] /**
// [ts] * Delete a property and trigger change if necessary.
// [ts] */
// [ts] fn del(target: auto, key: auto) {
// [ts] if (isUndef(target) || isPrimitive(target)
// [ts] ) {
// [ts] warn(("Cannot delete reactive property on undefined, null, or primitive value: " + ((target))));
// [ts] }
// [ts] if (Array.isArray(target) && isValidArrayIndex(key)) {
// [ts] target.splice(key, 1);
// [ts] return
// [ts] }
// [ts] let ob = (target).__ob__;
// [ts] if (target._isVue || (ob && ob.vmCount)) {
// [ts] warn(
// [ts] "Avoid deleting properties on a Vue instance or its root $data " +
// [ts] "- just set it to null."
// [ts] );
// [ts] return
// [ts] }
// [ts] if (!hasOwn(target, key)) {
// [ts] return
// [ts] }
// [ts] delete target[key];
// [ts] if (!ob) {
// [ts] return
// [ts] }
// [ts] ob.dep.notify();
// [ts] }
// [ts] /**
// [ts] * Collect dependencies on array elements when the array is touched, since
// [ts] * we cannot intercept array element access like property getters.
// [ts] */
// [ts] fn dependArray(value: auto) {
// [ts] let e = (void 0), i = 0, l = value.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] e = value[i];
// [ts] e && e.__ob__ && e.__ob__.dep.depend();
// [ts] if (Array.isArray(e)) {
// [ts] dependArray(e);
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Option overwriting strategies are functions that handle
// [ts] * how to merge a parent option value and a child option
// [ts] * value into the final value.
// [ts] */
// [ts] let strats = config.optionMergeStrategies;
// [ts] /**
// [ts] * Options with restrictions
// [ts] */
// [ts] {
// [ts] strats.el = strats.propsData = (parent, child, vm, key) => {
// [ts] if (!vm) {
// [ts] warn(
// [ts] "option \"" + key + "\" can only be used during instance " +
// [ts] "creation with the `new` keyword."
// [ts] );
// [ts] }
// [ts] return defaultStrat(parent, child)
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Helper that recursively merges two data objects together.
// [ts] */
// [ts] fn mergeData(to: auto, from: auto) {
// [ts] if (!from) { return to }
// [ts] let key, toVal, fromVal;
// [ts] let keys = if (hasSymbol) Reflect.ownKeys(from) else Object.keys(from);
// [ts] let i = 0;
// [ts] while (i < keys.length) { // [cap-migrate] for-step: i++
// [ts] key = keys[i];
// in case the object is already observed...
// [ts] if (key == "__ob__") { continue }
// [ts] toVal = to[key];
// [ts] fromVal = from[key];
// [ts] if (!hasOwn(to, key)) {
// [ts] set(to, key, fromVal);
// [ts] } else if (
// [ts] toVal != fromVal &&
// [ts] isPlainObject(toVal) &&
// [ts] isPlainObject(fromVal)
// [ts] ) {
// [ts] mergeData(toVal, fromVal);
// [ts] }
// [ts] }
// [ts] return to
// [ts] }
// [ts] /**
// [ts] * Data
// [ts] */
// [ts] fn mergeDataOrFn(parentVal: auto, childVal: auto, vm: auto) {
// [ts] if (!vm) {
// in a Vue.extend merge, both should be functions
// [ts] if (!childVal) {
// [ts] return parentVal
// [ts] }
// [ts] if (!parentVal) {
// [ts] return childVal
// [ts] }
// when parentVal & childVal are both present,
// we need to return a function that returns the
// merged result of both functions... no need to
// check if parentVal is a function here because
// it has to be a function to pass previous merges.
// [ts] return () => {
// [ts] return mergeData(
// [ts] if (@typeName(childVal) == "function") childVal.call(this, this) else childVal,
// [ts] if (@typeName(parentVal) == "function") parentVal.call(this, this) else parentVal
// [ts] )
// [ts] }
// [ts] } else {
// [ts] return () => {
// instance merge
// [ts] let instanceData = if (@typeName(childVal) == "function") childVal.call(vm, vm) else childVal;
// [ts] let defaultData = if (@typeName(parentVal) == "function") parentVal.call(vm, vm) else parentVal;
// [ts] if (instanceData) {
// [ts] return mergeData(instanceData, defaultData)
// [ts] } else {
// [ts] return defaultData
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] strats.data = function (
// [ts] parentVal,
// [ts] childVal,
// [ts] vm
// [ts] ) {
// [ts] if (!vm) {
// [ts] if (childVal && @typeName(childVal) != "function") {
// [ts] warn(
// [ts] "The \"data\" option should be a function " +
// [ts] "that returns a per-instance value in component " +
// [ts] "definitions.",
// [ts] vm
// [ts] );
// [ts] return parentVal
// [ts] }
// [ts] return mergeDataOrFn(parentVal, childVal)
// [ts] }
// [ts] return mergeDataOrFn(parentVal, childVal, vm)
// [ts] };
// [ts] /**
// [ts] * Hooks and props are merged as arrays.
// [ts] */
// [ts] fn mergeHook(parentVal: auto, childVal: auto) {
// [ts] let res = if (childVal) if (parentVal) parentVal.concat(childVal) else if (Array.isArray(childVal)) childVal else [childVal] else parentVal;
// [ts] return if (res) dedupeHooks(res) else res
// [ts] }
// [ts] fn dedupeHooks(hooks: auto) {
// [ts] let res = [];
// [ts] let i = 0;
// [ts] while (i < hooks.length) { // [cap-migrate] for-step: i++
// [ts] if (res.indexOf(hooks[i]) == -1) {
// [ts] res.push(hooks[i]);
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] LIFECYCLE_HOOKS.forEach((hook) => {
// [ts] strats[hook] = mergeHook;
// [ts] });
// [ts] /**
// [ts] * Assets
// [ts] *
// [ts] * When a vm is present (instance creation), we need to do
// [ts] * a three-way merge between constructor options, instance
// [ts] * options and parent options.
// [ts] */
// [ts] fn mergeAssets(parentVal: auto, childVal: auto, vm: auto, key: auto) {
// [ts] let res = Object.create(parentVal || null);
// [ts] if (childVal) {
// [ts] assertObjectType(key, childVal, vm);
// [ts] return extend(res, childVal)
// [ts] } else {
// [ts] return res
// [ts] }
// [ts] }
// [ts] ASSET_TYPES.forEach((type) => {
// [ts] strats[type + "s"] = mergeAssets;
// [ts] });
// [ts] /**
// [ts] * Watchers.
// [ts] *
// [ts] * Watchers hashes should not overwrite one
// [ts] * another, so we merge them as arrays.
// [ts] */
// [ts] strats.watch = function (
// [ts] parentVal,
// [ts] childVal,
// [ts] vm,
// [ts] key
// [ts] ) {
// work around Firefox's Object.prototype.watch...
// [ts] if (parentVal == nativeWatch) { parentVal = null; }
// [ts] if (childVal == nativeWatch) { childVal = null; }
// [ts] /* istanbul ignore if */
// [ts] if (!childVal) { return Object.create(parentVal || null) }
// [ts] {
// [ts] assertObjectType(key, childVal, vm);
// [ts] }
// [ts] if (!parentVal) { return childVal }
// [ts] let ret = {};
// [ts] extend(ret, parentVal);
// [ts] for (let key$1 in childVal) {
// [ts] let parent = ret[key$1];
// [ts] let child = childVal[key$1];
// [ts] if (parent && !Array.isArray(parent)) {
// [ts] parent = [parent];
// [ts] }
// [ts] ret[key$1] = if (parent) parent.concat(child) else if (Array.isArray(child)) child else [child];
// [ts] }
// [ts] return ret
// [ts] };
// [ts] /**
// [ts] * Other object hashes.
// [ts] */
// [ts] strats.props =
// [ts] strats.methods =
// [ts] strats.inject =
// [ts] strats.computed = function (
// [ts] parentVal,
// [ts] childVal,
// [ts] vm,
// [ts] key
// [ts] ) {
// [ts] if (childVal && "development" != "production") {
// [ts] assertObjectType(key, childVal, vm);
// [ts] }
// [ts] if (!parentVal) { return childVal }
// [ts] let ret = Object.create(null);
// [ts] extend(ret, parentVal);
// [ts] if (childVal) { extend(ret, childVal); }
// [ts] return ret
// [ts] };
// [ts] strats.provide = mergeDataOrFn;
// [ts] /**
// [ts] * Default strategy.
// [ts] */
// [ts] let defaultStrat = (parentVal, childVal) => {
// [ts] return if (childVal == null) parentVal else childVal
// [ts] };
// [ts] /**
// [ts] * Validate component names
// [ts] */
// [ts] fn checkComponents(options: auto) {
// [ts] for (let key in options.components) {
// [ts] validateComponentName(key);
// [ts] }
// [ts] }
// [ts] fn validateComponentName(name: auto) {
// [ts] if (!RegExp.init(("^[a-zA-Z][\\-\\.0-9_" + (unicodeRegExp.source) + "]*$")).test(name)) {
// [ts] warn(
// [ts] "Invalid component name: \"" + name + "\". Component names " +
// [ts] "should conform to valid custom element name in html5 specification."
// [ts] );
// [ts] }
// [ts] if (isBuiltInTag(name) || config.isReservedTag(name)) {
// [ts] warn(
// [ts] "Do not use built-in or reserved HTML elements as component " +
// [ts] "id: " + name
// [ts] );
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Ensure all props option syntax are normalized into the
// [ts] * Object-based format.
// [ts] */
// [ts] fn normalizeProps(options: auto, vm: auto) {
// [ts] let props = options.props;
// [ts] if (!props) { return }
// [ts] let res = {};
// [ts] let i, val, name;
// [ts] if (Array.isArray(props)) {
// [ts] i = props.length;
// [ts] while (i--) {
// [ts] val = props[i];
// [ts] if (@typeName(val) == "string") {
// [ts] name = camelize(val);
// [ts] res[name] = { type: null };
// [ts] } else {
// [ts] warn("props must be strings when using array syntax.");
// [ts] }
// [ts] }
// [ts] } else if (isPlainObject(props)) {
// [ts] for (let key in props) {
// [ts] val = props[key];
// [ts] name = camelize(key);
// [ts] res[name] = if (isPlainObject(val)) val else { type: val };
// [ts] }
// [ts] } else {
// [ts] warn(
// [ts] "Invalid value for option \"props\": expected an Array or an Object, " +
// [ts] "but got " + (toRawType(props)) + ".",
// [ts] vm
// [ts] );
// [ts] }
// [ts] options.props = res;
// [ts] }
// [ts] /**
// [ts] * Normalize all injections into Object-based format
// [ts] */
// [ts] fn normalizeInject(options: auto, vm: auto) {
// [ts] let inject = options.inject;
// [ts] if (!inject) { return }
// [ts] let normalized = options.inject = {};
// [ts] if (Array.isArray(inject)) {
// [ts] let i = 0;
// [ts] while (i < inject.length) { // [cap-migrate] for-step: i++
// [ts] normalized[inject[i]] = { from: inject[i] };
// [ts] }
// [ts] } else if (isPlainObject(inject)) {
// [ts] for (let key in inject) {
// [ts] let val = inject[key];
// [ts] normalized[key] = if (isPlainObject(val)) extend({ from: key }, val) else { from: val };
// [ts] }
// [ts] } else {
// [ts] warn(
// [ts] "Invalid value for option \"inject\": expected an Array or an Object, " +
// [ts] "but got " + (toRawType(inject)) + ".",
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Normalize raw function directives into object format.
// [ts] */
// [ts] fn normalizeDirectives(options: auto) {
// [ts] let dirs = options.directives;
// [ts] if (dirs) {
// [ts] for (let key in dirs) {
// [ts] let def$$1 = dirs[key];
// [ts] if (@typeName(def$$1) == "function") {
// [ts] dirs[key] = { bind: def$$1, update: def$$1 };
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn assertObjectType(name: auto, value: auto, vm: auto) {
// [ts] if (!isPlainObject(value)) {
// [ts] warn(
// [ts] "Invalid value for option \"" + name + "\": expected an Object, " +
// [ts] "but got " + (toRawType(value)) + ".",
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Merge two option objects into a new one.
// [ts] * Core utility used in both instantiation and inheritance.
// [ts] */
// [ts] fn mergeOptions(parent: auto, child: auto, vm: auto) {
// [ts] {
// [ts] checkComponents(child);
// [ts] }
// [ts] if (@typeName(child) == "function") {
// [ts] child = child.options;
// [ts] }
// [ts] normalizeProps(child, vm);
// [ts] normalizeInject(child, vm);
// [ts] normalizeDirectives(child);
// Apply extends and mixins on the child options,
// but only if it is a raw options object that isn't
// the result of another mergeOptions call.
// Only merged options has the _base property.
// [ts] if (!child._base) {
// [ts] if (child.extends) {
// [ts] parent = mergeOptions(parent, child.extends, vm);
// [ts] }
// [ts] if (child.mixins) {
// [ts] let i = 0, l = child.mixins.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] parent = mergeOptions(parent, child.mixins[i], vm);
// [ts] }
// [ts] }
// [ts] }
// [ts] let options = {};
// [ts] let key;
// [ts] for (key in parent) {
// [ts] mergeField(key);
// [ts] }
// [ts] for (key in child) {
// [ts] if (!hasOwn(parent, key)) {
// [ts] mergeField(key);
// [ts] }
// [ts] }
// [ts] fn mergeField(key: auto) {
// [ts] let strat = strats[key] || defaultStrat;
// [ts] options[key] = strat(parent[key], child[key], vm, key);
// [ts] }
// [ts] return options
// [ts] }
// [ts] /**
// [ts] * Resolve an asset.
// [ts] * This function is used because child instances need access
// [ts] * to assets defined in its ancestor chain.
// [ts] */
// [ts] fn resolveAsset(options: auto, type: auto, id: auto, warnMissing: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (@typeName(id) != "string") {
// [ts] return
// [ts] }
// [ts] let assets = options[type];
// check local registration variations first
// [ts] if (hasOwn(assets, id)) { return assets[id] }
// [ts] let camelizedId = camelize(id);
// [ts] if (hasOwn(assets, camelizedId)) { return assets[camelizedId] }
// [ts] let PascalCaseId = capitalize(camelizedId);
// [ts] if (hasOwn(assets, PascalCaseId)) { return assets[PascalCaseId] }
// fallback to prototype chain
// [ts] let res = assets[id] || assets[camelizedId] || assets[PascalCaseId];
// [ts] if (warnMissing && !res) {
// [ts] warn(
// [ts] "Failed to resolve " + type.slice(0, -1) + ": " + id,
// [ts] options
// [ts] );
// [ts] }
// [ts] return res
// [ts] }
// [ts] /*  */
// [ts] fn validateProp(key: auto, propOptions: auto, propsData: auto, vm: auto) {
// [ts] let prop = propOptions[key];
// [ts] let absent = !hasOwn(propsData, key);
// [ts] let value = propsData[key];
// boolean casting
// [ts] let booleanIndex = getTypeIndex(Boolean, prop.type);
// [ts] if (booleanIndex > -1) {
// [ts] if (absent && !hasOwn(prop, "default")) {
// [ts] value = false;
// [ts] } else if (value == "" || value == hyphenate(key)) {
// only cast empty string / same name to boolean if
// boolean has higher priority
// [ts] let stringIndex = getTypeIndex(String, prop.type);
// [ts] if (stringIndex < 0 || booleanIndex < stringIndex) {
// [ts] value = true;
// [ts] }
// [ts] }
// [ts] }
// check default value
// [ts] if (value == null) {
// [ts] value = getPropDefaultValue(vm, prop, key);
// since the default value is a fresh copy,
// make sure to observe it.
// [ts] let prevShouldObserve = shouldObserve;
// [ts] toggleObserving(true);
// [ts] observe(value);
// [ts] toggleObserving(prevShouldObserve);
// [ts] }
// [ts] {
// [ts] assertProp(prop, key, value, vm, absent);
// [ts] }
// [ts] return value
// [ts] }
// [ts] /**
// [ts] * Get the default value of a prop.
// [ts] */
// [ts] fn getPropDefaultValue(vm: auto, prop: auto, key: auto) {
// no default, return undefined
// [ts] if (!hasOwn(prop, "default")) {
// [ts] return null
// [ts] }
// [ts] let def = prop.default;
// warn against non-factory defaults for Object & Array
// [ts] if (isObject(def)) {
// [ts] warn(
// [ts] "Invalid default value for prop \"" + key + "\": " +
// [ts] "Props with type Object/Array must use a factory function " +
// [ts] "to return the default value.",
// [ts] vm
// [ts] );
// [ts] }
// the raw prop value was also undefined from previous render,
// return previous default value to avoid unnecessary watcher trigger
// [ts] if (vm && vm.$options.propsData &&
// [ts] vm.$options.propsData[key] == null &&
// [ts] vm._props[key] != null
// [ts] ) {
// [ts] return vm._props[key]
// [ts] }
// call factory function for non-Function types
// a value is Function if its prototype is function even across different execution context
// [ts] return if (@typeName(def) == "function" && getType(prop.type) != "Function") def.call(vm) else def
// [ts] }
// [ts] /**
// [ts] * Assert whether a prop is valid.
// [ts] */
// [ts] fn assertProp(prop: auto, name: auto, value: auto, vm: auto, absent: auto) {
// [ts] if (prop.required && absent) {
// [ts] warn(
// [ts] "Missing required prop: \"" + name + "\"",
// [ts] vm
// [ts] );
// [ts] return
// [ts] }
// [ts] if (value == null && !prop.required) {
// [ts] return
// [ts] }
// [ts] let type = prop.type;
// [ts] let valid = !type || type == true;
// [ts] let expectedTypes = [];
// [ts] if (type) {
// [ts] if (!Array.isArray(type)) {
// [ts] type = [type];
// [ts] }
// [ts] let i = 0;
// [ts] while (i < type.length && !valid) { // [cap-migrate] for-step: i++
// [ts] let assertedType = assertType(value, type[i], vm);
// [ts] expectedTypes.push(assertedType.expectedType || "");
// [ts] valid = assertedType.valid;
// [ts] }
// [ts] }
// [ts] let haveExpectedTypes = expectedTypes.some((t) => { return t; });
// [ts] if (!valid && haveExpectedTypes) {
// [ts] warn(
// [ts] getInvalidTypeMessage(name, value, expectedTypes),
// [ts] vm
// [ts] );
// [ts] return
// [ts] }
// [ts] let validator = prop.validator;
// [ts] if (validator) {
// [ts] if (!validator(value)) {
// [ts] warn(
// [ts] "Invalid prop: custom validator check failed for prop \"" + name + "\".",
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] let simpleCheckRE = "^(String|Number|Boolean|Function|Symbol|BigInt)$";
// [ts] fn assertType(value: auto, type: auto, vm: auto) {
// [ts] let valid;
// [ts] let expectedType = getType(type);
// [ts] if (simpleCheckRE.test(expectedType)) {
// [ts] let t = @typeName(value);
// [ts] valid = t == expectedType.toLowerCase();
// for primitive wrapper objects
// [ts] if (!valid && t == "object") {
// [ts] valid = value == type;
// [ts] }
// [ts] } else if (expectedType == "Object") {
// [ts] valid = isPlainObject(value);
// [ts] } else if (expectedType == "Array") {
// [ts] valid = Array.isArray(value);
// [ts] } else {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] valid = value == type;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] warn("Invalid prop type: \"" + String(type) + "\" is not a constructor", vm);
// [ts] valid = false;
// [ts] }
// [ts] }
// [ts] return {
// [ts] valid: valid,
// [ts] expectedType: expectedType
// [ts] }
// [ts] }
// [ts] let functionTypeCheckRE = "^\\s*function (\\w+)";
// [ts] /**
// [ts] * Use function string name to check built-in types,
// [ts] * because a simple equality check will fail when running
// [ts] * across different vms / iframes.
// [ts] */
// [ts] fn getType(fn: auto) {
// [ts] let match_val = fn && fn.toString().match(functionTypeCheckRE);
// [ts] return if (match_val) match_val[1] else ""
// [ts] }
// [ts] fn isSameType(a: auto, b: auto) {
// [ts] return getType(a) == getType(b)
// [ts] }
// [ts] fn getTypeIndex(type: auto, expectedTypes: auto) {
// [ts] if (!Array.isArray(expectedTypes)) {
// [ts] return if (isSameType(expectedTypes, type)) 0 else -1
// [ts] }
// [ts] let i = 0, len = expectedTypes.length;
// [ts] while (i < len) { // [cap-migrate] for-step: i++
// [ts] if (isSameType(expectedTypes[i], type)) {
// [ts] return i
// [ts] }
// [ts] }
// [ts] return -1
// [ts] }
// [ts] fn getInvalidTypeMessage(name: auto, value: auto, expectedTypes: auto) {
// [ts] let message = "Invalid prop: type check failed for prop \"" + name + "\"." +
// [ts] " Expected " + (expectedTypes.map(capitalize).join(", "));
// [ts] let expectedType = expectedTypes[0];
// [ts] let receivedType = toRawType(value);
// check if we need to specify expected value
// [ts] if (
// [ts] expectedTypes.length == 1 &&
// [ts] isExplicable(expectedType) &&
// [ts] isExplicable(@typeName(value)) &&
// [ts] !isBoolean(expectedType, receivedType)
// [ts] ) {
// [ts] message += " with value " + (styleValue(value, expectedType));
// [ts] }
// [ts] message += ", got " + receivedType + " ";
// check if we need to specify received value
// [ts] if (isExplicable(receivedType)) {
// [ts] message += "with value " + (styleValue(value, receivedType)) + ".";
// [ts] }
// [ts] return message
// [ts] }
// [ts] fn styleValue(value: auto, type: auto) {
// [ts] if (type == "String") {
// [ts] return ("\"" + value + "\"")
// [ts] } else if (type == "Number") {
// [ts] return ("" + (Number(value)))
// [ts] } else {
// [ts] return ("" + value)
// [ts] }
// [ts] }
// [ts] let EXPLICABLE_TYPES = ["string", "number", "boolean"];
// [ts] fn isExplicable(value: auto) {
// [ts] return EXPLICABLE_TYPES.some((elem) => { return value.toLowerCase() == elem; })
// [ts] }
// [ts] fn isBoolean() {
// [ts] let args = [], len = arguments.length;
// [ts] while ( len-- ) args[ len ] = arguments[ len ];
// [ts] return args.some((elem) => { return elem.toLowerCase() == "boolean"; })
// [ts] }
// [ts] /*  */
// [ts] fn handleError(err: auto, vm: auto, info: auto) {
// Deactivate deps tracking while processing error handler to avoid possible infinite rendering.
// See: https://github.com/vuejs/vuex/issues/1505
// [ts] pushTarget();
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (vm) {
// [ts] let cur = vm;
// [ts] while ((cur = cur.$parent)) {
// [ts] let hooks = cur.$options.errorCaptured;
// [ts] if (hooks) {
// [ts] let i = 0;
// [ts] while (i < hooks.length) { // [cap-migrate] for-step: i++
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let capture = hooks[i].call(cur, err, vm, info) == false;
// [ts] if (capture) { return }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] globalHandleError(e, cur, "errorCaptured hook");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] globalHandleError(err, vm, info);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] popTarget();
// [ts] }
// [ts] }
// [ts] fn invokeWithErrorHandling(handler: auto, context: auto, args: auto, vm: auto, info: auto) {
// [ts] let res;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] res = if (args) handler.apply(context, args) else handler.call(context);
// [ts] if (res && !res._isVue && isPromise(res) && !res._handled) {
// [ts] res.catch((e) => { return handleError(e, vm, info + " (Promise/async)"); });
// issue #9511
// avoid catch triggering multiple times when nested calls
// [ts] res._handled = true;
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] handleError(e, vm, info);
// [ts] }
// [ts] return res
// [ts] }
// [ts] fn globalHandleError(err: auto, vm: auto, info: auto) {
// [ts] if (config.errorHandler) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return config.errorHandler.call(null, err, vm, info)
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// if the user intentionally throws the original error in the handler,
// do not log it twice
// [ts] if (e != err) {
// [ts] logError(e, null, "config.errorHandler");
// [ts] }
// [ts] }
// [ts] }
// [ts] logError(err, vm, info);
// [ts] }
// [ts] fn logError(err: auto, vm: auto, info: auto) {
// [ts] {
// [ts] warn(("Error in " + info + ": \"" + (err.toString()) + "\""), vm);
// [ts] }
// [ts] /* istanbul ignore else */
// [ts] if ((inBrowser || inWeex) && @typeName(console) != "undefined") {
// [ts] @print(err);
// [ts] } else {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw err)
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let isUsingMicroTask = false;
// [ts] let callbacks = [];
// [ts] let pending = false;
// [ts] fn flushCallbacks() {
// [ts] pending = false;
// [ts] let copies = callbacks.slice(0);
// [ts] callbacks.length = 0;
// [ts] let i = 0;
// [ts] while (i < copies.length) { // [cap-migrate] for-step: i++
// [ts] copies[i]();
// [ts] }
// [ts] }
// Here we have async deferring wrappers using microtasks.
// In 2.5 we used (macro) tasks (in combination with microtasks).
// However, it has subtle problems when state is changed right before repaint
// (e.g. #6813, out-in transitions).
// Also, using (macro) tasks in event handler would cause some weird behaviors
// that cannot be circumvented (e.g. #7109, #7153, #7546, #7834, #8109).
// So we now use microtasks everywhere, again.
// A major drawback of this tradeoff is that there are some scenarios
// where microtasks have too high a priority and fire in between supposedly
// sequential events (e.g. #4521, #6690, which have workarounds)
// or even between bubbling of the same event (#6566).
// [ts] let timerFunc;
// The nextTick behavior leverages the microtask queue, which can be accessed
// via either native Promise.then or MutationObserver.
// MutationObserver has wider support, however it is seriously bugged in
// UIWebView in iOS >= 9.3.3 when triggered in touch event handlers. It
// completely stops working after triggering a few times... so, if native
// Promise is available, we will use it:
// [ts] /* istanbul ignore next, $flow-disable-line */
// [ts] if (@typeName(Promise) != "undefined" && isNative(Promise)) {
// [ts] let p = Promise.resolve();
// [ts] timerFunc = () => {
// [ts] p.then(flushCallbacks);
// In problematic UIWebViews, Promise.then doesn't completely break, but
// it can get stuck in a weird state where callbacks are pushed into the
// microtask queue but the queue isn't being flushed, until the browser
// needs to do some other work, e.g. handle a timer. Therefore we can
// "force" the microtask queue to be flushed by adding an empty timer.
// [ts] if (isIOS) { setTimeout(noop); }
// [ts] };
// [ts] isUsingMicroTask = true;
// [ts] } else if (!isIE && @typeName(MutationObserver) != "undefined" && (
// [ts] isNative(MutationObserver) ||
// PhantomJS and iOS 7.x
// [ts] MutationObserver.toString() == "[object MutationObserverConstructor]"
// [ts] )) {
// Use MutationObserver where native Promise is not available,
// e.g. PhantomJS, iOS7, Android 4.4
// (#6466 MutationObserver is unreliable in IE11)
// [ts] let counter = 1;
// [ts] let observer = MutationObserver.init(flushCallbacks);
// [ts] let textNode = document.createTextNode(String(counter));
// [ts] observer.observe(textNode, {
// [ts] characterData: true
// [ts] });
// [ts] timerFunc = () => {
// [ts] counter = (counter + 1) % 2;
// [ts] textNode.data = String(counter);
// [ts] };
// [ts] isUsingMicroTask = true;
// [ts] } else if (@typeName(setImmediate) != "undefined" && isNative(setImmediate)) {
// Fallback to setImmediate.
// Technically it leverages the (macro) task queue,
// but it is still a better choice than setTimeout.
// [ts] timerFunc = () => {
// [ts] setImmediate(flushCallbacks);
// [ts] };
// [ts] } else {
// Fallback to setTimeout.
// [ts] timerFunc = () => {
// [ts] setTimeout(flushCallbacks, 0);
// [ts] };
// [ts] }
// [ts] fn nextTick(cb: auto, ctx: auto) {
// [ts] let _resolve;
// [ts] callbacks.push(() => {
// [ts] if (cb) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] cb.call(ctx);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] handleError(e, ctx, "nextTick");
// [ts] }
// [ts] } else if (_resolve) {
// [ts] _resolve(ctx);
// [ts] }
// [ts] });
// [ts] if (!pending) {
// [ts] pending = true;
// [ts] timerFunc();
// [ts] }
// $flow-disable-line
// [ts] if (!cb && @typeName(Promise) != "undefined") {
// [ts] return Promise.init((resolve) => {
// [ts] _resolve = resolve;
// [ts] })
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let mark;
// [ts] let measure;
// [ts] {
// [ts] let perf = inBrowser && window.performance;
// [ts] /* istanbul ignore if */
// [ts] if (
// [ts] perf &&
// [ts] perf.mark &&
// [ts] perf.measure &&
// [ts] perf.clearMarks &&
// [ts] perf.clearMeasures
// [ts] ) {
// [ts] mark = (tag) => { return perf.mark(tag); };
// [ts] measure = (name, startTag, endTag) => {
// [ts] perf.measure(name, startTag, endTag);
// [ts] perf.clearMarks(startTag);
// [ts] perf.clearMarks(endTag);
// perf.clearMeasures(name)
// [ts] };
// [ts] }
// [ts] }
// [ts] /* not type checking this file because flow doesn't play well with Proxy */
// [ts] let initProxy;
// [ts] {
// [ts] let allowedGlobals = makeMap(
// [ts] "Infinity,undefined,NaN,isFinite,isNaN," +
// [ts] "parseFloat,parseInt,decodeURI,decodeURIComponent,encodeURI,encodeURIComponent," +
// [ts] "Math,Number,Date,Array,Object,Boolean,String,RegExp,Map,Set,JSON,Intl,BigInt," +
// [ts] "require" // for Webpack/Browserify
// [ts] );
// [ts] let warnNonPresent = (target, key) => {
// [ts] warn(
// [ts] "Property or method \"" + key + "\" is not defined on the instance but " +
// [ts] "referenced during render. Make sure that this property is reactive, " +
// [ts] "either in the data option, or for class-based components, by " +
// [ts] "initializing the property. " +
// [ts] "See: https://vuejs.org/v2/guide/reactivity.html#Declaring-Reactive-Properties.",
// [ts] target
// [ts] );
// [ts] };
// [ts] let warnReservedPrefix = (target, key) => {
// [ts] warn(
// [ts] "Property \"" + key + "\" must be accessed with \"$data." + key + "\" because " +
// [ts] "properties starting with \"$\" or \"_\" are not proxied in the Vue instance to " +
// [ts] "prevent conflicts with Vue internals. " +
// [ts] "See: https://vuejs.org/v2/api/#data",
// [ts] target
// [ts] );
// [ts] };
// [ts] let hasProxy =
// [ts] @typeName(Proxy) != "undefined" && isNative(Proxy);
// [ts] if (hasProxy) {
// [ts] let isBuiltInModifier = makeMap("stop,prevent,self,ctrl,shift,alt,meta,exact");
// [ts] config.keyCodes = Proxy.init(config.keyCodes, {
// [ts] set: (target, key, value) => {
// [ts] if (isBuiltInModifier(key)) {
// [ts] warn(("Avoid overwriting built-in modifier in config.keyCodes: ." + key));
// [ts] return false
// [ts] } else {
// [ts] target[key] = value;
// [ts] return true
// [ts] }
// [ts] }
// [ts] });
// [ts] }
// [ts] let hasHandler = {
// [ts] has: (target, key) => {
// [ts] let has = key == target;
// [ts] let isAllowed = allowedGlobals(key) ||
// [ts] (@typeName(key) == "string" && key.charAt(0) == "_" && !(key == target.$data));
// [ts] if (!has && !isAllowed) {
// [ts] if (key == target.$data) { warnReservedPrefix(target, key); }
// [ts] else { warnNonPresent(target, key); }
// [ts] }
// [ts] return has || !isAllowed
// [ts] }
// [ts] };
// [ts] let getHandler = {
// [ts] get: (target, key) => {
// [ts] if (@typeName(key) == "string" && !(key == target)) {
// [ts] if (key == target.$data) { warnReservedPrefix(target, key); }
// [ts] else { warnNonPresent(target, key); }
// [ts] }
// [ts] return target[key]
// [ts] }
// [ts] };
// [ts] initProxy = (vm) => {
// [ts] if (hasProxy) {
// determine which proxy handler to use
// [ts] let options = vm.$options;
// [ts] let handlers = if (options.render && options.render._withStripped) getHandler else hasHandler;
// [ts] vm._renderProxy = Proxy.init(vm, handlers);
// [ts] } else {
// [ts] vm._renderProxy = vm;
// [ts] }
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] let seenObjects = _Set.init();
// [ts] /**
// [ts] * Recursively traverse an object to evoke all converted
// [ts] * getters, so that every nested property inside the object
// [ts] * is collected as a "deep" dependency.
// [ts] */
// [ts] fn traverse(val: auto) {
// [ts] _traverse(val, seenObjects);
// [ts] seenObjects.clear();
// [ts] }
// [ts] fn _traverse(val: auto, seen: auto) {
// [ts] let i, keys;
// [ts] let isA = Array.isArray(val);
// [ts] if ((!isA && !isObject(val)) || Object.isFrozen(val) || val == VNode) {
// [ts] return
// [ts] }
// [ts] if (val.__ob__) {
// [ts] let depId = val.__ob__.dep.id;
// [ts] if (seen.has(depId)) {
// [ts] return
// [ts] }
// [ts] seen.add(depId);
// [ts] }
// [ts] if (isA) {
// [ts] i = val.length;
// [ts] while (i--) { _traverse(val[i], seen); }
// [ts] } else {
// [ts] keys = Object.keys(val);
// [ts] i = keys.length;
// [ts] while (i--) { _traverse(val[keys[i]], seen); }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let normalizeEvent = cached((name) => {
// [ts] let passive = name.charAt(0) == "&";
// [ts] name = if (passive) name.slice(1) else name;
// [ts] let once$$1 = name.charAt(0) == "~"; // Prefixed last, checked first
// [ts] name = if (once$$1) name.slice(1) else name;
// [ts] let capture = name.charAt(0) == "!";
// [ts] name = if (capture) name.slice(1) else name;
// [ts] return {
// [ts] name: name,
// [ts] once: once$$1,
// [ts] capture: capture,
// [ts] passive: passive
// [ts] }
// [ts] });
// [ts] fn createFnInvoker(fns: auto, vm: auto) {
// [ts] fn invoker() {
// [ts] let arguments$1 = arguments;
// [ts] let fns = invoker.fns;
// [ts] if (Array.isArray(fns)) {
// [ts] let cloned = fns.slice();
// [ts] let i = 0;
// [ts] while (i < cloned.length) { // [cap-migrate] for-step: i++
// [ts] invokeWithErrorHandling(cloned[i], null, arguments$1, vm, "v-on handler");
// [ts] }
// [ts] } else {
// return handler return value for single handlers
// [ts] return invokeWithErrorHandling(fns, null, arguments, vm, "v-on handler")
// [ts] }
// [ts] }
// [ts] invoker.fns = fns;
// [ts] return invoker
// [ts] }
// [ts] fn updateListeners(on: auto, oldOn: auto, add: auto, remove$$1: auto, createOnceHandler: auto, vm: auto) {
// [ts] let name, def$$1, cur, old, event;
// [ts] for (name in on) {
// [ts] def$$1 = cur = on[name];
// [ts] old = oldOn[name];
// [ts] event = normalizeEvent(name);
// [ts] if (isUndef(cur)) {
// [ts] warn(
// [ts] "Invalid handler for event \"" + (event.name) + "\": got " + String(cur),
// [ts] vm
// [ts] );
// [ts] } else if (isUndef(old)) {
// [ts] if (isUndef(cur.fns)) {
// [ts] cur = on[name] = createFnInvoker(cur, vm);
// [ts] }
// [ts] if (isTrue(event.once)) {
// [ts] cur = on[name] = createOnceHandler(event.name, cur, event.capture);
// [ts] }
// [ts] add(event.name, cur, event.capture, event.passive, event.params);
// [ts] } else if (cur != old) {
// [ts] old.fns = cur;
// [ts] on[name] = old;
// [ts] }
// [ts] }
// [ts] for (name in oldOn) {
// [ts] if (isUndef(on[name])) {
// [ts] event = normalizeEvent(name);
// [ts] remove$$1(event.name, oldOn[name], event.capture);
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn mergeVNodeHook(def: auto, hookKey: auto, hook: auto) {
// [ts] if (def == VNode) {
// [ts] def = def.data.hook || (def.data.hook = {});
// [ts] }
// [ts] let invoker;
// [ts] let oldHook = def[hookKey];
// [ts] fn wrappedHook() {
// [ts] hook.apply(this, arguments);
// important: remove merged hook to ensure it's called only once
// and prevent memory leak
// [ts] remove(invoker.fns, wrappedHook);
// [ts] }
// [ts] if (isUndef(oldHook)) {
// no existing hook
// [ts] invoker = createFnInvoker([wrappedHook]);
// [ts] } else {
// [ts] /* istanbul ignore if */
// [ts] if (isDef(oldHook.fns) && isTrue(oldHook.merged)) {
// already a merged invoker
// [ts] invoker = oldHook;
// [ts] invoker.fns.push(wrappedHook);
// [ts] } else {
// existing plain hook
// [ts] invoker = createFnInvoker([oldHook, wrappedHook]);
// [ts] }
// [ts] }
// [ts] invoker.merged = true;
// [ts] def[hookKey] = invoker;
// [ts] }
// [ts] /*  */
// [ts] fn extractPropsFromVNodeData(data: auto, Ctor: auto, tag: auto) {
// we are only extracting raw values here.
// validation and default values are handled in the child
// component itself.
// [ts] let propOptions = Ctor.options.props;
// [ts] if (isUndef(propOptions)) {
// [ts] return
// [ts] }
// [ts] let res = {};
// [ts] let attrs = data.attrs;
// [ts] let props = data.props;
// [ts] if (isDef(attrs) || isDef(props)) {
// [ts] for (let key in propOptions) {
// [ts] let altKey = hyphenate(key);
// [ts] {
// [ts] let keyInLowerCase = key.toLowerCase();
// [ts] if (
// [ts] key != keyInLowerCase &&
// [ts] attrs && hasOwn(attrs, keyInLowerCase)
// [ts] ) {
// [ts] tip(
// [ts] "Prop \"" + keyInLowerCase + "\" is passed to component " +
// [ts] (formatComponentName(tag || Ctor)) + ", but the declared prop name is" +
// [ts] " \"" + key + "\". " +
// [ts] "Note that HTML attributes are case-insensitive and camelCased " +
// [ts] "props need to use their kebab-case equivalents when using in-DOM " +
// [ts] "templates. You should probably use \"" + altKey + "\" instead of \"" + key + "\"."
// [ts] );
// [ts] }
// [ts] }
// [ts] checkProp(res, props, key, altKey, true) ||
// [ts] checkProp(res, attrs, key, altKey, false);
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] fn checkProp(res: auto, hash: auto, key: auto, altKey: auto, preserve: auto) {
// [ts] if (isDef(hash)) {
// [ts] if (hasOwn(hash, key)) {
// [ts] res[key] = hash[key];
// [ts] if (!preserve) {
// [ts] delete hash[key];
// [ts] }
// [ts] return true
// [ts] } else if (hasOwn(hash, altKey)) {
// [ts] res[key] = hash[altKey];
// [ts] if (!preserve) {
// [ts] delete hash[altKey];
// [ts] }
// [ts] return true
// [ts] }
// [ts] }
// [ts] return false
// [ts] }
// [ts] /*  */
// The template compiler attempts to minimize the need for normalization by
// statically analyzing the template at compile time.
//
// For plain HTML markup, normalization can be completely skipped because the
// generated render function is guaranteed to return Array<VNode>. There are
// two cases where extra normalization is needed:
// 1. When the children contains components - because a functional component
// may return an Array instead of a single root. In this case, just a simple
// normalization is needed - if any child is an Array, we flatten the whole
// thing with Array.prototype.concat. It is guaranteed to be only 1-level deep
// because functional components already normalize their own children.
// [ts] fn simpleNormalizeChildren(children: auto) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] if (Array.isArray(children[i])) {
// [ts] return Array.prototype.concat.apply([], children)
// [ts] }
// [ts] }
// [ts] return children
// [ts] }
// 2. When the children contains constructs that always generated nested Arrays,
// e.g. <template>, <slot>, v-for, or when the children is provided by user
// with hand-written render functions / JSX. In such cases a full normalization
// is needed to cater to all possible types of children values.
// [ts] fn normalizeChildren(children: auto) {
// [ts] return if (isPrimitive(children)) [createTextVNode(children)] else if (Array.isArray(children)) normalizeArrayChildren(children) else null
// [ts] }
// [ts] fn isTextNode(node: auto) {
// [ts] return isDef(node) && isDef(node.text) && isFalse(node.isComment)
// [ts] }
// [ts] fn normalizeArrayChildren(children: auto, nestedIndex: auto) {
// [ts] let res = [];
// [ts] let i, c, lastIndex, last;
// [ts] i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] c = children[i];
// [ts] if (isUndef(c) || @typeName(c) == "boolean") { continue }
// [ts] lastIndex = res.length - 1;
// [ts] last = res[lastIndex];
//  nested
// [ts] if (Array.isArray(c)) {
// [ts] if (c.length > 0) {
// [ts] c = normalizeArrayChildren(c, ((nestedIndex || "") + "_" + i));
// merge adjacent text nodes
// [ts] if (isTextNode(c[0]) && isTextNode(last)) {
// [ts] res[lastIndex] = createTextVNode(last.text + (c[0]).text);
// [ts] c.shift();
// [ts] }
// [ts] res.push.apply(res, c);
// [ts] }
// [ts] } else if (isPrimitive(c)) {
// [ts] if (isTextNode(last)) {
// merge adjacent text nodes
// this is necessary for SSR hydration because text nodes are
// essentially merged when rendered to HTML strings
// [ts] res[lastIndex] = createTextVNode(last.text + c);
// [ts] } else if (c != "") {
// convert primitive to vnode
// [ts] res.push(createTextVNode(c));
// [ts] }
// [ts] } else {
// [ts] if (isTextNode(c) && isTextNode(last)) {
// merge adjacent text nodes
// [ts] res[lastIndex] = createTextVNode(last.text + c.text);
// [ts] } else {
// default key for nested array children (likely generated by v-for)
// [ts] if (isTrue(children._isVList) &&
// [ts] isDef(c.tag) &&
// [ts] isUndef(c.key) &&
// [ts] isDef: (nestedIndex) => {
// [ts] c.key = "__vlist" + nestedIndex + "_" + i + "__";
// [ts] }
// [ts] res.push(c);
// [ts] }
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] /*  */
// [ts] fn initProvide(vm: auto) {
// [ts] let provide = vm.$options.provide;
// [ts] if (provide) {
// [ts] vm._provided = if (@typeName(provide) == "function") provide.call(vm) else provide;
// [ts] }
// [ts] }
// [ts] fn initInjections(vm: auto) {
// [ts] let result = resolveInject(vm.$options.inject, vm);
// [ts] if (result) {
// [ts] toggleObserving(false);
// [ts] Object.keys(result).forEach((key) => {
// [ts] /* istanbul ignore else */
// [ts] {
// [ts] defineReactive$$1(vm, key, result[key], () => {
// [ts] warn(
// [ts] "Avoid mutating an injected value directly since the changes will be " +
// [ts] "overwritten whenever the provided component re-renders. " +
// [ts] "injection being mutated: \"" + key + "\"",
// [ts] vm
// [ts] );
// [ts] });
// [ts] }
// [ts] });
// [ts] toggleObserving(true);
// [ts] }
// [ts] }
// [ts] fn resolveInject(inject: auto, vm: auto) {
// [ts] if (inject) {
// inject is :any because flow is not smart enough to figure out cached
// [ts] let result = Object.create(null);
// [ts] let keys = if (hasSymbol) Reflect.ownKeys(inject) else Object.keys(inject);
// [ts] let i = 0;
// [ts] while (i < keys.length) { // [cap-migrate] for-step: i++
// [ts] let key = keys[i];
// #6574 in case the inject object is observed...
// [ts] if (key == "__ob__") { continue }
// [ts] let provideKey = inject[key].from;
// [ts] let source = vm;
// [ts] while (source) {
// [ts] if (source._provided && hasOwn(source._provided, provideKey)) {
// [ts] result[key] = source._provided[provideKey];
// [ts] break
// [ts] }
// [ts] source = source.$parent;
// [ts] }
// [ts] if (!source) {
// [ts] if ("default" == inject[key]) {
// [ts] let provideDefault = inject[key].default;
// [ts] result[key] = if (@typeName(provideDefault) == "function") provideDefault.call(vm) else provideDefault;
// [ts] } else {
// [ts] warn(("Injection \"" + key + "\" not found"), vm);
// [ts] }
// [ts] }
// [ts] }
// [ts] return result
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Runtime helper for resolving raw children VNodes into a slot object.
// [ts] */
// [ts] fn resolveSlots(children: auto, context: auto) {
// [ts] if (!children || !children.length) {
// [ts] return {}
// [ts] }
// [ts] let slots = {};
// [ts] let i = 0, l = children.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] let child = children[i];
// [ts] let data = child.data;
// remove slot attribute if the node is resolved as a Vue slot node
// [ts] if (data && data.attrs && data.attrs.slot) {
// [ts] delete data.attrs.slot;
// [ts] }
// named slots should only be respected if the vnode was rendered in the
// same context.
// [ts] if ((child.context == context || child.fnContext == context) &&
// [ts] data && data.slot != null
// [ts] ) {
// [ts] let name = data.slot;
// [ts] let slot = (slots[name] || (slots[name] = []));
// [ts] if (child.tag == "template") {
// [ts] slot.push.apply(slot, child.children || []);
// [ts] } else {
// [ts] slot.push(child);
// [ts] }
// [ts] } else {
// [ts] (slots.default || (slots.default = [])).push(child);
// [ts] }
// [ts] }
// ignore slots that contains only whitespace
// [ts] for (let name$1 in slots) {
// [ts] if (slots[name$1].every(isWhitespace)) {
// [ts] delete slots[name$1];
// [ts] }
// [ts] }
// [ts] return slots
// [ts] }
// [ts] fn isWhitespace(node: auto) {
// [ts] return (node.isComment && !node.asyncFactory) || node.text == " "
// [ts] }
// [ts] /*  */
// [ts] fn isAsyncPlaceholder(node: auto) {
// [ts] return node.isComment && node.asyncFactory
// [ts] }
// [ts] /*  */
// [ts] fn normalizeScopedSlots(slots: auto, normalSlots: auto, prevSlots: auto) {
// [ts] let res;
// [ts] let hasNormalSlots = Object.keys(normalSlots).length > 0;
// [ts] let isStable = if (slots) !!slots.$stable else !hasNormalSlots;
// [ts] let key = slots && slots.$key;
// [ts] if (!slots) {
// [ts] res = {};
// [ts] } else if (slots._normalized) {
// fast path 1: child component re-render only, parent did not change
// [ts] return slots._normalized
// [ts] } else if (
// [ts] isStable &&
// [ts] prevSlots &&
// [ts] prevSlots != emptyObject &&
// [ts] key == prevSlots.$key &&
// [ts] !hasNormalSlots &&
// [ts] !prevSlots.$hasNormal
// [ts] ) {
// fast path 2: stable scoped slots w/ no normal slots to proxy,
// only need to normalize once
// [ts] return prevSlots
// [ts] } else {
// [ts] res = {};
// [ts] for (let key$1 in slots) {
// [ts] if (slots[key$1] && key$1[0] != "$") {
// [ts] res[key$1] = normalizeScopedSlot(normalSlots, key$1, slots[key$1]);
// [ts] }
// [ts] }
// [ts] }
// expose normal slots on scopedSlots
// [ts] for (let key$2 in normalSlots) {
// [ts] if (!(key$2 == res)) {
// [ts] res[key$2] = proxyNormalSlot(normalSlots, key$2);
// [ts] }
// [ts] }
// avoriaz seems to mock a non-extensible $scopedSlots object
// and when that is passed down this would cause an error
// [ts] if (slots && Object.isExtensible(slots)) {
// [ts] (slots)._normalized = res;
// [ts] }
// [ts] def(res, "$stable", isStable);
// [ts] def(res, "$key", key);
// [ts] def(res, "$hasNormal", hasNormalSlots);
// [ts] return res
// [ts] }
// [ts] fn normalizeScopedSlot(normalSlots: auto, key: auto, fn: auto) {
// [ts] let normalized = () => {
// [ts] let res = if (arguments.length) fn.apply(null, arguments) else fn({});
// [ts] res = res && @typeName(res) == "object" && !Array.isArray(res) ? [res] // single vnode : normalizeChildren(res);
// [ts] let vnode = res && res[0];
// [ts] return res && (
// [ts] !vnode ||
// [ts] (res.length == 1 && vnode.isComment && !isAsyncPlaceholder(vnode)) // #9658, #10391
// [ts] ) ? null : res
// [ts] };
// this is a slot using the new v-slot syntax without scope. although it is
// compiled as a scoped slot, render fn users would expect it to be present
// on this.$slots because the usage is semantically a normal slot.
// [ts] if (fn.proxy) {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(normalSlots, key, {
// [ts-skip] get: normalized,
// [ts-skip] enumerable: true,
// [ts-skip] configurable: true
// [ts-skip] });
// [ts] }
// [ts] return normalized
// [ts] }
// [ts] fn proxyNormalSlot(slots: auto, key: auto) {
// [ts] return () => { return slots[key]; }
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Runtime helper for rendering v-for lists.
// [ts] */
// [ts] fn renderList(val: auto, render: auto) {
// [ts] let ret, i, l, keys, key;
// [ts] if (Array.isArray(val) || @typeName(val) == "string") {
// [ts] ret = Array.init(val.length);
// [ts] i = 0, l = val.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] ret[i] = render(val[i], i);
// [ts] }
// [ts] } else if (@typeName(val) == "number") {
// [ts] ret = Array.init(val);
// [ts] i = 0;
// [ts] while (i < val) { // [cap-migrate] for-step: i++
// [ts] ret[i] = render(i + 1, i);
// [ts] }
// [ts] } else if (isObject(val)) {
// [ts] if (hasSymbol && val["__symbol_iterator"]) {
// [ts] ret = [];
// [ts] let iterator = val["__symbol_iterator"]();
// [ts] let result = iterator.next();
// [ts] while (!result.done) {
// [ts] ret.push(render(result.value, ret.length));
// [ts] result = iterator.next();
// [ts] }
// [ts] } else {
// [ts] keys = Object.keys(val);
// [ts] ret = Array.init(keys.length);
// [ts] i = 0, l = keys.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] key = keys[i];
// [ts] ret[i] = render(val[key], key, i);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!isDef(ret)) {
// [ts] ret = [];
// [ts] }
// [ts] (ret)._isVList = true;
// [ts] return ret
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Runtime helper for rendering <slot>
// [ts] */
// [ts] fn renderSlot(name: auto, fallbackRender: auto, props: auto, bindObject: auto) {
// [ts] let scopedSlotFn = _.$scopedSlots[name];
// [ts] let nodes;
// [ts] if (scopedSlotFn) {
// scoped slot
// [ts] props = props || {};
// [ts] if (bindObject) {
// [ts] if (!isObject(bindObject)) {
// [ts] warn("slot v-bind without argument expects an Object", this);
// [ts] }
// [ts] props = extend(extend({}, bindObject), props);
// [ts] }
// [ts] nodes =
// [ts] scopedSlotFn(props) ||
// [ts] (if (@typeName(fallbackRender) == "function") fallbackRender() else fallbackRender);
// [ts] } else {
// [ts] nodes =
// [ts] _.$slots[name] ||
// [ts] (if (@typeName(fallbackRender) == "function") fallbackRender() else fallbackRender);
// [ts] }
// [ts] let target = props && props.slot;
// [ts] if (target) {
// [ts] return _.$createElement("template", { slot: target }, nodes)
// [ts] } else {
// [ts] return nodes
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Runtime helper for resolving filters
// [ts] */
// [ts] fn resolveFilter(id: auto) {
// [ts] return resolveAsset(_.$options, "filters", id, true) || identity
// [ts] }
// [ts] /*  */
// [ts] fn isKeyNotMatch(expect: auto, actual: auto) {
// [ts] if (Array.isArray(expect)) {
// [ts] return expect.indexOf(actual) == -1
// [ts] } else {
// [ts] return expect != actual
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Runtime helper for checking keyCodes from config.
// [ts] * exposed as Vue.prototype._k
// [ts] * passing in eventKeyName as last argument separately for backwards compat
// [ts] */
// [ts] fn checkKeyCodes(eventKeyCode: auto, key: auto, builtInKeyCode: auto, eventKeyName: auto, builtInKeyName: auto) {
// [ts] let mappedKeyCode = config.keyCodes[key] || builtInKeyCode;
// [ts] if (builtInKeyName && eventKeyName && !config.keyCodes[key]) {
// [ts] return isKeyNotMatch(builtInKeyName, eventKeyName)
// [ts] } else if (mappedKeyCode) {
// [ts] return isKeyNotMatch(mappedKeyCode, eventKeyCode)
// [ts] } else if (eventKeyName) {
// [ts] return hyphenate(eventKeyName) != key
// [ts] }
// [ts] return eventKeyCode == null
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Runtime helper for merging v-bind="object" into a VNode's data.
// [ts] */
// [ts] fn bindObjectProps(data: auto, tag: auto, value: auto, asProp: auto, isSync: auto) {
// [ts] if (value) {
// [ts] if (!isObject(value)) {
// [ts] warn(
// [ts] "v-bind without argument expects an Object or Array value",
// [ts] this
// [ts] );
// [ts] } else {
// [ts] if (Array.isArray(value)) {
// [ts] value = toObject(value);
// [ts] }
// [ts] let hash;
// [ts] let loop = (key) => {
// [ts] if (
// [ts] key == "class" ||
// [ts] key == "style" ||
// [ts] isReservedAttribute(key)
// [ts] ) {
// [ts] hash = data;
// [ts] } else {
// [ts] let type = data.attrs && data.attrs.type;
// [ts] hash = if (asProp || config.mustUseProp(tag, type, key)) data.domProps || (data.domProps = {}) else data.attrs || (data.attrs = {});
// [ts] }
// [ts] let camelizedKey = camelize(key);
// [ts] let hyphenatedKey = hyphenate(key);
// [ts] if (!(camelizedKey == hash) && !(hyphenatedKey == hash)) {
// [ts] hash[key] = value[key];
// [ts] if (isSync) {
// [ts] let on = data.on || (data.on = {});
// [ts] on[("update:" + key)] = ($event) => {
// [ts] value[key] = $event;
// [ts] };
// [ts] }
// [ts] }
// [ts] };
// [ts] for (let key in value) loop( key );
// [ts] }
// [ts] }
// [ts] return data
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Runtime helper for rendering static trees.
// [ts] */
// [ts] fn renderStatic(index: auto, isInFor: auto) {
// [ts] let cached = _._staticTrees || (_._staticTrees = []);
// [ts] let tree = cached[index];
// if has already-rendered static tree and not inside v-for,
// we can reuse the same tree.
// [ts] if (tree && !isInFor) {
// [ts] return tree
// [ts] }
// otherwise, render a fresh tree.
// [ts] tree = cached[index] = _.$options.staticRenderFns[index].call(
// [ts] _._renderProxy,
// [ts] null,
// [ts] this // for render fns generated for functional component templates
// [ts] );
// [ts] markStatic(tree, ("__static__" + index), false);
// [ts] return tree
// [ts] }
// [ts] /**
// [ts] * Runtime helper for v-once.
// [ts] * Effectively it means marking the node as static with a unique key.
// [ts] */
// [ts] fn markOnce(tree: auto, index: auto, key: auto) {
// [ts] markStatic(tree, ("__once__" + index + (if (key) ("_" + key) else "")), true);
// [ts] return tree
// [ts] }
// [ts] fn markStatic(tree: auto, key: auto, isOnce: auto) {
// [ts] if (Array.isArray(tree)) {
// [ts] let i = 0;
// [ts] while (i < tree.length) { // [cap-migrate] for-step: i++
// [ts] if (tree[i] && @typeName(tree[i]) != "string") {
// [ts] markStaticNode(tree[i], (key + "_" + i), isOnce);
// [ts] }
// [ts] }
// [ts] } else {
// [ts] markStaticNode(tree, key, isOnce);
// [ts] }
// [ts] }
// [ts] fn markStaticNode(node: auto, key: auto, isOnce: auto) {
// [ts] node.isStatic = true;
// [ts] node.key = key;
// [ts] node.isOnce = isOnce;
// [ts] }
// [ts] /*  */
// [ts] fn bindObjectListeners(data: auto, value: auto) {
// [ts] if (value) {
// [ts] if (!isPlainObject(value)) {
// [ts] warn(
// [ts] "v-on without argument expects an Object value",
// [ts] this
// [ts] );
// [ts] } else {
// [ts] let on = data.on = if (data.on) extend({}, data.on) else {};
// [ts] for (let key in value) {
// [ts] let existing = on[key];
// [ts] let ours = value[key];
// [ts] on[key] = if (existing) [].concat(existing, ours) else ours;
// [ts] }
// [ts] }
// [ts] }
// [ts] return data
// [ts] }
// [ts] /*  */
// [ts] fn resolveScopedSlots(fns: auto, // see flow/vnode res: auto, // the following are added in 2.6 hasDynamicKeys: auto, contentHashKey: auto) {
// [ts] res = res || { $stable: !hasDynamicKeys };
// [ts] let i = 0;
// [ts] while (i < fns.length) { // [cap-migrate] for-step: i++
// [ts] let slot = fns[i];
// [ts] if (Array.isArray(slot)) {
// [ts] resolveScopedSlots(slot, res, hasDynamicKeys);
// [ts] } else if (slot) {
// marker for reverse proxying v-slot without scope on this.$slots
// [ts] if (slot.proxy) {
// [ts] slot.fn.proxy = true;
// [ts] }
// [ts] res[slot.key] = slot.fn;
// [ts] }
// [ts] }
// [ts] if (contentHashKey) {
// [ts] (res).$key = contentHashKey;
// [ts] }
// [ts] return res
// [ts] }
// [ts] /*  */
// [ts] fn bindDynamicKeys(baseObj: auto, values: auto) {
// [ts] let i = 0;
// [ts] while (i < values.length) { // [cap-migrate] for-step: i += 2
// [ts] let key = values[i];
// [ts] if (@typeName(key) == "string" && key) {
// [ts] baseObj[values[i]] = values[i + 1];
// [ts] } else if (key != "" && key != null) {
// null is a special value for explicitly removing a binding
// [ts] warn(
// [ts] ("Invalid value for dynamic directive argument (expected string or null): " + key),
// [ts] this
// [ts] );
// [ts] }
// [ts] }
// [ts] return baseObj
// [ts] }
// helper to dynamically append modifier runtime markers to event names.
// ensure only append when value is already string, otherwise it will be cast
// to string and cause the type check to miss.
// [ts] fn prependModifier(value: auto, symbol: auto) {
// [ts] return if (@typeName(value) == "string") symbol + value else value
// [ts] }
// [ts] /*  */
// [ts] fn installRenderHelpers(target: auto) {
// [ts] target._o = markOnce;
// [ts] target._n = toNumber;
// [ts] target._s = toString;
// [ts] target._l = renderList;
// [ts] target._t = renderSlot;
// [ts] target._q = looseEqual;
// [ts] target._i = looseIndexOf;
// [ts] target._m = renderStatic;
// [ts] target._f = resolveFilter;
// [ts] target._k = checkKeyCodes;
// [ts] target._b = bindObjectProps;
// [ts] target._v = createTextVNode;
// [ts] target._e = createEmptyVNode;
// [ts] target._u = resolveScopedSlots;
// [ts] target._g = bindObjectListeners;
// [ts] target._d = bindDynamicKeys;
// [ts] target._p = prependModifier;
// [ts] }
// [ts] /*  */
// [ts] fn FunctionalRenderContext(data: auto, props: auto, children: auto, parent: auto, Ctor: auto) {
// [ts] let this$1 = this;
// [ts] let options = Ctor.options;
// ensure the createElement function in functional components
// gets a unique context - this is necessary for correct named slot check
// [ts] let contextVm;
// [ts] if (hasOwn(parent, "_uid")) {
// [ts] contextVm = Object.create(parent);
// $flow-disable-line
// [ts] contextVm._original = parent;
// [ts] } else {
// the context vm passed in is a functional context as well.
// in this case we want to make sure we are able to get a hold to the
// real context instance.
// [ts] contextVm = parent;
// $flow-disable-line
// [ts] parent = parent._original;
// [ts] }
// [ts] let isCompiled = isTrue(options._compiled);
// [ts] let needNormalization = !isCompiled;
// [ts] _.data = data;
// [ts] _.props = props;
// [ts] _.children = children;
// [ts] _.parent = parent;
// [ts] _.listeners = data.on || emptyObject;
// [ts] _.injections = resolveInject(options.inject, parent);
// [ts] _.slots = () => {
// [ts] if (!this$1.$slots) {
// [ts] normalizeScopedSlots(
// [ts] data.scopedSlots,
// [ts] this$1.$slots = resolveSlots(children, parent)
// [ts] );
// [ts] }
// [ts] return this$1.$slots
// [ts] };
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'scopedSlots', ({
// [ts-skip] enumerable: true,
// [ts-skip] get: function get () {
// [ts-skip] return normalizeScopedSlots(data.scopedSlots, this.slots())
// [ts-skip] }
// [ts-skip] }));
// support for compiled functional template
// [ts] if (isCompiled) {
// exposing $options for renderStatic()
// [ts] _.$options = options;
// pre-resolve slots for renderSlot()
// [ts] _.$slots = _.slots();
// [ts] _.$scopedSlots = normalizeScopedSlots(data.scopedSlots, _.$slots);
// [ts] }
// [ts] if (options._scopeId) {
// [ts] _._c = (a, b, c, d) => {
// [ts] let vnode = createElement(contextVm, a, b, c, d, needNormalization);
// [ts] if (vnode && !Array.isArray(vnode)) {
// [ts] vnode.fnScopeId = options._scopeId;
// [ts] vnode.fnContext = parent;
// [ts] }
// [ts] return vnode
// [ts] };
// [ts] } else {
// [ts] _._c = (a, b, c, d) => { return createElement(contextVm, a, b, c, d, needNormalization); };
// [ts] }
// [ts] }
// [ts] installRenderHelpers(FunctionalRenderContext.prototype);
// [ts] fn createFunctionalComponent(Ctor: auto, propsData: auto, data: auto, contextVm: auto, children: auto) {
// [ts] let options = Ctor.options;
// [ts] let props = {};
// [ts] let propOptions = options.props;
// [ts] if (isDef(propOptions)) {
// [ts] for (let key in propOptions) {
// [ts] props[key] = validateProp(key, propOptions, propsData || emptyObject);
// [ts] }
// [ts] } else {
// [ts] if (isDef(data.attrs)) { mergeProps(props, data.attrs); }
// [ts] if (isDef(data.props)) { mergeProps(props, data.props); }
// [ts] }
// [ts] let renderContext = FunctionalRenderContext.init(
// [ts] data,
// [ts] props,
// [ts] children,
// [ts] contextVm,
// [ts] Ctor
// [ts] );
// [ts] let vnode = options.render.call(null, renderContext._c, renderContext);
// [ts] if (vnode == VNode) {
// [ts] return cloneAndMarkFunctionalResult(vnode, data, renderContext.parent, options, renderContext)
// [ts] } else if (Array.isArray(vnode)) {
// [ts] let vnodes = normalizeChildren(vnode) || [];
// [ts] let res = Array.init(vnodes.length);
// [ts] let i = 0;
// [ts] while (i < vnodes.length) { // [cap-migrate] for-step: i++
// [ts] res[i] = cloneAndMarkFunctionalResult(vnodes[i], data, renderContext.parent, options, renderContext);
// [ts] }
// [ts] return res
// [ts] }
// [ts] }
// [ts] fn cloneAndMarkFunctionalResult(vnode: auto, data: auto, contextVm: auto, options: auto, renderContext: auto) {
// #7817 clone node before setting fnContext, otherwise if the node is reused
// (e.g. it was from a cached normal slot) the fnContext causes named slots
// that should not be matched to match.
// [ts] let clone = cloneVNode(vnode);
// [ts] clone.fnContext = contextVm;
// [ts] clone.fnOptions = options;
// [ts] {
// [ts] (clone.devtoolsMeta = clone.devtoolsMeta || {}).renderContext = renderContext;
// [ts] }
// [ts] if (data.slot) {
// [ts] (clone.data || (clone.data = {})).slot = data.slot;
// [ts] }
// [ts] return clone
// [ts] }
// [ts] fn mergeProps(to: auto, from: auto) {
// [ts] for (let key in from) {
// [ts] to[camelize(key)] = from[key];
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] /*  */
// [ts] /*  */
// [ts] /*  */
// inline hooks to be invoked on component VNodes during patch
// [ts] let componentVNodeHooks = {
// [ts] init: (vnode, hydrating) => {
// [ts] if (
// [ts] vnode.componentInstance &&
// [ts] !vnode.componentInstance._isDestroyed &&
// [ts] vnode.data.keepAlive
// [ts] ) {
// kept-alive components, treat as a patch
// [ts] let mountedNode = vnode; // work around flow
// [ts] componentVNodeHooks.prepatch(mountedNode, mountedNode);
// [ts] } else {
// [ts] let child = vnode.componentInstance = createComponentInstanceForVnode(
// [ts] vnode,
// [ts] activeInstance
// [ts] );
// [ts] child.$mount(if (hydrating) vnode.elm else null, hydrating);
// [ts] }
// [ts] },
// [ts] prepatch: (oldVnode, vnode) => {
// [ts] let options = vnode.componentOptions;
// [ts] let child = vnode.componentInstance = oldVnode.componentInstance;
// [ts] updateChildComponent(
// [ts] child,
// [ts] options.propsData, // updated props
// [ts] options.listeners, // updated listeners
// [ts] vnode, // new parent vnode
// [ts] options.children // new children
// [ts] );
// [ts] },
// [ts] insert: (vnode) => {
// [ts] let context = vnode.context;
// [ts] let componentInstance = vnode.componentInstance;
// [ts] if (!componentInstance._isMounted) {
// [ts] componentInstance._isMounted = true;
// [ts] callHook(componentInstance, "mounted");
// [ts] }
// [ts] if (vnode.data.keepAlive) {
// [ts] if (context._isMounted) {
// vue-router#1212
// During updates, a kept-alive component's child components may
// change, so directly walking the tree here may call activated hooks
// on incorrect children. Instead we push them into a queue which will
// be processed after the whole patch process ended.
// [ts] queueActivatedComponent(componentInstance);
// [ts] } else {
// [ts] activateChildComponent(componentInstance, true /* direct */);
// [ts] }
// [ts] }
// [ts] },
// [ts] destroy: (vnode) => {
// [ts] let componentInstance = vnode.componentInstance;
// [ts] if (!componentInstance._isDestroyed) {
// [ts] if (!vnode.data.keepAlive) {
// [ts] componentInstance.$destroy();
// [ts] } else {
// [ts] deactivateChildComponent(componentInstance, true /* direct */);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] let hooksToMerge = Object.keys(componentVNodeHooks);
// [ts] fn createComponent(Ctor: auto, data: auto, context: auto, children: auto, tag: auto) {
// [ts] if (isUndef(Ctor)) {
// [ts] return
// [ts] }
// [ts] let baseCtor = context.$options._base;
// plain options object: turn it into a constructor
// [ts] if (isObject(Ctor)) {
// [ts] Ctor = baseCtor.extend(Ctor);
// [ts] }
// if at this stage it's not a constructor or an async component factory,
// reject.
// [ts] if (@typeName(Ctor) != "function") {
// [ts] {
// [ts] warn(("Invalid Component definition: " + (String(Ctor))), context);
// [ts] }
// [ts] return
// [ts] }
// async component
// [ts] let asyncFactory;
// [ts] if (isUndef(Ctor.cid)) {
// [ts] asyncFactory = Ctor;
// [ts] Ctor = resolveAsyncComponent(asyncFactory, baseCtor);
// [ts] if (Ctor == null) {
// return a placeholder node for async component, which is rendered
// as a comment node but preserves all the raw information for the node.
// the information will be used for async server-rendering and hydration.
// [ts] return createAsyncPlaceholder(
// [ts] asyncFactory,
// [ts] data,
// [ts] context,
// [ts] children,
// [ts] tag
// [ts] )
// [ts] }
// [ts] }
// [ts] data = data || {};
// resolve constructor options in case global mixins are applied after
// component constructor creation
// [ts] resolveConstructorOptions(Ctor);
// transform component v-model data into props & events
// [ts] if (isDef(data.model)) {
// [ts] transformModel(Ctor.options, data);
// [ts] }
// extract props
// [ts] let propsData = extractPropsFromVNodeData(data, Ctor, tag);
// functional component
// [ts] if (isTrue(Ctor.options.functional)) {
// [ts] return createFunctionalComponent(Ctor, propsData, data, context, children)
// [ts] }
// extract listeners, since these needs to be treated as
// child component listeners instead of DOM listeners
// [ts] let listeners = data.on;
// replace with listeners with .native modifier
// so it gets processed during parent component patch.
// [ts] data.on = data.nativeOn;
// [ts] if (isTrue(Ctor.options.abstract)) {
// abstract components do not keep anything
// other than props & listeners & slot
// work around flow
// [ts] let slot = data.slot;
// [ts] data = {};
// [ts] if (slot) {
// [ts] data.slot = slot;
// [ts] }
// [ts] }
// install component management hooks onto the placeholder node
// [ts] installComponentHooks(data);
// return a placeholder vnode
// [ts] let name = Ctor.options.name || tag;
// [ts] let vnode = VNode.init(
// [ts] ("vue-component-" + (Ctor.cid) + (if (name) ("-" + name) else "")),
// [ts] data, null, null, null, context,
// [ts] { Ctor: Ctor, propsData: propsData, listeners: listeners, tag: tag, children: children },
// [ts] asyncFactory
// [ts] );
// [ts] return vnode
// [ts] }
// [ts] fn createComponentInstanceForVnode(// we know it's MountedComponentVNode but flow doesn't vnode: auto, // activeInstance in lifecycle state parent: auto) {
// [ts] let options = {
// [ts] _isComponent: true,
// [ts] _parentVnode: vnode,
// [ts] parent: parent
// [ts] };
// check inline-template render functions
// [ts] let inlineTemplate = vnode.data.inlineTemplate;
// [ts] if (isDef(inlineTemplate)) {
// [ts] options.render = inlineTemplate.render;
// [ts] options.staticRenderFns = inlineTemplate.staticRenderFns;
// [ts] }
// [ts] return vnode.componentOptions.Ctor.init(options)
// [ts] }
// [ts] fn installComponentHooks(data: auto) {
// [ts] let hooks = data.hook || (data.hook = {});
// [ts] let i = 0;
// [ts] while (i < hooksToMerge.length) { // [cap-migrate] for-step: i++
// [ts] let key = hooksToMerge[i];
// [ts] let existing = hooks[key];
// [ts] let toMerge = componentVNodeHooks[key];
// [ts] if (existing != toMerge && !(existing && existing._merged)) {
// [ts] hooks[key] = if (existing) mergeHook$1(toMerge, existing) else toMerge;
// [ts] }
// [ts] }
// [ts] }
// [ts] (f1, f2) => {
// [ts] let merged = (a, b) => {
// flow complains about extra args which is why we use any
// [ts] f1(a, b);
// [ts] f2(a, b);
// [ts] };
// [ts] merged._merged = true;
// [ts] return merged
// [ts] }
// transform component v-model info (value and callback) into
// prop and event handler respectively.
// [ts] fn transformModel(options: auto, data: auto) {
// [ts] let prop = (options.model && options.model.prop) || "value";
// [ts] let event = (options.model && options.model.event) || "input"
// [ts] ;(data.attrs || (data.attrs = {}))[prop] = data.model.value;
// [ts] let on = data.on || (data.on = {});
// [ts] let existing = on[event];
// [ts] let callback = data.model.callback;
// [ts] if (isDef(existing)) {
// [ts] if (
// [ts] if (Array.isArray(existing)) existing.indexOf(callback) == -1 else existing != callback
// [ts] ) {
// [ts] on[event] = [callback].concat(existing);
// [ts] }
// [ts] } else {
// [ts] on[event] = callback;
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let SIMPLE_NORMALIZE = 1;
// [ts] let ALWAYS_NORMALIZE = 2;
// wrapper function for providing a more flexible interface
// without getting yelled at by flow
// [ts] fn createElement(context: auto, tag: auto, data: auto, children: auto, normalizationType: auto, alwaysNormalize: auto) {
// [ts] if (Array.isArray(data) || isPrimitive(data)) {
// [ts] normalizationType = children;
// [ts] children = data;
// [ts] data = null;
// [ts] }
// [ts] if (isTrue(alwaysNormalize)) {
// [ts] normalizationType = ALWAYS_NORMALIZE;
// [ts] }
// [ts] return _createElement(context, tag, data, children, normalizationType)
// [ts] }
// [ts] fn _createElement(context: auto, tag: auto, data: auto, children: auto, normalizationType: auto) {
// [ts] if (isDef(data) && isDef((data).__ob__)) {
// [ts] warn(
// [ts] "Avoid using observed data object as vnode data: " + (JSON.stringify(data)) + "\n" +
// [ts] "Always create fresh vnode data objects in each render!",
// [ts] context
// [ts] );
// [ts] return createEmptyVNode()
// [ts] }
// object syntax in v-bind
// [ts] if (isDef(data) && isDef(data.is)) {
// [ts] tag = data.is;
// [ts] }
// [ts] if (!tag) {
// in case of component :is set to falsy value
// [ts] return createEmptyVNode()
// [ts] }
// warn against non-primitive key
// [ts] if (isDef(data) && isDef(data.key) && !isPrimitive(data.key)
// [ts] ) {
// [ts] {
// [ts] warn(
// [ts] "Avoid using non-primitive value as key, " +
// [ts] "use string/number value instead.",
// [ts] context
// [ts] );
// [ts] }
// [ts] }
// support single function children as default scoped slot
// [ts] if (Array.isArray(children) &&
// [ts] @typeName(children[0]) == "function"
// [ts] ) {
// [ts] data = data || {};
// [ts] data.scopedSlots = { default: children[0] };
// [ts] children.length = 0;
// [ts] }
// [ts] if (normalizationType == ALWAYS_NORMALIZE) {
// [ts] children = normalizeChildren(children);
// [ts] } else if (normalizationType == SIMPLE_NORMALIZE) {
// [ts] children = simpleNormalizeChildren(children);
// [ts] }
// [ts] let vnode, ns;
// [ts] if (@typeName(tag) == "string") {
// [ts] let Ctor;
// [ts] ns = (context.$vnode && context.$vnode.ns) || config.getTagNamespace(tag);
// [ts] if (config.isReservedTag(tag)) {
// platform built-in elements
// [ts] if (isDef(data) && isDef(data.nativeOn) && data.tag != "component") {
// [ts] warn(
// [ts] ("The .native modifier for v-on is only valid on components but it was used on <" + tag + ">."),
// [ts] context
// [ts] );
// [ts] }
// [ts] vnode = VNode.init(
// [ts] config.parsePlatformTagName(tag), data, children,
// [ts] null, null, context
// [ts] );
// [ts] } else if ((!data || !data.pre) && isDef(Ctor = resolveAsset(context.$options, "components", tag))) {
// component
// [ts] vnode = createComponent(Ctor, data, context, children, tag);
// [ts] } else {
// unknown or unlisted namespaced elements
// check at runtime because it may get assigned a namespace when its
// parent normalizes children
// [ts] vnode = VNode.init(
// [ts] tag, data, children,
// [ts] null, null, context
// [ts] );
// [ts] }
// [ts] } else {
// direct component options / constructor
// [ts] vnode = createComponent(tag, data, context, children);
// [ts] }
// [ts] if (Array.isArray(vnode)) {
// [ts] return vnode
// [ts] } else if (isDef(vnode)) {
// [ts] if (isDef(ns)) { applyNS(vnode, ns); }
// [ts] if (isDef(data)) { registerDeepBindings(data); }
// [ts] return vnode
// [ts] } else {
// [ts] return createEmptyVNode()
// [ts] }
// [ts] }
// [ts] fn applyNS(vnode: auto, ns: auto, force: auto) {
// [ts] vnode.ns = ns;
// [ts] if (vnode.tag == "foreignObject") {
// use default namespace inside foreignObject
// [ts] ns = null;
// [ts] force = true;
// [ts] }
// [ts] if (isDef(vnode.children)) {
// [ts] let i = 0, l = vnode.children.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] let child = vnode.children[i];
// [ts] if (isDef(child.tag) && (
// [ts] isUndef: (child.ns) => {
// [ts] applyNS(child, ns, force);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// ref #5318
// necessary to ensure parent re-render when deep bindings like :style and
// :class are used on slot nodes
// [ts] fn registerDeepBindings(data: auto) {
// [ts] if (isObject(data.style)) {
// [ts] traverse(data.style);
// [ts] }
// [ts] if (isObject(data.class)) {
// [ts] traverse(data.class);
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn initRender(vm: auto) {
// [ts] vm._vnode = null; // the root of the child tree
// [ts] vm._staticTrees = null; // v-once cached trees
// [ts] let options = vm.$options;
// [ts] let parentVnode = vm.$vnode = options._parentVnode; // the placeholder node in parent tree
// [ts] let renderContext = parentVnode && parentVnode.context;
// [ts] vm.$slots = resolveSlots(options._renderChildren, renderContext);
// [ts] vm.$scopedSlots = emptyObject;
// bind the createElement fn to this instance
// so that we get proper render context inside it.
// args order: tag, data, children, normalizationType, alwaysNormalize
// internal version is used by render functions compiled from templates
// [ts] vm._c = (a, b, c, d) => { return createElement(vm, a, b, c, d, false); };
// normalization is always applied for the public version, used in
// user-written render functions.
// [ts] vm.$createElement = (a, b, c, d) => { return createElement(vm, a, b, c, d, true); };
// $attrs & $listeners are exposed for easier HOC creation.
// they need to be reactive so that HOCs using them are always updated
// [ts] let parentData = parentVnode && parentVnode.data;
// [ts] /* istanbul ignore else */
// [ts] {
// [ts] defineReactive$$1(vm, "$attrs", parentData && parentData.attrs || emptyObject, () => {
// [ts] !isUpdatingChildComponent && warn("$attrs is readonly.", vm);
// [ts] }, true);
// [ts] defineReactive$$1(vm, "$listeners", options._parentListeners || emptyObject, () => {
// [ts] !isUpdatingChildComponent && warn("$listeners is readonly.", vm);
// [ts] }, true);
// [ts] }
// [ts] }
// [ts] let currentRenderingInstance = null;
// [ts] fn renderMixin(Vue: auto) {
// install runtime convenience helpers
// [ts] installRenderHelpers(Vue.prototype);
// [ts] Vue.prototype.$nextTick = (fn) => {
// [ts] return nextTick(fn, this)
// [ts] };
// [ts] Vue.prototype._render = () => {
// [ts] let vm = this;
// [ts] let ref = vm.$options;
// [ts] let render = ref.render;
// [ts] let _parentVnode = ref._parentVnode;
// [ts] if (_parentVnode) {
// [ts] vm.$scopedSlots = normalizeScopedSlots(
// [ts] _parentVnode.data.scopedSlots,
// [ts] vm.$slots,
// [ts] vm.$scopedSlots
// [ts] );
// [ts] }
// set parent vnode. this allows render functions to have access
// to the data on the placeholder node.
// [ts] vm.$vnode = _parentVnode;
// render self
// [ts] let vnode;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// There's no need to maintain a stack because all render fns are called
// separately from one another. Nested component's render fns are called
// when parent component is patched.
// [ts] currentRenderingInstance = vm;
// [ts] vnode = render.call(vm._renderProxy, vm.$createElement);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] handleError(e, vm, "render");
// return error render result,
// or previous vnode to prevent render error causing blank component
// [ts] /* istanbul ignore else */
// [ts] if (vm.$options.renderError) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] vnode = vm.$options.renderError.call(vm._renderProxy, vm.$createElement, e);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] handleError(e, vm, "renderError");
// [ts] vnode = vm._vnode;
// [ts] }
// [ts] } else {
// [ts] vnode = vm._vnode;
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] currentRenderingInstance = null;
// [ts] }
// if the returned array contains only a single node, allow it
// [ts] if (Array.isArray(vnode) && vnode.length == 1) {
// [ts] vnode = vnode[0];
// [ts] }
// return empty vnode in case the render function errored out
// [ts] if (!(vnode == VNode)) {
// [ts] if (Array.isArray(vnode)) {
// [ts] warn(
// [ts] "Multiple root nodes returned from render function. Render function " +
// [ts] "should return a single root node.",
// [ts] vm
// [ts] );
// [ts] }
// [ts] vnode = createEmptyVNode();
// [ts] }
// set parent
// [ts] vnode.parent = _parentVnode;
// [ts] return vnode
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] fn ensureCtor(comp: auto, base: auto) {
// [ts] if (
// [ts] comp.__esModule ||
// [ts] (hasSymbol && comp["__symbol_toStringTag"] == "Module")
// [ts] ) {
// [ts] comp = comp.default;
// [ts] }
// [ts] return if (isObject(comp)) base.extend(comp) else comp
// [ts] }
// [ts] fn createAsyncPlaceholder(factory: auto, data: auto, context: auto, children: auto, tag: auto) {
// [ts] let node = createEmptyVNode();
// [ts] node.asyncFactory = factory;
// [ts] node.asyncMeta = { data: data, context: context, children: children, tag: tag };
// [ts] return node
// [ts] }
// [ts] fn resolveAsyncComponent(factory: auto, baseCtor: auto) {
// [ts] if (isTrue(factory.error) && isDef(factory.errorComp)) {
// [ts] return factory.errorComp
// [ts] }
// [ts] if (isDef(factory.resolved)) {
// [ts] return factory.resolved
// [ts] }
// [ts] let owner = currentRenderingInstance;
// [ts] if (owner && isDef(factory.owners) && factory.owners.indexOf(owner) == -1) {
// already pending
// [ts] factory.owners.push(owner);
// [ts] }
// [ts] if (isTrue(factory.loading) && isDef(factory.loadingComp)) {
// [ts] return factory.loadingComp
// [ts] }
// [ts] if (owner && !isDef(factory.owners)) {
// [ts] let owners = factory.owners = [owner];
// [ts] let sync = true;
// [ts] let timerLoading = null;
// [ts] let timerTimeout = null
// [ts] ;(owner).$on("hook:destroyed", () => { return remove(owners, owner); });
// [ts] let forceRender = (renderCompleted) => {
// [ts] let i = 0, l = owners.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] (owners[i]).$forceUpdate();
// [ts] }
// [ts] if (renderCompleted) {
// [ts] owners.length = 0;
// [ts] if (timerLoading != null) {
// [ts] clearTimeout(timerLoading);
// [ts] timerLoading = null;
// [ts] }
// [ts] if (timerTimeout != null) {
// [ts] clearTimeout(timerTimeout);
// [ts] timerTimeout = null;
// [ts] }
// [ts] }
// [ts] };
// [ts] let resolve = once((res) => {
// cache resolved
// [ts] factory.resolved = ensureCtor(res, baseCtor);
// invoke callbacks only if this is not a synchronous resolve
// (async resolves are shimmed as synchronous during SSR)
// [ts] if (!sync) {
// [ts] forceRender(true);
// [ts] } else {
// [ts] owners.length = 0;
// [ts] }
// [ts] });
// [ts] let reject = once((reason) => {
// [ts] warn(
// [ts] "Failed to resolve async component: " + (String(factory)) +
// [ts] (if (reason) ("\nReason: " + reason) else "")
// [ts] );
// [ts] if (isDef(factory.errorComp)) {
// [ts] factory.error = true;
// [ts] forceRender(true);
// [ts] }
// [ts] });
// [ts] let res = factory(resolve, reject);
// [ts] if (isObject(res)) {
// [ts] if (isPromise(res)) {
// () => Promise
// [ts] if (isUndef(factory.resolved)) {
// [ts] res.then(resolve, reject);
// [ts] }
// [ts] } else if (isPromise(res.component)) {
// [ts] res.component.then(resolve, reject);
// [ts] if (isDef(res.error)) {
// [ts] factory.errorComp = ensureCtor(res.error, baseCtor);
// [ts] }
// [ts] if (isDef(res.loading)) {
// [ts] factory.loadingComp = ensureCtor(res.loading, baseCtor);
// [ts] if (res.delay == 0) {
// [ts] factory.loading = true;
// [ts] } else {
// [ts] timerLoading = setTimeout(() => {
// [ts] timerLoading = null;
// [ts] if (isUndef(factory.resolved) && isUndef(factory.error)) {
// [ts] factory.loading = true;
// [ts] forceRender(false);
// [ts] }
// [ts] }, res.delay || 200);
// [ts] }
// [ts] }
// [ts] if (isDef(res.timeout)) {
// [ts] timerTimeout = setTimeout(() => {
// [ts] timerTimeout = null;
// [ts] if (isUndef(factory.resolved)) {
// [ts] reject(
// [ts] "timeout (" + (res.timeout) + "ms)"
// [ts] );
// [ts] }
// [ts] }, res.timeout);
// [ts] }
// [ts] }
// [ts] }
// [ts] sync = false;
// return in case resolved synchronously
// [ts] return if (factory.loading) factory.loadingComp else factory.resolved
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn getFirstComponentChild(children: auto) {
// [ts] if (Array.isArray(children)) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] let c = children[i];
// [ts] if (isDef(c) && (isDef(c.componentOptions) || isAsyncPlaceholder(c))) {
// [ts] return c
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] /*  */
// [ts] fn initEvents(vm: auto) {
// [ts] vm._events = Object.create(null);
// [ts] vm._hasHookEvent = false;
// init parent attached events
// [ts] let listeners = vm.$options._parentListeners;
// [ts] if (listeners) {
// [ts] updateComponentListeners(vm, listeners);
// [ts] }
// [ts] }
// [ts] let target;
// [ts] fn add(event: auto, fn: auto) {
// [ts] target.$on(event, fn);
// [ts] }
// [ts] (event, fn) => {
// [ts] target.$off(event, fn);
// [ts] }
// [ts] fn createOnceHandler(event: auto, fn: auto) {
// [ts] let _target = target;
// [ts] return () => {
// [ts] let res = fn.apply(null, arguments);
// [ts] if (res != null) {
// [ts] _target.$off(event, onceHandler);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn updateComponentListeners(vm: auto, listeners: auto, oldListeners: auto) {
// [ts] target = vm;
// [ts] updateListeners(listeners, oldListeners || {}, add, remove$1, createOnceHandler, vm);
// [ts] target = null;
// [ts] }
// [ts] fn eventsMixin(Vue: auto) {
// [ts] let hookRE = "^hook:";
// [ts] Vue.prototype.$on = (event, fn) => {
// [ts] let vm = this;
// [ts] if (Array.isArray(event)) {
// [ts] let i = 0, l = event.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] vm.$on(event[i], fn);
// [ts] }
// [ts] } else {
// [ts] (vm._events[event] || (vm._events[event] = [])).push(fn);
// optimize hook:event cost by using a boolean flag marked at registration
// instead of a hash lookup
// [ts] if (hookRE.test(event)) {
// [ts] vm._hasHookEvent = true;
// [ts] }
// [ts] }
// [ts] return vm
// [ts] };
// [ts] Vue.prototype.$once = (event, fn) => {
// [ts] let vm = this;
// [ts] fn on() {
// [ts] vm.$off(event, on);
// [ts] fn.apply(vm, arguments);
// [ts] }
// [ts] on.fn = fn;
// [ts] vm.$on(event, on);
// [ts] return vm
// [ts] };
// [ts] Vue.prototype.$off = (event, fn) => {
// [ts] let vm = this;
// all
// [ts] if (!arguments.length) {
// [ts] vm._events = Object.create(null);
// [ts] return vm
// [ts] }
// array of events
// [ts] if (Array.isArray(event)) {
// [ts] let i$1 = 0, l = event.length;
// [ts] while (i$1 < l) { // [cap-migrate] for-step: i$1++
// [ts] vm.$off(event[i$1], fn);
// [ts] }
// [ts] return vm
// [ts] }
// specific event
// [ts] let cbs = vm._events[event];
// [ts] if (!cbs) {
// [ts] return vm
// [ts] }
// [ts] if (!fn) {
// [ts] vm._events[event] = null;
// [ts] return vm
// [ts] }
// specific handler
// [ts] let cb;
// [ts] let i = cbs.length;
// [ts] while (i--) {
// [ts] cb = cbs[i];
// [ts] if (cb == fn || cb.fn == fn) {
// [ts] cbs.splice(i, 1);
// [ts] break
// [ts] }
// [ts] }
// [ts] return vm
// [ts] };
// [ts] Vue.prototype.$emit = (event) => {
// [ts] let vm = this;
// [ts] {
// [ts] let lowerCaseEvent = event.toLowerCase();
// [ts] if (lowerCaseEvent != event && vm._events[lowerCaseEvent]) {
// [ts] tip(
// [ts] "Event \"" + lowerCaseEvent + "\" is emitted in component " +
// [ts] (formatComponentName(vm)) + " but the handler is registered for \"" + event + "\". " +
// [ts] "Note that HTML attributes are case-insensitive and you cannot use " +
// [ts] "v-on to listen to camelCase events when using in-DOM templates. " +
// [ts] "You should probably use \"" + (hyphenate(event)) + "\" instead of \"" + event + "\"."
// [ts] );
// [ts] }
// [ts] }
// [ts] let cbs = vm._events[event];
// [ts] if (cbs) {
// [ts] cbs = if (cbs.length > 1) toArray(cbs) else cbs;
// [ts] let args = toArray(arguments, 1);
// [ts] let info = "event handler for \"" + event + "\"";
// [ts] let i = 0, l = cbs.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] invokeWithErrorHandling(cbs[i], vm, args, vm, info);
// [ts] }
// [ts] }
// [ts] return vm
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] let activeInstance = null;
// [ts] let isUpdatingChildComponent = false;
// [ts] fn setActiveInstance(vm: auto) {
// [ts] let prevActiveInstance = activeInstance;
// [ts] activeInstance = vm;
// [ts] return () => {
// [ts] activeInstance = prevActiveInstance;
// [ts] }
// [ts] }
// [ts] fn initLifecycle(vm: auto) {
// [ts] let options = vm.$options;
// locate first non-abstract parent
// [ts] let parent = options.parent;
// [ts] if (parent && !options.abstract) {
// [ts] while (parent.$options.abstract && parent.$parent) {
// [ts] parent = parent.$parent;
// [ts] }
// [ts] parent.$children.push(vm);
// [ts] }
// [ts] vm.$parent = parent;
// [ts] vm.$root = if (parent) parent.$root else vm;
// [ts] vm.$children = [];
// [ts] vm.$refs = {};
// [ts] vm._watcher = null;
// [ts] vm._inactive = null;
// [ts] vm._directInactive = false;
// [ts] vm._isMounted = false;
// [ts] vm._isDestroyed = false;
// [ts] vm._isBeingDestroyed = false;
// [ts] }
// [ts] fn lifecycleMixin(Vue: auto) {
// [ts] Vue.prototype._update = (vnode, hydrating) => {
// [ts] let vm = this;
// [ts] let prevEl = vm.$el;
// [ts] let prevVnode = vm._vnode;
// [ts] let restoreActiveInstance = setActiveInstance(vm);
// [ts] vm._vnode = vnode;
// Vue.prototype.__patch__ is injected in entry points
// based on the rendering backend used.
// [ts] if (!prevVnode) {
// initial render
// [ts] vm.$el = vm.__patch__(vm.$el, vnode, hydrating, false /* removeOnly */);
// [ts] } else {
// updates
// [ts] vm.$el = vm.__patch__(prevVnode, vnode);
// [ts] }
// [ts] restoreActiveInstance();
// update __vue__ reference
// [ts] if (prevEl) {
// [ts] prevEl.__vue__ = null;
// [ts] }
// [ts] if (vm.$el) {
// [ts] vm.$el.__vue__ = vm;
// [ts] }
// if parent is an HOC, update its $el as well
// [ts] if (vm.$vnode && vm.$parent && vm.$vnode == vm.$parent._vnode) {
// [ts] vm.$parent.$el = vm.$el;
// [ts] }
// updated hook is called by the scheduler to ensure that children are
// updated in a parent's updated hook.
// [ts] };
// [ts] Vue.prototype.$forceUpdate = () => {
// [ts] let vm = this;
// [ts] if (vm._watcher) {
// [ts] vm._watcher.update();
// [ts] }
// [ts] };
// [ts] Vue.prototype.$destroy = () => {
// [ts] let vm = this;
// [ts] if (vm._isBeingDestroyed) {
// [ts] return
// [ts] }
// [ts] callHook(vm, "beforeDestroy");
// [ts] vm._isBeingDestroyed = true;
// remove self from parent
// [ts] let parent = vm.$parent;
// [ts] if (parent && !parent._isBeingDestroyed && !vm.$options.abstract) {
// [ts] remove(parent.$children, vm);
// [ts] }
// teardown watchers
// [ts] if (vm._watcher) {
// [ts] vm._watcher.teardown();
// [ts] }
// [ts] let i = vm._watchers.length;
// [ts] while (i--) {
// [ts] vm._watchers[i].teardown();
// [ts] }
// remove reference from data ob
// frozen object may not have observer.
// [ts] if (vm._data.__ob__) {
// [ts] vm._data.__ob__.vmCount--;
// [ts] }
// call the last hook...
// [ts] vm._isDestroyed = true;
// invoke destroy hooks on current rendered tree
// [ts] vm.__patch__(vm._vnode, null);
// fire destroyed hook
// [ts] callHook(vm, "destroyed");
// turn off all instance listeners.
// [ts] vm.$off();
// remove __vue__ reference
// [ts] if (vm.$el) {
// [ts] vm.$el.__vue__ = null;
// [ts] }
// release circular reference (#6759)
// [ts] if (vm.$vnode) {
// [ts] vm.$vnode.parent = null;
// [ts] }
// [ts] };
// [ts] }
// [ts] fn mountComponent(vm: auto, el: auto, hydrating: auto) {
// [ts] vm.$el = el;
// [ts] if (!vm.$options.render) {
// [ts] vm.$options.render = createEmptyVNode;
// [ts] {
// [ts] /* istanbul ignore if */
// [ts] if ((vm.$options.template && vm.$options.template.charAt(0) != "#") ||
// [ts] vm.$options.el || el) {
// [ts] warn(
// [ts] "You are using the runtime-only build of Vue where the template " +
// [ts] "compiler is not available. Either pre-compile the templates into " +
// [ts] "render functions, or use the compiler-included build.",
// [ts] vm
// [ts] );
// [ts] } else {
// [ts] warn(
// [ts] "Failed to mount component: template or render function not defined.",
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] callHook(vm, "beforeMount");
// [ts] let updateComponent;
// [ts] /* istanbul ignore if */
// [ts] if (config.performance && mark) {
// [ts] updateComponent = () => {
// [ts] let name = vm._name;
// [ts] let id = vm._uid;
// [ts] let startTag = "vue-perf-start:" + id;
// [ts] let endTag = "vue-perf-end:" + id;
// [ts] mark(startTag);
// [ts] let vnode = vm._render();
// [ts] mark(endTag);
// [ts] measure(("vue " + name + " render"), startTag, endTag);
// [ts] mark(startTag);
// [ts] vm._update(vnode, hydrating);
// [ts] mark(endTag);
// [ts] measure(("vue " + name + " patch"), startTag, endTag);
// [ts] };
// [ts] } else {
// [ts] updateComponent = () => {
// [ts] vm._update(vm._render(), hydrating);
// [ts] };
// [ts] }
// we set this to vm._watcher inside the watcher's constructor
// since the watcher's initial patch may call $forceUpdate (e.g. inside child
// component's mounted hook), which relies on vm._watcher being already defined
// [ts] Watcher.init(vm, updateComponent, noop, {
// [ts] before: () => {
// [ts] if (vm._isMounted && !vm._isDestroyed) {
// [ts] callHook(vm, "beforeUpdate");
// [ts] }
// [ts] }
// [ts] }, true /* isRenderWatcher */);
// [ts] hydrating = false;
// manually mounted instance, call mounted on self
// mounted is called for render-created child components in its inserted hook
// [ts] if (vm.$vnode == null) {
// [ts] vm._isMounted = true;
// [ts] callHook(vm, "mounted");
// [ts] }
// [ts] return vm
// [ts] }
// [ts] fn updateChildComponent(vm: auto, propsData: auto, listeners: auto, parentVnode: auto, renderChildren: auto) {
// [ts] {
// [ts] isUpdatingChildComponent = true;
// [ts] }
// determine whether component has slot children
// we need to do this before overwriting $options._renderChildren.
// check if there are dynamic scopedSlots (hand-written or compiled but with
// dynamic slot names). Static scoped slots compiled from template has the
// "$stable" marker.
// [ts] let newScopedSlots = parentVnode.data.scopedSlots;
// [ts] let oldScopedSlots = vm.$scopedSlots;
// [ts] let hasDynamicScopedSlot = !!(
// [ts] (newScopedSlots && !newScopedSlots.$stable) ||
// [ts] (oldScopedSlots != emptyObject && !oldScopedSlots.$stable) ||
// [ts] (newScopedSlots && vm.$scopedSlots.$key != newScopedSlots.$key) ||
// [ts] (!newScopedSlots && vm.$scopedSlots.$key)
// [ts] );
// Any static slot children from the parent may have changed during parent's
// update. Dynamic scoped slots may also have changed. In such cases, a forced
// update is necessary to ensure correctness.
// [ts] let needsForceUpdate = !!(
// [ts] renderChildren ||               // has new static slots
// [ts] vm.$options._renderChildren ||  // has old static slots
// [ts] hasDynamicScopedSlot
// [ts] );
// [ts] vm.$options._parentVnode = parentVnode;
// [ts] vm.$vnode = parentVnode; // update vm's placeholder node without re-render
// [ts] if (vm._vnode) { // update child tree's parent
// [ts] vm._vnode.parent = parentVnode;
// [ts] }
// [ts] vm.$options._renderChildren = renderChildren;
// update $attrs and $listeners hash
// these are also reactive so they may trigger child update if the child
// used them during render
// [ts] vm.$attrs = parentVnode.data.attrs || emptyObject;
// [ts] vm.$listeners = listeners || emptyObject;
// update props
// [ts] if (propsData && vm.$options.props) {
// [ts] toggleObserving(false);
// [ts] let props = vm._props;
// [ts] let propKeys = vm.$options._propKeys || [];
// [ts] let i = 0;
// [ts] while (i < propKeys.length) { // [cap-migrate] for-step: i++
// [ts] let key = propKeys[i];
// [ts] let propOptions = vm.$options.props; // wtf flow?
// [ts] props[key] = validateProp(key, propOptions, propsData, vm);
// [ts] }
// [ts] toggleObserving(true);
// keep a copy of raw propsData
// [ts] vm.$options.propsData = propsData;
// [ts] }
// update listeners
// [ts] listeners = listeners || emptyObject;
// [ts] let oldListeners = vm.$options._parentListeners;
// [ts] vm.$options._parentListeners = listeners;
// [ts] updateComponentListeners(vm, listeners, oldListeners);
// resolve slots + force update if has children
// [ts] if (needsForceUpdate) {
// [ts] vm.$slots = resolveSlots(renderChildren, parentVnode.context);
// [ts] vm.$forceUpdate();
// [ts] }
// [ts] {
// [ts] isUpdatingChildComponent = false;
// [ts] }
// [ts] }
// [ts] fn isInInactiveTree(vm: auto) {
// [ts] while (vm && (vm = vm.$parent)) {
// [ts] if (vm._inactive) { return true }
// [ts] }
// [ts] return false
// [ts] }
// [ts] fn activateChildComponent(vm: auto, direct: auto) {
// [ts] if (direct) {
// [ts] vm._directInactive = false;
// [ts] if (isInInactiveTree(vm)) {
// [ts] return
// [ts] }
// [ts] } else if (vm._directInactive) {
// [ts] return
// [ts] }
// [ts] if (vm._inactive || vm._inactive == null) {
// [ts] vm._inactive = false;
// [ts] let i = 0;
// [ts] while (i < vm.$children.length) { // [cap-migrate] for-step: i++
// [ts] activateChildComponent(vm.$children[i]);
// [ts] }
// [ts] callHook(vm, "activated");
// [ts] }
// [ts] }
// [ts] fn deactivateChildComponent(vm: auto, direct: auto) {
// [ts] if (direct) {
// [ts] vm._directInactive = true;
// [ts] if (isInInactiveTree(vm)) {
// [ts] return
// [ts] }
// [ts] }
// [ts] if (!vm._inactive) {
// [ts] vm._inactive = true;
// [ts] let i = 0;
// [ts] while (i < vm.$children.length) { // [cap-migrate] for-step: i++
// [ts] deactivateChildComponent(vm.$children[i]);
// [ts] }
// [ts] callHook(vm, "deactivated");
// [ts] }
// [ts] }
// [ts] fn callHook(vm: auto, hook: auto) {
// #7573 disable dep collection when invoking lifecycle hooks
// [ts] pushTarget();
// [ts] let handlers = vm.$options[hook];
// [ts] let info = hook + " hook";
// [ts] if (handlers) {
// [ts] let i = 0, j = handlers.length;
// [ts] while (i < j) { // [cap-migrate] for-step: i++
// [ts] invokeWithErrorHandling(handlers[i], vm, null, vm, info);
// [ts] }
// [ts] }
// [ts] if (vm._hasHookEvent) {
// [ts] vm.$emit("hook:" + hook);
// [ts] }
// [ts] popTarget();
// [ts] }
// [ts] /*  */
// [ts] let MAX_UPDATE_COUNT = 100;
// [ts] let queue = [];
// [ts] let activatedChildren = [];
// [ts] let has = {};
// [ts] let circular = {};
// [ts] let waiting = false;
// [ts] let flushing = false;
// [ts] let index = 0;
// [ts] /**
// [ts] * Reset the scheduler's state.
// [ts] */
// [ts] fn resetSchedulerState() {
// [ts] index = queue.length = activatedChildren.length = 0;
// [ts] has = {};
// [ts] {
// [ts] circular = {};
// [ts] }
// [ts] waiting = flushing = false;
// [ts] }
// Async edge case #6566 requires saving the timestamp when event listeners are
// attached. However, calling performance.now() has a perf overhead especially
// if the page has thousands of event listeners. Instead, we take a timestamp
// every time the scheduler flushes and use that for all event listeners
// attached during that flush.
// [ts] let currentFlushTimestamp = 0;
// Async edge case fix requires storing an event listener's attach timestamp.
// [ts] let getNow = Date.now;
// Determine what event timestamp the browser is using. Annoyingly, the
// timestamp can either be hi-res (relative to page load) or low-res
// (relative to UNIX epoch), so in order to compare time we have to use the
// same timestamp type when saving the flush timestamp.
// All IE versions use low-res event timestamps, and have problematic clock
// implementations (#9632)
// [ts] if (inBrowser && !isIE) {
// [ts] let performance = window.performance;
// [ts] if (
// [ts] performance &&
// [ts] @typeName(performance.now) == "function" &&
// [ts] getNow() > document.createEvent("Event").timeStamp
// [ts] ) {
// if the event timestamp, although evaluated AFTER the Date.now(), is
// smaller than it, it means the event is using a hi-res timestamp,
// and we need to use the hi-res version for event listener timestamps as
// well.
// [ts] getNow = () => { return performance.now(); };
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Flush both queues and run the watchers.
// [ts] */
// [ts] fn flushSchedulerQueue() {
// [ts] currentFlushTimestamp = getNow();
// [ts] flushing = true;
// [ts] let watcher, id;
// Sort queue before flush.
// This ensures that:
// 1. Components are updated from parent to child. (because parent is always
//    created before the child)
// 2. A component's user watchers are run before its render watcher (because
//    user watchers are created before the render watcher)
// 3. If a component is destroyed during a parent component's watcher run,
//    its watchers can be skipped.
// [ts] queue.sort((a, b) => { return a.id - b.id; });
// do not cache length because more watchers might be pushed
// as we run existing watchers
// [ts] index = 0;
// [ts] while (index < queue.length) { // [cap-migrate] for-step: index++
// [ts] watcher = queue[index];
// [ts] if (watcher.before) {
// [ts] watcher.before();
// [ts] }
// [ts] id = watcher.id;
// [ts] has[id] = null;
// [ts] watcher.run();
// in dev build, check and stop circular updates.
// [ts] if (has[id] != null) {
// [ts] circular[id] = (circular[id] || 0) + 1;
// [ts] if (circular[id] > MAX_UPDATE_COUNT) {
// [ts] warn(
// [ts] "You may have an infinite update loop " + (
// [ts] if (watcher.user) ("in watcher with expression \"" + (watcher.expression) + "\"") else "in a component render function."
// [ts] ),
// [ts] watcher.vm
// [ts] );
// [ts] break
// [ts] }
// [ts] }
// [ts] }
// keep copies of post queues before resetting state
// [ts] let activatedQueue = activatedChildren.slice();
// [ts] let updatedQueue = queue.slice();
// [ts] resetSchedulerState();
// call component updated and activated hooks
// [ts] callActivatedHooks(activatedQueue);
// [ts] callUpdatedHooks(updatedQueue);
// devtool hook
// [ts] /* istanbul ignore if */
// [ts] if (devtools && config.devtools) {
// [ts] devtools.emit("flush");
// [ts] }
// [ts] }
// [ts] fn callUpdatedHooks(queue: auto) {
// [ts] let i = queue.length;
// [ts] while (i--) {
// [ts] let watcher = queue[i];
// [ts] let vm = watcher.vm;
// [ts] if (vm._watcher == watcher && vm._isMounted && !vm._isDestroyed) {
// [ts] callHook(vm, "updated");
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Queue a kept-alive component that was activated during patch.
// [ts] * The queue will be processed after the entire tree has been patched.
// [ts] */
// [ts] fn queueActivatedComponent(vm: auto) {
// setting _inactive to false here so that a render function can
// rely on checking whether it's in an inactive tree (e.g. router-view)
// [ts] vm._inactive = false;
// [ts] activatedChildren.push(vm);
// [ts] }
// [ts] fn callActivatedHooks(queue: auto) {
// [ts] let i = 0;
// [ts] while (i < queue.length) { // [cap-migrate] for-step: i++
// [ts] queue[i]._inactive = true;
// [ts] activateChildComponent(queue[i], true /* true */);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Push a watcher into the watcher queue.
// [ts] * Jobs with duplicate IDs will be skipped unless it's
// [ts] * pushed when the queue is being flushed.
// [ts] */
// [ts] fn queueWatcher(watcher: auto) {
// [ts] let id = watcher.id;
// [ts] if (has[id] == null) {
// [ts] has[id] = true;
// [ts] if (!flushing) {
// [ts] queue.push(watcher);
// [ts] } else {
// if already flushing, splice the watcher based on its id
// if already past its id, it will be run next immediately.
// [ts] let i = queue.length - 1;
// [ts] while (i > index && queue[i].id > watcher.id) {
// [ts] i--;
// [ts] }
// [ts] queue.splice(i + 1, 0, watcher);
// [ts] }
// queue the flush
// [ts] if (!waiting) {
// [ts] waiting = true;
// [ts] if (!config.async) {
// [ts] flushSchedulerQueue();
// [ts] return
// [ts] }
// [ts] nextTick(flushSchedulerQueue);
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let uid$2 = 0;
// [ts] /**
// [ts] * A watcher parses an expression, collects dependencies,
// [ts] * and fires callback when the expression value changes.
// [ts] * This is used for both the $watch() api and directives.
// [ts] */
// [ts] let Watcher = function Watcher (
// [ts] vm,
// [ts] expOrFn,
// [ts] cb,
// [ts] options,
// [ts] isRenderWatcher
// [ts] ) {
// [ts] _.vm = vm;
// [ts] if (isRenderWatcher) {
// [ts] vm._watcher = this;
// [ts] }
// [ts] vm._watchers.push(this);
// options
// [ts] if (options) {
// [ts] _.deep = !!options.deep;
// [ts] _.user = !!options.user;
// [ts] _.lazy = !!options.lazy;
// [ts] _.sync = !!options.sync;
// [ts] _.before = options.before;
// [ts] } else {
// [ts] _.deep = _.user = _.lazy = _.sync = false;
// [ts] }
// [ts] _.cb = cb;
// [ts] _.id = ++uid$2; // uid for batching
// [ts] _.active = true;
// [ts] _.dirty = _.lazy; // for lazy watchers
// [ts] _.deps = [];
// [ts] _.newDeps = [];
// [ts] _.depIds = _Set.init();
// [ts] _.newDepIds = _Set.init();
// [ts] _.expression = expOrFn.toString();
// parse expression for getter
// [ts] if (@typeName(expOrFn) == "function") {
// [ts] _.getter = expOrFn;
// [ts] } else {
// [ts] _.getter = parsePath(expOrFn);
// [ts] if (!_.getter) {
// [ts] _.getter = noop;
// [ts] warn(
// [ts] "Failed watching path: \"" + expOrFn + "\" " +
// [ts] "Watcher only accepts simple dot-delimited paths. " +
// [ts] "For full control, use a function instead.",
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] _.value = if (_.lazy) null else _.get();
// [ts] };
// [ts] /**
// [ts] * Evaluate the getter, and re-collect dependencies.
// [ts] */
// [ts] Watcher.prototype.get = () => {
// [ts] pushTarget(this);
// [ts] let value;
// [ts] let vm = _.vm;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] value = _.getter.call(vm, vm);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] if (_.user) {
// [ts] handleError(e, vm, ("getter for watcher \"" + (_.expression) + "\""));
// [ts] } else {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw e)
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// "touch" every property so they are all tracked as
// dependencies for deep watching
// [ts] if (_.deep) {
// [ts] traverse(value);
// [ts] }
// [ts] popTarget();
// [ts] _.cleanupDeps();
// [ts] }
// [ts] return value
// [ts] };
// [ts] /**
// [ts] * Add a dependency to this directive.
// [ts] */
// [ts] Watcher.prototype.addDep = (dep) => {
// [ts] let id = dep.id;
// [ts] if (!_.newDepIds.has(id)) {
// [ts] _.newDepIds.add(id);
// [ts] _.newDeps.push(dep);
// [ts] if (!_.depIds.has(id)) {
// [ts] dep.addSub(this);
// [ts] }
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Clean up for dependency collection.
// [ts] */
// [ts] Watcher.prototype.cleanupDeps = () => {
// [ts] let i = _.deps.length;
// [ts] while (i--) {
// [ts] let dep = _.deps[i];
// [ts] if (!_.newDepIds.has(dep.id)) {
// [ts] dep.removeSub(this);
// [ts] }
// [ts] }
// [ts] let tmp = _.depIds;
// [ts] _.depIds = _.newDepIds;
// [ts] _.newDepIds = tmp;
// [ts] _.newDepIds.clear();
// [ts] tmp = _.deps;
// [ts] _.deps = _.newDeps;
// [ts] _.newDeps = tmp;
// [ts] _.newDeps.length = 0;
// [ts] };
// [ts] /**
// [ts] * Subscriber interface.
// [ts] * Will be called when a dependency changes.
// [ts] */
// [ts] Watcher.prototype.update = () => {
// [ts] /* istanbul ignore else */
// [ts] if (_.lazy) {
// [ts] _.dirty = true;
// [ts] } else if (_.sync) {
// [ts] _.run();
// [ts] } else {
// [ts] queueWatcher(this);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Scheduler job interface.
// [ts] * Will be called by the scheduler.
// [ts] */
// [ts] Watcher.prototype.run = () => {
// [ts] if (_.active) {
// [ts] let value = _.get();
// [ts] if (
// [ts] value != _.value ||
// Deep watchers and watchers on Object/Arrays should fire even
// when the value is the same, because the value may
// have mutated.
// [ts] isObject(value) ||
// [ts] _.deep
// [ts] ) {
// set new value
// [ts] let oldValue = _.value;
// [ts] _.value = value;
// [ts] if (_.user) {
// [ts] let info = "callback for watcher \"" + (_.expression) + "\"";
// [ts] invokeWithErrorHandling(_.cb, _.vm, [value, oldValue], _.vm, info);
// [ts] } else {
// [ts] _.cb.call(_.vm, value, oldValue);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Evaluate the value of the watcher.
// [ts] * This only gets called for lazy watchers.
// [ts] */
// [ts] Watcher.prototype.evaluate = () => {
// [ts] _.value = _.get();
// [ts] _.dirty = false;
// [ts] };
// [ts] /**
// [ts] * Depend on all deps collected by this watcher.
// [ts] */
// [ts] Watcher.prototype.depend = () => {
// [ts] let i = _.deps.length;
// [ts] while (i--) {
// [ts] _.deps[i].depend();
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Remove self from all dependencies' subscriber list.
// [ts] */
// [ts] Watcher.prototype.teardown = () => {
// [ts] if (_.active) {
// remove self from vm's watcher list
// this is a somewhat expensive operation so we skip it
// if the vm is being destroyed.
// [ts] if (!_.vm._isBeingDestroyed) {
// [ts] remove(_.vm._watchers, this);
// [ts] }
// [ts] let i = _.deps.length;
// [ts] while (i--) {
// [ts] _.deps[i].removeSub(this);
// [ts] }
// [ts] _.active = false;
// [ts] }
// [ts] };
// [ts] /*  */
// [ts] let sharedPropertyDefinition = {
// [ts] enumerable: true,
// [ts] configurable: true,
// [ts] get: noop,
// [ts] set: noop
// [ts] };
// [ts] fn proxy(target: auto, sourceKey: auto, key: auto) {
// [ts] sharedPropertyDefinition.get = () => {
// [ts] return this[sourceKey][key]
// [ts] };
// [ts] sharedPropertyDefinition.set = (val) => {
// [ts] this[sourceKey][key] = val;
// [ts] };
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(target, key, sharedPropertyDefinition);
// [ts] }
// [ts] fn initState(vm: auto) {
// [ts] vm._watchers = [];
// [ts] let opts = vm.$options;
// [ts] if (opts.props) { initProps(vm, opts.props); }
// [ts] if (opts.methods) { initMethods(vm, opts.methods); }
// [ts] if (opts.data) {
// [ts] initData(vm);
// [ts] } else {
// [ts] observe(vm._data = {}, true /* asRootData */);
// [ts] }
// [ts] if (opts.computed) { initComputed(vm, opts.computed); }
// [ts] if (opts.watch && opts.watch != nativeWatch) {
// [ts] initWatch(vm, opts.watch);
// [ts] }
// [ts] }
// [ts] fn initProps(vm: auto, propsOptions: auto) {
// [ts] let propsData = vm.$options.propsData || {};
// [ts] let props = vm._props = {};
// cache prop keys so that future props updates can iterate using Array
// instead of dynamic object key enumeration.
// [ts] let keys = vm.$options._propKeys = [];
// [ts] let isRoot = !vm.$parent;
// root instance props should be converted
// [ts] if (!isRoot) {
// [ts] toggleObserving(false);
// [ts] }
// [ts] let loop = (key) => {
// [ts] keys.push(key);
// [ts] let value = validateProp(key, propsOptions, propsData, vm);
// [ts] /* istanbul ignore else */
// [ts] {
// [ts] let hyphenatedKey = hyphenate(key);
// [ts] if (isReservedAttribute(hyphenatedKey) ||
// [ts] config.isReservedAttr(hyphenatedKey)) {
// [ts] warn(
// [ts] ("\"" + hyphenatedKey + "\" is a reserved attribute and cannot be used as component prop."),
// [ts] vm
// [ts] );
// [ts] }
// [ts] defineReactive$$1(props, key, value, () => {
// [ts] if (!isRoot && !isUpdatingChildComponent) {
// [ts] warn(
// [ts] "Avoid mutating a prop directly since the value will be " +
// [ts] "overwritten whenever the parent component re-renders. " +
// [ts] "Instead, use a data or computed property based on the prop's " +
// [ts] "value. Prop being mutated: \"" + key + "\"",
// [ts] vm
// [ts] );
// [ts] }
// [ts] });
// [ts] }
// static props are already proxied on the component's prototype
// during Vue.extend(). We only need to proxy props defined at
// instantiation here.
// [ts] if (!(key == vm)) {
// [ts] proxy(vm, "_props", key);
// [ts] }
// [ts] };
// [ts] for (let key in propsOptions) loop( key );
// [ts] toggleObserving(true);
// [ts] }
// [ts] fn initData(vm: auto) {
// [ts] let data = vm.$options.data;
// [ts] data = vm._data = if (@typeName(data) == "function") getData(data, vm) else data || {};
// [ts] if (!isPlainObject(data)) {
// [ts] data = {};
// [ts] warn(
// [ts] "data functions should return an object:\n" +
// [ts] "https://vuejs.org/v2/guide/components.html#data-Must-Be-a-Function",
// [ts] vm
// [ts] );
// [ts] }
// proxy data on instance
// [ts] let keys = Object.keys(data);
// [ts] let props = vm.$options.props;
// [ts] let methods = vm.$options.methods;
// [ts] let i = keys.length;
// [ts] while (i--) {
// [ts] let key = keys[i];
// [ts] {
// [ts] if (methods && hasOwn(methods, key)) {
// [ts] warn(
// [ts] ("Method \"" + key + "\" has already been defined as a data property."),
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] if (props && hasOwn(props, key)) {
// [ts] warn(
// [ts] "The data property \"" + key + "\" is already declared as a prop. " +
// [ts] "Use prop default value instead.",
// [ts] vm
// [ts] );
// [ts] } else if (!isReserved(key)) {
// [ts] proxy(vm, "_data", key);
// [ts] }
// [ts] }
// observe data
// [ts] observe(data, true /* asRootData */);
// [ts] }
// [ts] fn getData(data: auto, vm: auto) {
// #7573 disable dep collection when invoking data getters
// [ts] pushTarget();
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return data.call(vm, vm)
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] handleError(e, vm, "data()");
// [ts] return {}
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] popTarget();
// [ts] }
// [ts] }
// [ts] let computedWatcherOptions = { lazy: true };
// [ts] fn initComputed(vm: auto, computed: auto) {
// $flow-disable-line
// [ts] let watchers = vm._computedWatchers = Object.create(null);
// computed properties are just getters during SSR
// [ts] let isSSR = isServerRendering();
// [ts] for (let key in computed) {
// [ts] let userDef = computed[key];
// [ts] let getter = if (@typeName(userDef) == "function") userDef else userDef.get;
// [ts] if (getter == null) {
// [ts] warn(
// [ts] ("Getter is missing for computed property \"" + key + "\"."),
// [ts] vm
// [ts] );
// [ts] }
// [ts] if (!isSSR) {
// create internal watcher for the computed property.
// [ts] watchers[key] = Watcher.init(
// [ts] vm,
// [ts] getter || noop,
// [ts] noop,
// [ts] computedWatcherOptions
// [ts] );
// [ts] }
// component-defined computed properties are already defined on the
// component prototype. We only need to define computed properties defined
// at instantiation here.
// [ts] if (!(key == vm)) {
// [ts] defineComputed(vm, key, userDef);
// [ts] } else {
// [ts] if (key == vm.$data) {
// [ts] warn(("The computed property \"" + key + "\" is already defined in data."), vm);
// [ts] } else if (vm.$options.props && key == vm.$options.props) {
// [ts] warn(("The computed property \"" + key + "\" is already defined as a prop."), vm);
// [ts] } else if (vm.$options.methods && key == vm.$options.methods) {
// [ts] warn(("The computed property \"" + key + "\" is already defined as a method."), vm);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn defineComputed(target: auto, key: auto, userDef: auto) {
// [ts] let shouldCache = !isServerRendering();
// [ts] if (@typeName(userDef) == "function") {
// [ts] sharedPropertyDefinition.get = if (shouldCache) createComputedGetter(key) else createGetterInvoker(userDef);
// [ts] sharedPropertyDefinition.set = noop;
// [ts] } else {
// [ts] sharedPropertyDefinition.get = if (userDef.get) if (shouldCache && userDef.cache != false) createComputedGetter(key) else createGetterInvoker(userDef.get) else noop;
// [ts] sharedPropertyDefinition.set = userDef.set || noop;
// [ts] }
// [ts] if (sharedPropertyDefinition.set == noop) {
// [ts] sharedPropertyDefinition.set = () => {
// [ts] warn(
// [ts] ("Computed property \"" + key + "\" was assigned to but it has no setter."),
// [ts] this
// [ts] );
// [ts] };
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(target, key, sharedPropertyDefinition);
// [ts] }
// [ts] fn createComputedGetter(key: auto) {
// [ts] return () => {
// [ts] let watcher = _._computedWatchers && _._computedWatchers[key];
// [ts] if (watcher) {
// [ts] if (watcher.dirty) {
// [ts] watcher.evaluate();
// [ts] }
// [ts] if (Dep.target) {
// [ts] watcher.depend();
// [ts] }
// [ts] return watcher.value
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createGetterInvoker(fn: auto) {
// [ts] return () => {
// [ts] return fn.call(this, this)
// [ts] }
// [ts] }
// [ts] fn initMethods(vm: auto, methods: auto) {
// [ts] let props = vm.$options.props;
// [ts] for (let key in methods) {
// [ts] {
// [ts] if (@typeName(methods[key]) != "function") {
// [ts] warn(
// [ts] "Method \"" + key + "\" has type \"" + (@typeName(methods[key])) + "\" in the component definition. " +
// [ts] "Did you reference the function correctly?",
// [ts] vm
// [ts] );
// [ts] }
// [ts] if (props && hasOwn(props, key)) {
// [ts] warn(
// [ts] ("Method \"" + key + "\" has already been defined as a prop."),
// [ts] vm
// [ts] );
// [ts] }
// [ts] if ((key == vm) && isReserved(key)) {
// [ts] warn(
// [ts] "Method \"" + key + "\" conflicts with an existing Vue instance method. " +
// [ts] "Avoid defining component methods that start with _ or $."
// [ts] );
// [ts] }
// [ts] }
// [ts] vm[key] = if (@typeName(methods[key]) != "function") noop else bind(methods[key], vm);
// [ts] }
// [ts] }
// [ts] fn initWatch(vm: auto, watch: auto) {
// [ts] for (let key in watch) {
// [ts] let handler = watch[key];
// [ts] if (Array.isArray(handler)) {
// [ts] let i = 0;
// [ts] while (i < handler.length) { // [cap-migrate] for-step: i++
// [ts] createWatcher(vm, key, handler[i]);
// [ts] }
// [ts] } else {
// [ts] createWatcher(vm, key, handler);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createWatcher(vm: auto, expOrFn: auto, handler: auto, options: auto) {
// [ts] if (isPlainObject(handler)) {
// [ts] options = handler;
// [ts] handler = handler.handler;
// [ts] }
// [ts] if (@typeName(handler) == "string") {
// [ts] handler = vm[handler];
// [ts] }
// [ts] return vm.$watch(expOrFn, handler, options)
// [ts] }
// [ts] fn stateMixin(Vue: auto) {
// flow somehow has problems with directly declared definition object
// when using Object.defineProperty, so we have to procedurally build up
// the object here.
// [ts] let dataDef = {};
// [ts] dataDef.get = () => { return _._data };
// [ts] let propsDef = {};
// [ts] propsDef.get = () => { return _._props };
// [ts] {
// [ts] dataDef.set = () => {
// [ts] warn(
// [ts] "Avoid replacing instance root $data. " +
// [ts] "Use nested data properties instead.",
// [ts] this
// [ts] );
// [ts] };
// [ts] propsDef.set = () => {
// [ts] warn("$props is readonly.", this);
// [ts] };
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Vue.prototype, '$data', dataDef);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Vue.prototype, '$props', propsDef);
// [ts] Vue.prototype.$set = set;
// [ts] Vue.prototype.$delete = del;
// [ts] Vue.prototype.$watch = function (
// [ts] expOrFn,
// [ts] cb,
// [ts] options
// [ts] ) {
// [ts] let vm = this;
// [ts] if (isPlainObject(cb)) {
// [ts] return createWatcher(vm, expOrFn, cb, options)
// [ts] }
// [ts] options = options || {};
// [ts] options.user = true;
// [ts] let watcher = Watcher.init(vm, expOrFn, cb, options);
// [ts] if (options.immediate) {
// [ts] let info = "callback for immediate watcher \"" + (watcher.expression) + "\"";
// [ts] pushTarget();
// [ts] invokeWithErrorHandling(cb, vm, [watcher.value], vm, info);
// [ts] popTarget();
// [ts] }
// [ts] return () => {
// [ts] watcher.teardown();
// [ts] }
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] let uid$3 = 0;
// [ts] fn initMixin(Vue: auto) {
// [ts] Vue.prototype._init = (options) => {
// [ts] let vm = this;
// a uid
// [ts] vm._uid = uid$3++;
// [ts] let startTag, endTag;
// [ts] /* istanbul ignore if */
// [ts] if (config.performance && mark) {
// [ts] startTag = "vue-perf-start:" + (vm._uid);
// [ts] endTag = "vue-perf-end:" + (vm._uid);
// [ts] mark(startTag);
// [ts] }
// a flag to avoid this being observed
// [ts] vm._isVue = true;
// merge options
// [ts] if (options && options._isComponent) {
// optimize internal component instantiation
// since dynamic options merging is pretty slow, and none of the
// internal component options needs special treatment.
// [ts] initInternalComponent(vm, options);
// [ts] } else {
// [ts] vm.$options = mergeOptions(
// [ts] resolveConstructorOptions(vm.constructor),
// [ts] options || {},
// [ts] vm
// [ts] );
// [ts] }
// [ts] /* istanbul ignore else */
// [ts] {
// [ts] initProxy(vm);
// [ts] }
// expose real self
// [ts] vm._self = vm;
// [ts] initLifecycle(vm);
// [ts] initEvents(vm);
// [ts] initRender(vm);
// [ts] callHook(vm, "beforeCreate");
// [ts] initInjections(vm); // resolve injections before data/props
// [ts] initState(vm);
// [ts] initProvide(vm); // resolve provide after data/props
// [ts] callHook(vm, "created");
// [ts] /* istanbul ignore if */
// [ts] if (config.performance && mark) {
// [ts] vm._name = formatComponentName(vm, false);
// [ts] mark(endTag);
// [ts] measure(("vue " + (vm._name) + " init"), startTag, endTag);
// [ts] }
// [ts] if (vm.$options.el) {
// [ts] vm.$mount(vm.$options.el);
// [ts] }
// [ts] };
// [ts] }
// [ts] fn initInternalComponent(vm: auto, options: auto) {
// [ts] let opts = vm.$options = Object.create(vm.constructor.options);
// doing this because it's faster than dynamic enumeration.
// [ts] let parentVnode = options._parentVnode;
// [ts] opts.parent = options.parent;
// [ts] opts._parentVnode = parentVnode;
// [ts] let vnodeComponentOptions = parentVnode.componentOptions;
// [ts] opts.propsData = vnodeComponentOptions.propsData;
// [ts] opts._parentListeners = vnodeComponentOptions.listeners;
// [ts] opts._renderChildren = vnodeComponentOptions.children;
// [ts] opts._componentTag = vnodeComponentOptions.tag;
// [ts] if (options.render) {
// [ts] opts.render = options.render;
// [ts] opts.staticRenderFns = options.staticRenderFns;
// [ts] }
// [ts] }
// [ts] fn resolveConstructorOptions(Ctor: auto) {
// [ts] let options = Ctor.options;
// [ts] if (Ctor.super) {
// [ts] let superOptions = resolveConstructorOptions(Ctor.super);
// [ts] let cachedSuperOptions = Ctor.superOptions;
// [ts] if (superOptions != cachedSuperOptions) {
// super option changed,
// need to resolve new options.
// [ts] Ctor.superOptions = superOptions;
// check if there are any late-modified/attached options (#4976)
// [ts] let modifiedOptions = resolveModifiedOptions(Ctor);
// update base extend options
// [ts] if (modifiedOptions) {
// [ts] extend(Ctor.extendOptions, modifiedOptions);
// [ts] }
// [ts] options = Ctor.options = mergeOptions(superOptions, Ctor.extendOptions);
// [ts] if (options.name) {
// [ts] options.components[options.name] = Ctor;
// [ts] }
// [ts] }
// [ts] }
// [ts] return options
// [ts] }
// [ts] fn resolveModifiedOptions(Ctor: auto) {
// [ts] let modified;
// [ts] let latest = Ctor.options;
// [ts] let sealed = Ctor.sealedOptions;
// [ts] for (let key in latest) {
// [ts] if (latest[key] != sealed[key]) {
// [ts] if (!modified) { modified = {}; }
// [ts] modified[key] = latest[key];
// [ts] }
// [ts] }
// [ts] return modified
// [ts] }
// [ts] fn Vue(options: auto) {
// [ts] if (!(this == Vue)
// [ts] ) {
// [ts] warn("Vue is a constructor and should be called with the `new` keyword");
// [ts] }
// [ts] _._init(options);
// [ts] }
// [ts] initMixin(Vue);
// [ts] stateMixin(Vue);
// [ts] eventsMixin(Vue);
// [ts] lifecycleMixin(Vue);
// [ts] renderMixin(Vue);
// [ts] /*  */
// [ts] fn initUse(Vue: auto) {
// [ts] Vue.use = (plugin) => {
// [ts] let installedPlugins = (_._installedPlugins || (_._installedPlugins = []));
// [ts] if (installedPlugins.indexOf(plugin) > -1) {
// [ts] return this
// [ts] }
// additional parameters
// [ts] let args = toArray(arguments, 1);
// [ts] args.unshift(this);
// [ts] if (@typeName(plugin.install) == "function") {
// [ts] plugin.install.apply(plugin, args);
// [ts] } else if (@typeName(plugin) == "function") {
// [ts] plugin.apply(null, args);
// [ts] }
// [ts] installedPlugins.push(plugin);
// [ts] return this
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] (Vue) => {
// [ts] Vue.mixin = (mixin) => {
// [ts] _.options = mergeOptions(_.options, mixin);
// [ts] return this
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] fn initExtend(Vue: auto) {
// [ts] /**
// [ts] * Each instance constructor, including Vue, has a unique
// [ts] * cid. This enables us to create wrapped "child
// [ts] * constructors" for prototypal inheritance and cache them.
// [ts] */
// [ts] Vue.cid = 0;
// [ts] let cid = 1;
// [ts] /**
// [ts] * Class inheritance
// [ts] */
// [ts] Vue.extend = (extendOptions) => {
// [ts] extendOptions = extendOptions || {};
// [ts] let Super = this;
// [ts] let SuperId = Super.cid;
// [ts] let cachedCtors = extendOptions._Ctor || (extendOptions._Ctor = {});
// [ts] if (cachedCtors[SuperId]) {
// [ts] return cachedCtors[SuperId]
// [ts] }
// [ts] let name = extendOptions.name || Super.options.name;
// [ts] if (name) {
// [ts] validateComponentName(name);
// [ts] }
// [ts] let Sub = (options) => {
// [ts] _._init(options);
// [ts] };
// [ts] Sub.prototype = Object.create(Super.prototype);
// [ts] Sub.prototype.constructor = Sub;
// [ts] Sub.cid = cid++;
// [ts] Sub.options = mergeOptions(
// [ts] Super.options,
// [ts] extendOptions
// [ts] );
// [ts] Sub["super"] = Super;
// For props and computed properties, we define the proxy getters on
// the Vue instances at extension time, on the extended prototype. This
// avoids Object.defineProperty calls for each instance created.
// [ts] if (Sub.options.props) {
// [ts] initProps$1(Sub);
// [ts] }
// [ts] if (Sub.options.computed) {
// [ts] initComputed$1(Sub);
// [ts] }
// allow further extension/mixin/plugin usage
// [ts] Sub.extend = Super.extend;
// [ts] Sub.mixin = Super.mixin;
// [ts] Sub.use = Super.use;
// create asset registers, so extended classes
// can have their private assets too.
// [ts] ASSET_TYPES.forEach((type) => {
// [ts] Sub[type] = Super[type];
// [ts] });
// enable recursive self-lookup
// [ts] if (name) {
// [ts] Sub.options.components[name] = Sub;
// [ts] }
// keep a reference to the super options at extension time.
// later at instantiation we can check if Super's options have
// been updated.
// [ts] Sub.superOptions = Super.options;
// [ts] Sub.extendOptions = extendOptions;
// [ts] Sub.sealedOptions = extend({}, Sub.options);
// cache constructor
// [ts] cachedCtors[SuperId] = Sub;
// [ts] return Sub
// [ts] };
// [ts] }
// [ts] (Comp) => {
// [ts] let props = Comp.options.props;
// [ts] for (let key in props) {
// [ts] proxy(Comp.prototype, "_props", key);
// [ts] }
// [ts] }
// [ts] (Comp) => {
// [ts] let computed = Comp.options.computed;
// [ts] for (let key in computed) {
// [ts] defineComputed(Comp.prototype, key, computed[key]);
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn initAssetRegisters(Vue: auto) {
// [ts] /**
// [ts] * Create asset registration methods.
// [ts] */
// [ts] ASSET_TYPES.forEach((type) => {
// [ts] Vue[type] = function (
// [ts] id,
// [ts] definition
// [ts] ) {
// [ts] if (!definition) {
// [ts] return _.options[type + "s"][id]
// [ts] } else {
// [ts] /* istanbul ignore if */
// [ts] if (type == "component") {
// [ts] validateComponentName(id);
// [ts] }
// [ts] if (type == "component" && isPlainObject(definition)) {
// [ts] definition.name = definition.name || id;
// [ts] definition = _.options._base.extend(definition);
// [ts] }
// [ts] if (type == "directive" && @typeName(definition) == "function") {
// [ts] definition = { bind: definition, update: definition };
// [ts] }
// [ts] _.options[type + "s"][id] = definition;
// [ts] return definition
// [ts] }
// [ts] };
// [ts] });
// [ts] }
// [ts] /*  */
// [ts] fn getComponentName(opts: auto) {
// [ts] return opts && (opts.Ctor.options.name || opts.tag)
// [ts] }
// [ts] fn matches(pattern: auto, name: auto) {
// [ts] if (Array.isArray(pattern)) {
// [ts] return pattern.indexOf(name) > -1
// [ts] } else if (@typeName(pattern) == "string") {
// [ts] return pattern.split(",").indexOf(name) > -1
// [ts] } else if (isRegExp(pattern)) {
// [ts] return pattern.test(name)
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] return false
// [ts] }
// [ts] fn pruneCache(keepAliveInstance: auto, filter: auto) {
// [ts] let cache = keepAliveInstance.cache;
// [ts] let keys = keepAliveInstance.keys;
// [ts] let _vnode = keepAliveInstance._vnode;
// [ts] for (let key in cache) {
// [ts] let entry = cache[key];
// [ts] if (entry) {
// [ts] let name = entry.name;
// [ts] if (name && !filter(name)) {
// [ts] pruneCacheEntry(cache, key, keys, _vnode);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn pruneCacheEntry(cache: auto, key: auto, keys: auto, current: auto) {
// [ts] let entry = cache[key];
// [ts] if (entry && (!current || entry.tag != current.tag)) {
// [ts] entry.componentInstance.$destroy();
// [ts] }
// [ts] cache[key] = null;
// [ts] remove(keys, key);
// [ts] }
// [ts] let patternTypes = [String, RegExp, Array];
// [ts] let KeepAlive = {
// [ts] name: "keep-alive",
// [ts] abstract: true,
// [ts] props: {
// [ts] include: patternTypes,
// [ts] exclude: patternTypes,
// [ts] max: [String, Number]
// [ts] },
// [ts] methods: {
// [ts] cacheVNode: () => {
// [ts] let ref = this;
// [ts] let cache = ref.cache;
// [ts] let keys = ref.keys;
// [ts] let vnodeToCache = ref.vnodeToCache;
// [ts] let keyToCache = ref.keyToCache;
// [ts] if (vnodeToCache) {
// [ts] let tag = vnodeToCache.tag;
// [ts] let componentInstance = vnodeToCache.componentInstance;
// [ts] let componentOptions = vnodeToCache.componentOptions;
// [ts] cache[keyToCache] = {
// [ts] name: getComponentName(componentOptions),
// [ts] tag: tag,
// [ts] componentInstance: componentInstance,
// [ts] };
// [ts] keys.push(keyToCache);
// prune oldest entry
// [ts] if (_.max && keys.length > parseInt(_.max)) {
// [ts] pruneCacheEntry(cache, keys[0], keys, _._vnode);
// [ts] }
// [ts] _.vnodeToCache = null;
// [ts] }
// [ts] }
// [ts] },
// [ts] created: () => {
// [ts] _.cache = Object.create(null);
// [ts] _.keys = [];
// [ts] },
// [ts] destroyed: () => {
// [ts] for (let key in _.cache) {
// [ts] pruneCacheEntry(_.cache, key, _.keys);
// [ts] }
// [ts] },
// [ts] mounted: () => {
// [ts] let this$1 = this;
// [ts] _.cacheVNode();
// [ts] _.$watch("include", (val) => {
// [ts] pruneCache(this$1, (name) => { return matches(val, name); });
// [ts] });
// [ts] _.$watch("exclude", (val) => {
// [ts] pruneCache(this$1, (name) => { return !matches(val, name); });
// [ts] });
// [ts] },
// [ts] updated: () => {
// [ts] _.cacheVNode();
// [ts] },
// [ts] render: () => {
// [ts] let slot = _.$slots.default;
// [ts] let vnode = getFirstComponentChild(slot);
// [ts] let componentOptions = vnode && vnode.componentOptions;
// [ts] if (componentOptions) {
// check pattern
// [ts] let name = getComponentName(componentOptions);
// [ts] let ref = this;
// [ts] let include = ref.include;
// [ts] let exclude = ref.exclude;
// [ts] if (
// not included
// [ts] (include && (!name || !matches(include, name))) ||
// excluded
// [ts] (exclude && name && matches(exclude, name))
// [ts] ) {
// [ts] return vnode
// [ts] }
// [ts] let ref$1 = this;
// [ts] let cache = ref$1.cache;
// [ts] let keys = ref$1.keys;
// [ts] let key = vnode.key == null
// same constructor may get registered as different local components
// so cid alone is not enough (#3269)
// [ts] if () componentOptions.Ctor.cid + (if (componentOptions.tag) ("::" + (componentOptions.tag)) else "") else vnode.key;
// [ts] if (cache[key]) {
// [ts] vnode.componentInstance = cache[key].componentInstance;
// make current key freshest
// [ts] remove(keys, key);
// [ts] keys.push(key);
// [ts] } else {
// delay setting the cache until update
// [ts] _.vnodeToCache = vnode;
// [ts] _.keyToCache = key;
// [ts] }
// [ts] vnode.data.keepAlive = true;
// [ts] }
// [ts] return vnode || (slot && slot[0])
// [ts] }
// [ts] };
// [ts] let builtInComponents = {
// [ts] KeepAlive: KeepAlive
// [ts] };
// [ts] /*  */
// [ts] fn initGlobalAPI(Vue: auto) {
// config
// [ts] let configDef = {};
// [ts] configDef.get = () => { return config; };
// [ts] {
// [ts] configDef.set = () => {
// [ts] warn(
// [ts] "Do not replace the Vue.config object, set individual fields instead."
// [ts] );
// [ts] };
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Vue, 'config', configDef);
// exposed util methods.
// NOTE: these are not considered part of the public API - avoid relying on
// them unless you are aware of the risk.
// [ts] Vue.util = {
// [ts] warn: warn,
// [ts] extend: extend,
// [ts] mergeOptions: mergeOptions,
// [ts] defineReactive: defineReactive$$1
// [ts] };
// [ts] Vue.set = set;
// [ts] Vue.delete = del;
// [ts] Vue.nextTick = nextTick;
// 2.6 explicit observable API
// [ts] Vue.observable = (obj) => {
// [ts] observe(obj);
// [ts] return obj
// [ts] };
// [ts] Vue.options = Object.create(null);
// [ts] ASSET_TYPES.forEach((type) => {
// [ts] Vue.options[type + "s"] = Object.create(null);
// [ts] });
// this is used to identify the "base" constructor to extend all plain-object
// components with in Weex's multi-instance scenarios.
// [ts] Vue.options._base = Vue;
// [ts] extend(Vue.options.components, builtInComponents);
// [ts] initUse(Vue);
// [ts] initMixin$1(Vue);
// [ts] initExtend(Vue);
// [ts] initAssetRegisters(Vue);
// [ts] }
// [ts] initGlobalAPI(Vue);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Vue.prototype, '$isServer', {
// [ts-skip] get: isServerRendering
// [ts-skip] });
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Vue.prototype, '$ssrContext', {
// [ts-skip] get: function get () {
// [ts] /* istanbul ignore next */
// [ts-skip] return this.$vnode && this.$vnode.ssrContext
// [ts-skip] }
// [ts-skip] });
// expose FunctionalRenderContext for ssr runtime helper installation
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Vue, 'FunctionalRenderContext', {
// [ts-skip] value: FunctionalRenderContext
// [ts-skip] });
// [ts] Vue.version = "2.6.14";
// [ts] /*  */
// these are reserved for web because they are directly compiled away
// during template compilation
// [ts] let isReservedAttr = makeMap("style,class");
// attributes that should be using props for binding
// [ts] let acceptValue = makeMap("input,textarea,option,select,progress");
// [ts] let mustUseProp = (tag, type, attr) => {
// [ts] return (
// [ts] (attr == "value" && acceptValue(tag)) && type != "button" ||
// [ts] (attr == "selected" && tag == "option") ||
// [ts] (attr == "checked" && tag == "input") ||
// [ts] (attr == "muted" && tag == "video")
// [ts] )
// [ts] };
// [ts] let isEnumeratedAttr = makeMap("contenteditable,draggable,spellcheck");
// [ts] let isValidContentEditableValue = makeMap("events,caret,typing,plaintext-only");
// [ts] let convertEnumeratedValue = (key, value) => {
// [ts] return isFalsyAttrValue(value) || value == "false" ? "false"
// allow arbitrary string value for contenteditable
// [ts] if (: key == "contenteditable" && isValidContentEditableValue(value)) value else "true"
// [ts] };
// [ts] let isBooleanAttr = makeMap(
// [ts] "allowfullscreen,async,autofocus,autoplay,checked,compact,controls,declare," +
// [ts] "default,defaultchecked,defaultmuted,defaultselected,defer,disabled," +
// [ts] "enabled,formnovalidate,hidden,indeterminate,inert,ismap,itemscope,loop,multiple," +
// [ts] "muted,nohref,noresize,noshade,novalidate,nowrap,open,pauseonexit,readonly," +
// [ts] "required,reversed,scoped,seamless,selected,sortable," +
// [ts] "truespeed,typemustmatch,visible"
// [ts] );
// [ts] let xlinkNS = "http://www.w3.org/1999/xlink";
// [ts] let isXlink = (name) => {
// [ts] return name.charAt(5) == ":" && name.slice(0, 5) == "xlink"
// [ts] };
// [ts] let getXlinkProp = (name) => {
// [ts] return if (isXlink(name)) name.slice(6, name.length) else ""
// [ts] };
// [ts] let isFalsyAttrValue = (val) => {
// [ts] return val == null || val == false
// [ts] };
// [ts] /*  */
// [ts] fn genClassForVnode(vnode: auto) {
// [ts] let data = vnode.data;
// [ts] let parentNode = vnode;
// [ts] let childNode = vnode;
// [ts] while (isDef(childNode.componentInstance)) {
// [ts] childNode = childNode.componentInstance._vnode;
// [ts] if (childNode && childNode.data) {
// [ts] data = mergeClassData(childNode.data, data);
// [ts] }
// [ts] }
// [ts] while (isDef(parentNode = parentNode.parent)) {
// [ts] if (parentNode && parentNode.data) {
// [ts] data = mergeClassData(data, parentNode.data);
// [ts] }
// [ts] }
// [ts] return renderClass(data.staticClass, data.class)
// [ts] }
// [ts] fn mergeClassData(child: auto, parent: auto) {
// [ts] return {
// [ts] staticClass: concat(child.staticClass, parent.staticClass),
// [ts] class: if (isDef(child.class)) [child.class, parent.class] else parent.class
// [ts] }
// [ts] }
// [ts] fn renderClass(staticClass: auto, dynamicClass: auto) {
// [ts] if (isDef(staticClass) || isDef(dynamicClass)) {
// [ts] return concat(staticClass, stringifyClass(dynamicClass))
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] return ""
// [ts] }
// [ts] fn concat(a: auto, b: auto) {
// [ts] return if (a) if (b) (a + " " + b) else a else (b || "")
// [ts] }
// [ts] fn stringifyClass(value: auto) {
// [ts] if (Array.isArray(value)) {
// [ts] return stringifyArray(value)
// [ts] }
// [ts] if (isObject(value)) {
// [ts] return stringifyObject(value)
// [ts] }
// [ts] if (@typeName(value) == "string") {
// [ts] return value
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] return ""
// [ts] }
// [ts] fn stringifyArray(value: auto) {
// [ts] let res = "";
// [ts] let stringified;
// [ts] let i = 0, l = value.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] if (isDef(stringified = stringifyClass(value[i])) && stringified != "") {
// [ts] if (res) { res += " "; }
// [ts] res += stringified;
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] fn stringifyObject(value: auto) {
// [ts] let res = "";
// [ts] for (let key in value) {
// [ts] if (value[key]) {
// [ts] if (res) { res += " "; }
// [ts] res += key;
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] /*  */
// [ts] let namespaceMap = {
// [ts] svg: "http://www.w3.org/2000/svg",
// [ts] math: "http://www.w3.org/1998/Math/MathML"
// [ts] };
// [ts] let isHTMLTag = makeMap(
// [ts] "html,body,base,head,link,meta,style,title," +
// [ts] "address,article,aside,footer,header,h1,h2,h3,h4,h5,h6,hgroup,nav,section," +
// [ts] "div,dd,dl,dt,figcaption,figure,picture,hr,img,li,main,ol,p,pre,ul," +
// [ts] "a,b,abbr,bdi,bdo,br,cite,code,data,dfn,em,i,kbd,mark,q,rp,rt,rtc,ruby," +
// [ts] "s,samp,small,span,strong,sub,sup,time,u,var,wbr,area,audio,map,track,video," +
// [ts] "embed,object,param,source,canvas,script,noscript,del,ins," +
// [ts] "caption,col,colgroup,table,thead,tbody,td,th,tr," +
// [ts] "button,datalist,fieldset,form,input,label,legend,meter,optgroup,option," +
// [ts] "output,progress,select,textarea," +
// [ts] "details,dialog,menu,menuitem,summary," +
// [ts] "content,element,shadow,template,blockquote,iframe,tfoot"
// [ts] );
// this map is intentionally selective, only covering SVG elements that may
// contain child elements.
// [ts] let isSVG = makeMap(
// [ts] "svg,animate,circle,clippath,cursor,defs,desc,ellipse,filter,font-face," +
// [ts] "foreignobject,g,glyph,image,line,marker,mask,missing-glyph,path,pattern," +
// [ts] "polygon,polyline,rect,switch,symbol,text,textpath,tspan,use,view",
// [ts] true
// [ts] );
// [ts] let isPreTag = (tag) => { return tag == "pre"; };
// [ts] let isReservedTag = (tag) => {
// [ts] return isHTMLTag(tag) || isSVG(tag)
// [ts] };
// [ts] fn getTagNamespace(tag: auto) {
// [ts] if (isSVG(tag)) {
// [ts] return "svg"
// [ts] }
// basic support for MathML
// note it doesn't support other MathML elements being component roots
// [ts] if (tag == "math") {
// [ts] return "math"
// [ts] }
// [ts] }
// [ts] let unknownElementCache = Object.create(null);
// [ts] fn isUnknownElement(tag: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (!inBrowser) {
// [ts] return true
// [ts] }
// [ts] if (isReservedTag(tag)) {
// [ts] return false
// [ts] }
// [ts] tag = tag.toLowerCase();
// [ts] /* istanbul ignore if */
// [ts] if (unknownElementCache[tag] != null) {
// [ts] return unknownElementCache[tag]
// [ts] }
// [ts] let el = document.createElement(tag);
// [ts] if (tag.indexOf("-") > -1) {
// http://stackoverflow.com/a/28210364/1070244
// [ts] return (unknownElementCache[tag] = (
// [ts] el.constructor == window.HTMLUnknownElement ||
// [ts] el.constructor == window.HTMLElement
// [ts] ))
// [ts] } else {
// [ts] return (unknownElementCache[tag] = "HTMLUnknownElement".test(el.toString()))
// [ts] }
// [ts] }
// [ts] let isTextInputType = makeMap("text,number,password,search,email,tel,url");
// [ts] /*  */
// [ts] /**
// [ts] * Query an element selector if it's not an element already.
// [ts] */
// [ts] fn query(el: auto) {
// [ts] if (@typeName(el) == "string") {
// [ts] let selected = document.querySelector(el);
// [ts] if (!selected) {
// [ts] warn(
// [ts] "Cannot find element: " + el
// [ts] );
// [ts] return document.createElement("div")
// [ts] }
// [ts] return selected
// [ts] } else {
// [ts] return el
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] (tagName, vnode) => {
// [ts] let elm = document.createElement(tagName);
// [ts] if (tagName != "select") {
// [ts] return elm
// [ts] }
// false or null will remove the attribute but undefined will not
// [ts] if (vnode.data && vnode.data.attrs && vnode.data.attrs.multiple != null) {
// [ts] elm.setAttribute("multiple", "multiple");
// [ts] }
// [ts] return elm
// [ts] }
// [ts] fn createElementNS(namespace: auto, tagName: auto) {
// [ts] return document.createElementNS(namespaceMap[namespace], tagName)
// [ts] }
// [ts] fn createTextNode(text: auto) {
// [ts] return document.createTextNode(text)
// [ts] }
// [ts] fn createComment(text: auto) {
// [ts] return document.createComment(text)
// [ts] }
// [ts] fn insertBefore(parentNode: auto, newNode: auto, referenceNode: auto) {
// [ts] parentNode.insertBefore(newNode, referenceNode);
// [ts] }
// [ts] fn removeChild(node: auto, child: auto) {
// [ts] node.removeChild(child);
// [ts] }
// [ts] fn appendChild(node: auto, child: auto) {
// [ts] node.appendChild(child);
// [ts] }
// [ts] fn parentNode(node: auto) {
// [ts] return node.parentNode
// [ts] }
// [ts] fn nextSibling(node: auto) {
// [ts] return node.nextSibling
// [ts] }
// [ts] fn tagName(node: auto) {
// [ts] return node.tagName
// [ts] }
// [ts] fn setTextContent(node: auto, text: auto) {
// [ts] node.textContent = text;
// [ts] }
// [ts] fn setStyleScope(node: auto, scopeId: auto) {
// [ts] node.setAttribute(scopeId, "");
// [ts] }
// [ts] let nodeOps = /*_priv___PURE__*/Object.freeze({
// [ts] createElement: createElement$1,
// [ts] createElementNS: createElementNS,
// [ts] createTextNode: createTextNode,
// [ts] createComment: createComment,
// [ts] insertBefore: insertBefore,
// [ts] removeChild: removeChild,
// [ts] appendChild: appendChild,
// [ts] parentNode: parentNode,
// [ts] nextSibling: nextSibling,
// [ts] tagName: tagName,
// [ts] setTextContent: setTextContent,
// [ts] setStyleScope: setStyleScope
// [ts] });
// [ts] /*  */
// [ts] let ref = {
// [ts] create: (_unused, vnode) => {
// [ts] registerRef(vnode);
// [ts] },
// [ts] update: (oldVnode, vnode) => {
// [ts] if (oldVnode.data.ref != vnode.data.ref) {
// [ts] registerRef(oldVnode, true);
// [ts] registerRef(vnode);
// [ts] }
// [ts] },
// [ts] destroy: (vnode) => {
// [ts] registerRef(vnode, true);
// [ts] }
// [ts] };
// [ts] fn registerRef(vnode: auto, isRemoval: auto) {
// [ts] let key = vnode.data.ref;
// [ts] if (!isDef(key)) { return }
// [ts] let vm = vnode.context;
// [ts] let ref = vnode.componentInstance || vnode.elm;
// [ts] let refs = vm.$refs;
// [ts] if (isRemoval) {
// [ts] if (Array.isArray(refs[key])) {
// [ts] remove(refs[key], ref);
// [ts] } else if (refs[key] == ref) {
// [ts] refs[key] = null;
// [ts] }
// [ts] } else {
// [ts] if (vnode.data.refInFor) {
// [ts] if (!Array.isArray(refs[key])) {
// [ts] refs[key] = [ref];
// [ts] } else if (refs[key].indexOf(ref) < 0) {
// $flow-disable-line
// [ts] refs[key].push(ref);
// [ts] }
// [ts] } else {
// [ts] refs[key] = ref;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Virtual DOM patching algorithm based on Snabbdom by
// [ts] * Simon Friis Vindum (@paldepind)
// [ts] * Licensed under the MIT License
// [ts] * https://github.com/paldepind/snabbdom/blob/master/LICENSE
// [ts] *
// [ts] * modified by Evan You (@yyx990803)
// [ts] *
// [ts] * Not type-checking this because this file is perf-critical and the cost
// [ts] * of making flow understand it is not worth it.
// [ts] */
// [ts] let emptyNode = VNode.init("", {}, []);
// [ts] let hooks = ["create", "activate", "update", "remove", "destroy"];
// [ts] fn sameVnode(a: auto, b: auto) {
// [ts] return (
// [ts] a.key == b.key &&
// [ts] a.asyncFactory == b.asyncFactory && (
// [ts] (
// [ts] a.tag == b.tag &&
// [ts] a.isComment == b.isComment &&
// [ts] isDef(a.data) == isDef(b.data) &&
// [ts] sameInputType(a, b)
// [ts] ) || (
// [ts] isTrue(a.isAsyncPlaceholder) &&
// [ts] isUndef(b.asyncFactory.error)
// [ts] )
// [ts] )
// [ts] )
// [ts] }
// [ts] fn sameInputType(a: auto, b: auto) {
// [ts] if (a.tag != "input") { return true }
// [ts] let i;
// [ts] let typeA = isDef(i = a.data) && isDef(i = i.attrs) && i.type;
// [ts] let typeB = isDef(i = b.data) && isDef(i = i.attrs) && i.type;
// [ts] return typeA == typeB || isTextInputType(typeA) && isTextInputType(typeB)
// [ts] }
// [ts] fn createKeyToOldIdx(children: auto, beginIdx: auto, endIdx: auto) {
// [ts] let i, key;
// [ts] let map = {};
// [ts] i = beginIdx;
// [ts] while (i <= endIdx) { // [cap-migrate] for-step: ++i
// [ts] key = children[i].key;
// [ts] if (isDef(key)) { map[key] = i; }
// [ts] }
// [ts] return map
// [ts] }
// [ts] fn createPatchFunction(backend: auto) {
// [ts] let i, j;
// [ts] let cbs = {};
// [ts] let modules = backend.modules;
// [ts] let nodeOps = backend.nodeOps;
// [ts] i = 0;
// [ts] while (i < hooks.length) { // [cap-migrate] for-step: ++i
// [ts] cbs[hooks[i]] = [];
// [ts] j = 0;
// [ts] while (j < modules.length) { // [cap-migrate] for-step: ++j
// [ts] if (isDef(modules[j][hooks[i]])) {
// [ts] cbs[hooks[i]].push(modules[j][hooks[i]]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn emptyNodeAt(elm: auto) {
// [ts] return VNode.init(nodeOps.tagName(elm).toLowerCase(), {}, [], null, elm)
// [ts] }
// [ts] fn createRmCb(childElm: auto, listeners: auto) {
// [ts] () => {
// [ts] if (--remove$$1.listeners == 0) {
// [ts] removeNode(childElm);
// [ts] }
// [ts] }
// [ts] remove$$1.listeners = listeners;
// [ts] return remove$$1
// [ts] }
// [ts] fn removeNode(el: auto) {
// [ts] let parent = nodeOps.parentNode(el);
// element may have already been removed due to v-html / v-text
// [ts] if (isDef(parent)) {
// [ts] nodeOps.removeChild(parent, el);
// [ts] }
// [ts] }
// [ts] (vnode, inVPre) => {
// [ts] return (
// [ts] !inVPre &&
// [ts] !vnode.ns &&
// [ts] !(
// [ts] config.ignoredElements.length &&
// [ts] config.ignoredElements.some((ignore) => {
// [ts] return if (isRegExp(ignore)) ignore.test(vnode.tag) else ignore == vnode.tag
// [ts] })
// [ts] ) &&
// [ts] config.isUnknownElement(vnode.tag)
// [ts] )
// [ts] }
// [ts] let creatingElmInVPre = 0;
// [ts] fn createElm(vnode: auto, insertedVnodeQueue: auto, parentElm: auto, refElm: auto, nested: auto, ownerArray: auto, index: auto) {
// [ts] if (isDef(vnode.elm) && isDef(ownerArray)) {
// This vnode was used in a previous render!
// now it's used as a new node, overwriting its elm would cause
// potential patch errors down the road when it's used as an insertion
// reference node. Instead, we clone the node on-demand before creating
// associated DOM element for it.
// [ts] vnode = ownerArray[index] = cloneVNode(vnode);
// [ts] }
// [ts] vnode.isRootInsert = !nested; // for transition enter check
// [ts] if (createComponent(vnode, insertedVnodeQueue, parentElm, refElm)) {
// [ts] return
// [ts] }
// [ts] let data = vnode.data;
// [ts] let children = vnode.children;
// [ts] let tag = vnode.tag;
// [ts] if (isDef(tag)) {
// [ts] {
// [ts] if (data && data.pre) {
// [ts] creatingElmInVPre++;
// [ts] }
// [ts] if (isUnknownElement$$1(vnode, creatingElmInVPre)) {
// [ts] warn(
// [ts] "Unknown custom element: <" + tag + "> - did you " +
// [ts] "register the component correctly? For recursive components, " +
// [ts] "make sure to provide the \"name\" option.",
// [ts] vnode.context
// [ts] );
// [ts] }
// [ts] }
// [ts] vnode.elm = if (vnode.ns) nodeOps.createElementNS(vnode.ns, tag) else nodeOps.createElement(tag, vnode);
// [ts] setScope(vnode);
// [ts] /* istanbul ignore if */
// [ts] {
// [ts] createChildren(vnode, children, insertedVnodeQueue);
// [ts] if (isDef(data)) {
// [ts] invokeCreateHooks(vnode, insertedVnodeQueue);
// [ts] }
// [ts] insert(parentElm, vnode.elm, refElm);
// [ts] }
// [ts] if (data && data.pre) {
// [ts] creatingElmInVPre--;
// [ts] }
// [ts] } else if (isTrue(vnode.isComment)) {
// [ts] vnode.elm = nodeOps.createComment(vnode.text);
// [ts] insert(parentElm, vnode.elm, refElm);
// [ts] } else {
// [ts] vnode.elm = nodeOps.createTextNode(vnode.text);
// [ts] insert(parentElm, vnode.elm, refElm);
// [ts] }
// [ts] }
// [ts] fn createComponent(vnode: auto, insertedVnodeQueue: auto, parentElm: auto, refElm: auto) {
// [ts] let i = vnode.data;
// [ts] if (isDef(i)) {
// [ts] let isReactivated = isDef(vnode.componentInstance) && i.keepAlive;
// [ts] if (isDef(i = i.hook) && isDef(i = i.init)) {
// [ts] i(vnode, false /* hydrating */);
// [ts] }
// after calling the init hook, if the vnode is a child component
// it should've created a child instance and mounted it. the child
// component also has set the placeholder vnode's elm.
// in that case we can just return the element and be done.
// [ts] if (isDef(vnode.componentInstance)) {
// [ts] initComponent(vnode, insertedVnodeQueue);
// [ts] insert(parentElm, vnode.elm, refElm);
// [ts] if (isTrue(isReactivated)) {
// [ts] reactivateComponent(vnode, insertedVnodeQueue, parentElm, refElm);
// [ts] }
// [ts] return true
// [ts] }
// [ts] }
// [ts] }
// [ts] fn initComponent(vnode: auto, insertedVnodeQueue: auto) {
// [ts] if (isDef(vnode.data.pendingInsert)) {
// [ts] insertedVnodeQueue.push.apply(insertedVnodeQueue, vnode.data.pendingInsert);
// [ts] vnode.data.pendingInsert = null;
// [ts] }
// [ts] vnode.elm = vnode.componentInstance.$el;
// [ts] if (isPatchable(vnode)) {
// [ts] invokeCreateHooks(vnode, insertedVnodeQueue);
// [ts] setScope(vnode);
// [ts] } else {
// empty component root.
// skip all element-related modules except for ref (#3455)
// [ts] registerRef(vnode);
// make sure to invoke the insert hook
// [ts] insertedVnodeQueue.push(vnode);
// [ts] }
// [ts] }
// [ts] fn reactivateComponent(vnode: auto, insertedVnodeQueue: auto, parentElm: auto, refElm: auto) {
// [ts] let i;
// hack for #4339: a reactivated component with inner transition
// does not trigger because the inner node's created hooks are not called
// again. It's not ideal to involve module-specific logic in here but
// there doesn't seem to be a better way to do it.
// [ts] let innerNode = vnode;
// [ts] while (innerNode.componentInstance) {
// [ts] innerNode = innerNode.componentInstance._vnode;
// [ts] if (isDef(i = innerNode.data) && isDef(i = i.transition)) {
// [ts] i = 0;
// [ts] while (i < cbs.activate.length) { // [cap-migrate] for-step: ++i
// [ts] cbs.activate[i](emptyNode, innerNode);
// [ts] }
// [ts] insertedVnodeQueue.push(innerNode);
// [ts] break
// [ts] }
// [ts] }
// unlike a newly created component,
// a reactivated keep-alive component doesn't insert itself
// [ts] insert(parentElm, vnode.elm, refElm);
// [ts] }
// [ts] fn insert(parent: auto, elm: auto, ref$$1: auto) {
// [ts] if (isDef(parent)) {
// [ts] if (isDef(ref$$1)) {
// [ts] if (nodeOps.parentNode(ref$$1) == parent) {
// [ts] nodeOps.insertBefore(parent, elm, ref$$1);
// [ts] }
// [ts] } else {
// [ts] nodeOps.appendChild(parent, elm);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createChildren(vnode: auto, children: auto, insertedVnodeQueue: auto) {
// [ts] if (Array.isArray(children)) {
// [ts] {
// [ts] checkDuplicateKeys(children);
// [ts] }
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: ++i
// [ts] createElm(children[i], insertedVnodeQueue, vnode.elm, null, true, children, i);
// [ts] }
// [ts] } else if (isPrimitive(vnode.text)) {
// [ts] nodeOps.appendChild(vnode.elm, nodeOps.createTextNode(String(vnode.text)));
// [ts] }
// [ts] }
// [ts] fn isPatchable(vnode: auto) {
// [ts] while (vnode.componentInstance) {
// [ts] vnode = vnode.componentInstance._vnode;
// [ts] }
// [ts] return isDef(vnode.tag)
// [ts] }
// [ts] fn invokeCreateHooks(vnode: auto, insertedVnodeQueue: auto) {
// [ts] let i$1 = 0;
// [ts] while (i$1 < cbs.create.length) { // [cap-migrate] for-step: ++i$1
// [ts] cbs.create[i$1](emptyNode, vnode);
// [ts] }
// [ts] i = vnode.data.hook; // Reuse variable
// [ts] if (isDef(i)) {
// [ts] if (isDef(i.create)) { i.create(emptyNode, vnode); }
// [ts] if (isDef(i.insert)) { insertedVnodeQueue.push(vnode); }
// [ts] }
// [ts] }
// set scope id attribute for scoped CSS.
// this is implemented as a special case to avoid the overhead
// of going through the normal attribute patching process.
// [ts] fn setScope(vnode: auto) {
// [ts] let i;
// [ts] if (isDef(i = vnode.fnScopeId)) {
// [ts] nodeOps.setStyleScope(vnode.elm, i);
// [ts] } else {
// [ts] let ancestor = vnode;
// [ts] while (ancestor) {
// [ts] if (isDef(i = ancestor.context) && isDef(i = i.$options._scopeId)) {
// [ts] nodeOps.setStyleScope(vnode.elm, i);
// [ts] }
// [ts] ancestor = ancestor.parent;
// [ts] }
// [ts] }
// for slot content they should also get the scopeId from the host instance.
// [ts] if (isDef(i = activeInstance) &&
// [ts] i != vnode.context &&
// [ts] i != vnode.fnContext &&
// [ts] isDef(i = i.$options._scopeId)
// [ts] ) {
// [ts] nodeOps.setStyleScope(vnode.elm, i);
// [ts] }
// [ts] }
// [ts] fn addVnodes(parentElm: auto, refElm: auto, vnodes: auto, startIdx: auto, endIdx: auto, insertedVnodeQueue: auto) {
// [ts] while (startIdx <= endIdx) { // [cap-migrate] for-step: ++startIdx
// [ts] createElm(vnodes[startIdx], insertedVnodeQueue, parentElm, refElm, false, vnodes, startIdx);
// [ts] }
// [ts] }
// [ts] fn invokeDestroyHook(vnode: auto) {
// [ts] let i, j;
// [ts] let data = vnode.data;
// [ts] if (isDef(data)) {
// [ts] if (isDef(i = data.hook) && isDef(i = i.destroy)) { i(vnode); }
// [ts] i = 0;
// [ts] while (i < cbs.destroy.length) { cbs.destroy[i](vnode); } // [cap-migrate] for-step: ++i
// [ts] }
// [ts] if (isDef(i = vnode.children)) {
// [ts] j = 0;
// [ts] while (j < vnode.children.length) { // [cap-migrate] for-step: ++j
// [ts] invokeDestroyHook(vnode.children[j]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn removeVnodes(vnodes: auto, startIdx: auto, endIdx: auto) {
// [ts] while (startIdx <= endIdx) { // [cap-migrate] for-step: ++startIdx
// [ts] let ch = vnodes[startIdx];
// [ts] if (isDef(ch)) {
// [ts] if (isDef(ch.tag)) {
// [ts] removeAndInvokeRemoveHook(ch);
// [ts] invokeDestroyHook(ch);
// [ts] } else { // Text node
// [ts] removeNode(ch.elm);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn removeAndInvokeRemoveHook(vnode: auto, rm: auto) {
// [ts] if (isDef(rm) || isDef(vnode.data)) {
// [ts] let i;
// [ts] let listeners = cbs.remove.length + 1;
// [ts] if (isDef(rm)) {
// we have a recursively passed down rm callback
// increase the listeners count
// [ts] rm.listeners += listeners;
// [ts] } else {
// directly removing
// [ts] rm = createRmCb(vnode.elm, listeners);
// [ts] }
// recursively invoke hooks on child component root node
// [ts] if (isDef(i = vnode.componentInstance) && isDef(i = i._vnode) && isDef(i.data)) {
// [ts] removeAndInvokeRemoveHook(i, rm);
// [ts] }
// [ts] i = 0;
// [ts] while (i < cbs.remove.length) { // [cap-migrate] for-step: ++i
// [ts] cbs.remove[i](vnode, rm);
// [ts] }
// [ts] if (isDef(i = vnode.data.hook) && isDef(i = i.remove)) {
// [ts] i(vnode, rm);
// [ts] } else {
// [ts] rm();
// [ts] }
// [ts] } else {
// [ts] removeNode(vnode.elm);
// [ts] }
// [ts] }
// [ts] fn updateChildren(parentElm: auto, oldCh: auto, newCh: auto, insertedVnodeQueue: auto, removeOnly: auto) {
// [ts] let oldStartIdx = 0;
// [ts] let newStartIdx = 0;
// [ts] let oldEndIdx = oldCh.length - 1;
// [ts] let oldStartVnode = oldCh[0];
// [ts] let oldEndVnode = oldCh[oldEndIdx];
// [ts] let newEndIdx = newCh.length - 1;
// [ts] let newStartVnode = newCh[0];
// [ts] let newEndVnode = newCh[newEndIdx];
// [ts] let oldKeyToIdx, idxInOld, vnodeToMove, refElm;
// removeOnly is a special flag used only by <transition-group>
// to ensure removed elements stay in correct relative positions
// during leaving transitions
// [ts] let canMove = !removeOnly;
// [ts] {
// [ts] checkDuplicateKeys(newCh);
// [ts] }
// [ts] while (oldStartIdx <= oldEndIdx && newStartIdx <= newEndIdx) {
// [ts] if (isUndef(oldStartVnode)) {
// [ts] oldStartVnode = oldCh[++oldStartIdx]; // Vnode has been moved left
// [ts] } else if (isUndef(oldEndVnode)) {
// [ts] oldEndVnode = oldCh[--oldEndIdx];
// [ts] } else if (sameVnode(oldStartVnode, newStartVnode)) {
// [ts] patchVnode(oldStartVnode, newStartVnode, insertedVnodeQueue, newCh, newStartIdx);
// [ts] oldStartVnode = oldCh[++oldStartIdx];
// [ts] newStartVnode = newCh[++newStartIdx];
// [ts] } else if (sameVnode(oldEndVnode, newEndVnode)) {
// [ts] patchVnode(oldEndVnode, newEndVnode, insertedVnodeQueue, newCh, newEndIdx);
// [ts] oldEndVnode = oldCh[--oldEndIdx];
// [ts] newEndVnode = newCh[--newEndIdx];
// [ts] } else if (sameVnode(oldStartVnode, newEndVnode)) { // Vnode moved right
// [ts] patchVnode(oldStartVnode, newEndVnode, insertedVnodeQueue, newCh, newEndIdx);
// [ts] canMove && nodeOps.insertBefore(parentElm, oldStartVnode.elm, nodeOps.nextSibling(oldEndVnode.elm));
// [ts] oldStartVnode = oldCh[++oldStartIdx];
// [ts] newEndVnode = newCh[--newEndIdx];
// [ts] } else if (sameVnode(oldEndVnode, newStartVnode)) { // Vnode moved left
// [ts] patchVnode(oldEndVnode, newStartVnode, insertedVnodeQueue, newCh, newStartIdx);
// [ts] canMove && nodeOps.insertBefore(parentElm, oldEndVnode.elm, oldStartVnode.elm);
// [ts] oldEndVnode = oldCh[--oldEndIdx];
// [ts] newStartVnode = newCh[++newStartIdx];
// [ts] } else {
// [ts] if (isUndef(oldKeyToIdx)) { oldKeyToIdx = createKeyToOldIdx(oldCh, oldStartIdx, oldEndIdx); }
// [ts] idxInOld = if (isDef(newStartVnode.key)) oldKeyToIdx[newStartVnode.key] else findIdxInOld(newStartVnode, oldCh, oldStartIdx, oldEndIdx);
// [ts] if (isUndef(idxInOld)) { // New element
// [ts] createElm(newStartVnode, insertedVnodeQueue, parentElm, oldStartVnode.elm, false, newCh, newStartIdx);
// [ts] } else {
// [ts] vnodeToMove = oldCh[idxInOld];
// [ts] if (sameVnode(vnodeToMove, newStartVnode)) {
// [ts] patchVnode(vnodeToMove, newStartVnode, insertedVnodeQueue, newCh, newStartIdx);
// [ts] oldCh[idxInOld] = null;
// [ts] canMove && nodeOps.insertBefore(parentElm, vnodeToMove.elm, oldStartVnode.elm);
// [ts] } else {
// same key but different element. treat as new element
// [ts] createElm(newStartVnode, insertedVnodeQueue, parentElm, oldStartVnode.elm, false, newCh, newStartIdx);
// [ts] }
// [ts] }
// [ts] newStartVnode = newCh[++newStartIdx];
// [ts] }
// [ts] }
// [ts] if (oldStartIdx > oldEndIdx) {
// [ts] refElm = if (isUndef(newCh[newEndIdx + 1])) null else newCh[newEndIdx + 1].elm;
// [ts] addVnodes(parentElm, refElm, newCh, newStartIdx, newEndIdx, insertedVnodeQueue);
// [ts] } else if (newStartIdx > newEndIdx) {
// [ts] removeVnodes(oldCh, oldStartIdx, oldEndIdx);
// [ts] }
// [ts] }
// [ts] fn checkDuplicateKeys(children: auto) {
// [ts] let seenKeys = {};
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] let vnode = children[i];
// [ts] let key = vnode.key;
// [ts] if (isDef(key)) {
// [ts] if (seenKeys[key]) {
// [ts] warn(
// [ts] ("Duplicate keys detected: '" + key + "'. This may cause an update error."),
// [ts] vnode.context
// [ts] );
// [ts] } else {
// [ts] seenKeys[key] = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn findIdxInOld(node: auto, oldCh: auto, start: auto, end: auto) {
// [ts] let i = start;
// [ts] while (i < end) { // [cap-migrate] for-step: i++
// [ts] let c = oldCh[i];
// [ts] if (isDef(c) && sameVnode(node, c)) { return i }
// [ts] }
// [ts] }
// [ts] fn patchVnode(oldVnode: auto, vnode: auto, insertedVnodeQueue: auto, ownerArray: auto, index: auto, removeOnly: auto) {
// [ts] if (oldVnode == vnode) {
// [ts] return
// [ts] }
// [ts] if (isDef(vnode.elm) && isDef(ownerArray)) {
// clone reused vnode
// [ts] vnode = ownerArray[index] = cloneVNode(vnode);
// [ts] }
// [ts] let elm = vnode.elm = oldVnode.elm;
// [ts] if (isTrue(oldVnode.isAsyncPlaceholder)) {
// [ts] if (isDef(vnode.asyncFactory.resolved)) {
// [ts] hydrate(oldVnode.elm, vnode, insertedVnodeQueue);
// [ts] } else {
// [ts] vnode.isAsyncPlaceholder = true;
// [ts] }
// [ts] return
// [ts] }
// reuse element for static trees.
// note we only do this if the vnode is cloned -
// if the new node is not cloned it means the render functions have been
// reset by the hot-reload-api and we need to do a proper re-render.
// [ts] if (isTrue(vnode.isStatic) &&
// [ts] isTrue(oldVnode.isStatic) &&
// [ts] vnode.key == oldVnode.key &&
// [ts] (isTrue(vnode.isCloned) || isTrue(vnode.isOnce))
// [ts] ) {
// [ts] vnode.componentInstance = oldVnode.componentInstance;
// [ts] return
// [ts] }
// [ts] let i;
// [ts] let data = vnode.data;
// [ts] if (isDef(data) && isDef(i = data.hook) && isDef(i = i.prepatch)) {
// [ts] i(oldVnode, vnode);
// [ts] }
// [ts] let oldCh = oldVnode.children;
// [ts] let ch = vnode.children;
// [ts] if (isDef(data) && isPatchable(vnode)) {
// [ts] i = 0;
// [ts] while (i < cbs.update.length) { cbs.update[i](oldVnode, vnode); } // [cap-migrate] for-step: ++i
// [ts] if (isDef(i = data.hook) && isDef(i = i.update)) { i(oldVnode, vnode); }
// [ts] }
// [ts] if (isUndef(vnode.text)) {
// [ts] if (isDef(oldCh) && isDef(ch)) {
// [ts] if (oldCh != ch) { updateChildren(elm, oldCh, ch, insertedVnodeQueue, removeOnly); }
// [ts] } else if (isDef(ch)) {
// [ts] {
// [ts] checkDuplicateKeys(ch);
// [ts] }
// [ts] if (isDef(oldVnode.text)) { nodeOps.setTextContent(elm, ""); }
// [ts] addVnodes(elm, null, ch, 0, ch.length - 1, insertedVnodeQueue);
// [ts] } else if (isDef(oldCh)) {
// [ts] removeVnodes(oldCh, 0, oldCh.length - 1);
// [ts] } else if (isDef(oldVnode.text)) {
// [ts] nodeOps.setTextContent(elm, "");
// [ts] }
// [ts] } else if (oldVnode.text != vnode.text) {
// [ts] nodeOps.setTextContent(elm, vnode.text);
// [ts] }
// [ts] if (isDef(data)) {
// [ts] if (isDef(i = data.hook) && isDef(i = i.postpatch)) { i(oldVnode, vnode); }
// [ts] }
// [ts] }
// [ts] fn invokeInsertHook(vnode: auto, queue: auto, initial: auto) {
// delay insert hooks for component root nodes, invoke them after the
// element is really inserted
// [ts] if (isTrue(initial) && isDef(vnode.parent)) {
// [ts] vnode.parent.data.pendingInsert = queue;
// [ts] } else {
// [ts] let i = 0;
// [ts] while (i < queue.length) { // [cap-migrate] for-step: ++i
// [ts] queue[i].data.hook.insert(queue[i]);
// [ts] }
// [ts] }
// [ts] }
// [ts] let hydrationBailed = false;
// list of modules that can skip create hook during hydration because they
// are already rendered on the client or has no need for initialization
// Note: style is excluded because it relies on initial clone for future
// deep updates (#7063).
// [ts] let isRenderedModule = makeMap("attrs,class,staticClass,staticStyle,key");
// Note: this is a browser-only function so we can assume elms are DOM nodes.
// [ts] fn hydrate(elm: auto, vnode: auto, insertedVnodeQueue: auto, inVPre: auto) {
// [ts] let i;
// [ts] let tag = vnode.tag;
// [ts] let data = vnode.data;
// [ts] let children = vnode.children;
// [ts] inVPre = inVPre || (data && data.pre);
// [ts] vnode.elm = elm;
// [ts] if (isTrue(vnode.isComment) && isDef(vnode.asyncFactory)) {
// [ts] vnode.isAsyncPlaceholder = true;
// [ts] return true
// [ts] }
// assert node match
// [ts] {
// [ts] if (!assertNodeMatch(elm, vnode, inVPre)) {
// [ts] return false
// [ts] }
// [ts] }
// [ts] if (isDef(data)) {
// [ts] if (isDef(i = data.hook) && isDef(i = i.init)) { i(vnode, true /* hydrating */); }
// [ts] if (isDef(i = vnode.componentInstance)) {
// child component. it should have hydrated its own tree.
// [ts] initComponent(vnode, insertedVnodeQueue);
// [ts] return true
// [ts] }
// [ts] }
// [ts] if (isDef(tag)) {
// [ts] if (isDef(children)) {
// empty element, allow client to pick up and populate children
// [ts] if (!elm.hasChildNodes()) {
// [ts] createChildren(vnode, children, insertedVnodeQueue);
// [ts] } else {
// v-html and domProps: innerHTML
// [ts] if (isDef(i = data) && isDef(i = i.domProps) && isDef(i = i.innerHTML)) {
// [ts] if (i != elm.innerHTML) {
// [ts] /* istanbul ignore if */
// [ts] if (@typeName(console) != "undefined" &&
// [ts] !hydrationBailed
// [ts] ) {
// [ts] hydrationBailed = true;
// [ts] @print("Parent: ", elm);
// [ts] @print("server innerHTML: ", i);
// [ts] @print("client innerHTML: ", elm.innerHTML);
// [ts] }
// [ts] return false
// [ts] }
// [ts] } else {
// iterate and compare children lists
// [ts] let childrenMatch = true;
// [ts] let childNode = elm.firstChild;
// [ts] let i$1 = 0;
// [ts] while (i$1 < children.length) { // [cap-migrate] for-step: i$1++
// [ts] if (!childNode || !hydrate(childNode, children[i$1], insertedVnodeQueue, inVPre)) {
// [ts] childrenMatch = false;
// [ts] break
// [ts] }
// [ts] childNode = childNode.nextSibling;
// [ts] }
// if childNode is not null, it means the actual childNodes list is
// longer than the virtual children list.
// [ts] if (!childrenMatch || childNode) {
// [ts] /* istanbul ignore if */
// [ts] if (@typeName(console) != "undefined" &&
// [ts] !hydrationBailed
// [ts] ) {
// [ts] hydrationBailed = true;
// [ts] @print("Parent: ", elm);
// [ts] @print("Mismatching childNodes vs. VNodes: ", elm.childNodes, children);
// [ts] }
// [ts] return false
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (isDef(data)) {
// [ts] let fullInvoke = false;
// [ts] for (let key in data) {
// [ts] if (!isRenderedModule(key)) {
// [ts] fullInvoke = true;
// [ts] invokeCreateHooks(vnode, insertedVnodeQueue);
// [ts] break
// [ts] }
// [ts] }
// [ts] if (!fullInvoke && data["class"]) {
// ensure collecting deps for deep class bindings for future updates
// [ts] traverse(data["class"]);
// [ts] }
// [ts] }
// [ts] } else if (elm.data != vnode.text) {
// [ts] elm.data = vnode.text;
// [ts] }
// [ts] return true
// [ts] }
// [ts] fn assertNodeMatch(node: auto, vnode: auto, inVPre: auto) {
// [ts] if (isDef(vnode.tag)) {
// [ts] return vnode.tag.indexOf("vue-component") == 0 || (
// [ts] !isUnknownElement$$1(vnode, inVPre) &&
// [ts] vnode.tag.toLowerCase() == (node.tagName && node.tagName.toLowerCase())
// [ts] )
// [ts] } else {
// [ts] return node.nodeType == (if (vnode.isComment) 8 else 3)
// [ts] }
// [ts] }
// [ts] return (oldVnode, vnode, hydrating, removeOnly) => {
// [ts] if (isUndef(vnode)) {
// [ts] if (isDef(oldVnode)) { invokeDestroyHook(oldVnode); }
// [ts] return
// [ts] }
// [ts] let isInitialPatch = false;
// [ts] let insertedVnodeQueue = [];
// [ts] if (isUndef(oldVnode)) {
// empty mount (likely as component), create new root element
// [ts] isInitialPatch = true;
// [ts] createElm(vnode, insertedVnodeQueue);
// [ts] } else {
// [ts] let isRealElement = isDef(oldVnode.nodeType);
// [ts] if (!isRealElement && sameVnode(oldVnode, vnode)) {
// patch existing root node
// [ts] patchVnode(oldVnode, vnode, insertedVnodeQueue, null, null, removeOnly);
// [ts] } else {
// [ts] if (isRealElement) {
// mounting to a real element
// check if this is server-rendered content and if we can perform
// a successful hydration.
// [ts] if (oldVnode.nodeType == 1 && oldVnode.hasAttribute(SSR_ATTR)) {
// [ts] oldVnode.removeAttribute(SSR_ATTR);
// [ts] hydrating = true;
// [ts] }
// [ts] if (isTrue(hydrating)) {
// [ts] if (hydrate(oldVnode, vnode, insertedVnodeQueue)) {
// [ts] invokeInsertHook(vnode, insertedVnodeQueue, true);
// [ts] return oldVnode
// [ts] } else {
// [ts] warn(
// [ts] "The client-side rendered virtual DOM tree is not matching " +
// [ts] "server-rendered content. This is likely caused by incorrect " +
// [ts] "HTML markup, for example nesting block-level elements inside " +
// [ts] "<p>, or missing <tbody>. Bailing hydration and performing " +
// [ts] "full client-side render."
// [ts] );
// [ts] }
// [ts] }
// either not server-rendered, or hydration failed.
// create an empty node and replace it
// [ts] oldVnode = emptyNodeAt(oldVnode);
// [ts] }
// replacing existing element
// [ts] let oldElm = oldVnode.elm;
// [ts] let parentElm = nodeOps.parentNode(oldElm);
// create new node
// [ts] createElm(
// [ts] vnode,
// [ts] insertedVnodeQueue,
// extremely rare edge case: do not insert if old element is in a
// leaving transition. Only happens when combining transition +
// keep-alive + HOCs. (#4590)
// [ts] if (oldElm._leaveCb) null else parentElm,
// [ts] nodeOps.nextSibling(oldElm)
// [ts] );
// update parent placeholder node element, recursively
// [ts] if (isDef(vnode.parent)) {
// [ts] let ancestor = vnode.parent;
// [ts] let patchable = isPatchable(vnode);
// [ts] while (ancestor) {
// [ts] let i = 0;
// [ts] while (i < cbs.destroy.length) { // [cap-migrate] for-step: ++i
// [ts] cbs.destroy[i](ancestor);
// [ts] }
// [ts] ancestor.elm = vnode.elm;
// [ts] if (patchable) {
// [ts] let i$1 = 0;
// [ts] while (i$1 < cbs.create.length) { // [cap-migrate] for-step: ++i$1
// [ts] cbs.create[i$1](emptyNode, ancestor);
// [ts] }
// #6513
// invoke insert hooks that may have been merged by create hooks.
// e.g. for directives that uses the "inserted" hook.
// [ts] let insert = ancestor.data.hook.insert;
// [ts] if (insert.merged) {
// start at index 1 to avoid re-invoking component mounted hook
// [ts] let i$2 = 1;
// [ts] while (i$2 < insert.fns.length) { // [cap-migrate] for-step: i$2++
// [ts] insert.fns[i$2]();
// [ts] }
// [ts] }
// [ts] } else {
// [ts] registerRef(ancestor);
// [ts] }
// [ts] ancestor = ancestor.parent;
// [ts] }
// [ts] }
// destroy old node
// [ts] if (isDef(parentElm)) {
// [ts] removeVnodes([oldVnode], 0, 0);
// [ts] } else if (isDef(oldVnode.tag)) {
// [ts] invokeDestroyHook(oldVnode);
// [ts] }
// [ts] }
// [ts] }
// [ts] invokeInsertHook(vnode, insertedVnodeQueue, isInitialPatch);
// [ts] return vnode.elm
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let directives = {
// [ts] create: updateDirectives,
// [ts] update: updateDirectives,
// [ts] destroy: (vnode) => {
// [ts] updateDirectives(vnode, emptyNode);
// [ts] }
// [ts] };
// [ts] fn updateDirectives(oldVnode: auto, vnode: auto) {
// [ts] if (oldVnode.data.directives || vnode.data.directives) {
// [ts] _update(oldVnode, vnode);
// [ts] }
// [ts] }
// [ts] fn _update(oldVnode: auto, vnode: auto) {
// [ts] let isCreate = oldVnode == emptyNode;
// [ts] let isDestroy = vnode == emptyNode;
// [ts] let oldDirs = normalizeDirectives$1(oldVnode.data.directives, oldVnode.context);
// [ts] let newDirs = normalizeDirectives$1(vnode.data.directives, vnode.context);
// [ts] let dirsWithInsert = [];
// [ts] let dirsWithPostpatch = [];
// [ts] let key, oldDir, dir;
// [ts] for (key in newDirs) {
// [ts] oldDir = oldDirs[key];
// [ts] dir = newDirs[key];
// [ts] if (!oldDir) {
// new directive, bind
// [ts] callHook$1(dir, "bind", vnode, oldVnode);
// [ts] if (dir.def && dir.def.inserted) {
// [ts] dirsWithInsert.push(dir);
// [ts] }
// [ts] } else {
// existing directive, update
// [ts] dir.oldValue = oldDir.value;
// [ts] dir.oldArg = oldDir.arg;
// [ts] callHook$1(dir, "update", vnode, oldVnode);
// [ts] if (dir.def && dir.def.componentUpdated) {
// [ts] dirsWithPostpatch.push(dir);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (dirsWithInsert.length) {
// [ts] let callInsert = () => {
// [ts] let i = 0;
// [ts] while (i < dirsWithInsert.length) { // [cap-migrate] for-step: i++
// [ts] callHook$1(dirsWithInsert[i], "inserted", vnode, oldVnode);
// [ts] }
// [ts] };
// [ts] if (isCreate) {
// [ts] mergeVNodeHook(vnode, "insert", callInsert);
// [ts] } else {
// [ts] callInsert();
// [ts] }
// [ts] }
// [ts] if (dirsWithPostpatch.length) {
// [ts] mergeVNodeHook(vnode, "postpatch", () => {
// [ts] let i = 0;
// [ts] while (i < dirsWithPostpatch.length) { // [cap-migrate] for-step: i++
// [ts] callHook$1(dirsWithPostpatch[i], "componentUpdated", vnode, oldVnode);
// [ts] }
// [ts] });
// [ts] }
// [ts] if (!isCreate) {
// [ts] for (key in oldDirs) {
// [ts] if (!newDirs[key]) {
// no longer present, unbind
// [ts] callHook$1(oldDirs[key], "unbind", oldVnode, oldVnode, isDestroy);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let emptyModifiers = Object.create(null);
// [ts] function normalizeDirectives$1 (
// [ts] dirs,
// [ts] vm
// [ts] ) {
// [ts] let res = Object.create(null);
// [ts] if (!dirs) {
// $flow-disable-line
// [ts] return res
// [ts] }
// [ts] let i, dir;
// [ts] i = 0;
// [ts] while (i < dirs.length) { // [cap-migrate] for-step: i++
// [ts] dir = dirs[i];
// [ts] if (!dir.modifiers) {
// $flow-disable-line
// [ts] dir.modifiers = emptyModifiers;
// [ts] }
// [ts] res[getRawDirName(dir)] = dir;
// [ts] dir.def = resolveAsset(vm.$options, "directives", dir.name, true);
// [ts] }
// $flow-disable-line
// [ts] return res
// [ts] }
// [ts] fn getRawDirName(dir: auto) {
// [ts] return dir.rawName || ((dir.name) + "." + (Object.keys(dir.modifiers || {}).join(".")))
// [ts] }
// [ts] (dir, hook, vnode, oldVnode, isDestroy) => {
// [ts] let fn = dir.def && dir.def[hook];
// [ts] if (fn) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] fn(vnode.elm, dir, vnode, oldVnode, isDestroy);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] handleError(e, vnode.context, ("directive " + (dir.name) + " " + hook + " hook"));
// [ts] }
// [ts] }
// [ts] }
// [ts] let baseModules = [
// [ts] ref,
// [ts] directives
// [ts] ];
// [ts] /*  */
// [ts] fn updateAttrs(oldVnode: auto, vnode: auto) {
// [ts] let opts = vnode.componentOptions;
// [ts] if (isDef(opts) && opts.Ctor.options.inheritAttrs == false) {
// [ts] return
// [ts] }
// [ts] if (isUndef(oldVnode.data.attrs) && isUndef(vnode.data.attrs)) {
// [ts] return
// [ts] }
// [ts] let key, cur, old;
// [ts] let elm = vnode.elm;
// [ts] let oldAttrs = oldVnode.data.attrs || {};
// [ts] let attrs = vnode.data.attrs || {};
// clone observed objects, as the user probably wants to mutate it
// [ts] if (isDef(attrs.__ob__)) {
// [ts] attrs = vnode.data.attrs = extend({}, attrs);
// [ts] }
// [ts] for (key in attrs) {
// [ts] cur = attrs[key];
// [ts] old = oldAttrs[key];
// [ts] if (old != cur) {
// [ts] setAttr(elm, key, cur, vnode.data.pre);
// [ts] }
// [ts] }
// #4391: in IE9, setting type can reset value for input[type=radio]
// #6666: IE/Edge forces progress value down to 1 before setting a max
// [ts] /* istanbul ignore if */
// [ts] if ((isIE || isEdge) && attrs.value != oldAttrs.value) {
// [ts] setAttr(elm, "value", attrs.value);
// [ts] }
// [ts] for (key in oldAttrs) {
// [ts] if (isUndef(attrs[key])) {
// [ts] if (isXlink(key)) {
// [ts] elm.removeAttributeNS(xlinkNS, getXlinkProp(key));
// [ts] } else if (!isEnumeratedAttr(key)) {
// [ts] elm.removeAttribute(key);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn setAttr(el: auto, key: auto, value: auto, isInPre: auto) {
// [ts] if (isInPre || el.tagName.indexOf("-") > -1) {
// [ts] baseSetAttr(el, key, value);
// [ts] } else if (isBooleanAttr(key)) {
// set attribute for blank value
// e.g. <option disabled>Select one</option>
// [ts] if (isFalsyAttrValue(value)) {
// [ts] el.removeAttribute(key);
// [ts] } else {
// technically allowfullscreen is a boolean attribute for <iframe>,
// but Flash expects a value of "true" when used on <embed> tag
// [ts] value = if (key == "allowfullscreen" && el.tagName == "EMBED") "true" else key;
// [ts] el.setAttribute(key, value);
// [ts] }
// [ts] } else if (isEnumeratedAttr(key)) {
// [ts] el.setAttribute(key, convertEnumeratedValue(key, value));
// [ts] } else if (isXlink(key)) {
// [ts] if (isFalsyAttrValue(value)) {
// [ts] el.removeAttributeNS(xlinkNS, getXlinkProp(key));
// [ts] } else {
// [ts] el.setAttributeNS(xlinkNS, key, value);
// [ts] }
// [ts] } else {
// [ts] baseSetAttr(el, key, value);
// [ts] }
// [ts] }
// [ts] fn baseSetAttr(el: auto, key: auto, value: auto) {
// [ts] if (isFalsyAttrValue(value)) {
// [ts] el.removeAttribute(key);
// [ts] } else {
// #7138: IE10 & 11 fires input event when setting placeholder on
// <textarea>... block the first input event and remove the blocker
// immediately.
// [ts] /* istanbul ignore if */
// [ts] if (
// [ts] isIE && !isIE9 &&
// [ts] el.tagName == "TEXTAREA" &&
// [ts] key == "placeholder" && value != "" && !el.__ieph
// [ts] ) {
// [ts] let blocker = (e) => {
// [ts] e.stopImmediatePropagation();
// [ts] el.removeEventListener("input", blocker);
// [ts] };
// [ts] el.addEventListener("input", blocker);
// $flow-disable-line
// [ts] el.__ieph = true; /* IE placeholder patched */
// [ts] }
// [ts] el.setAttribute(key, value);
// [ts] }
// [ts] }
// [ts] let attrs = {
// [ts] create: updateAttrs,
// [ts] update: updateAttrs
// [ts] };
// [ts] /*  */
// [ts] fn updateClass(oldVnode: auto, vnode: auto) {
// [ts] let el = vnode.elm;
// [ts] let data = vnode.data;
// [ts] let oldData = oldVnode.data;
// [ts] if (
// [ts] isUndef(data.staticClass) &&
// [ts] isUndef(data.class) && (
// [ts] isUndef(oldData) || (
// [ts] isUndef(oldData.staticClass) &&
// [ts] isUndef(oldData.class)
// [ts] )
// [ts] )
// [ts] ) {
// [ts] return
// [ts] }
// [ts] let cls = genClassForVnode(vnode);
// handle transition classes
// [ts] let transitionClass = el._transitionClasses;
// [ts] if (isDef(transitionClass)) {
// [ts] cls = concat(cls, stringifyClass(transitionClass));
// [ts] }
// set the class
// [ts] if (cls != el._prevClass) {
// [ts] el.setAttribute("class", cls);
// [ts] el._prevClass = cls;
// [ts] }
// [ts] }
// [ts] let klass = {
// [ts] create: updateClass,
// [ts] update: updateClass
// [ts] };
// [ts] /*  */
// [ts] let validDivisionCharRE = "[\\w).+\\-_$\\]]";
// [ts] fn parseFilters(exp: auto) {
// [ts] let inSingle = false;
// [ts] let inDouble = false;
// [ts] let inTemplateString = false;
// [ts] let inRegex = false;
// [ts] let curly = 0;
// [ts] let square = 0;
// [ts] let paren = 0;
// [ts] let lastFilterIndex = 0;
// [ts] let c, prev, i, expression, filters;
// [ts] i = 0;
// [ts] while (i < exp.length) { // [cap-migrate] for-step: i++
// [ts] prev = c;
// [ts] c = exp.charCodeAt(i);
// [ts] if (inSingle) {
// [ts] if (c == 0x27 && prev != 0x5C) { inSingle = false; }
// [ts] } else if (inDouble) {
// [ts] if (c == 0x22 && prev != 0x5C) { inDouble = false; }
// [ts] } else if (inTemplateString) {
// [ts] if (c == 0x60 && prev != 0x5C) { inTemplateString = false; }
// [ts] } else if (inRegex) {
// [ts] if (c == 0x2f && prev != 0x5C) { inRegex = false; }
// [ts] } else if (
// [ts] c == 0x7C && // pipe
// [ts] exp.charCodeAt(i + 1) != 0x7C &&
// [ts] exp.charCodeAt(i - 1) != 0x7C &&
// [ts] !curly && !square && !paren
// [ts] ) {
// [ts] if (expression == null) {
// first filter, end of expression
// [ts] lastFilterIndex = i + 1;
// [ts] expression = exp.slice(0, i).trim();
// [ts] } else {
// [ts] pushFilter();
// [ts] }
// [ts] } else {
// [ts] switch (c) {
// [ts] 0x22 => { inDouble = true; break         // "; },
// [ts] 0x27 => { inSingle = true; break         // '; },
// [ts] 0x60 => { inTemplateString = true; break // `; },
// [ts] 0x28 => { paren++; break                 // (; },
// [ts] 0x29 => { paren--; break                 // ); },
// [ts] 0x5B => { square++; break                // [; },
// [ts] 0x5D => { square--; break                // ]; },
// [ts] 0x7B => { curly++; break                 // {; },
// [ts] 0x7D => { curly--; break                 // }; },
// [ts] }
// [ts] if (c == 0x2f) { // /
// [ts] let j = i - 1;
// [ts] let p = (void 0);
// find first non-whitespace prev char
// [ts] while (j >= 0) { // [cap-migrate] for-step: j--
// [ts] p = exp.charAt(j);
// [ts] if (p != " ") { break }
// [ts] }
// [ts] if (!p || !validDivisionCharRE.test(p)) {
// [ts] inRegex = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (expression == null) {
// [ts] expression = exp.slice(0, i).trim();
// [ts] } else if (lastFilterIndex != 0) {
// [ts] pushFilter();
// [ts] }
// [ts] fn pushFilter() {
// [ts] (filters || (filters = [])).push(exp.slice(lastFilterIndex, i).trim());
// [ts] lastFilterIndex = i + 1;
// [ts] }
// [ts] if (filters) {
// [ts] i = 0;
// [ts] while (i < filters.length) { // [cap-migrate] for-step: i++
// [ts] expression = wrapFilter(expression, filters[i]);
// [ts] }
// [ts] }
// [ts] return expression
// [ts] }
// [ts] fn wrapFilter(exp: auto, filter: auto) {
// [ts] let i = filter.indexOf("(");
// [ts] if (i < 0) {
// _f: resolveFilter
// [ts] return ("_f(\"" + filter + "\")(" + exp + ")")
// [ts] } else {
// [ts] let name = filter.slice(0, i);
// [ts] let args = filter.slice(i + 1);
// [ts] return ("_f(\"" + name + "\")(" + exp + (if (args != ")") "," + args else args))
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] /* eslint-disable no-unused-vars */
// [ts] fn baseWarn(msg: auto, range: auto) {
// [ts] @print(("[Vue compiler]: " + msg));
// [ts] }
// [ts] /* eslint-enable no-unused-vars */
// [ts] fn pluckModuleFunction(modules: auto, key: auto) {
// [ts] return if (modules) modules.map((m) => { return m[key]; }).filter((_unused) => { return _; }) else []
// [ts] }
// [ts] fn addProp(el: auto, name: auto, value: auto, range: auto, dynamic: auto) {
// [ts] (el.props || (el.props = [])).push(rangeSetItem({ name: name, value: value, dynamic: dynamic }, range));
// [ts] el.plain = false;
// [ts] }
// [ts] fn addAttr(el: auto, name: auto, value: auto, range: auto, dynamic: auto) {
// [ts] let attrs = if (dynamic) (el.dynamicAttrs || (el.dynamicAttrs = [])) else (el.attrs || (el.attrs = []));
// [ts] attrs.push(rangeSetItem({ name: name, value: value, dynamic: dynamic }, range));
// [ts] el.plain = false;
// [ts] }
// add a raw attr (use this in preTransforms)
// [ts] fn addRawAttr(el: auto, name: auto, value: auto, range: auto) {
// [ts] el.attrsMap[name] = value;
// [ts] el.attrsList.push(rangeSetItem({ name: name, value: value }, range));
// [ts] }
// [ts] fn addDirective(el: auto, name: auto, rawName: auto, value: auto, arg: auto, isDynamicArg: auto, modifiers: auto, range: auto) {
// [ts] (el.directives || (el.directives = [])).push(rangeSetItem({
// [ts] name: name,
// [ts] rawName: rawName,
// [ts] value: value,
// [ts] arg: arg,
// [ts] isDynamicArg: isDynamicArg,
// [ts] modifiers: modifiers
// [ts] }, range));
// [ts] el.plain = false;
// [ts] }
// [ts] fn prependModifierMarker(symbol: auto, name: auto, dynamic: auto) {
// [ts] return if (dynamic) ("_p(" + name + ",\"" + symbol + "\")") else symbol + name // mark the event as captured
// [ts] }
// [ts] fn addHandler(el: auto, name: auto, value: auto, modifiers: auto, important: auto, warn: auto, range: auto, dynamic: auto) {
// [ts] modifiers = modifiers || emptyObject;
// warn prevent and passive modifier
// [ts] /* istanbul ignore if */
// [ts] if (
// [ts] warn &&
// [ts] modifiers.prevent && modifiers.passive
// [ts] ) {
// [ts] warn(
// [ts] "passive and prevent can't be used together. " +
// [ts] "Passive handler can't prevent default event.",
// [ts] range
// [ts] );
// [ts] }
// normalize click.right and click.middle since they don't actually fire
// this is technically browser-specific, but at least for now browsers are
// the only target envs that have right/middle clicks.
// [ts] if (modifiers.right) {
// [ts] if (dynamic) {
// [ts] name = "(" + name + ")==='click'?'contextmenu':(" + name + ")";
// [ts] } else if (name == "click") {
// [ts] name = "contextmenu";
// [ts] delete modifiers.right;
// [ts] }
// [ts] } else if (modifiers.middle) {
// [ts] if (dynamic) {
// [ts] name = "(" + name + ")==='click'?'mouseup':(" + name + ")";
// [ts] } else if (name == "click") {
// [ts] name = "mouseup";
// [ts] }
// [ts] }
// check capture modifier
// [ts] if (modifiers.capture) {
// [ts] delete modifiers.capture;
// [ts] name = prependModifierMarker("!", name, dynamic);
// [ts] }
// [ts] if (modifiers.once) {
// [ts] delete modifiers.once;
// [ts] name = prependModifierMarker("~", name, dynamic);
// [ts] }
// [ts] /* istanbul ignore if */
// [ts] if (modifiers.passive) {
// [ts] delete modifiers.passive;
// [ts] name = prependModifierMarker("&", name, dynamic);
// [ts] }
// [ts] let events;
// [ts] if (modifiers.native) {
// [ts] delete modifiers.native;
// [ts] events = el.nativeEvents || (el.nativeEvents = {});
// [ts] } else {
// [ts] events = el.events || (el.events = {});
// [ts] }
// [ts] let newHandler = rangeSetItem({ value: value.trim(), dynamic: dynamic }, range);
// [ts] if (modifiers != emptyObject) {
// [ts] newHandler.modifiers = modifiers;
// [ts] }
// [ts] let handlers = events[name];
// [ts] /* istanbul ignore if */
// [ts] if (Array.isArray(handlers)) {
// [ts] if (important) handlers.unshift(newHandler) else handlers.push(newHandler);
// [ts] } else if (handlers) {
// [ts] events[name] = if (important) [newHandler, handlers] else [handlers, newHandler];
// [ts] } else {
// [ts] events[name] = newHandler;
// [ts] }
// [ts] el.plain = false;
// [ts] }
// [ts] fn getRawBindingAttr(el: auto, name: auto) {
// [ts] return el.rawAttrsMap[":" + name] ||
// [ts] el.rawAttrsMap["v-bind:" + name] ||
// [ts] el.rawAttrsMap[name]
// [ts] }
// [ts] fn getBindingAttr(el: auto, name: auto, getStatic: auto) {
// [ts] let dynamicValue =
// [ts] getAndRemoveAttr(el, ":" + name) ||
// [ts] getAndRemoveAttr(el, "v-bind:" + name);
// [ts] if (dynamicValue != null) {
// [ts] return parseFilters(dynamicValue)
// [ts] } else if (getStatic != false) {
// [ts] let staticValue = getAndRemoveAttr(el, name);
// [ts] if (staticValue != null) {
// [ts] return JSON.stringify(staticValue)
// [ts] }
// [ts] }
// [ts] }
// note: this only removes the attr from the Array (attrsList) so that it
// doesn't get processed by processAttrs.
// By default it does NOT remove it from the map (attrsMap) because the map is
// needed during codegen.
// [ts] fn getAndRemoveAttr(el: auto, name: auto, removeFromMap: auto) {
// [ts] let val;
// [ts] if ((val = el.attrsMap[name]) != null) {
// [ts] let list = el.attrsList;
// [ts] let i = 0, l = list.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] if (list[i].name == name) {
// [ts] list.splice(i, 1);
// [ts] break
// [ts] }
// [ts] }
// [ts] }
// [ts] if (removeFromMap) {
// [ts] delete el.attrsMap[name];
// [ts] }
// [ts] return val
// [ts] }
// [ts] fn getAndRemoveAttrByRegex(el: auto, name: auto) {
// [ts] let list = el.attrsList;
// [ts] let i = 0, l = list.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] let attr = list[i];
// [ts] if (name.test(attr.name)) {
// [ts] list.splice(i, 1);
// [ts] return attr
// [ts] }
// [ts] }
// [ts] }
// [ts] fn rangeSetItem(item: auto, range: auto) {
// [ts] if (range) {
// [ts] if (range.start != null) {
// [ts] item.start = range.start;
// [ts] }
// [ts] if (range.end != null) {
// [ts] item.end = range.end;
// [ts] }
// [ts] }
// [ts] return item
// [ts] }
// [ts] /*  */
// [ts] /**
// [ts] * Cross-platform code generation for component v-model
// [ts] */
// [ts] fn genComponentModel(el: auto, value: auto, modifiers: auto) {
// [ts] let ref = modifiers || {};
// [ts] let number = ref.number;
// [ts] let trim = ref.trim;
// [ts] let baseValueExpression = "$$v";
// [ts] let valueExpression = baseValueExpression;
// [ts] if (trim) {
// [ts] valueExpression =
// [ts] "(typeof " + baseValueExpression + " === 'string'" +
// [ts] "? " + baseValueExpression + ".trim()" +
// [ts] ": " + baseValueExpression + ")";
// [ts] }
// [ts] if (number) {
// [ts] valueExpression = "_n(" + valueExpression + ")";
// [ts] }
// [ts] let assignment = genAssignmentCode(value, valueExpression);
// [ts] el.model = {
// [ts] value: ("(" + value + ")"),
// [ts] expression: JSON.stringify(value),
// [ts] callback: ("function (" + baseValueExpression + ") {" + assignment + "}")
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Cross-platform codegen helper for generating v-model value assignment code.
// [ts] */
// [ts] fn genAssignmentCode(value: auto, assignment: auto) {
// [ts] let res = parseModel(value);
// [ts] if (res.key == null) {
// [ts] return (value + "=" + assignment)
// [ts] } else {
// [ts] return ("$set(" + (res.exp) + ", " + (res.key) + ", " + assignment + ")")
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Parse a v-model expression into a base path and a final key segment.
// [ts] * Handles both dot-path and possible square brackets.
// [ts] *
// [ts] * Possible cases:
// [ts] *
// [ts] * - test
// [ts] * - test[key]
// [ts] * - test[test1[key]]
// [ts] * - test["a"][key]
// [ts] * - xxx.test[a[a].test1[key]]
// [ts] * - test.xxx.a["asa"][test1[key]]
// [ts] *
// [ts] */
// [ts] let len, str, chr, index$1, expressionPos, expressionEndPos;
// [ts] fn parseModel(val: auto) {
// Fix https://github.com/vuejs/vue/pull/7730
// allow v-model="obj.val " (trailing whitespace)
// [ts] val = val.trim();
// [ts] len = val.length;
// [ts] if (val.indexOf("[") < 0 || val.lastIndexOf("]") < len - 1) {
// [ts] index$1 = val.lastIndexOf(".");
// [ts] if (index$1 > -1) {
// [ts] return {
// [ts] exp: val.slice(0, index$1),
// [ts] key: "\"" + val.slice(index$1 + 1) + "\""
// [ts] }
// [ts] } else {
// [ts] return {
// [ts] exp: val,
// [ts] key: null
// [ts] }
// [ts] }
// [ts] }
// [ts] str = val;
// [ts] index$1 = expressionPos = expressionEndPos = 0;
// [ts] while (!eof()) {
// [ts] chr = next();
// [ts] /* istanbul ignore if */
// [ts] if (isStringStart(chr)) {
// [ts] parseString(chr);
// [ts] } else if (chr == 0x5B) {
// [ts] parseBracket(chr);
// [ts] }
// [ts] }
// [ts] return {
// [ts] exp: val.slice(0, expressionPos),
// [ts] key: val.slice(expressionPos + 1, expressionEndPos)
// [ts] }
// [ts] }
// [ts] fn next() {
// [ts] return str.charCodeAt(++index$1)
// [ts] }
// [ts] fn eof() {
// [ts] return index$1 >= len
// [ts] }
// [ts] fn isStringStart(chr: auto) {
// [ts] return chr == 0x22 || chr == 0x27
// [ts] }
// [ts] fn parseBracket(chr: auto) {
// [ts] let inBracket = 1;
// [ts] expressionPos = index$1;
// [ts] while (!eof()) {
// [ts] chr = next();
// [ts] if (isStringStart(chr)) {
// [ts] parseString(chr);
// [ts] continue
// [ts] }
// [ts] if (chr == 0x5B) { inBracket++; }
// [ts] if (chr == 0x5D) { inBracket--; }
// [ts] if (inBracket == 0) {
// [ts] expressionEndPos = index$1;
// [ts] break
// [ts] }
// [ts] }
// [ts] }
// [ts] fn parseString(chr: auto) {
// [ts] let stringQuote = chr;
// [ts] while (!eof()) {
// [ts] chr = next();
// [ts] if (chr == stringQuote) {
// [ts] break
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let warn$1;
// in some cases, the event used has to be determined at runtime
// so we used some reserved tokens during compile.
// [ts] let RANGE_TOKEN = "__r";
// [ts] let CHECKBOX_RADIO_TOKEN = "__c";
// [ts] fn model(el: auto, dir: auto, _warn: auto) {
// [ts] warn$1 = _warn;
// [ts] let value = dir.value;
// [ts] let modifiers = dir.modifiers;
// [ts] let tag = el.tag;
// [ts] let type = el.attrsMap.type;
// [ts] {
// inputs with type="file" are read only and setting the input's
// value will throw an error.
// [ts] if (tag == "input" && type == "file") {
// [ts] warn$1(
// [ts] "<" + (el.tag) + " v-model=\"" + value + "\" type=\"file\">:\n" +
// [ts] "File inputs are read only. Use a v-on:change listener instead.",
// [ts] el.rawAttrsMap["v-model"]
// [ts] );
// [ts] }
// [ts] }
// [ts] if (el.component) {
// [ts] genComponentModel(el, value, modifiers);
// component v-model doesn't need extra runtime
// [ts] return false
// [ts] } else if (tag == "select") {
// [ts] genSelect(el, value, modifiers);
// [ts] } else if (tag == "input" && type == "checkbox") {
// [ts] genCheckboxModel(el, value, modifiers);
// [ts] } else if (tag == "input" && type == "radio") {
// [ts] genRadioModel(el, value, modifiers);
// [ts] } else if (tag == "input" || tag == "textarea") {
// [ts] genDefaultModel(el, value, modifiers);
// [ts] } else if (!config.isReservedTag(tag)) {
// [ts] genComponentModel(el, value, modifiers);
// component v-model doesn't need extra runtime
// [ts] return false
// [ts] } else {
// [ts] warn$1(
// [ts] "<" + (el.tag) + " v-model=\"" + value + "\">: " +
// [ts] "v-model is not supported on this element type. " +
// [ts] "If you are working with contenteditable, it's recommended to " +
// [ts] "wrap a library dedicated for that purpose inside a custom component.",
// [ts] el.rawAttrsMap["v-model"]
// [ts] );
// [ts] }
// ensure runtime directive metadata
// [ts] return true
// [ts] }
// [ts] fn genCheckboxModel(el: auto, value: auto, modifiers: auto) {
// [ts] let number = modifiers && modifiers.number;
// [ts] let valueBinding = getBindingAttr(el, "value") || "null";
// [ts] let trueValueBinding = getBindingAttr(el, "true-value") || "true";
// [ts] let falseValueBinding = getBindingAttr(el, "false-value") || "false";
// [ts] addProp(el, "checked",
// [ts] "Array.isArray(" + value + ")" +
// [ts] "?_i(" + value + "," + valueBinding + ")>-1" + (
// [ts] if (trueValueBinding == "true") (":(" + value + ")") else (":_q(" + value + "," + trueValueBinding + ")")
// [ts] )
// [ts] );
// [ts] addHandler(el, "change",
// [ts] "var $$a=" + value + "," +
// [ts] "$$el=$event.target," +
// [ts] "$$c=$$el.checked?(" + trueValueBinding + "):(" + falseValueBinding + ");" +
// [ts] "if(Array.isArray($$a)){" +
// [ts] "var $$v=" + (if (number) "_n(" + valueBinding + ")" else valueBinding) + "," +
// [ts] "$$i=_i($$a,$$v);" +
// [ts] "if($$el.checked){$$i<0&&(" + (genAssignmentCode(value, "$$a.concat([$$v])")) + ")}" +
// [ts] "else{$$i>-1&&(" + (genAssignmentCode(value, "$$a.slice(0,$$i).concat($$a.slice($$i+1))")) + ")}" +
// [ts] "}else{" + (genAssignmentCode(value, "$$c")) + "}",
// [ts] null, true
// [ts] );
// [ts] }
// [ts] fn genRadioModel(el: auto, value: auto, modifiers: auto) {
// [ts] let number = modifiers && modifiers.number;
// [ts] let valueBinding = getBindingAttr(el, "value") || "null";
// [ts] valueBinding = if (number) ("_n(" + valueBinding + ")") else valueBinding;
// [ts] addProp(el, "checked", ("_q(" + value + "," + valueBinding + ")"));
// [ts] addHandler(el, "change", genAssignmentCode(value, valueBinding), null, true);
// [ts] }
// [ts] fn genSelect(el: auto, value: auto, modifiers: auto) {
// [ts] let number = modifiers && modifiers.number;
// [ts] let selectedVal = "Array.prototype.filter" +
// [ts] ".call($event.target.options,function(o){return o.selected})" +
// [ts] ".map(function(o){var val = \"_value\" in o ? o._value : o.value;" +
// [ts] "return " + (if (number) "_n(val)" else "val") + "})";
// [ts] let assignment = "$event.target.multiple ? $$selectedVal : $$selectedVal[0]";
// [ts] let code = "var $$selectedVal = " + selectedVal + ";";
// [ts] code = code + " " + (genAssignmentCode(value, assignment));
// [ts] addHandler(el, "change", code, null, true);
// [ts] }
// [ts] fn genDefaultModel(el: auto, value: auto, modifiers: auto) {
// [ts] let type = el.attrsMap.type;
// warn if v-bind:value conflicts with v-model
// except for inputs with v-bind:type
// [ts] {
// [ts] let value$1 = el.attrsMap["v-bind:value"] || el.attrsMap[":value"];
// [ts] let typeBinding = el.attrsMap["v-bind:type"] || el.attrsMap[":type"];
// [ts] if (value$1 && !typeBinding) {
// [ts] let binding = if (el.attrsMap["v-bind:value"]) "v-bind:value" else ":value";
// [ts] warn$1(
// [ts] binding + "=\"" + value$1 + "\" conflicts with v-model on the same element " +
// [ts] "because the latter already expands to a value binding internally",
// [ts] el.rawAttrsMap[binding]
// [ts] );
// [ts] }
// [ts] }
// [ts] let ref = modifiers || {};
// [ts] let lazy = ref.lazy;
// [ts] let number = ref.number;
// [ts] let trim = ref.trim;
// [ts] let needCompositionGuard = !lazy && type != "range";
// [ts] let event = if (lazy) "change" else if (type == "range") RANGE_TOKEN else "input";
// [ts] let valueExpression = "$event.target.value";
// [ts] if (trim) {
// [ts] valueExpression = "$event.target.value.trim()";
// [ts] }
// [ts] if (number) {
// [ts] valueExpression = "_n(" + valueExpression + ")";
// [ts] }
// [ts] let code = genAssignmentCode(value, valueExpression);
// [ts] if (needCompositionGuard) {
// [ts] code = "if($event.target.composing)return;" + code;
// [ts] }
// [ts] addProp(el, "value", ("(" + value + ")"));
// [ts] addHandler(el, event, code, null, true);
// [ts] if (trim || number) {
// [ts] addHandler(el, "blur", "$forceUpdate()");
// [ts] }
// [ts] }
// [ts] /*  */
// normalize v-model event tokens that can only be determined at runtime.
// it's important to place the event as the first in the array because
// the whole point is ensuring the v-model callback gets called before
// user-attached handlers.
// [ts] fn normalizeEvents(on: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (isDef(on[RANGE_TOKEN])) {
// IE input[type=range] only supports `change` event
// [ts] let event = if (isIE) "change" else "input";
// [ts] on[event] = [].concat(on[RANGE_TOKEN], on[event] || []);
// [ts] delete on[RANGE_TOKEN];
// [ts] }
// This was originally intended to fix #4521 but no longer necessary
// after 2.5. Keeping it for backwards compat with generated code from < 2.4
// [ts] /* istanbul ignore if */
// [ts] if (isDef(on[CHECKBOX_RADIO_TOKEN])) {
// [ts] on.change = [].concat(on[CHECKBOX_RADIO_TOKEN], on.change || []);
// [ts] delete on[CHECKBOX_RADIO_TOKEN];
// [ts] }
// [ts] }
// [ts] let target$1;
// [ts] (event, handler, capture) => {
// [ts] let _target = target$1; // save current target element in closure
// [ts] return () => {
// [ts] let res = handler.apply(null, arguments);
// [ts] if (res != null) {
// [ts] remove$2(event, onceHandler, capture, _target);
// [ts] }
// [ts] }
// [ts] }
// #9446: Firefox <= 53 (in particular, ESR 52) has incorrect Event.timeStamp
// implementation and does not fire microtasks in between event propagation, so
// safe to exclude.
// [ts] let useMicrotaskFix = isUsingMicroTask && !(isFF && Number(isFF[1]) <= 53);
// [ts] function add$1 (
// [ts] name,
// [ts] handler,
// [ts] capture,
// [ts] passive
// [ts] ) {
// async edge case #6566: inner click event triggers patch, event handler
// attached to outer element during patch, and triggered again. This
// happens because browsers fire microtask ticks between event propagation.
// the solution is simple: we save the timestamp when a handler is attached,
// and the handler would only fire if the event passed to it was fired
// AFTER it was attached.
// [ts] if (useMicrotaskFix) {
// [ts] let attachedTimestamp = currentFlushTimestamp;
// [ts] let original = handler;
// [ts] handler = original._wrapper = (e) => {
// [ts] if (
// no bubbling, should always fire.
// this is just a safety net in case event.timeStamp is unreliable in
// certain weird environments...
// [ts] e.target == e.currentTarget ||
// event is fired after handler attachment
// [ts] e.timeStamp >= attachedTimestamp ||
// bail for environments that have buggy event.timeStamp implementations
// #9462 iOS 9 bug: event.timeStamp is 0 after history.pushState
// #9681 QtWebEngine event.timeStamp is negative value
// [ts] e.timeStamp <= 0 ||
// #9448 bail if event is fired in another document in a multi-page
// electron/nw.js app, since event.timeStamp will be using a different
// starting reference
// [ts] e.target.ownerDocument != document
// [ts] ) {
// [ts] return original.apply(this, arguments)
// [ts] }
// [ts] };
// [ts] }
// [ts] target$1.addEventListener(
// [ts] name,
// [ts] handler,
// [ts] if (supportsPassive) { capture: capture, passive: passive } else capture
// [ts] );
// [ts] }
// [ts] function remove$2 (
// [ts] name,
// [ts] handler,
// [ts] capture,
// [ts] _target
// [ts] ) {
// [ts] (_target || target$1).removeEventListener(
// [ts] name,
// [ts] handler._wrapper || handler,
// [ts] capture
// [ts] );
// [ts] }
// [ts] fn updateDOMListeners(oldVnode: auto, vnode: auto) {
// [ts] if (isUndef(oldVnode.data.on) && isUndef(vnode.data.on)) {
// [ts] return
// [ts] }
// [ts] let on = vnode.data.on || {};
// [ts] let oldOn = oldVnode.data.on || {};
// [ts] target$1 = vnode.elm;
// [ts] normalizeEvents(on);
// [ts] updateListeners(on, oldOn, add$1, remove$2, createOnceHandler$1, vnode.context);
// [ts] target$1 = null;
// [ts] }
// [ts] let events = {
// [ts] create: updateDOMListeners,
// [ts] update: updateDOMListeners
// [ts] };
// [ts] /*  */
// [ts] let svgContainer;
// [ts] fn updateDOMProps(oldVnode: auto, vnode: auto) {
// [ts] if (isUndef(oldVnode.data.domProps) && isUndef(vnode.data.domProps)) {
// [ts] return
// [ts] }
// [ts] let key, cur;
// [ts] let elm = vnode.elm;
// [ts] let oldProps = oldVnode.data.domProps || {};
// [ts] let props = vnode.data.domProps || {};
// clone observed objects, as the user probably wants to mutate it
// [ts] if (isDef(props.__ob__)) {
// [ts] props = vnode.data.domProps = extend({}, props);
// [ts] }
// [ts] for (key in oldProps) {
// [ts] if (!(key == props)) {
// [ts] elm[key] = "";
// [ts] }
// [ts] }
// [ts] for (key in props) {
// [ts] cur = props[key];
// ignore children if the node has textContent or innerHTML,
// as these will throw away existing DOM nodes and cause removal errors
// on subsequent patches (#3360)
// [ts] if (key == "textContent" || key == "innerHTML") {
// [ts] if (vnode.children) { vnode.children.length = 0; }
// [ts] if (cur == oldProps[key]) { continue }
// #6601 work around Chrome version <= 55 bug where single textNode
// replaced by innerHTML/textContent retains its parentNode property
// [ts] if (elm.childNodes.length == 1) {
// [ts] elm.removeChild(elm.childNodes[0]);
// [ts] }
// [ts] }
// [ts] if (key == "value" && elm.tagName != "PROGRESS") {
// store value as _value as well since
// non-string values will be stringified
// [ts] elm._value = cur;
// avoid resetting cursor position when value is the same
// [ts] let strCur = if (isUndef(cur)) "" else String(cur);
// [ts] if (shouldUpdateValue(elm, strCur)) {
// [ts] elm.value = strCur;
// [ts] }
// [ts] } else if (key == "innerHTML" && isSVG(elm.tagName) && isUndef(elm.innerHTML)) {
// IE doesn't support innerHTML for SVG elements
// [ts] svgContainer = svgContainer || document.createElement("div");
// [ts] svgContainer.innerHTML = "<svg>" + cur + "</svg>";
// [ts] let svg = svgContainer.firstChild;
// [ts] while (elm.firstChild) {
// [ts] elm.removeChild(elm.firstChild);
// [ts] }
// [ts] while (svg.firstChild) {
// [ts] elm.appendChild(svg.firstChild);
// [ts] }
// [ts] } else if (
// skip the update if old and new VDOM state is the same.
// `value` is handled separately because the DOM value may be temporarily
// out of sync with VDOM state due to focus, composition and modifiers.
// This  #4521 by skipping the unnecessary `checked` update.
// [ts] cur != oldProps[key]
// [ts] ) {
// some property updates can throw
// e.g. `value` on <progress> w/ non-finite value
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] elm[key] = cur;
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {})
// [ts] }
// [ts] }
// [ts] }
// check platforms/web/util/attrs.js acceptValue
// [ts] fn shouldUpdateValue(elm: auto, checkVal: auto) {
// [ts] return (!elm.composing && (
// [ts] elm.tagName == "OPTION" ||
// [ts] isNotInFocusAndDirty(elm, checkVal) ||
// [ts] isDirtyWithModifiers(elm, checkVal)
// [ts] ))
// [ts] }
// [ts] fn isNotInFocusAndDirty(elm: auto, checkVal: auto) {
// return true when textbox (.number and .trim) loses focus and its value is
// not equal to the updated value
// [ts] let notInFocus = true;
// #6157
// work around IE bug when accessing document.activeElement in an iframe
// TODO(cap-migrate): replace try/catch with CP ?T / ?: try { notInFocus = document.activeElement !== elm; } catch (e) {}
// [ts] return notInFocus && elm.value != checkVal
// [ts] }
// [ts] fn isDirtyWithModifiers(elm: auto, newVal: auto) {
// [ts] let value = elm.value;
// [ts] let modifiers = elm._vModifiers; // injected by v-model runtime
// [ts] if (isDef(modifiers)) {
// [ts] if (modifiers.number) {
// [ts] return toNumber(value) != toNumber(newVal)
// [ts] }
// [ts] if (modifiers.trim) {
// [ts] return value.trim() != newVal.trim()
// [ts] }
// [ts] }
// [ts] return value != newVal
// [ts] }
// [ts] let domProps = {
// [ts] create: updateDOMProps,
// [ts] update: updateDOMProps
// [ts] };
// [ts] /*  */
// [ts] let parseStyleText = cached((cssText) => {
// [ts] let res = {};
// [ts] let listDelimiter = ";(?![^(]*\\))";
// [ts] let propertyDelimiter = ":(.+)";
// [ts] cssText.split(listDelimiter).forEach((item) => {
// [ts] if (item) {
// [ts] let tmp = item.split(propertyDelimiter);
// [ts] tmp.length > 1 && (res[tmp[0].trim()] = tmp[1].trim());
// [ts] }
// [ts] });
// [ts] return res
// [ts] });
// merge static and dynamic style data on the same vnode
// [ts] fn normalizeStyleData(data: auto) {
// [ts] let style = normalizeStyleBinding(data.style);
// static style is pre-processed into an object during compilation
// and is always a fresh object, so it's safe to merge into it
// [ts] return if (data.staticStyle) extend(data.staticStyle, style) else style
// [ts] }
// normalize possible array / string values into Object
// [ts] fn normalizeStyleBinding(bindingStyle: auto) {
// [ts] if (Array.isArray(bindingStyle)) {
// [ts] return toObject(bindingStyle)
// [ts] }
// [ts] if (@typeName(bindingStyle) == "string") {
// [ts] return parseStyleText(bindingStyle)
// [ts] }
// [ts] return bindingStyle
// [ts] }
// [ts] /**
// [ts] * parent component style should be after child's
// [ts] * so that parent component's style could override it
// [ts] */
// [ts] fn getStyle(vnode: auto, checkChild: auto) {
// [ts] let res = {};
// [ts] let styleData;
// [ts] if (checkChild) {
// [ts] let childNode = vnode;
// [ts] while (childNode.componentInstance) {
// [ts] childNode = childNode.componentInstance._vnode;
// [ts] if (
// [ts] childNode && childNode.data &&
// [ts] (styleData = normalizeStyleData(childNode.data))
// [ts] ) {
// [ts] extend(res, styleData);
// [ts] }
// [ts] }
// [ts] }
// [ts] if ((styleData = normalizeStyleData(vnode.data))) {
// [ts] extend(res, styleData);
// [ts] }
// [ts] let parentNode = vnode;
// [ts] while ((parentNode = parentNode.parent)) {
// [ts] if (parentNode.data && (styleData = normalizeStyleData(parentNode.data))) {
// [ts] extend(res, styleData);
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] /*  */
// [ts] let cssVarRE = "^--";
// [ts] let importantRE = "\\s*!important$";
// [ts] let setProp = (el, name, val) => {
// [ts] /* istanbul ignore if */
// [ts] if (cssVarRE.test(name)) {
// [ts] el.style.setProperty(name, val);
// [ts] } else if (importantRE.test(val)) {
// [ts] el.style.setProperty(hyphenate(name), val.replace(importantRE, ""), "important");
// [ts] } else {
// [ts] let normalizedName = normalize(name);
// [ts] if (Array.isArray(val)) {
// Support values array created by autoprefixer, e.g.
// {display: ["-webkit-box", "-ms-flexbox", "flex"]}
// Set them one by one, and the browser will only set those it can recognize
// [ts] let i = 0, len = val.length;
// [ts] while (i < len) { // [cap-migrate] for-step: i++
// [ts] el.style[normalizedName] = val[i];
// [ts] }
// [ts] } else {
// [ts] el.style[normalizedName] = val;
// [ts] }
// [ts] }
// [ts] };
// [ts] let vendorNames = ["Webkit", "Moz", "ms"];
// [ts] let emptyStyle;
// [ts] let normalize = cached((prop) => {
// [ts] emptyStyle = emptyStyle || document.createElement("div").style;
// [ts] prop = camelize(prop);
// [ts] if (prop != "filter" && (prop == emptyStyle)) {
// [ts] return prop
// [ts] }
// [ts] let capName = prop.charAt(0).toUpperCase() + prop.slice(1);
// [ts] let i = 0;
// [ts] while (i < vendorNames.length) { // [cap-migrate] for-step: i++
// [ts] let name = vendorNames[i] + capName;
// [ts] if (name == emptyStyle) {
// [ts] return name
// [ts] }
// [ts] }
// [ts] });
// [ts] fn updateStyle(oldVnode: auto, vnode: auto) {
// [ts] let data = vnode.data;
// [ts] let oldData = oldVnode.data;
// [ts] if (isUndef(data.staticStyle) && isUndef(data.style) &&
// [ts] isUndef(oldData.staticStyle) && isUndef(oldData.style)
// [ts] ) {
// [ts] return
// [ts] }
// [ts] let cur, name;
// [ts] let el = vnode.elm;
// [ts] let oldStaticStyle = oldData.staticStyle;
// [ts] let oldStyleBinding = oldData.normalizedStyle || oldData.style || {};
// if static style exists, stylebinding already merged into it when doing normalizeStyleData
// [ts] let oldStyle = oldStaticStyle || oldStyleBinding;
// [ts] let style = normalizeStyleBinding(vnode.data.style) || {};
// store normalized style under a different key for next diff
// make sure to clone it if it's reactive, since the user likely wants
// to mutate it.
// [ts] vnode.data.normalizedStyle = if (isDef(style.__ob__)) extend({}, style) else style;
// [ts] let newStyle = getStyle(vnode, true);
// [ts] for (name in oldStyle) {
// [ts] if (isUndef(newStyle[name])) {
// [ts] setProp(el, name, "");
// [ts] }
// [ts] }
// [ts] for (name in newStyle) {
// [ts] cur = newStyle[name];
// [ts] if (cur != oldStyle[name]) {
// ie9 setting to null has no effect, must use empty string
// [ts] setProp(if (el, name, cur == null) "" else cur);
// [ts] }
// [ts] }
// [ts] }
// [ts] let style = {
// [ts] create: updateStyle,
// [ts] update: updateStyle
// [ts] };
// [ts] /*  */
// [ts] let whitespaceRE = "\\s+";
// [ts] /**
// [ts] * Add class with compatibility for SVG since classList is not supported on
// [ts] * SVG elements in IE
// [ts] */
// [ts] fn addClass(el: auto, cls: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (!cls || !(cls = cls.trim())) {
// [ts] return
// [ts] }
// [ts] /* istanbul ignore else */
// [ts] if (el.classList) {
// [ts] if (cls.indexOf(" ") > -1) {
// [ts] cls.split(whitespaceRE).forEach((c) => { return el.classList.add(c); });
// [ts] } else {
// [ts] el.classList.add(cls);
// [ts] }
// [ts] } else {
// [ts] let cur = " " + (el.getAttribute("class") || "") + " ";
// [ts] if (cur.indexOf(" " + cls + " ") < 0) {
// [ts] el.setAttribute("class", (cur + cls).trim());
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Remove class with compatibility for SVG since classList is not supported on
// [ts] * SVG elements in IE
// [ts] */
// [ts] fn removeClass(el: auto, cls: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (!cls || !(cls = cls.trim())) {
// [ts] return
// [ts] }
// [ts] /* istanbul ignore else */
// [ts] if (el.classList) {
// [ts] if (cls.indexOf(" ") > -1) {
// [ts] cls.split(whitespaceRE).forEach((c) => { return el.classList.remove(c); });
// [ts] } else {
// [ts] el.classList.remove(cls);
// [ts] }
// [ts] if (!el.classList.length) {
// [ts] el.removeAttribute("class");
// [ts] }
// [ts] } else {
// [ts] let cur = " " + (el.getAttribute("class") || "") + " ";
// [ts] let tar = " " + cls + " ";
// [ts] while (cur.indexOf(tar) >= 0) {
// [ts] cur = cur.replace(tar, " ");
// [ts] }
// [ts] cur = cur.trim();
// [ts] if (cur) {
// [ts] el.setAttribute("class", cur);
// [ts] } else {
// [ts] el.removeAttribute("class");
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn resolveTransition(def$$1: auto) {
// [ts] if (!def$$1) {
// [ts] return
// [ts] }
// [ts] /* istanbul ignore else */
// [ts] if (@typeName(def$$1) == "object") {
// [ts] let res = {};
// [ts] if (def$$1.css != false) {
// [ts] extend(res, autoCssTransition(def$$1.name || "v"));
// [ts] }
// [ts] extend(res, def$$1);
// [ts] return res
// [ts] } else if (@typeName(def$$1) == "string") {
// [ts] return autoCssTransition(def$$1)
// [ts] }
// [ts] }
// [ts] let autoCssTransition = cached((name) => {
// [ts] return {
// [ts] enterClass: (name + "-enter"),
// [ts] enterToClass: (name + "-enter-to"),
// [ts] enterActiveClass: (name + "-enter-active"),
// [ts] leaveClass: (name + "-leave"),
// [ts] leaveToClass: (name + "-leave-to"),
// [ts] leaveActiveClass: (name + "-leave-active")
// [ts] }
// [ts] });
// [ts] let hasTransition = inBrowser && !isIE9;
// [ts] let TRANSITION = "transition";
// [ts] let ANIMATION = "animation";
// Transition property/event sniffing
// [ts] let transitionProp = "transition";
// [ts] let transitionEndEvent = "transitionend";
// [ts] let animationProp = "animation";
// [ts] let animationEndEvent = "animationend";
// [ts] if (hasTransition) {
// [ts] /* istanbul ignore if */
// [ts] if (window.ontransitionend == null &&
// [ts] window.onwebkittransitionend != null
// [ts] ) {
// [ts] transitionProp = "WebkitTransition";
// [ts] transitionEndEvent = "webkitTransitionEnd";
// [ts] }
// [ts] if (window.onanimationend == null &&
// [ts] window.onwebkitanimationend != null
// [ts] ) {
// [ts] animationProp = "WebkitAnimation";
// [ts] animationEndEvent = "webkitAnimationEnd";
// [ts] }
// [ts] }
// binding to window is necessary to make hot reload work in IE in strict mode
// [ts] let raf = inBrowser ? window.requestAnimationFrame ? window.requestAnimationFrame.bind(window) => { return fn(); };
// [ts] fn nextFrame(fn: auto) {
// [ts] raf(() => {
// [ts] raf(fn);
// [ts] });
// [ts] }
// [ts] fn addTransitionClass(el: auto, cls: auto) {
// [ts] let transitionClasses = el._transitionClasses || (el._transitionClasses = []);
// [ts] if (transitionClasses.indexOf(cls) < 0) {
// [ts] transitionClasses.push(cls);
// [ts] addClass(el, cls);
// [ts] }
// [ts] }
// [ts] fn removeTransitionClass(el: auto, cls: auto) {
// [ts] if (el._transitionClasses) {
// [ts] remove(el._transitionClasses, cls);
// [ts] }
// [ts] removeClass(el, cls);
// [ts] }
// [ts] fn whenTransitionEnds(el: auto, expectedType: auto, cb: auto) {
// [ts] let ref = getTransitionInfo(el, expectedType);
// [ts] let type = ref.type;
// [ts] let timeout = ref.timeout;
// [ts] let propCount = ref.propCount;
// [ts] if (!type) { return cb() }
// [ts] let event = if (type == TRANSITION) transitionEndEvent else animationEndEvent;
// [ts] let ended = 0;
// [ts] let end = () => {
// [ts] el.removeEventListener(event, onEnd);
// [ts] cb();
// [ts] };
// [ts] let onEnd = (e) => {
// [ts] if (e.target == el) {
// [ts] if (++ended >= propCount) {
// [ts] end();
// [ts] }
// [ts] }
// [ts] };
// [ts] setTimeout(() => {
// [ts] if (ended < propCount) {
// [ts] end();
// [ts] }
// [ts] }, timeout + 1);
// [ts] el.addEventListener(event, onEnd);
// [ts] }
// [ts] let transformRE = "\\b(transform|all)(,|$)";
// [ts] fn getTransitionInfo(el: auto, expectedType: auto) {
// [ts] let styles = window.getComputedStyle(el);
// JSDOM may return undefined for transition properties
// [ts] let transitionDelays = (styles[transitionProp + "Delay"] || "").split(", ");
// [ts] let transitionDurations = (styles[transitionProp + "Duration"] || "").split(", ");
// [ts] let transitionTimeout = getTimeout(transitionDelays, transitionDurations);
// [ts] let animationDelays = (styles[animationProp + "Delay"] || "").split(", ");
// [ts] let animationDurations = (styles[animationProp + "Duration"] || "").split(", ");
// [ts] let animationTimeout = getTimeout(animationDelays, animationDurations);
// [ts] let type;
// [ts] let timeout = 0;
// [ts] let propCount = 0;
// [ts] /* istanbul ignore if */
// [ts] if (expectedType == TRANSITION) {
// [ts] if (transitionTimeout > 0) {
// [ts] type = TRANSITION;
// [ts] timeout = transitionTimeout;
// [ts] propCount = transitionDurations.length;
// [ts] }
// [ts] } else if (expectedType == ANIMATION) {
// [ts] if (animationTimeout > 0) {
// [ts] type = ANIMATION;
// [ts] timeout = animationTimeout;
// [ts] propCount = animationDurations.length;
// [ts] }
// [ts] } else {
// [ts] timeout = Math.max(transitionTimeout, animationTimeout);
// [ts] type = if (timeout > 0) if (transitionTimeout > animationTimeout) TRANSITION else ANIMATION else null;
// [ts] propCount = if (type) if (type == TRANSITION) transitionDurations.length else animationDurations.length else 0;
// [ts] }
// [ts] let hasTransform =
// [ts] type == TRANSITION &&
// [ts] transformRE.test(styles[transitionProp + "Property"]);
// [ts] return {
// [ts] type: type,
// [ts] timeout: timeout,
// [ts] propCount: propCount,
// [ts] hasTransform: hasTransform
// [ts] }
// [ts] }
// [ts] fn getTimeout(delays: auto, durations: auto) {
// [ts] /* istanbul ignore next */
// [ts] while (delays.length < durations.length) {
// [ts] delays = delays.concat(delays);
// [ts] }
// [ts] return Math.max.apply(null, durations.map((d, i) => {
// [ts] return toMs(d) + toMs(delays[i])
// [ts] }))
// [ts] }
// Old versions of Chromium (below 61.0.3163.100) formats floating pointer numbers
// in a locale-dependent way, using a comma instead of a dot.
// If comma is not replaced with a dot, the input will be rounded down (i.e. acting
// as a floor function) causing unexpected behaviors
// [ts] fn toMs(s: auto) {
// [ts] return Number(s.slice(0, -1).replace(",", ".")) * 1000
// [ts] }
// [ts] /*  */
// [ts] fn enter(vnode: auto, toggleDisplay: auto) {
// [ts] let el = vnode.elm;
// call leave callback now
// [ts] if (isDef(el._leaveCb)) {
// [ts] el._leaveCb.cancelled = true;
// [ts] el._leaveCb();
// [ts] }
// [ts] let data = resolveTransition(vnode.data.transition);
// [ts] if (isUndef(data)) {
// [ts] return
// [ts] }
// [ts] /* istanbul ignore if */
// [ts] if (isDef(el._enterCb) || el.nodeType != 1) {
// [ts] return
// [ts] }
// [ts] let css = data.css;
// [ts] let type = data.type;
// [ts] let enterClass = data.enterClass;
// [ts] let enterToClass = data.enterToClass;
// [ts] let enterActiveClass = data.enterActiveClass;
// [ts] let appearClass = data.appearClass;
// [ts] let appearToClass = data.appearToClass;
// [ts] let appearActiveClass = data.appearActiveClass;
// [ts] let beforeEnter = data.beforeEnter;
// [ts] let enter = data.enter;
// [ts] let afterEnter = data.afterEnter;
// [ts] let enterCancelled = data.enterCancelled;
// [ts] let beforeAppear = data.beforeAppear;
// [ts] let appear = data.appear;
// [ts] let afterAppear = data.afterAppear;
// [ts] let appearCancelled = data.appearCancelled;
// [ts] let duration = data.duration;
// activeInstance will always be the <transition> component managing this
// transition. One edge case to check is when the <transition> is placed
// as the root node of a child component. In that case we need to check
// <transition>'s parent for appear check.
// [ts] let context = activeInstance;
// [ts] let transitionNode = activeInstance.$vnode;
// [ts] while (transitionNode && transitionNode.parent) {
// [ts] context = transitionNode.context;
// [ts] transitionNode = transitionNode.parent;
// [ts] }
// [ts] let isAppear = !context._isMounted || !vnode.isRootInsert;
// [ts] if (isAppear && !appear && appear != "") {
// [ts] return
// [ts] }
// [ts] let startClass = if (isAppear && appearClass) appearClass else enterClass;
// [ts] let activeClass = if (isAppear && appearActiveClass) appearActiveClass else enterActiveClass;
// [ts] let toClass = if (isAppear && appearToClass) appearToClass else enterToClass;
// [ts] let beforeEnterHook = if (isAppear) (beforeAppear || beforeEnter) else beforeEnter;
// [ts] let enterHook = if (isAppear) (if (@typeName(appear) == "function") appear else enter) else enter;
// [ts] let afterEnterHook = if (isAppear) (afterAppear || afterEnter) else afterEnter;
// [ts] let enterCancelledHook = if (isAppear) (appearCancelled || enterCancelled) else enterCancelled;
// [ts] let explicitEnterDuration = toNumber(
// [ts] if (isObject(duration)) duration.enter else duration
// [ts] );
// [ts] if (explicitEnterDuration != null) {
// [ts] checkDuration(explicitEnterDuration, "enter", vnode);
// [ts] }
// [ts] let expectsCSS = css != false && !isIE9;
// [ts] let userWantsControl = getHookArgumentsLength(enterHook);
// [ts] let cb = el._enterCb = once(() => {
// [ts] if (expectsCSS) {
// [ts] removeTransitionClass(el, toClass);
// [ts] removeTransitionClass(el, activeClass);
// [ts] }
// [ts] if (cb.cancelled) {
// [ts] if (expectsCSS) {
// [ts] removeTransitionClass(el, startClass);
// [ts] }
// [ts] enterCancelledHook && enterCancelledHook(el);
// [ts] } else {
// [ts] afterEnterHook && afterEnterHook(el);
// [ts] }
// [ts] el._enterCb = null;
// [ts] });
// [ts] if (!vnode.data.show) {
// remove pending leave element on enter by injecting an insert hook
// [ts] mergeVNodeHook(vnode, "insert", () => {
// [ts] let parent = el.parentNode;
// [ts] let pendingNode = parent && parent._pending && parent._pending[vnode.key];
// [ts] if (pendingNode &&
// [ts] pendingNode.tag == vnode.tag &&
// [ts] pendingNode.elm._leaveCb
// [ts] ) {
// [ts] pendingNode.elm._leaveCb();
// [ts] }
// [ts] enterHook && enterHook(el, cb);
// [ts] });
// [ts] }
// start enter transition
// [ts] beforeEnterHook && beforeEnterHook(el);
// [ts] if (expectsCSS) {
// [ts] addTransitionClass(el, startClass);
// [ts] addTransitionClass(el, activeClass);
// [ts] nextFrame(() => {
// [ts] removeTransitionClass(el, startClass);
// [ts] if (!cb.cancelled) {
// [ts] addTransitionClass(el, toClass);
// [ts] if (!userWantsControl) {
// [ts] if (isValidDuration(explicitEnterDuration)) {
// [ts] setTimeout(cb, explicitEnterDuration);
// [ts] } else {
// [ts] whenTransitionEnds(el, type, cb);
// [ts] }
// [ts] }
// [ts] }
// [ts] });
// [ts] }
// [ts] if (vnode.data.show) {
// [ts] toggleDisplay && toggleDisplay();
// [ts] enterHook && enterHook(el, cb);
// [ts] }
// [ts] if (!expectsCSS && !userWantsControl) {
// [ts] cb();
// [ts] }
// [ts] }
// [ts] fn leave(vnode: auto, rm: auto) {
// [ts] let el = vnode.elm;
// call enter callback now
// [ts] if (isDef(el._enterCb)) {
// [ts] el._enterCb.cancelled = true;
// [ts] el._enterCb();
// [ts] }
// [ts] let data = resolveTransition(vnode.data.transition);
// [ts] if (isUndef(data) || el.nodeType != 1) {
// [ts] return rm()
// [ts] }
// [ts] /* istanbul ignore if */
// [ts] if (isDef(el._leaveCb)) {
// [ts] return
// [ts] }
// [ts] let css = data.css;
// [ts] let type = data.type;
// [ts] let leaveClass = data.leaveClass;
// [ts] let leaveToClass = data.leaveToClass;
// [ts] let leaveActiveClass = data.leaveActiveClass;
// [ts] let beforeLeave = data.beforeLeave;
// [ts] let leave = data.leave;
// [ts] let afterLeave = data.afterLeave;
// [ts] let leaveCancelled = data.leaveCancelled;
// [ts] let delayLeave = data.delayLeave;
// [ts] let duration = data.duration;
// [ts] let expectsCSS = css != false && !isIE9;
// [ts] let userWantsControl = getHookArgumentsLength(leave);
// [ts] let explicitLeaveDuration = toNumber(
// [ts] if (isObject(duration)) duration.leave else duration
// [ts] );
// [ts] if (isDef(explicitLeaveDuration)) {
// [ts] checkDuration(explicitLeaveDuration, "leave", vnode);
// [ts] }
// [ts] let cb = el._leaveCb = once(() => {
// [ts] if (el.parentNode && el.parentNode._pending) {
// [ts] el.parentNode._pending[vnode.key] = null;
// [ts] }
// [ts] if (expectsCSS) {
// [ts] removeTransitionClass(el, leaveToClass);
// [ts] removeTransitionClass(el, leaveActiveClass);
// [ts] }
// [ts] if (cb.cancelled) {
// [ts] if (expectsCSS) {
// [ts] removeTransitionClass(el, leaveClass);
// [ts] }
// [ts] leaveCancelled && leaveCancelled(el);
// [ts] } else {
// [ts] rm();
// [ts] afterLeave && afterLeave(el);
// [ts] }
// [ts] el._leaveCb = null;
// [ts] });
// [ts] if (delayLeave) {
// [ts] delayLeave(performLeave);
// [ts] } else {
// [ts] performLeave();
// [ts] }
// [ts] fn performLeave() {
// the delayed leave may have already been cancelled
// [ts] if (cb.cancelled) {
// [ts] return
// [ts] }
// record leaving element
// [ts] if (!vnode.data.show && el.parentNode) {
// [ts] (el.parentNode._pending || (el.parentNode._pending = {}))[(vnode.key)] = vnode;
// [ts] }
// [ts] beforeLeave && beforeLeave(el);
// [ts] if (expectsCSS) {
// [ts] addTransitionClass(el, leaveClass);
// [ts] addTransitionClass(el, leaveActiveClass);
// [ts] nextFrame(() => {
// [ts] removeTransitionClass(el, leaveClass);
// [ts] if (!cb.cancelled) {
// [ts] addTransitionClass(el, leaveToClass);
// [ts] if (!userWantsControl) {
// [ts] if (isValidDuration(explicitLeaveDuration)) {
// [ts] setTimeout(cb, explicitLeaveDuration);
// [ts] } else {
// [ts] whenTransitionEnds(el, type, cb);
// [ts] }
// [ts] }
// [ts] }
// [ts] });
// [ts] }
// [ts] leave && leave(el, cb);
// [ts] if (!expectsCSS && !userWantsControl) {
// [ts] cb();
// [ts] }
// [ts] }
// [ts] }
// only used in dev mode
// [ts] fn checkDuration(val: auto, name: auto, vnode: auto) {
// [ts] if (@typeName(val) != "number") {
// [ts] warn(
// [ts] "<transition> explicit " + name + " duration is not a valid number - " +
// [ts] "got " + (JSON.stringify(val)) + ".",
// [ts] vnode.context
// [ts] );
// [ts] } else if (isNaN(val)) {
// [ts] warn(
// [ts] "<transition> explicit " + name + " duration is NaN - " +
// [ts] "the duration expression might be incorrect.",
// [ts] vnode.context
// [ts] );
// [ts] }
// [ts] }
// [ts] fn isValidDuration(val: auto) {
// [ts] return @typeName(val) == "number" && !isNaN(val)
// [ts] }
// [ts] /**
// [ts] * Normalize a transition hook's argument length. The hook may be:
// [ts] * - a merged hook (invoker) with the original in .fns
// [ts] * - a wrapped component method (check ._length)
// [ts] * - a plain function (.length)
// [ts] */
// [ts] fn getHookArgumentsLength(fn: auto) {
// [ts] if (isUndef(fn)) {
// [ts] return false
// [ts] }
// [ts] let invokerFns = fn.fns;
// [ts] if (isDef(invokerFns)) {
// invoker
// [ts] return getHookArgumentsLength(
// [ts] if (Array.isArray(invokerFns)) invokerFns[0] else invokerFns
// [ts] )
// [ts] } else {
// [ts] return (fn._length || fn.length) > 1
// [ts] }
// [ts] }
// [ts] fn _enter(_unused: auto, vnode: auto) {
// [ts] if (vnode.data.show != true) {
// [ts] enter(vnode);
// [ts] }
// [ts] }
// [ts] let transition = inBrowser ? {
// [ts] create: _enter,
// [ts] activate: _enter,
// [ts] remove: (vnode, rm) => {
// [ts] /* istanbul ignore else */
// [ts] if (vnode.data.show != true) {
// [ts] leave(vnode, rm);
// [ts] } else {
// [ts] rm();
// [ts] }
// [ts] }
// [ts] } : {};
// [ts] let platformModules = [
// [ts] attrs,
// [ts] klass,
// [ts] events,
// [ts] domProps,
// [ts] style,
// [ts] transition
// [ts] ];
// [ts] /*  */
// the directive module should be applied last, after all
// built-in modules have been applied.
// [ts] let modules = platformModules.concat(baseModules);
// [ts] let patch = createPatchFunction({ nodeOps: nodeOps, modules: modules });
// [ts] /**
// [ts] * Not type checking this file because flow doesn't like attaching
// [ts] * properties to Elements.
// [ts] */
// [ts] /* istanbul ignore if */
// [ts] if (isIE9) {
// http://www.matts411.com/post/internet-explorer-9-oninput/
// [ts] document.addEventListener("selectionchange", () => {
// [ts] let el = document.activeElement;
// [ts] if (el && el.vmodel) {
// [ts] trigger(el, "input");
// [ts] }
// [ts] });
// [ts] }
// [ts] let directive = {
// [ts] inserted: (el, binding, vnode, oldVnode) => {
// [ts] if (vnode.tag == "select") {
// #6903
// [ts] if (oldVnode.elm && !oldVnode.elm._vOptions) {
// [ts] mergeVNodeHook(vnode, "postpatch", () => {
// [ts] directive.componentUpdated(el, binding, vnode);
// [ts] });
// [ts] } else {
// [ts] setSelected(el, binding, vnode.context);
// [ts] }
// [ts] el._vOptions = [].map.call(el.options, getValue);
// [ts] } else if (vnode.tag == "textarea" || isTextInputType(el.type)) {
// [ts] el._vModifiers = binding.modifiers;
// [ts] if (!binding.modifiers.lazy) {
// [ts] el.addEventListener("compositionstart", onCompositionStart);
// [ts] el.addEventListener("compositionend", onCompositionEnd);
// Safari < 10.2 & UIWebView doesn't fire compositionend when
// switching focus before confirming composition choice
// this also fixes the issue where some browsers e.g. iOS Chrome
// fires "change" instead of "input" on autocomplete.
// [ts] el.addEventListener("change", onCompositionEnd);
// [ts] /* istanbul ignore if */
// [ts] if (isIE9) {
// [ts] el.vmodel = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] },
// [ts] componentUpdated: (el, binding, vnode) => {
// [ts] if (vnode.tag == "select") {
// [ts] setSelected(el, binding, vnode.context);
// in case the options rendered by v-for have changed,
// it's possible that the value is out-of-sync with the rendered options.
// detect such cases and filter out values that no longer has a matching
// option in the DOM.
// [ts] let prevOptions = el._vOptions;
// [ts] let curOptions = el._vOptions = [].map.call(el.options, getValue);
// [ts] if (curOptions.some((o, i) => { return !looseEqual(o, prevOptions[i]); })) {
// trigger change event if
// no matching option found for at least one value
// [ts] let needReset = if (el.multiple) binding.value.some((v) => { return hasNoMatchingOption(v, curOptions); }) else binding.value != binding.oldValue && hasNoMatchingOption(binding.value, curOptions);
// [ts] if (needReset) {
// [ts] trigger(el, "change");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] fn setSelected(el: auto, binding: auto, vm: auto) {
// [ts] actuallySetSelected(el, binding, vm);
// [ts] /* istanbul ignore if */
// [ts] if (isIE || isEdge) {
// [ts] setTimeout(() => {
// [ts] actuallySetSelected(el, binding, vm);
// [ts] }, 0);
// [ts] }
// [ts] }
// [ts] fn actuallySetSelected(el: auto, binding: auto, vm: auto) {
// [ts] let value = binding.value;
// [ts] let isMultiple = el.multiple;
// [ts] if (isMultiple && !Array.isArray(value)) {
// [ts] warn(
// [ts] "<select multiple v-model=\"" + (binding.expression) + "\"> " +
// [ts] "expects an Array value for its binding, but got " + (Object.prototype.toString.call(value).slice(8, -1)),
// [ts] vm
// [ts] );
// [ts] return
// [ts] }
// [ts] let selected, option;
// [ts] let i = 0, l = el.options.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] option = el.options[i];
// [ts] if (isMultiple) {
// [ts] selected = looseIndexOf(value, getValue(option)) > -1;
// [ts] if (option.selected != selected) {
// [ts] option.selected = selected;
// [ts] }
// [ts] } else {
// [ts] if (looseEqual(getValue(option), value)) {
// [ts] if (el.selectedIndex != i) {
// [ts] el.selectedIndex = i;
// [ts] }
// [ts] return
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!isMultiple) {
// [ts] el.selectedIndex = -1;
// [ts] }
// [ts] }
// [ts] fn hasNoMatchingOption(value: auto, options: auto) {
// [ts] return options.every((o) => { return !looseEqual(o, value); })
// [ts] }
// [ts] fn getValue(option: auto) {
// [ts] return if ("_value" == option) option._value else option.value
// [ts] }
// [ts] fn onCompositionStart(e: auto) {
// [ts] e.target.composing = true;
// [ts] }
// [ts] fn onCompositionEnd(e: auto) {
// prevent triggering an input event for no reason
// [ts] if (!e.target.composing) { return }
// [ts] e.target.composing = false;
// [ts] trigger(e.target, "input");
// [ts] }
// [ts] fn trigger(el: auto, type: auto) {
// [ts] let e = document.createEvent("HTMLEvents");
// [ts] e.initEvent(type, true, true);
// [ts] el.dispatchEvent(e);
// [ts] }
// [ts] /*  */
// recursively search for possible transition defined inside the component root
// [ts] fn locateNode(vnode: auto) {
// [ts] return if (vnode.componentInstance && (!vnode.data || !vnode.data.transition)) locateNode(vnode.componentInstance._vnode) else vnode
// [ts] }
// [ts] let show = {
// [ts] bind: (el, ref, vnode) => {
// [ts] let value = ref.value;
// [ts] vnode = locateNode(vnode);
// [ts] let transition$$1 = vnode.data && vnode.data.transition;
// [ts] let originalDisplay = el.__vOriginalDisplay =
// [ts] if (el.style.display == "none") "" else el.style.display;
// [ts] if (value && transition$$1) {
// [ts] vnode.data.show = true;
// [ts] enter(vnode, () => {
// [ts] el.style.display = originalDisplay;
// [ts] });
// [ts] } else {
// [ts] el.style.display = if (value) originalDisplay else "none";
// [ts] }
// [ts] },
// [ts] update: (el, ref, vnode) => {
// [ts] let value = ref.value;
// [ts] let oldValue = ref.oldValue;
// [ts] /* istanbul ignore if */
// [ts] if (!value == !oldValue) { return }
// [ts] vnode = locateNode(vnode);
// [ts] let transition$$1 = vnode.data && vnode.data.transition;
// [ts] if (transition$$1) {
// [ts] vnode.data.show = true;
// [ts] if (value) {
// [ts] enter(vnode, () => {
// [ts] el.style.display = el.__vOriginalDisplay;
// [ts] });
// [ts] } else {
// [ts] leave(vnode, () => {
// [ts] el.style.display = "none";
// [ts] });
// [ts] }
// [ts] } else {
// [ts] el.style.display = if (value) el.__vOriginalDisplay else "none";
// [ts] }
// [ts] },
// [ts] unbind: function unbind (
// [ts] el,
// [ts] binding,
// [ts] vnode,
// [ts] oldVnode,
// [ts] isDestroy
// [ts] ) {
// [ts] if (!isDestroy) {
// [ts] el.style.display = el.__vOriginalDisplay;
// [ts] }
// [ts] }
// [ts] };
// [ts] let platformDirectives = {
// [ts] model: directive,
// [ts] show: show
// [ts] };
// [ts] /*  */
// [ts] let transitionProps = {
// [ts] name: String,
// [ts] appear: Boolean,
// [ts] css: Boolean,
// [ts] mode: String,
// [ts] type: String,
// [ts] enterClass: String,
// [ts] leaveClass: String,
// [ts] enterToClass: String,
// [ts] leaveToClass: String,
// [ts] enterActiveClass: String,
// [ts] leaveActiveClass: String,
// [ts] appearClass: String,
// [ts] appearActiveClass: String,
// [ts] appearToClass: String,
// [ts] duration: [Number, String, Object]
// [ts] };
// in case the child is also an abstract component, e.g. <keep-alive>
// we want to recursively retrieve the real component to be rendered
// [ts] fn getRealChild(vnode: auto) {
// [ts] let compOptions = vnode && vnode.componentOptions;
// [ts] if (compOptions && compOptions.Ctor.options.abstract) {
// [ts] return getRealChild(getFirstComponentChild(compOptions.children))
// [ts] } else {
// [ts] return vnode
// [ts] }
// [ts] }
// [ts] fn extractTransitionData(comp: auto) {
// [ts] let data = {};
// [ts] let options = comp.$options;
// props
// [ts] for (let key in options.propsData) {
// [ts] data[key] = comp[key];
// [ts] }
// events.
// extract listeners and pass them directly to the transition methods
// [ts] let listeners = options._parentListeners;
// [ts] for (let key$1 in listeners) {
// [ts] data[camelize(key$1)] = listeners[key$1];
// [ts] }
// [ts] return data
// [ts] }
// [ts] fn placeholder(h: auto, rawChild: auto) {
// [ts] if ("\\d-keep-alive$".test(rawChild.tag)) {
// [ts] return h("keep-alive", {
// [ts] props: rawChild.componentOptions.propsData
// [ts] })
// [ts] }
// [ts] }
// [ts] fn hasParentTransition(vnode: auto) {
// [ts] while ((vnode = vnode.parent)) {
// [ts] if (vnode.data.transition) {
// [ts] return true
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isSameChild(child: auto, oldChild: auto) {
// [ts] return oldChild.key == child.key && oldChild.tag == child.tag
// [ts] }
// [ts] let isNotTextNode = (c) => { return c.tag || isAsyncPlaceholder(c); };
// [ts] let isVShowDirective = (d) => { return d.name == "show"; };
// [ts] let Transition = {
// [ts] name: "transition",
// [ts] props: transitionProps,
// [ts] abstract: true,
// [ts] render: (h) => {
// [ts] let this$1 = this;
// [ts] let children = _.$slots.default;
// [ts] if (!children) {
// [ts] return
// [ts] }
// filter out text nodes (possible whitespaces)
// [ts] children = children.filter(isNotTextNode);
// [ts] /* istanbul ignore if */
// [ts] if (!children.length) {
// [ts] return
// [ts] }
// warn multiple elements
// [ts] if (children.length > 1) {
// [ts] warn(
// [ts] "<transition> can only be used on a single element. Use " +
// [ts] "<transition-group> for lists.",
// [ts] _.$parent
// [ts] );
// [ts] }
// [ts] let mode = _.mode;
// warn invalid mode
// [ts] if (mode && mode != "in-out" && mode != "out-in"
// [ts] ) {
// [ts] warn(
// [ts] "invalid <transition> mode: " + mode,
// [ts] _.$parent
// [ts] );
// [ts] }
// [ts] let rawChild = children[0];
// if this is a component root node and the component's
// parent container node also has transition, skip.
// [ts] if (hasParentTransition(_.$vnode)) {
// [ts] return rawChild
// [ts] }
// apply transition data to child
// use getRealChild() to ignore abstract components e.g. keep-alive
// [ts] let child = getRealChild(rawChild);
// [ts] /* istanbul ignore if */
// [ts] if (!child) {
// [ts] return rawChild
// [ts] }
// [ts] if (_._leaving) {
// [ts] return placeholder(h, rawChild)
// [ts] }
// ensure a key that is unique to the vnode type and to this transition
// component instance. This key will be used to remove pending leaving nodes
// during entering.
// [ts] let id = "__transition-" + (_._uid) + "-";
// [ts] child.key = if (child.key == null) if (child.isComment) id + "comment" else id + child.tag else if (isPrimitive(child.key)) (if (String(child.key).indexOf(id) == 0) child.key else id + child.key) else child.key;
// [ts] let data = (child.data || (child.data = {})).transition = extractTransitionData(this);
// [ts] let oldRawChild = _._vnode;
// [ts] let oldChild = getRealChild(oldRawChild);
// mark v-show
// so that the transition module can hand over the control to the directive
// [ts] if (child.data.directives && child.data.directives.some(isVShowDirective)) {
// [ts] child.data.show = true;
// [ts] }
// [ts] if (
// [ts] oldChild &&
// [ts] oldChild.data &&
// [ts] !isSameChild(child, oldChild) &&
// [ts] !isAsyncPlaceholder(oldChild) &&
// #6687 component root is a comment node
// [ts] !(oldChild.componentInstance && oldChild.componentInstance._vnode.isComment)
// [ts] ) {
// replace old child transition data with fresh one
// important for dynamic transitions!
// [ts] let oldData = oldChild.data.transition = extend({}, data);
// handle transition mode
// [ts] if (mode == "out-in") {
// return placeholder node and queue update when leave finishes
// [ts] _._leaving = true;
// [ts] mergeVNodeHook(oldData, "afterLeave", () => {
// [ts] this$1._leaving = false;
// [ts] this$1.$forceUpdate();
// [ts] });
// [ts] return placeholder(h, rawChild)
// [ts] } else if (mode == "in-out") {
// [ts] if (isAsyncPlaceholder(child)) {
// [ts] return oldRawChild
// [ts] }
// [ts] let delayedLeave;
// [ts] let performLeave = () => { delayedLeave(); };
// [ts] mergeVNodeHook(data, "afterEnter", performLeave);
// [ts] mergeVNodeHook(data, "enterCancelled", performLeave);
// [ts] mergeVNodeHook(oldData, "delayLeave", (leave) => { delayedLeave = leave; });
// [ts] }
// [ts] }
// [ts] return rawChild
// [ts] }
// [ts] };
// [ts] /*  */
// [ts] let props = extend({
// [ts] tag: String,
// [ts] moveClass: String
// [ts] }, transitionProps);
// [ts] delete props.mode;
// [ts] let TransitionGroup = {
// [ts] props: props,
// [ts] beforeMount: () => {
// [ts] let this$1 = this;
// [ts] let update = _._update;
// [ts] _._update = (vnode, hydrating) => {
// [ts] let restoreActiveInstance = setActiveInstance(this$1);
// force removing pass
// [ts] this$1.__patch__(
// [ts] this$1._vnode,
// [ts] this$1.kept,
// [ts] false, // hydrating
// [ts] true // removeOnly (!important, avoids unnecessary moves)
// [ts] );
// [ts] this$1._vnode = this$1.kept;
// [ts] restoreActiveInstance();
// [ts] update.call(this$1, vnode, hydrating);
// [ts] };
// [ts] },
// [ts] render: (h) => {
// [ts] let tag = _.tag || _.$vnode.data.tag || "span";
// [ts] let map = Object.create(null);
// [ts] let prevChildren = _.prevChildren = _.children;
// [ts] let rawChildren = _.$slots.default || [];
// [ts] let children = _.children = [];
// [ts] let transitionData = extractTransitionData(this);
// [ts] let i = 0;
// [ts] while (i < rawChildren.length) { // [cap-migrate] for-step: i++
// [ts] let c = rawChildren[i];
// [ts] if (c.tag) {
// [ts] if (c.key != null && String(c.key).indexOf("__vlist") != 0) {
// [ts] children.push(c);
// [ts] map[c.key] = c
// [ts] ;(c.data || (c.data = {})).transition = transitionData;
// [ts] } else {
// [ts] let opts = c.componentOptions;
// [ts] let name = if (opts) (opts.Ctor.options.name || opts.tag || "") else c.tag;
// [ts] warn(("<transition-group> children must be keyed: <" + name + ">"));
// [ts] }
// [ts] }
// [ts] }
// [ts] if (prevChildren) {
// [ts] let kept = [];
// [ts] let removed = [];
// [ts] let i$1 = 0;
// [ts] while (i$1 < prevChildren.length) { // [cap-migrate] for-step: i$1++
// [ts] let c$1 = prevChildren[i$1];
// [ts] c$1.data.transition = transitionData;
// [ts] c$1.data.pos = c$1.elm.getBoundingClientRect();
// [ts] if (map[c$1.key]) {
// [ts] kept.push(c$1);
// [ts] } else {
// [ts] removed.push(c$1);
// [ts] }
// [ts] }
// [ts] _.kept = h(tag, null, kept);
// [ts] _.removed = removed;
// [ts] }
// [ts] return h(tag, null, children)
// [ts] },
// [ts] updated: () => {
// [ts] let children = _.prevChildren;
// [ts] let moveClass = _.moveClass || ((_.name || "v") + "-move");
// [ts] if (!children.length || !_.hasMove(children[0].elm, moveClass)) {
// [ts] return
// [ts] }
// we divide the work into three loops to avoid mixing DOM reads and writes
// in each iteration - which helps prevent layout thrashing.
// [ts] children.forEach(callPendingCbs);
// [ts] children.forEach(recordPosition);
// [ts] children.forEach(applyTranslation);
// force reflow to put everything in position
// assign to this to avoid being removed in tree-shaking
// $flow-disable-line
// [ts] _._reflow = document.body.offsetHeight;
// [ts] children.forEach((c) => {
// [ts] if (c.data.moved) {
// [ts] let el = c.elm;
// [ts] let s = el.style;
// [ts] addTransitionClass(el, moveClass);
// [ts] s.transform = s.WebkitTransform = s.transitionDuration = "";
// [ts] el.addEventListener(transitionEndEvent, el._moveCb = (e) => {
// [ts] if (e && e.target != el) {
// [ts] return
// [ts] }
// [ts] if (!e || "transform$".test(e.propertyName)) {
// [ts] el.removeEventListener(transitionEndEvent, cb);
// [ts] el._moveCb = null;
// [ts] removeTransitionClass(el, moveClass);
// [ts] }
// [ts] });
// [ts] }
// [ts] });
// [ts] },
// [ts] methods: {
// [ts] hasMove: (el, moveClass) => {
// [ts] /* istanbul ignore if */
// [ts] if (!hasTransition) {
// [ts] return false
// [ts] }
// [ts] /* istanbul ignore if */
// [ts] if (_._hasMove) {
// [ts] return _._hasMove
// [ts] }
// Detect whether an element with the move class applied has
// CSS transitions. Since the element may be inside an entering
// transition at this very moment, we make a clone of it and remove
// all other transition classes applied to ensure only the move class
// is applied.
// [ts] let clone = el.cloneNode();
// [ts] if (el._transitionClasses) {
// [ts] el._transitionClasses.forEach((cls) => { removeClass(clone, cls); });
// [ts] }
// [ts] addClass(clone, moveClass);
// [ts] clone.style.display = "none";
// [ts] _.$el.appendChild(clone);
// [ts] let info = getTransitionInfo(clone);
// [ts] _.$el.removeChild(clone);
// [ts] return (_._hasMove = info.hasTransform)
// [ts] }
// [ts] }
// [ts] };
// [ts] fn callPendingCbs(c: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (c.elm._moveCb) {
// [ts] c.elm._moveCb();
// [ts] }
// [ts] /* istanbul ignore if */
// [ts] if (c.elm._enterCb) {
// [ts] c.elm._enterCb();
// [ts] }
// [ts] }
// [ts] fn recordPosition(c: auto) {
// [ts] c.data.newPos = c.elm.getBoundingClientRect();
// [ts] }
// [ts] fn applyTranslation(c: auto) {
// [ts] let oldPos = c.data.pos;
// [ts] let newPos = c.data.newPos;
// [ts] let dx = oldPos.left - newPos.left;
// [ts] let dy = oldPos.top - newPos.top;
// [ts] if (dx || dy) {
// [ts] c.data.moved = true;
// [ts] let s = c.elm.style;
// [ts] s.transform = s.WebkitTransform = "translate(" + dx + "px," + dy + "px)";
// [ts] s.transitionDuration = "0s";
// [ts] }
// [ts] }
// [ts] let platformComponents = {
// [ts] Transition: Transition,
// [ts] TransitionGroup: TransitionGroup
// [ts] };
// [ts] /*  */
// install platform specific utils
// [ts] Vue.config.mustUseProp = mustUseProp;
// [ts] Vue.config.isReservedTag = isReservedTag;
// [ts] Vue.config.isReservedAttr = isReservedAttr;
// [ts] Vue.config.getTagNamespace = getTagNamespace;
// [ts] Vue.config.isUnknownElement = isUnknownElement;
// install platform runtime directives & components
// [ts] extend(Vue.options.directives, platformDirectives);
// [ts] extend(Vue.options.components, platformComponents);
// install platform patch function
// [ts] Vue.prototype.__patch__ = if (inBrowser) patch else noop;
// public mount method
// [ts] Vue.prototype.$mount = function (
// [ts] el,
// [ts] hydrating
// [ts] ) {
// [ts] el = if (el && inBrowser) query(el) else null;
// [ts] return mountComponent(this, el, hydrating)
// [ts] };
// devtools global hook
// [ts] /* istanbul ignore next */
// [ts] if (inBrowser) {
// [ts] setTimeout(() => {
// [ts] if (config.devtools) {
// [ts] if (devtools) {
// [ts] devtools.emit("init", Vue);
// [ts] } else {
// [ts] console[console.info ? "info" : "log"](
// [ts] "Download the Vue Devtools extension for a better development experience:\n" +
// [ts] "https://github.com/vuejs/vue-devtools"
// [ts] );
// [ts] }
// [ts] }
// [ts] if (config.productionTip != false &&
// [ts] @typeName(console) != "undefined"
// [ts] ) {
// [ts] console[console.info ? "info" : "log"](
// [ts] "You are running Vue in development mode.\n" +
// [ts] "Make sure to turn on production mode when deploying for production.\n" +
// [ts] "See more tips at https://vuejs.org/guide/deployment.html"
// [ts] );
// [ts] }
// [ts] }, 0);
// [ts] }
// [ts] /*  */
// [ts] let defaultTagRE = "\\{\\{((?:.|\\r?\\n)+?)\\}\\}";
// [ts] let regexEscapeRE = "[-.*+?^${}()|[\\]\\/\\\\]";
// [ts] let buildRegex = cached((delimiters) => {
// [ts] let open = delimiters[0].replace(regexEscapeRE, "\\$&");
// [ts] let close = delimiters[1].replace(regexEscapeRE, "\\$&");
// [ts] return RegExp.init(open + "((?:.|\\n)+?)" + close, "g")
// [ts] });
// [ts] fn parseText(text: auto, delimiters: auto) {
// [ts] let tagRE = if (delimiters) buildRegex(delimiters) else defaultTagRE;
// [ts] if (!tagRE.test(text)) {
// [ts] return
// [ts] }
// [ts] let tokens = [];
// [ts] let rawTokens = [];
// [ts] let lastIndex = tagRE.lastIndex = 0;
// [ts] let match_val, index, tokenValue;
// [ts] while ((match_val = tagRE.exec(text))) {
// [ts] index = match_val.index;
// push text token
// [ts] if (index > lastIndex) {
// [ts] rawTokens.push(tokenValue = text.slice(lastIndex, index));
// [ts] tokens.push(JSON.stringify(tokenValue));
// [ts] }
// tag token
// [ts] let exp = parseFilters(match_val[1].trim());
// [ts] tokens.push(("_s(" + exp + ")"));
// [ts] rawTokens.push({ "@binding": exp });
// [ts] lastIndex = index + match_val[0].length;
// [ts] }
// [ts] if (lastIndex < text.length) {
// [ts] rawTokens.push(tokenValue = text.slice(lastIndex));
// [ts] tokens.push(JSON.stringify(tokenValue));
// [ts] }
// [ts] return {
// [ts] expression: tokens.join("+"),
// [ts] tokens: rawTokens
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn transformNode(el: auto, options: auto) {
// [ts] let warn = options.warn || baseWarn;
// [ts] let staticClass = getAndRemoveAttr(el, "class");
// [ts] if (staticClass) {
// [ts] let res = parseText(staticClass, options.delimiters);
// [ts] if (res) {
// [ts] warn(
// [ts] "class=\"" + staticClass + "\": " +
// [ts] "Interpolation inside attributes has been removed. " +
// [ts] "Use v-bind or the colon shorthand instead. For example, " +
// [ts] "instead of <div class=\"{{ val }}\">, use <div :class=\"val\">.",
// [ts] el.rawAttrsMap["class"]
// [ts] );
// [ts] }
// [ts] }
// [ts] if (staticClass) {
// [ts] el.staticClass = JSON.stringify(staticClass);
// [ts] }
// [ts] let classBinding = getBindingAttr(el, "class", false /* getStatic */);
// [ts] if (classBinding) {
// [ts] el.classBinding = classBinding;
// [ts] }
// [ts] }
// [ts] fn genData(el: auto) {
// [ts] let data = "";
// [ts] if (el.staticClass) {
// [ts] data += "staticClass:" + (el.staticClass) + ",";
// [ts] }
// [ts] if (el.classBinding) {
// [ts] data += "class:" + (el.classBinding) + ",";
// [ts] }
// [ts] return data
// [ts] }
// [ts] let klass$1 = {
// [ts] staticKeys: ["staticClass"],
// [ts] transformNode: transformNode,
// [ts] genData: genData
// [ts] };
// [ts] /*  */
// [ts] (el, options) => {
// [ts] let warn = options.warn || baseWarn;
// [ts] let staticStyle = getAndRemoveAttr(el, "style");
// [ts] if (staticStyle) {
// [ts] /* istanbul ignore if */
// [ts] {
// [ts] let res = parseText(staticStyle, options.delimiters);
// [ts] if (res) {
// [ts] warn(
// [ts] "style=\"" + staticStyle + "\": " +
// [ts] "Interpolation inside attributes has been removed. " +
// [ts] "Use v-bind or the colon shorthand instead. For example, " +
// [ts] "instead of <div style=\"{{ val }}\">, use <div :style=\"val\">.",
// [ts] el.rawAttrsMap["style"]
// [ts] );
// [ts] }
// [ts] }
// [ts] el.staticStyle = JSON.stringify(parseStyleText(staticStyle));
// [ts] }
// [ts] let styleBinding = getBindingAttr(el, "style", false /* getStatic */);
// [ts] if (styleBinding) {
// [ts] el.styleBinding = styleBinding;
// [ts] }
// [ts] }
// [ts] (el) => {
// [ts] let data = "";
// [ts] if (el.staticStyle) {
// [ts] data += "staticStyle:" + (el.staticStyle) + ",";
// [ts] }
// [ts] if (el.styleBinding) {
// [ts] data += "style:(" + (el.styleBinding) + "),";
// [ts] }
// [ts] return data
// [ts] }
// [ts] let style$1 = {
// [ts] staticKeys: ["staticStyle"],
// [ts] transformNode: transformNode$1,
// [ts] genData: genData$1
// [ts] };
// [ts] /*  */
// [ts] let decoder;
// [ts] let he = {
// [ts] decode: (html) => {
// [ts] decoder = decoder || document.createElement("div");
// [ts] decoder.innerHTML = html;
// [ts] return decoder.textContent
// [ts] }
// [ts] };
// [ts] /*  */
// [ts] let isUnaryTag = makeMap(
// [ts] "area,base,br,col,embed,frame,hr,img,input,isindex,keygen," +
// [ts] "link,meta,param,source,track,wbr"
// [ts] );
// Elements that you can, intentionally, leave open
// (and which close themselves)
// [ts] let canBeLeftOpenTag = makeMap(
// [ts] "colgroup,dd,dt,li,options,p,td,tfoot,th,thead,tr,source"
// [ts] );
// HTML5 tags https://html.spec.whatwg.org/multipage/indices.html#elements-3
// Phrasing Content https://html.spec.whatwg.org/multipage/dom.html#phrasing-content
// [ts] let isNonPhrasingTag = makeMap(
// [ts] "address,article,aside,base,blockquote,body,caption,col,colgroup,dd," +
// [ts] "details,dialog,div,dl,dt,fieldset,figcaption,figure,footer,form," +
// [ts] "h1,h2,h3,h4,h5,h6,head,header,hgroup,hr,html,legend,li,menuitem,meta," +
// [ts] "optgroup,option,param,rp,rt,source,style,summary,tbody,td,tfoot,th,thead," +
// [ts] "title,tr,track"
// [ts] );
// [ts] /**
// [ts] * Not type-checking this file because it's mostly vendor code.
// [ts] */
// Regular Expressions for parsing tags and attributes
// [ts] let attribute = "^\\s*([^\\s\"'<>\\/=]+)(?:\\s*(=)\\s*(?:\"([^\"]*)\"+|'([^']*)'+|([^\\s\"'=<>`]+)))?";
// [ts] let dynamicArgAttribute = "^\\s*((?:v-[\\w-]+:|@|:|#)\\[[^=]+?\\][^\\s\"'<>\\/=]*)(?:\\s*(=)\\s*(?:\"([^\"]*)\"+|'([^']*)'+|([^\\s\"'=<>`]+)))?";
// [ts] let ncname = "[a-zA-Z_][\\-\\.0-9_a-zA-Z" + (unicodeRegExp.source) + "]*";
// [ts] let qnameCapture = "((?:" + ncname + "\\:)?" + ncname + ")";
// [ts] let startTagOpen = RegExp.init(("^<" + qnameCapture));
// [ts] let startTagClose = "^\\s*(\\/?)>";
// [ts] let endTag = RegExp.init(("^<\\/" + qnameCapture + "[^>]*>"));
// [ts] let doctype = "^<!DOCTYPE [^>]+>";
// #7298: escape - to avoid being passed as HTML comment when inlined in page
// [ts] let comment = "^<!\\--";
// [ts] let conditionalComment = "^<!\\[";
// Special Elements (can contain anything)
// [ts] let isPlainTextElement = makeMap("script,style,textarea", true);
// [ts] let reCache = {};
// [ts] let decodingMap = {
// [ts] "&lt;": "<",
// [ts] "&gt;": ">",
// [ts] "&quot;": "\"",
// [ts] "&amp;": "&",
// [ts] "&#10;": "\n",
// [ts] "&#9;": "\t",
// [ts] "&#39;": "'"
// [ts] };
// [ts] let encodedAttr = "&(?:lt|gt|quot|amp|#39);";
// [ts] let encodedAttrWithNewLines = "&(?:lt|gt|quot|amp|#39|#10|#9);";
// #5992
// [ts] let isIgnoreNewlineTag = makeMap("pre,textarea", true);
// [ts] let shouldIgnoreFirstNewline = (tag, html) => { return tag && isIgnoreNewlineTag(tag) && html[0] == "\n"; };
// [ts] fn decodeAttr(value: auto, shouldDecodeNewlines: auto) {
// [ts] let re = if (shouldDecodeNewlines) encodedAttrWithNewLines else encodedAttr;
// [ts] return value.replace(re, (match_val) => { return decodingMap[match_val]; })
// [ts] }
// [ts] fn parseHTML(html: auto, options: auto) {
// [ts] let stack = [];
// [ts] let expectHTML = options.expectHTML;
// [ts] let isUnaryTag$$1 = options.isUnaryTag || no;
// [ts] let canBeLeftOpenTag$$1 = options.canBeLeftOpenTag || no;
// [ts] let index = 0;
// [ts] let last, lastTag;
// [ts] while (html) {
// [ts] last = html;
// Make sure we're not in a plaintext content element like script/style
// [ts] if (!lastTag || !isPlainTextElement(lastTag)) {
// [ts] let textEnd = html.indexOf("<");
// [ts] if (textEnd == 0) {
// Comment:
// [ts] if (comment.test(html)) {
// [ts] let commentEnd = html.indexOf("-->");
// [ts] if (commentEnd >= 0) {
// [ts] if (options.shouldKeepComment) {
// [ts] options.comment(html.substring(4, commentEnd), index, index + commentEnd + 3);
// [ts] }
// [ts] advance(commentEnd + 3);
// [ts] continue
// [ts] }
// [ts] }
// http://en.wikipedia.org/wiki/Conditional_comment#Downlevel-revealed_conditional_comment
// [ts] if (conditionalComment.test(html)) {
// [ts] let conditionalEnd = html.indexOf("]>");
// [ts] if (conditionalEnd >= 0) {
// [ts] advance(conditionalEnd + 2);
// [ts] continue
// [ts] }
// [ts] }
// Doctype:
// [ts] let doctypeMatch = html.match(doctype);
// [ts] if (doctypeMatch) {
// [ts] advance(doctypeMatch[0].length);
// [ts] continue
// [ts] }
// End tag:
// [ts] let endTagMatch = html.match(endTag);
// [ts] if (endTagMatch) {
// [ts] let curIndex = index;
// [ts] advance(endTagMatch[0].length);
// [ts] parseEndTag(endTagMatch[1], curIndex, index);
// [ts] continue
// [ts] }
// Start tag:
// [ts] let startTagMatch = parseStartTag();
// [ts] if (startTagMatch) {
// [ts] handleStartTag(startTagMatch);
// [ts] if (shouldIgnoreFirstNewline(startTagMatch.tagName, html)) {
// [ts] advance(1);
// [ts] }
// [ts] continue
// [ts] }
// [ts] }
// [ts] let text = (void 0), rest = (void 0), next = (void 0);
// [ts] if (textEnd >= 0) {
// [ts] rest = html.slice(textEnd);
// [ts] while (
// [ts] !endTag.test(rest) &&
// [ts] !startTagOpen.test(rest) &&
// [ts] !comment.test(rest) &&
// [ts] !conditionalComment.test(rest)
// [ts] ) {
// < in plain text, be forgiving and treat it as text
// [ts] next = rest.indexOf("<", 1);
// [ts] if (next < 0) { break }
// [ts] textEnd += next;
// [ts] rest = html.slice(textEnd);
// [ts] }
// [ts] text = html.substring(0, textEnd);
// [ts] }
// [ts] if (textEnd < 0) {
// [ts] text = html;
// [ts] }
// [ts] if (text) {
// [ts] advance(text.length);
// [ts] }
// [ts] if (options.chars && text) {
// [ts] options.chars(text, index - text.length, index);
// [ts] }
// [ts] } else {
// [ts] let endTagLength = 0;
// [ts] let stackedTag = lastTag.toLowerCase();
// [ts] let reStackedTag = reCache[stackedTag] || (reCache[stackedTag] = RegExp.init("([\\s\\S]*?)(</" + stackedTag + "[^>]*>)", "i"));
// [ts] let rest$1 = html.replace(reStackedTag, (all, text, endTag) => {
// [ts] endTagLength = endTag.length;
// [ts] if (!isPlainTextElement(stackedTag) && stackedTag != "noscript") {
// [ts] text = text
// [ts] .replace("<!\\--([\\s\\S]*?)-->", "$1") // #7298
// [ts] .replace("<!\\[CDATA\\[([\\s\\S]*?)]]>", "$1");
// [ts] }
// [ts] if (shouldIgnoreFirstNewline(stackedTag, text)) {
// [ts] text = text.slice(1);
// [ts] }
// [ts] if (options.chars) {
// [ts] options.chars(text);
// [ts] }
// [ts] return ""
// [ts] });
// [ts] index += html.length - rest$1.length;
// [ts] html = rest$1;
// [ts] parseEndTag(stackedTag, index - endTagLength, index);
// [ts] }
// [ts] if (html == last) {
// [ts] options.chars && options.chars(html);
// [ts] if (!stack.length && options.warn) {
// [ts] options.warn(("Mal-formatted tag at end of template: \"" + html + "\""), { start: index + html.length });
// [ts] }
// [ts] break
// [ts] }
// [ts] }
// Clean up any remaining tags
// [ts] parseEndTag();
// [ts] fn advance(n: auto) {
// [ts] index += n;
// [ts] html = html.substring(n);
// [ts] }
// [ts] fn parseStartTag() {
// [ts] let start = html.match(startTagOpen);
// [ts] if (start) {
// [ts] let match_val = {
// [ts] tagName: start[1],
// [ts] attrs: [],
// [ts] start: index
// [ts] };
// [ts] advance(start[0].length);
// [ts] let end, attr;
// [ts] while (!(end = html.match(startTagClose)) && (attr = html.match(dynamicArgAttribute) || html.match(attribute))) {
// [ts] attr.start = index;
// [ts] advance(attr[0].length);
// [ts] attr.end = index;
// [ts] match_val.attrs.push(attr);
// [ts] }
// [ts] if (end) {
// [ts] match_val.unarySlash = end[1];
// [ts] advance(end[0].length);
// [ts] match_val.end = index;
// [ts] return match_val
// [ts] }
// [ts] }
// [ts] }
// [ts] fn handleStartTag(match: auto) {
// [ts] let tagName = match_val.tagName;
// [ts] let unarySlash = match_val.unarySlash;
// [ts] if (expectHTML) {
// [ts] if (lastTag == "p" && isNonPhrasingTag(tagName)) {
// [ts] parseEndTag(lastTag);
// [ts] }
// [ts] if (canBeLeftOpenTag$$1(tagName) && lastTag == tagName) {
// [ts] parseEndTag(tagName);
// [ts] }
// [ts] }
// [ts] let unary = isUnaryTag$$1(tagName) || !!unarySlash;
// [ts] let l = match_val.attrs.length;
// [ts] let attrs = Array.init(l);
// [ts] let i = 0;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] let args = match_val.attrs[i];
// [ts] let value = args[3] || args[4] || args[5] || "";
// [ts] let shouldDecodeNewlines = if (tagName == "a" && args[1] == "href") options.shouldDecodeNewlinesForHref else options.shouldDecodeNewlines;
// [ts] attrs[i] = {
// [ts] name: args[1],
// [ts] value: decodeAttr(value, shouldDecodeNewlines)
// [ts] };
// [ts] if (options.outputSourceRange) {
// [ts] attrs[i].start = args.start + args[0].match("^\\s*").length;
// [ts] attrs[i].end = args.end;
// [ts] }
// [ts] }
// [ts] if (!unary) {
// [ts] stack.push({ tag: tagName, lowerCasedTag: tagName.toLowerCase(), attrs: attrs, start: match_val.start, end: match_val.end });
// [ts] lastTag = tagName;
// [ts] }
// [ts] if (options.start) {
// [ts] options.start(tagName, attrs, unary, match_val.start, match_val.end);
// [ts] }
// [ts] }
// [ts] fn parseEndTag(tagName: auto, start: auto, end: auto) {
// [ts] let pos, lowerCasedTagName;
// [ts] if (start == null) { start = index; }
// [ts] if (end == null) { end = index; }
// Find the closest opened tag of the same type
// [ts] if (tagName) {
// [ts] lowerCasedTagName = tagName.toLowerCase();
// [ts] pos = stack.length - 1;
// [ts] while (pos >= 0) { // [cap-migrate] for-step: pos--
// [ts] if (stack[pos].lowerCasedTag == lowerCasedTagName) {
// [ts] break
// [ts] }
// [ts] }
// [ts] } else {
// If no tag name is provided, clean shop
// [ts] pos = 0;
// [ts] }
// [ts] if (pos >= 0) {
// Close all the open elements, up the stack
// [ts] let i = stack.length - 1;
// [ts] while (i >= pos) { // [cap-migrate] for-step: i--
// [ts] if (i > pos || !tagName &&
// [ts] options.warn
// [ts] ) {
// [ts] options.warn(
// [ts] ("tag <" + (stack[i].tag) + "> has no matching end tag."),
// [ts] { start: stack[i].start, end: stack[i].end }
// [ts] );
// [ts] }
// [ts] if (options.end) {
// [ts] options.end(stack[i].tag, start, end);
// [ts] }
// [ts] }
// Remove the open elements from the stack
// [ts] stack.length = pos;
// [ts] lastTag = pos && stack[pos - 1].tag;
// [ts] } else if (lowerCasedTagName == "br") {
// [ts] if (options.start) {
// [ts] options.start(tagName, [], true, start, end);
// [ts] }
// [ts] } else if (lowerCasedTagName == "p") {
// [ts] if (options.start) {
// [ts] options.start(tagName, [], false, start, end);
// [ts] }
// [ts] if (options.end) {
// [ts] options.end(tagName, start, end);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let onRE = "^@|^v-on:";
// [ts] let dirRE = "^v-|^@|^:|^#";
// [ts] let forAliasRE = "([\\s\\S]*?)\\s+(?:in|of)\\s+([\\s\\S]*)";
// [ts] let forIteratorRE = ",([^,\\}\\]]*)(?:,([^,\\}\\]]*))?$";
// [ts] let stripParensRE = "^\\(|\\)$";
// [ts] let dynamicArgRE = "^\\[.*\\]$";
// [ts] let argRE = ":(.*)$";
// [ts] let bindRE = "^:|^\\.|^v-bind:";
// [ts] let modifierRE = "\\.[^.\\]]+(?=[^\\]]*$)";
// [ts] let slotRE = "^v-slot(:|$)|^#";
// [ts] let lineBreakRE = "[\\r\\n]";
// [ts] let whitespaceRE$1 = "[ \\f\\t\\r\\n]+";
// [ts] let invalidAttributeRE = "[\\s\"'<>\\/=]";
// [ts] let decodeHTMLCached = cached(he.decode);
// [ts] let emptySlotScopeToken = "_empty_";
// configurable state
// [ts] let warn$2;
// [ts] let delimiters;
// [ts] let transforms;
// [ts] let preTransforms;
// [ts] let postTransforms;
// [ts] let platformIsPreTag;
// [ts] let platformMustUseProp;
// [ts] let platformGetTagNamespace;
// [ts] let maybeComponent;
// [ts] fn createASTElement(tag: auto, attrs: auto, parent: auto) {
// [ts] return {
// [ts] type: 1,
// [ts] tag: tag,
// [ts] attrsList: attrs,
// [ts] attrsMap: makeAttrsMap(attrs),
// [ts] rawAttrsMap: {},
// [ts] parent: parent,
// [ts] children: []
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Convert HTML string to AST.
// [ts] */
// [ts] fn parse(template: auto, options: auto) {
// [ts] warn$2 = options.warn || baseWarn;
// [ts] platformIsPreTag = options.isPreTag || no;
// [ts] platformMustUseProp = options.mustUseProp || no;
// [ts] platformGetTagNamespace = options.getTagNamespace || no;
// [ts] let isReservedTag = options.isReservedTag || no;
// [ts] maybeComponent = (el) => { return !!(
// [ts] el.component ||
// [ts] el.attrsMap[":is"] ||
// [ts] el.attrsMap["v-bind:is"] ||
// [ts] !(if (el.attrsMap.is) isReservedTag(el.attrsMap.is) else isReservedTag(el.tag))
// [ts] ); };
// [ts] transforms = pluckModuleFunction(options.modules, "transformNode");
// [ts] preTransforms = pluckModuleFunction(options.modules, "preTransformNode");
// [ts] postTransforms = pluckModuleFunction(options.modules, "postTransformNode");
// [ts] delimiters = options.delimiters;
// [ts] let stack = [];
// [ts] let preserveWhitespace = options.preserveWhitespace != false;
// [ts] let whitespaceOption = options.whitespace;
// [ts] let root;
// [ts] let currentParent;
// [ts] let inVPre = false;
// [ts] let inPre = false;
// [ts] let warned = false;
// [ts] fn warnOnce(msg: auto, range: auto) {
// [ts] if (!warned) {
// [ts] warned = true;
// [ts] warn$2(msg, range);
// [ts] }
// [ts] }
// [ts] fn closeElement(element: auto) {
// [ts] trimEndingWhitespace(element);
// [ts] if (!inVPre && !element.processed) {
// [ts] element = processElement(element, options);
// [ts] }
// tree management
// [ts] if (!stack.length && element != root) {
// allow root elements with v-if, v-else-if and v-else
// [ts] if (root.if && (element.elseif || element.else)) {
// [ts] {
// [ts] checkRootConstraints(element);
// [ts] }
// [ts] addIfCondition(root, {
// [ts] exp: element.elseif,
// [ts] block: element
// [ts] });
// [ts] } else {
// [ts] warnOnce(
// [ts] "Component template should contain exactly one root element. " +
// [ts] "If you are using v-if on multiple elements, " +
// [ts] "use v-else-if to chain them instead.",
// [ts] { start: element.start }
// [ts] );
// [ts] }
// [ts] }
// [ts] if (currentParent && !element.forbidden) {
// [ts] if (element.elseif || element.else) {
// [ts] processIfConditions(element, currentParent);
// [ts] } else {
// [ts] if (element.slotScope) {
// scoped slot
// keep it in the children list so that v-else(-if) conditions can
// find it as the prev node.
// [ts] let name = element.slotTarget || "\"default\""
// [ts] ;(currentParent.scopedSlots || (currentParent.scopedSlots = {}))[name] = element;
// [ts] }
// [ts] currentParent.children.push(element);
// [ts] element.parent = currentParent;
// [ts] }
// [ts] }
// final children cleanup
// filter out scoped slots
// [ts] element.children = element.children.filter((c) => { return !(c).slotScope; });
// remove trailing whitespace node again
// [ts] trimEndingWhitespace(element);
// check pre state
// [ts] if (element.pre) {
// [ts] inVPre = false;
// [ts] }
// [ts] if (platformIsPreTag(element.tag)) {
// [ts] inPre = false;
// [ts] }
// apply post-transforms
// [ts] let i = 0;
// [ts] while (i < postTransforms.length) { // [cap-migrate] for-step: i++
// [ts] postTransforms[i](element, options);
// [ts] }
// [ts] }
// [ts] fn trimEndingWhitespace(el: auto) {
// remove trailing whitespace node
// [ts] if (!inPre) {
// [ts] let lastNode;
// [ts] while (
// [ts] (lastNode = el.children[el.children.length - 1]) &&
// [ts] lastNode.type == 3 &&
// [ts] lastNode.text == " "
// [ts] ) {
// [ts] el.children.pop();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn checkRootConstraints(el: auto) {
// [ts] if (el.tag == "slot" || el.tag == "template") {
// [ts] warnOnce(
// [ts] "Cannot use <" + (el.tag) + "> as component root element because it may " +
// [ts] "contain multiple nodes.",
// [ts] { start: el.start }
// [ts] );
// [ts] }
// [ts] if (el.attrsMap.hasOwnProperty("v-for")) {
// [ts] warnOnce(
// [ts] "Cannot use v-for on stateful component root element because " +
// [ts] "it renders multiple elements.",
// [ts] el.rawAttrsMap["v-for"]
// [ts] );
// [ts] }
// [ts] }
// [ts] parseHTML(template, {
// [ts] warn: warn$2,
// [ts] expectHTML: options.expectHTML,
// [ts] isUnaryTag: options.isUnaryTag,
// [ts] canBeLeftOpenTag: options.canBeLeftOpenTag,
// [ts] shouldDecodeNewlines: options.shouldDecodeNewlines,
// [ts] shouldDecodeNewlinesForHref: options.shouldDecodeNewlinesForHref,
// [ts] shouldKeepComment: options.comments,
// [ts] outputSourceRange: options.outputSourceRange,
// [ts] start: (tag, attrs, unary, start$1, end) => {
// check namespace.
// inherit parent ns if there is one
// [ts] let ns = (currentParent && currentParent.ns) || platformGetTagNamespace(tag);
// handle IE svg bug
// [ts] /* istanbul ignore if */
// [ts] if (isIE && ns == "svg") {
// [ts] attrs = guardIESVGBug(attrs);
// [ts] }
// [ts] let element = createASTElement(tag, attrs, currentParent);
// [ts] if (ns) {
// [ts] element.ns = ns;
// [ts] }
// [ts] {
// [ts] if (options.outputSourceRange) {
// [ts] element.start = start$1;
// [ts] element.end = end;
// [ts] element.rawAttrsMap = element.attrsList.reduce((cumulated, attr) => {
// [ts] cumulated[attr.name] = attr;
// [ts] return cumulated
// [ts] }, {});
// [ts] }
// [ts] attrs.forEach((attr) => {
// [ts] if (invalidAttributeRE.test(attr.name)) {
// [ts] warn$2(
// [ts] "Invalid dynamic argument expression: attribute names cannot contain " +
// [ts] "spaces, quotes, <, >, / or =.",
// [ts] {
// [ts] start: attr.start + attr.name.indexOf("["),
// [ts] end: attr.start + attr.name.length
// [ts] }
// [ts] );
// [ts] }
// [ts] });
// [ts] }
// [ts] if (isForbiddenTag(element) && !isServerRendering()) {
// [ts] element.forbidden = true;
// [ts] warn$2(
// [ts] "Templates should only be responsible for mapping the state to the " +
// [ts] "UI. Avoid placing tags with side-effects in your templates, such as " +
// [ts] "<" + tag + ">" + ", as they will not be parsed.",
// [ts] { start: element.start }
// [ts] );
// [ts] }
// apply pre-transforms
// [ts] let i = 0;
// [ts] while (i < preTransforms.length) { // [cap-migrate] for-step: i++
// [ts] element = preTransforms[i](element, options) || element;
// [ts] }
// [ts] if (!inVPre) {
// [ts] processPre(element);
// [ts] if (element.pre) {
// [ts] inVPre = true;
// [ts] }
// [ts] }
// [ts] if (platformIsPreTag(element.tag)) {
// [ts] inPre = true;
// [ts] }
// [ts] if (inVPre) {
// [ts] processRawAttrs(element);
// [ts] } else if (!element.processed) {
// structural directives
// [ts] processFor(element);
// [ts] processIf(element);
// [ts] processOnce(element);
// [ts] }
// [ts] if (!root) {
// [ts] root = element;
// [ts] {
// [ts] checkRootConstraints(root);
// [ts] }
// [ts] }
// [ts] if (!unary) {
// [ts] currentParent = element;
// [ts] stack.push(element);
// [ts] } else {
// [ts] closeElement(element);
// [ts] }
// [ts] },
// [ts] end: (tag, start, end$1) => {
// [ts] let element = stack[stack.length - 1];
// pop stack
// [ts] stack.length -= 1;
// [ts] currentParent = stack[stack.length - 1];
// [ts] if (options.outputSourceRange) {
// [ts] element.end = end$1;
// [ts] }
// [ts] closeElement(element);
// [ts] },
// [ts] chars: (text, start, end) => {
// [ts] if (!currentParent) {
// [ts] {
// [ts] if (text == template) {
// [ts] warnOnce(
// [ts] "Component template requires a root element, rather than just text.",
// [ts] { start: start }
// [ts] );
// [ts] } else if ((text = text.trim())) {
// [ts] warnOnce(
// [ts] ("text \"" + text + "\" outside root element will be ignored."),
// [ts] { start: start }
// [ts] );
// [ts] }
// [ts] }
// [ts] return
// [ts] }
// IE textarea placeholder bug
// [ts] /* istanbul ignore if */
// [ts] if (isIE &&
// [ts] currentParent.tag == "textarea" &&
// [ts] currentParent.attrsMap.placeholder == text
// [ts] ) {
// [ts] return
// [ts] }
// [ts] let children = currentParent.children;
// [ts] if (inPre || text.trim()) {
// [ts] text = if (isTextTag(currentParent)) text else decodeHTMLCached(text);
// [ts] } else if (!children.length) {
// remove the whitespace-only node right after an opening tag
// [ts] text = "";
// [ts] } else if (whitespaceOption) {
// [ts] if (whitespaceOption == "condense") {
// in condense mode, remove the whitespace node if it contains
// line break, otherwise condense to a single space
// [ts] text = if (lineBreakRE.test(text)) "" else " ";
// [ts] } else {
// [ts] text = " ";
// [ts] }
// [ts] } else {
// [ts] text = if (preserveWhitespace) " " else "";
// [ts] }
// [ts] if (text) {
// [ts] if (!inPre && whitespaceOption == "condense") {
// condense consecutive whitespaces into single space
// [ts] text = text.replace(whitespaceRE$1, " ");
// [ts] }
// [ts] let res;
// [ts] let child;
// [ts] if (!inVPre && text != " " && (res = parseText(text, delimiters))) {
// [ts] child = {
// [ts] type: 2,
// [ts] expression: res.expression,
// [ts] tokens: res.tokens,
// [ts] text: text
// [ts] };
// [ts] } else if (text != " " || !children.length || children[children.length - 1].text != " ") {
// [ts] child = {
// [ts] type: 3,
// [ts] text: text
// [ts] };
// [ts] }
// [ts] if (child) {
// [ts] if (options.outputSourceRange) {
// [ts] child.start = start;
// [ts] child.end = end;
// [ts] }
// [ts] children.push(child);
// [ts] }
// [ts] }
// [ts] },
// [ts] comment: (text, start, end) => {
// adding anything as a sibling to the root node is forbidden
// comments should still be allowed, but ignored
// [ts] if (currentParent) {
// [ts] let child = {
// [ts] type: 3,
// [ts] text: text,
// [ts] isComment: true
// [ts] };
// [ts] if (options.outputSourceRange) {
// [ts] child.start = start;
// [ts] child.end = end;
// [ts] }
// [ts] currentParent.children.push(child);
// [ts] }
// [ts] }
// [ts] });
// [ts] return root
// [ts] }
// [ts] fn processPre(el: auto) {
// [ts] if (getAndRemoveAttr(el, "v-pre") != null) {
// [ts] el.pre = true;
// [ts] }
// [ts] }
// [ts] fn processRawAttrs(el: auto) {
// [ts] let list = el.attrsList;
// [ts] let len = list.length;
// [ts] if (len) {
// [ts] let attrs = el.attrs = Array.init(len);
// [ts] let i = 0;
// [ts] while (i < len) { // [cap-migrate] for-step: i++
// [ts] attrs[i] = {
// [ts] name: list[i].name,
// [ts] value: JSON.stringify(list[i].value)
// [ts] };
// [ts] if (list[i].start != null) {
// [ts] attrs[i].start = list[i].start;
// [ts] attrs[i].end = list[i].end;
// [ts] }
// [ts] }
// [ts] } else if (!el.pre) {
// non root node in pre blocks with no attributes
// [ts] el.plain = true;
// [ts] }
// [ts] }
// [ts] fn processElement(element: auto, options: auto) {
// [ts] processKey(element);
// determine whether this is a plain element after
// removing structural attributes
// [ts] element.plain = (
// [ts] !element.key &&
// [ts] !element.scopedSlots &&
// [ts] !element.attrsList.length
// [ts] );
// [ts] processRef(element);
// [ts] processSlotContent(element);
// [ts] processSlotOutlet(element);
// [ts] processComponent(element);
// [ts] let i = 0;
// [ts] while (i < transforms.length) { // [cap-migrate] for-step: i++
// [ts] element = transforms[i](element, options) || element;
// [ts] }
// [ts] processAttrs(element);
// [ts] return element
// [ts] }
// [ts] fn processKey(el: auto) {
// [ts] let exp = getBindingAttr(el, "key");
// [ts] if (exp) {
// [ts] {
// [ts] if (el.tag == "template") {
// [ts] warn$2(
// [ts] "<template> cannot be keyed. Place the key on real elements instead.",
// [ts] getRawBindingAttr(el, "key")
// [ts] );
// [ts] }
// [ts] if (el.for) {
// [ts] let iterator = el.iterator2 || el.iterator1;
// [ts] let parent = el.parent;
// [ts] if (iterator && iterator == exp && parent && parent.tag == "transition-group") {
// [ts] warn$2(
// [ts] "Do not use v-for index as key on <transition-group> children, " +
// [ts] "this is the same as not using keys.",
// [ts] getRawBindingAttr(el, "key"),
// [ts] true /* tip */
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] el.key = exp;
// [ts] }
// [ts] }
// [ts] fn processRef(el: auto) {
// [ts] let ref = getBindingAttr(el, "ref");
// [ts] if (ref) {
// [ts] el.ref = ref;
// [ts] el.refInFor = checkInFor(el);
// [ts] }
// [ts] }
// [ts] fn processFor(el: auto) {
// [ts] let exp;
// [ts] if ((exp = getAndRemoveAttr(el, "v-for"))) {
// [ts] let res = parseFor(exp);
// [ts] if (res) {
// [ts] extend(el, res);
// [ts] } else {
// [ts] warn$2(
// [ts] ("Invalid v-for expression: " + exp),
// [ts] el.rawAttrsMap["v-for"]
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] fn parseFor(exp: auto) {
// [ts] let inMatch = exp.match(forAliasRE);
// [ts] if (!inMatch) { return }
// [ts] let res = {};
// [ts] res.for = inMatch[2].trim();
// [ts] let alias = inMatch[1].trim().replace(stripParensRE, "");
// [ts] let iteratorMatch = alias.match(forIteratorRE);
// [ts] if (iteratorMatch) {
// [ts] res.alias = alias.replace(forIteratorRE, "").trim();
// [ts] res.iterator1 = iteratorMatch[1].trim();
// [ts] if (iteratorMatch[2]) {
// [ts] res.iterator2 = iteratorMatch[2].trim();
// [ts] }
// [ts] } else {
// [ts] res.alias = alias;
// [ts] }
// [ts] return res
// [ts] }
// [ts] fn processIf(el: auto) {
// [ts] let exp = getAndRemoveAttr(el, "v-if");
// [ts] if (exp) {
// [ts] el.if = exp;
// [ts] addIfCondition(el, {
// [ts] exp: exp,
// [ts] block: el
// [ts] });
// [ts] } else {
// [ts] if (getAndRemoveAttr(el, "v-else") != null) {
// [ts] el.else = true;
// [ts] }
// [ts] let elseif = getAndRemoveAttr(el, "v-else-if");
// [ts] if (elseif) {
// [ts] el.elseif = elseif;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn processIfConditions(el: auto, parent: auto) {
// [ts] let prev = findPrevElement(parent.children);
// [ts] if (prev && prev.if) {
// [ts] addIfCondition(prev, {
// [ts] exp: el.elseif,
// [ts] block: el
// [ts] });
// [ts] } else {
// [ts] warn$2(
// [ts] "v-" + (if (el.elseif) ("else-if=\"" + el.elseif + "\"") else "else") + " " +
// [ts] "used on element <" + (el.tag) + "> without corresponding v-if.",
// [ts] el.rawAttrsMap[el.elseif ? "v-else-if" : "v-else"]
// [ts] );
// [ts] }
// [ts] }
// [ts] fn findPrevElement(children: auto) {
// [ts] let i = children.length;
// [ts] while (i--) {
// [ts] if (children[i].type == 1) {
// [ts] return children[i]
// [ts] } else {
// [ts] if (children[i].text != " ") {
// [ts] warn$2(
// [ts] "text \"" + (children[i].text.trim()) + "\" between v-if and v-else(-if) " +
// [ts] "will be ignored.",
// [ts] children[i]
// [ts] );
// [ts] }
// [ts] children.pop();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn addIfCondition(el: auto, condition: auto) {
// [ts] if (!el.ifConditions) {
// [ts] el.ifConditions = [];
// [ts] }
// [ts] el.ifConditions.push(condition);
// [ts] }
// [ts] fn processOnce(el: auto) {
// [ts] let once$$1 = getAndRemoveAttr(el, "v-once");
// [ts] if (once$$1 != null) {
// [ts] el.once = true;
// [ts] }
// [ts] }
// handle content being passed to a component as slot,
// e.g. <template slot="xxx">, <div slot-scope="xxx">
// [ts] fn processSlotContent(el: auto) {
// [ts] let slotScope;
// [ts] if (el.tag == "template") {
// [ts] slotScope = getAndRemoveAttr(el, "scope");
// [ts] /* istanbul ignore if */
// [ts] if (slotScope) {
// [ts] warn$2(
// [ts] "the \"scope\" attribute for scoped slots have been deprecated and " +
// [ts] "replaced by \"slot-scope\" since 2.5. The new \"slot-scope\" attribute " +
// [ts] "can also be used on plain elements in addition to <template> to " +
// [ts] "denote scoped slots.",
// [ts] el.rawAttrsMap["scope"],
// [ts] true
// [ts] );
// [ts] }
// [ts] el.slotScope = slotScope || getAndRemoveAttr(el, "slot-scope");
// [ts] } else if ((slotScope = getAndRemoveAttr(el, "slot-scope"))) {
// [ts] /* istanbul ignore if */
// [ts] if (el.attrsMap["v-for"]) {
// [ts] warn$2(
// [ts] "Ambiguous combined usage of slot-scope and v-for on <" + (el.tag) + "> " +
// [ts] "(v-for takes higher priority). Use a wrapper <template> for the " +
// [ts] "scoped slot to make it clearer.",
// [ts] el.rawAttrsMap["slot-scope"],
// [ts] true
// [ts] );
// [ts] }
// [ts] el.slotScope = slotScope;
// [ts] }
// slot="xxx"
// [ts] let slotTarget = getBindingAttr(el, "slot");
// [ts] if (slotTarget) {
// [ts] el.slotTarget = if (slotTarget == "\"\"") "\"default\"" else slotTarget;
// [ts] el.slotTargetDynamic = !!(el.attrsMap[":slot"] || el.attrsMap["v-bind:slot"]);
// preserve slot as an attribute for native shadow DOM compat
// only for non-scoped slots.
// [ts] if (el.tag != "template" && !el.slotScope) {
// [ts] addAttr(el, "slot", slotTarget, getRawBindingAttr(el, "slot"));
// [ts] }
// [ts] }
// 2.6 v-slot syntax
// [ts] {
// [ts] if (el.tag == "template") {
// v-slot on <template>
// [ts] let slotBinding = getAndRemoveAttrByRegex(el, slotRE);
// [ts] if (slotBinding) {
// [ts] {
// [ts] if (el.slotTarget || el.slotScope) {
// [ts] warn$2(
// [ts] "Unexpected mixed usage of different slot syntaxes.",
// [ts] el
// [ts] );
// [ts] }
// [ts] if (el.parent && !maybeComponent(el.parent)) {
// [ts] warn$2(
// [ts] "<template v-slot> can only appear at the root level inside " +
// [ts] "the receiving component",
// [ts] el
// [ts] );
// [ts] }
// [ts] }
// [ts] let ref = getSlotName(slotBinding);
// [ts] let name = ref.name;
// [ts] let dynamic = ref.dynamic;
// [ts] el.slotTarget = name;
// [ts] el.slotTargetDynamic = dynamic;
// [ts] el.slotScope = slotBinding.value || emptySlotScopeToken; // force it into a scoped slot for perf
// [ts] }
// [ts] } else {
// v-slot on component, denotes default slot
// [ts] let slotBinding$1 = getAndRemoveAttrByRegex(el, slotRE);
// [ts] if (slotBinding$1) {
// [ts] {
// [ts] if (!maybeComponent(el)) {
// [ts] warn$2(
// [ts] "v-slot can only be used on components or <template>.",
// [ts] slotBinding$1
// [ts] );
// [ts] }
// [ts] if (el.slotScope || el.slotTarget) {
// [ts] warn$2(
// [ts] "Unexpected mixed usage of different slot syntaxes.",
// [ts] el
// [ts] );
// [ts] }
// [ts] if (el.scopedSlots) {
// [ts] warn$2(
// [ts] "To avoid scope ambiguity, the default slot should also use " +
// [ts] "<template> syntax when there are other named slots.",
// [ts] slotBinding$1
// [ts] );
// [ts] }
// [ts] }
// add the component's children to its default slot
// [ts] let slots = el.scopedSlots || (el.scopedSlots = {});
// [ts] let ref$1 = getSlotName(slotBinding$1);
// [ts] let name$1 = ref$1.name;
// [ts] let dynamic$1 = ref$1.dynamic;
// [ts] let slotContainer = slots[name$1] = createASTElement("template", [], el);
// [ts] slotContainer.slotTarget = name$1;
// [ts] slotContainer.slotTargetDynamic = dynamic$1;
// [ts] slotContainer.children = el.children.filter((c) => {
// [ts] if (!c.slotScope) {
// [ts] c.parent = slotContainer;
// [ts] return true
// [ts] }
// [ts] });
// [ts] slotContainer.slotScope = slotBinding$1.value || emptySlotScopeToken;
// remove children as they are returned from scopedSlots now
// [ts] el.children = [];
// mark el non-plain so data gets generated
// [ts] el.plain = false;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getSlotName(binding: auto) {
// [ts] let name = binding.name.replace(slotRE, "");
// [ts] if (!name) {
// [ts] if (binding.name[0] != "#") {
// [ts] name = "default";
// [ts] } else {
// [ts] warn$2(
// [ts] "v-slot shorthand syntax requires a slot name.",
// [ts] binding
// [ts] );
// [ts] }
// [ts] }
// [ts] return dynamicArgRE.test(name)
// dynamic [name]
// [ts] ? { name: name.slice(1, -1), dynamic: true }
// static name
// [ts] : { name: ("\"" + name + "\""), dynamic: false }
// [ts] }
// handle <slot/> outlets
// [ts] fn processSlotOutlet(el: auto) {
// [ts] if (el.tag == "slot") {
// [ts] el.slotName = getBindingAttr(el, "name");
// [ts] if (el.key) {
// [ts] warn$2(
// [ts] "`key` does not work on <slot> because slots are abstract outlets " +
// [ts] "and can possibly expand into multiple elements. " +
// [ts] "Use the key on a wrapping element instead.",
// [ts] getRawBindingAttr(el, "key")
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] fn processComponent(el: auto) {
// [ts] let binding;
// [ts] if ((binding = getBindingAttr(el, "is"))) {
// [ts] el.component = binding;
// [ts] }
// [ts] if (getAndRemoveAttr(el, "inline-template") != null) {
// [ts] el.inlineTemplate = true;
// [ts] }
// [ts] }
// [ts] fn processAttrs(el: auto) {
// [ts] let list = el.attrsList;
// [ts] let i, l, name, rawName, value, modifiers, syncGen, isDynamic;
// [ts] i = 0, l = list.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] name = rawName = list[i].name;
// [ts] value = list[i].value;
// [ts] if (dirRE.test(name)) {
// mark element as dynamic
// [ts] el.hasBindings = true;
// modifiers
// [ts] modifiers = parseModifiers(name.replace(dirRE, ""));
// support .foo shorthand syntax for the .prop modifier
// [ts] if (modifiers) {
// [ts] name = name.replace(modifierRE, "");
// [ts] }
// [ts] if (bindRE.test(name)) { // v-bind
// [ts] name = name.replace(bindRE, "");
// [ts] value = parseFilters(value);
// [ts] isDynamic = dynamicArgRE.test(name);
// [ts] if (isDynamic) {
// [ts] name = name.slice(1, -1);
// [ts] }
// [ts] if (
// [ts] value.trim().length == 0
// [ts] ) {
// [ts] warn$2(
// [ts] ("The value for a v-bind expression cannot be empty. Found in \"v-bind:" + name + "\"")
// [ts] );
// [ts] }
// [ts] if (modifiers) {
// [ts] if (modifiers.prop && !isDynamic) {
// [ts] name = camelize(name);
// [ts] if (name == "innerHtml") { name = "innerHTML"; }
// [ts] }
// [ts] if (modifiers.camel && !isDynamic) {
// [ts] name = camelize(name);
// [ts] }
// [ts] if (modifiers.sync) {
// [ts] syncGen = genAssignmentCode(value, "$event");
// [ts] if (!isDynamic) {
// [ts] addHandler(
// [ts] el,
// [ts] ("update:" + (camelize(name))),
// [ts] syncGen,
// [ts] null,
// [ts] false,
// [ts] warn$2,
// [ts] list[i]
// [ts] );
// [ts] if (hyphenate(name) != camelize(name)) {
// [ts] addHandler(
// [ts] el,
// [ts] ("update:" + (hyphenate(name))),
// [ts] syncGen,
// [ts] null,
// [ts] false,
// [ts] warn$2,
// [ts] list[i]
// [ts] );
// [ts] }
// [ts] } else {
// handler w/ dynamic event name
// [ts] addHandler(
// [ts] el,
// [ts] ("\"update:\"+(" + name + ")"),
// [ts] syncGen,
// [ts] null,
// [ts] false,
// [ts] warn$2,
// [ts] list[i],
// [ts] true // dynamic
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] if ((modifiers && modifiers.prop) || (
// [ts] !el.component && platformMustUseProp(el.tag, el.attrsMap.type, name)
// [ts] )) {
// [ts] addProp(el, name, value, list[i], isDynamic);
// [ts] } else {
// [ts] addAttr(el, name, value, list[i], isDynamic);
// [ts] }
// [ts] } else if (onRE.test(name)) { // v-on
// [ts] name = name.replace(onRE, "");
// [ts] isDynamic = dynamicArgRE.test(name);
// [ts] if (isDynamic) {
// [ts] name = name.slice(1, -1);
// [ts] }
// [ts] addHandler(el, name, value, modifiers, false, warn$2, list[i], isDynamic);
// [ts] } else { // normal directives
// [ts] name = name.replace(dirRE, "");
// parse arg
// [ts] let argMatch = name.match(argRE);
// [ts] let arg = argMatch && argMatch[1];
// [ts] isDynamic = false;
// [ts] if (arg) {
// [ts] name = name.slice(0, -(arg.length + 1));
// [ts] if (dynamicArgRE.test(arg)) {
// [ts] arg = arg.slice(1, -1);
// [ts] isDynamic = true;
// [ts] }
// [ts] }
// [ts] addDirective(el, name, rawName, value, arg, isDynamic, modifiers, list[i]);
// [ts] if (name == "model") {
// [ts] checkForAliasModel(el, value);
// [ts] }
// [ts] }
// [ts] } else {
// literal attribute
// [ts] {
// [ts] let res = parseText(value, delimiters);
// [ts] if (res) {
// [ts] warn$2(
// [ts] name + "=\"" + value + "\": " +
// [ts] "Interpolation inside attributes has been removed. " +
// [ts] "Use v-bind or the colon shorthand instead. For example, " +
// [ts] "instead of <div id=\"{{ val }}\">, use <div :id=\"val\">.",
// [ts] list[i]
// [ts] );
// [ts] }
// [ts] }
// [ts] addAttr(el, name, JSON.stringify(value), list[i]);
// #6887 firefox doesn't update muted state if set via attribute
// even immediately after element creation
// [ts] if (!el.component &&
// [ts] name == "muted" &&
// [ts] platformMustUseProp: (el.tag, el.attrsMap.type, name) => {
// [ts] addProp(el, name, "true", list[i]);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn checkInFor(el: auto) {
// [ts] let parent = el;
// [ts] while (parent) {
// [ts] if (parent.for != null) {
// [ts] return true
// [ts] }
// [ts] parent = parent.parent;
// [ts] }
// [ts] return false
// [ts] }
// [ts] fn parseModifiers(name: auto) {
// [ts] let match_val = name.match(modifierRE);
// [ts] if (match_val) {
// [ts] let ret = {};
// [ts] match_val.forEach((m) => { ret[m.slice(1)] = true; });
// [ts] return ret
// [ts] }
// [ts] }
// [ts] fn makeAttrsMap(attrs: auto) {
// [ts] let map = {};
// [ts] let i = 0, l = attrs.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] if (
// [ts] map[attrs[i].name] && !isIE && !isEdge
// [ts] ) {
// [ts] warn$2("duplicate attribute: " + attrs[i].name, attrs[i]);
// [ts] }
// [ts] map[attrs[i].name] = attrs[i].value;
// [ts] }
// [ts] return map
// [ts] }
// for script (e.g. type="x/template") or style, do not decode content
// [ts] fn isTextTag(el: auto) {
// [ts] return el.tag == "script" || el.tag == "style"
// [ts] }
// [ts] fn isForbiddenTag(el: auto) {
// [ts] return (
// [ts] el.tag == "style" ||
// [ts] (el.tag == "script" && (
// [ts] !el.attrsMap.type ||
// [ts] el.attrsMap.type == "text/javascript"
// [ts] ))
// [ts] )
// [ts] }
// [ts] let ieNSBug = "^xmlns:NS\\d+";
// [ts] let ieNSPrefix = "^NS\\d+:";
// [ts] /* istanbul ignore next */
// [ts] fn guardIESVGBug(attrs: auto) {
// [ts] let res = [];
// [ts] let i = 0;
// [ts] while (i < attrs.length) { // [cap-migrate] for-step: i++
// [ts] let attr = attrs[i];
// [ts] if (!ieNSBug.test(attr.name)) {
// [ts] attr.name = attr.name.replace(ieNSPrefix, "");
// [ts] res.push(attr);
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] fn checkForAliasModel(el: auto, value: auto) {
// [ts] let _el = el;
// [ts] while (_el) {
// [ts] if (_el.for && _el.alias == value) {
// [ts] warn$2(
// [ts] "<" + (el.tag) + " v-model=\"" + value + "\">: " +
// [ts] "You are binding v-model directly to a v-for iteration alias. " +
// [ts] "This will not be able to modify the v-for source array because " +
// [ts] "writing to the alias is like modifying a function local variable. " +
// [ts] "Consider using an array of objects and use v-model on an object property instead.",
// [ts] el.rawAttrsMap["v-model"]
// [ts] );
// [ts] }
// [ts] _el = _el.parent;
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn preTransformNode(el: auto, options: auto) {
// [ts] if (el.tag == "input") {
// [ts] let map = el.attrsMap;
// [ts] if (!map["v-model"]) {
// [ts] return
// [ts] }
// [ts] let typeBinding;
// [ts] if (map[":type"] || map["v-bind:type"]) {
// [ts] typeBinding = getBindingAttr(el, "type");
// [ts] }
// [ts] if (!map.type && !typeBinding && map["v-bind"]) {
// [ts] typeBinding = "(" + (map["v-bind"]) + ").type";
// [ts] }
// [ts] if (typeBinding) {
// [ts] let ifCondition = getAndRemoveAttr(el, "v-if", true);
// [ts] let ifConditionExtra = if (ifCondition) ("&&(" + ifCondition + ")") else "";
// [ts] let hasElse = getAndRemoveAttr(el, "v-else", true) != null;
// [ts] let elseIfCondition = getAndRemoveAttr(el, "v-else-if", true);
// 1. checkbox
// [ts] let branch0 = cloneASTElement(el);
// process for on the main node
// [ts] processFor(branch0);
// [ts] addRawAttr(branch0, "type", "checkbox");
// [ts] processElement(branch0, options);
// [ts] branch0.processed = true; // prevent it from double-processed
// [ts] branch0.if = "(" + typeBinding + ")==='checkbox'" + ifConditionExtra;
// [ts] addIfCondition(branch0, {
// [ts] exp: branch0.if,
// [ts] block: branch0
// [ts] });
// 2. add radio else-if condition
// [ts] let branch1 = cloneASTElement(el);
// [ts] getAndRemoveAttr(branch1, "v-for", true);
// [ts] addRawAttr(branch1, "type", "radio");
// [ts] processElement(branch1, options);
// [ts] addIfCondition(branch0, {
// [ts] exp: "(" + typeBinding + ")==='radio'" + ifConditionExtra,
// [ts] block: branch1
// [ts] });
// 3. other
// [ts] let branch2 = cloneASTElement(el);
// [ts] getAndRemoveAttr(branch2, "v-for", true);
// [ts] addRawAttr(branch2, ":type", typeBinding);
// [ts] processElement(branch2, options);
// [ts] addIfCondition(branch0, {
// [ts] exp: ifCondition,
// [ts] block: branch2
// [ts] });
// [ts] if (hasElse) {
// [ts] branch0.else = true;
// [ts] } else if (elseIfCondition) {
// [ts] branch0.elseif = elseIfCondition;
// [ts] }
// [ts] return branch0
// [ts] }
// [ts] }
// [ts] }
// [ts] fn cloneASTElement(el: auto) {
// [ts] return createASTElement(el.tag, el.attrsList.slice(), el.parent)
// [ts] }
// [ts] let model$1 = {
// [ts] preTransformNode: preTransformNode
// [ts] };
// [ts] let modules$1 = [
// [ts] klass$1,
// [ts] style$1,
// [ts] model$1
// [ts] ];
// [ts] /*  */
// [ts] fn text(el: auto, dir: auto) {
// [ts] if (dir.value) {
// [ts] addProp(el, "textContent", ("_s(" + (dir.value) + ")"), dir);
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn html(el: auto, dir: auto) {
// [ts] if (dir.value) {
// [ts] addProp(el, "innerHTML", ("_s(" + (dir.value) + ")"), dir);
// [ts] }
// [ts] }
// [ts] let directives$1 = {
// [ts] model: model,
// [ts] text: text,
// [ts] html: html
// [ts] };
// [ts] /*  */
// [ts] let baseOptions = {
// [ts] expectHTML: true,
// [ts] modules: modules$1,
// [ts] directives: directives$1,
// [ts] isPreTag: isPreTag,
// [ts] isUnaryTag: isUnaryTag,
// [ts] mustUseProp: mustUseProp,
// [ts] canBeLeftOpenTag: canBeLeftOpenTag,
// [ts] isReservedTag: isReservedTag,
// [ts] getTagNamespace: getTagNamespace,
// [ts] staticKeys: genStaticKeys(modules$1)
// [ts] };
// [ts] /*  */
// [ts] let isStaticKey;
// [ts] let isPlatformReservedTag;
// [ts] let genStaticKeysCached = cached(genStaticKeys$1);
// [ts] /**
// [ts] * Goal of the optimizer: walk the generated template AST tree
// [ts] * and detect sub-trees that are purely static, i.e. parts of
// [ts] * the DOM that never needs to change.
// [ts] *
// [ts] * Once we detect these sub-trees, we can:
// [ts] *
// [ts] * 1. Hoist them into constants, so that we no longer need to
// [ts] *    create fresh nodes for them on each re-render;
// [ts] * 2. Completely skip them in the patching process.
// [ts] */
// [ts] fn optimize(root: auto, options: auto) {
// [ts] if (!root) { return }
// [ts] isStaticKey = genStaticKeysCached(options.staticKeys || "");
// [ts] isPlatformReservedTag = options.isReservedTag || no;
// first pass: mark all non-static nodes.
// [ts] markStatic$1(root);
// second pass: mark static roots.
// [ts] markStaticRoots(root, false);
// [ts] }
// [ts] (keys) => {
// [ts] return makeMap(
// [ts] "type,tag,attrsList,attrsMap,plain,parent,children,attrs,start,end,rawAttrsMap" +
// [ts] (if (keys) "," + keys else "")
// [ts] )
// [ts] }
// [ts] (node) => {
// [ts] node.static = isStatic(node);
// [ts] if (node.type == 1) {
// do not make component slot content static. this avoids
// 1. components not able to mutate slot nodes
// 2. static slot content fails for hot-reloading
// [ts] if (
// [ts] !isPlatformReservedTag(node.tag) &&
// [ts] node.tag != "slot" &&
// [ts] node.attrsMap["inline-template"] == null
// [ts] ) {
// [ts] return
// [ts] }
// [ts] let i = 0, l = node.children.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] let child = node.children[i];
// [ts] markStatic$1(child);
// [ts] if (!child.static) {
// [ts] node.static = false;
// [ts] }
// [ts] }
// [ts] if (node.ifConditions) {
// [ts] let i$1 = 1, l$1 = node.ifConditions.length;
// [ts] while (i$1 < l$1) { // [cap-migrate] for-step: i$1++
// [ts] let block = node.ifConditions[i$1].block;
// [ts] markStatic$1(block);
// [ts] if (!block.static) {
// [ts] node.static = false;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markStaticRoots(node: auto, isInFor: auto) {
// [ts] if (node.type == 1) {
// [ts] if (node.static || node.once) {
// [ts] node.staticInFor = isInFor;
// [ts] }
// For a node to qualify as a static root, it should have children that
// are not just static text. Otherwise the cost of hoisting out will
// outweigh the benefits and it's better off to just always render it fresh.
// [ts] if (node.static && node.children.length && !(
// [ts] node.children.length == 1 &&
// [ts] node.children[0].type == 3
// [ts] )) {
// [ts] node.staticRoot = true;
// [ts] return
// [ts] } else {
// [ts] node.staticRoot = false;
// [ts] }
// [ts] if (node.children) {
// [ts] let i = 0, l = node.children.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] markStaticRoots(node.children[i], isInFor || !!node.for);
// [ts] }
// [ts] }
// [ts] if (node.ifConditions) {
// [ts] let i$1 = 1, l$1 = node.ifConditions.length;
// [ts] while (i$1 < l$1) { // [cap-migrate] for-step: i$1++
// [ts] markStaticRoots(node.ifConditions[i$1].block, isInFor);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isStatic(node: auto) {
// [ts] if (node.type == 2) { // expression
// [ts] return false
// [ts] }
// [ts] if (node.type == 3) { // text
// [ts] return true
// [ts] }
// [ts] return !!(node.pre || (
// [ts] !node.hasBindings && // no dynamic bindings
// [ts] !node.if && !node.for && // not v-if or v-for or v-else
// [ts] !isBuiltInTag(node.tag) && // not a built-in
// [ts] isPlatformReservedTag(node.tag) && // not a component
// [ts] !isDirectChildOfTemplateFor(node) &&
// [ts] Object.keys(node).every(isStaticKey)
// [ts] ))
// [ts] }
// [ts] fn isDirectChildOfTemplateFor(node: auto) {
// [ts] while (node.parent) {
// [ts] node = node.parent;
// [ts] if (node.tag != "template") {
// [ts] return false
// [ts] }
// [ts] if (node.for) {
// [ts] return true
// [ts] }
// [ts] }
// [ts] return false
// [ts] }
// [ts] /*  */
// [ts] let fnExpRE = "^([\\w$_]+|\\([^)]*?\\))\\s*=>|^function(?:\\s+[\\w$]+)?\\s*\\(";
// [ts] let fnInvokeRE = "\\([^)]*?\\);*$";
// [ts] let simplePathRE = "^[A-Za-z_$][\\w$]*(?:\\.[A-Za-z_$][\\w$]*|\\['[^']*?']|\\[\"[^\"]*?\"]|\\[\\d+]|\\[[A-Za-z_$][\\w$]*])*$";
// KeyboardEvent.keyCode aliases
// [ts] let keyCodes = {
// [ts] esc: 27,
// [ts] tab: 9,
// [ts] enter: 13,
// [ts] space: 32,
// [ts] up: 38,
// [ts] left: 37,
// [ts] right: 39,
// [ts] down: 40,
// [ts] "delete": [8, 46]
// [ts] };
// KeyboardEvent.key aliases
// [ts] let keyNames = {
// #7880: IE11 and Edge use `Esc` for Escape key name.
// [ts] esc: ["Esc", "Escape"],
// [ts] tab: "Tab",
// [ts] enter: "Enter",
// #9112: IE11 uses `Spacebar` for Space key name.
// [ts] space: [" ", "Spacebar"],
// #7806: IE11 uses key names without `Arrow` prefix for arrow keys.
// [ts] up: ["Up", "ArrowUp"],
// [ts] left: ["Left", "ArrowLeft"],
// [ts] right: ["Right", "ArrowRight"],
// [ts] down: ["Down", "ArrowDown"],
// #9112: IE11 uses `Del` for Delete key name.
// [ts] "delete": ["Backspace", "Delete", "Del"]
// [ts] };
// #4868: modifiers that prevent the execution of the listener
// need to explicitly return null so that we can determine whether to remove
// the listener for .once
// [ts] let genGuard = (condition) => { return ("if(" + condition + ")return null;"); };
// [ts] let modifierCode = {
// [ts] stop: "$event.stopPropagation();",
// [ts] prevent: "$event.preventDefault();",
// [ts] self: genGuard("$event.target !== $event.currentTarget"),
// [ts] ctrl: genGuard("!$event.ctrlKey"),
// [ts] shift: genGuard("!$event.shiftKey"),
// [ts] alt: genGuard("!$event.altKey"),
// [ts] meta: genGuard("!$event.metaKey"),
// [ts] left: genGuard("'button' in $event && $event.button !== 0"),
// [ts] middle: genGuard("'button' in $event && $event.button !== 1"),
// [ts] right: genGuard("'button' in $event && $event.button !== 2")
// [ts] };
// [ts] fn genHandlers(events: auto, isNative: auto) {
// [ts] let prefix = if (isNative) "nativeOn:" else "on:";
// [ts] let staticHandlers = "";
// [ts] let dynamicHandlers = "";
// [ts] for (let name in events) {
// [ts] let handlerCode = genHandler(events[name]);
// [ts] if (events[name] && events[name].dynamic) {
// [ts] dynamicHandlers += name + "," + handlerCode + ",";
// [ts] } else {
// [ts] staticHandlers += "\"" + name + "\":" + handlerCode + ",";
// [ts] }
// [ts] }
// [ts] staticHandlers = "{" + (staticHandlers.slice(0, -1)) + "}";
// [ts] if (dynamicHandlers) {
// [ts] return prefix + "_d(" + staticHandlers + ",[" + (dynamicHandlers.slice(0, -1)) + "])"
// [ts] } else {
// [ts] return prefix + staticHandlers
// [ts] }
// [ts] }
// [ts] fn genHandler(handler: auto) {
// [ts] if (!handler) {
// [ts] return "function(){}"
// [ts] }
// [ts] if (Array.isArray(handler)) {
// [ts] return ("[" + (handler.map((handler) => { return genHandler(handler); }).join(",")) + "]")
// [ts] }
// [ts] let isMethodPath = simplePathRE.test(handler.value);
// [ts] let isFunctionExpression = fnExpRE.test(handler.value);
// [ts] let isFunctionInvocation = simplePathRE.test(handler.value.replace(fnInvokeRE, ""));
// [ts] if (!handler.modifiers) {
// [ts] if (isMethodPath || isFunctionExpression) {
// [ts] return handler.value
// [ts] }
// [ts] return ("function($event){" + (if (isFunctionInvocation) ("return " + (handler.value)) else handler.value) + "}") // inline statement
// [ts] } else {
// [ts] let code = "";
// [ts] let genModifierCode = "";
// [ts] let keys = [];
// [ts] for (let key in handler.modifiers) {
// [ts] if (modifierCode[key]) {
// [ts] genModifierCode += modifierCode[key];
// left/right
// [ts] if (keyCodes[key]) {
// [ts] keys.push(key);
// [ts] }
// [ts] } else if (key == "exact") {
// [ts] let modifiers = (handler.modifiers);
// [ts] genModifierCode += genGuard(
// [ts] ["ctrl", "shift", "alt", "meta"]
// [ts] .filter((keyModifier) => { return !modifiers[keyModifier]; })
// [ts] .map((keyModifier) => { return ("$event." + keyModifier + "Key"); })
// [ts] .join("||")
// [ts] );
// [ts] } else {
// [ts] keys.push(key);
// [ts] }
// [ts] }
// [ts] if (keys.length) {
// [ts] code += genKeyFilter(keys);
// [ts] }
// Make sure modifiers like prevent and stop get executed after key filtering
// [ts] if (genModifierCode) {
// [ts] code += genModifierCode;
// [ts] }
// [ts] let handlerCode = if (isMethodPath) ("return " + (handler.value) + ".apply(null, arguments)") else if (isFunctionExpression) ("return (" + (handler.value) + ").apply(null, arguments)") else if (isFunctionInvocation) ("return " + (handler.value)) else handler.value;
// [ts] return ("function($event){" + code + handlerCode + "}")
// [ts] }
// [ts] }
// [ts] fn genKeyFilter(keys: auto) {
// [ts] return (
// make sure the key filters only apply to KeyboardEvents
// #9441: can't use 'keyCode' in $event because Chrome autofill fires fake
// key events that do not have keyCode property...
// [ts] "if(!$event.type.indexOf('key')&&" +
// [ts] (keys.map(genFilterCode).join("&&")) + ")return null;"
// [ts] )
// [ts] }
// [ts] fn genFilterCode(key: auto) {
// [ts] let keyVal = parseInt(key, 10);
// [ts] if (keyVal) {
// [ts] return ("$event.keyCode!==" + keyVal)
// [ts] }
// [ts] let keyCode = keyCodes[key];
// [ts] let keyName = keyNames[key];
// [ts] return (
// [ts] "_k($event.keyCode," +
// [ts] (JSON.stringify(key)) + "," +
// [ts] (JSON.stringify(keyCode)) + "," +
// [ts] "$event.key," +
// [ts] "" + (JSON.stringify(keyName)) +
// [ts] ")"
// [ts] )
// [ts] }
// [ts] /*  */
// [ts] fn on(el: auto, dir: auto) {
// [ts] if (dir.modifiers) {
// [ts] warn("v-on without argument does not support modifiers.");
// [ts] }
// [ts] el.wrapListeners = (code) => { return ("_g(" + code + "," + (dir.value) + ")"); };
// [ts] }
// [ts] /*  */
// [ts] (el, dir) => {
// [ts] el.wrapData = (code) => {
// [ts] return ("_b(" + code + ",'" + (el.tag) + "'," + (dir.value) + "," + (if (dir.modifiers && dir.modifiers.prop) "true" else "false") + (if (dir.modifiers && dir.modifiers.sync) ",true" else "") + ")")
// [ts] };
// [ts] }
// [ts] /*  */
// [ts] let baseDirectives = {
// [ts] on: on,
// [ts] bind: bind$1,
// [ts] cloak: noop
// [ts] };
// [ts] /*  */
// [ts] let CodegenState = (options) => {
// [ts] _.options = options;
// [ts] _.warn = options.warn || baseWarn;
// [ts] _.transforms = pluckModuleFunction(options.modules, "transformCode");
// [ts] _.dataGenFns = pluckModuleFunction(options.modules, "genData");
// [ts] _.directives = extend(extend({}, baseDirectives), options.directives);
// [ts] let isReservedTag = options.isReservedTag || no;
// [ts] _.maybeComponent = (el) => { return !!el.component || !isReservedTag(el.tag); };
// [ts] _.onceId = 0;
// [ts] _.staticRenderFns = [];
// [ts] _.pre = false;
// [ts] };
// [ts] fn generate(ast: auto, options: auto) {
// [ts] let state = CodegenState.init(options);
// fix #11483, Root level <script> tags should not be rendered.
// [ts] let code = if (ast) (if (ast.tag == "script") "null" else genElement(ast, state)) else "_c(\"div\")";
// [ts] return {
// [ts] render: ("with(this){return " + code + "}"),
// [ts] staticRenderFns: state.staticRenderFns
// [ts] }
// [ts] }
// [ts] fn genElement(el: auto, state: auto) {
// [ts] if (el.parent) {
// [ts] el.pre = el.pre || el.parent.pre;
// [ts] }
// [ts] if (el.staticRoot && !el.staticProcessed) {
// [ts] return genStatic(el, state)
// [ts] } else if (el.once && !el.onceProcessed) {
// [ts] return genOnce(el, state)
// [ts] } else if (el.for && !el.forProcessed) {
// [ts] return genFor(el, state)
// [ts] } else if (el.if && !el.ifProcessed) {
// [ts] return genIf(el, state)
// [ts] } else if (el.tag == "template" && !el.slotTarget && !state.pre) {
// [ts] return genChildren(el, state) || "void 0"
// [ts] } else if (el.tag == "slot") {
// [ts] return genSlot(el, state)
// [ts] } else {
// component or element
// [ts] let code;
// [ts] if (el.component) {
// [ts] code = genComponent(el.component, el, state);
// [ts] } else {
// [ts] let data;
// [ts] if (!el.plain || (el.pre && state.maybeComponent(el))) {
// [ts] data = genData$2(el, state);
// [ts] }
// [ts] let children = if (el.inlineTemplate) null else genChildren(el, state, true);
// [ts] code = "_c('" + (el.tag) + "'" + (if (data) ("," + data) else "") + (if (children) ("," + children) else "") + ")";
// [ts] }
// module transforms
// [ts] let i = 0;
// [ts] while (i < state.transforms.length) { // [cap-migrate] for-step: i++
// [ts] code = state.transforms[i](el, code);
// [ts] }
// [ts] return code
// [ts] }
// [ts] }
// hoist static sub-trees out
// [ts] fn genStatic(el: auto, state: auto) {
// [ts] el.staticProcessed = true;
// Some elements (templates) need to behave differently inside of a v-pre
// node.  All pre nodes are static roots, so we can use this as a location to
// wrap a state change and reset it upon exiting the pre node.
// [ts] let originalPreState = state.pre;
// [ts] if (el.pre) {
// [ts] state.pre = el.pre;
// [ts] }
// [ts] state.staticRenderFns.push(("with(this){return " + (genElement(el, state)) + "}"));
// [ts] state.pre = originalPreState;
// [ts] return ("_m(" + (state.staticRenderFns.length - 1) + (if (el.staticInFor) ",true" else "") + ")")
// [ts] }
// v-once
// [ts] fn genOnce(el: auto, state: auto) {
// [ts] el.onceProcessed = true;
// [ts] if (el.if && !el.ifProcessed) {
// [ts] return genIf(el, state)
// [ts] } else if (el.staticInFor) {
// [ts] let key = "";
// [ts] let parent = el.parent;
// [ts] while (parent) {
// [ts] if (parent.for) {
// [ts] key = parent.key;
// [ts] break
// [ts] }
// [ts] parent = parent.parent;
// [ts] }
// [ts] if (!key) {
// [ts] state.warn(
// [ts] "v-once can only be used inside v-for that is keyed. ",
// [ts] el.rawAttrsMap["v-once"]
// [ts] );
// [ts] return genElement(el, state)
// [ts] }
// [ts] return ("_o(" + (genElement(el, state)) + "," + (state.onceId++) + "," + key + ")")
// [ts] } else {
// [ts] return genStatic(el, state)
// [ts] }
// [ts] }
// [ts] fn genIf(el: auto, state: auto, altGen: auto, altEmpty: auto) {
// [ts] el.ifProcessed = true; // avoid recursion
// [ts] return genIfConditions(el.ifConditions.slice(), state, altGen, altEmpty)
// [ts] }
// [ts] fn genIfConditions(conditions: auto, state: auto, altGen: auto, altEmpty: auto) {
// [ts] if (!conditions.length) {
// [ts] return altEmpty || "_e()"
// [ts] }
// [ts] let condition = conditions.shift();
// [ts] if (condition.exp) {
// [ts] return ("(" + (condition.exp) + ")?" + (genTernaryExp(condition.block)) + ":" + (genIfConditions(conditions, state, altGen, altEmpty)))
// [ts] } else {
// [ts] return ("" + (genTernaryExp(condition.block)))
// [ts] }
// v-if with v-once should generate code like (a)?_m(0):_m(1)
// [ts] fn genTernaryExp(el: auto) {
// [ts] return if (altGen) altGen(el, state) else if (el.once) genOnce(el, state) else genElement(el, state)
// [ts] }
// [ts] }
// [ts] fn genFor(el: auto, state: auto, altGen: auto, altHelper: auto) {
// [ts] let exp = el.for;
// [ts] let alias = el.alias;
// [ts] let iterator1 = if (el.iterator1) ("," + (el.iterator1)) else "";
// [ts] let iterator2 = if (el.iterator2) ("," + (el.iterator2)) else "";
// [ts] if (state.maybeComponent(el) &&
// [ts] el.tag != "slot" &&
// [ts] el.tag != "template" &&
// [ts] !el.key
// [ts] ) {
// [ts] state.warn(
// [ts] "<" + (el.tag) + " v-for=\"" + alias + " in " + exp + "\">: component lists rendered with " +
// [ts] "v-for should have explicit keys. " +
// [ts] "See https://vuejs.org/guide/list.html#key for more info.",
// [ts] el.rawAttrsMap["v-for"],
// [ts] true /* tip */
// [ts] );
// [ts] }
// [ts] el.forProcessed = true; // avoid recursion
// [ts] return (altHelper || "_l") + "((" + exp + ")," +
// [ts] "function(" + alias + iterator1 + iterator2 + "){" +
// [ts] "return " + ((altGen || genElement)(el, state)) +
// [ts] "})"
// [ts] }
// [ts] (el, state) => {
// [ts] let data = "{";
// directives first.
// directives may mutate the el's other properties before they are generated.
// [ts] let dirs = genDirectives(el, state);
// [ts] if (dirs) { data += dirs + ","; }
// key
// [ts] if (el.key) {
// [ts] data += "key:" + (el.key) + ",";
// [ts] }
// ref
// [ts] if (el.ref) {
// [ts] data += "ref:" + (el.ref) + ",";
// [ts] }
// [ts] if (el.refInFor) {
// [ts] data += "refInFor:true,";
// [ts] }
// pre
// [ts] if (el.pre) {
// [ts] data += "pre:true,";
// [ts] }
// record original tag name for components using "is" attribute
// [ts] if (el.component) {
// [ts] data += "tag:\"" + (el.tag) + "\",";
// [ts] }
// module data generation functions
// [ts] let i = 0;
// [ts] while (i < state.dataGenFns.length) { // [cap-migrate] for-step: i++
// [ts] data += state.dataGenFns[i](el);
// [ts] }
// attributes
// [ts] if (el.attrs) {
// [ts] data += "attrs:" + (genProps(el.attrs)) + ",";
// [ts] }
// DOM props
// [ts] if (el.props) {
// [ts] data += "domProps:" + (genProps(el.props)) + ",";
// [ts] }
// event handlers
// [ts] if (el.events) {
// [ts] data += (genHandlers(el.events, false)) + ",";
// [ts] }
// [ts] if (el.nativeEvents) {
// [ts] data += (genHandlers(el.nativeEvents, true)) + ",";
// [ts] }
// slot target
// only for non-scoped slots
// [ts] if (el.slotTarget && !el.slotScope) {
// [ts] data += "slot:" + (el.slotTarget) + ",";
// [ts] }
// scoped slots
// [ts] if (el.scopedSlots) {
// [ts] data += (genScopedSlots(el, el.scopedSlots, state)) + ",";
// [ts] }
// component v-model
// [ts] if (el.model) {
// [ts] data += "model:{value:" + (el.model.value) + ",callback:" + (el.model.callback) + ",expression:" + (el.model.expression) + "},";
// [ts] }
// inline-template
// [ts] if (el.inlineTemplate) {
// [ts] let inlineTemplate = genInlineTemplate(el, state);
// [ts] if (inlineTemplate) {
// [ts] data += inlineTemplate + ",";
// [ts] }
// [ts] }
// [ts] data = data.replace(",$", "") + "}";
// v-bind dynamic argument wrap
// v-bind with dynamic arguments must be applied using the same v-bind object
// merge helper so that class/style/mustUseProp attrs are handled correctly.
// [ts] if (el.dynamicAttrs) {
// [ts] data = "_b(" + data + ",\"" + (el.tag) + "\"," + (genProps(el.dynamicAttrs)) + ")";
// [ts] }
// v-bind data wrap
// [ts] if (el.wrapData) {
// [ts] data = el.wrapData(data);
// [ts] }
// v-on data wrap
// [ts] if (el.wrapListeners) {
// [ts] data = el.wrapListeners(data);
// [ts] }
// [ts] return data
// [ts] }
// [ts] fn genDirectives(el: auto, state: auto) {
// [ts] let dirs = el.directives;
// [ts] if (!dirs) { return }
// [ts] let res = "directives:[";
// [ts] let hasRuntime = false;
// [ts] let i, l, dir, needRuntime;
// [ts] i = 0, l = dirs.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] dir = dirs[i];
// [ts] needRuntime = true;
// [ts] let gen = state.directives[dir.name];
// [ts] if (gen) {
// compile-time directive that manipulates AST.
// returns true if it also needs a runtime counterpart.
// [ts] needRuntime = !!gen(el, dir, state.warn);
// [ts] }
// [ts] if (needRuntime) {
// [ts] hasRuntime = true;
// [ts] res += "{name:\"" + (dir.name) + "\",rawName:\"" + (dir.rawName) + "\"" + (if (dir.value) (",value:(" + (dir.value) + "),expression:" + (JSON.stringify(dir.value))) else "") + (if (dir.arg) (",arg:" + (if (dir.isDynamicArg) dir.arg else ("\"" + (dir.arg) + "\""))) else "") + (if (dir.modifiers) (",modifiers:" + (JSON.stringify(dir.modifiers))) else "") + "},";
// [ts] }
// [ts] }
// [ts] if (hasRuntime) {
// [ts] return res.slice(0, -1) + "]"
// [ts] }
// [ts] }
// [ts] fn genInlineTemplate(el: auto, state: auto) {
// [ts] let ast = el.children[0];
// [ts] if (el.children.length != 1 || ast.type != 1) {
// [ts] state.warn(
// [ts] "Inline-template components must have exactly one child element.",
// [ts] { start: el.start }
// [ts] );
// [ts] }
// [ts] if (ast && ast.type == 1) {
// [ts] let inlineRenderFns = generate(ast, state.options);
// [ts] return ("inlineTemplate:{render:function(){" + (inlineRenderFns.render) + "},staticRenderFns:[" + (inlineRenderFns.staticRenderFns.map((code) => { return ("function(){" + code + "}"); }).join(",")) + "]}")
// [ts] }
// [ts] }
// [ts] fn genScopedSlots(el: auto, slots: auto, state: auto) {
// by default scoped slots are considered "stable", this allows child
// components with only scoped slots to skip forced updates from parent.
// but in some cases we have to bail-out of this optimization
// for example if the slot contains dynamic names, has v-if or v-for on them...
// [ts] let needsForceUpdate = el.for || Object.keys(slots).some((key) => {
// [ts] let slot = slots[key];
// [ts] return (
// [ts] slot.slotTargetDynamic ||
// [ts] slot.if ||
// [ts] slot.for ||
// [ts] containsSlotChild(slot) // is passing down slot from parent which may be dynamic
// [ts] )
// [ts] });
// #9534: if a component with scoped slots is inside a conditional branch,
// it's possible for the same component to be reused but with different
// compiled slot content. To avoid that, we generate a unique key based on
// the generated code of all the slot contents.
// [ts] let needsKey = !!el.if;
// OR when it is inside another scoped slot or v-for (the reactivity may be
// disconnected due to the intermediate scope variable)
// #9438, #9506
// TODO: this can be further optimized by properly analyzing in-scope bindings
// and skip force updating ones that do not actually use scope variables.
// [ts] if (!needsForceUpdate) {
// [ts] let parent = el.parent;
// [ts] while (parent) {
// [ts] if (
// [ts] (parent.slotScope && parent.slotScope != emptySlotScopeToken) ||
// [ts] parent.for
// [ts] ) {
// [ts] needsForceUpdate = true;
// [ts] break
// [ts] }
// [ts] if (parent.if) {
// [ts] needsKey = true;
// [ts] }
// [ts] parent = parent.parent;
// [ts] }
// [ts] }
// [ts] let generatedSlots = Object.keys(slots)
// [ts] .map((key) => { return genScopedSlot(slots[key], state); })
// [ts] .join(",");
// [ts] return ("scopedSlots:_u([" + generatedSlots + "]" + (if (needsForceUpdate) ",null,true" else "") + (if (!needsForceUpdate && needsKey) (",null,false," + (hash(generatedSlots))) else "") + ")")
// [ts] }
// [ts] fn hash(str: auto) {
// [ts] let hash = 5381;
// [ts] let i = str.length;
// [ts] while(i) {
// [ts] hash = (hash * 33) ^ str.charCodeAt(--i);
// [ts] }
// [ts] return hash >> 0
// [ts] }
// [ts] fn containsSlotChild(el: auto) {
// [ts] if (el.type == 1) {
// [ts] if (el.tag == "slot") {
// [ts] return true
// [ts] }
// [ts] return el.children.some(containsSlotChild)
// [ts] }
// [ts] return false
// [ts] }
// [ts] fn genScopedSlot(el: auto, state: auto) {
// [ts] let isLegacySyntax = el.attrsMap["slot-scope"];
// [ts] if (el.if && !el.ifProcessed && !isLegacySyntax) {
// [ts] return genIf(el, state, genScopedSlot, "null")
// [ts] }
// [ts] if (el.for && !el.forProcessed) {
// [ts] return genFor(el, state, genScopedSlot)
// [ts] }
// [ts] let slotScope = if (el.slotScope == emptySlotScopeToken) "" else String(el.slotScope);
// [ts] let fn = "function(" + slotScope + "){" +
// [ts] "return " + (if (el.tag == "template") if (el.if && isLegacySyntax) ("(" + (el.if) + ")?" + (genChildren(el, state) || "undefined") + ":undefined") else genChildren(el, state) || "undefined" else genElement(el, state)) + "}";
// reverse proxy v-slot without scope on this.$slots
// [ts] let reverseProxy = if (slotScope) "" else ",proxy:true";
// [ts] return ("{key:" + (el.slotTarget || "\"default\"") + ",fn:" + fn + reverseProxy + "}")
// [ts] }
// [ts] fn genChildren(el: auto, state: auto, checkSkip: auto, altGenElement: auto, altGenNode: auto) {
// [ts] let children = el.children;
// [ts] if (children.length) {
// [ts] let el$1 = children[0];
// optimize single v-for
// [ts] if (children.length == 1 &&
// [ts] el$1.for &&
// [ts] el$1.tag != "template" &&
// [ts] el$1.tag != "slot"
// [ts] ) {
// [ts] let normalizationType = if (checkSkip) if (state.maybeComponent(el$1)) ",1" else ",0" else "";
// [ts] return ("" + ((altGenElement || genElement)(el$1, state)) + normalizationType)
// [ts] }
// [ts] let normalizationType$1 = if (checkSkip) getNormalizationType(children, state.maybeComponent) else 0;
// [ts] let gen = altGenNode || genNode;
// [ts] return ("[" + (children.map((c) => { return gen(c, state); }).join(",")) + "]" + (if (normalizationType$1) ("," + normalizationType$1) else ""))
// [ts] }
// [ts] }
// determine the normalization needed for the children array.
// 0: no normalization needed
// 1: simple normalization needed (possible 1-level deep nested array)
// 2: full normalization needed
// [ts] fn getNormalizationType(children: auto, maybeComponent: auto) {
// [ts] let res = 0;
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] let el = children[i];
// [ts] if (el.type != 1) {
// [ts] continue
// [ts] }
// [ts] if (needsNormalization(el) ||
// [ts] (el.ifConditions && el.ifConditions.some((c) => { return needsNormalization(c.block); }))) {
// [ts] res = 2;
// [ts] break
// [ts] }
// [ts] if (maybeComponent(el) ||
// [ts] (el.ifConditions && el.ifConditions.some((c) => { return maybeComponent(c.block); }))) {
// [ts] res = 1;
// [ts] }
// [ts] }
// [ts] return res
// [ts] }
// [ts] fn needsNormalization(el: auto) {
// [ts] return el.for != null || el.tag == "template" || el.tag == "slot"
// [ts] }
// [ts] fn genNode(node: auto, state: auto) {
// [ts] if (node.type == 1) {
// [ts] return genElement(node, state)
// [ts] } else if (node.type == 3 && node.isComment) {
// [ts] return genComment(node)
// [ts] } else {
// [ts] return genText(node)
// [ts] }
// [ts] }
// [ts] fn genText(text: auto) {
// [ts] return ("_v(" + (text.type == 2 ? text.expression // no need for () because already wrapped in _s() : transformSpecialNewlines(JSON.stringify(text.text))) + ")")
// [ts] }
// [ts] fn genComment(comment: auto) {
// [ts] return ("_e(" + (JSON.stringify(comment.text)) + ")")
// [ts] }
// [ts] fn genSlot(el: auto, state: auto) {
// [ts] let slotName = el.slotName || "\"default\"";
// [ts] let children = genChildren(el, state);
// [ts] let res = "_t(" + slotName + (if (children) (",function(){return " + children + "}") else "");
// [ts] let attrs = el.attrs || el.dynamicAttrs ? genProps((el.attrs || []).concat(el.dynamicAttrs || []).map((attr) => { return ({
// slot props are camelized
// [ts] name: camelize(attr.name),
// [ts] value: attr.value,
// [ts] dynamic: attr.dynamic
// [ts] }); })) : null;
// [ts] let bind$$1 = el.attrsMap["v-bind"];
// [ts] if ((attrs || bind$$1) && !children) {
// [ts] res += ",null";
// [ts] }
// [ts] if (attrs) {
// [ts] res += "," + attrs;
// [ts] }
// [ts] if (bind$$1) {
// [ts] res += (if (attrs) "" else ",null") + "," + bind$$1;
// [ts] }
// [ts] return res + ")"
// [ts] }
// componentName is el.component, take it as argument to shun flow's pessimistic refinement
// [ts] fn genComponent(componentName: auto, el: auto, state: auto) {
// [ts] let children = if (el.inlineTemplate) null else genChildren(el, state, true);
// [ts] return ("_c(" + componentName + "," + (genData$2(el, state)) + (if (children) ("," + children) else "") + ")")
// [ts] }
// [ts] fn genProps(props: auto) {
// [ts] let staticProps = "";
// [ts] let dynamicProps = "";
// [ts] let i = 0;
// [ts] while (i < props.length) { // [cap-migrate] for-step: i++
// [ts] let prop = props[i];
// [ts] let value = transformSpecialNewlines(prop.value);
// [ts] if (prop.dynamic) {
// [ts] dynamicProps += (prop.name) + "," + value + ",";
// [ts] } else {
// [ts] staticProps += "\"" + (prop.name) + "\":" + value + ",";
// [ts] }
// [ts] }
// [ts] staticProps = "{" + (staticProps.slice(0, -1)) + "}";
// [ts] if (dynamicProps) {
// [ts] return ("_d(" + staticProps + ",[" + (dynamicProps.slice(0, -1)) + "])")
// [ts] } else {
// [ts] return staticProps
// [ts] }
// [ts] }
// #3895, #4268
// [ts] fn transformSpecialNewlines(text: auto) {
// [ts] return text
// [ts] .replace("\\u2028", "\\u2028")
// [ts] .replace("\\u2029", "\\u2029")
// [ts] }
// [ts] /*  */
// these keywords should not appear inside expressions, but operators like
// typeof, instanceof and in are allowed
// [ts] let prohibitedKeywordRE = RegExp.init("\\b" + (
// [ts] "do,if,for,let,new,try,var,case,else,with,await,break,catch,class,const," +
// [ts] "super,throw,while,yield,delete,export,import,return,switch,default," +
// [ts] "extends,finally,continue,debugger,function,arguments"
// [ts] ).split(",").join("\\b|\\b") + "\\b");
// these unary operators should not be used as property/method names
// [ts] let unaryOperatorsRE = RegExp.init("\\b" + (
// [ts] "delete,typeof,void"
// [ts] ).split(",").join("\\s*\\([^\\)]*\\)|\\b") + "\\s*\\([^\\)]*\\)");
// strip strings in expressions
// [ts] let stripStringRE = "'(?:[^'\\\\]|\\\\.)*'|\"(?:[^\"\\\\]|\\\\.)*\"|`(?:[^`\\\\]|\\\\.)*\\$\\{|\\}(?:[^`\\\\]|\\\\.)*`|`(?:[^`\\\\]|\\\\.)*`";
// detect problematic expressions in a template
// [ts] fn detectErrors(ast: auto, warn: auto) {
// [ts] if (ast) {
// [ts] checkNode(ast, warn);
// [ts] }
// [ts] }
// [ts] fn checkNode(node: auto, warn: auto) {
// [ts] if (node.type == 1) {
// [ts] for (let name in node.attrsMap) {
// [ts] if (dirRE.test(name)) {
// [ts] let value = node.attrsMap[name];
// [ts] if (value) {
// [ts] let range = node.rawAttrsMap[name];
// [ts] if (name == "v-for") {
// [ts] checkFor(node, ("v-for=\"" + value + "\""), warn, range);
// [ts] } else if (name == "v-slot" || name[0] == "#") {
// [ts] checkFunctionParameterExpression(value, (name + "=\"" + value + "\""), warn, range);
// [ts] } else if (onRE.test(name)) {
// [ts] checkEvent(value, (name + "=\"" + value + "\""), warn, range);
// [ts] } else {
// [ts] checkExpression(value, (name + "=\"" + value + "\""), warn, range);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (node.children) {
// [ts] let i = 0;
// [ts] while (i < node.children.length) { // [cap-migrate] for-step: i++
// [ts] checkNode(node.children[i], warn);
// [ts] }
// [ts] }
// [ts] } else if (node.type == 2) {
// [ts] checkExpression(node.expression, node.text, warn, node);
// [ts] }
// [ts] }
// [ts] fn checkEvent(exp: auto, text: auto, warn: auto, range: auto) {
// [ts] let stripped = exp.replace(stripStringRE, "");
// [ts] let keywordMatch = stripped.match(unaryOperatorsRE);
// [ts] if (keywordMatch && stripped.charAt(keywordMatch.index - 1) != "$") {
// [ts] warn(
// [ts] "avoid using JavaScript unary operator as property name: " +
// [ts] "\"" + (keywordMatch[0]) + "\" in expression " + (text.trim()),
// [ts] range
// [ts] );
// [ts] }
// [ts] checkExpression(exp, text, warn, range);
// [ts] }
// [ts] fn checkFor(node: auto, text: auto, warn: auto, range: auto) {
// [ts] checkExpression(node.for || "", text, warn, range);
// [ts] checkIdentifier(node.alias, "v-for alias", text, warn, range);
// [ts] checkIdentifier(node.iterator1, "v-for iterator", text, warn, range);
// [ts] checkIdentifier(node.iterator2, "v-for iterator", text, warn, range);
// [ts] }
// [ts] fn checkIdentifier(ident: auto, type: auto, text: auto, warn: auto, range: auto) {
// [ts] if (@typeName(ident) == "string") {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: new Function(("var " + ident + "=_"));
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] warn(("invalid " + type + " \"" + ident + "\" in expression: " + (text.trim())), range);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn checkExpression(exp: auto, text: auto, warn: auto, range: auto) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: new Function(("return " + exp));
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] let keywordMatch = exp.replace(stripStringRE, "").match(prohibitedKeywordRE);
// [ts] if (keywordMatch) {
// [ts] warn(
// [ts] "avoid using JavaScript keyword as property name: " +
// [ts] "\"" + (keywordMatch[0]) + "\"\n  Raw expression: " + (text.trim()),
// [ts] range
// [ts] );
// [ts] } else {
// [ts] warn(
// [ts] "invalid expression: " + (e.message) + " in\n\n" +
// [ts] "    " + exp + "\n\n" +
// [ts] "  Raw expression: " + (text.trim()) + "\n",
// [ts] range
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] fn checkFunctionParameterExpression(exp: auto, text: auto, warn: auto, range: auto) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: new Function(exp, '');
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] warn(
// [ts] "invalid function parameter expression: " + (e.message) + " in\n\n" +
// [ts] "    " + exp + "\n\n" +
// [ts] "  Raw expression: " + (text.trim()) + "\n",
// [ts] range
// [ts] );
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] let range = 2;
// [ts] fn generateCodeFrame(source: auto, start: auto, end: auto) {
// [ts] if ( start == void 0 ) { start = 0; }
// [ts] if ( end == void 0 ) { end = source.length; }
// [ts] let lines = source.split("\\r?\\n");
// [ts] let count = 0;
// [ts] let res = [];
// [ts] let i = 0;
// [ts] while (i < lines.length) { // [cap-migrate] for-step: i++
// [ts] count += lines[i].length + 1;
// [ts] if (count >= start) {
// [ts] let j = i - range;
// [ts] while (j <= i + range || end > count) { // [cap-migrate] for-step: j++
// [ts] if (j < 0 || j >= lines.length) { continue }
// [ts] res.push(("" + (j + 1) + (repeat$1(" ", 3 - String(j + 1).length)) + "|  " + (lines[j])));
// [ts] let lineLength = lines[j].length;
// [ts] if (j == i) {
// push underline
// [ts] let pad = start - (count - lineLength) + 1;
// [ts] let length = if (end > count) lineLength - pad else end - start;
// [ts] res.push("   |  " + repeat$1(" ", pad) + repeat$1("^", length));
// [ts] } else if (j > i) {
// [ts] if (end > count) {
// [ts] let length$1 = Math.min(end - count, lineLength);
// [ts] res.push("   |  " + repeat$1("^", length$1));
// [ts] }
// [ts] count += lineLength + 1;
// [ts] }
// [ts] }
// [ts] break
// [ts] }
// [ts] }
// [ts] return res.join("\n")
// [ts] }
// [ts] (str, n) => {
// [ts] let result = "";
// [ts] if (n > 0) {
// [ts] while (true) { // eslint-disable-line
// [ts] if (n & 1) { result += str; }
// [ts] n >>= 1;
// [ts] if (n <= 0) { break }
// [ts] str += str;
// [ts] }
// [ts] }
// [ts] return result
// [ts] }
// [ts] /*  */
// [ts] fn createFunction(code: auto, errors: auto) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return new Function(code)
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// [ts] errors.push({ err: err, code: code });
// [ts] return noop
// [ts] }
// [ts] }
// [ts] fn createCompileToFunctionFn(compile: auto) {
// [ts] let cache = Object.create(null);
// [ts] return function compileToFunctions (
// [ts] template,
// [ts] options,
// [ts] vm
// [ts] ) {
// [ts] options = extend({}, options);
// [ts] let warn$$1 = options.warn || warn;
// [ts] delete options.warn;
// [ts] /* istanbul ignore if */
// [ts] {
// detect possible CSP restriction
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: new Function('return 1');
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] if (e.toString().match("unsafe-eval|CSP")) {
// [ts] warn$$1(
// [ts] "It seems you are using the standalone build of Vue.js in an " +
// [ts] "environment with Content Security Policy that prohibits unsafe-eval. " +
// [ts] "The template compiler cannot work in this environment. Consider " +
// [ts] "relaxing the policy to allow unsafe-eval or pre-compiling your " +
// [ts] "templates into render functions."
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// check cache
// [ts] let key = if (options.delimiters) String(options.delimiters) + template else template;
// [ts] if (cache[key]) {
// [ts] return cache[key]
// [ts] }
// compile
// [ts] let compiled = compile(template, options);
// check compilation errors/tips
// [ts] {
// [ts] if (compiled.errors && compiled.errors.length) {
// [ts] if (options.outputSourceRange) {
// [ts] compiled.errors.forEach((e) => {
// [ts] warn$$1(
// [ts] "Error compiling template:\n\n" + (e.msg) + "\n\n" +
// [ts] generateCodeFrame(template, e.start, e.end),
// [ts] vm
// [ts] );
// [ts] });
// [ts] } else {
// [ts] warn$$1(
// [ts] "Error compiling template:\n\n" + template + "\n\n" +
// [ts] compiled.errors.map((e) => { return ("- " + e); }).join("\n") + "\n",
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] if (compiled.tips && compiled.tips.length) {
// [ts] if (options.outputSourceRange) {
// [ts] compiled.tips.forEach((e) => { return tip(e.msg, vm); });
// [ts] } else {
// [ts] compiled.tips.forEach((msg) => { return tip(msg, vm); });
// [ts] }
// [ts] }
// [ts] }
// turn code into functions
// [ts] let res = {};
// [ts] let fnGenErrors = [];
// [ts] res.render = createFunction(compiled.render, fnGenErrors);
// [ts] res.staticRenderFns = compiled.staticRenderFns.map((code) => {
// [ts] return createFunction(code, fnGenErrors)
// [ts] });
// check function generation errors.
// this should only happen if there is a bug in the compiler itself.
// mostly for codegen development use
// [ts] /* istanbul ignore if */
// [ts] {
// [ts] if ((!compiled.errors || !compiled.errors.length) && fnGenErrors.length) {
// [ts] warn$$1(
// [ts] "Failed to generate render function:\n\n" +
// [ts] fnGenErrors.map((ref) => {
// [ts] let err = ref.err;
// [ts] let code = ref.code;
// [ts] return ((err.toString()) + " in\n\n" + code + "\n");
// [ts] }).join("\n"),
// [ts] vm
// [ts] );
// [ts] }
// [ts] }
// [ts] return (cache[key] = res)
// [ts] }
// [ts] }
// [ts] /*  */
// [ts] fn createCompilerCreator(baseCompile: auto) {
// [ts] return (baseOptions) => {
// [ts] fn compile(template: auto, options: auto) {
// [ts] let finalOptions = Object.create(baseOptions);
// [ts] let errors = [];
// [ts] let tips = [];
// [ts] let warn = (msg, range, tip) => {
// [ts] (if (tip) tips else errors).push(msg);
// [ts] };
// [ts] if (options) {
// [ts] if (options.outputSourceRange) {
// $flow-disable-line
// [ts] let leadingSpaceLength = template.match("^\\s*")[0].length;
// [ts] warn = (msg, range, tip) => {
// [ts] let data = { msg: msg };
// [ts] if (range) {
// [ts] if (range.start != null) {
// [ts] data.start = range.start + leadingSpaceLength;
// [ts] }
// [ts] if (range.end != null) {
// [ts] data.end = range.end + leadingSpaceLength;
// [ts] }
// [ts] }
// [ts] (if (tip) tips else errors).push(data);
// [ts] };
// [ts] }
// merge custom modules
// [ts] if (options.modules) {
// [ts] finalOptions.modules =
// [ts] (baseOptions.modules || []).concat(options.modules);
// [ts] }
// merge custom directives
// [ts] if (options.directives) {
// [ts] finalOptions.directives = extend(
// [ts] Object.create(baseOptions.directives || null),
// [ts] options.directives
// [ts] );
// [ts] }
// copy other options
// [ts] for (let key in options) {
// [ts] if (key != "modules" && key != "directives") {
// [ts] finalOptions[key] = options[key];
// [ts] }
// [ts] }
// [ts] }
// [ts] finalOptions.warn = warn;
// [ts] let compiled = baseCompile(template.trim(), finalOptions);
// [ts] {
// [ts] detectErrors(compiled.ast, warn);
// [ts] }
// [ts] compiled.errors = errors;
// [ts] compiled.tips = tips;
// [ts] return compiled
// [ts] }
// [ts] return {
// [ts] compile: compile,
// [ts] compileToFunctions: createCompileToFunctionFn(compile)
// [ts] }
// [ts] }
// [ts] }
// [ts] /*  */
// `createCompilerCreator` allows creating compilers that use alternative
// parser/optimizer/codegen, e.g the SSR optimizing compiler.
// Here we just export a default compiler using the default parts.
// [ts] let createCompiler = createCompilerCreator(function baseCompile (
// [ts] template,
// [ts] options
// [ts] ) {
// [ts] let ast = parse(template.trim(), options);
// [ts] if (options.optimize != false) {
// [ts] optimize(ast, options);
// [ts] }
// [ts] let code = generate(ast, options);
// [ts] return {
// [ts] ast: ast,
// [ts] render: code.render,
// [ts] staticRenderFns: code.staticRenderFns
// [ts] }
// [ts] });
// [ts] /*  */
// [ts] let ref$1 = createCompiler(baseOptions);
// [ts] let compile = ref$1.compile;
// [ts] let compileToFunctions = ref$1.compileToFunctions;
// [ts] /*  */
// check whether current browser encodes a char inside attribute values
// [ts] let div;
// [ts] fn getShouldDecode(href: auto) {
// [ts] div = div || document.createElement("div");
// [ts] div.innerHTML = if (href) "<a href=\"\n\"/>" else "<div a=\"\n\"/>";
// [ts] return div.innerHTML.indexOf("&#10;") > 0
// [ts] }
// #3663: IE encodes newlines inside attribute values while other browsers don't
// [ts] let shouldDecodeNewlines = if (inBrowser) getShouldDecode(false) else false;
// #6828: chrome encodes content in a[href]
// [ts] let shouldDecodeNewlinesForHref = if (inBrowser) getShouldDecode(true) else false;
// [ts] /*  */
// [ts] let idToTemplate = cached((id) => {
// [ts] let el = query(id);
// [ts] return el && el.innerHTML
// [ts] });
// [ts] let mount = Vue.prototype.$mount;
// [ts] Vue.prototype.$mount = function (
// [ts] el,
// [ts] hydrating
// [ts] ) {
// [ts] el = el && query(el);
// [ts] /* istanbul ignore if */
// [ts] if (el == document.body || el == document.documentElement) {
// [ts] warn(
// [ts] "Do not mount Vue to <html> or <body> - mount to normal elements instead."
// [ts] );
// [ts] return this
// [ts] }
// [ts] let options = _.$options;
// resolve template/el and convert to render function
// [ts] if (!options.render) {
// [ts] let template = options.template;
// [ts] if (template) {
// [ts] if (@typeName(template) == "string") {
// [ts] if (template.charAt(0) == "#") {
// [ts] template = idToTemplate(template);
// [ts] /* istanbul ignore if */
// [ts] if (!template) {
// [ts] warn(
// [ts] ("Template element not found or is empty: " + (options.template)),
// [ts] this
// [ts] );
// [ts] }
// [ts] }
// [ts] } else if (template.nodeType) {
// [ts] template = template.innerHTML;
// [ts] } else {
// [ts] {
// [ts] warn("invalid template option:" + template, this);
// [ts] }
// [ts] return this
// [ts] }
// [ts] } else if (el) {
// [ts] template = getOuterHTML(el);
// [ts] }
// [ts] if (template) {
// [ts] /* istanbul ignore if */
// [ts] if (config.performance && mark) {
// [ts] mark("compile");
// [ts] }
// [ts] let ref = compileToFunctions(template, {
// [ts] outputSourceRange: "development" != "production",
// [ts] shouldDecodeNewlines: shouldDecodeNewlines,
// [ts] shouldDecodeNewlinesForHref: shouldDecodeNewlinesForHref,
// [ts] delimiters: options.delimiters,
// [ts] comments: options.comments
// [ts] }, this);
// [ts] let render = ref.render;
// [ts] let staticRenderFns = ref.staticRenderFns;
// [ts] options.render = render;
// [ts] options.staticRenderFns = staticRenderFns;
// [ts] /* istanbul ignore if */
// [ts] if (config.performance && mark) {
// [ts] mark("compile end");
// [ts] measure(("vue " + (_._name) + " compile"), "compile", "compile end");
// [ts] }
// [ts] }
// [ts] }
// [ts] return mount.call(this, el, hydrating)
// [ts] };
// [ts] /**
// [ts] * Get outerHTML of elements, taking care
// [ts] * of SVG elements in IE as well.
// [ts] */
// [ts] fn getOuterHTML(el: auto) {
// [ts] if (el.outerHTML) {
// [ts] return el.outerHTML
// [ts] } else {
// [ts] let container = document.createElement("div");
// [ts] container.appendChild(el.cloneNode(true));
// [ts] return container.innerHTML
// [ts] }
// [ts] }
// [ts] Vue.compile = compileToFunctions;
// [ts] return Vue;
// [ts] }));
