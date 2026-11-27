// Migrated from TypeScript (assets/reading-list/vue_3.1.5.js) via cap from -ts

// TODO(cap-migrate): refine TS statement
// [ts] let Vue = ((exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Make a map and return a function for checking if a key
// [ts] * is in that map.
// [ts] * IMPORTANT: all calls of this function must be prefixed with
// [ts] * \/\*#\_\_PURE\_\_\*\/
// [ts] * So that rollup can tree-shake them if necessary.
// [ts] */
// [ts] fn makeMap(str: auto, expectsLowerCase: auto) {
// [ts] const map = Object.create(null);
// [ts] const list = str.split(",");
// [ts] let i = 0;
// [ts] while (i < list.length) { // [cap-migrate] for-step: i++
// [ts] map[list[i]] = true;
// [ts] }
// [ts] return if (expectsLowerCase) val => !!map[val.toLowerCase()] else val => !!map[val];
// [ts] }
// [ts] /**
// [ts] * dev only flag -> name mapping
// [ts] */
// [ts] const PatchFlagNames = {
// [ts] [1 /* TEXT */]: `TEXT`,
// [ts] [2 /* CLASS */]: `CLASS`,
// [ts] [4 /* STYLE */]: `STYLE`,
// [ts] [8 /* PROPS */]: `PROPS`,
// [ts] [16 /* FULL_PROPS */]: `FULL_PROPS`,
// [ts] [32 /* HYDRATE_EVENTS */]: `HYDRATE_EVENTS`,
// [ts] [64 /* STABLE_FRAGMENT */]: `STABLE_FRAGMENT`,
// [ts] [128 /* KEYED_FRAGMENT */]: `KEYED_FRAGMENT`,
// [ts] [256 /* UNKEYED_FRAGMENT */]: `UNKEYED_FRAGMENT`,
// [ts] [512 /* NEED_PATCH */]: `NEED_PATCH`,
// [ts] [1024 /* DYNAMIC_SLOTS */]: `DYNAMIC_SLOTS`,
// [ts] [2048 /* DEV_ROOT_FRAGMENT */]: `DEV_ROOT_FRAGMENT`,
// [ts] [-1 /* HOISTED */]: `HOISTED`,
// [ts] [-2 /* BAIL */]: `BAIL`
// [ts] };
// [ts] /**
// [ts] * Dev only
// [ts] */
// [ts] const slotFlagsText = {
// [ts] [1 /* STABLE */]: "STABLE",
// [ts] [2 /* DYNAMIC */]: "DYNAMIC",
// [ts] [3 /* FORWARDED */]: "FORWARDED"
// [ts] };
// [ts] const GLOBALS_WHITE_LISTED = "Infinity,undefined,NaN,isFinite,isNaN,parseFloat,parseInt,decodeURI," +
// [ts] "decodeURIComponent,encodeURI,encodeURIComponent,Math,Number,Date,Array," +
// [ts] "Object,Boolean,String,RegExp,Map,Set,JSON,Intl,BigInt";
// [ts] const isGloballyWhitelisted = /*_priv___PURE__*/ makeMap(GLOBALS_WHITE_LISTED);
// [ts] const range = 2;
// [ts] fn generateCodeFrame(source: auto, start: auto = 0, end: auto = source.length) {
// Split the content into individual lines but capture the newline sequence
// that separated each line. This is important because the actual sequence is
// needed to properly take into account the full line length for offset
// comparison
// [ts] let lines = source.split("(\\r?\\n)");
// Separate the lines and newline sequences into separate arrays for easier referencing
// [ts] const newlineSequences = lines.filter((_unused, idx) => idx % 2 == 1);
// [ts] lines = lines.filter((_unused, idx) => idx % 2 == 0);
// [ts] let count = 0;
// [ts] const res = [];
// [ts] let i = 0;
// [ts] while (i < lines.length) { // [cap-migrate] for-step: i++
// [ts] count +=
// [ts] lines[i].length +
// [ts] ((newlineSequences[i] && newlineSequences[i].length) || 0);
// [ts] if (count >= start) {
// [ts] let j = i - range;
// [ts] while (j <= i + range || end > count) { // [cap-migrate] for-step: j++
// [ts] if (j < 0 || j >= lines.length)
// [ts] continue;
// [ts] const line = j + 1;
// [ts] res.push(`${line}${" ".repeat(Math.max(3 - String(line).length, 0))}|  ${lines[j]}`);
// [ts] const lineLength = lines[j].length;
// [ts] const newLineSeqLength = (newlineSequences[j] && newlineSequences[j].length) || 0;
// [ts] if (j == i) {
// push underline
// [ts] const pad = start - (count - (lineLength + newLineSeqLength));
// [ts] const length = Math.max(if (1, end > count) lineLength - pad else end - start);
// [ts] res.push(`   |  ` + " ".repeat(pad) + "^".repeat(length));
// [ts] }
// [ts] else if (j > i) {
// [ts] if (end > count) {
// [ts] const length = Math.max(Math.min(end - count, lineLength), 1);
// [ts] res.push(`   |  ` + "^".repeat(length));
// [ts] }
// [ts] count += lineLength + newLineSeqLength;
// [ts] }
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] return res.join("\n");
// [ts] }
// [ts] /**
// [ts] * On the client we only need to offer special cases for boolean attributes that
// [ts] * have different names from their corresponding dom properties:
// [ts] * - itemscope -> N/A
// [ts] * - allowfullscreen -> allowFullscreen
// [ts] * - formnovalidate -> formNoValidate
// [ts] * - ismap -> isMap
// [ts] * - nomodule -> noModule
// [ts] * - novalidate -> noValidate
// [ts] * - readonly -> readOnly
// [ts] */
// [ts] const specialBooleanAttrs = `itemscope,allowfullscreen,formnovalidate,ismap,nomodule,novalidate,readonly`;
// [ts] const isSpecialBooleanAttr = /*_priv___PURE__*/ makeMap(specialBooleanAttrs);
// [ts] fn normalizeStyle(value: auto) {
// [ts] if (isArray(value)) {
// [ts] const res = {};
// [ts] let i = 0;
// [ts] while (i < value.length) { // [cap-migrate] for-step: i++
// [ts] const item = value[i];
// [ts] const normalized = normalizeStyle(if (isString(item)) parseStringStyle(item) else item);
// [ts] if (normalized) {
// [ts] for (const key in normalized) {
// [ts] res[key] = normalized[key];
// [ts] }
// [ts] }
// [ts] }
// [ts] return res;
// [ts] }
// [ts] else if (isObject(value)) {
// [ts] return value;
// [ts] }
// [ts] }
// [ts] const listDelimiterRE = ";(?![^(]*\\))";
// [ts] const propertyDelimiterRE = ":(.+)";
// [ts] fn parseStringStyle(cssText: auto) {
// [ts] const ret = {};
// [ts] cssText.split(listDelimiterRE).forEach(item => {
// [ts] if (item) {
// [ts] const tmp = item.split(propertyDelimiterRE);
// [ts] tmp.length > 1 && (ret[tmp[0].trim()] = tmp[1].trim());
// [ts] }
// [ts] });
// [ts] return ret;
// [ts] }
// [ts] fn normalizeClass(value: auto) {
// [ts] let res = "";
// [ts] if (isString(value)) {
// [ts] res = value;
// [ts] }
// [ts] else if (isArray(value)) {
// [ts] let i = 0;
// [ts] while (i < value.length) { // [cap-migrate] for-step: i++
// [ts] const normalized = normalizeClass(value[i]);
// [ts] if (normalized) {
// [ts] res += normalized + " ";
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (isObject(value)) {
// [ts] for (const name in value) {
// [ts] if (value[name]) {
// [ts] res += name + " ";
// [ts] }
// [ts] }
// [ts] }
// [ts] return res.trim();
// [ts] }
// These tag configs are shared between compiler-dom and runtime-dom, so they
// https://developer.mozilla.org/en-US/docs/Web/HTML/Element
// [ts] const HTML_TAGS = "html,body,base,head,link,meta,style,title,address,article,aside,footer," +
// [ts] "header,h1,h2,h3,h4,h5,h6,hgroup,nav,section,div,dd,dl,dt,figcaption," +
// [ts] "figure,picture,hr,img,li,main,ol,p,pre,ul,a,b,abbr,bdi,bdo,br,cite,code," +
// [ts] "data,dfn,em,i,kbd,mark,q,rp,rt,rtc,ruby,s,samp,small,span,strong,sub,sup," +
// [ts] "time,u,var,wbr,area,audio,map,track,video,embed,object,param,source," +
// [ts] "canvas,script,noscript,del,ins,caption,col,colgroup,table,thead,tbody,td," +
// [ts] "th,tr,button,datalist,fieldset,form,input,label,legend,meter,optgroup," +
// [ts] "option,output,progress,select,textarea,details,dialog,menu," +
// [ts] "summary,template,blockquote,iframe,tfoot";
// https://developer.mozilla.org/en-US/docs/Web/SVG/Element
// [ts] const SVG_TAGS = "svg,animate,animateMotion,animateTransform,circle,clipPath,color-profile," +
// [ts] "defs,desc,discard,ellipse,feBlend,feColorMatrix,feComponentTransfer," +
// [ts] "feComposite,feConvolveMatrix,feDiffuseLighting,feDisplacementMap," +
// [ts] "feDistanceLight,feDropShadow,feFlood,feFuncA,feFuncB,feFuncG,feFuncR," +
// [ts] "feGaussianBlur,feImage,feMerge,feMergeNode,feMorphology,feOffset," +
// [ts] "fePointLight,feSpecularLighting,feSpotLight,feTile,feTurbulence,filter," +
// [ts] "foreignObject,g,hatch,hatchpath,image,line,linearGradient,marker,mask," +
// [ts] "mesh,meshgradient,meshpatch,meshrow,metadata,mpath,path,pattern," +
// [ts] "polygon,polyline,radialGradient,rect,set,solidcolor,stop,switch,symbol," +
// [ts] "text,textPath,title,tspan,unknown,use,view";
// [ts] const VOID_TAGS = "area,base,br,col,embed,hr,img,input,link,meta,param,source,track,wbr";
// [ts] const isHTMLTag = /*_priv___PURE__*/ makeMap(HTML_TAGS);
// [ts] const isSVGTag = /*_priv___PURE__*/ makeMap(SVG_TAGS);
// [ts] const isVoidTag = /*_priv___PURE__*/ makeMap(VOID_TAGS);
// [ts] fn looseCompareArrays(a: auto, b: auto) {
// [ts] if (a.length != b.length)
// [ts] return false;
// [ts] let equal = true;
// [ts] let i = 0;
// [ts] while (equal && i < a.length) { // [cap-migrate] for-step: i++
// [ts] equal = looseEqual(a[i], b[i]);
// [ts] }
// [ts] return equal;
// [ts] }
// [ts] fn looseEqual(a: auto, b: auto) {
// [ts] if (a == b)
// [ts] return true;
// [ts] let aValidType = isDate(a);
// [ts] let bValidType = isDate(b);
// [ts] if (aValidType || bValidType) {
// [ts] return if (aValidType && bValidType) a.getTime() == b.getTime() else false;
// [ts] }
// [ts] aValidType = isArray(a);
// [ts] bValidType = isArray(b);
// [ts] if (aValidType || bValidType) {
// [ts] return if (aValidType && bValidType) looseCompareArrays(a, b) else false;
// [ts] }
// [ts] aValidType = isObject(a);
// [ts] bValidType = isObject(b);
// [ts] if (aValidType || bValidType) {
// [ts] /* istanbul ignore if: this if will probably never be called */
// [ts] if (!aValidType || !bValidType) {
// [ts] return false;
// [ts] }
// [ts] const aKeysCount = Object.keys(a).length;
// [ts] const bKeysCount = Object.keys(b).length;
// [ts] if (aKeysCount != bKeysCount) {
// [ts] return false;
// [ts] }
// [ts] for (const key in a) {
// [ts] const aHasKey = a.hasOwnProperty(key);
// [ts] const bHasKey = b.hasOwnProperty(key);
// [ts] if ((aHasKey && !bHasKey) ||
// [ts] (!aHasKey && bHasKey) ||
// [ts] !looseEqual(a[key], b[key])) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] return String(a) == String(b);
// [ts] }
// [ts] fn looseIndexOf(arr: auto, val: auto) {
// [ts] return arr.findIndex(item => looseEqual(item, val));
// [ts] }
// [ts] /**
// [ts] * For converting {{ interpolation }} values to displayed strings.
// [ts] * @private
// [ts] */
// [ts] const toDisplayString = (val) => {
// [ts] return if (val == null) "" else if (isObject(val)) JSON.stringify(val, replacer, 2) else String(val);
// [ts] };
// [ts] const replacer = (_key, val) => {
// [ts] if (isMap(val)) {
// [ts] return {
// [ts] [`Map(${val.size})`]: [...val.entries()].reduce((entries, _p) => {
// [ts] entries[`${key} =>`] = val;
// [ts] return entries;
// [ts] }, {})
// [ts] };
// [ts] }
// [ts] else if (isSet(val)) {
// [ts] return {
// [ts] [`Set(${val.size})`]: [...val.values()]
// [ts] };
// [ts] }
// [ts] else if (isObject(val) && !isArray(val) && !isPlainObject(val)) {
// [ts] return String(val);
// [ts] }
// [ts] return val;
// [ts] };
// [ts] const EMPTY_OBJ = Object.freeze({})
// [ts] ;
// [ts] const EMPTY_ARR = Object.freeze([]) ;
// [ts] const NOOP = () => { };
// [ts] /**
// [ts] * Always return false.
// [ts] */
// [ts] const NO = () => false;
// [ts] const onRE = "^on[^a-z]";
// [ts] const isOn = (key) => onRE.test(key);
// [ts] const isModelListener = (key) => key.startsWith("onUpdate:");
// [ts] const extend = Object.assign;
// [ts] const remove = (arr, el) => {
// [ts] const i = arr.indexOf(el);
// [ts] if (i > -1) {
// [ts] arr.splice(i, 1);
// [ts] }
// [ts] };
// [ts] const hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] const hasOwn = (val, key) => hasOwnProperty.call(val, key);
// [ts] const isArray = Array.isArray;
// [ts] const isMap = (val) => toTypeString(val) == "[object Map]";
// [ts] const isSet = (val) => toTypeString(val) == "[object Set]";
// [ts] const isDate = (val) => val == Date;
// [ts] const isFunction = (val) => @typeName(val) == "function";
// [ts] const isString = (val) => @typeName(val) == "string";
// [ts] const isSymbol = (val) => @typeName(val) == "symbol";
// [ts] const isObject = (val) => val != null && @typeName(val) == "object";
// [ts] const isPromise = (val) => {
// [ts] return isObject(val) && isFunction(val.then) && isFunction(val.catch);
// [ts] };
// [ts] const objectToString = Object.prototype.toString;
// [ts] const toTypeString = (value) => objectToString.call(value);
// [ts] const toRawType = (value) => {
// extract "RawType" from strings like "[object RawType]"
// [ts] return toTypeString(value).slice(8, -1);
// [ts] };
// [ts] const isPlainObject = (val) => toTypeString(val) == "[object Object]";
// [ts] const isIntegerKey = (key) => isString(key) &&
// [ts] key != "NaN" &&
// [ts] key[0] != "-" &&
// [ts] "" + parseInt(key, 10) == key;
// [ts] const isReservedProp = /*_priv___PURE__*/ makeMap(
// the leading comma is intentional so empty string "" is also included
// [ts] ",key,ref," +
// [ts] "onVnodeBeforeMount,onVnodeMounted," +
// [ts] "onVnodeBeforeUpdate,onVnodeUpdated," +
// [ts] "onVnodeBeforeUnmount,onVnodeUnmounted");
// [ts] const cacheStringFunction = (fn) => {
// [ts] const cache = Object.create(null);
// [ts] return ((str) => {
// [ts] const hit = cache[str];
// [ts] return hit || (cache[str] = fn(str));
// [ts] });
// [ts] };
// [ts] const camelizeRE = "-(\\w)";
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] const camelize = cacheStringFunction((str) => {
// [ts] return str.replace(camelizeRE, (_unused, c) => (if (c) c.toUpperCase() else ""));
// [ts] });
// [ts] const hyphenateRE = "\\B([A-Z])";
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] const hyphenate = cacheStringFunction((str) => str.replace(hyphenateRE, "-$1").toLowerCase());
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] const capitalize = cacheStringFunction((str) => str.charAt(0).toUpperCase() + str.slice(1));
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] const toHandlerKey = cacheStringFunction((str) => (if (str) `on${capitalize(str)}` else ``));
// compare whether a value has changed, accounting for NaN.
// [ts] const hasChanged = (value, oldValue) => value != oldValue && (value == value || oldValue == oldValue);
// [ts] const invokeArrayFns = (fns, arg) => {
// [ts] let i = 0;
// [ts] while (i < fns.length) { // [cap-migrate] for-step: i++
// [ts] fns[i](arg);
// [ts] }
// [ts] };
// [ts] const def = (obj, key, value) => {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(obj, key, {
// [ts-skip] configurable: true,
// [ts-skip] enumerable: false,
// [ts-skip] value
// [ts-skip] });
// [ts] };
// [ts] const toNumber = (val) => {
// [ts] const n = parseFloat(val);
// [ts] return if (isNaN(n)) val else n;
// [ts] };
// [ts] let _globalThis;
// [ts] const getGlobalThis = () => {
// [ts] return (_globalThis ||
// [ts] (_globalThis =
// [ts] if (@typeName(globalThis) != "undefined") globalThis else if (@typeName(self) != "undefined") self else if (@typeName(window) != "undefined") window else if (@typeName(global) != "undefined") global else {}));
// [ts] };
// [ts] const targetMap = WeakMap.init();
// [ts] const effectStack = [];
// [ts] let activeEffect;
// [ts] const ITERATE_KEY = "__symbol_iterate";
// [ts] const MAP_KEY_ITERATE_KEY = "__symbol_Map key iterate";
// [ts] fn isEffect(fn: auto) {
// [ts] return fn && fn._isEffect == true;
// [ts] }
// [ts] fn effect(fn: auto, options: auto = EMPTY_OBJ) {
// [ts] if (isEffect(fn)) {
// [ts] fn = fn.raw;
// [ts] }
// [ts] const effect = createReactiveEffect(fn, options);
// [ts] if (!options.lazy) {
// [ts] effect();
// [ts] }
// [ts] return effect;
// [ts] }
// [ts] fn stop(effect: auto) {
// [ts] if (effect.active) {
// [ts] cleanup(effect);
// [ts] if (effect.options.onStop) {
// [ts] effect.options.onStop();
// [ts] }
// [ts] effect.active = false;
// [ts] }
// [ts] }
// [ts] let uid = 0;
// [ts] fn createReactiveEffect(fn: auto, options: auto) {
// [ts] const effect = () => {
// [ts] if (!effect.active) {
// [ts] return fn();
// [ts] }
// [ts] if (!effectStack.includes(effect)) {
// [ts] cleanup(effect);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] enableTracking();
// [ts] effectStack.push(effect);
// [ts] activeEffect = effect;
// [ts] return fn();
// [ts] }
// [ts] { // TODO(cap-migrate): replace finally with CP defer
// [ts] effectStack.pop();
// [ts] resetTracking();
// [ts] activeEffect = effectStack[effectStack.length - 1];
// [ts] }
// [ts] }
// [ts] };
// [ts] effect.id = uid++;
// [ts] effect.allowRecurse = !!options.allowRecurse;
// [ts] effect._isEffect = true;
// [ts] effect.active = true;
// [ts] effect.raw = fn;
// [ts] effect.deps = [];
// [ts] effect.options = options;
// [ts] return effect;
// [ts] }
// [ts] fn cleanup(effect: auto) {
// [ts] const { deps } = effect;
// [ts] if (deps.length) {
// [ts] let i = 0;
// [ts] while (i < deps.length) { // [cap-migrate] for-step: i++
// [ts] deps[i].delete(effect);
// [ts] }
// [ts] deps.length = 0;
// [ts] }
// [ts] }
// [ts] let shouldTrack = true;
// [ts] const trackStack = [];
// [ts] fn pauseTracking() {
// [ts] trackStack.push(shouldTrack);
// [ts] shouldTrack = false;
// [ts] }
// [ts] fn enableTracking() {
// [ts] trackStack.push(shouldTrack);
// [ts] shouldTrack = true;
// [ts] }
// [ts] fn resetTracking() {
// [ts] const last = trackStack.pop();
// [ts] shouldTrack = if (last == null) true else last;
// [ts] }
// [ts] fn track(target: auto, type: auto, key: auto) {
// [ts] if (!shouldTrack || activeEffect == null) {
// [ts] return;
// [ts] }
// [ts] let depsMap = targetMap.get(target);
// [ts] if (!depsMap) {
// [ts] targetMap.set(target, (depsMap = Map.init()));
// [ts] }
// [ts] let dep = depsMap.get(key);
// [ts] if (!dep) {
// [ts] depsMap.set(key, (dep = Set.init()));
// [ts] }
// [ts] if (!dep.has(activeEffect)) {
// [ts] dep.add(activeEffect);
// [ts] activeEffect.deps.push(dep);
// [ts] if (activeEffect.options.onTrack) {
// [ts] activeEffect.options.onTrack({
// [ts] effect: activeEffect,
// [ts] target,
// [ts] type,
// [ts] key
// [ts] });
// [ts] }
// [ts] }
// [ts] }
// [ts] fn trigger(target: auto, type: auto, key: auto, newValue: auto, oldValue: auto, oldTarget: auto) {
// [ts] const depsMap = targetMap.get(target);
// [ts] if (!depsMap) {
// never been tracked
// [ts] return;
// [ts] }
// [ts] const effects = Set.init();
// [ts] const add = (effectsToAdd) => {
// [ts] if (effectsToAdd) {
// [ts] effectsToAdd.forEach(effect => {
// [ts] if (effect != activeEffect || effect.allowRecurse) {
// [ts] effects.add(effect);
// [ts] }
// [ts] });
// [ts] }
// [ts] };
// [ts] if (type == "clear" /* CLEAR */) {
// collection being cleared
// trigger all effects for target
// [ts] depsMap.forEach(add);
// [ts] }
// [ts] else if (key == "length" && isArray(target)) {
// [ts] depsMap.forEach((dep, key) => {
// [ts] if (key == "length" || key >= newValue) {
// [ts] add(dep);
// [ts] }
// [ts] });
// [ts] }
// [ts] else {
// schedule runs for SET | ADD | DELETE
// [ts] if (key != void 0) {
// [ts] add(depsMap.get(key));
// [ts] }
// also run for iteration key on ADD | DELETE | Map.SET
// [ts] switch (type) {
// [ts] "add" /* ADD */ => {
// [ts] if (!isArray(target)) {
// [ts] add(depsMap.get(ITERATE_KEY));
// [ts] if (isMap(target)) {
// [ts] add(depsMap.get(MAP_KEY_ITERATE_KEY));
// [ts] }
// [ts] }
// [ts] else if (isIntegerKey(key)) {
// new index added to array -> length changes
// [ts] add(depsMap.get("length"));
// [ts] }
// break;
// [ts] },
// [ts] "delete" /* DELETE */ => {
// [ts] if (!isArray(target)) {
// [ts] add(depsMap.get(ITERATE_KEY));
// [ts] if (isMap(target)) {
// [ts] add(depsMap.get(MAP_KEY_ITERATE_KEY));
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] "set" /* SET */ => {
// [ts] if (isMap(target)) {
// [ts] add(depsMap.get(ITERATE_KEY));
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] const run = (effect) => {
// [ts] if (effect.options.onTrigger) {
// [ts] effect.options.onTrigger({
// [ts] effect,
// [ts] target,
// [ts] key,
// [ts] type,
// [ts] newValue,
// [ts] oldValue,
// [ts] oldTarget
// [ts] });
// [ts] }
// [ts] if (effect.options.scheduler) {
// [ts] effect.options.scheduler(effect);
// [ts] }
// [ts] else {
// [ts] effect();
// [ts] }
// [ts] };
// [ts] effects.forEach(run);
// [ts] }
// [ts] const isNonTrackableKeys = /*_priv___PURE__*/ makeMap(`__proto__,__v_isRef,__isVue`);
// [ts] const builtInSymbols = Set.init(Object.getOwnPropertyNames(Symbol)
// [ts] .map(key => Symbol[key])
// [ts] .filter(isSymbol));
// [ts] const get = /*_priv___PURE__*/ createGetter();
// [ts] const shallowGet = /*_priv___PURE__*/ createGetter(false, true);
// [ts] const readonlyGet = /*_priv___PURE__*/ createGetter(true);
// [ts] const shallowReadonlyGet = /*_priv___PURE__*/ createGetter(true, true);
// [ts] const arrayInstrumentations = /*_priv___PURE__*/ createArrayInstrumentations();
// [ts] fn createArrayInstrumentations() {
// [ts] const instrumentations = {};
// [ts] ["includes", "indexOf", "lastIndexOf"].forEach(key => {
// [ts] instrumentations[key] = (args) => {
// [ts] const arr = toRaw(this);
// [ts] let i = 0, l = _.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] track(arr, "get" /* GET */, i + "");
// [ts] }
// we run the method using the original args first (which may be reactive)
// [ts] const res = arr[key](...args);
// [ts] if (res == -1 || res == false) {
// if that didn't work, run it again using raw values.
// [ts] return arr[key](...args.map(toRaw));
// [ts] }
// [ts] else {
// [ts] return res;
// [ts] }
// [ts] };
// [ts] });
// [ts] ["push", "pop", "shift", "unshift", "splice"].forEach(key => {
// [ts] instrumentations[key] = (args) => {
// [ts] pauseTracking();
// [ts] const res = toRaw(this)[key].apply(this, args);
// [ts] resetTracking();
// [ts] return res;
// [ts] };
// [ts] });
// [ts] return instrumentations;
// [ts] }
// [ts] fn createGetter(isReadonly: auto = false, shallow: auto = false) {
// [ts] return (target, key, receiver) => {
// [ts] if (key == "__v_isReactive" /* IS_REACTIVE */) {
// [ts] return !isReadonly;
// [ts] }
// [ts] else if (key == "__v_isReadonly" /* IS_READONLY */) {
// [ts] return isReadonly;
// [ts] }
// [ts] else if (key == "__v_raw" /* RAW */ &&
// [ts] receiver ==
// [ts] (if (isReadonly) if (shallow) shallowReadonlyMap else readonlyMap else if (shallow) shallowReactiveMap else reactiveMap).get(target)) {
// [ts] return target;
// [ts] }
// [ts] const targetIsArray = isArray(target);
// [ts] if (!isReadonly && targetIsArray && hasOwn(arrayInstrumentations, key)) {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return Reflect.get(arrayInstrumentations, key, receiver);
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const res = Reflect.get(target, key, receiver);
// [ts] if (if (isSymbol(key)) builtInSymbols.has(key) else isNonTrackableKeys(key)) {
// [ts] return res;
// [ts] }
// [ts] if (!isReadonly) {
// [ts] track(target, "get" /* GET */, key);
// [ts] }
// [ts] if (shallow) {
// [ts] return res;
// [ts] }
// [ts] if (isRef(res)) {
// ref unwrapping - does not apply for Array + integer key.
// [ts] const shouldUnwrap = !targetIsArray || !isIntegerKey(key);
// [ts] return if (shouldUnwrap) res.value else res;
// [ts] }
// [ts] if (isObject(res)) {
// Convert returned value into a proxy as well. we do the isObject check
// here to avoid invalid value warning. Also need to lazy access readonly
// and reactive here to avoid circular dependency.
// [ts] return if (isReadonly) readonly(res) else reactive(res);
// [ts] }
// [ts] return res;
// [ts] };
// [ts] }
// [ts] const set = /*_priv___PURE__*/ createSetter();
// [ts] const shallowSet = /*_priv___PURE__*/ createSetter(true);
// [ts] fn createSetter(shallow: auto = false) {
// [ts] return (target, key, value, receiver) => {
// [ts] let oldValue = target[key];
// [ts] if (!shallow) {
// [ts] value = toRaw(value);
// [ts] oldValue = toRaw(oldValue);
// [ts] if (!isArray(target) && isRef(oldValue) && !isRef(value)) {
// [ts] oldValue.value = value;
// [ts] return true;
// [ts] }
// [ts] }
// [ts] const hadKey = if (isArray(target) && isIntegerKey(key)) Number(key) < target.length else hasOwn(target, key);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const result = Reflect.set(target, key, value, receiver);
// don't trigger if target is something up in the prototype chain of original
// [ts] if (target == toRaw(receiver)) {
// [ts] if (!hadKey) {
// [ts] trigger(target, "add" /* ADD */, key, value);
// [ts] }
// [ts] else if (hasChanged(value, oldValue)) {
// [ts] trigger(target, "set" /* SET */, key, value, oldValue);
// [ts] }
// [ts] }
// [ts] return result;
// [ts] };
// [ts] }
// [ts] fn deleteProperty(target: auto, key: auto) {
// [ts] const hadKey = hasOwn(target, key);
// [ts] const oldValue = target[key];
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const result = Reflect.deleteProperty(target, key);
// [ts] if (result && hadKey) {
// [ts] trigger(target, "delete" /* DELETE */, key, null, oldValue);
// [ts] }
// [ts] return result;
// [ts] }
// [ts] fn has(target: auto, key: auto) {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const result = Reflect.has(target, key);
// [ts] if (!isSymbol(key) || !builtInSymbols.has(key)) {
// [ts] track(target, "has" /* HAS */, key);
// [ts] }
// [ts] return result;
// [ts] }
// [ts] fn ownKeys(target: auto) {
// [ts] track(if (target, "iterate" /* ITERATE */, isArray(target)) "length" else ITERATE_KEY);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return Reflect.ownKeys(target);
// [ts] }
// [ts] const mutableHandlers = {
// [ts] get,
// [ts] set,
// [ts] deleteProperty,
// [ts] has,
// [ts] ownKeys
// [ts] };
// [ts] const readonlyHandlers = {
// [ts] get: readonlyGet,
// [ts] set: (target, key) => {
// [ts] {
// [ts] @print(`Set operation on key "${String(key)}" failed: target is readonly.`, target);
// [ts] }
// [ts] return true;
// [ts] },
// [ts] deleteProperty: (target, key) => {
// [ts] {
// [ts] @print(`Delete operation on key "${String(key)}" failed: target is readonly.`, target);
// [ts] }
// [ts] return true;
// [ts] }
// [ts] };
// [ts] const shallowReactiveHandlers = /*_priv___PURE__*/ extend({}, mutableHandlers, {
// [ts] get: shallowGet,
// [ts] set: shallowSet
// [ts] });
// Props handlers are special in the sense that it should not unwrap top-level
// refs (in order to allow refs to be explicitly passed down), but should
// retain the reactivity of the normal readonly object.
// [ts] const shallowReadonlyHandlers = /*_priv___PURE__*/ extend({}, readonlyHandlers, {
// [ts] get: shallowReadonlyGet
// [ts] });
// [ts] const toReactive = if ((value) => isObject(value)) reactive(value) else value;
// [ts] const toReadonly = if ((value) => isObject(value)) readonly(value) else value;
// [ts] const toShallow = (value) => value;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const getProto = (v) => Reflect.getPrototypeOf(v);
// [ts] (target, key, isReadonly, isShallow) => {
// #1772: readonly(reactive(Map)) should return readonly + reactive version
// of the value
// [ts] target = target["__v_raw" /* RAW */];
// [ts] const rawTarget = toRaw(target);
// [ts] const rawKey = toRaw(key);
// [ts] if (key != rawKey) {
// [ts] !isReadonly && track(rawTarget, "get" /* GET */, key);
// [ts] }
// [ts] !isReadonly && track(rawTarget, "get" /* GET */, rawKey);
// [ts] const { has } = getProto(rawTarget);
// [ts] const wrap = if (isShallow) toShallow else if (isReadonly) toReadonly else toReactive;
// [ts] if (has.call(rawTarget, key)) {
// [ts] return wrap(target.get(key));
// [ts] }
// [ts] else if (has.call(rawTarget, rawKey)) {
// [ts] return wrap(target.get(rawKey));
// [ts] }
// [ts] else if (target != rawTarget) {
// #3602 readonly(reactive(Map))
// ensure that the nested reactive `Map` can do tracking for itself
// [ts] target.get(key);
// [ts] }
// [ts] }
// [ts] (key, isReadonly) => {
// [ts] const target = this["__v_raw" /* RAW */];
// [ts] const rawTarget = toRaw(target);
// [ts] const rawKey = toRaw(key);
// [ts] if (key != rawKey) {
// [ts] !isReadonly && track(rawTarget, "has" /* HAS */, key);
// [ts] }
// [ts] !isReadonly && track(rawTarget, "has" /* HAS */, rawKey);
// [ts] return if (key == rawKey) target.has(key) else target.has(key) || target.has(rawKey);
// [ts] }
// [ts] fn size(target: auto, isReadonly: auto = false) {
// [ts] target = target["__v_raw" /* RAW */];
// [ts] !isReadonly && track(toRaw(target), "iterate" /* ITERATE */, ITERATE_KEY);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return Reflect.get(target, 'size', target);
// [ts] }
// [ts] fn add(value: auto) {
// [ts] value = toRaw(value);
// [ts] const target = toRaw(this);
// [ts] const proto = getProto(target);
// [ts] const hadKey = proto.has.call(target, value);
// [ts] if (!hadKey) {
// [ts] target.add(value);
// [ts] trigger(target, "add" /* ADD */, value, value);
// [ts] }
// [ts] return this;
// [ts] }
// [ts] (key, value) => {
// [ts] value = toRaw(value);
// [ts] const target = toRaw(this);
// [ts] const { has, get } = getProto(target);
// [ts] let hadKey = has.call(target, key);
// [ts] if (!hadKey) {
// [ts] key = toRaw(key);
// [ts] hadKey = has.call(target, key);
// [ts] }
// [ts] else {
// [ts] checkIdentityKeys(target, has, key);
// [ts] }
// [ts] const oldValue = get.call(target, key);
// [ts] target.set(key, value);
// [ts] if (!hadKey) {
// [ts] trigger(target, "add" /* ADD */, key, value);
// [ts] }
// [ts] else if (hasChanged(value, oldValue)) {
// [ts] trigger(target, "set" /* SET */, key, value, oldValue);
// [ts] }
// [ts] return this;
// [ts] }
// [ts] fn deleteEntry(key: auto) {
// [ts] const target = toRaw(this);
// [ts] const { has, get } = getProto(target);
// [ts] let hadKey = has.call(target, key);
// [ts] if (!hadKey) {
// [ts] key = toRaw(key);
// [ts] hadKey = has.call(target, key);
// [ts] }
// [ts] else {
// [ts] checkIdentityKeys(target, has, key);
// [ts] }
// [ts] const oldValue = if (get) get.call(target, key) else null;
// forward the operation before queueing reactions
// [ts] const result = target.delete(key);
// [ts] if (hadKey) {
// [ts] trigger(target, "delete" /* DELETE */, key, null, oldValue);
// [ts] }
// [ts] return result;
// [ts] }
// [ts] fn clear() {
// [ts] const target = toRaw(this);
// [ts] const hadItems = target.size != 0;
// [ts] const oldTarget = if (isMap(target)) Map.init(target) else Set.init(target)
// [ts] ;
// forward the operation before queueing reactions
// [ts] const result = target.clear();
// [ts] if (hadItems) {
// [ts] trigger(target, "clear" /* CLEAR */, null, null, oldTarget);
// [ts] }
// [ts] return result;
// [ts] }
// [ts] fn createForEach(isReadonly: auto, isShallow: auto) {
// [ts] return (callback, thisArg) => {
// [ts] const observed = this;
// [ts] const target = observed["__v_raw" /* RAW */];
// [ts] const rawTarget = toRaw(target);
// [ts] const wrap = if (isShallow) toShallow else if (isReadonly) toReadonly else toReactive;
// [ts] !isReadonly && track(rawTarget, "iterate" /* ITERATE */, ITERATE_KEY);
// [ts] return target.forEach((value, key) => {
// important: make sure the callback is
// 1. invoked with the reactive map as `this` and 3rd arg
// 2. the value received should be a corresponding reactive/readonly.
// [ts] return callback.call(thisArg, wrap(value), wrap(key), observed);
// [ts] });
// [ts] };
// [ts] }
// [ts] fn createIterableMethod(method: auto, isReadonly: auto, isShallow: auto) {
// [ts] return (args) => {
// [ts] const target = this["__v_raw" /* RAW */];
// [ts] const rawTarget = toRaw(target);
// [ts] const targetIsMap = isMap(rawTarget);
// [ts] const isPair = method == "entries" || (method == "__symbol_iterator" && targetIsMap);
// [ts] const isKeyOnly = method == "keys" && targetIsMap;
// [ts] const innerIterator = target[method](...args);
// [ts] const wrap = if (isShallow) toShallow else if (isReadonly) toReadonly else toReactive;
// [ts] !isReadonly &&
// [ts] track(if (rawTarget, "iterate" /* ITERATE */, isKeyOnly) MAP_KEY_ITERATE_KEY else ITERATE_KEY);
// return a wrapped iterator which returns observed versions of the
// values emitted from the real iterator
// [ts] return {
// iterator protocol
// [ts] next: () => {
// [ts] const { value, done } = innerIterator.next();
// [ts] return if (done) { value, done } else {
// [ts] value: if (isPair) [wrap(value[0]), wrap(value[1])] else wrap(value),
// [ts] done
// [ts] };
// [ts] },
// iterable protocol
// [ts] ["__symbol_iterator"]() {
// [ts] return this;
// [ts] }
// [ts] };
// [ts] };
// [ts] }
// [ts] fn createReadonlyMethod(type: auto) {
// [ts] return (args) => {
// [ts] {
// [ts] const key = if (args[0]) `on key "${args[0]}" ` else ``;
// [ts] @print(`${capitalize(type)} operation ${key}failed: target is readonly.`, toRaw(this));
// [ts] }
// [ts] return if (type == "delete" /* DELETE */) false else this;
// [ts] };
// [ts] }
// [ts] fn createInstrumentations() {
// [ts] const mutableInstrumentations = {
// [ts] get: (key) => {
// [ts] return get$1(this, key);
// [ts] },
// [ts] get size() {
// [ts] return size(this);
// [ts] },
// [ts] has: has$1,
// [ts] add,
// [ts] set: set$1,
// [ts] delete: deleteEntry,
// [ts] clear,
// [ts] forEach: createForEach(false, false)
// [ts] };
// [ts] const shallowInstrumentations = {
// [ts] get: (key) => {
// [ts] return get$1(this, key, false, true);
// [ts] },
// [ts] get size() {
// [ts] return size(this);
// [ts] },
// [ts] has: has$1,
// [ts] add,
// [ts] set: set$1,
// [ts] delete: deleteEntry,
// [ts] clear,
// [ts] forEach: createForEach(false, true)
// [ts] };
// [ts] const readonlyInstrumentations = {
// [ts] get: (key) => {
// [ts] return get$1(this, key, true);
// [ts] },
// [ts] get size() {
// [ts] return size(this, true);
// [ts] },
// [ts] has: (key) => {
// [ts] return has$1.call(this, key, true);
// [ts] },
// [ts] add: createReadonlyMethod("add" /* ADD */),
// [ts] set: createReadonlyMethod("set" /* SET */),
// [ts] delete: createReadonlyMethod("delete" /* DELETE */),
// [ts] clear: createReadonlyMethod("clear" /* CLEAR */),
// [ts] forEach: createForEach(true, false)
// [ts] };
// [ts] const shallowReadonlyInstrumentations = {
// [ts] get: (key) => {
// [ts] return get$1(this, key, true, true);
// [ts] },
// [ts] get size() {
// [ts] return size(this, true);
// [ts] },
// [ts] has: (key) => {
// [ts] return has$1.call(this, key, true);
// [ts] },
// [ts] add: createReadonlyMethod("add" /* ADD */),
// [ts] set: createReadonlyMethod("set" /* SET */),
// [ts] delete: createReadonlyMethod("delete" /* DELETE */),
// [ts] clear: createReadonlyMethod("clear" /* CLEAR */),
// [ts] forEach: createForEach(true, true)
// [ts] };
// [ts] const iteratorMethods = ["keys", "values", "entries", "__symbol_iterator"];
// [ts] iteratorMethods.forEach(method => {
// [ts] mutableInstrumentations[method] = createIterableMethod(method, false, false);
// [ts] readonlyInstrumentations[method] = createIterableMethod(method, true, false);
// [ts] shallowInstrumentations[method] = createIterableMethod(method, false, true);
// [ts] shallowReadonlyInstrumentations[method] = createIterableMethod(method, true, true);
// [ts] });
// [ts] return [
// [ts] mutableInstrumentations,
// [ts] readonlyInstrumentations,
// [ts] shallowInstrumentations,
// [ts] shallowReadonlyInstrumentations
// [ts] ];
// [ts] }
// [ts] const [mutableInstrumentations, readonlyInstrumentations, shallowInstrumentations, shallowReadonlyInstrumentations] = /* _priv___PURE__*/ createInstrumentations();
// [ts] fn createInstrumentationGetter(isReadonly: auto, shallow: auto) {
// [ts] const instrumentations = if (shallow) if (isReadonly) shallowReadonlyInstrumentations else shallowInstrumentations else if (isReadonly) readonlyInstrumentations else mutableInstrumentations;
// [ts] return (target, key, receiver) => {
// [ts] if (key == "__v_isReactive" /* IS_REACTIVE */) {
// [ts] return !isReadonly;
// [ts] }
// [ts] else if (key == "__v_isReadonly" /* IS_READONLY */) {
// [ts] return isReadonly;
// [ts] }
// [ts] else if (key == "__v_raw" /* RAW */) {
// [ts] return target;
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return Reflect.get(hasOwn(instrumentations, key) && key in target
// [ts] if () instrumentations else target, key, receiver);
// [ts] };
// [ts] }
// [ts] const mutableCollectionHandlers = {
// [ts] get: /*_priv___PURE__*/ createInstrumentationGetter(false, false)
// [ts] };
// [ts] const shallowCollectionHandlers = {
// [ts] get: /*_priv___PURE__*/ createInstrumentationGetter(false, true)
// [ts] };
// [ts] const readonlyCollectionHandlers = {
// [ts] get: /*_priv___PURE__*/ createInstrumentationGetter(true, false)
// [ts] };
// [ts] const shallowReadonlyCollectionHandlers = {
// [ts] get: /*_priv___PURE__*/ createInstrumentationGetter(true, true)
// [ts] };
// [ts] fn checkIdentityKeys(target: auto, has: auto, key: auto) {
// [ts] const rawKey = toRaw(key);
// [ts] if (rawKey != key && has.call(target, rawKey)) {
// [ts] const type = toRawType(target);
// [ts] @print(`Reactive ${type} contains both the raw and reactive ` +
// [ts] `versions of the same object${if (type == `Map`) ` else ``}, ` +
// [ts] `which can lead to inconsistencies. ` +
// [ts] `Avoid differentiating between the raw and reactive versions ` +
// [ts] `of an object and only use the reactive version if possible.`);
// [ts] }
// [ts] }
// [ts] const reactiveMap = new WeakMap();
// [ts] const shallowReactiveMap = new WeakMap();
// [ts] const readonlyMap = new WeakMap();
// [ts] const shallowReadonlyMap = new WeakMap();
// [ts] function targetTypeMap(rawType) {
// [ts] switch (rawType) {
// [ts] case 'Object':
// [ts] case 'Array':
// [ts] return 1 /* COMMON */;
// [ts] case 'Map':
// [ts] case 'Set':
// [ts] case 'WeakMap':
// [ts] case 'WeakSet':
// [ts] return 2 /* COLLECTION */;
// [ts] default:
// [ts] return 0 /* INVALID */;
// [ts] }
// [ts] }
// [ts] function getTargetType(value) {
// [ts] return value["__v_skip" /* SKIP */] || !Object.isExtensible(value)
// [ts] ? 0 /* INVALID */
// [ts] : targetTypeMap(toRawType(value));
// [ts] }
// [ts] function reactive(target) {
// if trying to observe a readonly proxy, return the readonly version.
// [ts] if (target && target["__v_isReadonly" /* IS_READONLY */]) {
// [ts] return target;
// [ts] }
// [ts] return createReactiveObject(target, false, mutableHandlers, mutableCollectionHandlers, reactiveMap);
// [ts] }
// [ts] /**
// [ts] * Return a shallowly-reactive copy of the original object, where only the root
// [ts] * level properties are reactive. It also does not auto-unwrap refs (even at the
// [ts] * root level).
// [ts] */
// [ts] function shallowReactive(target) {
// [ts] return createReactiveObject(target, false, shallowReactiveHandlers, shallowCollectionHandlers, shallowReactiveMap);
// [ts] }
// [ts] /**
// [ts] * Creates a readonly copy of the original object. Note the returned copy is not
// [ts] * made reactive, but `readonly` can be called on an already reactive object.
// [ts] */
// [ts] function readonly(target) {
// [ts] return createReactiveObject(target, true, readonlyHandlers, readonlyCollectionHandlers, readonlyMap);
// [ts] }
// [ts] /**
// [ts] * Returns a reactive-copy of the original object, where only the root level
// [ts] * properties are readonly, and does NOT unwrap refs nor recursively convert
// [ts] * returned properties.
// [ts] * This is used for creating the props proxy object for stateful components.
// [ts] */
// [ts] function shallowReadonly(target) {
// [ts] return createReactiveObject(target, true, shallowReadonlyHandlers, shallowReadonlyCollectionHandlers, shallowReadonlyMap);
// [ts] }
// [ts] function createReactiveObject(target, isReadonly, baseHandlers, collectionHandlers, proxyMap) {
// [ts] if (!isObject(target)) {
// [ts] {
// [ts] console.warn(`value cannot be made reactive: ${String(target)}`);
// [ts] }
// [ts] return target;
// [ts] }
// target is already a Proxy, return it.
// exception: calling readonly() on a reactive object
// [ts] if (target["__v_raw" /* RAW */] &&
// [ts] !(isReadonly && target["__v_isReactive" /* IS_REACTIVE */])) {
// [ts] return target;
// [ts] }
// target already has corresponding Proxy
// [ts] const existingProxy = proxyMap.get(target);
// [ts] if (existingProxy) {
// [ts] return existingProxy;
// [ts] }
// only a whitelist of value types can be observed.
// [ts] const targetType = getTargetType(target);
// [ts] if (targetType === 0 /* INVALID */) {
// [ts] return target;
// [ts] }
// [ts] const proxy = new Proxy(target, targetType === 2 /* COLLECTION */ ? collectionHandlers : baseHandlers);
// [ts] proxyMap.set(target, proxy);
// [ts] return proxy;
// [ts] }
// [ts] function isReactive(value) {
// [ts] if (isReadonly(value)) {
// [ts] return isReactive(value["__v_raw" /* RAW */]);
// [ts] }
// [ts] return !!(value && value["__v_isReactive" /* IS_REACTIVE */]);
// [ts] }
// [ts] function isReadonly(value) {
// [ts] return !!(value && value["__v_isReadonly" /* IS_READONLY */]);
// [ts] }
// [ts] function isProxy(value) {
// [ts] return isReactive(value) || isReadonly(value);
// [ts] }
// [ts] function toRaw(observed) {
// [ts] return ((observed && toRaw(observed["__v_raw" /* RAW */])) || observed);
// [ts] }
// [ts] function markRaw(value) {
// [ts] def(value, "__v_skip" /* SKIP */, true);
// [ts] return value;
// [ts] }
// [ts] const convert = (val) => isObject(val) ? reactive(val) : val;
// [ts] function isRef(r) {
// [ts] return Boolean(r && r.__v_isRef === true);
// [ts] }
// [ts] function ref(value) {
// [ts] return createRef(value);
// [ts] }
// [ts] function shallowRef(value) {
// [ts] return createRef(value, true);
// [ts] }
// [ts] class RefImpl {
// [ts] constructor(value, _shallow = false) {
// [ts] this._shallow = _shallow;
// [ts] this.__v_isRef = true;
// [ts] this._rawValue = _shallow ? value : toRaw(value);
// [ts] this._value = _shallow ? value : convert(value);
// [ts] }
// [ts] get value() {
// [ts] track(toRaw(this), "get" /* GET */, 'value');
// [ts] return this._value;
// [ts] }
// [ts] set value(newVal) {
// [ts] newVal = this._shallow ? newVal : toRaw(newVal);
// [ts] if (hasChanged(newVal, this._rawValue)) {
// [ts] this._rawValue = newVal;
// [ts] this._value = this._shallow ? newVal : convert(newVal);
// [ts] trigger(toRaw(this), "set" /* SET */, 'value', newVal);
// [ts] }
// [ts] }
// [ts] }
// [ts] function createRef(rawValue, shallow = false) {
// [ts] if (isRef(rawValue)) {
// [ts] return rawValue;
// [ts] }
// [ts] return new RefImpl(rawValue, shallow);
// [ts] }
// [ts] function triggerRef(ref) {
// [ts] trigger(toRaw(ref), "set" /* SET */, 'value', ref.value );
// [ts] }
// [ts] function unref(ref) {
// [ts] return isRef(ref) ? ref.value : ref;
// [ts] }
// [ts] const shallowUnwrapHandlers = {
// [ts] get: (target, key, receiver) => unref(Reflect.get(target, key, receiver)),
// [ts] set: (target, key, value, receiver) => {
// [ts] const oldValue = target[key];
// [ts] if (isRef(oldValue) && !isRef(value)) {
// [ts] oldValue.value = value;
// [ts] return true;
// [ts] }
// [ts] else {
// [ts] return Reflect.set(target, key, value, receiver);
// [ts] }
// [ts] }
// [ts] };
// [ts] function proxyRefs(objectWithRefs) {
// [ts] return isReactive(objectWithRefs)
// [ts] ? objectWithRefs
// [ts] : new Proxy(objectWithRefs, shallowUnwrapHandlers);
// [ts] }
// [ts] class CustomRefImpl {
// [ts] constructor(factory) {
// [ts] this.__v_isRef = true;
// [ts] const { get, set } = factory(() => track(this, "get" /* GET */, 'value'), () => trigger(this, "set" /* SET */, 'value'));
// [ts] this._get = get;
// [ts] this._set = set;
// [ts] }
// [ts] get value() {
// [ts] return this._get();
// [ts] }
// [ts] set value(newVal) {
// [ts] this._set(newVal);
// [ts] }
// [ts] }
// [ts] function customRef(factory) {
// [ts] return new CustomRefImpl(factory);
// [ts] }
// [ts] function toRefs(object) {
// [ts] if (!isProxy(object)) {
// [ts] console.warn(`toRefs() expects a reactive object but received a plain one.`);
// [ts] }
// [ts] const ret = isArray(object) ? new Array(object.length) : {};
// [ts] for (const key in object) {
// [ts] ret[key] = toRef(object, key);
// [ts] }
// [ts] return ret;
// [ts] }
// [ts] class ObjectRefImpl {
// [ts] constructor(_object, _key) {
// [ts] this._object = _object;
// [ts] this._key = _key;
// [ts] this.__v_isRef = true;
// [ts] }
// [ts] get value() {
// [ts] return this._object[this._key];
// [ts] }
// [ts] set value(newVal) {
// [ts] this._object[this._key] = newVal;
// [ts] }
// [ts] }
// [ts] function toRef(object, key) {
// [ts] return isRef(object[key])
// [ts] ? object[key]
// [ts] : new ObjectRefImpl(object, key);
// [ts] }
// [ts] class ComputedRefImpl {
// [ts] constructor(getter, _setter, isReadonly) {
// [ts] this._setter = _setter;
// [ts] this._dirty = true;
// [ts] this.__v_isRef = true;
// [ts] this.effect = effect(getter, {
// [ts] lazy: true,
// [ts] scheduler: () => {
// [ts] if (!this._dirty) {
// [ts] this._dirty = true;
// [ts] trigger(toRaw(this), "set" /* SET */, 'value');
// [ts] }
// [ts] }
// [ts] });
// [ts] this["__v_isReadonly" /* IS_READONLY */] = isReadonly;
// [ts] }
// [ts] get value() {
// the computed ref may get wrapped by other proxies e.g. readonly() #3376
// [ts] const self = toRaw(this);
// [ts] if (self._dirty) {
// [ts] self._value = this.effect();
// [ts] self._dirty = false;
// [ts] }
// [ts] track(self, "get" /* GET */, 'value');
// [ts] return self._value;
// [ts] }
// [ts] set value(newValue) {
// [ts] this._setter(newValue);
// [ts] }
// [ts] }
// [ts] function computed(getterOrOptions) {
// [ts] let getter;
// [ts] let setter;
// [ts] if (isFunction(getterOrOptions)) {
// [ts] getter = getterOrOptions;
// [ts] setter = () => {
// [ts] console.warn('Write operation failed: computed value is readonly');
// [ts] }
// [ts] ;
// [ts] }
// [ts] else {
// [ts] getter = getterOrOptions.get;
// [ts] setter = getterOrOptions.set;
// [ts] }
// [ts] return new ComputedRefImpl(getter, setter, isFunction(getterOrOptions) || !getterOrOptions.set);
// [ts] }
// [ts] const stack = [];
// [ts] function pushWarningContext(vnode) {
// [ts] stack.push(vnode);
// [ts] }
// [ts] function popWarningContext() {
// [ts] stack.pop();
// [ts] }
// [ts] function warn(msg, ...args) {
// avoid props formatting or warn handler tracking deps that might be mutated
// during patch, leading to infinite recursion.
// [ts] pauseTracking();
// [ts] const instance = stack.length ? stack[stack.length - 1].component : null;
// [ts] const appWarnHandler = instance && instance.appContext.config.warnHandler;
// [ts] const trace = getComponentTrace();
// [ts] if (appWarnHandler) {
// [ts] callWithErrorHandling(appWarnHandler, instance, 11 /* APP_WARN_HANDLER */, [
// [ts] msg + args.join(''),
// [ts] instance && instance.proxy,
// [ts] trace
// [ts] .map(({ vnode }) => `at <${formatComponentName(instance, vnode.type)}>`)
// [ts] .join('\n'),
// [ts] trace
// [ts] ]);
// [ts] }
// [ts] else {
// [ts] const warnArgs = [`[Vue warn]: ${msg}`, ...args];
// [ts] /* istanbul ignore if */
// [ts] if (trace.length &&
// avoid spamming console during tests
// [ts] !false) {
// [ts] warnArgs.push(`\n`, ...formatTrace(trace));
// [ts] }
// [ts] console.warn(...warnArgs);
// [ts] }
// [ts] resetTracking();
// [ts] }
// [ts] function getComponentTrace() {
// [ts] let currentVNode = stack[stack.length - 1];
// [ts] if (!currentVNode) {
// [ts] return [];
// [ts] }
// we can't just use the stack because it will be incomplete during updates
// that did not start from the root. Re-construct the parent chain using
// instance parent pointers.
// [ts] const normalizedStack = [];
// [ts] while (currentVNode) {
// [ts] const last = normalizedStack[0];
// [ts] if (last && last.vnode === currentVNode) {
// [ts] last.recurseCount++;
// [ts] }
// [ts] else {
// [ts] normalizedStack.push({
// [ts] vnode: currentVNode,
// [ts] recurseCount: 0
// [ts] });
// [ts] }
// [ts] const parentInstance = currentVNode.component && currentVNode.component.parent;
// [ts] currentVNode = parentInstance && parentInstance.vnode;
// [ts] }
// [ts] return normalizedStack;
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] function formatTrace(trace) {
// [ts] const logs = [];
// [ts] trace.forEach((entry, i) => {
// [ts] logs.push(...(i === 0 ? [] : [`\n`]), ...formatTraceEntry(entry));
// [ts] });
// [ts] return logs;
// [ts] }
// [ts] function formatTraceEntry({ vnode, recurseCount }) {
// [ts] const postfix = recurseCount > 0 ? `... (${recurseCount} recursive calls)` : ``;
// [ts] const isRoot = vnode.component ? vnode.component.parent == null : false;
// [ts] const open = ` at <${formatComponentName(vnode.component, vnode.type, isRoot)}`;
// [ts] const close = `>` + postfix;
// [ts] return vnode.props
// [ts] ? [open, ...formatProps(vnode.props), close]
// [ts] : [open + close];
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] function formatProps(props) {
// [ts] const res = [];
// [ts] const keys = Object.keys(props);
// [ts] keys.slice(0, 3).forEach(key => {
// [ts] res.push(...formatProp(key, props[key]));
// [ts] });
// [ts] if (keys.length > 3) {
// [ts] res.push(` ...`);
// [ts] }
// [ts] return res;
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] function formatProp(key, value, raw) {
// [ts] if (isString(value)) {
// [ts] value = JSON.stringify(value);
// [ts] return raw ? value : [`${key}=${value}`];
// [ts] }
// [ts] else if (typeof value === 'number' ||
// [ts] typeof value === 'boolean' ||
// [ts] value == null) {
// [ts] return raw ? value : [`${key}=${value}`];
// [ts] }
// [ts] else if (isRef(value)) {
// [ts] value = formatProp(key, toRaw(value.value), true);
// [ts] return raw ? value : [`${key}=Ref<`, value, `>`];
// [ts] }
// [ts] else if (isFunction(value)) {
// [ts] return [`${key}=fn${if (value.name) `<${value.name}>` else ``}`];
// [ts] }
// [ts] else {
// [ts] value = toRaw(value);
// [ts] return raw ? value : [`${key}=`, value];
// [ts] }
// [ts] }
// [ts] const ErrorTypeStrings = {
// [ts] ["sp" /* SERVER_PREFETCH */]: 'serverPrefetch hook',
// [ts] ["bc" /* BEFORE_CREATE */]: 'beforeCreate hook',
// [ts] ["c" /* CREATED */]: 'created hook',
// [ts] ["bm" /* BEFORE_MOUNT */]: 'beforeMount hook',
// [ts] ["m" /* MOUNTED */]: 'mounted hook',
// [ts] ["bu" /* BEFORE_UPDATE */]: 'beforeUpdate hook',
// [ts] ["u" /* UPDATED */]: 'updated',
// [ts] ["bum" /* BEFORE_UNMOUNT */]: 'beforeUnmount hook',
// [ts] ["um" /* UNMOUNTED */]: 'unmounted hook',
// [ts] ["a" /* ACTIVATED */]: 'activated hook',
// [ts] ["da" /* DEACTIVATED */]: 'deactivated hook',
// [ts] ["ec" /* ERROR_CAPTURED */]: 'errorCaptured hook',
// [ts] ["rtc" /* RENDER_TRACKED */]: 'renderTracked hook',
// [ts] ["rtg" /* RENDER_TRIGGERED */]: 'renderTriggered hook',
// [ts] [0 /* SETUP_FUNCTION */]: 'setup function',
// [ts] [1 /* RENDER_FUNCTION */]: 'render function',
// [ts] [2 /* WATCH_GETTER */]: 'watcher getter',
// [ts] [3 /* WATCH_CALLBACK */]: 'watcher callback',
// [ts] [4 /* WATCH_CLEANUP */]: 'watcher cleanup function',
// [ts] [5 /* NATIVE_EVENT_HANDLER */]: 'native event handler',
// [ts] [6 /* COMPONENT_EVENT_HANDLER */]: 'component event handler',
// [ts] [7 /* VNODE_HOOK */]: 'vnode hook',
// [ts] [8 /* DIRECTIVE_HOOK */]: 'directive hook',
// [ts] [9 /* TRANSITION_HOOK */]: 'transition hook',
// [ts] [10 /* APP_ERROR_HANDLER */]: 'app errorHandler',
// [ts] [11 /* APP_WARN_HANDLER */]: 'app warnHandler',
// [ts] [12 /* FUNCTION_REF */]: 'ref function',
// [ts] [13 /* ASYNC_COMPONENT_LOADER */]: 'async component loader',
// [ts] [14 /* SCHEDULER */]: 'scheduler flush. This is likely a Vue internals bug. ' +
// [ts] 'Please open an issue at https://new-issue.vuejs.org/?repo=vuejs/vue-next'
// [ts] };
// [ts] function callWithErrorHandling(fn, instance, type, args) {
// [ts] let res;
// [ts] try {
// [ts] res = args ? fn(...args) : fn();
// [ts] }
// [ts] catch (err) {
// [ts] handleError(err, instance, type);
// [ts] }
// [ts] return res;
// [ts] }
// [ts] function callWithAsyncErrorHandling(fn, instance, type, args) {
// [ts] if (isFunction(fn)) {
// [ts] const res = callWithErrorHandling(fn, instance, type, args);
// [ts] if (res && isPromise(res)) {
// [ts] res.catch(err => {
// [ts] handleError(err, instance, type);
// [ts] });
// [ts] }
// [ts] return res;
// [ts] }
// [ts] const values = [];
// [ts] for (let i = 0; i < fn.length; i++) {
// [ts] values.push(callWithAsyncErrorHandling(fn[i], instance, type, args));
// [ts] }
// [ts] return values;
// [ts] }
// [ts] function handleError(err, instance, type, throwInDev = true) {
// [ts] const contextVNode = instance ? instance.vnode : null;
// [ts] if (instance) {
// [ts] let cur = instance.parent;
// the exposed instance is the render proxy to keep it consistent with 2.x
// [ts] const exposedInstance = instance.proxy;
// in production the hook receives only the error code
// [ts] const errorInfo = ErrorTypeStrings[type] ;
// [ts] while (cur) {
// [ts] const errorCapturedHooks = cur.ec;
// [ts] if (errorCapturedHooks) {
// [ts] for (let i = 0; i < errorCapturedHooks.length; i++) {
// [ts] if (errorCapturedHooks[i](err, exposedInstance, errorInfo) === false) {
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] cur = cur.parent;
// [ts] }
// app-level handling
// [ts] const appErrorHandler = instance.appContext.config.errorHandler;
// [ts] if (appErrorHandler) {
// [ts] callWithErrorHandling(appErrorHandler, null, 10 /* APP_ERROR_HANDLER */, [err, exposedInstance, errorInfo]);
// [ts] return;
// [ts] }
// [ts] }
// [ts] logError(err, type, contextVNode, throwInDev);
// [ts] }
// [ts] function logError(err, type, contextVNode, throwInDev = true) {
// [ts] {
// [ts] const info = ErrorTypeStrings[type];
// [ts] if (contextVNode) {
// [ts] pushWarningContext(contextVNode);
// [ts] }
// [ts] warn(`Unhandled error${if (info) ` during execution of ${info}` else ``}`);
// [ts] if (contextVNode) {
// [ts] popWarningContext();
// [ts] }
// crash in dev by default so it's more noticeable
// [ts] if (throwInDev) {
// [ts] throw err;
// [ts] }
// [ts] else {
// [ts] console.error(err);
// [ts] }
// [ts] }
// [ts] }
// [ts] let isFlushing = false;
// [ts] let isFlushPending = false;
// [ts] const queue = [];
// [ts] let flushIndex = 0;
// [ts] const pendingPreFlushCbs = [];
// [ts] let activePreFlushCbs = null;
// [ts] let preFlushIndex = 0;
// [ts] const pendingPostFlushCbs = [];
// [ts] let activePostFlushCbs = null;
// [ts] let postFlushIndex = 0;
// [ts] const resolvedPromise = Promise.resolve();
// [ts] let currentFlushPromise = null;
// [ts] let currentPreFlushParentJob = null;
// [ts] const RECURSION_LIMIT = 100;
// [ts] function nextTick(fn) {
// [ts] const p = currentFlushPromise || resolvedPromise;
// [ts] return fn ? p.then(this ? fn.bind(this) : fn) : p;
// [ts] }
// #2768
// Use binary-search to find a suitable position in the queue,
// so that the queue maintains the increasing order of job's id,
// which can prevent the job from being skipped and also can avoid repeated patching.
// [ts] function findInsertionIndex(job) {
// the start index should be `flushIndex + 1`
// [ts] let start = flushIndex + 1;
// [ts] let end = queue.length;
// [ts] const jobId = getId(job);
// [ts] while (start < end) {
// [ts] const middle = (start + end) >>> 1;
// [ts] const middleJobId = getId(queue[middle]);
// [ts] middleJobId < jobId ? (start = middle + 1) : (end = middle);
// [ts] }
// [ts] return start;
// [ts] }
// [ts] function queueJob(job) {
// the dedupe search uses the startIndex argument of Array.includes()
// by default the search index includes the current job that is being run
// so it cannot recursively trigger itself again.
// if the job is a watch() callback, the search will start with a +1 index to
// allow it recursively trigger itself - it is the user's responsibility to
// ensure it doesn't end up in an infinite loop.
// [ts] if ((!queue.length ||
// [ts] !queue.includes(job, isFlushing && job.allowRecurse ? flushIndex + 1 : flushIndex)) &&
// [ts] job !== currentPreFlushParentJob) {
// [ts] const pos = findInsertionIndex(job);
// [ts] if (pos > -1) {
// [ts] queue.splice(pos, 0, job);
// [ts] }
// [ts] else {
// [ts] queue.push(job);
// [ts] }
// [ts] queueFlush();
// [ts] }
// [ts] }
// [ts] function queueFlush() {
// [ts] if (!isFlushing && !isFlushPending) {
// [ts] isFlushPending = true;
// [ts] currentFlushPromise = resolvedPromise.then(flushJobs);
// [ts] }
// [ts] }
// [ts] function invalidateJob(job) {
// [ts] const i = queue.indexOf(job);
// [ts] if (i > flushIndex) {
// [ts] queue.splice(i, 1);
// [ts] }
// [ts] }
// [ts] function queueCb(cb, activeQueue, pendingQueue, index) {
// [ts] if (!isArray(cb)) {
// [ts] if (!activeQueue ||
// [ts] !activeQueue.includes(cb, cb.allowRecurse ? index + 1 : index)) {
// [ts] pendingQueue.push(cb);
// [ts] }
// [ts] }
// [ts] else {
// if cb is an array, it is a component lifecycle hook which can only be
// triggered by a job, which is already deduped in the main queue, so
// we can skip duplicate check here to improve perf
// [ts] pendingQueue.push(...cb);
// [ts] }
// [ts] queueFlush();
// [ts] }
// [ts] function queuePreFlushCb(cb) {
// [ts] queueCb(cb, activePreFlushCbs, pendingPreFlushCbs, preFlushIndex);
// [ts] }
// [ts] function queuePostFlushCb(cb) {
// [ts] queueCb(cb, activePostFlushCbs, pendingPostFlushCbs, postFlushIndex);
// [ts] }
// [ts] function flushPreFlushCbs(seen, parentJob = null) {
// [ts] if (pendingPreFlushCbs.length) {
// [ts] currentPreFlushParentJob = parentJob;
// [ts] activePreFlushCbs = [...new Set(pendingPreFlushCbs)];
// [ts] pendingPreFlushCbs.length = 0;
// [ts] {
// [ts] seen = seen || new Map();
// [ts] }
// [ts] for (preFlushIndex = 0; preFlushIndex < activePreFlushCbs.length; preFlushIndex++) {
// [ts] if (checkRecursiveUpdates(seen, activePreFlushCbs[preFlushIndex])) {
// [ts] continue;
// [ts] }
// [ts] activePreFlushCbs[preFlushIndex]();
// [ts] }
// [ts] activePreFlushCbs = null;
// [ts] preFlushIndex = 0;
// [ts] currentPreFlushParentJob = null;
// recursively flush until it drains
// [ts] flushPreFlushCbs(seen, parentJob);
// [ts] }
// [ts] }
// [ts] function flushPostFlushCbs(seen) {
// [ts] if (pendingPostFlushCbs.length) {
// [ts] const deduped = [...new Set(pendingPostFlushCbs)];
// [ts] pendingPostFlushCbs.length = 0;
// #1947 already has active queue, nested flushPostFlushCbs call
// [ts] if (activePostFlushCbs) {
// [ts] activePostFlushCbs.push(...deduped);
// [ts] return;
// [ts] }
// [ts] activePostFlushCbs = deduped;
// [ts] {
// [ts] seen = seen || new Map();
// [ts] }
// [ts] activePostFlushCbs.sort((a, b) => getId(a) - getId(b));
// [ts] for (postFlushIndex = 0; postFlushIndex < activePostFlushCbs.length; postFlushIndex++) {
// [ts] if (checkRecursiveUpdates(seen, activePostFlushCbs[postFlushIndex])) {
// [ts] continue;
// [ts] }
// [ts] activePostFlushCbs[postFlushIndex]();
// [ts] }
// [ts] activePostFlushCbs = null;
// [ts] postFlushIndex = 0;
// [ts] }
// [ts] }
// [ts] const getId = (job) => job.id == null ? Infinity : job.id;
// [ts] function flushJobs(seen) {
// [ts] isFlushPending = false;
// [ts] isFlushing = true;
// [ts] {
// [ts] seen = seen || new Map();
// [ts] }
// [ts] flushPreFlushCbs(seen);
// Sort queue before flush.
// This ensures that:
// 1. Components are updated from parent to child. (because parent is always
//    created before the child so its render effect will have smaller
//    priority number)
// 2. If a component is unmounted during a parent component's update,
//    its update can be skipped.
// [ts] queue.sort((a, b) => getId(a) - getId(b));
// [ts] try {
// [ts] for (flushIndex = 0; flushIndex < queue.length; flushIndex++) {
// [ts] const job = queue[flushIndex];
// [ts] if (job && job.active !== false) {
// [ts] if (true && checkRecursiveUpdates(seen, job)) {
// [ts] continue;
// [ts] }
// [ts] callWithErrorHandling(job, null, 14 /* SCHEDULER */);
// [ts] }
// [ts] }
// [ts] }
// [ts] finally {
// [ts] flushIndex = 0;
// [ts] queue.length = 0;
// [ts] flushPostFlushCbs(seen);
// [ts] isFlushing = false;
// [ts] currentFlushPromise = null;
// some postFlushCb queued jobs!
// keep flushing until it drains.
// [ts] if (queue.length ||
// [ts] pendingPreFlushCbs.length ||
// [ts] pendingPostFlushCbs.length) {
// [ts] flushJobs(seen);
// [ts] }
// [ts] }
// [ts] }
// [ts] function checkRecursiveUpdates(seen, fn) {
// [ts] if (!seen.has(fn)) {
// [ts] seen.set(fn, 1);
// [ts] }
// [ts] else {
// [ts] const count = seen.get(fn);
// [ts] if (count > RECURSION_LIMIT) {
// [ts] const instance = fn.ownerInstance;
// [ts] const componentName = instance && getComponentName(instance.type);
// [ts] warn(`Maximum recursive updates exceeded${if (componentName) ` in component <${componentName}>` else ``}. ` +
// [ts] `This means you have a reactive effect that is mutating its own ` +
// [ts] `dependencies and thus recursively triggering itself. Possible sources ` +
// [ts] `include component template, render function, updated hook or ` +
// [ts] `watcher source function.`);
// [ts] return true;
// [ts] }
// [ts] else {
// [ts] seen.set(fn, count + 1);
// [ts] }
// [ts] }
// [ts] }
// [ts] /* eslint-disable no-restricted-globals */
// [ts] let isHmrUpdating = false;
// [ts] const hmrDirtyComponents = new Set();
// Expose the HMR runtime on the global object
// This makes it entirely tree-shakable without polluting the exports and makes
// it easier to be used in toolings like vue-loader
// Note: for a component to be eligible for HMR it also needs the __hmrId option
// to be set so that its instances can be registered / removed.
// [ts] {
// [ts] const globalObject = typeof global !== 'undefined'
// [ts] ? global
// [ts] : typeof self !== 'undefined'
// [ts] ? self
// [ts] : typeof window !== 'undefined'
// [ts] ? window
// [ts] : {};
// [ts] globalObject.__VUE_HMR_RUNTIME__ = {
// [ts] createRecord: tryWrap(createRecord),
// [ts] rerender: tryWrap(rerender),
// [ts] reload: tryWrap(reload)
// [ts] };
// [ts] }
// [ts] const map = new Map();
// [ts] function registerHMR(instance) {
// [ts] const id = instance.type.__hmrId;
// [ts] let record = map.get(id);
// [ts] if (!record) {
// [ts] createRecord(id, instance.type);
// [ts] record = map.get(id);
// [ts] }
// [ts] record.instances.add(instance);
// [ts] }
// [ts] function unregisterHMR(instance) {
// [ts] map.get(instance.type.__hmrId).instances.delete(instance);
// [ts] }
// [ts] function createRecord(id, component) {
// [ts] if (!component) {
// [ts] warn(`HMR API usage is out of date.\n` +
// [ts] `Please upgrade vue-loader/vite/rollup-plugin-vue or other relevant ` +
// [ts] `dependency that handles Vue SFC compilation.`);
// [ts] component = {};
// [ts] }
// [ts] if (map.has(id)) {
// [ts] return false;
// [ts] }
// [ts] map.set(id, {
// [ts] component: isClassComponent(component) ? component.__vccOpts : component,
// [ts] instances: new Set()
// [ts] });
// [ts] return true;
// [ts] }
// [ts] function rerender(id, newRender) {
// [ts] const record = map.get(id);
// [ts] if (!record) {
// [ts] return;
// [ts] }
// [ts] if (newRender) {
// [ts] record.component.render = newRender;
// [ts] }
// Array.from creates a snapshot which avoids the set being mutated during
// updates
// [ts] Array.from(record.instances).forEach(instance => {
// [ts] if (newRender) {
// [ts] instance.render = newRender;
// [ts] }
// [ts] instance.renderCache = [];
// this flag forces child components with slot content to update
// [ts] isHmrUpdating = true;
// [ts] instance.update();
// [ts] isHmrUpdating = false;
// [ts] });
// [ts] }
// [ts] function reload(id, newComp) {
// [ts] const record = map.get(id);
// [ts] if (!record) {
// [ts] return;
// [ts] }
// Array.from creates a snapshot which avoids the set being mutated during
// updates
// [ts] const { component, instances } = record;
// [ts] if (!hmrDirtyComponents.has(component)) {
// 1. Update existing comp definition to match new one
// [ts] newComp = isClassComponent(newComp) ? newComp.__vccOpts : newComp;
// [ts] extend(component, newComp);
// [ts] for (const key in component) {
// [ts] if (key !== '__file' && !(key in newComp)) {
// [ts] delete component[key];
// [ts] }
// [ts] }
// 2. Mark component dirty. This forces the renderer to replace the component
// on patch.
// [ts] hmrDirtyComponents.add(component);
// 3. Make sure to unmark the component after the reload.
// [ts] queuePostFlushCb(() => {
// [ts] hmrDirtyComponents.delete(component);
// [ts] });
// [ts] }
// [ts] Array.from(instances).forEach(instance => {
// [ts] if (instance.parent) {
// 4. Force the parent instance to re-render. This will cause all updated
// components to be unmounted and re-mounted. Queue the update so that we
// don't end up forcing the same parent to re-render multiple times.
// [ts] queueJob(instance.parent.update);
// [ts] }
// [ts] else if (instance.appContext.reload) {
// root instance mounted via createApp() has a reload method
// [ts] instance.appContext.reload();
// [ts] }
// [ts] else if (typeof window !== 'undefined') {
// root instance inside tree created via raw render(). Force reload.
// [ts] window.location.reload();
// [ts] }
// [ts] else {
// [ts] console.warn('[HMR] Root or manually mounted instance modified. Full reload required.');
// [ts] }
// [ts] });
// [ts] }
// [ts] function tryWrap(fn) {
// [ts] return (id, arg) => {
// [ts] try {
// [ts] return fn(id, arg);
// [ts] }
// [ts] catch (e) {
// [ts] console.error(e);
// [ts] console.warn(`[HMR] Something went wrong during Vue component hot-reload. ` +
// [ts] `Full reload required.`);
// [ts] }
// [ts] };
// [ts] }
// [ts] function setDevtoolsHook(hook) {
// [ts] exports.devtools = hook;
// [ts] }
// [ts] function devtoolsInitApp(app, version) {
// TODO queue if devtools is undefined
// [ts] if (!exports.devtools) {
// [ts] return;
// [ts] }
// [ts] exports.devtools.emit("app:init" /* APP_INIT */, app, version, {
// [ts] Fragment,
// [ts] Text,
// [ts] Comment: Comment$1,
// [ts] Static
// [ts] });
// [ts] }
// [ts] function devtoolsUnmountApp(app) {
// [ts] if (!exports.devtools) {
// [ts] return;
// [ts] }
// [ts] exports.devtools.emit("app:unmount" /* APP_UNMOUNT */, app);
// [ts] }
// [ts] const devtoolsComponentAdded = /*#__PURE__*/ createDevtoolsComponentHook("component:added" /* COMPONENT_ADDED */);
// [ts] const devtoolsComponentUpdated = /*#__PURE__*/ createDevtoolsComponentHook("component:updated" /* COMPONENT_UPDATED */);
// [ts] const devtoolsComponentRemoved = /*#__PURE__*/ createDevtoolsComponentHook("component:removed" /* COMPONENT_REMOVED */);
// [ts] function createDevtoolsComponentHook(hook) {
// [ts] return (component) => {
// [ts] if (!exports.devtools) {
// [ts] return;
// [ts] }
// [ts] exports.devtools.emit(hook, component.appContext.app, component.uid, component.parent ? component.parent.uid : undefined, component);
// [ts] };
// [ts] }
// [ts] const devtoolsPerfStart = /*#__PURE__*/ createDevtoolsPerformanceHook("perf:start" /* PERFORMANCE_START */);
// [ts] const devtoolsPerfEnd = /*#__PURE__*/ createDevtoolsPerformanceHook("perf:end" /* PERFORMANCE_END */);
// [ts] function createDevtoolsPerformanceHook(hook) {
// [ts] return (component, type, time) => {
// [ts] if (!exports.devtools) {
// [ts] return;
// [ts] }
// [ts] exports.devtools.emit(hook, component.appContext.app, component.uid, component, type, time);
// [ts] };
// [ts] }
// [ts] function devtoolsComponentEmit(component, event, params) {
// [ts] if (!exports.devtools) {
// [ts] return;
// [ts] }
// [ts] exports.devtools.emit("component:emit" /* COMPONENT_EMIT */, component.appContext.app, component, event, params);
// [ts] }
// [ts] const deprecationData = {
// [ts] ["GLOBAL_MOUNT" /* GLOBAL_MOUNT */]: {
// [ts] message: `The global app bootstrapping API has changed: vm.$mount() and the "el" ` +
// [ts] `option have been removed. Use createApp(RootComponent).mount() instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/global-api.html#mounting-app-instance`
// [ts] },
// [ts] ["GLOBAL_MOUNT_CONTAINER" /* GLOBAL_MOUNT_CONTAINER */]: {
// [ts] message: `Vue detected directives on the mount container. ` +
// [ts] `In Vue 3, the container is no longer considered part of the template ` +
// [ts] `and will not be processed/replaced.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/mount-changes.html`
// [ts] },
// [ts] ["GLOBAL_EXTEND" /* GLOBAL_EXTEND */]: {
// [ts] message: `Vue.extend() has been removed in Vue 3. ` +
// [ts] `Use defineComponent() instead.`,
// [ts] link: `https://v3.vuejs.org/api/global-api.html#definecomponent`
// [ts] },
// [ts] ["GLOBAL_PROTOTYPE" /* GLOBAL_PROTOTYPE */]: {
// [ts] message: `Vue.prototype is no longer available in Vue 3. ` +
// [ts] `Use app.config.globalProperties instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/global-api.html#vue-prototype-replaced-by-config-globalproperties`
// [ts] },
// [ts] ["GLOBAL_SET" /* GLOBAL_SET */]: {
// [ts] message: `Vue.set() has been removed as it is no longer needed in Vue 3. ` +
// [ts] `Simply use native JavaScript mutations.`
// [ts] },
// [ts] ["GLOBAL_DELETE" /* GLOBAL_DELETE */]: {
// [ts] message: `Vue.delete() has been removed as it is no longer needed in Vue 3. ` +
// [ts] `Simply use native JavaScript mutations.`
// [ts] },
// [ts] ["GLOBAL_OBSERVABLE" /* GLOBAL_OBSERVABLE */]: {
// [ts] message: `Vue.observable() has been removed. ` +
// [ts] `Use \`import { reactive } from "vue"\` from Composition API instead.`,
// [ts] link: `https://v3.vuejs.org/api/basic-reactivity.html`
// [ts] },
// [ts] ["GLOBAL_PRIVATE_UTIL" /* GLOBAL_PRIVATE_UTIL */]: {
// [ts] message: `Vue.util has been removed. Please refactor to avoid its usage ` +
// [ts] `since it was an internal API even in Vue 2.`
// [ts] },
// [ts] ["CONFIG_SILENT" /* CONFIG_SILENT */]: {
// [ts] message: `config.silent has been removed because it is not good practice to ` +
// [ts] `intentionally suppress warnings. You can use your browser console's ` +
// [ts] `filter features to focus on relevant messages.`
// [ts] },
// [ts] ["CONFIG_DEVTOOLS" /* CONFIG_DEVTOOLS */]: {
// [ts] message: `config.devtools has been removed. To enable devtools for ` +
// [ts] `production, configure the __VUE_PROD_DEVTOOLS__ compile-time flag.`,
// [ts] link: `https://github.com/vuejs/vue-next/tree/master/packages/vue#bundler-build-feature-flags`
// [ts] },
// [ts] ["CONFIG_KEY_CODES" /* CONFIG_KEY_CODES */]: {
// [ts] message: `config.keyCodes has been removed. ` +
// [ts] `In Vue 3, you can directly use the kebab-case key names as v-on modifiers.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/keycode-modifiers.html`
// [ts] },
// [ts] ["CONFIG_PRODUCTION_TIP" /* CONFIG_PRODUCTION_TIP */]: {
// [ts] message: `config.productionTip has been removed.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/global-api.html#config-productiontip-removed`
// [ts] },
// [ts] ["CONFIG_IGNORED_ELEMENTS" /* CONFIG_IGNORED_ELEMENTS */]: {
// [ts] message: () => {
// [ts] let msg = `config.ignoredElements has been removed.`;
// [ts] if (isRuntimeOnly()) {
// [ts] msg += ` Pass the "isCustomElement" option to @vue/compiler-dom instead.`;
// [ts] }
// [ts] else {
// [ts] msg += ` Use config.isCustomElement instead.`;
// [ts] }
// [ts] return msg;
// [ts] },
// [ts] link: `https://v3.vuejs.org/guide/migration/global-api.html#config-ignoredelements-is-now-config-iscustomelement`
// [ts] },
// [ts] ["CONFIG_WHITESPACE" /* CONFIG_WHITESPACE */]: {
// this warning is only relevant in the full build when using runtime
// compilation, so it's put in the runtime compatConfig list.
// [ts] message: `Vue 3 compiler's whitespace option will default to "condense" instead of ` +
// [ts] `"preserve". To suppress this warning, provide an explicit value for ` +
// [ts] `\`config.compilerOptions.whitespace\`.`
// [ts] },
// [ts] ["CONFIG_OPTION_MERGE_STRATS" /* CONFIG_OPTION_MERGE_STRATS */]: {
// [ts] message: `config.optionMergeStrategies no longer exposes internal strategies. ` +
// [ts] `Use custom merge functions instead.`
// [ts] },
// [ts] ["INSTANCE_SET" /* INSTANCE_SET */]: {
// [ts] message: `vm.$set() has been removed as it is no longer needed in Vue 3. ` +
// [ts] `Simply use native JavaScript mutations.`
// [ts] },
// [ts] ["INSTANCE_DELETE" /* INSTANCE_DELETE */]: {
// [ts] message: `vm.$delete() has been removed as it is no longer needed in Vue 3. ` +
// [ts] `Simply use native JavaScript mutations.`
// [ts] },
// [ts] ["INSTANCE_DESTROY" /* INSTANCE_DESTROY */]: {
// [ts] message: `vm.$destroy() has been removed. Use app.unmount() instead.`,
// [ts] link: `https://v3.vuejs.org/api/application-api.html#unmount`
// [ts] },
// [ts] ["INSTANCE_EVENT_EMITTER" /* INSTANCE_EVENT_EMITTER */]: {
// [ts] message: `vm.$on/$once/$off() have been removed. ` +
// [ts] `Use an external event emitter library instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/events-api.html`
// [ts] },
// [ts] ["INSTANCE_EVENT_HOOKS" /* INSTANCE_EVENT_HOOKS */]: {
// [ts] message: event => `"${event}" lifecycle events are no longer supported. From templates, ` +
// [ts] `use the "vnode" prefix instead of "hook:". For example, @${event} ` +
// [ts] `should be changed to @vnode-${event.slice(5)}. ` +
// [ts] `From JavaScript, use Composition API to dynamically register lifecycle ` +
// [ts] `hooks.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/vnode-lifecycle-events.html`
// [ts] },
// [ts] ["INSTANCE_CHILDREN" /* INSTANCE_CHILDREN */]: {
// [ts] message: `vm.$children has been removed. Consider refactoring your logic ` +
// [ts] `to avoid relying on direct access to child components.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/children.html`
// [ts] },
// [ts] ["INSTANCE_LISTENERS" /* INSTANCE_LISTENERS */]: {
// [ts] message: `vm.$listeners has been removed. In Vue 3, parent v-on listeners are ` +
// [ts] `included in vm.$attrs and it is no longer necessary to separately use ` +
// [ts] `v-on="$listeners" if you are already using v-bind="$attrs". ` +
// [ts] `(Note: the Vue 3 behavior only applies if this compat config is disabled)`,
// [ts] link: `https://v3.vuejs.org/guide/migration/listeners-removed.html`
// [ts] },
// [ts] ["INSTANCE_SCOPED_SLOTS" /* INSTANCE_SCOPED_SLOTS */]: {
// [ts] message: `vm.$scopedSlots has been removed. Use vm.$slots instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/slots-unification.html`
// [ts] },
// [ts] ["INSTANCE_ATTRS_CLASS_STYLE" /* INSTANCE_ATTRS_CLASS_STYLE */]: {
// [ts] message: componentName => `Component <${componentName ||
// [ts] 'Anonymous'}> has \`inheritAttrs: false\` but is ` +
// [ts] `relying on class/style fallthrough from parent. In Vue 3, class/style ` +
// [ts] `are now included in $attrs and will no longer fallthrough when ` +
// [ts] `inheritAttrs is false. If you are already using v-bind="$attrs" on ` +
// [ts] `component root it should render the same end result. ` +
// [ts] `If you are binding $attrs to a non-root element and expecting ` +
// [ts] `class/style to fallthrough on root, you will need to now manually bind ` +
// [ts] `them on root via :class="$attrs.class".`,
// [ts] link: `https://v3.vuejs.org/guide/migration/attrs-includes-class-style.html`
// [ts] },
// [ts] ["OPTIONS_DATA_FN" /* OPTIONS_DATA_FN */]: {
// [ts] message: `The "data" option can no longer be a plain object. ` +
// [ts] `Always use a function.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/data-option.html`
// [ts] },
// [ts] ["OPTIONS_DATA_MERGE" /* OPTIONS_DATA_MERGE */]: {
// [ts] message: (key) => `Detected conflicting key "${key}" when merging data option values. ` +
// [ts] `In Vue 3, data keys are merged shallowly and will override one another.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/data-option.html#mixin-merge-behavior-change`
// [ts] },
// [ts] ["OPTIONS_BEFORE_DESTROY" /* OPTIONS_BEFORE_DESTROY */]: {
// [ts] message: `\`beforeDestroy\` has been renamed to \`beforeUnmount\`.`
// [ts] },
// [ts] ["OPTIONS_DESTROYED" /* OPTIONS_DESTROYED */]: {
// [ts] message: `\`destroyed\` has been renamed to \`unmounted\`.`
// [ts] },
// [ts] ["WATCH_ARRAY" /* WATCH_ARRAY */]: {
// [ts] message: `"watch" option or vm.$watch on an array value will no longer ` +
// [ts] `trigger on array mutation unless the "deep" option is specified. ` +
// [ts] `If current usage is intended, you can disable the compat behavior and ` +
// [ts] `suppress this warning with:` +
// [ts] `\n\n  configureCompat({ ${"WATCH_ARRAY" /* WATCH_ARRAY */}: false })\n`,
// [ts] link: `https://v3.vuejs.org/guide/migration/watch.html`
// [ts] },
// [ts] ["PROPS_DEFAULT_THIS" /* PROPS_DEFAULT_THIS */]: {
// [ts] message: (key) => `props default value function no longer has access to "this". The compat ` +
// [ts] `build only offers access to this.$options.` +
// [ts] `(found in prop "${key}")`,
// [ts] link: `https://v3.vuejs.org/guide/migration/props-default-this.html`
// [ts] },
// [ts] ["CUSTOM_DIR" /* CUSTOM_DIR */]: {
// [ts] message: (legacyHook, newHook) => `Custom directive hook "${legacyHook}" has been removed. ` +
// [ts] `Use "${newHook}" instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/custom-directives.html`
// [ts] },
// [ts] ["V_FOR_REF" /* V_FOR_REF */]: {
// [ts] message: `Ref usage on v-for no longer creates array ref values in Vue 3. ` +
// [ts] `Consider using function refs or refactor to avoid ref usage altogether.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/array-refs.html`
// [ts] },
// [ts] ["V_ON_KEYCODE_MODIFIER" /* V_ON_KEYCODE_MODIFIER */]: {
// [ts] message: `Using keyCode as v-on modifier is no longer supported. ` +
// [ts] `Use kebab-case key name modifiers instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/keycode-modifiers.html`
// [ts] },
// [ts] ["ATTR_FALSE_VALUE" /* ATTR_FALSE_VALUE */]: {
// [ts] message: (name) => `Attribute "${name}" with v-bind value \`false\` will render ` +
// [ts] `${name}="false" instead of removing it in Vue 3. To remove the attribute, ` +
// [ts] `use \`null\` or \`undefined\` instead. If the usage is intended, ` +
// [ts] `you can disable the compat behavior and suppress this warning with:` +
// [ts] `\n\n  configureCompat({ ${"ATTR_FALSE_VALUE" /* ATTR_FALSE_VALUE */}: false })\n`,
// [ts] link: `https://v3.vuejs.org/guide/migration/attribute-coercion.html`
// [ts] },
// [ts] ["ATTR_ENUMERATED_COERCION" /* ATTR_ENUMERATED_COERCION */]: {
// [ts] message: (name, value, coerced) => `Enumerated attribute "${name}" with v-bind value \`${value}\` will ` +
// [ts] `${if (value == null) `be removed` else `render the value as-is`} instead of coercing the value to "${coerced}" in Vue 3. ` +
// [ts] `Always use explicit "true" or "false" values for enumerated attributes. ` +
// [ts] `If the usage is intended, ` +
// [ts] `you can disable the compat behavior and suppress this warning with:` +
// [ts] `\n\n  configureCompat({ ${"ATTR_ENUMERATED_COERCION" /* ATTR_ENUMERATED_COERCION */}: false })\n`,
// [ts] link: `https://v3.vuejs.org/guide/migration/attribute-coercion.html`
// [ts] },
// [ts] ["TRANSITION_CLASSES" /* TRANSITION_CLASSES */]: {
// [ts] message: `` // this feature cannot be runtime-detected
// [ts] },
// [ts] ["TRANSITION_GROUP_ROOT" /* TRANSITION_GROUP_ROOT */]: {
// [ts] message: `<TransitionGroup> no longer renders a root <span> element by ` +
// [ts] `default if no "tag" prop is specified. If you do not rely on the span ` +
// [ts] `for styling, you can disable the compat behavior and suppress this ` +
// [ts] `warning with:` +
// [ts] `\n\n  configureCompat({ ${"TRANSITION_GROUP_ROOT" /* TRANSITION_GROUP_ROOT */}: false })\n`,
// [ts] link: `https://v3.vuejs.org/guide/migration/transition-group.html`
// [ts] },
// [ts] ["COMPONENT_ASYNC" /* COMPONENT_ASYNC */]: {
// [ts] message: (comp) => {
// [ts] const name = getComponentName(comp);
// [ts] return (`Async component${if (name) ` <${name}>` else `s`} should be explicitly created via \`defineAsyncComponent()\` ` +
// [ts] `in Vue 3. Plain functions will be treated as functional components in ` +
// [ts] `non-compat build. If you have already migrated all async component ` +
// [ts] `usage and intend to use plain functions for functional components, ` +
// [ts] `you can disable the compat behavior and suppress this ` +
// [ts] `warning with:` +
// [ts] `\n\n  configureCompat({ ${"COMPONENT_ASYNC" /* COMPONENT_ASYNC */}: false })\n`);
// [ts] },
// [ts] link: `https://v3.vuejs.org/guide/migration/async-components.html`
// [ts] },
// [ts] ["COMPONENT_FUNCTIONAL" /* COMPONENT_FUNCTIONAL */]: {
// [ts] message: (comp) => {
// [ts] const name = getComponentName(comp);
// [ts] return (`Functional component${if (name) ` <${name}>` else `s`} should be defined as a plain function in Vue 3. The "functional" ` +
// [ts] `option has been removed. NOTE: Before migrating to use plain ` +
// [ts] `functions for functional components, first make sure that all async ` +
// [ts] `components usage have been migrated and its compat behavior has ` +
// [ts] `been disabled.`);
// [ts] },
// [ts] link: `https://v3.vuejs.org/guide/migration/functional-components.html`
// [ts] },
// [ts] ["COMPONENT_V_MODEL" /* COMPONENT_V_MODEL */]: {
// [ts] message: (comp) => {
// [ts] const configMsg = `opt-in to ` +
// [ts] `Vue 3 behavior on a per-component basis with \`compatConfig: { ${"COMPONENT_V_MODEL" /* COMPONENT_V_MODEL */}: false }\`.`;
// [ts] if (comp.props &&
// [ts] (if (isArray(comp.props)) comp.props.includes("modelValue") else hasOwn(comp.props, "modelValue"))) {
// [ts] return (`Component delcares "modelValue" prop, which is Vue 3 usage, but ` +
// [ts] `is running under Vue 2 compat v-model behavior. You can ${configMsg}`);
// [ts] }
// [ts] return (`v-model usage on component has changed in Vue 3. Component that expects ` +
// [ts] `to work with v-model should now use the "modelValue" prop and emit the ` +
// [ts] `"update:modelValue" event. You can update the usage and then ${configMsg}`);
// [ts] },
// [ts] link: `https://v3.vuejs.org/guide/migration/v-model.html`
// [ts] },
// [ts] ["RENDER_FUNCTION" /* RENDER_FUNCTION */]: {
// [ts] message: `Vue 3's render function API has changed. ` +
// [ts] `You can opt-in to the new API with:` +
// [ts] `\n\n  configureCompat({ ${"RENDER_FUNCTION" /* RENDER_FUNCTION */}: false })\n` +
// [ts] `\n  (This can also be done per-component via the "compatConfig" option.)`,
// [ts] link: `https://v3.vuejs.org/guide/migration/render-function-api.html`
// [ts] },
// [ts] ["FILTERS" /* FILTERS */]: {
// [ts] message: `filters have been removed in Vue 3. ` +
// [ts] `The "|" symbol will be treated as native JavaScript bitwise OR operator. ` +
// [ts] `Use method calls or computed properties instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/filters.html`
// [ts] },
// [ts] ["PRIVATE_APIS" /* PRIVATE_APIS */]: {
// [ts] message: name => `"${name}" is a Vue 2 private API that no longer exists in Vue 3. ` +
// [ts] `If you are seeing this warning only due to a dependency, you can ` +
// [ts] `suppress this warning via { PRIVATE_APIS: 'supress-warning' }.`
// [ts] }
// [ts] };
// [ts] const instanceWarned = Object.create(null);
// [ts] const warnCount = Object.create(null);
// [ts] fn warnDeprecation(key: auto, instance: auto, args: auto) {
// [ts] instance = instance || getCurrentInstance();
// check user config
// [ts] const config = getCompatConfigForKey(key, instance);
// [ts] if (config == "suppress-warning") {
// [ts] return;
// [ts] }
// [ts] const dupKey = key + args.join("");
// [ts] let compId = instance && formatComponentName(instance, instance.type);
// [ts] if (compId == "Anonymous" && instance) {
// [ts] compId = instance.uid;
// [ts] }
// skip if the same warning is emitted for the same component type
// [ts] const componentDupKey = dupKey + compId;
// [ts] if (componentDupKey == instanceWarned) {
// [ts] return;
// [ts] }
// [ts] instanceWarned[componentDupKey] = true;
// same warning, but different component. skip the long message and just
// log the key and count.
// [ts] if (dupKey == warnCount) {
// [ts] warn(`(deprecation ${key}) (${++warnCount[dupKey] + 1})`);
// [ts] return;
// [ts] }
// [ts] warnCount[dupKey] = 0;
// [ts] const { message, link } = deprecationData[key];
// [ts] warn(`(deprecation ${key}) ${if (@typeName(message) == "function") message(...args) else message}${if (link) `\n  Details: ${link}` else ``}`);
// [ts] if (!isCompatEnabled(key, instance, true)) {
// [ts] @print(`^ The above deprecation's compat behavior is disabled and will likely ` +
// [ts] `lead to runtime errors.`);
// [ts] }
// [ts] }
// [ts] const globalCompatConfig = {
// [ts] MODE: 2
// [ts] };
// [ts] fn getCompatConfigForKey(key: auto, instance: auto) {
// [ts] const instanceConfig = instance && instance.type.compatConfig;
// [ts] if (instanceConfig && key == instanceConfig) {
// [ts] return instanceConfig[key];
// [ts] }
// [ts] return globalCompatConfig[key];
// [ts] }
// [ts] fn isCompatEnabled(key: auto, instance: auto, enableForBuiltIn: auto = false) {
// skip compat for built-in components
// [ts] if (!enableForBuiltIn && instance && instance.type.__isBuiltIn) {
// [ts] return false;
// [ts] }
// [ts] const rawMode = getCompatConfigForKey("MODE", instance) || 2;
// [ts] const val = getCompatConfigForKey(key, instance);
// [ts] const mode = if (isFunction(rawMode)) rawMode(instance && instance.type) else rawMode;
// [ts] if (mode == 2) {
// [ts] return val != false;
// [ts] }
// [ts] else {
// [ts] return val == true || val == "suppress-warning";
// [ts] }
// [ts] }
// [ts] fn emit(instance: auto, event: auto, rawArgs: auto) {
// [ts] const props = instance.vnode.props || EMPTY_OBJ;
// [ts] {
// [ts] const { emitsOptions, propsOptions: [propsOptions] } = instance;
// [ts] if (emitsOptions) {
// [ts] if (!(event == emitsOptions) &&
// [ts] !(false )) {
// [ts] if (!propsOptions || !(toHandlerKey(event) == propsOptions)) {
// [ts] warn(`Component emitted event "${event}" but it is neither declared in ` +
// [ts] `the emits option nor as an "${toHandlerKey(event)}" prop.`);
// [ts] }
// [ts] }
// [ts] else {
// [ts] const validator = emitsOptions[event];
// [ts] if (isFunction(validator)) {
// [ts] const isValid = validator(...rawArgs);
// [ts] if (!isValid) {
// [ts] warn(`Invalid event arguments: event validation failed for event "${event}".`);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let args = rawArgs;
// [ts] const isModelListener = event.startsWith("update:");
// for v-model update:xxx events, apply modifiers on args
// [ts] const modelArg = isModelListener && event.slice(7);
// [ts] if (modelArg && modelArg == props) {
// [ts] const modifiersKey = `${if (modelArg == "modelValue") "model" else modelArg}Modifiers`;
// [ts] const { number, trim } = props[modifiersKey] || EMPTY_OBJ;
// [ts] if (trim) {
// [ts] args = rawArgs.map(a => a.trim());
// [ts] }
// [ts] else if (number) {
// [ts] args = rawArgs.map(toNumber);
// [ts] }
// [ts] }
// [ts] {
// [ts] devtoolsComponentEmit(instance, event, args);
// [ts] }
// [ts] {
// [ts] const lowerCaseEvent = event.toLowerCase();
// [ts] if (lowerCaseEvent != event && props[toHandlerKey(lowerCaseEvent)]) {
// [ts] warn(`Event "${lowerCaseEvent}" is emitted in component ` +
// [ts] `${formatComponentName(instance, instance.type)} but the handler is registered for "${event}". ` +
// [ts] `Note that HTML attributes are case-insensitive and you cannot use ` +
// [ts] `v-on to listen to camelCase events when using in-DOM templates. ` +
// [ts] `You should probably use "${hyphenate(event)}" instead of "${event}".`);
// [ts] }
// [ts] }
// [ts] let handlerName;
// [ts] let handler = props[(handlerName = toHandlerKey(event))] ||
// also try camelCase event handler (#2249)
// [ts] props[(handlerName = toHandlerKey(camelize(event)))];
// for v-model update:xxx events, also trigger kebab-case equivalent
// for props passed via kebab-case
// [ts] if (!handler && isModelListener) {
// [ts] handler = props[(handlerName = toHandlerKey(hyphenate(event)))];
// [ts] }
// [ts] if (handler) {
// [ts] callWithAsyncErrorHandling(handler, instance, 6 /* COMPONENT_EVENT_HANDLER */, args);
// [ts] }
// [ts] const onceHandler = props[handlerName + `Once`];
// [ts] if (onceHandler) {
// [ts] if (!instance.emitted) {
// [ts] instance.emitted = {};
// [ts] }
// [ts] else if (instance.emitted[handlerName]) {
// [ts] return;
// [ts] }
// [ts] instance.emitted[handlerName] = true;
// [ts] callWithAsyncErrorHandling(onceHandler, instance, 6 /* COMPONENT_EVENT_HANDLER */, args);
// [ts] }
// [ts] }
// [ts] fn normalizeEmitsOptions(comp: auto, appContext: auto, asMixin: auto = false) {
// [ts] const cache = appContext.emitsCache;
// [ts] const cached = cache.get(comp);
// [ts] if (cached != null) {
// [ts] return cached;
// [ts] }
// [ts] const raw = comp.emits;
// [ts] let normalized = {};
// apply mixin/extends props
// [ts] let hasExtends = false;
// [ts] if (!isFunction(comp)) {
// [ts] const extendEmits = (raw) => {
// [ts] const normalizedFromExtend = normalizeEmitsOptions(raw, appContext, true);
// [ts] if (normalizedFromExtend) {
// [ts] hasExtends = true;
// [ts] extend(normalized, normalizedFromExtend);
// [ts] }
// [ts] };
// [ts] if (!asMixin && appContext.mixins.length) {
// [ts] appContext.mixins.forEach(extendEmits);
// [ts] }
// [ts] if (comp.extends) {
// [ts] extendEmits(comp.extends);
// [ts] }
// [ts] if (comp.mixins) {
// [ts] comp.mixins.forEach(extendEmits);
// [ts] }
// [ts] }
// [ts] if (!raw && !hasExtends) {
// [ts] cache.set(comp, null);
// [ts] return null;
// [ts] }
// [ts] if (isArray(raw)) {
// [ts] raw.forEach(key => (normalized[key] = null));
// [ts] }
// [ts] else {
// [ts] extend(normalized, raw);
// [ts] }
// [ts] cache.set(comp, normalized);
// [ts] return normalized;
// [ts] }
// Check if an incoming prop key is a declared emit event listener.
// e.g. With `emits: { click: null }`, props named `onClick` and `onclick` are
// both considered matched listeners.
// [ts] fn isEmitListener(options: auto, key: auto) {
// [ts] if (!options || !isOn(key)) {
// [ts] return false;
// [ts] }
// [ts] key = key.slice(2).replace("Once$", "");
// [ts] return (hasOwn(options, key[0].toLowerCase() + key.slice(1)) ||
// [ts] hasOwn(options, hyphenate(key)) ||
// [ts] hasOwn(options, key));
// [ts] }
// [ts] /**
// [ts] * mark the current rendering instance for asset resolution (e.g.
// [ts] * resolveComponent, resolveDirective) during render
// [ts] */
// [ts] let currentRenderingInstance = null;
// [ts] let currentScopeId = null;
// [ts] /**
// [ts] * Note: rendering calls maybe nested. The function returns the parent rendering
// [ts] * instance if present, which should be restored after the render is done:
// [ts] *
// [ts] * ```js
// [ts] * const prev = setCurrentRenderingInstance(i)
// [ts] * // ...render
// [ts] * setCurrentRenderingInstance(prev)
// [ts] * ```
// [ts] */
// [ts] fn setCurrentRenderingInstance(instance: auto) {
// [ts] const prev = currentRenderingInstance;
// [ts] currentRenderingInstance = instance;
// [ts] currentScopeId = (instance && instance.type.__scopeId) || null;
// [ts] return prev;
// [ts] }
// [ts] /**
// [ts] * Set scope id when creating hoisted vnodes.
// [ts] * @private compiler helper
// [ts] */
// [ts] fn pushScopeId(id: auto) {
// [ts] currentScopeId = id;
// [ts] }
// [ts] /**
// [ts] * Technically we no longer need this after 3.0.8 but we need to keep the same
// [ts] * API for backwards compat w/ code generated by compilers.
// [ts] * @private
// [ts] */
// [ts] fn popScopeId() {
// [ts] currentScopeId = null;
// [ts] }
// [ts] /**
// [ts] * Only for backwards compat
// [ts] * @private
// [ts] */
// [ts] fn withScopeId(_id: auto) { return withCtx; }
// [ts] /**
// [ts] * Wrap a slot function to memoize current rendering instance
// [ts] * @private compiler helper
// [ts] */
// [ts] fn withCtx(fn: auto, ctx: auto = currentRenderingInstance, isNonScopedSlot // false only: auto) {
// [ts] if (!ctx)
// [ts] return fn;
// already normalized
// [ts] if (fn._n) {
// [ts] return fn;
// [ts] }
// [ts] fn renderFnWithContext(args: auto) {
// If a user calls a compiled slot inside a template expression (#1745), it
// can mess up block tracking, so by default we disable block tracking and
// force bail out when invoking a compiled slot (indicated by the ._d flag).
// This isn't necessary if rendering a compiled `<slot>`, so we flip the
// ._d flag off when invoking the wrapped fn inside `renderSlot`.
// [ts] if (renderFnWithContext._d) {
// [ts] setBlockTracking(-1);
// [ts] }
// [ts] const prevInstance = setCurrentRenderingInstance(ctx);
// [ts] const res = fn(...args);
// [ts] setCurrentRenderingInstance(prevInstance);
// [ts] if (renderFnWithContext._d) {
// [ts] setBlockTracking(1);
// [ts] }
// [ts] {
// [ts] devtoolsComponentUpdated(ctx);
// [ts] }
// [ts] return res;
// [ts] };
// mark normalized to avoid duplicated wrapping
// [ts] renderFnWithContext._n = true;
// mark this as compiled by default
// this is used in vnode.ts -> normalizeChildren() to set the slot
// rendering flag.
// [ts] renderFnWithContext._c = true;
// disable block tracking by default
// [ts] renderFnWithContext._d = true;
// [ts] return renderFnWithContext;
// [ts] }
// [ts] /**
// [ts] * dev only flag to track whether $attrs was used during render.
// [ts] * If $attrs was used during render then the warning for failed attrs
// [ts] * fallthrough can be suppressed.
// [ts] */
// [ts] let accessedAttrs = false;
// [ts] fn markAttrsAccessed() {
// [ts] accessedAttrs = true;
// [ts] }
// [ts] fn renderComponentRoot(instance: auto) {
// [ts] const { type: Component, vnode, proxy, withProxy, props, propsOptions: [propsOptions], slots, attrs, emit, render, renderCache, data, setupState, ctx, inheritAttrs } = instance;
// [ts] let result;
// [ts] const prev = setCurrentRenderingInstance(instance);
// [ts] {
// [ts] accessedAttrs = false;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let fallthroughAttrs;
// [ts] if (vnode.shapeFlag & 4 /* STATEFUL_COMPONENT */) {
// withProxy is a proxy with a different `has` trap only for
// runtime-compiled render functions using `with` block.
// [ts] const proxyToUse = withProxy || proxy;
// [ts] result = normalizeVNode(render.call(proxyToUse, proxyToUse, renderCache, props, setupState, data, ctx));
// [ts] fallthroughAttrs = attrs;
// [ts] }
// [ts] else {
// functional
// [ts] const render = Component;
// in dev, mark attrs accessed if optional props (attrs === props)
// [ts] if (true && attrs == props) {
// [ts] markAttrsAccessed();
// [ts] }
// [ts] result = normalizeVNode(render.length > 1 ? render(props, true ? {
// [ts] get attrs() {
// [ts] markAttrsAccessed();
// [ts] return attrs;
// [ts] },
// [ts] slots,
// [ts] emit
// [ts] } : { attrs, slots, emit }) : render(props, null /* we know it doesn"t need it */));
// [ts] fallthroughAttrs = if (Component.props) attrs else getFunctionalFallthrough(attrs);
// [ts] }
// attr merging
// in dev mode, comments are preserved, and it's possible for a template
// to have comments along side the root element which makes it a fragment
// [ts] let root = result;
// [ts] let setRoot = null;
// [ts] if (true &&
// [ts] result.patchFlag > 0 &&
// [ts] result.patchFlag & 2048 /* DEV_ROOT_FRAGMENT */) {
// [ts] ;
// [ts] [root, setRoot] = getChildRoot(result);
// [ts] }
// [ts] if (fallthroughAttrs && inheritAttrs != false) {
// [ts] const keys = Object.keys(fallthroughAttrs);
// [ts] const { shapeFlag } = root;
// [ts] if (keys.length) {
// [ts] if (shapeFlag & 1 /* ELEMENT */ ||
// [ts] shapeFlag & 6 /* COMPONENT */) {
// [ts] if (propsOptions && keys.some(isModelListener)) {
// If a v-model listener (onUpdate:xxx) has a corresponding declared
// prop, it indicates this component expects to handle v-model and
// it should not fallthrough.
// related: #1543, #1643, #1989
// [ts] fallthroughAttrs = filterModelListeners(fallthroughAttrs, propsOptions);
// [ts] }
// [ts] root = cloneVNode(root, fallthroughAttrs);
// [ts] }
// [ts] else if (true && !accessedAttrs && root.type != Comment$1) {
// [ts] const allAttrs = Object.keys(attrs);
// [ts] const eventAttrs = [];
// [ts] const extraAttrs = [];
// [ts] let i = 0, l = allAttrs.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] const key = allAttrs[i];
// [ts] if (isOn(key)) {
// ignore v-model handlers when they fail to fallthrough
// [ts] if (!isModelListener(key)) {
// remove `on`, lowercase first letter to reflect event casing
// accurately
// [ts] eventAttrs.push(key[2].toLowerCase() + key.slice(3));
// [ts] }
// [ts] }
// [ts] else {
// [ts] extraAttrs.push(key);
// [ts] }
// [ts] }
// [ts] if (extraAttrs.length) {
// [ts] warn(`Extraneous non-props attributes (` +
// [ts] `${extraAttrs.join(", ")}) ` +
// [ts] `were passed to component but could not be automatically inherited ` +
// [ts] `because component renders fragment or text root nodes.`);
// [ts] }
// [ts] if (eventAttrs.length) {
// [ts] warn(`Extraneous non-emits event listeners (` +
// [ts] `${eventAttrs.join(", ")}) ` +
// [ts] `were passed to component but could not be automatically inherited ` +
// [ts] `because component renders fragment or text root nodes. ` +
// [ts] `If the listener is intended to be a component custom event listener only, ` +
// [ts] `declare it using the "emits" option.`);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (false &&
// [ts] isCompatEnabled("INSTANCE_ATTRS_CLASS_STYLE" /* INSTANCE_ATTRS_CLASS_STYLE */, instance) &&
// [ts] vnode.shapeFlag & 4 /* STATEFUL_COMPONENT */ &&
// [ts] (root.shapeFlag & 1 /* ELEMENT */ ||
// [ts] root.shapeFlag & 6 /* COMPONENT */)) ;
// inherit directives
// [ts] if (vnode.dirs) {
// [ts] if (true && !isElementRoot(root)) {
// [ts] warn(`Runtime directive used on component with non-element root node. ` +
// [ts] `The directives will not function as intended.`);
// [ts] }
// [ts] root.dirs = if (root.dirs) root.dirs.concat(vnode.dirs) else vnode.dirs;
// [ts] }
// inherit transition data
// [ts] if (vnode.transition) {
// [ts] if (true && !isElementRoot(root)) {
// [ts] warn(`Component inside <Transition> renders non-element root node ` +
// [ts] `that cannot be animated.`);
// [ts] }
// [ts] root.transition = vnode.transition;
// [ts] }
// [ts] if (true && setRoot) {
// [ts] setRoot(root);
// [ts] }
// [ts] else {
// [ts] result = root;
// [ts] }
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ? (catch (err) {)
// [ts] blockStack.length = 0;
// [ts] handleError(err, instance, 1 /* RENDER_FUNCTION */);
// [ts] result = createVNode(Comment$1);
// [ts] }
// [ts] setCurrentRenderingInstance(prev);
// [ts] return result;
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] * In dev mode, template root level comments are rendered, which turns the
// [ts] * template into a fragment root, but we need to locate the single element
// [ts] * root for attrs and scope id processing.
// [ts] */
// [ts] fn getChildRoot(vnode: auto) {
// [ts] const rawChildren = vnode.children;
// [ts] const dynamicChildren = vnode.dynamicChildren;
// [ts] const childRoot = filterSingleRoot(rawChildren);
// [ts] if (!childRoot) {
// [ts] return [vnode, null];
// [ts] }
// [ts] const index = rawChildren.indexOf(childRoot);
// [ts] const dynamicIndex = if (dynamicChildren) dynamicChildren.indexOf(childRoot) else -1;
// [ts] const setRoot = (updatedRoot) => {
// [ts] rawChildren[index] = updatedRoot;
// [ts] if (dynamicChildren) {
// [ts] if (dynamicIndex > -1) {
// [ts] dynamicChildren[dynamicIndex] = updatedRoot;
// [ts] }
// [ts] else if (updatedRoot.patchFlag > 0) {
// [ts] vnode.dynamicChildren = [...dynamicChildren, updatedRoot];
// [ts] }
// [ts] }
// [ts] };
// [ts] return [normalizeVNode(childRoot), setRoot];
// [ts] };
// [ts] fn filterSingleRoot(children: auto) {
// [ts] let singleRoot;
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// [ts] if (isVNode(child)) {
// ignore user comment
// [ts] if (child.type != Comment$1 || child.children == "v-if") {
// [ts] if (singleRoot) {
// has more than 1 non-comment child, return now
// [ts] return;
// [ts] }
// [ts] else {
// [ts] singleRoot = child;
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] return;
// [ts] }
// [ts] }
// [ts] return singleRoot;
// [ts] }
// [ts] fn getFunctionalFallthrough(attrs: auto) {
// [ts] let res;
// [ts] for (const key in attrs) {
// [ts] if (key == "class" || key == "style" || isOn(key)) {
// [ts] (res || (res = {}))[key] = attrs[key];
// [ts] }
// [ts] }
// [ts] return res;
// [ts] };
// [ts] fn filterModelListeners(attrs: auto, props: auto) {
// [ts] const res = {};
// [ts] for (const key in attrs) {
// [ts] if (!isModelListener(key) || !(key.slice(9) == props)) {
// [ts] res[key] = attrs[key];
// [ts] }
// [ts] }
// [ts] return res;
// [ts] };
// [ts] fn isElementRoot(vnode: auto) {
// [ts] return (vnode.shapeFlag & 6 /* COMPONENT */ ||
// [ts] vnode.shapeFlag & 1 /* ELEMENT */ ||
// [ts] vnode.type == Comment$1 // potential v-if branch switch
// [ts] );
// [ts] };
// [ts] fn shouldUpdateComponent(prevVNode: auto, nextVNode: auto, optimized: auto) {
// [ts] const { props: prevProps, children: prevChildren, component } = prevVNode;
// [ts] const { props: nextProps, children: nextChildren, patchFlag } = nextVNode;
// [ts] const emits = component.emitsOptions;
// Parent component's render function was hot-updated. Since this may have
// caused the child component's slots content to have changed, we need to
// force the child to update as well.
// [ts] if ((prevChildren || nextChildren) && isHmrUpdating) {
// [ts] return true;
// [ts] }
// force child update for runtime directive or transition on component vnode.
// [ts] if (nextVNode.dirs || nextVNode.transition) {
// [ts] return true;
// [ts] }
// [ts] if (optimized && patchFlag >= 0) {
// [ts] if (patchFlag & 1024 /* DYNAMIC_SLOTS */) {
// slot content that references values that might have changed,
// e.g. in a v-for
// [ts] return true;
// [ts] }
// [ts] if (patchFlag & 16 /* FULL_PROPS */) {
// [ts] if (!prevProps) {
// [ts] return !!nextProps;
// [ts] }
// presence of this flag indicates props are always non-null
// [ts] return hasPropsChanged(prevProps, nextProps, emits);
// [ts] }
// [ts] else if (patchFlag & 8 /* PROPS */) {
// [ts] const dynamicProps = nextVNode.dynamicProps;
// [ts] let i = 0;
// [ts] while (i < dynamicProps.length) { // [cap-migrate] for-step: i++
// [ts] const key = dynamicProps[i];
// [ts] if (nextProps[key] != prevProps[key] &&
// [ts] !isEmitListener(emits, key)) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// this path is only taken by manually written render functions
// so presence of any children leads to a forced update
// [ts] if (prevChildren || nextChildren) {
// [ts] if (!nextChildren || !nextChildren.$stable) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] if (prevProps == nextProps) {
// [ts] return false;
// [ts] }
// [ts] if (!prevProps) {
// [ts] return !!nextProps;
// [ts] }
// [ts] if (!nextProps) {
// [ts] return true;
// [ts] }
// [ts] return hasPropsChanged(prevProps, nextProps, emits);
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn hasPropsChanged(prevProps: auto, nextProps: auto, emitsOptions: auto) {
// [ts] const nextKeys = Object.keys(nextProps);
// [ts] if (nextKeys.length != Object.keys(prevProps).length) {
// [ts] return true;
// [ts] }
// [ts] let i = 0;
// [ts] while (i < nextKeys.length) { // [cap-migrate] for-step: i++
// [ts] const key = nextKeys[i];
// [ts] if (nextProps[key] != prevProps[key] &&
// [ts] !isEmitListener(emitsOptions, key)) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn updateHOCHostEl(_param: string, el // HostNode: auto) {
// [ts] while (parent && parent.subTree == vnode) {
// [ts] (vnode = parent.vnode).el = el;
// [ts] parent = parent.parent;
// [ts] }
// [ts] }
// [ts] fn isSuspense(type: auto) { return type.__isSuspense; }
// Suspense exposes a component-like API, and is treated like a component
// in the compiler, but internally it's a special built-in type that hooks
// directly into the renderer.
// [ts] const SuspenseImpl = {
// [ts] name: "Suspense",
// In order to make Suspense tree-shakable, we need to avoid importing it
// directly in the renderer. The renderer checks for the __isSuspense flag
// on a vnode's type and calls the `process` method, passing in renderer
// internals.
// [ts] __isSuspense: true,
// [ts] process(n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized,
// platform-specific impl passed from renderer
// [ts] rendererInternals) {
// [ts] if (n1 == null) {
// [ts] mountSuspense(n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized, rendererInternals);
// [ts] }
// [ts] else {
// [ts] patchSuspense(n1, n2, container, anchor, parentComponent, isSVG, slotScopeIds, optimized, rendererInternals);
// [ts] }
// [ts] },
// [ts] hydrate: hydrateSuspense,
// [ts] create: createSuspenseBoundary,
// [ts] normalize: normalizeSuspenseChildren
// [ts] };
// Force-casted public typing for h and TSX props inference
// [ts] const Suspense = (SuspenseImpl
// [ts] );
// [ts] fn triggerEvent(vnode: auto, name: auto) {
// [ts] const eventListener = vnode.props && vnode.props[name];
// [ts] if (isFunction(eventListener)) {
// [ts] eventListener();
// [ts] }
// [ts] }
// [ts] fn mountSuspense(vnode: auto, container: auto, anchor: auto, parentComponent: auto, parentSuspense: auto, isSVG: auto, slotScopeIds: auto, optimized: auto, rendererInternals: auto) {
// [ts] const { p: patch, o: { createElement } } = rendererInternals;
// [ts] const hiddenContainer = createElement("div");
// [ts] const suspense = (vnode.suspense = createSuspenseBoundary(vnode, parentSuspense, parentComponent, container, hiddenContainer, anchor, isSVG, slotScopeIds, optimized, rendererInternals));
// start mounting the content subtree in an off-dom container
// [ts] patch(null, (suspense.pendingBranch = vnode.ssContent), hiddenContainer, null, parentComponent, suspense, isSVG, slotScopeIds);
// now check if we have encountered any async deps
// [ts] if (suspense.deps > 0) {
// has async
// invoke @fallback event
// [ts] triggerEvent(vnode, "onPending");
// [ts] triggerEvent(vnode, "onFallback");
// mount the fallback tree
// [ts] patch(null, vnode.ssFallback, container, anchor, parentComponent, null, // fallback tree will not have suspense context
// [ts] isSVG, slotScopeIds);
// [ts] setActiveBranch(suspense, vnode.ssFallback);
// [ts] }
// [ts] else {
// Suspense has no async deps. Just resolve.
// [ts] suspense.resolve();
// [ts] }
// [ts] }
// [ts] fn patchSuspense(n1: auto, n2: auto, container: auto, anchor: auto, parentComponent: auto, isSVG: auto, slotScopeIds: auto, optimized: auto, _param: string) {
// [ts] const suspense = (n2.suspense = n1.suspense);
// [ts] suspense.vnode = n2;
// [ts] n2.el = n1.el;
// [ts] const newBranch = n2.ssContent;
// [ts] const newFallback = n2.ssFallback;
// [ts] const { activeBranch, pendingBranch, isInFallback, isHydrating } = suspense;
// [ts] if (pendingBranch) {
// [ts] suspense.pendingBranch = newBranch;
// [ts] if (isSameVNodeType(newBranch, pendingBranch)) {
// same root type but content may have changed.
// [ts] patch(pendingBranch, newBranch, suspense.hiddenContainer, null, parentComponent, suspense, isSVG, slotScopeIds, optimized);
// [ts] if (suspense.deps <= 0) {
// [ts] suspense.resolve();
// [ts] }
// [ts] else if (isInFallback) {
// [ts] patch(activeBranch, newFallback, container, anchor, parentComponent, null, // fallback tree will not have suspense context
// [ts] isSVG, slotScopeIds, optimized);
// [ts] setActiveBranch(suspense, newFallback);
// [ts] }
// [ts] }
// [ts] else {
// toggled before pending tree is resolved
// [ts] suspense.pendingId++;
// [ts] if (isHydrating) {
// if toggled before hydration is finished, the current DOM tree is
// no longer valid. set it as the active branch so it will be unmounted
// when resolved
// [ts] suspense.isHydrating = false;
// [ts] suspense.activeBranch = pendingBranch;
// [ts] }
// [ts] else {
// [ts] unmount(pendingBranch, parentComponent, suspense);
// [ts] }
// increment pending ID. this is used to invalidate async callbacks
// reset suspense state
// [ts] suspense.deps = 0;
// discard effects from pending branch
// [ts] suspense.effects.length = 0;
// discard previous container
// [ts] suspense.hiddenContainer = createElement("div");
// [ts] if (isInFallback) {
// already in fallback state
// [ts] patch(null, newBranch, suspense.hiddenContainer, null, parentComponent, suspense, isSVG, slotScopeIds, optimized);
// [ts] if (suspense.deps <= 0) {
// [ts] suspense.resolve();
// [ts] }
// [ts] else {
// [ts] patch(activeBranch, newFallback, container, anchor, parentComponent, null, // fallback tree will not have suspense context
// [ts] isSVG, slotScopeIds, optimized);
// [ts] setActiveBranch(suspense, newFallback);
// [ts] }
// [ts] }
// [ts] else if (activeBranch && isSameVNodeType(newBranch, activeBranch)) {
// toggled "back" to current active branch
// [ts] patch(activeBranch, newBranch, container, anchor, parentComponent, suspense, isSVG, slotScopeIds, optimized);
// force resolve
// [ts] suspense.resolve(true);
// [ts] }
// [ts] else {
// switched to a 3rd branch
// [ts] patch(null, newBranch, suspense.hiddenContainer, null, parentComponent, suspense, isSVG, slotScopeIds, optimized);
// [ts] if (suspense.deps <= 0) {
// [ts] suspense.resolve();
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (activeBranch && isSameVNodeType(newBranch, activeBranch)) {
// root did not change, just normal patch
// [ts] patch(activeBranch, newBranch, container, anchor, parentComponent, suspense, isSVG, slotScopeIds, optimized);
// [ts] setActiveBranch(suspense, newBranch);
// [ts] }
// [ts] else {
// root node toggled
// invoke @pending event
// [ts] triggerEvent(n2, "onPending");
// mount pending branch in off-dom container
// [ts] suspense.pendingBranch = newBranch;
// [ts] suspense.pendingId++;
// [ts] patch(null, newBranch, suspense.hiddenContainer, null, parentComponent, suspense, isSVG, slotScopeIds, optimized);
// [ts] if (suspense.deps <= 0) {
// incoming branch has no async deps, resolve now.
// [ts] suspense.resolve();
// [ts] }
// [ts] else {
// [ts] const { timeout, pendingId } = suspense;
// [ts] if (timeout > 0) {
// [ts] setTimeout(() => {
// [ts] if (suspense.pendingId == pendingId) {
// [ts] suspense.fallback(newFallback);
// [ts] }
// [ts] }, timeout);
// [ts] }
// [ts] else if (timeout == 0) {
// [ts] suspense.fallback(newFallback);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let hasWarned = false;
// [ts] fn createSuspenseBoundary(vnode: auto, parent: auto, parentComponent: auto, container: auto, hiddenContainer: auto, anchor: auto, isSVG: auto, slotScopeIds: auto, optimized: auto, rendererInternals: auto, isHydrating: auto = false) {
// [ts] /* istanbul ignore if */
// [ts] if (!hasWarned) {
// [ts] hasWarned = true;
// @ts-ignore `console.info` cannot be null error
// [ts] console[console.info ? "info" : "log"](`<Suspense> is an experimental feature and its API will likely change.`);
// [ts] }
// [ts] const { p: patch, m: move, um: unmount, n: next, o: { parentNode, remove } } = rendererInternals;
// [ts] const timeout = toNumber(vnode.props && vnode.props.timeout);
// [ts] const suspense = {
// [ts] vnode,
// [ts] parent,
// [ts] parentComponent,
// [ts] isSVG,
// [ts] container,
// [ts] hiddenContainer,
// [ts] anchor,
// [ts] deps: 0,
// [ts] pendingId: 0,
// [ts] timeout: if (@typeName(timeout) == "number") timeout else -1,
// [ts] activeBranch: null,
// [ts] pendingBranch: null,
// [ts] isInFallback: true,
// [ts] isHydrating,
// [ts] isUnmounted: false,
// [ts] effects: [],
// [ts] resolve: (resume) => {
// [ts] {
// [ts] if (!resume && !suspense.pendingBranch) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`suspense.resolve() is called without a pending branch.`);)
// [ts] }
// [ts] if (suspense.isUnmounted) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`suspense.resolve() is called on an already unmounted suspense boundary.`);)
// [ts] }
// [ts] }
// [ts] const { vnode, activeBranch, pendingBranch, pendingId, effects, parentComponent, container } = suspense;
// [ts] if (suspense.isHydrating) {
// [ts] suspense.isHydrating = false;
// [ts] }
// [ts] else if (!resume) {
// [ts] const delayEnter = activeBranch &&
// [ts] pendingBranch.transition &&
// [ts] pendingBranch.transition.mode == "out-in";
// [ts] if (delayEnter) {
// [ts] activeBranch.transition.afterLeave = () => {
// [ts] if (pendingId == suspense.pendingId) {
// [ts] move(pendingBranch, container, anchor, 0 /* ENTER */);
// [ts] }
// [ts] };
// [ts] }
// this is initial anchor on mount
// [ts] let { anchor } = suspense;
// unmount current active tree
// [ts] if (activeBranch) {
// if the fallback tree was mounted, it may have been moved
// as part of a parent suspense. get the latest anchor for insertion
// [ts] anchor = next(activeBranch);
// [ts] unmount(activeBranch, parentComponent, suspense, true);
// [ts] }
// [ts] if (!delayEnter) {
// move content from off-dom container to actual container
// [ts] move(pendingBranch, container, anchor, 0 /* ENTER */);
// [ts] }
// [ts] }
// [ts] setActiveBranch(suspense, pendingBranch);
// [ts] suspense.pendingBranch = null;
// [ts] suspense.isInFallback = false;
// flush buffered effects
// check if there is a pending parent suspense
// [ts] let parent = suspense.parent;
// [ts] let hasUnresolvedAncestor = false;
// [ts] while (parent) {
// [ts] if (parent.pendingBranch) {
// found a pending parent suspense, merge buffered post jobs
// into that parent
// [ts] parent.effects.push(...effects);
// [ts] hasUnresolvedAncestor = true;
// [ts] break;
// [ts] }
// [ts] parent = parent.parent;
// [ts] }
// no pending parent suspense, flush all jobs
// [ts] if (!hasUnresolvedAncestor) {
// [ts] queuePostFlushCb(effects);
// [ts] }
// [ts] suspense.effects = [];
// invoke @resolve event
// [ts] triggerEvent(vnode, "onResolve");
// [ts] },
// [ts] fallback: (fallbackVNode) => {
// [ts] if (!suspense.pendingBranch) {
// [ts] return;
// [ts] }
// [ts] const { vnode, activeBranch, parentComponent, container, isSVG } = suspense;
// invoke @fallback event
// [ts] triggerEvent(vnode, "onFallback");
// [ts] const anchor = next(activeBranch);
// [ts] const mountFallback = () => {
// [ts] if (!suspense.isInFallback) {
// [ts] return;
// [ts] }
// mount the fallback tree
// [ts] patch(null, fallbackVNode, container, anchor, parentComponent, null, // fallback tree will not have suspense context
// [ts] isSVG, slotScopeIds, optimized);
// [ts] setActiveBranch(suspense, fallbackVNode);
// [ts] };
// [ts] const delayEnter = fallbackVNode.transition && fallbackVNode.transition.mode == "out-in";
// [ts] if (delayEnter) {
// [ts] activeBranch.transition.afterLeave = mountFallback;
// [ts] }
// [ts] suspense.isInFallback = true;
// unmount current active branch
// [ts] unmount(activeBranch, parentComponent, null, // no suspense so unmount hooks fire now
// [ts] true // shouldRemove
// [ts] );
// [ts] if (!delayEnter) {
// [ts] mountFallback();
// [ts] }
// [ts] },
// [ts] move: (container, anchor, type) => {
// [ts] suspense.activeBranch &&
// [ts] move(suspense.activeBranch, container, anchor, type);
// [ts] suspense.container = container;
// [ts] },
// [ts] next: () => {
// [ts] return suspense.activeBranch && next(suspense.activeBranch);
// [ts] },
// [ts] registerDep: (instance, setupRenderEffect) => {
// [ts] const isInPendingSuspense = !!suspense.pendingBranch;
// [ts] if (isInPendingSuspense) {
// [ts] suspense.deps++;
// [ts] }
// [ts] const hydratedEl = instance.vnode.el;
// [ts] instance
// [ts] .asyncDep.catch(err => {
// [ts] handleError(err, instance, 0 /* SETUP_FUNCTION */);
// [ts] })
// [ts] .then(asyncSetupResult => {
// retry when the setup() promise resolves.
// component may have been unmounted before resolve.
// [ts] if (instance.isUnmounted ||
// [ts] suspense.isUnmounted ||
// [ts] suspense.pendingId != instance.suspenseId) {
// [ts] return;
// [ts] }
// retry from this component
// [ts] instance.asyncResolved = true;
// [ts] const { vnode } = instance;
// [ts] {
// [ts] pushWarningContext(vnode);
// [ts] }
// [ts] handleSetupResult(instance, asyncSetupResult, false);
// [ts] if (hydratedEl) {
// vnode may have been replaced if an update happened before the
// async dep is resolved.
// [ts] vnode.el = hydratedEl;
// [ts] }
// [ts] const placeholder = !hydratedEl && instance.subTree.el;
// [ts] setupRenderEffect(instance, vnode,
// component may have been moved before resolve.
// if this is not a hydration, instance.subTree will be the comment
// placeholder.
// [ts] parentNode(hydratedEl || instance.subTree.el),
// anchor will not be used if this is hydration, so only need to
// consider the comment placeholder case.
// [ts] if (hydratedEl) null else next(instance.subTree), suspense, isSVG, optimized);
// [ts] if (placeholder) {
// [ts] remove(placeholder);
// [ts] }
// [ts] updateHOCHostEl(instance, vnode.el);
// [ts] {
// [ts] popWarningContext();
// [ts] }
// only decrease deps count if suspense is not already resolved
// [ts] if (isInPendingSuspense && --suspense.deps == 0) {
// [ts] suspense.resolve();
// [ts] }
// [ts] });
// [ts] },
// [ts] unmount: (parentSuspense, doRemove) => {
// [ts] suspense.isUnmounted = true;
// [ts] if (suspense.activeBranch) {
// [ts] unmount(suspense.activeBranch, parentComponent, parentSuspense, doRemove);
// [ts] }
// [ts] if (suspense.pendingBranch) {
// [ts] unmount(suspense.pendingBranch, parentComponent, parentSuspense, doRemove);
// [ts] }
// [ts] }
// [ts] };
// [ts] return suspense;
// [ts] }
// [ts] fn hydrateSuspense(node: auto, vnode: auto, parentComponent: auto, parentSuspense: auto, isSVG: auto, slotScopeIds: auto, optimized: auto, rendererInternals: auto, hydrateNode: auto) {
// [ts] /* eslint-disable no-restricted-globals */
// [ts] const suspense = (vnode.suspense = createSuspenseBoundary(vnode, parentSuspense, parentComponent, node.parentNode, document.createElement("div"), null, isSVG, slotScopeIds, optimized, rendererInternals, true /* hydrating */));
// there are two possible scenarios for server-rendered suspense:
// - success: ssr content should be fully resolved
// - failure: ssr content should be the fallback branch.
// however, on the client we don't really know if it has failed or not
// attempt to hydrate the DOM assuming it has succeeded, but we still
// need to construct a suspense boundary first
// [ts] const result = hydrateNode(node, (suspense.pendingBranch = vnode.ssContent), parentComponent, suspense, slotScopeIds, optimized);
// [ts] if (suspense.deps == 0) {
// [ts] suspense.resolve();
// [ts] }
// [ts] return result;
// [ts] /* eslint-enable no-restricted-globals */
// [ts] }
// [ts] fn normalizeSuspenseChildren(vnode: auto) {
// [ts] const { shapeFlag, children } = vnode;
// [ts] const isSlotChildren = shapeFlag & 32 /* SLOTS_CHILDREN */;
// [ts] vnode.ssContent = normalizeSuspenseSlot(if (isSlotChildren) children.default else children);
// [ts] vnode.ssFallback = if (isSlotChildren) normalizeSuspenseSlot(children.fallback) else createVNode(Comment);
// [ts] }
// [ts] fn normalizeSuspenseSlot(s: auto) {
// [ts] let block;
// [ts] if (isFunction(s)) {
// [ts] const isCompiledSlot = s._c;
// [ts] if (isCompiledSlot) {
// disableTracking: false
// allow block tracking for compiled slots
// (see ./componentRenderContext.ts)
// [ts] s._d = false;
// [ts] openBlock();
// [ts] }
// [ts] s = s();
// [ts] if (isCompiledSlot) {
// [ts] s._d = true;
// [ts] block = currentBlock;
// [ts] closeBlock();
// [ts] }
// [ts] }
// [ts] if (isArray(s)) {
// [ts] const singleChild = filterSingleRoot(s);
// [ts] if (!singleChild) {
// [ts] warn(`<Suspense> slots expect a single root node.`);
// [ts] }
// [ts] s = singleChild;
// [ts] }
// [ts] s = normalizeVNode(s);
// [ts] if (block) {
// [ts] s.dynamicChildren = block.filter(c => c != s);
// [ts] }
// [ts] return s;
// [ts] }
// [ts] fn queueEffectWithSuspense(fn: auto, suspense: auto) {
// [ts] if (suspense && suspense.pendingBranch) {
// [ts] if (isArray(fn)) {
// [ts] suspense.effects.push(...fn);
// [ts] }
// [ts] else {
// [ts] suspense.effects.push(fn);
// [ts] }
// [ts] }
// [ts] else {
// [ts] queuePostFlushCb(fn);
// [ts] }
// [ts] }
// [ts] fn setActiveBranch(suspense: auto, branch: auto) {
// [ts] suspense.activeBranch = branch;
// [ts] const { vnode, parentComponent } = suspense;
// [ts] const el = (vnode.el = branch.el);
// in case suspense is the root node of a component,
// recursively update the HOC el
// [ts] if (parentComponent && parentComponent.subTree == vnode) {
// [ts] parentComponent.vnode.el = el;
// [ts] updateHOCHostEl(parentComponent, el);
// [ts] }
// [ts] }
// [ts] fn provide(key: auto, value: auto) {
// [ts] if (!currentInstance) {
// [ts] {
// [ts] warn(`provide() can only be used inside setup().`);
// [ts] }
// [ts] }
// [ts] else {
// [ts] let provides = currentInstance.provides;
// by default an instance inherits its parent's provides object
// but when it needs to provide values of its own, it creates its
// own provides object using parent provides object as prototype.
// this way in `inject` we can simply look up injections from direct
// parent and let the prototype chain do the work.
// [ts] const parentProvides = currentInstance.parent && currentInstance.parent.provides;
// [ts] if (parentProvides == provides) {
// [ts] provides = currentInstance.provides = Object.create(parentProvides);
// [ts] }
// TS doesn't allow symbol as index type
// [ts] provides[key] = value;
// [ts] }
// [ts] }
// [ts] fn inject(key: auto, defaultValue: auto, treatDefaultAsFactory: auto = false) {
// fallback to `currentRenderingInstance` so that this can be called in
// a functional component
// [ts] const instance = currentInstance || currentRenderingInstance;
// [ts] if (instance) {
// #2400
// to support `app.use` plugins,
// fallback to appContext's `provides` if the intance is at root
// [ts] const provides = if (instance.parent == null) instance.vnode.appContext && instance.vnode.appContext.provides else instance.parent.provides;
// [ts] if (provides && key == provides) {
// TS doesn't allow symbol as index type
// [ts] return provides[key];
// [ts] }
// [ts] else if (arguments.length > 1) {
// [ts] return if (treatDefaultAsFactory && isFunction(defaultValue)) defaultValue.call(instance.proxy) else defaultValue;
// [ts] }
// [ts] else {
// [ts] warn(`injection "${String(key)}" not found.`);
// [ts] }
// [ts] }
// [ts] else {
// [ts] warn(`inject() can only be used inside setup() or functional components.`);
// [ts] }
// [ts] }
// Simple effect.
// [ts] fn watchEffect(effect: auto, options: auto) {
// [ts] return doWatch(effect, null, options);
// [ts] }
// initial value for watchers to trigger on undefined initial values
// [ts] const INITIAL_WATCHER_VALUE = {};
// implementation
// [ts] fn watch(source: auto, cb: auto, options: auto) {
// [ts] if (!isFunction(cb)) {
// [ts] warn(`\`watch(fn, options?)\` signature has been moved to a separate API. ` +
// [ts] `Use \`watchEffect(fn, options?)\` instead. \`watch\` now only ` +
// [ts] `supports \`watch(source, cb, options?) signature.`);
// [ts] }
// [ts] return doWatch(source, cb, options);
// [ts] }
// [ts] fn doWatch(source: auto, cb: auto, _param: string, instance: auto = currentInstance) {
// [ts] if (!cb) {
// [ts] if (immediate != null) {
// [ts] warn(`watch() "immediate" option is only respected when using the ` +
// [ts] `watch(source, callback, options?) signature.`);
// [ts] }
// [ts] if (deep != null) {
// [ts] warn(`watch() "deep" option is only respected when using the ` +
// [ts] `watch(source, callback, options?) signature.`);
// [ts] }
// [ts] }
// [ts] const warnInvalidSource = (s) => {
// [ts] warn(`Invalid watch source: `, s, `A watch source can only be a getter/effect function, a ref, ` +
// [ts] `a reactive object, or an array of these types.`);
// [ts] };
// [ts] let getter;
// [ts] let forceTrigger = false;
// [ts] let isMultiSource = false;
// [ts] if (isRef(source)) {
// [ts] getter = () => source.value;
// [ts] forceTrigger = !!source._shallow;
// [ts] }
// [ts] else if (isReactive(source)) {
// [ts] getter = () => source;
// [ts] deep = true;
// [ts] }
// [ts] else if (isArray(source)) {
// [ts] isMultiSource = true;
// [ts] forceTrigger = source.some(isReactive);
// [ts] getter = () => source.map(s => {
// [ts] if (isRef(s)) {
// [ts] return s.value;
// [ts] }
// [ts] else if (isReactive(s)) {
// [ts] return traverse(s);
// [ts] }
// [ts] else if (isFunction(s)) {
// [ts] return callWithErrorHandling(s, instance, 2 /* WATCH_GETTER */);
// [ts] }
// [ts] else {
// [ts] warnInvalidSource(s);
// [ts] }
// [ts] });
// [ts] }
// [ts] else if (isFunction(source)) {
// [ts] if (cb) {
// getter with cb
// [ts] getter = () => callWithErrorHandling(source, instance, 2 /* WATCH_GETTER */);
// [ts] }
// [ts] else {
// no cb -> simple effect
// [ts] getter = () => {
// [ts] if (instance && instance.isUnmounted) {
// [ts] return;
// [ts] }
// [ts] if (cleanup) {
// [ts] cleanup();
// [ts] }
// [ts] return callWithAsyncErrorHandling(source, instance, 3 /* WATCH_CALLBACK */, [onInvalidate]);
// [ts] };
// [ts] }
// [ts] }
// [ts] else {
// [ts] getter = NOOP;
// [ts] warnInvalidSource(source);
// [ts] }
// [ts] if (cb && deep) {
// [ts] const baseGetter = getter;
// [ts] getter = () => traverse(baseGetter());
// [ts] }
// [ts] let cleanup;
// [ts] let onInvalidate = (fn) => {
// [ts] cleanup = runner.options.onStop = () => {
// [ts] callWithErrorHandling(fn, instance, 4 /* WATCH_CLEANUP */);
// [ts] };
// [ts] };
// [ts] let oldValue = if (isMultiSource) [] else INITIAL_WATCHER_VALUE;
// [ts] const job = () => {
// [ts] if (!runner.active) {
// [ts] return;
// [ts] }
// [ts] if (cb) {
// watch(source, cb)
// [ts] const newValue = runner();
// [ts] if (deep ||
// [ts] forceTrigger ||
// [ts] (if (isMultiSource) newValue.some((v, i) => hasChanged(v, oldValue[i])) else hasChanged(newValue, oldValue)) ||
// [ts] (false  )) {
// cleanup before running cb again
// [ts] if (cleanup) {
// [ts] cleanup();
// [ts] }
// [ts] callWithAsyncErrorHandling(cb, instance, 3 /* WATCH_CALLBACK */, [
// [ts] newValue,
// pass undefined as the old value when it's changed for the first time
// [ts] if (oldValue == INITIAL_WATCHER_VALUE) null else oldValue,
// [ts] onInvalidate
// [ts] ]);
// [ts] oldValue = newValue;
// [ts] }
// [ts] }
// [ts] else {
// watchEffect
// [ts] runner();
// [ts] }
// [ts] };
// important: mark the job as a watcher callback so that scheduler knows
// it is allowed to self-trigger (#1727)
// [ts] job.allowRecurse = !!cb;
// [ts] let scheduler;
// [ts] if (flush == "sync") {
// [ts] scheduler = job; // the scheduler function gets called directly
// [ts] }
// [ts] else if (flush == "post") {
// [ts] scheduler = () => queuePostRenderEffect(job, instance && instance.suspense);
// [ts] }
// [ts] else {
// default: 'pre'
// [ts] scheduler = () => {
// [ts] if (!instance || instance.isMounted) {
// [ts] queuePreFlushCb(job);
// [ts] }
// [ts] else {
// with 'pre' option, the first call must happen before
// the component is mounted so it is called synchronously.
// [ts] job();
// [ts] }
// [ts] };
// [ts] }
// [ts] const runner = effect(getter, {
// [ts] lazy: true,
// [ts] onTrack,
// [ts] onTrigger,
// [ts] scheduler
// [ts] });
// [ts] recordInstanceBoundEffect(runner, instance);
// initial run
// [ts] if (cb) {
// [ts] if (immediate) {
// [ts] job();
// [ts] }
// [ts] else {
// [ts] oldValue = runner();
// [ts] }
// [ts] }
// [ts] else if (flush == "post") {
// [ts] queuePostRenderEffect(runner, instance && instance.suspense);
// [ts] }
// [ts] else {
// [ts] runner();
// [ts] }
// [ts] return () => {
// [ts] stop(runner);
// [ts] if (instance) {
// [ts] remove(instance.effects, runner);
// [ts] }
// [ts] };
// [ts] }
// this.$watch
// [ts] fn instanceWatch(source: auto, value: auto, options: auto) {
// [ts] const publicThis = _.proxy;
// [ts] const getter = isString(source) ? source.includes(".") ? createPathGetter(publicThis, source) => publicThis[source] : source.bind(publicThis, publicThis);
// [ts] let cb;
// [ts] if (isFunction(value)) {
// [ts] cb = value;
// [ts] }
// [ts] else {
// [ts] cb = value.handler;
// [ts] options = value;
// [ts] }
// [ts] return doWatch(getter, cb.bind(publicThis), options, this);
// [ts] }
// [ts] fn createPathGetter(ctx: auto, path: auto) {
// [ts] const segments = path.split(".");
// [ts] return () => {
// [ts] let cur = ctx;
// [ts] let i = 0;
// [ts] while (i < segments.length && cur) { // [cap-migrate] for-step: i++
// [ts] cur = cur[segments[i]];
// [ts] }
// [ts] return cur;
// [ts] };
// [ts] }
// [ts] fn traverse(value: auto, seen: auto = Set.init()) {
// [ts] if (!isObject(value) || value["__v_skip" /* SKIP */]) {
// [ts] return value;
// [ts] }
// [ts] seen = seen || Set.init();
// [ts] if (seen.has(value)) {
// [ts] return value;
// [ts] }
// [ts] seen.add(value);
// [ts] if (isRef(value)) {
// [ts] traverse(value.value, seen);
// [ts] }
// [ts] else if (isArray(value)) {
// [ts] let i = 0;
// [ts] while (i < value.length) { // [cap-migrate] for-step: i++
// [ts] traverse(value[i], seen);
// [ts] }
// [ts] }
// [ts] else if (isSet(value) || isMap(value)) {
// [ts] value.forEach((v) => {
// [ts] traverse(v, seen);
// [ts] });
// [ts] }
// [ts] else if (isPlainObject(value)) {
// [ts] for (const key in value) {
// [ts] traverse(value[key], seen);
// [ts] }
// [ts] }
// [ts] return value;
// [ts] }
// [ts] fn useTransitionState() {
// [ts] const state = {
// [ts] isMounted: false,
// [ts] isLeaving: false,
// [ts] isUnmounting: false,
// [ts] leavingVNodes: Map.init()
// [ts] };
// [ts] onMounted(() => {
// [ts] state.isMounted = true;
// [ts] });
// [ts] onBeforeUnmount(() => {
// [ts] state.isUnmounting = true;
// [ts] });
// [ts] return state;
// [ts] }
// [ts] const TransitionHookValidator = [Function, Array];
// [ts] const BaseTransitionImpl = {
// [ts] name: `BaseTransition`,
// [ts] props: {
// [ts] mode: String,
// [ts] appear: Boolean,
// [ts] persisted: Boolean,
// enter
// [ts] onBeforeEnter: TransitionHookValidator,
// [ts] onEnter: TransitionHookValidator,
// [ts] onAfterEnter: TransitionHookValidator,
// [ts] onEnterCancelled: TransitionHookValidator,
// leave
// [ts] onBeforeLeave: TransitionHookValidator,
// [ts] onLeave: TransitionHookValidator,
// [ts] onAfterLeave: TransitionHookValidator,
// [ts] onLeaveCancelled: TransitionHookValidator,
// appear
// [ts] onBeforeAppear: TransitionHookValidator,
// [ts] onAppear: TransitionHookValidator,
// [ts] onAfterAppear: TransitionHookValidator,
// [ts] onAppearCancelled: TransitionHookValidator
// [ts] },
// [ts] setup: (props, _p) => {
// [ts] const instance = getCurrentInstance();
// [ts] const state = useTransitionState();
// [ts] let prevTransitionKey;
// [ts] return () => {
// [ts] const children = slots.default && getTransitionRawChildren(slots.default(), true);
// [ts] if (!children || !children.length) {
// [ts] return;
// [ts] }
// warn multiple elements
// [ts] if (children.length > 1) {
// [ts] warn("<transition> can only be used on a single element or component. Use " +
// [ts] "<transition-group> for lists.");
// [ts] }
// there's no need to track reactivity for these props so use the raw
// props for a bit better perf
// [ts] const rawProps = toRaw(props);
// [ts] const { mode } = rawProps;
// check mode
// [ts] if (mode && !["in-out", "out-in", "default"].includes(mode)) {
// [ts] warn(`invalid <transition> mode: ${mode}`);
// [ts] }
// at this point children has a guaranteed length of 1.
// [ts] const child = children[0];
// [ts] if (state.isLeaving) {
// [ts] return emptyPlaceholder(child);
// [ts] }
// in the case of <transition><keep-alive/></transition>, we need to
// compare the type of the kept-alive children.
// [ts] const innerChild = getKeepAliveChild(child);
// [ts] if (!innerChild) {
// [ts] return emptyPlaceholder(child);
// [ts] }
// [ts] const enterHooks = resolveTransitionHooks(innerChild, rawProps, state, instance);
// [ts] setTransitionHooks(innerChild, enterHooks);
// [ts] const oldChild = instance.subTree;
// [ts] const oldInnerChild = oldChild && getKeepAliveChild(oldChild);
// [ts] let transitionKeyChanged = false;
// [ts] const { getTransitionKey } = innerChild.type;
// [ts] if (getTransitionKey) {
// [ts] const key = getTransitionKey();
// [ts] if (prevTransitionKey == null) {
// [ts] prevTransitionKey = key;
// [ts] }
// [ts] else if (key != prevTransitionKey) {
// [ts] prevTransitionKey = key;
// [ts] transitionKeyChanged = true;
// [ts] }
// [ts] }
// handle mode
// [ts] if (oldInnerChild &&
// [ts] oldInnerChild.type != Comment$1 &&
// [ts] (!isSameVNodeType(innerChild, oldInnerChild) || transitionKeyChanged)) {
// [ts] const leavingHooks = resolveTransitionHooks(oldInnerChild, rawProps, state, instance);
// update old tree's hooks in case of dynamic transition
// [ts] setTransitionHooks(oldInnerChild, leavingHooks);
// switching between different views
// [ts] if (mode == "out-in") {
// [ts] state.isLeaving = true;
// return placeholder node and queue update when leave finishes
// [ts] leavingHooks.afterLeave = () => {
// [ts] state.isLeaving = false;
// [ts] instance.update();
// [ts] };
// [ts] return emptyPlaceholder(child);
// [ts] }
// [ts] else if (mode == "in-out" && innerChild.type != Comment$1) {
// [ts] leavingHooks.delayLeave = (el, earlyRemove, delayedLeave) => {
// [ts] const leavingVNodesCache = getLeavingNodesForType(state, oldInnerChild);
// [ts] leavingVNodesCache[String(oldInnerChild.key)] = oldInnerChild;
// early removal callback
// [ts] el._leaveCb = () => {
// [ts] earlyRemove();
// [ts] el._leaveCb = null;
// [ts] delete enterHooks.delayedLeave;
// [ts] };
// [ts] enterHooks.delayedLeave = delayedLeave;
// [ts] };
// [ts] }
// [ts] }
// [ts] return child;
// [ts] };
// [ts] }
// [ts] };
// export the public type for h/tsx inference
// also to avoid inline import() in generated d.ts files
// [ts] const BaseTransition = BaseTransitionImpl;
// [ts] fn getLeavingNodesForType(state: auto, vnode: auto) {
// [ts] const { leavingVNodes } = state;
// [ts] let leavingVNodesCache = leavingVNodes.get(vnode.type);
// [ts] if (!leavingVNodesCache) {
// [ts] leavingVNodesCache = Object.create(null);
// [ts] leavingVNodes.set(vnode.type, leavingVNodesCache);
// [ts] }
// [ts] return leavingVNodesCache;
// [ts] }
// The transition hooks are attached to the vnode as vnode.transition
// and will be called at appropriate timing in the renderer.
// [ts] fn resolveTransitionHooks(vnode: auto, props: auto, state: auto, instance: auto) {
// [ts] const { appear, mode, persisted = false, onBeforeEnter, onEnter, onAfterEnter, onEnterCancelled, onBeforeLeave, onLeave, onAfterLeave, onLeaveCancelled, onBeforeAppear, onAppear, onAfterAppear, onAppearCancelled } = props;
// [ts] const key = String(vnode.key);
// [ts] const leavingVNodesCache = getLeavingNodesForType(state, vnode);
// [ts] const callHook = (hook, args) => {
// [ts] hook &&
// [ts] callWithAsyncErrorHandling(hook, instance, 9 /* TRANSITION_HOOK */, args);
// [ts] };
// [ts] const hooks = {
// [ts] mode,
// [ts] persisted,
// [ts] beforeEnter: (el) => {
// [ts] let hook = onBeforeEnter;
// [ts] if (!state.isMounted) {
// [ts] if (appear) {
// [ts] hook = onBeforeAppear || onBeforeEnter;
// [ts] }
// [ts] else {
// [ts] return;
// [ts] }
// [ts] }
// for same element (v-show)
// [ts] if (el._leaveCb) {
// [ts] el._leaveCb(true /* cancelled */);
// [ts] }
// for toggled element with same key (v-if)
// [ts] const leavingVNode = leavingVNodesCache[key];
// [ts] if (leavingVNode &&
// [ts] isSameVNodeType(vnode, leavingVNode) &&
// [ts] leavingVNode.el._leaveCb) {
// force early removal (not cancelled)
// [ts] leavingVNode.el._leaveCb();
// [ts] }
// [ts] callHook(hook, [el]);
// [ts] },
// [ts] enter: (el) => {
// [ts] let hook = onEnter;
// [ts] let afterHook = onAfterEnter;
// [ts] let cancelHook = onEnterCancelled;
// [ts] if (!state.isMounted) {
// [ts] if (appear) {
// [ts] hook = onAppear || onEnter;
// [ts] afterHook = onAfterAppear || onAfterEnter;
// [ts] cancelHook = onAppearCancelled || onEnterCancelled;
// [ts] }
// [ts] else {
// [ts] return;
// [ts] }
// [ts] }
// [ts] let called = false;
// [ts] const done = (el._enterCb = (cancelled) => {
// [ts] if (called)
// [ts] return;
// [ts] called = true;
// [ts] if (cancelled) {
// [ts] callHook(cancelHook, [el]);
// [ts] }
// [ts] else {
// [ts] callHook(afterHook, [el]);
// [ts] }
// [ts] if (hooks.delayedLeave) {
// [ts] hooks.delayedLeave();
// [ts] }
// [ts] el._enterCb = null;
// [ts] });
// [ts] if (hook) {
// [ts] hook(el, done);
// [ts] if (hook.length <= 1) {
// [ts] done();
// [ts] }
// [ts] }
// [ts] else {
// [ts] done();
// [ts] }
// [ts] },
// [ts] leave: (el, remove) => {
// [ts] const key = String(vnode.key);
// [ts] if (el._enterCb) {
// [ts] el._enterCb(true /* cancelled */);
// [ts] }
// [ts] if (state.isUnmounting) {
// [ts] return remove();
// [ts] }
// [ts] callHook(onBeforeLeave, [el]);
// [ts] let called = false;
// [ts] const done = (el._leaveCb = (cancelled) => {
// [ts] if (called)
// [ts] return;
// [ts] called = true;
// [ts] remove();
// [ts] if (cancelled) {
// [ts] callHook(onLeaveCancelled, [el]);
// [ts] }
// [ts] else {
// [ts] callHook(onAfterLeave, [el]);
// [ts] }
// [ts] el._leaveCb = null;
// [ts] if (leavingVNodesCache[key] == vnode) {
// [ts] delete leavingVNodesCache[key];
// [ts] }
// [ts] });
// [ts] leavingVNodesCache[key] = vnode;
// [ts] if (onLeave) {
// [ts] onLeave(el, done);
// [ts] if (onLeave.length <= 1) {
// [ts] done();
// [ts] }
// [ts] }
// [ts] else {
// [ts] done();
// [ts] }
// [ts] },
// [ts] clone: (vnode) => {
// [ts] return resolveTransitionHooks(vnode, props, state, instance);
// [ts] }
// [ts] };
// [ts] return hooks;
// [ts] }
// the placeholder really only handles one special case: KeepAlive
// in the case of a KeepAlive in a leave phase we need to return a KeepAlive
// placeholder with empty content to avoid the KeepAlive instance from being
// unmounted.
// [ts] fn emptyPlaceholder(vnode: auto) {
// [ts] if (isKeepAlive(vnode)) {
// [ts] vnode = cloneVNode(vnode);
// [ts] vnode.children = null;
// [ts] return vnode;
// [ts] }
// [ts] }
// [ts] fn getKeepAliveChild(vnode: auto) {
// [ts] return if (isKeepAlive(vnode)) if (vnode.children) vnode.children[0] else null else vnode;
// [ts] }
// [ts] fn setTransitionHooks(vnode: auto, hooks: auto) {
// [ts] if (vnode.shapeFlag & 6 /* COMPONENT */ && vnode.component) {
// [ts] setTransitionHooks(vnode.component.subTree, hooks);
// [ts] }
// [ts] else if (vnode.shapeFlag & 128 /* SUSPENSE */) {
// [ts] vnode.ssContent.transition = hooks.clone(vnode.ssContent);
// [ts] vnode.ssFallback.transition = hooks.clone(vnode.ssFallback);
// [ts] }
// [ts] else {
// [ts] vnode.transition = hooks;
// [ts] }
// [ts] }
// [ts] fn getTransitionRawChildren(children: auto, keepComment: auto = false) {
// [ts] let ret = [];
// [ts] let keyedFragmentCount = 0;
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// handle fragment children case, e.g. v-for
// [ts] if (child.type == Fragment) {
// [ts] if (child.patchFlag & 128 /* KEYED_FRAGMENT */)
// [ts] keyedFragmentCount++;
// [ts] ret = ret.concat(getTransitionRawChildren(child.children, keepComment));
// [ts] }
// comment placeholders should be skipped, e.g. v-if
// [ts] else if (keepComment || child.type != Comment$1) {
// [ts] ret.push(child);
// [ts] }
// [ts] }
// #1126 if a transition children list contains multiple sub fragments, these
// fragments will be merged into a flat children array. Since each v-for
// fragment may contain different static bindings inside, we need to de-op
// these children to force full diffs to ensure correct behavior.
// [ts] if (keyedFragmentCount > 1) {
// [ts] let i = 0;
// [ts] while (i < ret.length) { // [cap-migrate] for-step: i++
// [ts] ret[i].patchFlag = -2 /* BAIL */;
// [ts] }
// [ts] }
// [ts] return ret;
// [ts] }
// implementation, close to no-op
// [ts] fn defineComponent(options: auto) {
// [ts] return if (isFunction(options)) { setup: options, name: options.name } else options;
// [ts] }
// [ts] fn isAsyncWrapper(i: auto) { return !!i.type.__asyncLoader; }
// [ts] fn defineAsyncComponent(source: auto) {
// [ts] if (isFunction(source)) {
// [ts] source = { loader: source };
// [ts] }
// [ts] const { loader, loadingComponent, errorComponent, delay = 200, timeout, // undefined = never times out
// [ts] suspensible = true, onError: userOnError } = source;
// [ts] let pendingRequest = null;
// [ts] let resolvedComp;
// [ts] let retries = 0;
// [ts] const retry = () => {
// [ts] retries++;
// [ts] pendingRequest = null;
// [ts] return load();
// [ts] };
// [ts] const load = () => {
// [ts] let thisRequest;
// [ts] return (pendingRequest ||
// [ts] (thisRequest = pendingRequest = loader()
// [ts] .catch(err => {
// [ts] err = if (err == Error) err else Error.init(String(err));
// [ts] if (userOnError) {
// [ts] return Promise.init((resolve, reject) => {
// [ts] const userRetry = () => resolve(retry());
// [ts] const userFail = () => reject(err);
// [ts] userOnError(err, userRetry, userFail, retries + 1);
// [ts] });
// [ts] }
// [ts] else {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw err;)
// [ts] }
// [ts] })
// [ts] .then((comp) => {
// [ts] if (thisRequest != pendingRequest && pendingRequest) {
// [ts] return pendingRequest;
// [ts] }
// [ts] if (!comp) {
// [ts] warn(`Async component loader resolved to undefined. ` +
// [ts] `If you are using retry(), make sure to return its return value.`);
// [ts] }
// interop module default
// [ts] if (comp &&
// [ts] (comp.__esModule || comp["__symbol_toStringTag"] == "Module")) {
// [ts] comp = comp.default;
// [ts] }
// [ts] if (comp && !isObject(comp) && !isFunction(comp)) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Invalid async component load result: ${comp}`);)
// [ts] }
// [ts] resolvedComp = comp;
// [ts] return comp;
// [ts] })));
// [ts] };
// [ts] return defineComponent({
// [ts] name: "AsyncComponentWrapper",
// [ts] __asyncLoader: load,
// [ts] get __asyncResolved() {
// [ts] return resolvedComp;
// [ts] },
// [ts] setup: () => {
// [ts] const instance = currentInstance;
// already resolved
// [ts] if (resolvedComp) {
// [ts] return () => createInnerComp(resolvedComp, instance);
// [ts] }
// [ts] const onError = (err) => {
// [ts] pendingRequest = null;
// [ts] handleError(err, instance, 13 /* ASYNC_COMPONENT_LOADER */, !errorComponent /* do not throw == dev if user provided error component */);
// [ts] };
// suspense-controlled or SSR.
// [ts] if ((suspensible && instance.suspense) ||
// [ts] (false )) {
// [ts] return load()
// [ts] .then(comp => {
// [ts] return () => createInnerComp(comp, instance);
// [ts] })
// [ts] .catch(err => {
// [ts] onError(err);
// [ts] return () => errorComponent ? createVNode(errorComponent, {
// [ts] error: err
// [ts] }) : null;
// [ts] });
// [ts] }
// [ts] const loaded = ref(false);
// [ts] const error = ref();
// [ts] const delayed = ref(!!delay);
// [ts] if (delay) {
// [ts] setTimeout(() => {
// [ts] delayed.value = false;
// [ts] }, delay);
// [ts] }
// [ts] if (timeout != null) {
// [ts] setTimeout(() => {
// [ts] if (!loaded.value && !error.value) {
// [ts] const err = Error.init(`Async component timed out after ${timeout}ms.`);
// [ts] onError(err);
// [ts] error.value = err;
// [ts] }
// [ts] }, timeout);
// [ts] }
// [ts] load()
// [ts] .then(() => {
// [ts] loaded.value = true;
// [ts] if (instance.parent && isKeepAlive(instance.parent.vnode)) {
// parent is keep-alive, force update so the loaded component's
// name is taken into account
// [ts] queueJob(instance.parent.update);
// [ts] }
// [ts] })
// [ts] .catch(err => {
// [ts] onError(err);
// [ts] error.value = err;
// [ts] });
// [ts] return () => {
// [ts] if (loaded.value && resolvedComp) {
// [ts] return createInnerComp(resolvedComp, instance);
// [ts] }
// [ts] else if (error.value && errorComponent) {
// [ts] return createVNode(errorComponent, {
// [ts] error: error.value
// [ts] });
// [ts] }
// [ts] else if (loadingComponent && !delayed.value) {
// [ts] return createVNode(loadingComponent);
// [ts] }
// [ts] };
// [ts] }
// [ts] });
// [ts] }
// [ts] fn createInnerComp(comp: auto, _param: string) {
// [ts] const vnode = createVNode(comp, props, children);
// ensure inner component inherits the async wrapper's ref owner
// [ts] vnode.ref = ref;
// [ts] return vnode;
// [ts] }
// [ts] fn isKeepAlive(vnode: auto) { return vnode.type.__isKeepAlive; }
// [ts] const KeepAliveImpl = {
// [ts] name: `KeepAlive`,
// Marker for special handling inside the renderer. We are not using a ===
// check directly on KeepAlive in the renderer, because importing it directly
// would prevent it from being tree-shaken.
// [ts] __isKeepAlive: true,
// [ts] props: {
// [ts] include: [String, RegExp, Array],
// [ts] exclude: [String, RegExp, Array],
// [ts] max: [String, Number]
// [ts] },
// [ts] setup: (props, _p) => {
// [ts] const instance = getCurrentInstance();
// KeepAlive communicates with the instantiated renderer via the
// ctx where the renderer passes in its internals,
// and the KeepAlive instance exposes activate/deactivate implementations.
// The whole point of this is to avoid importing KeepAlive directly in the
// renderer to facilitate tree-shaking.
// [ts] const sharedContext = instance.ctx;
// if the internal renderer is not registered, it indicates that this is server-side rendering,
// for KeepAlive, we just need to render its children
// [ts] if (!sharedContext.renderer) {
// [ts] return slots.default;
// [ts] }
// [ts] const cache = Map.init();
// [ts] const keys = Set.init();
// [ts] let current = null;
// [ts] {
// [ts] instance.__v_cache = cache;
// [ts] }
// [ts] const parentSuspense = instance.suspense;
// [ts] const { renderer: { p: patch, m: move, um: _unmount, o: { createElement } } } = sharedContext;
// [ts] const storageContainer = createElement("div");
// [ts] sharedContext.activate = (vnode, container, anchor, isSVG, optimized) => {
// [ts] const instance = vnode.component;
// [ts] move(vnode, container, anchor, 0 /* ENTER */, parentSuspense);
// in case props have changed
// [ts] patch(instance.vnode, vnode, container, anchor, instance, parentSuspense, isSVG, vnode.slotScopeIds, optimized);
// [ts] queuePostRenderEffect(() => {
// [ts] instance.isDeactivated = false;
// [ts] if (instance.a) {
// [ts] invokeArrayFns(instance.a);
// [ts] }
// [ts] const vnodeHook = vnode.props && vnode.props.onVnodeMounted;
// [ts] if (vnodeHook) {
// [ts] invokeVNodeHook(vnodeHook, instance.parent, vnode);
// [ts] }
// [ts] }, parentSuspense);
// [ts] {
// Update components tree
// [ts] devtoolsComponentAdded(instance);
// [ts] }
// [ts] };
// [ts] sharedContext.deactivate = (vnode) => {
// [ts] const instance = vnode.component;
// [ts] move(vnode, storageContainer, null, 1 /* LEAVE */, parentSuspense);
// [ts] queuePostRenderEffect(() => {
// [ts] if (instance.da) {
// [ts] invokeArrayFns(instance.da);
// [ts] }
// [ts] const vnodeHook = vnode.props && vnode.props.onVnodeUnmounted;
// [ts] if (vnodeHook) {
// [ts] invokeVNodeHook(vnodeHook, instance.parent, vnode);
// [ts] }
// [ts] instance.isDeactivated = true;
// [ts] }, parentSuspense);
// [ts] {
// Update components tree
// [ts] devtoolsComponentAdded(instance);
// [ts] }
// [ts] };
// [ts] fn unmount(vnode: auto) {
// reset the shapeFlag so it can be properly unmounted
// [ts] resetShapeFlag(vnode);
// [ts] _unmount(vnode, instance, parentSuspense);
// [ts] }
// [ts] fn pruneCache(filter: auto) {
// [ts] cache.forEach((vnode, key) => {
// [ts] const name = getComponentName(vnode.type);
// [ts] if (name && (!filter || !filter(name))) {
// [ts] pruneCacheEntry(key);
// [ts] }
// [ts] });
// [ts] }
// [ts] fn pruneCacheEntry(key: auto) {
// [ts] const cached = cache.get(key);
// [ts] if (!current || cached.type != current.type) {
// [ts] unmount(cached);
// [ts] }
// [ts] else if (current) {
// current active instance should no longer be kept-alive.
// we can't unmount it now but it might be later, so reset its flag now.
// [ts] resetShapeFlag(current);
// [ts] }
// [ts] cache.delete(key);
// [ts] keys.delete(key);
// [ts] }
// prune cache on include/exclude prop change
// [ts] watch(() => [props.include, props.exclude], (_p) => {
// [ts] include && pruneCache(name => matches(include, name));
// [ts] exclude && pruneCache(name => !matches(exclude, name));
// [ts] },
// prune post-render after `current` has been updated
// [ts] { flush: "post", deep: true });
// cache sub tree after render
// [ts] let pendingCacheKey = null;
// [ts] const cacheSubtree = () => {
// fix #1621, the pendingCacheKey could be 0
// [ts] if (pendingCacheKey != null) {
// [ts] cache.set(pendingCacheKey, getInnerChild(instance.subTree));
// [ts] }
// [ts] };
// [ts] onMounted(cacheSubtree);
// [ts] onUpdated(cacheSubtree);
// [ts] onBeforeUnmount(() => {
// [ts] cache.forEach(cached => {
// [ts] const { subTree, suspense } = instance;
// [ts] const vnode = getInnerChild(subTree);
// [ts] if (cached.type == vnode.type) {
// current instance will be unmounted as part of keep-alive's unmount
// [ts] resetShapeFlag(vnode);
// but invoke its deactivated hook here
// [ts] const da = vnode.component.da;
// [ts] da && queuePostRenderEffect(da, suspense);
// [ts] return;
// [ts] }
// [ts] unmount(cached);
// [ts] });
// [ts] });
// [ts] return () => {
// [ts] pendingCacheKey = null;
// [ts] if (!slots.default) {
// [ts] return null;
// [ts] }
// [ts] const children = slots.default();
// [ts] const rawVNode = children[0];
// [ts] if (children.length > 1) {
// [ts] {
// [ts] warn(`KeepAlive should contain exactly one component child.`);
// [ts] }
// [ts] current = null;
// [ts] return children;
// [ts] }
// [ts] else if (!isVNode(rawVNode) ||
// [ts] (!(rawVNode.shapeFlag & 4 /* STATEFUL_COMPONENT */) &&
// [ts] !(rawVNode.shapeFlag & 128 /* SUSPENSE */))) {
// [ts] current = null;
// [ts] return rawVNode;
// [ts] }
// [ts] let vnode = getInnerChild(rawVNode);
// [ts] const comp = vnode.type;
// for async components, name check should be based in its loaded
// inner component if available
// [ts] const name = getComponentName(if (isAsyncWrapper(vnode)) vnode.type.__asyncResolved || {} else comp);
// [ts] const { include, exclude, max } = props;
// [ts] if ((include && (!name || !matches(include, name))) ||
// [ts] (exclude && name && matches(exclude, name))) {
// [ts] current = vnode;
// [ts] return rawVNode;
// [ts] }
// [ts] const key = if (vnode.key == null) comp else vnode.key;
// [ts] const cachedVNode = cache.get(key);
// clone vnode if it's reused because we are going to mutate it
// [ts] if (vnode.el) {
// [ts] vnode = cloneVNode(vnode);
// [ts] if (rawVNode.shapeFlag & 128 /* SUSPENSE */) {
// [ts] rawVNode.ssContent = vnode;
// [ts] }
// [ts] }
// #1513 it's possible for the returned vnode to be cloned due to attr
// fallthrough or scopeId, so the vnode here may not be the final vnode
// that is mounted. Instead of caching it directly, we store the pending
// key and cache `instance.subTree` (the normalized vnode) in
// beforeMount/beforeUpdate hooks.
// [ts] pendingCacheKey = key;
// [ts] if (cachedVNode) {
// copy over mounted state
// [ts] vnode.el = cachedVNode.el;
// [ts] vnode.component = cachedVNode.component;
// [ts] if (vnode.transition) {
// recursively update transition hooks on subTree
// [ts] setTransitionHooks(vnode, vnode.transition);
// [ts] }
// avoid vnode being mounted as fresh
// [ts] vnode.shapeFlag |= 512 /* COMPONENT_KEPT_ALIVE */;
// make this key the freshest
// [ts] keys.delete(key);
// [ts] keys.add(key);
// [ts] }
// [ts] else {
// [ts] keys.add(key);
// prune oldest entry
// [ts] if (max && keys.size > parseInt(max, 10)) {
// [ts] pruneCacheEntry(keys.values().next().value);
// [ts] }
// [ts] }
// avoid vnode being unmounted
// [ts] vnode.shapeFlag |= 256 /* COMPONENT_SHOULD_KEEP_ALIVE */;
// [ts] current = vnode;
// [ts] return rawVNode;
// [ts] };
// [ts] }
// [ts] };
// export the public type for h/tsx inference
// also to avoid inline import() in generated d.ts files
// [ts] const KeepAlive = KeepAliveImpl;
// [ts] fn matches(pattern: auto, name: auto) {
// [ts] if (isArray(pattern)) {
// [ts] return pattern.some((p) => matches(p, name));
// [ts] }
// [ts] else if (isString(pattern)) {
// [ts] return pattern.split(",").indexOf(name) > -1;
// [ts] }
// [ts] else if (pattern.test) {
// [ts] return pattern.test(name);
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] return false;
// [ts] }
// [ts] fn onActivated(hook: auto, target: auto) {
// [ts] registerKeepAliveHook(hook, "a" /* ACTIVATED */, target);
// [ts] }
// [ts] fn onDeactivated(hook: auto, target: auto) {
// [ts] registerKeepAliveHook(hook, "da" /* DEACTIVATED */, target);
// [ts] }
// [ts] fn registerKeepAliveHook(hook: auto, type: auto, target: auto = currentInstance) {
// cache the deactivate branch check wrapper for injected hooks so the same
// hook can be properly deduped by the scheduler. "__wdc" stands for "with
// deactivation check".
// [ts] const wrappedHook = hook.__wdc ||
// [ts] (hook.__wdc = () => {
// only fire the hook if the target instance is NOT in a deactivated branch.
// [ts] let current = target;
// [ts] while (current) {
// [ts] if (current.isDeactivated) {
// [ts] return;
// [ts] }
// [ts] current = current.parent;
// [ts] }
// [ts] hook();
// [ts] });
// [ts] injectHook(type, wrappedHook, target);
// In addition to registering it on the target instance, we walk up the parent
// chain and register it on all ancestor instances that are keep-alive roots.
// This avoids the need to walk the entire component tree when invoking these
// hooks, and more importantly, avoids the need to track child components in
// arrays.
// [ts] if (target) {
// [ts] let current = target.parent;
// [ts] while (current && current.parent) {
// [ts] if (isKeepAlive(current.parent.vnode)) {
// [ts] injectToKeepAliveRoot(wrappedHook, type, target, current);
// [ts] }
// [ts] current = current.parent;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn injectToKeepAliveRoot(hook: auto, type: auto, target: auto, keepAliveRoot: auto) {
// injectHook wraps the original for error handling, so make sure to remove
// the wrapped version.
// [ts] const injected = injectHook(type, hook, keepAliveRoot, true /* prepend */);
// [ts] onUnmounted(() => {
// [ts] remove(keepAliveRoot[type], injected);
// [ts] }, target);
// [ts] }
// [ts] fn resetShapeFlag(vnode: auto) {
// [ts] let shapeFlag = vnode.shapeFlag;
// [ts] if (shapeFlag & 256 /* COMPONENT_SHOULD_KEEP_ALIVE */) {
// [ts] shapeFlag -= 256 /* COMPONENT_SHOULD_KEEP_ALIVE */;
// [ts] }
// [ts] if (shapeFlag & 512 /* COMPONENT_KEPT_ALIVE */) {
// [ts] shapeFlag -= 512 /* COMPONENT_KEPT_ALIVE */;
// [ts] }
// [ts] vnode.shapeFlag = shapeFlag;
// [ts] }
// [ts] fn getInnerChild(vnode: auto) {
// [ts] return if (vnode.shapeFlag & 128 /* SUSPENSE */) vnode.ssContent else vnode;
// [ts] }
// [ts] fn injectHook(type: auto, hook: auto, target: auto = currentInstance, prepend: auto = false) {
// [ts] if (target) {
// [ts] const hooks = target[type] || (target[type] = []);
// cache the error handling wrapper for injected hooks so the same hook
// can be properly deduped by the scheduler. "__weh" stands for "with error
// handling".
// [ts] const wrappedHook = hook.__weh ||
// [ts] (hook.__weh = (args) => {
// [ts] if (target.isUnmounted) {
// [ts] return;
// [ts] }
// disable tracking inside all lifecycle hooks
// since they can potentially be called inside effects.
// [ts] pauseTracking();
// Set currentInstance during hook invocation.
// This assumes the hook does not synchronously trigger other hooks, which
// can only be false when the user does something really funky.
// [ts] setCurrentInstance(target);
// [ts] const res = callWithAsyncErrorHandling(hook, target, type, args);
// [ts] setCurrentInstance(null);
// [ts] resetTracking();
// [ts] return res;
// [ts] });
// [ts] if (prepend) {
// [ts] hooks.unshift(wrappedHook);
// [ts] }
// [ts] else {
// [ts] hooks.push(wrappedHook);
// [ts] }
// [ts] return wrappedHook;
// [ts] }
// [ts] else {
// [ts] const apiName = toHandlerKey(ErrorTypeStrings[type].replace(" hook$", ""));
// [ts] warn(`${apiName} is called when there is no active component instance to be ` +
// [ts] `associated with. ` +
// [ts] `Lifecycle injection APIs can only be used during execution of setup().` +
// [ts] (` If you are using async setup(), make sure to register lifecycle ` +
// [ts] `hooks before the first await statement.`
// [ts] ));
// [ts] }
// [ts] }
// [ts] fn createHook(lifecycle: auto) { return (hook, target) => // post-create lifecycle registrations are noops during SSR (except for serverPrefetch); }
// [ts] (!isInSSRComponentSetup || lifecycle == "sp" /* SERVER_PREFETCH */) &&
// [ts] injectHook(lifecycle, hook, target);
// [ts] const onBeforeMount = createHook("bm" /* BEFORE_MOUNT */);
// [ts] const onMounted = createHook("m" /* MOUNTED */);
// [ts] const onBeforeUpdate = createHook("bu" /* BEFORE_UPDATE */);
// [ts] const onUpdated = createHook("u" /* UPDATED */);
// [ts] const onBeforeUnmount = createHook("bum" /* BEFORE_UNMOUNT */);
// [ts] const onUnmounted = createHook("um" /* UNMOUNTED */);
// [ts] const onServerPrefetch = createHook("sp" /* SERVER_PREFETCH */);
// [ts] const onRenderTriggered = createHook("rtg" /* RENDER_TRIGGERED */);
// [ts] const onRenderTracked = createHook("rtc" /* RENDER_TRACKED */);
// [ts] fn onErrorCaptured(hook: auto, target: auto = currentInstance) {
// [ts] injectHook("ec" /* ERROR_CAPTURED */, hook, target);
// [ts] }
// [ts] fn createDuplicateChecker() {
// [ts] const cache = Object.create(null);
// [ts] return (type, key) => {
// [ts] if (cache[key]) {
// [ts] warn(`${type} property "${key}" is already defined in ${cache[key]}.`);
// [ts] }
// [ts] else {
// [ts] cache[key] = type;
// [ts] }
// [ts] };
// [ts] }
// [ts] let shouldCacheAccess = true;
// [ts] fn applyOptions(instance: auto) {
// [ts] const options = resolveMergedOptions(instance);
// [ts] const publicThis = instance.proxy;
// [ts] const ctx = instance.ctx;
// do not cache property access on public proxy during state initialization
// [ts] shouldCacheAccess = false;
// call beforeCreate first before accessing other options since
// the hook may mutate resolved options (#2791)
// [ts] if (options.beforeCreate) {
// [ts] callHook(options.beforeCreate, instance, "bc" /* BEFORE_CREATE */);
// [ts] }
// [ts] const {
// state
// [ts] data: dataOptions, computed: computedOptions, methods, watch: watchOptions, provide: provideOptions, inject: injectOptions,
// lifecycle
// [ts] created, beforeMount, mounted, beforeUpdate, updated, activated, deactivated, beforeDestroy, beforeUnmount, destroyed, unmounted, render, renderTracked, renderTriggered, errorCaptured, serverPrefetch,
// public API
// [ts] expose, inheritAttrs,
// assets
// [ts] components, directives, filters } = options;
// [ts] const checkDuplicateProperties = createDuplicateChecker() ;
// [ts] {
// [ts] const [propsOptions] = instance.propsOptions;
// [ts] if (propsOptions) {
// [ts] for (const key in propsOptions) {
// [ts] checkDuplicateProperties("Props" /* PROPS */, key);
// [ts] }
// [ts] }
// [ts] }
// options initialization order (to be consistent with Vue 2):
// - props (already done outside of this function)
// - inject
// - methods
// - data (deferred since it relies on `this` access)
// - computed
// - watch (deferred since it relies on `this` access)
// [ts] if (injectOptions) {
// [ts] resolveInjections(injectOptions, ctx, checkDuplicateProperties);
// [ts] }
// [ts] if (methods) {
// [ts] for (const key in methods) {
// [ts] const methodHandler = methods[key];
// [ts] if (isFunction(methodHandler)) {
// In dev mode, we use the `createRenderContext` function to define methods to the proxy target,
// and those are read-only but reconfigurable, so it needs to be redefined here
// [ts] {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ctx, key, {
// [ts-skip] value: methodHandler.bind(publicThis),
// [ts-skip] configurable: true,
// [ts-skip] enumerable: true,
// [ts-skip] writable: true
// [ts-skip] });
// [ts] }
// [ts] {
// [ts] checkDuplicateProperties("Methods" /* METHODS */, key);
// [ts] }
// [ts] }
// [ts] else {
// [ts] warn(`Method "${key}" has type "${@typeName(methodHandler)}" in the component definition. ` +
// [ts] `Did you reference the function correctly?`);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (dataOptions) {
// [ts] if (!isFunction(dataOptions)) {
// [ts] warn(`The data option must be a function. ` +
// [ts] `Plain object usage is no longer supported.`);
// [ts] }
// [ts] const data = dataOptions.call(publicThis, publicThis);
// [ts] if (isPromise(data)) {
// [ts] warn(`data() returned a Promise - note data() cannot be async; If you ` +
// [ts] `intend to perform data fetching before component renders, use ` +
// [ts] `async setup() + <Suspense>.`);
// [ts] }
// [ts] if (!isObject(data)) {
// [ts] warn(`data() should return an object.`);
// [ts] }
// [ts] else {
// [ts] instance.data = reactive(data);
// [ts] {
// [ts] for (const key in data) {
// [ts] checkDuplicateProperties("Data" /* DATA */, key);
// expose data on ctx during dev
// [ts] if (key[0] != "$" && key[0] != "_") {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ctx, key, {
// [ts-skip] configurable: true,
// [ts-skip] enumerable: true,
// [ts-skip] get: () => data[key],
// [ts-skip] set: NOOP
// [ts-skip] });
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// state initialization complete at this point - start caching access
// [ts] shouldCacheAccess = true;
// [ts] if (computedOptions) {
// [ts] for (const key in computedOptions) {
// [ts] const opt = computedOptions[key];
// [ts] const get = if (isFunction(opt)) opt.bind(publicThis, publicThis) else if (isFunction(opt.get)) opt.get.bind(publicThis, publicThis) else NOOP;
// [ts] if (get == NOOP) {
// [ts] warn(`Computed property "${key}" has no getter.`);
// [ts] }
// [ts] const set = !isFunction(opt) && isFunction(opt.set) ? opt.set.bind(publicThis) => {
// [ts] warn(`Write operation failed: computed property "${key}" is readonly.`);
// [ts] }
// [ts] ;
// [ts] const c = computed$1({
// [ts] get,
// [ts] set
// [ts] });
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ctx, key, {
// [ts-skip] enumerable: true,
// [ts-skip] configurable: true,
// [ts-skip] get: () => c.value,
// [ts-skip] set: v => (c.value = v)
// [ts-skip] });
// [ts] {
// [ts] checkDuplicateProperties("Computed" /* COMPUTED */, key);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (watchOptions) {
// [ts] for (const key in watchOptions) {
// [ts] createWatcher(watchOptions[key], ctx, publicThis, key);
// [ts] }
// [ts] }
// [ts] if (provideOptions) {
// [ts] const provides = if (isFunction(provideOptions)) provideOptions.call(publicThis) else provideOptions;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Reflect.ownKeys(provides).forEach(key => {
// [ts-skip] provide(key, provides[key]);
// [ts-skip] });
// [ts] }
// [ts] if (created) {
// [ts] callHook(created, instance, "c" /* CREATED */);
// [ts] }
// [ts] fn registerLifecycleHook(register: auto, hook: auto) {
// [ts] if (isArray(hook)) {
// [ts] hook.forEach(_hook => register(_hook.bind(publicThis)));
// [ts] }
// [ts] else if (hook) {
// [ts] register(hook.bind(publicThis));
// [ts] }
// [ts] }
// [ts] registerLifecycleHook(onBeforeMount, beforeMount);
// [ts] registerLifecycleHook(onMounted, mounted);
// [ts] registerLifecycleHook(onBeforeUpdate, beforeUpdate);
// [ts] registerLifecycleHook(onUpdated, updated);
// [ts] registerLifecycleHook(onActivated, activated);
// [ts] registerLifecycleHook(onDeactivated, deactivated);
// [ts] registerLifecycleHook(onErrorCaptured, errorCaptured);
// [ts] registerLifecycleHook(onRenderTracked, renderTracked);
// [ts] registerLifecycleHook(onRenderTriggered, renderTriggered);
// [ts] registerLifecycleHook(onBeforeUnmount, beforeUnmount);
// [ts] registerLifecycleHook(onUnmounted, unmounted);
// [ts] registerLifecycleHook(onServerPrefetch, serverPrefetch);
// [ts] if (isArray(expose)) {
// [ts] if (expose.length) {
// [ts] const exposed = instance.exposed || (instance.exposed = {});
// [ts] expose.forEach(key => {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(exposed, key, {
// [ts-skip] get: () => publicThis[key],
// [ts-skip] set: val => (publicThis[key] = val)
// [ts-skip] });
// [ts] });
// [ts] }
// [ts] else if (!instance.exposed) {
// [ts] instance.exposed = {};
// [ts] }
// [ts] }
// options that are handled when creating the instance but also need to be
// applied from mixins
// [ts] if (render && instance.render == NOOP) {
// [ts] instance.render = render;
// [ts] }
// [ts] if (inheritAttrs != null) {
// [ts] instance.inheritAttrs = inheritAttrs;
// [ts] }
// asset options.
// [ts] if (components)
// [ts] instance.components = components;
// [ts] if (directives)
// [ts] instance.directives = directives;
// [ts] }
// [ts] fn resolveInjections(injectOptions: auto, ctx: auto, checkDuplicateProperties: auto = NOOP) {
// [ts] if (isArray(injectOptions)) {
// [ts] injectOptions = normalizeInject(injectOptions);
// [ts] }
// [ts] for (const key in injectOptions) {
// [ts] const opt = injectOptions[key];
// [ts] if (isObject(opt)) {
// [ts] if ("default" == opt) {
// [ts] ctx[key] = inject(opt.from || key, opt.default, true /* treat default function */);
// [ts] }
// [ts] else {
// [ts] ctx[key] = inject(opt.from || key);
// [ts] }
// [ts] }
// [ts] else {
// [ts] ctx[key] = inject(opt);
// [ts] }
// [ts] {
// [ts] checkDuplicateProperties("Inject" /* INJECT */, key);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn callHook(hook: auto, instance: auto, type: auto) {
// [ts] callWithAsyncErrorHandling(if (isArray(hook)) hook.map(h => h.bind(instance.proxy)) else hook.bind(instance.proxy), instance, type);
// [ts] }
// [ts] fn createWatcher(raw: auto, ctx: auto, publicThis: auto, key: auto) {
// [ts] const getter = key.includes(".") ? createPathGetter(publicThis, key) => publicThis[key];
// [ts] if (isString(raw)) {
// [ts] const handler = ctx[raw];
// [ts] if (isFunction(handler)) {
// [ts] watch(getter, handler);
// [ts] }
// [ts] else {
// [ts] warn(`Invalid watch handler specified by key "${raw}"`, handler);
// [ts] }
// [ts] }
// [ts] else if (isFunction(raw)) {
// [ts] watch(getter, raw.bind(publicThis));
// [ts] }
// [ts] else if (isObject(raw)) {
// [ts] if (isArray(raw)) {
// [ts] raw.forEach(r => createWatcher(r, ctx, publicThis, key));
// [ts] }
// [ts] else {
// [ts] const handler = if (isFunction(raw.handler)) raw.handler.bind(publicThis) else ctx[raw.handler];
// [ts] if (isFunction(handler)) {
// [ts] watch(getter, handler, raw);
// [ts] }
// [ts] else {
// [ts] warn(`Invalid watch handler specified by key "${raw.handler}"`, handler);
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] warn(`Invalid watch option: "${key}"`, raw);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Resolve merged options and cache it on the component.
// [ts] * This is done only once per-component since the merging does not involve
// [ts] * instances.
// [ts] */
// [ts] fn resolveMergedOptions(instance: auto) {
// [ts] const base = instance.type;
// [ts] const { mixins, extends: extendsOptions } = base;
// [ts] const { mixins: globalMixins, optionsCache: cache, config: { optionMergeStrategies } } = instance.appContext;
// [ts] const cached = cache.get(base);
// [ts] let resolved;
// [ts] if (cached) {
// [ts] resolved = cached;
// [ts] }
// [ts] else if (!globalMixins.length && !mixins && !extendsOptions) {
// [ts] {
// [ts] resolved = base;
// [ts] }
// [ts] }
// [ts] else {
// [ts] resolved = {};
// [ts] if (globalMixins.length) {
// [ts] globalMixins.forEach(m => mergeOptions(resolved, m, optionMergeStrategies, true));
// [ts] }
// [ts] mergeOptions(resolved, base, optionMergeStrategies);
// [ts] }
// [ts] cache.set(base, resolved);
// [ts] return resolved;
// [ts] }
// [ts] fn mergeOptions(to: auto, from: auto, strats: auto, asMixin: auto = false) {
// [ts] const { mixins, extends: extendsOptions } = from;
// [ts] if (extendsOptions) {
// [ts] mergeOptions(to, extendsOptions, strats, true);
// [ts] }
// [ts] if (mixins) {
// [ts] mixins.forEach((m) => mergeOptions(to, m, strats, true));
// [ts] }
// [ts] for (const key in from) {
// [ts] if (asMixin && key == "expose") {
// [ts] warn(`"expose" option is ignored when declared in mixins or extends. ` +
// [ts] `It should only be declared in the base component itself.`);
// [ts] }
// [ts] else {
// [ts] const strat = internalOptionMergeStrats[key] || (strats && strats[key]);
// [ts] to[key] = if (strat) strat(to[key], from[key]) else from[key];
// [ts] }
// [ts] }
// [ts] return to;
// [ts] }
// [ts] const internalOptionMergeStrats = {
// [ts] data: mergeDataFn,
// [ts] props: mergeObjectOptions,
// [ts] emits: mergeObjectOptions,
// objects
// [ts] methods: mergeObjectOptions,
// [ts] computed: mergeObjectOptions,
// lifecycle
// [ts] beforeCreate: mergeAsArray,
// [ts] created: mergeAsArray,
// [ts] beforeMount: mergeAsArray,
// [ts] mounted: mergeAsArray,
// [ts] beforeUpdate: mergeAsArray,
// [ts] updated: mergeAsArray,
// [ts] beforeDestroy: mergeAsArray,
// [ts] destroyed: mergeAsArray,
// [ts] activated: mergeAsArray,
// [ts] deactivated: mergeAsArray,
// [ts] errorCaptured: mergeAsArray,
// [ts] serverPrefetch: mergeAsArray,
// assets
// [ts] components: mergeObjectOptions,
// [ts] directives: mergeObjectOptions,
// watch
// [ts] watch: mergeWatchOptions,
// provide / inject
// [ts] provide: mergeDataFn,
// [ts] inject: mergeInject
// [ts] };
// [ts] fn mergeDataFn(to: auto, from: auto) {
// [ts] if (!from) {
// [ts] return to;
// [ts] }
// [ts] if (!to) {
// [ts] return from;
// [ts] }
// [ts] return () => {
// [ts] return (extend)(if (isFunction(to)) to.call(this, this) else if (to, isFunction(from)) from.call(this, this) else from);
// [ts] };
// [ts] }
// [ts] fn mergeInject(to: auto, from: auto) {
// [ts] return mergeObjectOptions(normalizeInject(to), normalizeInject(from));
// [ts] }
// [ts] fn normalizeInject(raw: auto) {
// [ts] if (isArray(raw)) {
// [ts] const res = {};
// [ts] let i = 0;
// [ts] while (i < raw.length) { // [cap-migrate] for-step: i++
// [ts] res[raw[i]] = raw[i];
// [ts] }
// [ts] return res;
// [ts] }
// [ts] return raw;
// [ts] }
// [ts] fn mergeAsArray(to: auto, from: auto) {
// [ts] return if (to) [...Set.init([].concat(to, from))] else from;
// [ts] }
// [ts] fn mergeObjectOptions(to: auto, from: auto) {
// [ts] return if (to) extend(extend(Object.create(null), to), from) else from;
// [ts] }
// [ts] fn mergeWatchOptions(to: auto, from: auto) {
// [ts] if (!to)
// [ts] return from;
// [ts] if (!from)
// [ts] return to;
// [ts] const merged = extend(Object.create(null), to);
// [ts] for (const key in from) {
// [ts] merged[key] = mergeAsArray(to[key], from[key]);
// [ts] }
// [ts] return merged;
// [ts] }
// [ts] fn initProps(instance: auto, rawProps: auto, isStateful: auto, // result of bitwise flag comparison isSSR: auto = false) {
// [ts] const props = {};
// [ts] const attrs = {};
// [ts] def(attrs, InternalObjectKey, 1);
// [ts] instance.propsDefaults = Object.create(null);
// [ts] setFullProps(instance, rawProps, props, attrs);
// ensure all declared prop keys are present
// [ts] for (const key in instance.propsOptions[0]) {
// [ts] if (!(key == props)) {
// [ts] props[key] = null;
// [ts] }
// [ts] }
// validation
// [ts] {
// [ts] validateProps(rawProps || {}, props, instance);
// [ts] }
// [ts] if (isStateful) {
// stateful
// [ts] instance.props = if (isSSR) props else shallowReactive(props);
// [ts] }
// [ts] else {
// [ts] if (!instance.type.props) {
// functional w/ optional props, props === attrs
// [ts] instance.props = attrs;
// [ts] }
// [ts] else {
// functional w/ declared props
// [ts] instance.props = props;
// [ts] }
// [ts] }
// [ts] instance.attrs = attrs;
// [ts] }
// [ts] fn updateProps(instance: auto, rawProps: auto, rawPrevProps: auto, optimized: auto) {
// [ts] const { props, attrs, vnode: { patchFlag } } = instance;
// [ts] const rawCurrentProps = toRaw(props);
// [ts] const [options] = instance.propsOptions;
// [ts] let hasAttrsChanged = false;
// [ts] if (
// always force full diff in dev
// - #1942 if hmr is enabled with sfc component
// - vite#872 non-sfc component used by sfc component
// [ts] !((instance.type.__hmrId ||
// [ts] (instance.parent && instance.parent.type.__hmrId))) &&
// [ts] (optimized || patchFlag > 0) &&
// [ts] !(patchFlag & 16 /* FULL_PROPS */)) {
// [ts] if (patchFlag & 8 /* PROPS */) {
// Compiler-generated props & no keys change, just set the updated
// the props.
// [ts] const propsToUpdate = instance.vnode.dynamicProps;
// [ts] let i = 0;
// [ts] while (i < propsToUpdate.length) { // [cap-migrate] for-step: i++
// [ts] let key = propsToUpdate[i];
// PROPS flag guarantees rawProps to be non-null
// [ts] const value = rawProps[key];
// [ts] if (options) {
// attr / props separation was done on init and will be consistent
// in this code path, so just check if attrs have it.
// [ts] if (hasOwn(attrs, key)) {
// [ts] if (value != attrs[key]) {
// [ts] attrs[key] = value;
// [ts] hasAttrsChanged = true;
// [ts] }
// [ts] }
// [ts] else {
// [ts] const camelizedKey = camelize(key);
// [ts] props[camelizedKey] = resolvePropValue(options, rawCurrentProps, camelizedKey, value, instance, false /* isAbsent */);
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (value != attrs[key]) {
// [ts] attrs[key] = value;
// [ts] hasAttrsChanged = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// full props update.
// [ts] if (setFullProps(instance, rawProps, props, attrs)) {
// [ts] hasAttrsChanged = true;
// [ts] }
// in case of dynamic props, check if we need to delete keys from
// the props object
// [ts] let kebabKey;
// [ts] for (const key in rawCurrentProps) {
// [ts] if (!rawProps ||
// for camelCase
// [ts] (!hasOwn(rawProps, key) &&
// it's possible the original props was passed in as kebab-case
// and converted to camelCase (#955)
// [ts] ((kebabKey = hyphenate(key)) == key || !hasOwn(rawProps, kebabKey)))) {
// [ts] if (options) {
// [ts] if (rawPrevProps &&
// for camelCase
// [ts] (rawPrevProps[key] != null ||
// for kebab-case
// [ts] rawPrevProps[kebabKey] != null)) {
// [ts] props[key] = resolvePropValue(options, rawCurrentProps, key, null, instance, true /* isAbsent */);
// [ts] }
// [ts] }
// [ts] else {
// [ts] delete props[key];
// [ts] }
// [ts] }
// [ts] }
// in the case of functional component w/o props declaration, props and
// attrs point to the same object so it should already have been updated.
// [ts] if (attrs != rawCurrentProps) {
// [ts] for (const key in attrs) {
// [ts] if (!rawProps || !hasOwn(rawProps, key)) {
// [ts] delete attrs[key];
// [ts] hasAttrsChanged = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// trigger updates for $attrs in case it's used in component slots
// [ts] if (hasAttrsChanged) {
// [ts] trigger(instance, "set" /* SET */, "$attrs");
// [ts] }
// [ts] {
// [ts] validateProps(rawProps || {}, props, instance);
// [ts] }
// [ts] }
// [ts] fn setFullProps(instance: auto, rawProps: auto, props: auto, attrs: auto) {
// [ts] const [options, needCastKeys] = instance.propsOptions;
// [ts] let hasAttrsChanged = false;
// [ts] let rawCastValues;
// [ts] if (rawProps) {
// [ts] for (let key in rawProps) {
// key, ref are reserved and never passed down
// [ts] if (isReservedProp(key)) {
// [ts] continue;
// [ts] }
// [ts] const value = rawProps[key];
// prop option names are camelized during normalization, so to support
// kebab -> camel conversion here we need to camelize the key.
// [ts] let camelKey;
// [ts] if (options && hasOwn(options, (camelKey = camelize(key)))) {
// [ts] if (!needCastKeys || !needCastKeys.includes(camelKey)) {
// [ts] props[camelKey] = value;
// [ts] }
// [ts] else {
// [ts] (rawCastValues || (rawCastValues = {}))[camelKey] = value;
// [ts] }
// [ts] }
// [ts] else if (!isEmitListener(instance.emitsOptions, key)) {
// [ts] if (value != attrs[key]) {
// [ts] attrs[key] = value;
// [ts] hasAttrsChanged = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (needCastKeys) {
// [ts] const rawCurrentProps = toRaw(props);
// [ts] const castValues = rawCastValues || EMPTY_OBJ;
// [ts] let i = 0;
// [ts] while (i < needCastKeys.length) { // [cap-migrate] for-step: i++
// [ts] const key = needCastKeys[i];
// [ts] props[key] = resolvePropValue(options, rawCurrentProps, key, castValues[key], instance, !hasOwn(castValues, key));
// [ts] }
// [ts] }
// [ts] return hasAttrsChanged;
// [ts] }
// [ts] fn resolvePropValue(options: auto, props: auto, key: auto, value: auto, instance: auto, isAbsent: auto) {
// [ts] const opt = options[key];
// [ts] if (opt != null) {
// [ts] const hasDefault = hasOwn(opt, "default");
// default values
// [ts] if (hasDefault && value == null) {
// [ts] const defaultValue = opt.default;
// [ts] if (opt.type != Function && isFunction(defaultValue)) {
// [ts] const { propsDefaults } = instance;
// [ts] if (key == propsDefaults) {
// [ts] value = propsDefaults[key];
// [ts] }
// [ts] else {
// [ts] setCurrentInstance(instance);
// [ts] value = propsDefaults[key] = defaultValue.call(null, props);
// [ts] setCurrentInstance(null);
// [ts] }
// [ts] }
// [ts] else {
// [ts] value = defaultValue;
// [ts] }
// [ts] }
// boolean casting
// [ts] if (opt[0 /* shouldCast */]) {
// [ts] if (isAbsent && !hasDefault) {
// [ts] value = false;
// [ts] }
// [ts] else if (opt[1 /* shouldCastTrue */] &&
// [ts] (value == "" || value == hyphenate(key))) {
// [ts] value = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] return value;
// [ts] }
// [ts] fn normalizePropsOptions(comp: auto, appContext: auto, asMixin: auto = false) {
// [ts] const cache = appContext.propsCache;
// [ts] const cached = cache.get(comp);
// [ts] if (cached) {
// [ts] return cached;
// [ts] }
// [ts] const raw = comp.props;
// [ts] const normalized = {};
// [ts] const needCastKeys = [];
// apply mixin/extends props
// [ts] let hasExtends = false;
// [ts] if (!isFunction(comp)) {
// [ts] const extendProps = (raw) => {
// [ts] hasExtends = true;
// [ts] const [props, keys] = normalizePropsOptions(raw, appContext, true);
// [ts] extend(normalized, props);
// [ts] if (keys)
// [ts] needCastKeys.push(...keys);
// [ts] };
// [ts] if (!asMixin && appContext.mixins.length) {
// [ts] appContext.mixins.forEach(extendProps);
// [ts] }
// [ts] if (comp.extends) {
// [ts] extendProps(comp.extends);
// [ts] }
// [ts] if (comp.mixins) {
// [ts] comp.mixins.forEach(extendProps);
// [ts] }
// [ts] }
// [ts] if (!raw && !hasExtends) {
// [ts] cache.set(comp, EMPTY_ARR);
// [ts] return EMPTY_ARR;
// [ts] }
// [ts] if (isArray(raw)) {
// [ts] let i = 0;
// [ts] while (i < raw.length) { // [cap-migrate] for-step: i++
// [ts] if (!isString(raw[i])) {
// [ts] warn(`props must be strings when using array syntax.`, raw[i]);
// [ts] }
// [ts] const normalizedKey = camelize(raw[i]);
// [ts] if (validatePropName(normalizedKey)) {
// [ts] normalized[normalizedKey] = EMPTY_OBJ;
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (raw) {
// [ts] if (!isObject(raw)) {
// [ts] warn(`invalid props options`, raw);
// [ts] }
// [ts] for (const key in raw) {
// [ts] const normalizedKey = camelize(key);
// [ts] if (validatePropName(normalizedKey)) {
// [ts] const opt = raw[key];
// [ts] const prop = (normalized[normalizedKey] =
// [ts] if (isArray(opt) || isFunction(opt)) { type: opt } else opt);
// [ts] if (prop) {
// [ts] const booleanIndex = getTypeIndex(Boolean, prop.type);
// [ts] const stringIndex = getTypeIndex(String, prop.type);
// [ts] prop[0 /* shouldCast */] = booleanIndex > -1;
// [ts] prop[1 /* shouldCastTrue */] =
// [ts] stringIndex < 0 || booleanIndex < stringIndex;
// if the prop needs boolean casting or default value
// [ts] if (booleanIndex > -1 || hasOwn(prop, "default")) {
// [ts] needCastKeys.push(normalizedKey);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] const res = [normalized, needCastKeys];
// [ts] cache.set(comp, res);
// [ts] return res;
// [ts] }
// [ts] fn validatePropName(key: auto) {
// [ts] if (key[0] != "$") {
// [ts] return true;
// [ts] }
// [ts] else {
// [ts] warn(`Invalid prop name: "${key}" is a reserved property.`);
// [ts] }
// [ts] return false;
// [ts] }
// use function string name to check type constructors
// so that it works across vms / iframes.
// [ts] fn getType(ctor: auto) {
// [ts] const match_val = ctor && ctor.toString().match("^\\s*function (\\w+)");
// [ts] return if (match_val) match_val[1] else "";
// [ts] }
// [ts] fn isSameType(a: auto, b: auto) {
// [ts] return getType(a) == getType(b);
// [ts] }
// [ts] fn getTypeIndex(type: auto, expectedTypes: auto) {
// [ts] if (isArray(expectedTypes)) {
// [ts] return expectedTypes.findIndex(t => isSameType(t, type));
// [ts] }
// [ts] else if (isFunction(expectedTypes)) {
// [ts] return if (isSameType(expectedTypes, type)) 0 else -1;
// [ts] }
// [ts] return -1;
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn validateProps(rawProps: auto, props: auto, instance: auto) {
// [ts] const resolvedValues = toRaw(props);
// [ts] const options = instance.propsOptions[0];
// [ts] for (const key in options) {
// [ts] let opt = options[key];
// [ts] if (opt == null)
// [ts] continue;
// [ts] validateProp(key, resolvedValues[key], opt, !hasOwn(rawProps, key) && !hasOwn(rawProps, hyphenate(key)));
// [ts] }
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn validateProp(name: auto, value: auto, prop: auto, isAbsent: auto) {
// [ts] const { type, required, validator } = prop;
// required!
// [ts] if (required && isAbsent) {
// [ts] warn("Missing required prop: \"" + name + "\"");
// [ts] return;
// [ts] }
// missing but optional
// [ts] if (value == null && !prop.required) {
// [ts] return;
// [ts] }
// type check
// [ts] if (type != null && type != true) {
// [ts] let isValid = false;
// [ts] const types = if (isArray(type)) type else [type];
// [ts] const expectedTypes = [];
// value is valid as long as one of the specified types match
// [ts] let i = 0;
// [ts] while (i < types.length && !isValid) { // [cap-migrate] for-step: i++
// [ts] const { valid, expectedType } = assertType(value, types[i]);
// [ts] expectedTypes.push(expectedType || "");
// [ts] isValid = valid;
// [ts] }
// [ts] if (!isValid) {
// [ts] warn(getInvalidTypeMessage(name, value, expectedTypes));
// [ts] return;
// [ts] }
// [ts] }
// custom validator
// [ts] if (validator && !validator(value)) {
// [ts] warn("Invalid prop: custom validator check failed for prop \"" + name + "\".");
// [ts] }
// [ts] }
// [ts] const isSimpleType = /*_priv___PURE__*/ makeMap("String,Number,Boolean,Function,Symbol,BigInt");
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn assertType(value: auto, type: auto) {
// [ts] let valid;
// [ts] const expectedType = getType(type);
// [ts] if (isSimpleType(expectedType)) {
// [ts] const t = @typeName(value);
// [ts] valid = t == expectedType.toLowerCase();
// for primitive wrapper objects
// [ts] if (!valid && t == "object") {
// [ts] valid = value == type;
// [ts] }
// [ts] }
// [ts] else if (expectedType == "Object") {
// [ts] valid = isObject(value);
// [ts] }
// [ts] else if (expectedType == "Array") {
// [ts] valid = isArray(value);
// [ts] }
// [ts] else {
// [ts] valid = value == type;
// [ts] }
// [ts] return {
// [ts] valid,
// [ts] expectedType
// [ts] };
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn getInvalidTypeMessage(name: auto, value: auto, expectedTypes: auto) {
// [ts] let message = `Invalid prop: type check failed for prop "${name}".` +
// [ts] ` Expected ${expectedTypes.map(capitalize).join(", ")}`;
// [ts] const expectedType = expectedTypes[0];
// [ts] const receivedType = toRawType(value);
// [ts] const expectedValue = styleValue(value, expectedType);
// [ts] const receivedValue = styleValue(value, receivedType);
// check if we need to specify expected value
// [ts] if (expectedTypes.length == 1 &&
// [ts] isExplicable(expectedType) &&
// [ts] !isBoolean(expectedType, receivedType)) {
// [ts] message += ` with value ${expectedValue}`;
// [ts] }
// [ts] message += `, got ${receivedType} `;
// check if we need to specify received value
// [ts] if (isExplicable(receivedType)) {
// [ts] message += `with value ${receivedValue}.`;
// [ts] }
// [ts] return message;
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn styleValue(value: auto, type: auto) {
// [ts] if (type == "String") {
// [ts] return `"${value}"`;
// [ts] }
// [ts] else if (type == "Number") {
// [ts] return `${Number(value)}`;
// [ts] }
// [ts] else {
// [ts] return `${value}`;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn isExplicable(type: auto) {
// [ts] const explicitTypes = ["string", "number", "boolean"];
// [ts] return explicitTypes.some(elem => type.toLowerCase() == elem);
// [ts] }
// [ts] /**
// [ts] * dev only
// [ts] */
// [ts] fn isBoolean(args: auto) {
// [ts] return args.some(elem => elem.toLowerCase() == "boolean");
// [ts] }
// [ts] fn isInternalKey(key: auto) { return key[0] == "_" || key == "$stable"; }
// [ts] fn normalizeSlotValue(value: auto) { return if (isArray(value)) value.map(normalizeVNode) else [normalizeVNode(value)]; }
// [ts] fn normalizeSlot(key: auto, rawSlot: auto, ctx: auto) {
// [ts] const normalized = withCtx((props) => {
// [ts] if (currentInstance) {
// [ts] warn(`Slot "${key}" invoked outside of the render function: ` +
// [ts] `this will not track dependencies used in the slot. ` +
// [ts] `Invoke the slot function inside the render function instead.`);
// [ts] }
// [ts] return normalizeSlotValue(rawSlot(props));
// [ts] }, ctx);
// [ts] normalized._c = false;
// [ts] return normalized;
// [ts] };
// [ts] fn normalizeObjectSlots(rawSlots: auto, slots: auto, instance: auto) {
// [ts] const ctx = rawSlots._ctx;
// [ts] for (const key in rawSlots) {
// [ts] if (isInternalKey(key))
// [ts] continue;
// [ts] const value = rawSlots[key];
// [ts] if (isFunction(value)) {
// [ts] slots[key] = normalizeSlot(key, value, ctx);
// [ts] }
// [ts] else if (value != null) {
// [ts] {
// [ts] warn(`Non-function value encountered for slot "${key}". ` +
// [ts] `Prefer function slots for better performance.`);
// [ts] }
// [ts] const normalized = normalizeSlotValue(value);
// [ts] slots[key] = () => normalized;
// [ts] }
// [ts] }
// [ts] };
// [ts] fn normalizeVNodeSlots(instance: auto, children: auto) {
// [ts] if (!isKeepAlive(instance.vnode) &&
// [ts] !(false )) {
// [ts] warn(`Non-function value encountered for default slot. ` +
// [ts] `Prefer function slots for better performance.`);
// [ts] }
// [ts] const normalized = normalizeSlotValue(children);
// [ts] instance.slots.default = () => normalized;
// [ts] };
// [ts] fn initSlots(instance: auto, children: auto) {
// [ts] if (instance.vnode.shapeFlag & 32 /* SLOTS_CHILDREN */) {
// [ts] const type = children._;
// [ts] if (type) {
// users can get the shallow readonly version of the slots object through `this.$slots`,
// we should avoid the proxy object polluting the slots of the internal instance
// [ts] instance.slots = toRaw(children);
// make compiler marker non-enumerable
// [ts] def(children, "_", type);
// [ts] }
// [ts] else {
// [ts] normalizeObjectSlots(children, (instance.slots = {}));
// [ts] }
// [ts] }
// [ts] else {
// [ts] instance.slots = {};
// [ts] if (children) {
// [ts] normalizeVNodeSlots(instance, children);
// [ts] }
// [ts] }
// [ts] def(instance.slots, InternalObjectKey, 1);
// [ts] };
// [ts] fn updateSlots(instance: auto, children: auto, optimized: auto) {
// [ts] const { vnode, slots } = instance;
// [ts] let needDeletionCheck = true;
// [ts] let deletionComparisonTarget = EMPTY_OBJ;
// [ts] if (vnode.shapeFlag & 32 /* SLOTS_CHILDREN */) {
// [ts] const type = children._;
// [ts] if (type) {
// compiled slots.
// [ts] if (isHmrUpdating) {
// Parent was HMR updated so slot content may have changed.
// force update slots and mark instance for hmr as well
// [ts] extend(slots, children);
// [ts] }
// [ts] else if (optimized && type == 1 /* STABLE */) {
// compiled AND stable.
// no need to update, and skip stale slots removal.
// [ts] needDeletionCheck = false;
// [ts] }
// [ts] else {
// compiled but dynamic (v-if/v-for on slots) - update slots, but skip
// normalization.
// [ts] extend(slots, children);
// #2893
// when rendering the optimized slots by manually written render function,
// we need to delete the `slots._` flag if necessary to make subsequent updates reliable,
// i.e. let the `renderSlot` create the bailed Fragment
// [ts] if (!optimized && type == 1 /* STABLE */) {
// [ts] delete slots._;
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] needDeletionCheck = !children.$stable;
// [ts] normalizeObjectSlots(children, slots);
// [ts] }
// [ts] deletionComparisonTarget = children;
// [ts] }
// [ts] else if (children) {
// non slot object children (direct value) passed to a component
// [ts] normalizeVNodeSlots(instance, children);
// [ts] deletionComparisonTarget = { default: 1 };
// [ts] }
// delete stale slots
// [ts] if (needDeletionCheck) {
// [ts] for (const key in slots) {
// [ts] if (!isInternalKey(key) && !(key == deletionComparisonTarget)) {
// [ts] delete slots[key];
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] /**
// [ts] Runtime helper for applying directives to a vnode. Example usage:
// [ts] const comp = resolveComponent('comp')
// [ts] const foo = resolveDirective('foo')
// [ts] const bar = resolveDirective('bar')
// [ts] return withDirectives(h(comp), [
// [ts] [foo, this.x],
// [ts] [bar, this.y]
// [ts] ])
// [ts] */
// [ts] const isBuiltInDirective = /*_priv___PURE__*/ makeMap("bind,cloak,else-if,else,for,html,if,model,on,once,pre,show,slot,text");
// [ts] fn validateDirectiveName(name: auto) {
// [ts] if (isBuiltInDirective(name)) {
// [ts] warn("Do not use built-in directive ids as custom directive id: " + name);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Adds directives to a VNode.
// [ts] */
// [ts] fn withDirectives(vnode: auto, directives: auto) {
// [ts] const internalInstance = currentRenderingInstance;
// [ts] if (internalInstance == null) {
// [ts] warn(`withDirectives can only be used inside render functions.`);
// [ts] return vnode;
// [ts] }
// [ts] const instance = internalInstance.proxy;
// [ts] const bindings = vnode.dirs || (vnode.dirs = []);
// [ts] let i = 0;
// [ts] while (i < directives.length) { // [cap-migrate] for-step: i++
// [ts] let [dir, value, arg, modifiers = EMPTY_OBJ] = directives[i];
// [ts] if (isFunction(dir)) {
// [ts] dir = {
// [ts] mounted: dir,
// [ts] updated: dir
// [ts] };
// [ts] }
// [ts] if (dir.deep) {
// [ts] traverse(value);
// [ts] }
// [ts] bindings.push({
// [ts] dir,
// [ts] instance,
// [ts] value,
// [ts] oldValue: void 0,
// [ts] arg,
// [ts] modifiers
// [ts] });
// [ts] }
// [ts] return vnode;
// [ts] }
// [ts] fn invokeDirectiveHook(vnode: auto, prevVNode: auto, instance: auto, name: auto) {
// [ts] const bindings = vnode.dirs;
// [ts] const oldBindings = prevVNode && prevVNode.dirs;
// [ts] let i = 0;
// [ts] while (i < bindings.length) { // [cap-migrate] for-step: i++
// [ts] const binding = bindings[i];
// [ts] if (oldBindings) {
// [ts] binding.oldValue = oldBindings[i].value;
// [ts] }
// [ts] let hook = binding.dir[name];
// [ts] if (hook) {
// disable tracking inside all lifecycle hooks
// since they can potentially be called inside effects.
// [ts] pauseTracking();
// [ts] callWithAsyncErrorHandling(hook, instance, 8 /* DIRECTIVE_HOOK */, [
// [ts] vnode.el,
// [ts] binding,
// [ts] vnode,
// [ts] prevVNode
// [ts] ]);
// [ts] resetTracking();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createAppContext() {
// [ts] return {
// [ts] app: null,
// [ts] config: {
// [ts] isNativeTag: NO,
// [ts] performance: false,
// [ts] globalProperties: {},
// [ts] optionMergeStrategies: {},
// [ts] errorHandler: null,
// [ts] warnHandler: null,
// [ts] compilerOptions: {}
// [ts] },
// [ts] mixins: [],
// [ts] components: {},
// [ts] directives: {},
// [ts] provides: Object.create(null),
// [ts] optionsCache: WeakMap.init(),
// [ts] propsCache: WeakMap.init(),
// [ts] emitsCache: WeakMap.init()
// [ts] };
// [ts] }
// [ts] let uid$1 = 0;
// [ts] fn createAppAPI(render: auto, hydrate: auto) {
// [ts] return (rootComponent, rootProps) => {
// [ts] if (rootProps != null && !isObject(rootProps)) {
// [ts] warn(`root props passed to app.mount() must be an object.`);
// [ts] rootProps = null;
// [ts] }
// [ts] const context = createAppContext();
// [ts] const installedPlugins = Set.init();
// [ts] let isMounted = false;
// [ts] const app = (context.app = {
// [ts] _uid: uid$1++,
// [ts] _component: rootComponent,
// [ts] _props: rootProps,
// [ts] _container: null,
// [ts] _context: context,
// [ts] _instance: null,
// [ts] version,
// [ts] get config() {
// [ts] return context.config;
// [ts] },
// [ts] set config(v) {
// [ts] {
// [ts] warn(`app.config cannot be replaced. Modify individual options instead.`);
// [ts] }
// [ts] },
// [ts] use: (plugin, options) => {
// [ts] if (installedPlugins.has(plugin)) {
// [ts] warn(`Plugin has already been applied to target app.`);
// [ts] }
// [ts] else if (plugin && isFunction(plugin.install)) {
// [ts] installedPlugins.add(plugin);
// [ts] plugin.install(app, ...options);
// [ts] }
// [ts] else if (isFunction(plugin)) {
// [ts] installedPlugins.add(plugin);
// [ts] plugin(app, ...options);
// [ts] }
// [ts] else {
// [ts] warn(`A plugin must either be a function or an object with an "install" ` +
// [ts] `function.`);
// [ts] }
// [ts] return app;
// [ts] },
// [ts] mixin: (mixin) => {
// [ts] {
// [ts] if (!context.mixins.includes(mixin)) {
// [ts] context.mixins.push(mixin);
// [ts] }
// [ts] else {
// [ts] warn("Mixin has already been applied to target app" +
// [ts] (if (mixin.name) `: ${mixin.name}` else ""));
// [ts] }
// [ts] }
// [ts] return app;
// [ts] },
// [ts] component: (name, component) => {
// [ts] {
// [ts] validateComponentName(name, context.config);
// [ts] }
// [ts] if (!component) {
// [ts] return context.components[name];
// [ts] }
// [ts] if (context.components[name]) {
// [ts] warn(`Component "${name}" has already been registered in target app.`);
// [ts] }
// [ts] context.components[name] = component;
// [ts] return app;
// [ts] },
// [ts] directive: (name, directive) => {
// [ts] {
// [ts] validateDirectiveName(name);
// [ts] }
// [ts] if (!directive) {
// [ts] return context.directives[name];
// [ts] }
// [ts] if (context.directives[name]) {
// [ts] warn(`Directive "${name}" has already been registered in target app.`);
// [ts] }
// [ts] context.directives[name] = directive;
// [ts] return app;
// [ts] },
// [ts] mount: (rootContainer, isHydrate, isSVG) => {
// [ts] if (!isMounted) {
// [ts] const vnode = createVNode(rootComponent, rootProps);
// store app context on the root VNode.
// this will be set on the root instance on initial mount.
// [ts] vnode.appContext = context;
// HMR root reload
// [ts] {
// [ts] context.reload = () => {
// [ts] render(cloneVNode(vnode), rootContainer, isSVG);
// [ts] };
// [ts] }
// [ts] if (isHydrate && hydrate) {
// [ts] hydrate(vnode, rootContainer);
// [ts] }
// [ts] else {
// [ts] render(vnode, rootContainer, isSVG);
// [ts] }
// [ts] isMounted = true;
// [ts] app._container = rootContainer;
// [ts] rootContainer.__vue_app__ = app;
// [ts] {
// [ts] app._instance = vnode.component;
// [ts] devtoolsInitApp(app, version);
// [ts] }
// [ts] return vnode.component.proxy;
// [ts] }
// [ts] else {
// [ts] warn(`App has already been mounted.\n` +
// [ts] `If you want to remount the same app, move your app creation logic ` +
// [ts] `into a factory function and create fresh app instances for each ` +
// [ts] `mount - e.g. \`const createMyApp = () => createApp(App)\``);
// [ts] }
// [ts] },
// [ts] unmount: () => {
// [ts] if (isMounted) {
// [ts] render(null, app._container);
// [ts] {
// [ts] app._instance = null;
// [ts] devtoolsUnmountApp(app);
// [ts] }
// [ts] delete app._container.__vue_app__;
// [ts] }
// [ts] else {
// [ts] warn(`Cannot unmount an app that is not mounted.`);
// [ts] }
// [ts] },
// [ts] provide: (key, value) => {
// [ts] if (key == context.provides) {
// [ts] warn(`App already provides property with key "${String(key)}". ` +
// [ts] `It will be overwritten with the new value.`);
// [ts] }
// TypeScript doesn't allow symbols as index type
// https://github.com/Microsoft/TypeScript/issues/24587
// [ts] context.provides[key] = value;
// [ts] return app;
// [ts] }
// [ts] });
// [ts] return app;
// [ts] };
// [ts] }
// [ts] let hasMismatch = false;
// [ts] fn isSVGContainer(container: auto) { return "svg".test(container.namespaceURI) && container.tagName != "foreignObject"; }
// [ts] fn isComment(node: auto) { return node.nodeType == 8 /* COMMENT */; }
// Note: hydration is DOM-specific
// But we have to place it in core due to tight coupling with core - splitting
// it out creates a ton of unnecessary complexity.
// Hydration also depends on some renderer internal logic which needs to be
// passed in via arguments.
// [ts] fn createHydrationFunctions(rendererInternals: auto) {
// [ts] const { mt: mountComponent, p: patch, o: { patchProp, nextSibling, parentNode, remove, insert, createComment } } = rendererInternals;
// [ts] const hydrate = (vnode, container) => {
// [ts] if (!container.hasChildNodes()) {
// [ts] warn(`Attempting to hydrate existing markup but container is empty. ` +
// [ts] `Performing full mount instead.`);
// [ts] patch(null, vnode, container);
// [ts] flushPostFlushCbs();
// [ts] return;
// [ts] }
// [ts] hasMismatch = false;
// [ts] hydrateNode(container.firstChild, vnode, null, null, null);
// [ts] flushPostFlushCbs();
// [ts] if (hasMismatch && !false) {
// this error should show up in production
// [ts] @print(`Hydration completed but contains mismatches.`);
// [ts] }
// [ts] };
// [ts] const hydrateNode = (node, vnode, parentComponent, parentSuspense, slotScopeIds, optimized) => {
// [ts] const isFragmentStart = isComment(node) && node.data == "[";
// [ts] const onMismatch = () => handleMismatch(node, vnode, parentComponent, parentSuspense, slotScopeIds, isFragmentStart);
// [ts] const { type, ref, shapeFlag } = vnode;
// [ts] const domType = node.nodeType;
// [ts] vnode.el = node;
// [ts] let nextNode = null;
// [ts] switch (type) {
// [ts] Text => {
// [ts] if (domType != 3 /* TEXT */) {
// [ts] nextNode = onMismatch();
// [ts] }
// [ts] else {
// [ts] if (node.data != vnode.children) {
// [ts] hasMismatch = true;
// [ts] warn(`Hydration text mismatch:` +
// [ts] `\n- Client: ${JSON.stringify(node.data)}` +
// [ts] `\n- Server: ${JSON.stringify(vnode.children)}`);
// [ts] node.data = vnode.children;
// [ts] }
// [ts] nextNode = nextSibling(node);
// [ts] }
// break;
// [ts] },
// [ts] Comment$1 => {
// [ts] if (domType != 8 /* COMMENT */ || isFragmentStart) {
// [ts] nextNode = onMismatch();
// [ts] }
// [ts] else {
// [ts] nextNode = nextSibling(node);
// [ts] }
// break;
// [ts] },
// [ts] Static => {
// [ts] if (domType != 1 /* ELEMENT */) {
// [ts] nextNode = onMismatch();
// [ts] }
// [ts] else {
// determine anchor, adopt content
// [ts] nextNode = node;
// if the static vnode has its content stripped during build,
// adopt it from the server-rendered HTML.
// [ts] const needToAdoptContent = !vnode.children.length;
// [ts] let i = 0;
// [ts] while (i < vnode.staticCount) { // [cap-migrate] for-step: i++
// [ts] if (needToAdoptContent)
// [ts] vnode.children += nextNode.outerHTML;
// [ts] if (i == vnode.staticCount - 1) {
// [ts] vnode.anchor = nextNode;
// [ts] }
// [ts] nextNode = nextSibling(nextNode);
// [ts] }
// [ts] return nextNode;
// [ts] }
// break;
// [ts] },
// [ts] Fragment => {
// [ts] if (!isFragmentStart) {
// [ts] nextNode = onMismatch();
// [ts] }
// [ts] else {
// [ts] nextNode = hydrateFragment(node, vnode, parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] }
// break;
// [ts] },
// [ts] else => {
// [ts] if (shapeFlag & 1 /* ELEMENT */) {
// [ts] if (domType != 1 /* ELEMENT */ ||
// [ts] vnode.type.toLowerCase() !=
// [ts] node.tagName.toLowerCase()) {
// [ts] nextNode = onMismatch();
// [ts] }
// [ts] else {
// [ts] nextNode = hydrateElement(node, vnode, parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] }
// [ts] }
// [ts] else if (shapeFlag & 6 /* COMPONENT */) {
// when setting up the render effect, if the initial vnode already
// has .el set, the component will perform hydration instead of mount
// on its sub-tree.
// [ts] vnode.slotScopeIds = slotScopeIds;
// [ts] const container = parentNode(node);
// [ts] mountComponent(vnode, container, null, parentComponent, parentSuspense, isSVGContainer(container), optimized);
// component may be async, so in the case of fragments we cannot rely
// on component's rendered output to determine the end of the fragment
// instead, we do a lookahead to find the end anchor node.
// [ts] nextNode = if (isFragmentStart) locateClosingAsyncAnchor(node) else nextSibling(node);
// #3787
// if component is async, it may get moved / unmounted before its
// inner component is loaded, so we need to give it a placeholder
// vnode that matches its adopted DOM.
// [ts] if (isAsyncWrapper(vnode)) {
// [ts] let subTree;
// [ts] if (isFragmentStart) {
// [ts] subTree = createVNode(Fragment);
// [ts] subTree.anchor = if (nextNode) nextNode.previousSibling else container.lastChild;
// [ts] }
// [ts] else {
// [ts] subTree =
// [ts] if (node.nodeType == 3) createTextVNode("") else createVNode("div");
// [ts] }
// [ts] subTree.el = node;
// [ts] vnode.component.subTree = subTree;
// [ts] }
// [ts] }
// [ts] else if (shapeFlag & 64 /* TELEPORT */) {
// [ts] if (domType != 8 /* COMMENT */) {
// [ts] nextNode = onMismatch();
// [ts] }
// [ts] else {
// [ts] nextNode = vnode.type.hydrate(node, vnode, parentComponent, parentSuspense, slotScopeIds, optimized, rendererInternals, hydrateChildren);
// [ts] }
// [ts] }
// [ts] else if (shapeFlag & 128 /* SUSPENSE */) {
// [ts] nextNode = vnode.type.hydrate(node, vnode, parentComponent, parentSuspense, isSVGContainer(parentNode(node)), slotScopeIds, optimized, rendererInternals, hydrateNode);
// [ts] }
// [ts] else {
// [ts] warn("Invalid HostVNode type:", type, `(${@typeName(type)})`);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (ref != null) {
// [ts] setRef(ref, null, parentSuspense, vnode);
// [ts] }
// [ts] return nextNode;
// [ts] };
// [ts] const hydrateElement = (el, vnode, parentComponent, parentSuspense, slotScopeIds, optimized) => {
// [ts] optimized = optimized || !!vnode.dynamicChildren;
// [ts] const { type, props, patchFlag, shapeFlag, dirs } = vnode;
// #4006 for form elements with non-string v-model value bindings
// e.g. <option :value="obj">, <input type="checkbox" :true-value="1">
// [ts] const forcePatchValue = (type == "input" && dirs) || type == "option";
// skip props & children if this is hoisted static nodes
// [ts] if (forcePatchValue || patchFlag != -1 /* HOISTED */) {
// [ts] if (dirs) {
// [ts] invokeDirectiveHook(vnode, null, parentComponent, "created");
// [ts] }
// props
// [ts] if (props) {
// [ts] if (forcePatchValue ||
// [ts] !optimized ||
// [ts] (patchFlag & 16 /* FULL_PROPS */ ||
// [ts] patchFlag & 32 /* HYDRATE_EVENTS */)) {
// [ts] for (const key in props) {
// [ts] if ((forcePatchValue && key.endsWith("value")) ||
// [ts] (isOn(key) && !isReservedProp(key))) {
// [ts] patchProp(el, key, null, props[key]);
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (props.onClick) {
// Fast path for click listeners (which is most often) to avoid
// iterating through props.
// [ts] patchProp(el, "onClick", null, props.onClick);
// [ts] }
// [ts] }
// vnode / directive hooks
// [ts] let vnodeHooks;
// [ts] if ((vnodeHooks = props && props.onVnodeBeforeMount)) {
// [ts] invokeVNodeHook(vnodeHooks, parentComponent, vnode);
// [ts] }
// [ts] if (dirs) {
// [ts] invokeDirectiveHook(vnode, null, parentComponent, "beforeMount");
// [ts] }
// [ts] if ((vnodeHooks = props && props.onVnodeMounted) || dirs) {
// [ts] queueEffectWithSuspense(() => {
// [ts] vnodeHooks && invokeVNodeHook(vnodeHooks, parentComponent, vnode);
// [ts] dirs && invokeDirectiveHook(vnode, null, parentComponent, "mounted");
// [ts] }, parentSuspense);
// [ts] }
// children
// [ts] if (shapeFlag & 16 /* ARRAY_CHILDREN */ &&
// skip if element has innerHTML / textContent
// [ts] !(props && (props.innerHTML || props.textContent))) {
// [ts] let next = hydrateChildren(el.firstChild, vnode, el, parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] let hasWarned = false;
// [ts] while (next) {
// [ts] hasMismatch = true;
// [ts] if (!hasWarned) {
// [ts] warn(`Hydration children mismatch in <${vnode.type}>: ` +
// [ts] `server rendered element contains more child nodes than client vdom.`);
// [ts] hasWarned = true;
// [ts] }
// The SSRed DOM contains more nodes than it should. Remove them.
// [ts] const cur = next;
// [ts] next = next.nextSibling;
// [ts] remove(cur);
// [ts] }
// [ts] }
// [ts] else if (shapeFlag & 8 /* TEXT_CHILDREN */) {
// [ts] if (el.textContent != vnode.children) {
// [ts] hasMismatch = true;
// [ts] warn(`Hydration text content mismatch in <${vnode.type}>:\n` +
// [ts] `- Client: ${el.textContent}\n` +
// [ts] `- Server: ${vnode.children}`);
// [ts] el.textContent = vnode.children;
// [ts] }
// [ts] }
// [ts] }
// [ts] return el.nextSibling;
// [ts] };
// [ts] const hydrateChildren = (node, parentVNode, container, parentComponent, parentSuspense, slotScopeIds, optimized) => {
// [ts] optimized = optimized || !!parentVNode.dynamicChildren;
// [ts] const children = parentVNode.children;
// [ts] const l = children.length;
// [ts] let hasWarned = false;
// [ts] let i = 0;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] const vnode = if (optimized) children[i] else (children[i] = normalizeVNode(children[i]));
// [ts] if (node) {
// [ts] node = hydrateNode(node, vnode, parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] }
// [ts] else if (vnode.type == Text && !vnode.children) {
// [ts] continue;
// [ts] }
// [ts] else {
// [ts] hasMismatch = true;
// [ts] if (!hasWarned) {
// [ts] warn(`Hydration children mismatch in <${container.tagName.toLowerCase()}>: ` +
// [ts] `server rendered element contains fewer child nodes than client vdom.`);
// [ts] hasWarned = true;
// [ts] }
// the SSRed DOM didn't contain enough nodes. Mount the missing ones.
// [ts] patch(null, vnode, container, null, parentComponent, parentSuspense, isSVGContainer(container), slotScopeIds);
// [ts] }
// [ts] }
// [ts] return node;
// [ts] };
// [ts] const hydrateFragment = (node, vnode, parentComponent, parentSuspense, slotScopeIds, optimized) => {
// [ts] const { slotScopeIds: fragmentSlotScopeIds } = vnode;
// [ts] if (fragmentSlotScopeIds) {
// [ts] slotScopeIds = if (slotScopeIds) slotScopeIds.concat(fragmentSlotScopeIds) else fragmentSlotScopeIds;
// [ts] }
// [ts] const container = parentNode(node);
// [ts] const next = hydrateChildren(nextSibling(node), vnode, container, parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] if (next && isComment(next) && next.data == "]") {
// [ts] return nextSibling((vnode.anchor = next));
// [ts] }
// [ts] else {
// fragment didn't hydrate successfully, since we didn't get a end anchor
// back. This should have led to node/children mismatch warnings.
// [ts] hasMismatch = true;
// since the anchor is missing, we need to create one and insert it
// [ts] insert((vnode.anchor = createComment(`]`)), container, next);
// [ts] return next;
// [ts] }
// [ts] };
// [ts] const handleMismatch = (node, vnode, parentComponent, parentSuspense, slotScopeIds, isFragment) => {
// [ts] hasMismatch = true;
// [ts] warn(if (`Hydration node mismatch:\n- Client vnode:`, vnode.type, `\n- Server rendered DOM:`, node, node.nodeType == 3 /* TEXT */) `(text)` else if (isComment(node) && node.data == "[") `(start of fragment)` else ``);
// [ts] vnode.el = null;
// [ts] if (isFragment) {
// remove excessive fragment nodes
// [ts] const end = locateClosingAsyncAnchor(node);
// [ts] while (true) {
// [ts] const next = nextSibling(node);
// [ts] if (next && next != end) {
// [ts] remove(next);
// [ts] }
// [ts] else {
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] const next = nextSibling(node);
// [ts] const container = parentNode(node);
// [ts] remove(node);
// [ts] patch(null, vnode, container, next, parentComponent, parentSuspense, isSVGContainer(container), slotScopeIds);
// [ts] return next;
// [ts] };
// [ts] const locateClosingAsyncAnchor = (node) => {
// [ts] let match_val = 0;
// [ts] while (node) {
// [ts] node = nextSibling(node);
// [ts] if (node && isComment(node)) {
// [ts] if (node.data == "[")
// [ts] match_val++;
// [ts] if (node.data == "]") {
// [ts] if (match_val == 0) {
// [ts] return nextSibling(node);
// [ts] }
// [ts] else {
// [ts] match_val--;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return node;
// [ts] };
// [ts] return [hydrate, hydrateNode];
// [ts] }
// [ts] let supported;
// [ts] let perf;
// [ts] fn startMeasure(instance: auto, type: auto) {
// [ts] if (instance.appContext.config.performance && isSupported()) {
// [ts] perf.mark(`vue-${type}-${instance.uid}`);
// [ts] }
// [ts] {
// [ts] devtoolsPerfStart(if (instance, type, supported) perf.now() else Date.now());
// [ts] }
// [ts] }
// [ts] fn endMeasure(instance: auto, type: auto) {
// [ts] if (instance.appContext.config.performance && isSupported()) {
// [ts] const startTag = `vue-${type}-${instance.uid}`;
// [ts] const endTag = startTag + `:end`;
// [ts] perf.mark(endTag);
// [ts] perf.measure(`<${formatComponentName(instance, instance.type)}> ${type}`, startTag, endTag);
// [ts] perf.clearMarks(startTag);
// [ts] perf.clearMarks(endTag);
// [ts] }
// [ts] {
// [ts] devtoolsPerfEnd(if (instance, type, supported) perf.now() else Date.now());
// [ts] }
// [ts] }
// [ts] fn isSupported() {
// [ts] if (supported != null) {
// [ts] return supported;
// [ts] }
// [ts] /* eslint-disable no-restricted-globals */
// [ts] if (@typeName(window) != "undefined" && window.performance) {
// [ts] supported = true;
// [ts] perf = window.performance;
// [ts] }
// [ts] else {
// [ts] supported = false;
// [ts] }
// [ts] /* eslint-enable no-restricted-globals */
// [ts] return supported;
// [ts] }
// [ts] fn createDevEffectOptions(instance: auto) {
// [ts] return {
// [ts] scheduler: queueJob,
// [ts] allowRecurse: true,
// [ts] onTrack: if (instance.rtc) e => invokeArrayFns(instance.rtc, e) else void 0,
// [ts] onTrigger: if (instance.rtg) e => invokeArrayFns(instance.rtg, e) else void 0
// [ts] };
// [ts] }
// [ts] const queuePostRenderEffect = queueEffectWithSuspense
// [ts] ;
// [ts] fn setRef(rawRef: auto, oldRawRef: auto, parentSuspense: auto, vnode: auto, isUnmount: auto = false) {
// [ts] if (isArray(rawRef)) {
// [ts] rawRef.forEach((r, i) => setRef(r, oldRawRef && (if (isArray(oldRawRef)) oldRawRef[i] else oldRawRef), parentSuspense, vnode, isUnmount));
// [ts] return;
// [ts] }
// [ts] if (isAsyncWrapper(vnode) && !isUnmount) {
// when mounting async components, nothing needs to be done,
// because the template ref is forwarded to inner component
// [ts] return;
// [ts] }
// [ts] const refValue = if (vnode.shapeFlag & 4 /* STATEFUL_COMPONENT */) getExposeProxy(vnode.component) || vnode.component.proxy else vnode.el;
// [ts] const value = if (isUnmount) null else refValue;
// [ts] const { i: owner, r: ref } = rawRef;
// [ts] if (!owner) {
// [ts] warn(`Missing ref owner context. ref cannot be used on hoisted vnodes. ` +
// [ts] `A vnode with ref must be created inside the render function.`);
// [ts] return;
// [ts] }
// [ts] const oldRef = oldRawRef && oldRawRef.r;
// [ts] const refs = if (owner.refs == EMPTY_OBJ) (owner.refs = {}) else owner.refs;
// [ts] const setupState = owner.setupState;
// dynamic ref changed. unset old ref
// [ts] if (oldRef != null && oldRef != ref) {
// [ts] if (isString(oldRef)) {
// [ts] refs[oldRef] = null;
// [ts] if (hasOwn(setupState, oldRef)) {
// [ts] setupState[oldRef] = null;
// [ts] }
// [ts] }
// [ts] else if (isRef(oldRef)) {
// [ts] oldRef.value = null;
// [ts] }
// [ts] }
// [ts] if (isString(ref)) {
// [ts] const doSet = () => {
// [ts] {
// [ts] refs[ref] = value;
// [ts] }
// [ts] if (hasOwn(setupState, ref)) {
// [ts] setupState[ref] = value;
// [ts] }
// [ts] };
// #1789: for non-null values, set them after render
// null values means this is unmount and it should not overwrite another
// ref with the same key
// [ts] if (value) {
// [ts] doSet.id = -1;
// [ts] queuePostRenderEffect(doSet, parentSuspense);
// [ts] }
// [ts] else {
// [ts] doSet();
// [ts] }
// [ts] }
// [ts] else if (isRef(ref)) {
// [ts] const doSet = () => {
// [ts] ref.value = value;
// [ts] };
// [ts] if (value) {
// [ts] doSet.id = -1;
// [ts] queuePostRenderEffect(doSet, parentSuspense);
// [ts] }
// [ts] else {
// [ts] doSet();
// [ts] }
// [ts] }
// [ts] else if (isFunction(ref)) {
// [ts] callWithErrorHandling(ref, owner, 12 /* FUNCTION_REF */, [value, refs]);
// [ts] }
// [ts] else {
// [ts] warn("Invalid template ref type:", value, `(${@typeName(value)})`);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * The createRenderer function accepts two generic arguments:
// [ts] * HostNode and HostElement, corresponding to Node and Element types in the
// [ts] * host environment. For example, for runtime-dom, HostNode would be the DOM
// [ts] * `Node` interface and HostElement would be the DOM `Element` interface.
// [ts] *
// [ts] * Custom renderers can pass in the platform specific types like this:
// [ts] *
// [ts] * ``` js
// [ts] * const { render, createApp } = createRenderer<Node, Element>({
// [ts] *   patchProp,
// [ts] *   ...nodeOps
// [ts] * })
// [ts] * ```
// [ts] */
// [ts] fn createRenderer(options: auto) {
// [ts] return baseCreateRenderer(options);
// [ts] }
// Separate API for creating hydration-enabled renderer.
// Hydration logic is only used when calling this function, making it
// tree-shakable.
// [ts] fn createHydrationRenderer(options: auto) {
// [ts] return baseCreateRenderer(options, createHydrationFunctions);
// [ts] }
// implementation
// [ts] fn baseCreateRenderer(options: auto, createHydrationFns: auto) {
// [ts] {
// [ts] const target = getGlobalThis();
// [ts] target.__VUE__ = true;
// [ts] setDevtoolsHook(target.__VUE_DEVTOOLS_GLOBAL_HOOK__);
// [ts] }
// [ts] const { insert: hostInsert, remove: hostRemove, patchProp: hostPatchProp, forcePatchProp: hostForcePatchProp, createElement: hostCreateElement, createText: hostCreateText, createComment: hostCreateComment, setText: hostSetText, setElementText: hostSetElementText, parentNode: hostParentNode, nextSibling: hostNextSibling, setScopeId: hostSetScopeId = NOOP, cloneNode: hostCloneNode, insertStaticContent: hostInsertStaticContent } = options;
// Note: functions inside this closure should use `const xxx = () => {}`
// style in order to prevent being inlined by minifiers.
// [ts] const patch = (n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// patching & not same type, unmount old tree
// [ts] if (n1 && !isSameVNodeType(n1, n2)) {
// [ts] anchor = getNextHostNode(n1);
// [ts] unmount(n1, parentComponent, parentSuspense, true);
// [ts] n1 = null;
// [ts] }
// [ts] if (n2.patchFlag == -2 /* BAIL */) {
// [ts] optimized = false;
// [ts] n2.dynamicChildren = null;
// [ts] }
// [ts] const { type, ref, shapeFlag } = n2;
// [ts] switch (type) {
// [ts] Text => {
// [ts] processText(n1, n2, container, anchor);
// break;
// [ts] },
// [ts] Comment$1 => {
// [ts] processCommentNode(n1, n2, container, anchor);
// break;
// [ts] },
// [ts] Static => {
// [ts] if (n1 == null) {
// [ts] mountStaticNode(n2, container, anchor, isSVG);
// [ts] }
// [ts] else {
// [ts] patchStaticNode(n1, n2, container, isSVG);
// [ts] }
// break;
// [ts] },
// [ts] Fragment => {
// [ts] processFragment(n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// break;
// [ts] },
// [ts] else => {
// [ts] if (shapeFlag & 1 /* ELEMENT */) {
// [ts] processElement(n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else if (shapeFlag & 6 /* COMPONENT */) {
// [ts] processComponent(n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else if (shapeFlag & 64 /* TELEPORT */) {
// [ts] type.process(n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized, internals);
// [ts] }
// [ts] else if (shapeFlag & 128 /* SUSPENSE */) {
// [ts] type.process(n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized, internals);
// [ts] }
// [ts] else {
// [ts] warn("Invalid VNode type:", type, `(${@typeName(type)})`);
// [ts] }
// [ts] }
// [ts] }
// set ref
// [ts] if (ref != null && parentComponent) {
// [ts] setRef(ref, n1 && n1.ref, parentSuspense, n2 || n1, !n2);
// [ts] }
// [ts] };
// [ts] const processText = (n1, n2, container, anchor) => {
// [ts] if (n1 == null) {
// [ts] hostInsert((n2.el = hostCreateText(n2.children)), container, anchor);
// [ts] }
// [ts] else {
// [ts] const el = (n2.el = n1.el);
// [ts] if (n2.children != n1.children) {
// [ts] hostSetText(el, n2.children);
// [ts] }
// [ts] }
// [ts] };
// [ts] const processCommentNode = (n1, n2, container, anchor) => {
// [ts] if (n1 == null) {
// [ts] hostInsert((n2.el = hostCreateComment(n2.children || "")), container, anchor);
// [ts] }
// [ts] else {
// there's no support for dynamic comments
// [ts] n2.el = n1.el;
// [ts] }
// [ts] };
// [ts] const mountStaticNode = (n2, container, anchor, isSVG) => {
// [ts] [n2.el, n2.anchor] = hostInsertStaticContent(n2.children, container, anchor, isSVG);
// [ts] };
// [ts] /**
// [ts] * Dev / HMR only
// [ts] */
// [ts] const patchStaticNode = (n1, n2, container, isSVG) => {
// static nodes are only patched during dev for HMR
// [ts] if (n2.children != n1.children) {
// [ts] const anchor = hostNextSibling(n1.anchor);
// remove existing
// [ts] removeStaticNode(n1);
// [ts] [n2.el, n2.anchor] = hostInsertStaticContent(n2.children, container, anchor, isSVG);
// [ts] }
// [ts] else {
// [ts] n2.el = n1.el;
// [ts] n2.anchor = n1.anchor;
// [ts] }
// [ts] };
// [ts] const moveStaticNode = (_p, container, nextSibling) => {
// [ts] let next;
// [ts] while (el && el != anchor) {
// [ts] next = hostNextSibling(el);
// [ts] hostInsert(el, container, nextSibling);
// [ts] el = next;
// [ts] }
// [ts] hostInsert(anchor, container, nextSibling);
// [ts] };
// [ts] const removeStaticNode = (_p) => {
// [ts] let next;
// [ts] while (el && el != anchor) {
// [ts] next = hostNextSibling(el);
// [ts] hostRemove(el);
// [ts] el = next;
// [ts] }
// [ts] hostRemove(anchor);
// [ts] };
// [ts] const processElement = (n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] isSVG = isSVG || n2.type == "svg";
// [ts] if (n1 == null) {
// [ts] mountElement(n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else {
// [ts] patchElement(n1, n2, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] };
// [ts] const mountElement = (vnode, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] let el;
// [ts] let vnodeHook;
// [ts] const { type, props, shapeFlag, transition, patchFlag, dirs } = vnode;
// [ts] {
// [ts] el = vnode.el = hostCreateElement(vnode.type, isSVG, props && props.is, props);
// mount children first, since some props may rely on child content
// being already rendered, e.g. `<select value>`
// [ts] if (shapeFlag & 8 /* TEXT_CHILDREN */) {
// [ts] hostSetElementText(el, vnode.children);
// [ts] }
// [ts] else if (shapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] mountChildren(vnode.children, el, null, parentComponent, parentSuspense, isSVG && type != "foreignObject", slotScopeIds, optimized);
// [ts] }
// [ts] if (dirs) {
// [ts] invokeDirectiveHook(vnode, null, parentComponent, "created");
// [ts] }
// props
// [ts] if (props) {
// [ts] for (const key in props) {
// [ts] if (!isReservedProp(key)) {
// [ts] hostPatchProp(el, key, null, props[key], isSVG, vnode.children, parentComponent, parentSuspense, unmountChildren);
// [ts] }
// [ts] }
// [ts] if ((vnodeHook = props.onVnodeBeforeMount)) {
// [ts] invokeVNodeHook(vnodeHook, parentComponent, vnode);
// [ts] }
// [ts] }
// scopeId
// [ts] setScopeId(el, vnode, vnode.scopeId, slotScopeIds, parentComponent);
// [ts] }
// [ts] {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(el, '__vnode', {
// [ts-skip] value: vnode,
// [ts-skip] enumerable: false
// [ts-skip] });
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(el, '__vueParentComponent', {
// [ts-skip] value: parentComponent,
// [ts-skip] enumerable: false
// [ts-skip] });
// [ts] }
// [ts] if (dirs) {
// [ts] invokeDirectiveHook(vnode, null, parentComponent, "beforeMount");
// [ts] }
// #1583 For inside suspense + suspense not resolved case, enter hook should call when suspense resolved
// #1689 For inside suspense + suspense resolved case, just call it
// [ts] const needCallTransitionHooks = (!parentSuspense || (parentSuspense && !parentSuspense.pendingBranch)) &&
// [ts] transition &&
// [ts] !transition.persisted;
// [ts] if (needCallTransitionHooks) {
// [ts] transition.beforeEnter(el);
// [ts] }
// [ts] hostInsert(el, container, anchor);
// [ts] if ((vnodeHook = props && props.onVnodeMounted) ||
// [ts] needCallTransitionHooks ||
// [ts] dirs) {
// [ts] queuePostRenderEffect(() => {
// [ts] vnodeHook && invokeVNodeHook(vnodeHook, parentComponent, vnode);
// [ts] needCallTransitionHooks && transition.enter(el);
// [ts] dirs && invokeDirectiveHook(vnode, null, parentComponent, "mounted");
// [ts] }, parentSuspense);
// [ts] }
// [ts] };
// [ts] const setScopeId = (el, vnode, scopeId, slotScopeIds, parentComponent) => {
// [ts] if (scopeId) {
// [ts] hostSetScopeId(el, scopeId);
// [ts] }
// [ts] if (slotScopeIds) {
// [ts] let i = 0;
// [ts] while (i < slotScopeIds.length) { // [cap-migrate] for-step: i++
// [ts] hostSetScopeId(el, slotScopeIds[i]);
// [ts] }
// [ts] }
// [ts] if (parentComponent) {
// [ts] let subTree = parentComponent.subTree;
// [ts] if (subTree.patchFlag > 0 &&
// [ts] subTree.patchFlag & 2048 /* DEV_ROOT_FRAGMENT */) {
// [ts] subTree =
// [ts] filterSingleRoot(subTree.children) || subTree;
// [ts] }
// [ts] if (vnode == subTree) {
// [ts] const parentVNode = parentComponent.vnode;
// [ts] setScopeId(el, parentVNode, parentVNode.scopeId, parentVNode.slotScopeIds, parentComponent.parent);
// [ts] }
// [ts] }
// [ts] };
// [ts] const mountChildren = (children, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized, start) => {
// [ts] let i = start;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = (children[i] = if (optimized) cloneIfMounted(children[i]) else normalizeVNode(children[i]));
// [ts] patch(null, child, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] };
// [ts] const patchElement = (n1, n2, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] const el = (n2.el = n1.el);
// [ts] let { patchFlag, dynamicChildren, dirs } = n2;
// #1426 take the old vnode's patch flag into account since user may clone a
// compiler-generated vnode, which de-opts to FULL_PROPS
// [ts] patchFlag |= n1.patchFlag & 16 /* FULL_PROPS */;
// [ts] const oldProps = n1.props || EMPTY_OBJ;
// [ts] const newProps = n2.props || EMPTY_OBJ;
// [ts] let vnodeHook;
// [ts] if ((vnodeHook = newProps.onVnodeBeforeUpdate)) {
// [ts] invokeVNodeHook(vnodeHook, parentComponent, n2, n1);
// [ts] }
// [ts] if (dirs) {
// [ts] invokeDirectiveHook(n2, n1, parentComponent, "beforeUpdate");
// [ts] }
// [ts] if (isHmrUpdating) {
// HMR updated, force full diff
// [ts] patchFlag = 0;
// [ts] optimized = false;
// [ts] dynamicChildren = null;
// [ts] }
// [ts] if (patchFlag > 0) {
// the presence of a patchFlag means this element's render code was
// generated by the compiler and can take the fast path.
// in this path old node and new node are guaranteed to have the same shape
// (i.e. at the exact same position in the source template)
// [ts] if (patchFlag & 16 /* FULL_PROPS */) {
// element props contain dynamic keys, full diff needed
// [ts] patchProps(el, n2, oldProps, newProps, parentComponent, parentSuspense, isSVG);
// [ts] }
// [ts] else {
// class
// this flag is matched when the element has dynamic class bindings.
// [ts] if (patchFlag & 2 /* CLASS */) {
// [ts] if (oldProps.class != newProps.class) {
// [ts] hostPatchProp(el, "class", null, newProps.class, isSVG);
// [ts] }
// [ts] }
// style
// this flag is matched when the element has dynamic style bindings
// [ts] if (patchFlag & 4 /* STYLE */) {
// [ts] hostPatchProp(el, "style", oldProps.style, newProps.style, isSVG);
// [ts] }
// props
// This flag is matched when the element has dynamic prop/attr bindings
// other than class and style. The keys of dynamic prop/attrs are saved for
// faster iteration.
// Note dynamic keys like :[foo]="bar" will cause this optimization to
// bail out and go through a full diff because we need to unset the old key
// [ts] if (patchFlag & 8 /* PROPS */) {
// if the flag is present then dynamicProps must be non-null
// [ts] const propsToUpdate = n2.dynamicProps;
// [ts] let i = 0;
// [ts] while (i < propsToUpdate.length) { // [cap-migrate] for-step: i++
// [ts] const key = propsToUpdate[i];
// [ts] const prev = oldProps[key];
// [ts] const next = newProps[key];
// [ts] if (next != prev ||
// [ts] (hostForcePatchProp && hostForcePatchProp(el, key))) {
// [ts] hostPatchProp(el, key, prev, next, isSVG, n1.children, parentComponent, parentSuspense, unmountChildren);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// text
// This flag is matched when the element has only dynamic text children.
// [ts] if (patchFlag & 1 /* TEXT */) {
// [ts] if (n1.children != n2.children) {
// [ts] hostSetElementText(el, n2.children);
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (!optimized && dynamicChildren == null) {
// unoptimized, full diff
// [ts] patchProps(el, n2, oldProps, newProps, parentComponent, parentSuspense, isSVG);
// [ts] }
// [ts] const areChildrenSVG = isSVG && n2.type != "foreignObject";
// [ts] if (dynamicChildren) {
// [ts] patchBlockChildren(n1.dynamicChildren, dynamicChildren, el, parentComponent, parentSuspense, areChildrenSVG, slotScopeIds);
// [ts] if (parentComponent && parentComponent.type.__hmrId) {
// [ts] traverseStaticChildren(n1, n2);
// [ts] }
// [ts] }
// [ts] else if (!optimized) {
// full diff
// [ts] patchChildren(n1, n2, el, null, parentComponent, parentSuspense, areChildrenSVG, slotScopeIds, false);
// [ts] }
// [ts] if ((vnodeHook = newProps.onVnodeUpdated) || dirs) {
// [ts] queuePostRenderEffect(() => {
// [ts] vnodeHook && invokeVNodeHook(vnodeHook, parentComponent, n2, n1);
// [ts] dirs && invokeDirectiveHook(n2, n1, parentComponent, "updated");
// [ts] }, parentSuspense);
// [ts] }
// [ts] };
// The fast path for blocks.
// [ts] const patchBlockChildren = (oldChildren, newChildren, fallbackContainer, parentComponent, parentSuspense, isSVG, slotScopeIds) => {
// [ts] let i = 0;
// [ts] while (i < newChildren.length) { // [cap-migrate] for-step: i++
// [ts] const oldVNode = oldChildren[i];
// [ts] const newVNode = newChildren[i];
// Determine the container (parent element) for the patch.
// [ts] const container =
// oldVNode may be an errored async setup() component inside Suspense
// which will not have a mounted element
// [ts] oldVNode.el &&
// - In the case of a Fragment, we need to provide the actual parent
// of the Fragment itself so it can move its children.
// [ts] (oldVNode.type == Fragment ||
// - In the case of different nodes, there is going to be a replacement
// which also requires the correct parent container
// [ts] !isSameVNodeType(oldVNode, newVNode) ||
// - In the case of a component, it could contain anything.
// [ts] oldVNode.shapeFlag & 6 /* COMPONENT */ ||
// [ts] oldVNode.shapeFlag & 64 /* TELEPORT */) ? hostParentNode(oldVNode.el) : // In other cases, the parent container is not actually used so we
// just pass the block element here to avoid a DOM parentNode call.
// [ts] fallbackContainer;
// [ts] patch(oldVNode, newVNode, container, null, parentComponent, parentSuspense, isSVG, slotScopeIds, true);
// [ts] }
// [ts] };
// [ts] const patchProps = (el, vnode, oldProps, newProps, parentComponent, parentSuspense, isSVG) => {
// [ts] if (oldProps != newProps) {
// [ts] for (const key in newProps) {
// empty string is not valid prop
// [ts] if (isReservedProp(key))
// [ts] continue;
// [ts] const next = newProps[key];
// [ts] const prev = oldProps[key];
// [ts] if (next != prev ||
// [ts] (hostForcePatchProp && hostForcePatchProp(el, key))) {
// [ts] hostPatchProp(el, key, prev, next, isSVG, vnode.children, parentComponent, parentSuspense, unmountChildren);
// [ts] }
// [ts] }
// [ts] if (oldProps != EMPTY_OBJ) {
// [ts] for (const key in oldProps) {
// [ts] if (!isReservedProp(key) && !(key == newProps)) {
// [ts] hostPatchProp(el, key, oldProps[key], null, isSVG, vnode.children, parentComponent, parentSuspense, unmountChildren);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] const processFragment = (n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] const fragmentStartAnchor = (n2.el = if (n1) n1.el else hostCreateText(""));
// [ts] const fragmentEndAnchor = (n2.anchor = if (n1) n1.anchor else hostCreateText(""));
// [ts] let { patchFlag, dynamicChildren, slotScopeIds: fragmentSlotScopeIds } = n2;
// [ts] if (dynamicChildren) {
// [ts] optimized = true;
// [ts] }
// check if this is a slot fragment with :slotted scope ids
// [ts] if (fragmentSlotScopeIds) {
// [ts] slotScopeIds = if (slotScopeIds) slotScopeIds.concat(fragmentSlotScopeIds) else fragmentSlotScopeIds;
// [ts] }
// [ts] if (isHmrUpdating) {
// HMR updated, force full diff
// [ts] patchFlag = 0;
// [ts] optimized = false;
// [ts] dynamicChildren = null;
// [ts] }
// [ts] if (n1 == null) {
// [ts] hostInsert(fragmentStartAnchor, container, anchor);
// [ts] hostInsert(fragmentEndAnchor, container, anchor);
// a fragment can only have array children
// since they are either generated by the compiler, or implicitly created
// from arrays.
// [ts] mountChildren(n2.children, container, fragmentEndAnchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else {
// [ts] if (patchFlag > 0 &&
// [ts] patchFlag & 64 /* STABLE_FRAGMENT */ &&
// [ts] dynamicChildren &&
// #2715 the previous fragment could've been a BAILed one as a result
// of renderSlot() with no valid children
// [ts] n1.dynamicChildren) {
// a stable fragment (template root or <template v-for>) doesn't need to
// patch children order, but it may contain dynamicChildren.
// [ts] patchBlockChildren(n1.dynamicChildren, dynamicChildren, container, parentComponent, parentSuspense, isSVG, slotScopeIds);
// [ts] if (parentComponent && parentComponent.type.__hmrId) {
// [ts] traverseStaticChildren(n1, n2);
// [ts] }
// [ts] else if (
// #2080 if the stable fragment has a key, it's a <template v-for> that may
//  get moved around. Make sure all root level vnodes inherit el.
// #2134 or if it's a component root, it may also get moved around
// as the component is being moved.
// [ts] n2.key != null ||
// [ts] (parentComponent && n2 == parentComponent.subTree)) {
// [ts] traverseStaticChildren(n1, n2, true /* shallow */);
// [ts] }
// [ts] }
// [ts] else {
// keyed / unkeyed, or manual fragments.
// for keyed & unkeyed, since they are compiler generated from v-for,
// each child is guaranteed to be a block so the fragment will never
// have dynamicChildren.
// [ts] patchChildren(n1, n2, container, fragmentEndAnchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] }
// [ts] };
// [ts] const processComponent = (n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] n2.slotScopeIds = slotScopeIds;
// [ts] if (n1 == null) {
// [ts] if (n2.shapeFlag & 512 /* COMPONENT_KEPT_ALIVE */) {
// [ts] parentComponent.ctx.activate(n2, container, anchor, isSVG, optimized);
// [ts] }
// [ts] else {
// [ts] mountComponent(n2, container, anchor, parentComponent, parentSuspense, isSVG, optimized);
// [ts] }
// [ts] }
// [ts] else {
// [ts] updateComponent(n1, n2, optimized);
// [ts] }
// [ts] };
// [ts] const mountComponent = (initialVNode, container, anchor, parentComponent, parentSuspense, isSVG, optimized) => {
// [ts] const instance = (initialVNode.component = createComponentInstance(initialVNode, parentComponent, parentSuspense));
// [ts] if (instance.type.__hmrId) {
// [ts] registerHMR(instance);
// [ts] }
// [ts] {
// [ts] pushWarningContext(initialVNode);
// [ts] startMeasure(instance, `mount`);
// [ts] }
// inject renderer internals for keepAlive
// [ts] if (isKeepAlive(initialVNode)) {
// [ts] instance.ctx.renderer = internals;
// [ts] }
// resolve props and slots for setup context
// [ts] {
// [ts] {
// [ts] startMeasure(instance, `init`);
// [ts] }
// [ts] setupComponent(instance);
// [ts] {
// [ts] endMeasure(instance, `init`);
// [ts] }
// [ts] }
// setup() is async. This component relies on async logic to be resolved
// before proceeding
// [ts] if (instance.asyncDep) {
// [ts] parentSuspense && parentSuspense.registerDep(instance, setupRenderEffect);
// Give it a placeholder if this is not hydration
// TODO handle self-defined fallback
// [ts] if (!initialVNode.el) {
// [ts] const placeholder = (instance.subTree = createVNode(Comment$1));
// [ts] processCommentNode(null, placeholder, container, anchor);
// [ts] }
// [ts] return;
// [ts] }
// [ts] setupRenderEffect(instance, initialVNode, container, anchor, parentSuspense, isSVG, optimized);
// [ts] {
// [ts] popWarningContext();
// [ts] endMeasure(instance, `mount`);
// [ts] }
// [ts] };
// [ts] const updateComponent = (n1, n2, optimized) => {
// [ts] const instance = (n2.component = n1.component);
// [ts] if (shouldUpdateComponent(n1, n2, optimized)) {
// [ts] if (instance.asyncDep &&
// [ts] !instance.asyncResolved) {
// async & still pending - just update props and slots
// since the component's reactive effect for render isn't set-up yet
// [ts] {
// [ts] pushWarningContext(n2);
// [ts] }
// [ts] updateComponentPreRender(instance, n2, optimized);
// [ts] {
// [ts] popWarningContext();
// [ts] }
// [ts] return;
// [ts] }
// [ts] else {
// normal update
// [ts] instance.next = n2;
// in case the child component is also queued, remove it to avoid
// double updating the same child component in the same flush.
// [ts] invalidateJob(instance.update);
// instance.update is the reactive effect runner.
// [ts] instance.update();
// [ts] }
// [ts] }
// [ts] else {
// no update needed. just copy over properties
// [ts] n2.component = n1.component;
// [ts] n2.el = n1.el;
// [ts] instance.vnode = n2;
// [ts] }
// [ts] };
// [ts] const setupRenderEffect = (instance, initialVNode, container, anchor, parentSuspense, isSVG, optimized) => {
// create reactive effect for rendering
// [ts] instance.update = effect(() => {
// [ts] if (!instance.isMounted) {
// [ts] let vnodeHook;
// [ts] const { el, props } = initialVNode;
// [ts] const { bm, m, parent } = instance;
// beforeMount hook
// [ts] if (bm) {
// [ts] invokeArrayFns(bm);
// [ts] }
// onVnodeBeforeMount
// [ts] if ((vnodeHook = props && props.onVnodeBeforeMount)) {
// [ts] invokeVNodeHook(vnodeHook, parent, initialVNode);
// [ts] }
// [ts] if (el && hydrateNode) {
// vnode has adopted host node - perform hydration instead of mount.
// [ts] const hydrateSubTree = () => {
// [ts] {
// [ts] startMeasure(instance, `render`);
// [ts] }
// [ts] instance.subTree = renderComponentRoot(instance);
// [ts] {
// [ts] endMeasure(instance, `render`);
// [ts] }
// [ts] {
// [ts] startMeasure(instance, `hydrate`);
// [ts] }
// [ts] hydrateNode(el, instance.subTree, instance, parentSuspense, null);
// [ts] {
// [ts] endMeasure(instance, `hydrate`);
// [ts] }
// [ts] };
// [ts] if (isAsyncWrapper(initialVNode)) {
// [ts] initialVNode.type.__asyncLoader().then(
// note: we are moving the render call into an async callback,
// which means it won't track dependencies - but it's ok because
// a server-rendered async wrapper is already in resolved state
// and it will never need to change.
// [ts] () => !instance.isUnmounted && hydrateSubTree());
// [ts] }
// [ts] else {
// [ts] hydrateSubTree();
// [ts] }
// [ts] }
// [ts] else {
// [ts] {
// [ts] startMeasure(instance, `render`);
// [ts] }
// [ts] const subTree = (instance.subTree = renderComponentRoot(instance));
// [ts] {
// [ts] endMeasure(instance, `render`);
// [ts] }
// [ts] {
// [ts] startMeasure(instance, `patch`);
// [ts] }
// [ts] patch(null, subTree, container, anchor, instance, parentSuspense, isSVG);
// [ts] {
// [ts] endMeasure(instance, `patch`);
// [ts] }
// [ts] initialVNode.el = subTree.el;
// [ts] }
// mounted hook
// [ts] if (m) {
// [ts] queuePostRenderEffect(m, parentSuspense);
// [ts] }
// onVnodeMounted
// [ts] if ((vnodeHook = props && props.onVnodeMounted)) {
// [ts] const scopedInitialVNode = initialVNode;
// [ts] queuePostRenderEffect(() => invokeVNodeHook(vnodeHook, parent, scopedInitialVNode), parentSuspense);
// [ts] }
// activated hook for keep-alive roots.
// #1742 activated hook must be accessed after first render
// since the hook may be injected by a child keep-alive
// [ts] if (initialVNode.shapeFlag & 256 /* COMPONENT_SHOULD_KEEP_ALIVE */) {
// [ts] instance.a && queuePostRenderEffect(instance.a, parentSuspense);
// [ts] }
// [ts] instance.isMounted = true;
// [ts] {
// [ts] devtoolsComponentAdded(instance);
// [ts] }
// #2458: deference mount-only object parameters to prevent memleaks
// [ts] initialVNode = container = anchor = null;
// [ts] }
// [ts] else {
// updateComponent
// This is triggered by mutation of component's own state (next: null)
// OR parent calling processComponent (next: VNode)
// [ts] let { next, bu, u, parent, vnode } = instance;
// [ts] let originNext = next;
// [ts] let vnodeHook;
// [ts] {
// [ts] pushWarningContext(next || instance.vnode);
// [ts] }
// [ts] if (next) {
// [ts] next.el = vnode.el;
// [ts] updateComponentPreRender(instance, next, optimized);
// [ts] }
// [ts] else {
// [ts] next = vnode;
// [ts] }
// beforeUpdate hook
// [ts] if (bu) {
// [ts] invokeArrayFns(bu);
// [ts] }
// onVnodeBeforeUpdate
// [ts] if ((vnodeHook = next.props && next.props.onVnodeBeforeUpdate)) {
// [ts] invokeVNodeHook(vnodeHook, parent, next, vnode);
// [ts] }
// render
// [ts] {
// [ts] startMeasure(instance, `render`);
// [ts] }
// [ts] const nextTree = renderComponentRoot(instance);
// [ts] {
// [ts] endMeasure(instance, `render`);
// [ts] }
// [ts] const prevTree = instance.subTree;
// [ts] instance.subTree = nextTree;
// [ts] {
// [ts] startMeasure(instance, `patch`);
// [ts] }
// [ts] patch(prevTree, nextTree,
// parent may have changed if it's in a teleport
// [ts] hostParentNode(prevTree.el),
// anchor may have changed if it's in a fragment
// [ts] getNextHostNode(prevTree), instance, parentSuspense, isSVG);
// [ts] {
// [ts] endMeasure(instance, `patch`);
// [ts] }
// [ts] next.el = nextTree.el;
// [ts] if (originNext == null) {
// self-triggered update. In case of HOC, update parent component
// vnode el. HOC is indicated by parent instance's subTree pointing
// to child component's vnode
// [ts] updateHOCHostEl(instance, nextTree.el);
// [ts] }
// updated hook
// [ts] if (u) {
// [ts] queuePostRenderEffect(u, parentSuspense);
// [ts] }
// onVnodeUpdated
// [ts] if ((vnodeHook = next.props && next.props.onVnodeUpdated)) {
// [ts] queuePostRenderEffect(() => invokeVNodeHook(vnodeHook, parent, next, vnode), parentSuspense);
// [ts] }
// [ts] {
// [ts] devtoolsComponentUpdated(instance);
// [ts] }
// [ts] {
// [ts] popWarningContext();
// [ts] }
// [ts] }
// [ts] }, createDevEffectOptions(instance) );
// [ts] {
// @ts-ignore
// [ts] instance.update.ownerInstance = instance;
// [ts] }
// [ts] };
// [ts] const updateComponentPreRender = (instance, nextVNode, optimized) => {
// [ts] nextVNode.component = instance;
// [ts] const prevProps = instance.vnode.props;
// [ts] instance.vnode = nextVNode;
// [ts] instance.next = null;
// [ts] updateProps(instance, nextVNode.props, prevProps, optimized);
// [ts] updateSlots(instance, nextVNode.children, optimized);
// [ts] pauseTracking();
// props update may have triggered pre-flush watchers.
// flush them before the render update.
// [ts] flushPreFlushCbs(null, instance.update);
// [ts] resetTracking();
// [ts] };
// [ts] const patchChildren = (n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] const c1 = n1 && n1.children;
// [ts] const prevShapeFlag = if (n1) n1.shapeFlag else 0;
// [ts] const c2 = n2.children;
// [ts] const { patchFlag, shapeFlag } = n2;
// fast path
// [ts] if (patchFlag > 0) {
// [ts] if (patchFlag & 128 /* KEYED_FRAGMENT */) {
// this could be either fully-keyed or mixed (some keyed some not)
// presence of patchFlag means children are guaranteed to be arrays
// [ts] patchKeyedChildren(c1, c2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] return;
// [ts] }
// [ts] else if (patchFlag & 256 /* UNKEYED_FRAGMENT */) {
// unkeyed
// [ts] patchUnkeyedChildren(c1, c2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] return;
// [ts] }
// [ts] }
// children has 3 possibilities: text, array or no children.
// [ts] if (shapeFlag & 8 /* TEXT_CHILDREN */) {
// text children fast path
// [ts] if (prevShapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] unmountChildren(c1, parentComponent, parentSuspense);
// [ts] }
// [ts] if (c2 != c1) {
// [ts] hostSetElementText(container, c2);
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (prevShapeFlag & 16 /* ARRAY_CHILDREN */) {
// prev children was array
// [ts] if (shapeFlag & 16 /* ARRAY_CHILDREN */) {
// two arrays, cannot assume anything, do full diff
// [ts] patchKeyedChildren(c1, c2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else {
// no new children, just unmount old
// [ts] unmountChildren(c1, parentComponent, parentSuspense, true);
// [ts] }
// [ts] }
// [ts] else {
// prev children was text OR null
// new children is array OR null
// [ts] if (prevShapeFlag & 8 /* TEXT_CHILDREN */) {
// [ts] hostSetElementText(container, "");
// [ts] }
// mount new if array
// [ts] if (shapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] mountChildren(c2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] const patchUnkeyedChildren = (c1, c2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] c1 = c1 || EMPTY_ARR;
// [ts] c2 = c2 || EMPTY_ARR;
// [ts] const oldLength = c1.length;
// [ts] const newLength = c2.length;
// [ts] const commonLength = Math.min(oldLength, newLength);
// [ts] let i;
// [ts] i = 0;
// [ts] while (i < commonLength) { // [cap-migrate] for-step: i++
// [ts] const nextChild = (c2[i] = if (optimized) cloneIfMounted(c2[i]) else normalizeVNode(c2[i]));
// [ts] patch(c1[i], nextChild, container, null, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] if (oldLength > newLength) {
// remove old
// [ts] unmountChildren(c1, parentComponent, parentSuspense, true, false, commonLength);
// [ts] }
// [ts] else {
// mount new
// [ts] mountChildren(c2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized, commonLength);
// [ts] }
// [ts] };
// can be all-keyed or mixed
// [ts] const patchKeyedChildren = (c1, c2, container, parentAnchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized) => {
// [ts] let i = 0;
// [ts] const l2 = c2.length;
// [ts] let e1 = c1.length - 1; // prev ending index
// [ts] let e2 = l2 - 1; // next ending index
// 1. sync from start
// (a b) c
// (a b) d e
// [ts] while (i <= e1 && i <= e2) {
// [ts] const n1 = c1[i];
// [ts] const n2 = (c2[i] = if (optimized) cloneIfMounted(c2[i]) else normalizeVNode(c2[i]));
// [ts] if (isSameVNodeType(n1, n2)) {
// [ts] patch(n1, n2, container, null, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else {
// [ts] break;
// [ts] }
// [ts] i++;
// [ts] }
// 2. sync from end
// a (b c)
// d e (b c)
// [ts] while (i <= e1 && i <= e2) {
// [ts] const n1 = c1[e1];
// [ts] const n2 = (c2[e2] = if (optimized) cloneIfMounted(c2[e2]) else normalizeVNode(c2[e2]));
// [ts] if (isSameVNodeType(n1, n2)) {
// [ts] patch(n1, n2, container, null, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else {
// [ts] break;
// [ts] }
// [ts] e1--;
// [ts] e2--;
// [ts] }
// 3. common sequence + mount
// (a b)
// (a b) c
// i = 2, e1 = 1, e2 = 2
// (a b)
// c (a b)
// i = 0, e1 = -1, e2 = 0
// [ts] if (i > e1) {
// [ts] if (i <= e2) {
// [ts] const nextPos = e2 + 1;
// [ts] const anchor = if (nextPos < l2) c2[nextPos].el else parentAnchor;
// [ts] while (i <= e2) {
// [ts] patch(null, (c2[i] = if (optimized) cloneIfMounted(c2[i]) else normalizeVNode(c2[i])), container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] i++;
// [ts] }
// [ts] }
// [ts] }
// 4. common sequence + unmount
// (a b) c
// (a b)
// i = 2, e1 = 2, e2 = 1
// a (b c)
// (b c)
// i = 0, e1 = 0, e2 = -1
// [ts] else if (i > e2) {
// [ts] while (i <= e1) {
// [ts] unmount(c1[i], parentComponent, parentSuspense, true);
// [ts] i++;
// [ts] }
// [ts] }
// 5. unknown sequence
// [i ... e1 + 1]: a b [c d e] f g
// [i ... e2 + 1]: a b [e d c h] f g
// i = 2, e1 = 4, e2 = 5
// [ts] else {
// [ts] const s1 = i; // prev starting index
// [ts] const s2 = i; // next starting index
// 5.1 build key:index map for newChildren
// [ts] const keyToNewIndexMap = Map.init();
// [ts] i = s2;
// [ts] while (i <= e2) { // [cap-migrate] for-step: i++
// [ts] const nextChild = (c2[i] = if (optimized) cloneIfMounted(c2[i]) else normalizeVNode(c2[i]));
// [ts] if (nextChild.key != null) {
// [ts] if (keyToNewIndexMap.has(nextChild.key)) {
// [ts] warn(`Duplicate keys found during update:`, JSON.stringify(nextChild.key), `Make sure keys are unique.`);
// [ts] }
// [ts] keyToNewIndexMap.set(nextChild.key, i);
// [ts] }
// [ts] }
// 5.2 loop through old children left to be patched and try to patch
// matching nodes & remove nodes that are no longer present
// [ts] let j;
// [ts] let patched = 0;
// [ts] const toBePatched = e2 - s2 + 1;
// [ts] let moved = false;
// used to track whether any node has moved
// [ts] let maxNewIndexSoFar = 0;
// works as Map<newIndex, oldIndex>
// Note that oldIndex is offset by +1
// and oldIndex = 0 is a special value indicating the new node has
// no corresponding old node.
// used for determining longest stable subsequence
// [ts] const newIndexToOldIndexMap = Array.init(toBePatched);
// [ts] i = 0;
// [ts] while (i < toBePatched) // [cap-migrate] for-step: i++
// [ts] newIndexToOldIndexMap[i] = 0;
// [ts] i = s1;
// [ts] while (i <= e1) { // [cap-migrate] for-step: i++
// [ts] const prevChild = c1[i];
// [ts] if (patched >= toBePatched) {
// all new children have been patched so this can only be a removal
// [ts] unmount(prevChild, parentComponent, parentSuspense, true);
// [ts] continue;
// [ts] }
// [ts] let newIndex;
// [ts] if (prevChild.key != null) {
// [ts] newIndex = keyToNewIndexMap.get(prevChild.key);
// [ts] }
// [ts] else {
// key-less node, try to locate a key-less node of the same type
// [ts] j = s2;
// [ts] while (j <= e2) { // [cap-migrate] for-step: j++
// [ts] if (newIndexToOldIndexMap[j - s2] == 0 &&
// [ts] isSameVNodeType: (prevChild, c2[j]) => {
// [ts] newIndex = j;
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (newIndex == null) {
// [ts] unmount(prevChild, parentComponent, parentSuspense, true);
// [ts] }
// [ts] else {
// [ts] newIndexToOldIndexMap[newIndex - s2] = i + 1;
// [ts] if (newIndex >= maxNewIndexSoFar) {
// [ts] maxNewIndexSoFar = newIndex;
// [ts] }
// [ts] else {
// [ts] moved = true;
// [ts] }
// [ts] patch(prevChild, c2[newIndex], container, null, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] patched++;
// [ts] }
// [ts] }
// 5.3 move and mount
// generate longest stable subsequence only when nodes have moved
// [ts] const increasingNewIndexSequence = if (moved) getSequence(newIndexToOldIndexMap) else EMPTY_ARR;
// [ts] j = increasingNewIndexSequence.length - 1;
// looping backwards so that we can use last patched node as anchor
// [ts] i = toBePatched - 1;
// [ts] while (i >= 0) { // [cap-migrate] for-step: i--
// [ts] const nextIndex = s2 + i;
// [ts] const nextChild = c2[nextIndex];
// [ts] const anchor = if (nextIndex + 1 < l2) c2[nextIndex + 1].el else parentAnchor;
// [ts] if (newIndexToOldIndexMap[i] == 0) {
// mount new
// [ts] patch(null, nextChild, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] else if (moved) {
// move if:
// There is no stable subsequence (e.g. a reverse)
// OR current node is not among the stable sequence
// [ts] if (j < 0 || i != increasingNewIndexSequence[j]) {
// [ts] move(nextChild, container, anchor, 2 /* REORDER */);
// [ts] }
// [ts] else {
// [ts] j--;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] const move = (vnode, container, anchor, moveType, parentSuspense) => {
// [ts] const { el, type, transition, children, shapeFlag } = vnode;
// [ts] if (shapeFlag & 6 /* COMPONENT */) {
// [ts] move(vnode.component.subTree, container, anchor, moveType);
// [ts] return;
// [ts] }
// [ts] if (shapeFlag & 128 /* SUSPENSE */) {
// [ts] vnode.suspense.move(container, anchor, moveType);
// [ts] return;
// [ts] }
// [ts] if (shapeFlag & 64 /* TELEPORT */) {
// [ts] type.move(vnode, container, anchor, internals);
// [ts] return;
// [ts] }
// [ts] if (type == Fragment) {
// [ts] hostInsert(el, container, anchor);
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] move(children[i], container, anchor, moveType);
// [ts] }
// [ts] hostInsert(vnode.anchor, container, anchor);
// [ts] return;
// [ts] }
// [ts] if (type == Static) {
// [ts] moveStaticNode(vnode, container, anchor);
// [ts] return;
// [ts] }
// single nodes
// [ts] const needTransition = moveType != 2 /* REORDER */ &&
// [ts] shapeFlag & 1 /* ELEMENT */ &&
// [ts] transition;
// [ts] if (needTransition) {
// [ts] if (moveType == 0 /* ENTER */) {
// [ts] transition.beforeEnter(el);
// [ts] hostInsert(el, container, anchor);
// [ts] queuePostRenderEffect(() => transition.enter(el), parentSuspense);
// [ts] }
// [ts] else {
// [ts] const { leave, delayLeave, afterLeave } = transition;
// [ts] const remove = () => hostInsert(el, container, anchor);
// [ts] const performLeave = () => {
// [ts] leave(el, () => {
// [ts] remove();
// [ts] afterLeave && afterLeave();
// [ts] });
// [ts] };
// [ts] if (delayLeave) {
// [ts] delayLeave(el, remove, performLeave);
// [ts] }
// [ts] else {
// [ts] performLeave();
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] hostInsert(el, container, anchor);
// [ts] }
// [ts] };
// [ts] const unmount = (vnode, parentComponent, parentSuspense, doRemove, optimized) => {
// [ts] const { type, props, ref, children, dynamicChildren, shapeFlag, patchFlag, dirs } = vnode;
// unset ref
// [ts] if (ref != null) {
// [ts] setRef(ref, null, parentSuspense, vnode, true);
// [ts] }
// [ts] if (shapeFlag & 256 /* COMPONENT_SHOULD_KEEP_ALIVE */) {
// [ts] parentComponent.ctx.deactivate(vnode);
// [ts] return;
// [ts] }
// [ts] const shouldInvokeDirs = shapeFlag & 1 /* ELEMENT */ && dirs;
// [ts] let vnodeHook;
// [ts] if ((vnodeHook = props && props.onVnodeBeforeUnmount)) {
// [ts] invokeVNodeHook(vnodeHook, parentComponent, vnode);
// [ts] }
// [ts] if (shapeFlag & 6 /* COMPONENT */) {
// [ts] unmountComponent(vnode.component, parentSuspense, doRemove);
// [ts] }
// [ts] else {
// [ts] if (shapeFlag & 128 /* SUSPENSE */) {
// [ts] vnode.suspense.unmount(parentSuspense, doRemove);
// [ts] return;
// [ts] }
// [ts] if (shouldInvokeDirs) {
// [ts] invokeDirectiveHook(vnode, null, parentComponent, "beforeUnmount");
// [ts] }
// [ts] if (shapeFlag & 64 /* TELEPORT */) {
// [ts] vnode.type.remove(vnode, parentComponent, parentSuspense, optimized, internals, doRemove);
// [ts] }
// [ts] else if (dynamicChildren &&
// #1153: fast path should not be taken for non-stable (v-for) fragments
// [ts] (type != Fragment ||
// [ts] (patchFlag > 0 && patchFlag & 64 /* STABLE_FRAGMENT */))) {
// fast path for block nodes: only need to unmount dynamic children.
// [ts] unmountChildren(dynamicChildren, parentComponent, parentSuspense, false, true);
// [ts] }
// [ts] else if ((type == Fragment &&
// [ts] (patchFlag & 128 /* KEYED_FRAGMENT */ ||
// [ts] patchFlag & 256 /* UNKEYED_FRAGMENT */)) ||
// [ts] (!optimized && shapeFlag & 16 /* ARRAY_CHILDREN */)) {
// [ts] unmountChildren(children, parentComponent, parentSuspense);
// [ts] }
// [ts] if (doRemove) {
// [ts] remove(vnode);
// [ts] }
// [ts] }
// [ts] if ((vnodeHook = props && props.onVnodeUnmounted) || shouldInvokeDirs) {
// [ts] queuePostRenderEffect(() => {
// [ts] vnodeHook && invokeVNodeHook(vnodeHook, parentComponent, vnode);
// [ts] shouldInvokeDirs &&
// [ts] invokeDirectiveHook(vnode, null, parentComponent, "unmounted");
// [ts] }, parentSuspense);
// [ts] }
// [ts] };
// [ts] const remove = vnode => {
// [ts] const { type, el, anchor, transition } = vnode;
// [ts] if (type == Fragment) {
// [ts] removeFragment(el, anchor);
// [ts] return;
// [ts] }
// [ts] if (type == Static) {
// [ts] removeStaticNode(vnode);
// [ts] return;
// [ts] }
// [ts] const performRemove = () => {
// [ts] hostRemove(el);
// [ts] if (transition && !transition.persisted && transition.afterLeave) {
// [ts] transition.afterLeave();
// [ts] }
// [ts] };
// [ts] if (vnode.shapeFlag & 1 /* ELEMENT */ &&
// [ts] transition &&
// [ts] !transition.persisted) {
// [ts] const { leave, delayLeave } = transition;
// [ts] const performLeave = () => leave(el, performRemove);
// [ts] if (delayLeave) {
// [ts] delayLeave(vnode.el, performRemove, performLeave);
// [ts] }
// [ts] else {
// [ts] performLeave();
// [ts] }
// [ts] }
// [ts] else {
// [ts] performRemove();
// [ts] }
// [ts] };
// [ts] const removeFragment = (cur, end) => {
// For fragments, directly remove all contained DOM nodes.
// (fragment child nodes cannot have transition)
// [ts] let next;
// [ts] while (cur != end) {
// [ts] next = hostNextSibling(cur);
// [ts] hostRemove(cur);
// [ts] cur = next;
// [ts] }
// [ts] hostRemove(end);
// [ts] };
// [ts] const unmountComponent = (instance, parentSuspense, doRemove) => {
// [ts] if (instance.type.__hmrId) {
// [ts] unregisterHMR(instance);
// [ts] }
// [ts] const { bum, effects, update, subTree, um } = instance;
// beforeUnmount hook
// [ts] if (bum) {
// [ts] invokeArrayFns(bum);
// [ts] }
// [ts] if (effects) {
// [ts] let i = 0;
// [ts] while (i < effects.length) { // [cap-migrate] for-step: i++
// [ts] stop(effects[i]);
// [ts] }
// [ts] }
// update may be null if a component is unmounted before its async
// setup has resolved.
// [ts] if (update) {
// [ts] stop(update);
// [ts] unmount(subTree, instance, parentSuspense, doRemove);
// [ts] }
// unmounted hook
// [ts] if (um) {
// [ts] queuePostRenderEffect(um, parentSuspense);
// [ts] }
// [ts] queuePostRenderEffect(() => {
// [ts] instance.isUnmounted = true;
// [ts] }, parentSuspense);
// A component with async dep inside a pending suspense is unmounted before
// its async dep resolves. This should remove the dep from the suspense, and
// cause the suspense to resolve immediately if that was the last dep.
// [ts] if (parentSuspense &&
// [ts] parentSuspense.pendingBranch &&
// [ts] !parentSuspense.isUnmounted &&
// [ts] instance.asyncDep &&
// [ts] !instance.asyncResolved &&
// [ts] instance.suspenseId == parentSuspense.pendingId) {
// [ts] parentSuspense.deps--;
// [ts] if (parentSuspense.deps == 0) {
// [ts] parentSuspense.resolve();
// [ts] }
// [ts] }
// [ts] {
// [ts] devtoolsComponentRemoved(instance);
// [ts] }
// [ts] };
// [ts] const unmountChildren = (children, parentComponent, parentSuspense, doRemove, optimized, start) => {
// [ts] let i = start;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] unmount(children[i], parentComponent, parentSuspense, doRemove, optimized);
// [ts] }
// [ts] };
// [ts] const getNextHostNode = vnode => {
// [ts] if (vnode.shapeFlag & 6 /* COMPONENT */) {
// [ts] return getNextHostNode(vnode.component.subTree);
// [ts] }
// [ts] if (vnode.shapeFlag & 128 /* SUSPENSE */) {
// [ts] return vnode.suspense.next();
// [ts] }
// [ts] return hostNextSibling((vnode.anchor || vnode.el));
// [ts] };
// [ts] const render = (vnode, container, isSVG) => {
// [ts] if (vnode == null) {
// [ts] if (container._vnode) {
// [ts] unmount(container._vnode, null, null, true);
// [ts] }
// [ts] }
// [ts] else {
// [ts] patch(container._vnode || null, vnode, container, null, null, null, isSVG);
// [ts] }
// [ts] flushPostFlushCbs();
// [ts] container._vnode = vnode;
// [ts] };
// [ts] const internals = {
// [ts] p: patch,
// [ts] um: unmount,
// [ts] m: move,
// [ts] r: remove,
// [ts] mt: mountComponent,
// [ts] mc: mountChildren,
// [ts] pc: patchChildren,
// [ts] pbc: patchBlockChildren,
// [ts] n: getNextHostNode,
// [ts] o: options
// [ts] };
// [ts] let hydrate;
// [ts] let hydrateNode;
// [ts] if (createHydrationFns) {
// [ts] [hydrate, hydrateNode] = createHydrationFns(internals);
// [ts] }
// [ts] return {
// [ts] render,
// [ts] hydrate,
// [ts] createApp: createAppAPI(render, hydrate)
// [ts] };
// [ts] }
// [ts] fn invokeVNodeHook(hook: auto, instance: auto, vnode: auto, prevVNode: auto = null) {
// [ts] callWithAsyncErrorHandling(hook, instance, 7 /* VNODE_HOOK */, [
// [ts] vnode,
// [ts] prevVNode
// [ts] ]);
// [ts] }
// [ts] /**
// [ts] * #1156
// [ts] * When a component is HMR-enabled, we need to make sure that all static nodes
// [ts] * inside a block also inherit the DOM element from the previous tree so that
// [ts] * HMR updates (which are full updates) can retrieve the element for patching.
// [ts] *
// [ts] * #2080
// [ts] * Inside keyed `template` fragment static children, if a fragment is moved,
// [ts] * the children will always moved so that need inherit el form previous nodes
// [ts] * to ensure correct moved position.
// [ts] */
// [ts] fn traverseStaticChildren(n1: auto, n2: auto, shallow: auto = false) {
// [ts] const ch1 = n1.children;
// [ts] const ch2 = n2.children;
// [ts] if (isArray(ch1) && isArray(ch2)) {
// [ts] let i = 0;
// [ts] while (i < ch1.length) { // [cap-migrate] for-step: i++
// this is only called in the optimized path so array children are
// guaranteed to be vnodes
// [ts] const c1 = ch1[i];
// [ts] let c2 = ch2[i];
// [ts] if (c2.shapeFlag & 1 /* ELEMENT */ && !c2.dynamicChildren) {
// [ts] if (c2.patchFlag <= 0 || c2.patchFlag == 32 /* HYDRATE_EVENTS */) {
// [ts] c2 = ch2[i] = cloneIfMounted(ch2[i]);
// [ts] c2.el = c1.el;
// [ts] }
// [ts] if (!shallow)
// [ts] traverseStaticChildren(c1, c2);
// [ts] }
// also inherit for comment nodes, but not placeholders (e.g. v-if which
// would have received .el during block patch)
// [ts] if (c2.type == Comment$1 && !c2.el) {
// [ts] c2.el = c1.el;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// https://en.wikipedia.org/wiki/Longest_increasing_subsequence
// [ts] fn getSequence(arr: auto) {
// [ts] const p = arr.slice();
// [ts] const result = [0];
// [ts] let i, j, u, v, c;
// [ts] const len = arr.length;
// [ts] i = 0;
// [ts] while (i < len) { // [cap-migrate] for-step: i++
// [ts] const arrI = arr[i];
// [ts] if (arrI != 0) {
// [ts] j = result[result.length - 1];
// [ts] if (arr[j] < arrI) {
// [ts] p[i] = j;
// [ts] result.push(i);
// [ts] continue;
// [ts] }
// [ts] u = 0;
// [ts] v = result.length - 1;
// [ts] while (u < v) {
// [ts] c = ((u + v) / 2) | 0;
// [ts] if (arr[result[c]] < arrI) {
// [ts] u = c + 1;
// [ts] }
// [ts] else {
// [ts] v = c;
// [ts] }
// [ts] }
// [ts] if (arrI < arr[result[u]]) {
// [ts] if (u > 0) {
// [ts] p[i] = result[u - 1];
// [ts] }
// [ts] result[u] = i;
// [ts] }
// [ts] }
// [ts] }
// [ts] u = result.length;
// [ts] v = result[u - 1];
// [ts] while (u-- > 0) {
// [ts] result[u] = v;
// [ts] v = p[v];
// [ts] }
// [ts] return result;
// [ts] }
// [ts] fn isTeleport(type: auto) { return type.__isTeleport; }
// [ts] fn isTeleportDisabled(props: auto) { return props && (props.disabled || props.disabled == ""); }
// [ts] fn isTargetSVG(target: auto) { return @typeName(SVGElement) != "undefined" && target == SVGElement; }
// [ts] fn resolveTarget(props: auto, select: auto) {
// [ts] const targetSelector = props && props.to;
// [ts] if (isString(targetSelector)) {
// [ts] if (!select) {
// [ts] warn(`Current renderer does not support string target for Teleports. ` +
// [ts] `(missing querySelector renderer option)`);
// [ts] return null;
// [ts] }
// [ts] else {
// [ts] const target = select(targetSelector);
// [ts] if (!target) {
// [ts] warn(`Failed to locate Teleport target with selector "${targetSelector}". ` +
// [ts] `Note the target element must exist before the component is mounted - ` +
// [ts] `i.e. the target cannot be rendered by the component itself, and ` +
// [ts] `ideally should be outside of the entire Vue component tree.`);
// [ts] }
// [ts] return target;
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (!targetSelector && !isTeleportDisabled(props)) {
// [ts] warn(`Invalid Teleport target: ${targetSelector}`);
// [ts] }
// [ts] return targetSelector;
// [ts] }
// [ts] };
// [ts] const TeleportImpl = {
// [ts] __isTeleport: true,
// [ts] process: (n1, n2, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized, internals) => {
// [ts] const { mc: mountChildren, pc: patchChildren, pbc: patchBlockChildren, o: { insert, querySelector, createText, createComment } } = internals;
// [ts] const disabled = isTeleportDisabled(n2.props);
// [ts] let { shapeFlag, children, dynamicChildren } = n2;
// #3302
// HMR updated, force full diff
// [ts] if (isHmrUpdating) {
// [ts] optimized = false;
// [ts] dynamicChildren = null;
// [ts] }
// [ts] if (n1 == null) {
// insert anchors in the main view
// [ts] const placeholder = (n2.el = createComment("teleport start")
// [ts] );
// [ts] const mainAnchor = (n2.anchor = createComment("teleport end")
// [ts] );
// [ts] insert(placeholder, container, anchor);
// [ts] insert(mainAnchor, container, anchor);
// [ts] const target = (n2.target = resolveTarget(n2.props, querySelector));
// [ts] const targetAnchor = (n2.targetAnchor = createText(""));
// [ts] if (target) {
// [ts] insert(targetAnchor, target);
// #2652 we could be teleporting from a non-SVG tree into an SVG tree
// [ts] isSVG = isSVG || isTargetSVG(target);
// [ts] }
// [ts] else if (!disabled) {
// [ts] warn("Invalid Teleport target on mount:", target, `(${@typeName(target)})`);
// [ts] }
// [ts] const mount = (container, anchor) => {
// Teleport *always* has Array children. This is enforced in both the
// compiler and vnode children normalization.
// [ts] if (shapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] mountChildren(children, container, anchor, parentComponent, parentSuspense, isSVG, slotScopeIds, optimized);
// [ts] }
// [ts] };
// [ts] if (disabled) {
// [ts] mount(container, mainAnchor);
// [ts] }
// [ts] else if (target) {
// [ts] mount(target, targetAnchor);
// [ts] }
// [ts] }
// [ts] else {
// update content
// [ts] n2.el = n1.el;
// [ts] const mainAnchor = (n2.anchor = n1.anchor);
// [ts] const target = (n2.target = n1.target);
// [ts] const targetAnchor = (n2.targetAnchor = n1.targetAnchor);
// [ts] const wasDisabled = isTeleportDisabled(n1.props);
// [ts] const currentContainer = if (wasDisabled) container else target;
// [ts] const currentAnchor = if (wasDisabled) mainAnchor else targetAnchor;
// [ts] isSVG = isSVG || isTargetSVG(target);
// [ts] if (dynamicChildren) {
// fast path when the teleport happens to be a block root
// [ts] patchBlockChildren(n1.dynamicChildren, dynamicChildren, currentContainer, parentComponent, parentSuspense, isSVG, slotScopeIds);
// even in block tree mode we need to make sure all root-level nodes
// in the teleport inherit previous DOM references so that they can
// be moved in future patches.
// [ts] traverseStaticChildren(n1, n2, true);
// [ts] }
// [ts] else if (!optimized) {
// [ts] patchChildren(n1, n2, currentContainer, currentAnchor, parentComponent, parentSuspense, isSVG, slotScopeIds, false);
// [ts] }
// [ts] if (disabled) {
// [ts] if (!wasDisabled) {
// enabled -> disabled
// move into main container
// [ts] moveTeleport(n2, container, mainAnchor, internals, 1 /* TOGGLE */);
// [ts] }
// [ts] }
// [ts] else {
// target changed
// [ts] if ((n2.props && n2.props.to) != (n1.props && n1.props.to)) {
// [ts] const nextTarget = (n2.target = resolveTarget(n2.props, querySelector));
// [ts] if (nextTarget) {
// [ts] moveTeleport(n2, nextTarget, null, internals, 0 /* TARGET_CHANGE */);
// [ts] }
// [ts] else {
// [ts] warn("Invalid Teleport target on update:", target, `(${@typeName(target)})`);
// [ts] }
// [ts] }
// [ts] else if (wasDisabled) {
// disabled -> enabled
// move into teleport target
// [ts] moveTeleport(n2, target, targetAnchor, internals, 1 /* TOGGLE */);
// [ts] }
// [ts] }
// [ts] }
// [ts] },
// [ts] remove: (vnode, parentComponent, parentSuspense, optimized, _p, doRemove) => {
// [ts] const { shapeFlag, children, anchor, targetAnchor, target, props } = vnode;
// [ts] if (target) {
// [ts] hostRemove(targetAnchor);
// [ts] }
// an unmounted teleport should always remove its children if not disabled
// [ts] if (doRemove || !isTeleportDisabled(props)) {
// [ts] hostRemove(anchor);
// [ts] if (shapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// [ts] unmount(child, parentComponent, parentSuspense, true, !!child.dynamicChildren);
// [ts] }
// [ts] }
// [ts] }
// [ts] },
// [ts] move: moveTeleport,
// [ts] hydrate: hydrateTeleport
// [ts] };
// [ts] fn moveTeleport(vnode: auto, container: auto, parentAnchor: auto, _param: string, moveType: auto = 2 /* REORDER */) {
// move target anchor if this is a target change.
// [ts] if (moveType == 0 /* TARGET_CHANGE */) {
// [ts] insert(vnode.targetAnchor, container, parentAnchor);
// [ts] }
// [ts] const { el, anchor, shapeFlag, children, props } = vnode;
// [ts] const isReorder = moveType == 2 /* REORDER */;
// move main view anchor if this is a re-order.
// [ts] if (isReorder) {
// [ts] insert(el, container, parentAnchor);
// [ts] }
// if this is a re-order and teleport is enabled (content is in target)
// do not move children. So the opposite is: only move children if this
// is not a reorder, or the teleport is disabled
// [ts] if (!isReorder || isTeleportDisabled(props)) {
// Teleport has either Array children or no children.
// [ts] if (shapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] move(children[i], container, parentAnchor, 2 /* REORDER */);
// [ts] }
// [ts] }
// [ts] }
// move main view anchor if this is a re-order.
// [ts] if (isReorder) {
// [ts] insert(anchor, container, parentAnchor);
// [ts] }
// [ts] }
// [ts] fn hydrateTeleport(node: auto, vnode: auto, parentComponent: auto, parentSuspense: auto, slotScopeIds: auto, optimized: auto, _param: string, hydrateChildren: auto) {
// [ts] const target = (vnode.target = resolveTarget(vnode.props, querySelector));
// [ts] if (target) {
// if multiple teleports rendered to the same target element, we need to
// pick up from where the last teleport finished instead of the first node
// [ts] const targetNode = target._lpa || target.firstChild;
// [ts] if (vnode.shapeFlag & 16 /* ARRAY_CHILDREN */) {
// [ts] if (isTeleportDisabled(vnode.props)) {
// [ts] vnode.anchor = hydrateChildren(nextSibling(node), vnode, parentNode(node), parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] vnode.targetAnchor = targetNode;
// [ts] }
// [ts] else {
// [ts] vnode.anchor = nextSibling(node);
// [ts] vnode.targetAnchor = hydrateChildren(targetNode, vnode, target, parentComponent, parentSuspense, slotScopeIds, optimized);
// [ts] }
// [ts] target._lpa =
// [ts] vnode.targetAnchor && nextSibling(vnode.targetAnchor);
// [ts] }
// [ts] }
// [ts] return vnode.anchor && nextSibling(vnode.anchor);
// [ts] }
// Force-casted public typing for h and TSX props inference
// [ts] const Teleport = TeleportImpl;
// [ts] const COMPONENTS = "components";
// [ts] const DIRECTIVES = "directives";
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn resolveComponent(name: auto, maybeSelfReference: auto) {
// [ts] return resolveAsset(COMPONENTS, name, true, maybeSelfReference) || name;
// [ts] }
// [ts] const NULL_DYNAMIC_COMPONENT = "__symbol";
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn resolveDynamicComponent(component: auto) {
// [ts] if (isString(component)) {
// [ts] return resolveAsset(COMPONENTS, component, false) || component;
// [ts] }
// [ts] else {
// invalid types will fallthrough to createVNode and raise warning
// [ts] return (component || NULL_DYNAMIC_COMPONENT);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn resolveDirective(name: auto) {
// [ts] return resolveAsset(DIRECTIVES, name);
// [ts] }
// implementation
// [ts] fn resolveAsset(type: auto, name: auto, warnMissing: auto = true, maybeSelfReference: auto = false) {
// [ts] const instance = currentRenderingInstance || currentInstance;
// [ts] if (instance) {
// [ts] const Component = instance.type;
// explicit self name has highest priority
// [ts] if (type == COMPONENTS) {
// [ts] const selfName = getComponentName(Component);
// [ts] if (selfName &&
// [ts] (selfName == name ||
// [ts] selfName == camelize(name) ||
// [ts] selfName == capitalize(camelize(name)))) {
// [ts] return Component;
// [ts] }
// [ts] }
// [ts] const res =
// local registration
// check instance[type] first which is resolved for options API
// [ts] resolve(instance[type] || Component[type], name) ||
// global registration
// [ts] resolve(instance.appContext[type], name);
// [ts] if (!res && maybeSelfReference) {
// fallback to implicit self-reference
// [ts] return Component;
// [ts] }
// [ts] if (warnMissing && !res) {
// [ts] warn(`Failed to resolve ${type.slice(0, -1)}: ${name}`);
// [ts] }
// [ts] return res;
// [ts] }
// [ts] else {
// [ts] warn(`resolve${capitalize(type.slice(0, -1))} ` +
// [ts] `can only be used in render() or setup().`);
// [ts] }
// [ts] }
// [ts] fn resolve(registry: auto, name: auto) {
// [ts] return (registry &&
// [ts] (registry[name] ||
// [ts] registry[camelize(name)] ||
// [ts] registry[capitalize(camelize(name))]));
// [ts] }
// [ts] const Fragment = "__symbol_Fragment";
// [ts] const Text = "__symbol_Text";
// [ts] const Comment$1 = "__symbol_Comment";
// [ts] const Static = "__symbol_Static";
// Since v-if and v-for are the two possible ways node structure can dynamically
// change, once we consider v-if branches and each v-for fragment a block, we
// can divide a template into nested blocks, and within each block the node
// structure would be stable. This allows us to skip most children diffing
// and only worry about the dynamic nodes (indicated by patch flags).
// [ts] const blockStack = [];
// [ts] let currentBlock = null;
// [ts] /**
// [ts] * Open a block.
// [ts] * This must be called before `createBlock`. It cannot be part of `createBlock`
// [ts] * because the children of the block are evaluated before `createBlock` itself
// [ts] * is called. The generated code typically looks like this:
// [ts] *
// [ts] * ```js
// [ts] * function render() {
// [ts] *   return (openBlock(),createBlock('div', null, [...]))
// [ts] * }
// [ts] * ```
// [ts] * disableTracking is true when creating a v-for fragment block, since a v-for
// [ts] * fragment always diffs its children.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] fn openBlock(disableTracking: auto = false) {
// [ts] blockStack.push((currentBlock = if (disableTracking) null else []));
// [ts] }
// [ts] fn closeBlock() {
// [ts] blockStack.pop();
// [ts] currentBlock = blockStack[blockStack.length - 1] || null;
// [ts] }
// Whether we should be tracking dynamic child nodes inside a block.
// Only tracks when this value is > 0
// We are not using a simple boolean because this value may need to be
// incremented/decremented by nested usage of v-once (see below)
// [ts] let isBlockTreeEnabled = 1;
// [ts] /**
// [ts] * Block tracking sometimes needs to be disabled, for example during the
// [ts] * creation of a tree that needs to be cached by v-once. The compiler generates
// [ts] * code like this:
// [ts] *
// [ts] * ``` js
// [ts] * _cache[1] || (
// [ts] *   setBlockTracking(-1),
// [ts] *   _cache[1] = createVNode(...),
// [ts] *   setBlockTracking(1),
// [ts] *   _cache[1]
// [ts] * )
// [ts] * ```
// [ts] *
// [ts] * @private
// [ts] */
// [ts] fn setBlockTracking(value: auto) {
// [ts] isBlockTreeEnabled += value;
// [ts] }
// [ts] /**
// [ts] * Create a block root vnode. Takes the same exact arguments as `createVNode`.
// [ts] * A block root keeps track of dynamic nodes within the block in the
// [ts] * `dynamicChildren` array.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] fn createBlock(type: auto, props: auto, children: auto, patchFlag: auto, dynamicProps: auto) {
// [ts] const vnode = createVNode(type, props, children, patchFlag, dynamicProps, true /* isBlock: prevent a block from tracking itself */);
// save current block children on the block vnode
// [ts] vnode.dynamicChildren =
// [ts] if (isBlockTreeEnabled > 0) currentBlock || EMPTY_ARR else null;
// close block
// [ts] closeBlock();
// a block is always going to be patched, so track it as a child of its
// parent block
// [ts] if (isBlockTreeEnabled > 0 && currentBlock) {
// [ts] currentBlock.push(vnode);
// [ts] }
// [ts] return vnode;
// [ts] }
// [ts] fn isVNode(value: auto) {
// [ts] return if (value) value.__v_isVNode == true else false;
// [ts] }
// [ts] fn isSameVNodeType(n1: auto, n2: auto) {
// [ts] if (n2.shapeFlag & 6 /* COMPONENT */ &&
// [ts] hmrDirtyComponents.has(n2.type)) {
// HMR only: if the component has been hot-updated, force a reload.
// [ts] return false;
// [ts] }
// [ts] return n1.type == n2.type && n1.key == n2.key;
// [ts] }
// [ts] let vnodeArgsTransformer;
// [ts] /**
// [ts] * Internal API for registering an arguments transform for createVNode
// [ts] * used for creating stubs in the test-utils
// [ts] * It is *internal* but needs to be exposed for test-utils to pick up proper
// [ts] * typings
// [ts] */
// [ts] fn transformVNodeArgs(transformer: auto) {
// [ts] vnodeArgsTransformer = transformer;
// [ts] }
// [ts] fn createVNodeWithArgsTransform(args: auto) {
// [ts] return _createVNode(...(if (vnodeArgsTransformer) vnodeArgsTransformer(args, currentRenderingInstance) else args));
// [ts] };
// [ts] const InternalObjectKey = `__vInternal`;
// [ts] fn normalizeKey(_param: string) { return if (key != null) key else null; }
// [ts] fn normalizeRef(_param: string) {
// [ts] return (if (ref != null) if (isString(ref) || isRef(ref) || isFunction(ref)) { i: currentRenderingInstance, r: ref } else ref else null);
// [ts] };
// [ts] const createVNode = (createVNodeWithArgsTransform
// [ts] );
// [ts] fn _createVNode(type: auto, props: auto = null, children: auto = null, patchFlag: auto = 0, dynamicProps: auto = null, isBlockNode: auto = false) {
// [ts] if (!type || type == NULL_DYNAMIC_COMPONENT) {
// [ts] if (!type) {
// [ts] warn(`Invalid vnode type when creating vnode: ${type}.`);
// [ts] }
// [ts] type = Comment$1;
// [ts] }
// [ts] if (isVNode(type)) {
// createVNode receiving an existing vnode. This happens in cases like
// <component :is="vnode"/>
// #2078 make sure to merge refs during the clone instead of overwriting it
// [ts] const cloned = cloneVNode(type, props, true /* mergeRef: true */);
// [ts] if (children) {
// [ts] normalizeChildren(cloned, children);
// [ts] }
// [ts] return cloned;
// [ts] }
// class component normalization.
// [ts] if (isClassComponent(type)) {
// [ts] type = type.__vccOpts;
// [ts] }
// class & style normalization.
// [ts] if (props) {
// for reactive or proxy objects, we need to clone it to enable mutation.
// [ts] if (isProxy(props) || InternalObjectKey == props) {
// [ts] props = extend({}, props);
// [ts] }
// [ts] let { class: klass, style } = props;
// [ts] if (klass && !isString(klass)) {
// [ts] props.class = normalizeClass(klass);
// [ts] }
// [ts] if (isObject(style)) {
// reactive state objects need to be cloned since they are likely to be
// mutated
// [ts] if (isProxy(style) && !isArray(style)) {
// [ts] style = extend({}, style);
// [ts] }
// [ts] props.style = normalizeStyle(style);
// [ts] }
// [ts] }
// encode the vnode type information into a bitmap
// [ts] const shapeFlag = if (isString(type)) 1 /* ELEMENT */ else if (isSuspense(type)) 128 /* SUSPENSE */ else if (isTeleport(type)) 64 /* TELEPORT */ else if (isObject(type)) 4 /* STATEFUL_COMPONENT */ else if (isFunction(type)) 2 /* FUNCTIONAL_COMPONENT */ else 0;
// [ts] if (shapeFlag & 4 /* STATEFUL_COMPONENT */ && isProxy(type)) {
// [ts] type = toRaw(type);
// [ts] warn(`Vue received a Component which was made a reactive object. This can ` +
// [ts] `lead to unnecessary performance overhead, and should be avoided by ` +
// [ts] `marking the component with \`markRaw\` or using \`shallowRef\` ` +
// [ts] `instead of \`ref\`.`, `\nComponent that was made reactive: `, type);
// [ts] }
// [ts] const vnode = {
// [ts] __v_isVNode: true,
// [ts] __v_skip: true,
// [ts] type,
// [ts] props,
// [ts] key: props && normalizeKey(props),
// [ts] ref: props && normalizeRef(props),
// [ts] scopeId: currentScopeId,
// [ts] slotScopeIds: null,
// [ts] children: null,
// [ts] component: null,
// [ts] suspense: null,
// [ts] ssContent: null,
// [ts] ssFallback: null,
// [ts] dirs: null,
// [ts] transition: null,
// [ts] el: null,
// [ts] anchor: null,
// [ts] target: null,
// [ts] targetAnchor: null,
// [ts] shapeFlag,
// [ts] patchFlag,
// [ts] dynamicProps,
// [ts] dynamicChildren: null,
// [ts] appContext: null
// [ts] };
// validate key
// [ts] if (vnode.key != vnode.key) {
// [ts] warn(`VNode created with invalid key (NaN). VNode type:`, vnode.type);
// [ts] }
// [ts] normalizeChildren(vnode, children);
// normalize suspense children
// [ts] if (shapeFlag & 128 /* SUSPENSE */) {
// [ts] type.normalize(vnode);
// [ts] }
// [ts] if (isBlockTreeEnabled > 0 &&
// avoid a block node from tracking itself
// [ts] !isBlockNode &&
// has current parent block
// [ts] currentBlock &&
// presence of a patch flag indicates this node needs patching on updates.
// component nodes also should always be patched, because even if the
// component doesn't need to update, it needs to persist the instance on to
// the next vnode so that it can be properly unmounted later.
// [ts] (patchFlag > 0 || shapeFlag & 6 /* COMPONENT */) &&
// the EVENTS flag is only for hydration and if it is the only flag, the
// vnode should not be considered dynamic due to handler caching.
// [ts] patchFlag != 32 /* HYDRATE_EVENTS */) {
// [ts] currentBlock.push(vnode);
// [ts] }
// [ts] return vnode;
// [ts] }
// [ts] fn cloneVNode(vnode: auto, extraProps: auto, mergeRef: auto = false) {
// This is intentionally NOT using spread or extend to avoid the runtime
// key enumeration cost.
// [ts] const { props, ref, patchFlag, children } = vnode;
// [ts] const mergedProps = if (extraProps) mergeProps(props || {}, extraProps) else props;
// [ts] const cloned = {
// [ts] __v_isVNode: true,
// [ts] __v_skip: true,
// [ts] type: vnode.type,
// [ts] props: mergedProps,
// [ts] key: mergedProps && normalizeKey(mergedProps),
// [ts] ref: extraProps && extraProps.ref ? // #2078 in the case of <component :is="vnode" ref="extra"/>
// if the vnode itself already has a ref, cloneVNode will need to merge
// the refs so the single vnode can be set on multiple refs
// [ts] if (mergeRef && ref) if (isArray(ref)) ref.concat(normalizeRef(extraProps)) else [ref, normalizeRef(extraProps)] else normalizeRef(extraProps) : ref,
// [ts] scopeId: vnode.scopeId,
// [ts] slotScopeIds: vnode.slotScopeIds,
// [ts] children: if (patchFlag == -1 /* HOISTED */ && isArray(children)) children.map(deepCloneVNode) else children,
// [ts] target: vnode.target,
// [ts] targetAnchor: vnode.targetAnchor,
// [ts] staticCount: vnode.staticCount,
// [ts] shapeFlag: vnode.shapeFlag,
// if the vnode is cloned with extra props, we can no longer assume its
// existing patch flag to be reliable and need to add the FULL_PROPS flag.
// note: perserve flag for fragments since they use the flag for children
// fast paths only.
// [ts] patchFlag: extraProps && vnode.type != Fragment ? patchFlag == -1 // hoisted node ? 16 /* FULL_PROPS */ : patchFlag | 16 /* FULL_PROPS */ : patchFlag,
// [ts] dynamicProps: vnode.dynamicProps,
// [ts] dynamicChildren: vnode.dynamicChildren,
// [ts] appContext: vnode.appContext,
// [ts] dirs: vnode.dirs,
// [ts] transition: vnode.transition,
// These should technically only be non-null on mounted VNodes. However,
// they *should* be copied for kept-alive vnodes. So we just always copy
// them since them being non-null during a mount doesn't affect the logic as
// they will simply be overwritten.
// [ts] component: vnode.component,
// [ts] suspense: vnode.suspense,
// [ts] ssContent: vnode.ssContent && cloneVNode(vnode.ssContent),
// [ts] ssFallback: vnode.ssFallback && cloneVNode(vnode.ssFallback),
// [ts] el: vnode.el,
// [ts] anchor: vnode.anchor
// [ts] };
// [ts] return cloned;
// [ts] }
// [ts] /**
// [ts] * Dev only, for HMR of hoisted vnodes reused in v-for
// [ts] * https://github.com/vitejs/vite/issues/2022
// [ts] */
// [ts] fn deepCloneVNode(vnode: auto) {
// [ts] const cloned = cloneVNode(vnode);
// [ts] if (isArray(vnode.children)) {
// [ts] cloned.children = vnode.children.map(deepCloneVNode);
// [ts] }
// [ts] return cloned;
// [ts] }
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn createTextVNode(text: auto = " ", flag: auto = 0) {
// [ts] return createVNode(Text, null, text, flag);
// [ts] }
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn createStaticVNode(content: auto, numberOfNodes: auto) {
// A static vnode can contain multiple stringified elements, and the number
// of elements is necessary for hydration.
// [ts] const vnode = createVNode(Static, null, content);
// [ts] vnode.staticCount = numberOfNodes;
// [ts] return vnode;
// [ts] }
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn createCommentVNode(text: auto = "", // when used as the v-else branch: auto, the comment node must be created as a // block to ensure correct updates. asBlock: auto = false) {
// [ts] return if (asBlock) (openBlock(), createBlock(Comment$1, null, text)) else createVNode(Comment$1, null, text);
// [ts] }
// [ts] fn normalizeVNode(child: auto) {
// [ts] if (child == null || @typeName(child) == "boolean") {
// empty placeholder
// [ts] return createVNode(Comment$1);
// [ts] }
// [ts] else if (isArray(child)) {
// fragment
// [ts] return createVNode(Fragment, null,
// #3666, avoid reference pollution when reusing vnode
// [ts] child.slice());
// [ts] }
// [ts] else if (@typeName(child) == "object") {
// already vnode, this should be the most common since compiled templates
// always produce all-vnode children arrays
// [ts] return cloneIfMounted(child);
// [ts] }
// [ts] else {
// strings and numbers
// [ts] return createVNode(Text, null, String(child));
// [ts] }
// [ts] }
// optimized normalization for template-compiled render fns
// [ts] fn cloneIfMounted(child: auto) {
// [ts] return if (child.el == null) child else cloneVNode(child);
// [ts] }
// [ts] fn normalizeChildren(vnode: auto, children: auto) {
// [ts] let type = 0;
// [ts] const { shapeFlag } = vnode;
// [ts] if (children == null) {
// [ts] children = null;
// [ts] }
// [ts] else if (isArray(children)) {
// [ts] type = 16 /* ARRAY_CHILDREN */;
// [ts] }
// [ts] else if (@typeName(children) == "object") {
// [ts] if (shapeFlag & 1 /* ELEMENT */ || shapeFlag & 64 /* TELEPORT */) {
// Normalize slot to plain children for plain element and Teleport
// [ts] const slot = children.default;
// [ts] if (slot) {
// _c marker is added by withCtx() indicating this is a compiled slot
// [ts] slot._c && (slot._d = false);
// [ts] normalizeChildren(vnode, slot());
// [ts] slot._c && (slot._d = true);
// [ts] }
// [ts] return;
// [ts] }
// [ts] else {
// [ts] type = 32 /* SLOTS_CHILDREN */;
// [ts] const slotFlag = children._;
// [ts] if (!slotFlag && !(InternalObjectKey == children)) {
// [ts] children._ctx = currentRenderingInstance;
// [ts] }
// [ts] else if (slotFlag == 3 /* FORWARDED */ && currentRenderingInstance) {
// a child component receives forwarded slots from the parent.
// its slot type is determined by its parent's slot type.
// [ts] if (currentRenderingInstance.slots._ == 1 /* STABLE */) {
// [ts] children._ = 1 /* STABLE */;
// [ts] }
// [ts] else {
// [ts] children._ = 2 /* DYNAMIC */;
// [ts] vnode.patchFlag |= 1024 /* DYNAMIC_SLOTS */;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (isFunction(children)) {
// [ts] children = { default: children, _ctx: currentRenderingInstance };
// [ts] type = 32 /* SLOTS_CHILDREN */;
// [ts] }
// [ts] else {
// [ts] children = String(children);
// force teleport children to array so it can be moved around
// [ts] if (shapeFlag & 64 /* TELEPORT */) {
// [ts] type = 16 /* ARRAY_CHILDREN */;
// [ts] children = [createTextVNode(children)];
// [ts] }
// [ts] else {
// [ts] type = 8 /* TEXT_CHILDREN */;
// [ts] }
// [ts] }
// [ts] vnode.children = children;
// [ts] vnode.shapeFlag |= type;
// [ts] }
// [ts] fn mergeProps(args: auto) {
// [ts] const ret = extend({}, args[0]);
// [ts] let i = 1;
// [ts] while (i < args.length) { // [cap-migrate] for-step: i++
// [ts] const toMerge = args[i];
// [ts] for (const key in toMerge) {
// [ts] if (key == "class") {
// [ts] if (ret.class != toMerge.class) {
// [ts] ret.class = normalizeClass([ret.class, toMerge.class]);
// [ts] }
// [ts] }
// [ts] else if (key == "style") {
// [ts] ret.style = normalizeStyle([ret.style, toMerge.style]);
// [ts] }
// [ts] else if (isOn(key)) {
// [ts] const existing = ret[key];
// [ts] const incoming = toMerge[key];
// [ts] if (existing != incoming) {
// [ts] ret[key] = if (existing) [].concat(existing, incoming) else incoming;
// [ts] }
// [ts] }
// [ts] else if (key != "") {
// [ts] ret[key] = toMerge[key];
// [ts] }
// [ts] }
// [ts] }
// [ts] return ret;
// [ts] }
// [ts] /**
// [ts] * Actual implementation
// [ts] */
// [ts] fn renderList(source: auto, renderItem: auto) {
// [ts] let ret;
// [ts] if (isArray(source) || isString(source)) {
// [ts] ret = Array.init(source.length);
// [ts] let i = 0, l = source.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] ret[i] = renderItem(source[i], i);
// [ts] }
// [ts] }
// [ts] else if (@typeName(source) == "number") {
// [ts] if (!Number.isInteger(source)) {
// [ts] warn(`The v-for range expect an integer value but got ${source}.`);
// [ts] return [];
// [ts] }
// [ts] ret = Array.init(source);
// [ts] let i = 0;
// [ts] while (i < source) { // [cap-migrate] for-step: i++
// [ts] ret[i] = renderItem(i + 1, i);
// [ts] }
// [ts] }
// [ts] else if (isObject(source)) {
// [ts] if (source["__symbol_iterator"]) {
// [ts] ret = Array.from(source, renderItem);
// [ts] }
// [ts] else {
// [ts] const keys = Object.keys(source);
// [ts] ret = Array.init(keys.length);
// [ts] let i = 0, l = keys.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] const key = keys[i];
// [ts] ret[i] = renderItem(source[key], key, i);
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] ret = [];
// [ts] }
// [ts] return ret;
// [ts] }
// [ts] /**
// [ts] * Compiler runtime helper for creating dynamic slots object
// [ts] * @private
// [ts] */
// [ts] fn createSlots(slots: auto, dynamicSlots: auto) {
// [ts] let i = 0;
// [ts] while (i < dynamicSlots.length) { // [cap-migrate] for-step: i++
// [ts] const slot = dynamicSlots[i];
// array of dynamic slot generated by <template v-for="..." #[...]>
// [ts] if (isArray(slot)) {
// [ts] let j = 0;
// [ts] while (j < slot.length) { // [cap-migrate] for-step: j++
// [ts] slots[slot[j].name] = slot[j].fn;
// [ts] }
// [ts] }
// [ts] else if (slot) {
// conditional single slot generated by <template v-if="..." #foo>
// [ts] slots[slot.name] = slot.fn;
// [ts] }
// [ts] }
// [ts] return slots;
// [ts] }
// [ts] /**
// [ts] * Compiler runtime helper for rendering `<slot/>`
// [ts] * @private
// [ts] */
// [ts] fn renderSlot(slots: auto, name: auto, props: auto = {}, // this is not a user-facing function: auto, so the fallback is always generated by // the compiler and guaranteed to be a function returning an array fallback: auto, noSlotted: auto) {
// [ts] let slot = slots[name];
// [ts] if (slot && slot.length > 1) {
// [ts] warn(`SSR-optimized slot function detected in a non-SSR-optimized render ` +
// [ts] `function. You need to mark this component with $dynamic-slots in the ` +
// [ts] `parent template.`);
// [ts] slot = () => [];
// [ts] }
// a compiled slot disables block tracking by default to avoid manual
// invocation interfering with template-based block tracking, but in
// `renderSlot` we can be sure that it's template-based so we can force
// enable it.
// [ts] if (slot && slot._c) {
// [ts] slot._d = false;
// [ts] }
// [ts] openBlock();
// [ts] const validSlotContent = slot && ensureValidVNode(slot(props));
// [ts] const rendered = createBlock(if (Fragment, { key: props.key || `_${name}` }, validSlotContent || (if (fallback) fallback() else []), validSlotContent && slots._ == 1 /* STABLE */) 64 /* STABLE_FRAGMENT */ else -2 /* BAIL */);
// [ts] if (!noSlotted && rendered.scopeId) {
// [ts] rendered.slotScopeIds = [rendered.scopeId + "-s"];
// [ts] }
// [ts] if (slot && slot._c) {
// [ts] slot._d = true;
// [ts] }
// [ts] return rendered;
// [ts] }
// [ts] fn ensureValidVNode(vnodes: auto) {
// [ts] return vnodes.some(child => {
// [ts] if (!isVNode(child))
// [ts] return true;
// [ts] if (child.type == Comment$1)
// [ts] return false;
// [ts] if (child.type == Fragment &&
// [ts] !ensureValidVNode(child.children))
// [ts] return false;
// [ts] return true;
// [ts] }) ? vnodes : null;
// [ts] }
// [ts] /**
// [ts] * For prefixing keys in v-on="obj" with "on"
// [ts] * @private
// [ts] */
// [ts] fn toHandlers(obj: auto) {
// [ts] const ret = {};
// [ts] if (!isObject(obj)) {
// [ts] warn(`v-on with no argument expects an object value.`);
// [ts] return ret;
// [ts] }
// [ts] for (const key in obj) {
// [ts] ret[toHandlerKey(key)] = obj[key];
// [ts] }
// [ts] return ret;
// [ts] }
// [ts] /**
// [ts] * #2437 In Vue 3, functional components do not have a public instance proxy but
// [ts] * they exist in the internal parent chain. For code that relies on traversing
// [ts] * public $parent chains, skip functional ones and go to the parent instead.
// [ts] */
// [ts] fn getPublicInstance(i: auto) {
// [ts] if (!i)
// [ts] return null;
// [ts] if (isStatefulComponent(i))
// [ts] return getExposeProxy(i) || i.proxy;
// [ts] return getPublicInstance(i.parent);
// [ts] };
// [ts] const publicPropertiesMap = extend(Object.create(null), {
// [ts] $: i => i,
// [ts] $el: i => i.vnode.el,
// [ts] $data: i => i.data,
// [ts] $props: i => (shallowReadonly(i.props) ),
// [ts] $attrs: i => (shallowReadonly(i.attrs) ),
// [ts] $slots: i => (shallowReadonly(i.slots) ),
// [ts] $refs: i => (shallowReadonly(i.refs) ),
// [ts] $parent: i => getPublicInstance(i.parent),
// [ts] $root: i => getPublicInstance(i.root),
// [ts] $emit: i => i.emit,
// [ts] $options: i => (resolveMergedOptions(i) ),
// [ts] $forceUpdate: i => () => queueJob(i.update),
// [ts] $nextTick: i => nextTick.bind(i.proxy),
// [ts] $watch: i => (instanceWatch.bind(i) )
// [ts] });
// [ts] const PublicInstanceProxyHandlers = {
// [ts] get: (_p, key) => {
// [ts] const { ctx, setupState, data, props, accessCache, type, appContext } = instance;
// for internal formatters to know that this is a Vue instance
// [ts] if (key == "__isVue") {
// [ts] return true;
// [ts] }
// prioritize <script setup> bindings during dev.
// this allows even properties that start with _ or $ to be used - so that
// it aligns with the production behavior where the render fn is inlined and
// indeed has access to all declared variables.
// [ts] if (setupState != EMPTY_OBJ &&
// [ts] setupState.__isScriptSetup &&
// [ts] hasOwn: (setupState, key) => {
// [ts] return setupState[key];
// [ts] }
// data / props / ctx
// This getter gets called for every property access on the render context
// during render and is a major hotspot. The most expensive part of this
// is the multiple hasOwn() calls. It's much faster to do a simple property
// access on a plain object, so we use an accessCache object (with null
// prototype) to memoize what access type a key corresponds to.
// [ts] let normalizedProps;
// [ts] if (key[0] != "$") {
// [ts] const n = accessCache[key];
// [ts] if (n != null) {
// [ts] switch (n) {
// [ts] 0 /* SETUP */ => {
// [ts] return setupState[key];
// [ts] },
// [ts] 1 /* DATA */ => {
// [ts] return data[key];
// [ts] },
// [ts] 3 /* CONTEXT */ => {
// [ts] return ctx[key];
// [ts] },
// [ts] 2 /* PROPS */ => {
// [ts] return props[key];
// default: just fallthrough
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (setupState != EMPTY_OBJ && hasOwn(setupState, key)) {
// [ts] accessCache[key] = 0 /* SETUP */;
// [ts] return setupState[key];
// [ts] }
// [ts] else if (data != EMPTY_OBJ && hasOwn(data, key)) {
// [ts] accessCache[key] = 1 /* DATA */;
// [ts] return data[key];
// [ts] }
// [ts] else if (
// only cache other properties when instance has declared (thus stable)
// props
// [ts] (normalizedProps = instance.propsOptions[0]) &&
// [ts] hasOwn: (normalizedProps, key) => {
// [ts] accessCache[key] = 2 /* PROPS */;
// [ts] return props[key];
// [ts] }
// [ts] else if (ctx != EMPTY_OBJ && hasOwn(ctx, key)) {
// [ts] accessCache[key] = 3 /* CONTEXT */;
// [ts] return ctx[key];
// [ts] }
// [ts] else if (shouldCacheAccess) {
// [ts] accessCache[key] = 4 /* OTHER */;
// [ts] }
// [ts] }
// [ts] const publicGetter = publicPropertiesMap[key];
// [ts] let cssModule, globalProperties;
// public $xxx properties
// [ts] if (publicGetter) {
// [ts] if (key == "$attrs") {
// [ts] track(instance, "get" /* GET */, key);
// [ts] markAttrsAccessed();
// [ts] }
// [ts] return publicGetter(instance);
// [ts] }
// [ts] else if (
// css module (injected by vue-loader)
// [ts] (cssModule = type.__cssModules) &&
// [ts] (cssModule = cssModule[key])) {
// [ts] return cssModule;
// [ts] }
// [ts] else if (ctx != EMPTY_OBJ && hasOwn(ctx, key)) {
// user may set custom properties to `this` that start with `$`
// [ts] accessCache[key] = 3 /* CONTEXT */;
// [ts] return ctx[key];
// [ts] }
// [ts] else if (
// global properties
// [ts] ((globalProperties = appContext.config.globalProperties),
// [ts] hasOwn: (globalProperties, key) => {
// [ts] {
// [ts] return globalProperties[key];
// [ts] }
// [ts] }
// [ts] else if (currentRenderingInstance &&
// [ts] (!isString(key) ||
// #1091 avoid internal isRef/isVNode checks on component instance leading
// to infinite warning loop
// [ts] key.indexOf("__v") != 0)) {
// [ts] if (data != EMPTY_OBJ &&
// [ts] (key[0] == "$" || key[0] == "_") &&
// [ts] hasOwn: (data, key) => {
// [ts] warn(`Property ${JSON.stringify(key)} must be accessed via $data because it starts with a reserved ` +
// [ts] `character ("$" or "_") and is not proxied on the render context.`);
// [ts] }
// [ts] else if (instance == currentRenderingInstance) {
// [ts] warn(`Property ${JSON.stringify(key)} was accessed during render ` +
// [ts] `but is not defined on instance.`);
// [ts] }
// [ts] }
// [ts] },
// [ts] set: (_p, key, value) => {
// [ts] const { data, setupState, ctx } = instance;
// [ts] if (setupState != EMPTY_OBJ && hasOwn(setupState, key)) {
// [ts] setupState[key] = value;
// [ts] }
// [ts] else if (data != EMPTY_OBJ && hasOwn(data, key)) {
// [ts] data[key] = value;
// [ts] }
// [ts] else if (hasOwn(instance.props, key)) {
// [ts] warn(`Attempting to mutate prop "${key}". Props are readonly.`, instance);
// [ts] return false;
// [ts] }
// [ts] if (key[0] == "$" && key.slice(1) == instance) {
// [ts] warn(`Attempting to mutate public property "${key}". ` +
// [ts] `Properties starting with $ are reserved and readonly.`, instance);
// [ts] return false;
// [ts] }
// [ts] else {
// [ts] if (key == instance.appContext.config.globalProperties) {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ctx, key, {
// [ts-skip] enumerable: true,
// [ts-skip] configurable: true,
// [ts-skip] value
// [ts-skip] });
// [ts] }
// [ts] else {
// [ts] ctx[key] = value;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] },
// [ts] has: (_p, key) => {
// [ts] let normalizedProps;
// [ts] return (accessCache[key] != null ||
// [ts] (data != EMPTY_OBJ && hasOwn(data, key)) ||
// [ts] (setupState != EMPTY_OBJ && hasOwn(setupState, key)) ||
// [ts] ((normalizedProps = propsOptions[0]) && hasOwn(normalizedProps, key)) ||
// [ts] hasOwn(ctx, key) ||
// [ts] hasOwn(publicPropertiesMap, key) ||
// [ts] hasOwn(appContext.config.globalProperties, key));
// [ts] }
// [ts] };
// [ts] {
// [ts] PublicInstanceProxyHandlers.ownKeys = (target) => {
// [ts] warn(`Avoid app logic that relies on enumerating keys on a component instance. ` +
// [ts] `The keys will be empty in production mode to avoid performance overhead.`);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return Reflect.ownKeys(target);
// [ts] };
// [ts] }
// [ts] const RuntimeCompiledPublicInstanceProxyHandlers = extend({}, PublicInstanceProxyHandlers, {
// [ts] get: (target, key) => {
// fast path for unscopables when using `with` block
// [ts] if (key == "__symbol_unscopables") {
// [ts] return;
// [ts] }
// [ts] return PublicInstanceProxyHandlers.get(target, key, target);
// [ts] },
// [ts] has: (_, key) => {
// [ts] const has = key[0] != "_" && !isGloballyWhitelisted(key);
// [ts] if (!has && PublicInstanceProxyHandlers.has(_, key)) {
// [ts] warn(`Property ${JSON.stringify(key)} should not start with _ which is a reserved prefix for Vue internals.`);
// [ts] }
// [ts] return has;
// [ts] }
// [ts] });
// In dev mode, the proxy target exposes the same properties as seen on `this`
// for easier console inspection. In prod mode it will be an empty object so
// these properties definitions can be skipped.
// [ts] fn createRenderContext(instance: auto) {
// [ts] const target = {};
// expose internal instance for proxy handlers
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(target, `_`, {
// [ts-skip] configurable: true,
// [ts-skip] enumerable: false,
// [ts-skip] get: () => instance
// [ts-skip] });
// expose public properties
// [ts] Object.keys(publicPropertiesMap).forEach(key => {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(target, key, {
// [ts-skip] configurable: true,
// [ts-skip] enumerable: false,
// [ts-skip] get: () => publicPropertiesMap[key](instance),
// [ts-skip] // intercepted by the proxy so no need for implementation,
// [ts-skip] // but needed to prevent set errors
// [ts-skip] set: NOOP
// [ts-skip] });
// [ts] });
// [ts] return target;
// [ts] }
// dev only
// [ts] fn exposePropsOnRenderContext(instance: auto) {
// [ts] const { ctx, propsOptions: [propsOptions] } = instance;
// [ts] if (propsOptions) {
// [ts] Object.keys(propsOptions).forEach(key => {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ctx, key, {
// [ts-skip] enumerable: true,
// [ts-skip] configurable: true,
// [ts-skip] get: () => instance.props[key],
// [ts-skip] set: NOOP
// [ts-skip] });
// [ts] });
// [ts] }
// [ts] }
// dev only
// [ts] fn exposeSetupStateOnRenderContext(instance: auto) {
// [ts] const { ctx, setupState } = instance;
// [ts] Object.keys(toRaw(setupState)).forEach(key => {
// [ts] if (!setupState.__isScriptSetup && (key[0] == "$" || key[0] == "_")) {
// [ts] warn(`setup() return property ${JSON.stringify(key)} should not start with "$" or "_" ` +
// [ts] `which are reserved prefixes for Vue internals.`);
// [ts] return;
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ctx, key, {
// [ts-skip] enumerable: true,
// [ts-skip] configurable: true,
// [ts-skip] get: () => setupState[key],
// [ts-skip] set: NOOP
// [ts-skip] });
// [ts] });
// [ts] }
// [ts] const emptyAppContext = createAppContext();
// [ts] let uid$2 = 0;
// [ts] fn createComponentInstance(vnode: auto, parent: auto, suspense: auto) {
// [ts] const type = vnode.type;
// inherit parent app context - or - if root, adopt from root vnode
// [ts] const appContext = (if (parent) parent.appContext else vnode.appContext) || emptyAppContext;
// [ts] const instance = {
// [ts] uid: uid$2++,
// [ts] vnode,
// [ts] type,
// [ts] parent,
// [ts] appContext,
// [ts] root: null,
// [ts] next: null,
// [ts] subTree: null,
// [ts] update: null,
// [ts] render: null,
// [ts] proxy: null,
// [ts] exposed: null,
// [ts] exposeProxy: null,
// [ts] withProxy: null,
// [ts] effects: null,
// [ts] provides: if (parent) parent.provides else Object.create(appContext.provides),
// [ts] accessCache: null,
// [ts] renderCache: [],
// local resovled assets
// [ts] components: null,
// [ts] directives: null,
// resolved props and emits options
// [ts] propsOptions: normalizePropsOptions(type, appContext),
// [ts] emitsOptions: normalizeEmitsOptions(type, appContext),
// emit
// [ts] emit: null,
// [ts] emitted: null,
// props default value
// [ts] propsDefaults: EMPTY_OBJ,
// inheritAttrs
// [ts] inheritAttrs: type.inheritAttrs,
// state
// [ts] ctx: EMPTY_OBJ,
// [ts] data: EMPTY_OBJ,
// [ts] props: EMPTY_OBJ,
// [ts] attrs: EMPTY_OBJ,
// [ts] slots: EMPTY_OBJ,
// [ts] refs: EMPTY_OBJ,
// [ts] setupState: EMPTY_OBJ,
// [ts] setupContext: null,
// suspense related
// [ts] suspense,
// [ts] suspenseId: if (suspense) suspense.pendingId else 0,
// [ts] asyncDep: null,
// [ts] asyncResolved: false,
// lifecycle hooks
// not using enums here because it results in computed properties
// [ts] isMounted: false,
// [ts] isUnmounted: false,
// [ts] isDeactivated: false,
// [ts] bc: null,
// [ts] c: null,
// [ts] bm: null,
// [ts] m: null,
// [ts] bu: null,
// [ts] u: null,
// [ts] um: null,
// [ts] bum: null,
// [ts] da: null,
// [ts] a: null,
// [ts] rtg: null,
// [ts] rtc: null,
// [ts] ec: null,
// [ts] sp: null
// [ts] };
// [ts] {
// [ts] instance.ctx = createRenderContext(instance);
// [ts] }
// [ts] instance.root = if (parent) parent.root else instance;
// [ts] instance.emit = emit.bind(null, instance);
// [ts] return instance;
// [ts] }
// [ts] let currentInstance = null;
// [ts] fn getCurrentInstance() { return currentInstance || currentRenderingInstance; }
// [ts] fn setCurrentInstance(instance: auto) {
// [ts] currentInstance = instance;
// [ts] };
// [ts] const isBuiltInTag = /*_priv___PURE__*/ makeMap("slot,component");
// [ts] fn validateComponentName(name: auto, config: auto) {
// [ts] const appIsNativeTag = config.isNativeTag || NO;
// [ts] if (isBuiltInTag(name) || appIsNativeTag(name)) {
// [ts] warn("Do not use built-in or reserved HTML elements as component id: " + name);
// [ts] }
// [ts] }
// [ts] fn isStatefulComponent(instance: auto) {
// [ts] return instance.vnode.shapeFlag & 4 /* STATEFUL_COMPONENT */;
// [ts] }
// [ts] let isInSSRComponentSetup = false;
// [ts] fn setupComponent(instance: auto, isSSR: auto = false) {
// [ts] isInSSRComponentSetup = isSSR;
// [ts] const { props, children } = instance.vnode;
// [ts] const isStateful = isStatefulComponent(instance);
// [ts] initProps(instance, props, isStateful, isSSR);
// [ts] initSlots(instance, children);
// [ts] const setupResult = if (isStateful) setupStatefulComponent(instance, isSSR) else null;
// [ts] isInSSRComponentSetup = false;
// [ts] return setupResult;
// [ts] }
// [ts] fn setupStatefulComponent(instance: auto, isSSR: auto) {
// [ts] const Component = instance.type;
// [ts] {
// [ts] if (Component.name) {
// [ts] validateComponentName(Component.name, instance.appContext.config);
// [ts] }
// [ts] if (Component.components) {
// [ts] const names = Object.keys(Component.components);
// [ts] let i = 0;
// [ts] while (i < names.length) { // [cap-migrate] for-step: i++
// [ts] validateComponentName(names[i], instance.appContext.config);
// [ts] }
// [ts] }
// [ts] if (Component.directives) {
// [ts] const names = Object.keys(Component.directives);
// [ts] let i = 0;
// [ts] while (i < names.length) { // [cap-migrate] for-step: i++
// [ts] validateDirectiveName(names[i]);
// [ts] }
// [ts] }
// [ts] if (Component.compilerOptions && isRuntimeOnly()) {
// [ts] warn(`"compilerOptions" is only supported when using a build of Vue that ` +
// [ts] `includes the runtime compiler. Since you are using a runtime-only ` +
// [ts] `build, the options should be passed via your build tool config instead.`);
// [ts] }
// [ts] }
// 0. create render proxy property access cache
// [ts] instance.accessCache = Object.create(null);
// 1. create public instance / render proxy
// also mark it raw so it's never observed
// [ts] instance.proxy = markRaw(Proxy.init(instance.ctx, PublicInstanceProxyHandlers));
// [ts] {
// [ts] exposePropsOnRenderContext(instance);
// [ts] }
// 2. call setup()
// [ts] const { setup } = Component;
// [ts] if (setup) {
// [ts] const setupContext = (instance.setupContext =
// [ts] if (setup.length > 1) createSetupContext(instance) else null);
// [ts] currentInstance = instance;
// [ts] pauseTracking();
// [ts] const setupResult = callWithErrorHandling(setup, instance, 0 /* SETUP_FUNCTION */, [shallowReadonly(instance.props) , setupContext]);
// [ts] resetTracking();
// [ts] currentInstance = null;
// [ts] if (isPromise(setupResult)) {
// [ts] const unsetInstance = () => {
// [ts] currentInstance = null;
// [ts] };
// [ts] setupResult.then(unsetInstance, unsetInstance);
// [ts] if (isSSR) {
// return the promise so server-renderer can wait on it
// [ts] return setupResult
// [ts] .then((resolvedResult) => {
// [ts] handleSetupResult(instance, resolvedResult, isSSR);
// [ts] })
// [ts] .catch(e => {
// [ts] handleError(e, instance, 0 /* SETUP_FUNCTION */);
// [ts] });
// [ts] }
// [ts] else {
// async setup returned Promise.
// bail here and wait for re-entry.
// [ts] instance.asyncDep = setupResult;
// [ts] }
// [ts] }
// [ts] else {
// [ts] handleSetupResult(instance, setupResult, isSSR);
// [ts] }
// [ts] }
// [ts] else {
// [ts] finishComponentSetup(instance, isSSR);
// [ts] }
// [ts] }
// [ts] fn handleSetupResult(instance: auto, setupResult: auto, isSSR: auto) {
// [ts] if (isFunction(setupResult)) {
// setup returned an inline render function
// [ts] {
// [ts] instance.render = setupResult;
// [ts] }
// [ts] }
// [ts] else if (isObject(setupResult)) {
// [ts] if (isVNode(setupResult)) {
// [ts] warn(`setup() should not return VNodes directly - ` +
// [ts] `return a render function instead.`);
// [ts] }
// setup returned bindings.
// assuming a render function compiled from template is present.
// [ts] {
// [ts] instance.devtoolsRawSetupState = setupResult;
// [ts] }
// [ts] instance.setupState = proxyRefs(setupResult);
// [ts] {
// [ts] exposeSetupStateOnRenderContext(instance);
// [ts] }
// [ts] }
// [ts] else if (setupResult != null) {
// [ts] warn(`setup() should return an object. Received: ${if (setupResult == null) "null" else @typeName(setupResult)}`);
// [ts] }
// [ts] finishComponentSetup(instance, isSSR);
// [ts] }
// [ts] let compile;
// dev only
// [ts] fn isRuntimeOnly() { return !compile; }
// [ts] /**
// [ts] * For runtime-dom to register the compiler.
// [ts] * Note the exported method uses any to avoid d.ts relying on the compiler types.
// [ts] */
// [ts] fn registerRuntimeCompiler(_compile: auto) {
// [ts] compile = _compile;
// [ts] }
// [ts] fn finishComponentSetup(instance: auto, isSSR: auto, skipOptions: auto) {
// [ts] const Component = instance.type;
// template / render function normalization
// [ts] if (!instance.render) {
// could be set from setup()
// [ts] if (compile && !Component.render) {
// [ts] const template = Component.template;
// [ts] if (template) {
// [ts] {
// [ts] startMeasure(instance, `compile`);
// [ts] }
// [ts] const { isCustomElement, compilerOptions } = instance.appContext.config;
// [ts] const { delimiters, compilerOptions: componentCompilerOptions } = Component;
// [ts] const finalCompilerOptions = extend(extend({
// [ts] isCustomElement,
// [ts] delimiters
// [ts] }, compilerOptions), componentCompilerOptions);
// [ts] Component.render = compile(template, finalCompilerOptions);
// [ts] {
// [ts] endMeasure(instance, `compile`);
// [ts] }
// [ts] }
// [ts] }
// [ts] instance.render = (Component.render || NOOP);
// for runtime-compiled render functions using `with` blocks, the render
// proxy used needs a different `has` handler which is more performant and
// also only allows a whitelist of globals to fallthrough.
// [ts] if (instance.render._rc) {
// [ts] instance.withProxy = Proxy.init(instance.ctx, RuntimeCompiledPublicInstanceProxyHandlers);
// [ts] }
// [ts] }
// support for 2.x options
// [ts] {
// [ts] currentInstance = instance;
// [ts] pauseTracking();
// [ts] applyOptions(instance);
// [ts] resetTracking();
// [ts] currentInstance = null;
// [ts] }
// warn missing template/render
// the runtime compilation of template in SSR is done by server-render
// [ts] if (!Component.render && instance.render == NOOP && !isSSR) {
// [ts] /* istanbul ignore if */
// [ts] if (!compile && Component.template) {
// [ts] warn(`Component provided template option but ` +
// [ts] `runtime compilation is not supported in this build of Vue.` +
// [ts] (` Use "vue.global.js" instead.`
// [ts] ) /* should not happen */);
// [ts] }
// [ts] else {
// [ts] warn(`Component is missing template or render function.`);
// [ts] }
// [ts] }
// [ts] }
// [ts] const attrDevProxyHandlers = {
// [ts] get: (target, key) => {
// [ts] markAttrsAccessed();
// [ts] return target[key];
// [ts] },
// [ts] set: () => {
// [ts] warn(`setupContext.attrs is readonly.`);
// [ts] return false;
// [ts] },
// [ts] deleteProperty: () => {
// [ts] warn(`setupContext.attrs is readonly.`);
// [ts] return false;
// [ts] }
// [ts] };
// [ts] fn createSetupContext(instance: auto) {
// [ts] const expose = exposed => {
// [ts] if (instance.exposed) {
// [ts] warn(`expose() should be called only once per setup().`);
// [ts] }
// [ts] instance.exposed = exposed || {};
// [ts] };
// [ts] {
// [ts] let attrs;
// We use getters in dev in case libs like test-utils overwrite instance
// properties (overwrites should not be done in prod)
// [ts] return Object.freeze({
// [ts] get attrs() {
// [ts] return (attrs || (attrs = Proxy.init(instance.attrs, attrDevProxyHandlers)));
// [ts] },
// [ts] get slots() {
// [ts] return shallowReadonly(instance.slots);
// [ts] },
// [ts] get emit() {
// [ts] return (event, args) => instance.emit(event, ...args);
// [ts] },
// [ts] expose
// [ts] });
// [ts] }
// [ts] }
// [ts] fn getExposeProxy(instance: auto) {
// [ts] if (instance.exposed) {
// [ts] return (instance.exposeProxy ||
// [ts] (instance.exposeProxy = Proxy.init(proxyRefs(markRaw(instance.exposed)), {
// [ts] get: (target, key) => {
// [ts] if (key == target) {
// [ts] return target[key];
// [ts] }
// [ts] else if (key == publicPropertiesMap) {
// [ts] return publicPropertiesMap[key](instance);
// [ts] }
// [ts] }
// [ts] })));
// [ts] }
// [ts] }
// record effects created during a component's setup() so that they can be
// stopped when the component unmounts
// [ts] fn recordInstanceBoundEffect(effect: auto, instance: auto = currentInstance) {
// [ts] if (instance) {
// [ts] (instance.effects || (instance.effects = [])).push(effect);
// [ts] }
// [ts] }
// [ts] const classifyRE = "(?:^|[-_])(\\w)";
// [ts] fn classify(str: auto) { return str.replace(classifyRE, c => c.toUpperCase()).replace("[-_]", ""); }
// [ts] fn getComponentName(Component: auto) {
// [ts] return if (isFunction(Component)) Component.displayName || Component.name else Component.name;
// [ts] }
// [ts] /* istanbul ignore next */
// [ts] fn formatComponentName(instance: auto, Component: auto, isRoot: auto = false) {
// [ts] let name = getComponentName(Component);
// [ts] if (!name && Component.__file) {
// [ts] const match_val = Component.__file.match("([^/\\\\]+)\\.\\w+$");
// [ts] if (match_val) {
// [ts] name = match_val[1];
// [ts] }
// [ts] }
// [ts] if (!name && instance && instance.parent) {
// try to infer the name based on reverse resolution
// [ts] const inferFromRegistry = (registry) => {
// [ts] for (const key in registry) {
// [ts] if (registry[key] == Component) {
// [ts] return key;
// [ts] }
// [ts] }
// [ts] };
// [ts] name =
// [ts] inferFromRegistry(instance.components ||
// [ts] instance.parent.type.components) || inferFromRegistry(instance.appContext.components);
// [ts] }
// [ts] return if (name) classify(name) else if (isRoot) `App` else `Anonymous`;
// [ts] }
// [ts] fn isClassComponent(value: auto) {
// [ts] return isFunction(value) && "__vccOpts" == value;
// [ts] }
// [ts] (getterOrOptions) => {
// [ts] const c = computed(getterOrOptions);
// [ts] recordInstanceBoundEffect(c.effect);
// [ts] return c;
// [ts] }
// dev only
// [ts] fn warnRuntimeUsage(method: auto) { return warn(`${method}() is a compiler-hint helper that is only usable inside ` + `<script setup> of a single file component. Its arguments should be ` + `compiled away and passing it at runtime has no effect.`); }
// implementation
// [ts] fn defineProps() {
// [ts] {
// [ts] warnRuntimeUsage(`defineProps`);
// [ts] }
// [ts] return null;
// [ts] }
// implementation
// [ts] fn defineEmits() {
// [ts] {
// [ts] warnRuntimeUsage(`defineEmits`);
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * @deprecated use `defineEmits` instead.
// [ts] */
// [ts] const defineEmit = defineEmits;
// [ts] /**
// [ts] * Vue `<script setup>` compiler macro for declaring a component's exposed
// [ts] * instance properties when it is accessed by a parent component via template
// [ts] * refs.
// [ts] *
// [ts] * `<script setup>` components are closed by default - i.e. varaibles inside
// [ts] * the `<script setup>` scope is not exposed to parent unless explicitly exposed
// [ts] * via `defineExpose`.
// [ts] *
// [ts] * This is only usable inside `<script setup>`, is compiled away in the
// [ts] * output and should **not** be actually called at runtime.
// [ts] */
// [ts] fn defineExpose(exposed: auto) {
// [ts] {
// [ts] warnRuntimeUsage(`defineExpose`);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Vue `<script setup>` compiler macro for providing props default values when
// [ts] * using type-based `defineProps` decalration.
// [ts] *
// [ts] * Example usage:
// [ts] * ```ts
// [ts] * withDefaults(defineProps<{
// [ts] *   size?: number
// [ts] *   labels?: string[]
// [ts] * }>(), {
// [ts] *   size: 3,
// [ts] *   labels: () => ['default label']
// [ts] * })
// [ts] * ```
// [ts] *
// [ts] * This is only usable inside `<script setup>`, is compiled away in the output
// [ts] * and should **not** be actually called at runtime.
// [ts] */
// [ts] fn withDefaults(props: auto, defaults: auto) {
// [ts] {
// [ts] warnRuntimeUsage(`withDefaults`);
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * @deprecated use `useSlots` and `useAttrs` instead.
// [ts] */
// [ts] fn useContext() {
// [ts] {
// [ts] warn(`\`useContext()\` has been deprecated and will be removed in the ` +
// [ts] `next minor release. Use \`useSlots()\` and \`useAttrs()\` instead.`);
// [ts] }
// [ts] return getContext();
// [ts] }
// [ts] fn useSlots() {
// [ts] return getContext().slots;
// [ts] }
// [ts] fn useAttrs() {
// [ts] return getContext().attrs;
// [ts] }
// [ts] fn getContext() {
// [ts] const i = getCurrentInstance();
// [ts] if (!i) {
// [ts] warn(`useContext() called without active instance.`);
// [ts] }
// [ts] return i.setupContext || (i.setupContext = createSetupContext(i));
// [ts] }
// [ts] /**
// [ts] * Runtime helper for merging default declarations. Imported by compiled code
// [ts] * only.
// [ts] * @internal
// [ts] */
// [ts] fn mergeDefaults(// the base props is compiler-generated and guaranteed to be in this shape. props: auto, defaults: auto) {
// [ts] for (const key in defaults) {
// [ts] const val = props[key];
// [ts] if (val) {
// [ts] val.default = defaults[key];
// [ts] }
// [ts] else if (val == null) {
// [ts] props[key] = { default: defaults[key] };
// [ts] }
// [ts] else {
// [ts] warn(`props default key "${key}" has no corresponding declaration.`);
// [ts] }
// [ts] }
// [ts] return props;
// [ts] }
// [ts] /**
// [ts] * `<script setup>` helper for persisting the current instance context over
// [ts] * async/await flows.
// [ts] *
// [ts] * `@vue/compiler-sfc` converts the following:
// [ts] *
// [ts] * ```ts
// [ts] * const x = await foo()
// [ts] * ```
// [ts] *
// [ts] * into:
// [ts] *
// [ts] * ```ts
// [ts] * let __temp, __restore
// [ts] * const x = (([__temp, __restore] = withAsyncContext(() => foo())),__temp=await __temp,__restore(),__temp)
// [ts] * ```
// [ts] * @internal
// [ts] */
// [ts] fn withAsyncContext(getAwaitable: auto) {
// [ts] const ctx = getCurrentInstance();
// [ts] let awaitable = getAwaitable();
// [ts] setCurrentInstance(null);
// [ts] if (isPromise(awaitable)) {
// [ts] awaitable = awaitable.catch(e => {
// [ts] setCurrentInstance(ctx);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw e;)
// [ts] });
// [ts] }
// [ts] return [awaitable, () => setCurrentInstance(ctx)];
// [ts] }
// Actual implementation
// [ts] fn h(type: auto, propsOrChildren: auto, children: auto) {
// [ts] const l = arguments.length;
// [ts] if (l == 2) {
// [ts] if (isObject(propsOrChildren) && !isArray(propsOrChildren)) {
// single vnode without props
// [ts] if (isVNode(propsOrChildren)) {
// [ts] return createVNode(type, null, [propsOrChildren]);
// [ts] }
// props without children
// [ts] return createVNode(type, propsOrChildren);
// [ts] }
// [ts] else {
// omit props
// [ts] return createVNode(type, null, propsOrChildren);
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (l > 3) {
// [ts] children = Array.prototype.slice.call(arguments, 2);
// [ts] }
// [ts] else if (l == 3 && isVNode(children)) {
// [ts] children = [children];
// [ts] }
// [ts] return createVNode(type, propsOrChildren, children);
// [ts] }
// [ts] }
// [ts] const ssrContextKey = "__symbol";
// [ts] fn useSSRContext() {
// [ts] {
// [ts] warn(`useSSRContext() is not supported in the global build.`);
// [ts] }
// [ts] };
// [ts] fn initCustomFormatter() {
// [ts] /* eslint-disable no-restricted-globals */
// [ts] if (@typeName(window) == "undefined") {
// [ts] return;
// [ts] }
// [ts] const vueStyle = { style: "color:#3ba776" };
// [ts] const numberStyle = { style: "color:#0b1bc9" };
// [ts] const stringStyle = { style: "color:#b62e24" };
// [ts] const keywordStyle = { style: "color:#9d288c" };
// custom formatter for Chrome
// https://www.mattzeunert.com/2016/02/19/custom-chrome-devtools-object-formatters.html
// [ts] const formatter = {
// [ts] header: (obj) => {
// TODO also format ComponentPublicInstance & ctx.slots/attrs in setup
// [ts] if (!isObject(obj)) {
// [ts] return null;
// [ts] }
// [ts] if (obj.__isVue) {
// [ts] return ["div", vueStyle, `VueInstance`];
// [ts] }
// [ts] else if (isRef(obj)) {
// [ts] return [
// [ts] "div",
// [ts] {},
// [ts] ["span", vueStyle, genRefFlag(obj)],
// [ts] "<",
// [ts] formatValue(obj.value),
// [ts] `>`
// [ts] ];
// [ts] }
// [ts] else if (isReactive(obj)) {
// [ts] return [
// [ts] "div",
// [ts] {},
// [ts] ["span", vueStyle, "Reactive"],
// [ts] "<",
// [ts] formatValue(obj),
// [ts] `>${if (isReadonly(obj)) ` (readonly)` else ``}`
// [ts] ];
// [ts] }
// [ts] else if (isReadonly(obj)) {
// [ts] return [
// [ts] "div",
// [ts] {},
// [ts] ["span", vueStyle, "Readonly"],
// [ts] "<",
// [ts] formatValue(obj),
// [ts] ">"
// [ts] ];
// [ts] }
// [ts] return null;
// [ts] },
// [ts] hasBody: (obj) => {
// [ts] return obj && obj.__isVue;
// [ts] },
// [ts] body: (obj) => {
// [ts] if (obj && obj.__isVue) {
// [ts] return [
// [ts] "div",
// [ts] {},
// [ts] ...formatInstance(obj.$)
// [ts] ];
// [ts] }
// [ts] }
// [ts] };
// [ts] fn formatInstance(instance: auto) {
// [ts] const blocks = [];
// [ts] if (instance.type.props && instance.props) {
// [ts] blocks.push(createInstanceBlock("props", toRaw(instance.props)));
// [ts] }
// [ts] if (instance.setupState != EMPTY_OBJ) {
// [ts] blocks.push(createInstanceBlock("setup", instance.setupState));
// [ts] }
// [ts] if (instance.data != EMPTY_OBJ) {
// [ts] blocks.push(createInstanceBlock("data", toRaw(instance.data)));
// [ts] }
// [ts] const computed = extractKeys(instance, "computed");
// [ts] if (computed) {
// [ts] blocks.push(createInstanceBlock("computed", computed));
// [ts] }
// [ts] const injected = extractKeys(instance, "inject");
// [ts] if (injected) {
// [ts] blocks.push(createInstanceBlock("injected", injected));
// [ts] }
// [ts] blocks.push([
// [ts] "div",
// [ts] {},
// [ts] [
// [ts] "span",
// [ts] {
// [ts] style: keywordStyle.style + ";opacity:0.66"
// [ts] },
// [ts] "$ (internal): "
// [ts] ],
// [ts] ["object", { object: instance }]
// [ts] ]);
// [ts] return blocks;
// [ts] }
// [ts] fn createInstanceBlock(type: auto, target: auto) {
// [ts] target = extend({}, target);
// [ts] if (!Object.keys(target).length) {
// [ts] return ["span", {}];
// [ts] }
// [ts] return [
// [ts] "div",
// [ts] { style: "line-height:1.25em;margin-bottom:0.6em" },
// [ts] [
// [ts] "div",
// [ts] {
// [ts] style: "color:#476582"
// [ts] },
// [ts] type
// [ts] ],
// [ts] [
// [ts] "div",
// [ts] {
// [ts] style: "padding-left:1.25em"
// [ts] },
// [ts] ...Object.keys(target).map(key => {
// [ts] return [
// [ts] "div",
// [ts] {},
// [ts] ["span", keywordStyle, key + ": "],
// [ts] formatValue(target[key], false)
// [ts] ];
// [ts] })
// [ts] ]
// [ts] ];
// [ts] }
// [ts] fn formatValue(v: auto, asRaw: auto = true) {
// [ts] if (@typeName(v) == "number") {
// [ts] return ["span", numberStyle, v];
// [ts] }
// [ts] else if (@typeName(v) == "string") {
// [ts] return ["span", stringStyle, JSON.stringify(v)];
// [ts] }
// [ts] else if (@typeName(v) == "boolean") {
// [ts] return ["span", keywordStyle, v];
// [ts] }
// [ts] else if (isObject(v)) {
// [ts] return ["object", { object: asRaw ? toRaw(v) : v }];
// [ts] }
// [ts] else {
// [ts] return ["span", stringStyle, String(v)];
// [ts] }
// [ts] }
// [ts] fn extractKeys(instance: auto, type: auto) {
// [ts] const Comp = instance.type;
// [ts] if (isFunction(Comp)) {
// [ts] return;
// [ts] }
// [ts] const extracted = {};
// [ts] for (const key in instance.ctx) {
// [ts] if (isKeyOfType(Comp, key, type)) {
// [ts] extracted[key] = instance.ctx[key];
// [ts] }
// [ts] }
// [ts] return extracted;
// [ts] }
// [ts] fn isKeyOfType(Comp: auto, key: auto, type: auto) {
// [ts] const opts = Comp[type];
// [ts] if ((isArray(opts) && opts.includes(key)) ||
// [ts] (isObject(opts) && key == opts)) {
// [ts] return true;
// [ts] }
// [ts] if (Comp.extends && isKeyOfType(Comp.extends, key, type)) {
// [ts] return true;
// [ts] }
// [ts] if (Comp.mixins && Comp.mixins.some(m => isKeyOfType(m, key, type))) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] fn genRefFlag(v: auto) {
// [ts] if (v._shallow) {
// [ts] return `ShallowRef`;
// [ts] }
// [ts] if (v.effect) {
// [ts] return `ComputedRef`;
// [ts] }
// [ts] return `Ref`;
// [ts] }
// [ts] if (window.devtoolsFormatters) {
// [ts] window.devtoolsFormatters.push(formatter);
// [ts] }
// [ts] else {
// [ts] window.devtoolsFormatters = [formatter];
// [ts] }
// [ts] }
// Core API ------------------------------------------------------------------
// [ts] const version = "3.1.5";
// [ts] /**
// [ts] * SSR utils for \@vue/server-renderer. Only exposed in cjs builds.
// [ts] * @internal
// [ts] */
// [ts] const ssrUtils = (null);
// [ts] /**
// [ts] * @internal only exposed in compat builds
// [ts] */
// [ts] const resolveFilter = null;
// [ts] /**
// [ts] * @internal only exposed in compat builds.
// [ts] */
// [ts] const compatUtils = (null);
// [ts] const svgNS = "http://www.w3.org/2000/svg";
// [ts] const doc = (if (@typeName(document) != "undefined") document else null);
// [ts] const staticTemplateCache = Map.init();
// [ts] const nodeOps = {
// [ts] insert: (child, parent, anchor) => {
// [ts] parent.insertBefore(child, anchor || null);
// [ts] },
// [ts] remove: child => {
// [ts] const parent = child.parentNode;
// [ts] if (parent) {
// [ts] parent.removeChild(child);
// [ts] }
// [ts] },
// [ts] createElement: (tag, isSVG, is, props) => {
// [ts] const el = if (isSVG) doc.createElementNS(svgNS, tag) else doc.createElement(if (tag, is) { is } else null);
// [ts] if (tag == "select" && props && props.multiple != null) {
// [ts] el.setAttribute("multiple", props.multiple);
// [ts] }
// [ts] return el;
// [ts] },
// [ts] createText: text => doc.createTextNode(text),
// [ts] createComment: text => doc.createComment(text),
// [ts] setText: (node, text) => {
// [ts] node.nodeValue = text;
// [ts] },
// [ts] setElementText: (el, text) => {
// [ts] el.textContent = text;
// [ts] },
// [ts] parentNode: node => node.parentNode,
// [ts] nextSibling: node => node.nextSibling,
// [ts] querySelector: selector => doc.querySelector(selector),
// [ts] setScopeId: (el, id) => {
// [ts] el.setAttribute(id, "");
// [ts] },
// [ts] cloneNode: (el) => {
// [ts] const cloned = el.cloneNode(true);
// #3072
// - in `patchDOMProp`, we store the actual value in the `el._value` property.
// - normally, elements using `:value` bindings will not be hoisted, but if
//   the bound value is a constant, e.g. `:value="true"` - they do get
//   hoisted.
// - in production, hoisted nodes are cloned when subsequent inserts, but
//   cloneNode() does not copy the custom property we attached.
// - This may need to account for other custom DOM properties we attach to
//   elements in addition to `_value` in the future.
// [ts] if (`_value` == el) {
// [ts] cloned._value = el._value;
// [ts] }
// [ts] return cloned;
// [ts] },
// __UNSAFE__
// Reason: innerHTML.
// Static content here can only come from compiled templates.
// As long as the user only uses trusted templates, this is safe.
// [ts] insertStaticContent: (content, parent, anchor, isSVG) => {
// <parent> before | first ... last | anchor </parent>
// [ts] const before = if (anchor) anchor.previousSibling else parent.lastChild;
// [ts] let template = staticTemplateCache.get(content);
// [ts] if (!template) {
// [ts] const t = doc.createElement("template");
// [ts] t.innerHTML = if (isSVG) `<svg>${content}</svg>` else content;
// [ts] template = t.content;
// [ts] if (isSVG) {
// remove outer svg wrapper
// [ts] const wrapper = template.firstChild;
// [ts] while (wrapper.firstChild) {
// [ts] template.appendChild(wrapper.firstChild);
// [ts] }
// [ts] template.removeChild(wrapper);
// [ts] }
// [ts] staticTemplateCache.set(content, template);
// [ts] }
// [ts] parent.insertBefore(template.cloneNode(true), anchor);
// [ts] return [
// first
// [ts] if (before) before.nextSibling else parent.firstChild,
// last
// [ts] if (anchor) anchor.previousSibling else parent.lastChild
// [ts] ];
// [ts] }
// [ts] };
// compiler should normalize class + :class bindings on the same element
// into a single binding ['staticClass', dynamic]
// [ts] fn patchClass(el: auto, value: auto, isSVG: auto) {
// directly setting className should be faster than setAttribute in theory
// if this is an element during a transition, take the temporary transition
// classes into account.
// [ts] const transitionClasses = el._vtc;
// [ts] if (transitionClasses) {
// [ts] value = (if (value) [value, ...transitionClasses] else [...transitionClasses]).join(" ");
// [ts] }
// [ts] if (value == null) {
// [ts] el.removeAttribute("class");
// [ts] }
// [ts] else if (isSVG) {
// [ts] el.setAttribute("class", value);
// [ts] }
// [ts] else {
// [ts] el.className = value;
// [ts] }
// [ts] }
// [ts] fn patchStyle(el: auto, prev: auto, next: auto) {
// [ts] const style = el.style;
// [ts] if (!next) {
// [ts] el.removeAttribute("style");
// [ts] }
// [ts] else if (isString(next)) {
// [ts] if (prev != next) {
// [ts] const current = style.display;
// [ts] style.cssText = next;
// indicates that the `display` of the element is controlled by `v-show`,
// so we always keep the current `display` value regardless of the `style` value,
// thus handing over control to `v-show`.
// [ts] if ("_vod" == el) {
// [ts] style.display = current;
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] for (const key in next) {
// [ts] setStyle(style, key, next[key]);
// [ts] }
// [ts] if (prev && !isString(prev)) {
// [ts] for (const key in prev) {
// [ts] if (next[key] == null) {
// [ts] setStyle(style, key, "");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] const importantRE = "\\s*!important$";
// [ts] fn setStyle(style: auto, name: auto, val: auto) {
// [ts] if (isArray(val)) {
// [ts] val.forEach(v => setStyle(style, name, v));
// [ts] }
// [ts] else {
// [ts] if (name.startsWith("--")) {
// custom property definition
// [ts] style.setProperty(name, val);
// [ts] }
// [ts] else {
// [ts] const prefixed = autoPrefix(style, name);
// [ts] if (importantRE.test(val)) {
// !important
// [ts] style.setProperty(hyphenate(prefixed), val.replace(importantRE, ""), "important");
// [ts] }
// [ts] else {
// [ts] style[prefixed] = val;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] const prefixes = ["Webkit", "Moz", "ms"];
// [ts] const prefixCache = {};
// [ts] fn autoPrefix(style: auto, rawName: auto) {
// [ts] const cached = prefixCache[rawName];
// [ts] if (cached) {
// [ts] return cached;
// [ts] }
// [ts] let name = camelize(rawName);
// [ts] if (name != "filter" && name == style) {
// [ts] return (prefixCache[rawName] = name);
// [ts] }
// [ts] name = capitalize(name);
// [ts] let i = 0;
// [ts] while (i < prefixes.length) { // [cap-migrate] for-step: i++
// [ts] const prefixed = prefixes[i] + name;
// [ts] if (prefixed == style) {
// [ts] return (prefixCache[rawName] = prefixed);
// [ts] }
// [ts] }
// [ts] return rawName;
// [ts] }
// [ts] const xlinkNS = "http://www.w3.org/1999/xlink";
// [ts] fn patchAttr(el: auto, key: auto, value: auto, isSVG: auto, instance: auto) {
// [ts] if (isSVG && key.startsWith("xlink:")) {
// [ts] if (value == null) {
// [ts] el.removeAttributeNS(xlinkNS, key.slice(6, key.length));
// [ts] }
// [ts] else {
// [ts] el.setAttributeNS(xlinkNS, key, value);
// [ts] }
// [ts] }
// [ts] else {
// note we are only checking boolean attributes that don't have a
// corresponding dom prop of the same name here.
// [ts] const isBoolean = isSpecialBooleanAttr(key);
// [ts] if (value == null || (isBoolean && value == false)) {
// [ts] el.removeAttribute(key);
// [ts] }
// [ts] else {
// [ts] el.setAttribute(if (key, isBoolean) "" else value);
// [ts] }
// [ts] }
// [ts] }
// __UNSAFE__
// functions. The user is responsible for using them with only trusted content.
// [ts] fn patchDOMProp(el: auto, key: auto, value: auto, // the following args are passed only due to potential innerHTML/textContent // overriding existing VNodes: auto, in which case the old tree must be properly // unmounted. prevChildren: auto, parentComponent: auto, parentSuspense: auto, unmountChildren: auto) {
// [ts] if (key == "innerHTML" || key == "textContent") {
// [ts] if (prevChildren) {
// [ts] unmountChildren(prevChildren, parentComponent, parentSuspense);
// [ts] }
// [ts] el[key] = if (value == null) "" else value;
// [ts] return;
// [ts] }
// [ts] if (key == "value" && el.tagName != "PROGRESS") {
// store value as _value as well since
// non-string values will be stringified.
// [ts] el._value = value;
// [ts] const newValue = if (value == null) "" else value;
// [ts] if (el.value != newValue) {
// [ts] el.value = newValue;
// [ts] }
// [ts] if (value == null) {
// [ts] el.removeAttribute(key);
// [ts] }
// [ts] return;
// [ts] }
// [ts] if (value == "" || value == null) {
// [ts] const type = @typeName(el[key]);
// [ts] if (value == "" && type == "boolean") {
// e.g. <select multiple> compiles to { multiple: '' }
// [ts] el[key] = true;
// [ts] return;
// [ts] }
// [ts] else if (value == null && type == "string") {
// e.g. <div :id="null">
// [ts] el[key] = "";
// [ts] el.removeAttribute(key);
// [ts] return;
// [ts] }
// [ts] else if (type == "number") {
// e.g. <img :width="null">
// the value of some IDL attr must be greater than 0, e.g. input.size = 0 -> error
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] el[key] = 0;
// [ts] }
// TODO(cap-migrate): replace try/catch with CP ?T / ? (catch (_a) { })
// [ts] el.removeAttribute(key);
// [ts] return;
// [ts] }
// [ts] }
// some properties perform value validation and throw
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] el[key] = value;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ? (catch (e) {)
// [ts] {
// [ts] warn(`Failed setting prop "${key}" on <${el.tagName.toLowerCase()}>: ` +
// [ts] `value ${value} is invalid.`, e);
// [ts] }
// [ts] }
// [ts] }
// Async edge case fix requires storing an event listener's attach timestamp.
// [ts] let _getNow = Date.now;
// [ts] let skipTimestampCheck = false;
// [ts] if (@typeName(window) != "undefined") {
// Determine what event timestamp the browser is using. Annoyingly, the
// timestamp can either be hi-res (relative to page load) or low-res
// (relative to UNIX epoch), so in order to compare time we have to use the
// same timestamp type when saving the flush timestamp.
// [ts] if (_getNow() > document.createEvent("Event").timeStamp) {
// if the low-res timestamp which is bigger than the event timestamp
// (which is evaluated AFTER) it means the event is using a hi-res timestamp,
// and we need to use the hi-res version for event listeners as well.
// [ts] _getNow = () => performance.now();
// [ts] }
// #3485: Firefox <= 53 has incorrect Event.timeStamp implementation
// and does not fire microtasks in between event propagation, so safe to exclude.
// [ts] const ffMatch = navigator.userAgent.match("firefox\\/(\\d+)");
// [ts] skipTimestampCheck = !!(ffMatch && Number(ffMatch[1]) <= 53);
// [ts] }
// To avoid the overhead of repeatedly calling performance.now(), we cache
// and use the same timestamp for all event listeners attached in the same tick.
// [ts] let cachedNow = 0;
// [ts] const p = Promise.resolve();
// [ts] fn reset() {
// [ts] cachedNow = 0;
// [ts] };
// [ts] fn getNow() { return cachedNow || (p.then(reset), (cachedNow = _getNow())); }
// [ts] fn addEventListener(el: auto, event: auto, handler: auto, options: auto) {
// [ts] el.addEventListener(event, handler, options);
// [ts] }
// [ts] fn removeEventListener(el: auto, event: auto, handler: auto, options: auto) {
// [ts] el.removeEventListener(event, handler, options);
// [ts] }
// [ts] fn patchEvent(el: auto, rawName: auto, prevValue: auto, nextValue: auto, instance: auto = null) {
// vei = vue event invokers
// [ts] const invokers = el._vei || (el._vei = {});
// [ts] const existingInvoker = invokers[rawName];
// [ts] if (nextValue && existingInvoker) {
// patch
// [ts] existingInvoker.value = nextValue;
// [ts] }
// [ts] else {
// [ts] const [name, options] = parseName(rawName);
// [ts] if (nextValue) {
// add
// [ts] const invoker = (invokers[rawName] = createInvoker(nextValue, instance));
// [ts] addEventListener(el, name, invoker, options);
// [ts] }
// [ts] else if (existingInvoker) {
// remove
// [ts] removeEventListener(el, name, existingInvoker, options);
// [ts] invokers[rawName] = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] const optionsModifierRE = "(?:Once|Passive|Capture)$";
// [ts] fn parseName(name: auto) {
// [ts] let options;
// [ts] if (optionsModifierRE.test(name)) {
// [ts] options = {};
// [ts] let m;
// [ts] while ((m = name.match(optionsModifierRE))) {
// [ts] name = name.slice(0, name.length - m[0].length);
// [ts] options[m[0].toLowerCase()] = true;
// [ts] }
// [ts] }
// [ts] return [hyphenate(name.slice(2)), options];
// [ts] }
// [ts] fn createInvoker(initialValue: auto, instance: auto) {
// [ts] const invoker = (e) => {
// async edge case #6566: inner click event triggers patch, event handler
// attached to outer element during patch, and triggered again. This
// happens because browsers fire microtask ticks between event propagation.
// the solution is simple: we save the timestamp when a handler is attached,
// and the handler would only fire if the event passed to it was fired
// AFTER it was attached.
// [ts] const timeStamp = e.timeStamp || _getNow();
// [ts] if (skipTimestampCheck || timeStamp >= invoker.attached - 1) {
// [ts] callWithAsyncErrorHandling(patchStopImmediatePropagation(e, invoker.value), instance, 5 /* NATIVE_EVENT_HANDLER */, [e]);
// [ts] }
// [ts] };
// [ts] invoker.value = initialValue;
// [ts] invoker.attached = getNow();
// [ts] return invoker;
// [ts] }
// [ts] fn patchStopImmediatePropagation(e: auto, value: auto) {
// [ts] if (isArray(value)) {
// [ts] const originalStop = e.stopImmediatePropagation;
// [ts] e.stopImmediatePropagation = () => {
// [ts] originalStop.call(e);
// [ts] e._stopped = true;
// [ts] };
// [ts] return value.map(fn => (e) => !e._stopped && fn(e));
// [ts] }
// [ts] else {
// [ts] return value;
// [ts] }
// [ts] }
// [ts] const nativeOnRE = "^on[a-z]";
// [ts] fn forcePatchProp(_unused: auto, key: auto) { return key == "value"; }
// [ts] fn patchProp(el: auto, key: auto, prevValue: auto, nextValue: auto, isSVG: auto = false, prevChildren: auto, parentComponent: auto, parentSuspense: auto, unmountChildren: auto) {
// [ts] switch (key) {
// special
// [ts] "class" => {
// [ts] patchClass(el, nextValue, isSVG);
// break;
// [ts] },
// [ts] "style" => {
// [ts] patchStyle(el, prevValue, nextValue);
// break;
// [ts] },
// [ts] else => {
// [ts] if (isOn(key)) {
// ignore v-model listeners
// [ts] if (!isModelListener(key)) {
// [ts] patchEvent(el, key, prevValue, nextValue, parentComponent);
// [ts] }
// [ts] }
// [ts] else if (shouldSetAsProp(el, key, nextValue, isSVG)) {
// [ts] patchDOMProp(el, key, nextValue, prevChildren, parentComponent, parentSuspense, unmountChildren);
// [ts] }
// [ts] else {
// special case for <input v-model type="checkbox"> with
// :true-value & :false-value
// store value as dom properties since non-string values will be
// stringified.
// [ts] if (key == "true-value") {
// [ts] el._trueValue = nextValue;
// [ts] }
// [ts] else if (key == "false-value") {
// [ts] el._falseValue = nextValue;
// [ts] }
// [ts] patchAttr(el, key, nextValue, isSVG);
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] };
// [ts] fn shouldSetAsProp(el: auto, key: auto, value: auto, isSVG: auto) {
// [ts] if (isSVG) {
// most keys must be set as attribute on svg elements to work
// ...except innerHTML
// [ts] if (key == "innerHTML") {
// [ts] return true;
// [ts] }
// or native onclick with function values
// [ts] if (key == el && nativeOnRE.test(key) && isFunction(value)) {
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// spellcheck and draggable are numerated attrs, however their
// corresponding DOM properties are actually booleans - this leads to
// setting it with a string "false" value leading it to be coerced to
// `true`, so we need to always treat them as attributes.
// Note that `contentEditable` doesn't have this problem: its DOM
// property is also enumerated string values.
// [ts] if (key == "spellcheck" || key == "draggable") {
// [ts] return false;
// [ts] }
// #1787, #2840 form property on form elements is readonly and must be set as
// attribute.
// [ts] if (key == "form") {
// [ts] return false;
// [ts] }
// #1526 <input list> must be set as attribute
// [ts] if (key == "list" && el.tagName == "INPUT") {
// [ts] return false;
// [ts] }
// #2766 <textarea type> must be set as attribute
// [ts] if (key == "type" && el.tagName == "TEXTAREA") {
// [ts] return false;
// [ts] }
// native onclick with string value, must be set as attribute
// [ts] if (nativeOnRE.test(key) && isString(value)) {
// [ts] return false;
// [ts] }
// [ts] return key == el;
// [ts] }
// [ts] fn useCssModule(name: auto = "$style") {
// [ts] /* istanbul ignore else */
// [ts] {
// [ts] {
// [ts] warn(`useCssModule() is not supported in the global build.`);
// [ts] }
// [ts] return EMPTY_OBJ;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Runtime helper for SFC's CSS variable injection feature.
// [ts] * @private
// [ts] */
// [ts] fn useCssVars(getter: auto) {
// [ts] const instance = getCurrentInstance();
// [ts] /* istanbul ignore next */
// [ts] if (!instance) {
// [ts] warn(`useCssVars is called without current active component instance.`);
// [ts] return;
// [ts] }
// [ts] const setVars = () => setVarsOnVNode(instance.subTree, getter(instance.proxy));
// [ts] onMounted(() => watchEffect(setVars, { flush: "post" }));
// [ts] onUpdated(setVars);
// [ts] }
// [ts] fn setVarsOnVNode(vnode: auto, vars: auto) {
// [ts] if (vnode.shapeFlag & 128 /* SUSPENSE */) {
// [ts] const suspense = vnode.suspense;
// [ts] vnode = suspense.activeBranch;
// [ts] if (suspense.pendingBranch && !suspense.isHydrating) {
// [ts] suspense.effects.push(() => {
// [ts] setVarsOnVNode(suspense.activeBranch, vars);
// [ts] });
// [ts] }
// [ts] }
// drill down HOCs until it's a non-component vnode
// [ts] while (vnode.component) {
// [ts] vnode = vnode.component.subTree;
// [ts] }
// [ts] if (vnode.shapeFlag & 1 /* ELEMENT */ && vnode.el) {
// [ts] setVarsOnNode(vnode.el, vars);
// [ts] }
// [ts] else if (vnode.type == Fragment) {
// [ts] vnode.children.forEach(c => setVarsOnVNode(c, vars));
// [ts] }
// [ts] else if (vnode.type == Static) {
// [ts] let { el, anchor } = vnode;
// [ts] while (el) {
// [ts] setVarsOnNode(el, vars);
// [ts] if (el == anchor)
// [ts] break;
// [ts] el = el.nextSibling;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn setVarsOnNode(el: auto, vars: auto) {
// [ts] if (el.nodeType == 1) {
// [ts] const style = el.style;
// [ts] for (const key in vars) {
// [ts] style.setProperty(`--${key}`, vars[key]);
// [ts] }
// [ts] }
// [ts] }
// [ts] const TRANSITION = "transition";
// [ts] const ANIMATION = "animation";
// DOM Transition is a higher-order-component based on the platform-agnostic
// base Transition component, with DOM-specific logic.
// [ts] fn Transition(props: auto, _param: string) { return h(BaseTransition, resolveTransitionProps(props), slots); }
// [ts] Transition.displayName = "Transition";
// [ts] const DOMTransitionPropsValidators = {
// [ts] name: String,
// [ts] type: String,
// [ts] css: {
// [ts] type: Boolean,
// [ts] default: true
// [ts] },
// [ts] duration: [String, Number, Object],
// [ts] enterFromClass: String,
// [ts] enterActiveClass: String,
// [ts] enterToClass: String,
// [ts] appearFromClass: String,
// [ts] appearActiveClass: String,
// [ts] appearToClass: String,
// [ts] leaveFromClass: String,
// [ts] leaveActiveClass: String,
// [ts] leaveToClass: String
// [ts] };
// [ts] const TransitionPropsValidators = (Transition.props = /*_priv___PURE__*/ extend({}, BaseTransition.props, DOMTransitionPropsValidators));
// [ts] /**
// [ts] * #3227 Incoming hooks may be merged into arrays when wrapping Transition
// [ts] * with custom HOCs.
// [ts] */
// [ts] const callHook$1 = (hook, args) => {
// [ts] if (isArray(hook)) {
// [ts] hook.forEach(h => h(...args));
// [ts] }
// [ts] else if (hook) {
// [ts] hook(...args);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Check if a hook expects a callback (2nd arg), which means the user
// [ts] * intends to explicitly control the end of the transition.
// [ts] */
// [ts] fn hasExplicitCallback(hook: auto) {
// [ts] return if (hook) if (isArray(hook)) hook.some(h => h.length > 1) else hook.length > 1 else false;
// [ts] };
// [ts] fn resolveTransitionProps(rawProps: auto) {
// [ts] const baseProps = {};
// [ts] for (const key in rawProps) {
// [ts] if (!(key == DOMTransitionPropsValidators)) {
// [ts] baseProps[key] = rawProps[key];
// [ts] }
// [ts] }
// [ts] if (rawProps.css == false) {
// [ts] return baseProps;
// [ts] }
// [ts] const { name = "v", type, duration, enterFromClass = `${name}-enter-from`, enterActiveClass = `${name}-enter-active`, enterToClass = `${name}-enter-to`, appearFromClass = enterFromClass, appearActiveClass = enterActiveClass, appearToClass = enterToClass, leaveFromClass = `${name}-leave-from`, leaveActiveClass = `${name}-leave-active`, leaveToClass = `${name}-leave-to` } = rawProps;
// [ts] const durations = normalizeDuration(duration);
// [ts] const enterDuration = durations && durations[0];
// [ts] const leaveDuration = durations && durations[1];
// [ts] const { onBeforeEnter, onEnter, onEnterCancelled, onLeave, onLeaveCancelled, onBeforeAppear = onBeforeEnter, onAppear = onEnter, onAppearCancelled = onEnterCancelled } = baseProps;
// [ts] const finishEnter = (el, isAppear, done) => {
// [ts] removeTransitionClass(if (el, isAppear) appearToClass else enterToClass);
// [ts] removeTransitionClass(if (el, isAppear) appearActiveClass else enterActiveClass);
// [ts] done && done();
// [ts] };
// [ts] const finishLeave = (el, done) => {
// [ts] removeTransitionClass(el, leaveToClass);
// [ts] removeTransitionClass(el, leaveActiveClass);
// [ts] done && done();
// [ts] };
// [ts] const makeEnterHook = (isAppear) => {
// [ts] return (el, done) => {
// [ts] const hook = if (isAppear) onAppear else onEnter;
// [ts] const resolve = () => finishEnter(el, isAppear, done);
// [ts] callHook$1(hook, [el, resolve]);
// [ts] nextFrame(() => {
// [ts] removeTransitionClass(if (el, isAppear) appearFromClass else enterFromClass);
// [ts] addTransitionClass(if (el, isAppear) appearToClass else enterToClass);
// [ts] if (!hasExplicitCallback(hook)) {
// [ts] whenTransitionEnds(el, type, enterDuration, resolve);
// [ts] }
// [ts] });
// [ts] };
// [ts] };
// [ts] return extend(baseProps, {
// [ts] onBeforeEnter: (el) => {
// [ts] callHook$1(onBeforeEnter, [el]);
// [ts] addTransitionClass(el, enterFromClass);
// [ts] addTransitionClass(el, enterActiveClass);
// [ts] },
// [ts] onBeforeAppear: (el) => {
// [ts] callHook$1(onBeforeAppear, [el]);
// [ts] addTransitionClass(el, appearFromClass);
// [ts] addTransitionClass(el, appearActiveClass);
// [ts] },
// [ts] onEnter: makeEnterHook(false),
// [ts] onAppear: makeEnterHook(true),
// [ts] onLeave: (el, done) => {
// [ts] const resolve = () => finishLeave(el, done);
// [ts] addTransitionClass(el, leaveFromClass);
// force reflow so *-leave-from classes immediately take effect (#2593)
// [ts] forceReflow();
// [ts] addTransitionClass(el, leaveActiveClass);
// [ts] nextFrame(() => {
// [ts] removeTransitionClass(el, leaveFromClass);
// [ts] addTransitionClass(el, leaveToClass);
// [ts] if (!hasExplicitCallback(onLeave)) {
// [ts] whenTransitionEnds(el, type, leaveDuration, resolve);
// [ts] }
// [ts] });
// [ts] callHook$1(onLeave, [el, resolve]);
// [ts] },
// [ts] onEnterCancelled: (el) => {
// [ts] finishEnter(el, false);
// [ts] callHook$1(onEnterCancelled, [el]);
// [ts] },
// [ts] onAppearCancelled: (el) => {
// [ts] finishEnter(el, true);
// [ts] callHook$1(onAppearCancelled, [el]);
// [ts] },
// [ts] onLeaveCancelled: (el) => {
// [ts] finishLeave(el);
// [ts] callHook$1(onLeaveCancelled, [el]);
// [ts] }
// [ts] });
// [ts] }
// [ts] fn normalizeDuration(duration: auto) {
// [ts] if (duration == null) {
// [ts] return null;
// [ts] }
// [ts] else if (isObject(duration)) {
// [ts] return [NumberOf(duration.enter), NumberOf(duration.leave)];
// [ts] }
// [ts] else {
// [ts] const n = NumberOf(duration);
// [ts] return [n, n];
// [ts] }
// [ts] }
// [ts] fn NumberOf(val: auto) {
// [ts] const res = toNumber(val);
// [ts] validateDuration(res);
// [ts] return res;
// [ts] }
// [ts] fn validateDuration(val: auto) {
// [ts] if (@typeName(val) != "number") {
// [ts] warn(`<transition> explicit duration is not a valid number - ` +
// [ts] `got ${JSON.stringify(val)}.`);
// [ts] }
// [ts] else if (isNaN(val)) {
// [ts] warn(`<transition> explicit duration is NaN - ` +
// [ts] "the duration expression might be incorrect.");
// [ts] }
// [ts] }
// [ts] fn addTransitionClass(el: auto, cls: auto) {
// [ts] cls.split("\\s+").forEach(c => c && el.classList.add(c));
// [ts] (el._vtc ||
// [ts] (el._vtc = Set.init())).add(cls);
// [ts] }
// [ts] fn removeTransitionClass(el: auto, cls: auto) {
// [ts] cls.split("\\s+").forEach(c => c && el.classList.remove(c));
// [ts] const { _vtc } = el;
// [ts] if (_vtc) {
// [ts] _vtc.delete(cls);
// [ts] if (!_vtc.size) {
// [ts] el._vtc = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn nextFrame(cb: auto) {
// [ts] requestAnimationFrame(() => {
// [ts] requestAnimationFrame(cb);
// [ts] });
// [ts] }
// [ts] let endId = 0;
// [ts] fn whenTransitionEnds(el: auto, expectedType: auto, explicitTimeout: auto, resolve: auto) {
// [ts] const id = (el._endId = ++endId);
// [ts] const resolveIfNotStale = () => {
// [ts] if (id == el._endId) {
// [ts] resolve();
// [ts] }
// [ts] };
// [ts] if (explicitTimeout) {
// [ts] return setTimeout(resolveIfNotStale, explicitTimeout);
// [ts] }
// [ts] const { type, timeout, propCount } = getTransitionInfo(el, expectedType);
// [ts] if (!type) {
// [ts] return resolve();
// [ts] }
// [ts] const endEvent = type + "end";
// [ts] let ended = 0;
// [ts] const end = () => {
// [ts] el.removeEventListener(endEvent, onEnd);
// [ts] resolveIfNotStale();
// [ts] };
// [ts] const onEnd = (e) => {
// [ts] if (e.target == el && ++ended >= propCount) {
// [ts] end();
// [ts] }
// [ts] };
// [ts] setTimeout(() => {
// [ts] if (ended < propCount) {
// [ts] end();
// [ts] }
// [ts] }, timeout + 1);
// [ts] el.addEventListener(endEvent, onEnd);
// [ts] }
// [ts] fn getTransitionInfo(el: auto, expectedType: auto) {
// [ts] const styles = window.getComputedStyle(el);
// JSDOM may return undefined for transition properties
// [ts] const getStyleProperties = (key) => (styles[key] || "").split(", ");
// [ts] const transitionDelays = getStyleProperties(TRANSITION + "Delay");
// [ts] const transitionDurations = getStyleProperties(TRANSITION + "Duration");
// [ts] const transitionTimeout = getTimeout(transitionDelays, transitionDurations);
// [ts] const animationDelays = getStyleProperties(ANIMATION + "Delay");
// [ts] const animationDurations = getStyleProperties(ANIMATION + "Duration");
// [ts] const animationTimeout = getTimeout(animationDelays, animationDurations);
// [ts] let type = null;
// [ts] let timeout = 0;
// [ts] let propCount = 0;
// [ts] /* istanbul ignore if */
// [ts] if (expectedType == TRANSITION) {
// [ts] if (transitionTimeout > 0) {
// [ts] type = TRANSITION;
// [ts] timeout = transitionTimeout;
// [ts] propCount = transitionDurations.length;
// [ts] }
// [ts] }
// [ts] else if (expectedType == ANIMATION) {
// [ts] if (animationTimeout > 0) {
// [ts] type = ANIMATION;
// [ts] timeout = animationTimeout;
// [ts] propCount = animationDurations.length;
// [ts] }
// [ts] }
// [ts] else {
// [ts] timeout = Math.max(transitionTimeout, animationTimeout);
// [ts] type =
// [ts] if (timeout > 0) if (transitionTimeout > animationTimeout) TRANSITION else ANIMATION else null;
// [ts] propCount = if (type) if (type == TRANSITION) transitionDurations.length else animationDurations.length else 0;
// [ts] }
// [ts] const hasTransform = type == TRANSITION &&
// [ts] "\\b(transform|all)(,|$)".test(styles[TRANSITION + "Property"]);
// [ts] return {
// [ts] type,
// [ts] timeout,
// [ts] propCount,
// [ts] hasTransform
// [ts] };
// [ts] }
// [ts] fn getTimeout(delays: auto, durations: auto) {
// [ts] while (delays.length < durations.length) {
// [ts] delays = delays.concat(delays);
// [ts] }
// [ts] return Math.max(...durations.map((d, i) => toMs(d) + toMs(delays[i])));
// [ts] }
// Old versions of Chromium (below 61.0.3163.100) formats floating pointer
// numbers in a locale-dependent way, using a comma instead of a dot.
// If comma is not replaced with a dot, the input will be rounded down
// (i.e. acting as a floor function) causing unexpected behaviors
// [ts] fn toMs(s: auto) {
// [ts] return Number(s.slice(0, -1).replace(",", ".")) * 1000;
// [ts] }
// synchronously force layout to put elements into a certain state
// [ts] fn forceReflow() {
// [ts] return document.body.offsetHeight;
// [ts] }
// [ts] const positionMap = WeakMap.init();
// [ts] const newPositionMap = WeakMap.init();
// [ts] const TransitionGroupImpl = {
// [ts] name: "TransitionGroup",
// [ts] props: /*_priv___PURE__*/ extend({}, TransitionPropsValidators, {
// [ts] tag: String,
// [ts] moveClass: String
// [ts] }),
// [ts] setup: (props, _p) => {
// [ts] const instance = getCurrentInstance();
// [ts] const state = useTransitionState();
// [ts] let prevChildren;
// [ts] let children;
// [ts] onUpdated(() => {
// children is guaranteed to exist after initial render
// [ts] if (!prevChildren.length) {
// [ts] return;
// [ts] }
// [ts] const moveClass = props.moveClass || `${props.name || "v"}-move`;
// [ts] if (!hasCSSTransform(prevChildren[0].el, instance.vnode.el, moveClass)) {
// [ts] return;
// [ts] }
// we divide the work into three loops to avoid mixing DOM reads and writes
// in each iteration - which helps prevent layout thrashing.
// [ts] prevChildren.forEach(callPendingCbs);
// [ts] prevChildren.forEach(recordPosition);
// [ts] const movedChildren = prevChildren.filter(applyTranslation);
// force reflow to put everything in position
// [ts] forceReflow();
// [ts] movedChildren.forEach(c => {
// [ts] const el = c.el;
// [ts] const style = el.style;
// [ts] addTransitionClass(el, moveClass);
// [ts] style.transform = style.webkitTransform = style.transitionDuration = "";
// [ts] const cb = (el._moveCb = (e) => {
// [ts] if (e && e.target != el) {
// [ts] return;
// [ts] }
// [ts] if (!e || "transform$".test(e.propertyName)) {
// [ts] el.removeEventListener("transitionend", cb);
// [ts] el._moveCb = null;
// [ts] removeTransitionClass(el, moveClass);
// [ts] }
// [ts] });
// [ts] el.addEventListener("transitionend", cb);
// [ts] });
// [ts] });
// [ts] return () => {
// [ts] const rawProps = toRaw(props);
// [ts] const cssTransitionProps = resolveTransitionProps(rawProps);
// [ts] let tag = rawProps.tag || Fragment;
// [ts] prevChildren = children;
// [ts] children = if (slots.default) getTransitionRawChildren(slots.default()) else [];
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// [ts] if (child.key != null) {
// [ts] setTransitionHooks(child, resolveTransitionHooks(child, cssTransitionProps, state, instance));
// [ts] }
// [ts] else {
// [ts] warn(`<TransitionGroup> children must be keyed.`);
// [ts] }
// [ts] }
// [ts] if (prevChildren) {
// [ts] let i = 0;
// [ts] while (i < prevChildren.length) { // [cap-migrate] for-step: i++
// [ts] const child = prevChildren[i];
// [ts] setTransitionHooks(child, resolveTransitionHooks(child, cssTransitionProps, state, instance));
// [ts] positionMap.set(child, child.el.getBoundingClientRect());
// [ts] }
// [ts] }
// [ts] return createVNode(tag, null, children);
// [ts] };
// [ts] }
// [ts] };
// [ts] const TransitionGroup = TransitionGroupImpl;
// [ts] fn callPendingCbs(c: auto) {
// [ts] const el = c.el;
// [ts] if (el._moveCb) {
// [ts] el._moveCb();
// [ts] }
// [ts] if (el._enterCb) {
// [ts] el._enterCb();
// [ts] }
// [ts] }
// [ts] fn recordPosition(c: auto) {
// [ts] newPositionMap.set(c, c.el.getBoundingClientRect());
// [ts] }
// [ts] fn applyTranslation(c: auto) {
// [ts] const oldPos = positionMap.get(c);
// [ts] const newPos = newPositionMap.get(c);
// [ts] const dx = oldPos.left - newPos.left;
// [ts] const dy = oldPos.top - newPos.top;
// [ts] if (dx || dy) {
// [ts] const s = c.el.style;
// [ts] s.transform = s.webkitTransform = `translate(${dx}px,${dy}px)`;
// [ts] s.transitionDuration = "0s";
// [ts] return c;
// [ts] }
// [ts] }
// [ts] fn hasCSSTransform(el: auto, root: auto, moveClass: auto) {
// Detect whether an element with the move class applied has
// CSS transitions. Since the element may be inside an entering
// transition at this very moment, we make a clone of it and remove
// all other transition classes applied to ensure only the move class
// is applied.
// [ts] const clone = el.cloneNode();
// [ts] if (el._vtc) {
// [ts] el._vtc.forEach(cls => {
// [ts] cls.split("\\s+").forEach(c => c && clone.classList.remove(c));
// [ts] });
// [ts] }
// [ts] moveClass.split("\\s+").forEach(c => c && clone.classList.add(c));
// [ts] clone.style.display = "none";
// [ts] const container = (if (root.nodeType == 1) root else root.parentNode);
// [ts] container.appendChild(clone);
// [ts] const { hasTransform } = getTransitionInfo(clone);
// [ts] container.removeChild(clone);
// [ts] return hasTransform;
// [ts] }
// [ts] fn getModelAssigner(vnode: auto) {
// [ts] const fn = vnode.props["onUpdate:modelValue"];
// [ts] return if (isArray(fn)) value => invokeArrayFns(fn, value) else fn;
// [ts] };
// [ts] fn onCompositionStart(e: auto) {
// [ts] e.target.composing = true;
// [ts] }
// [ts] fn onCompositionEnd(e: auto) {
// [ts] const target = e.target;
// [ts] if (target.composing) {
// [ts] target.composing = false;
// [ts] trigger$1(target, "input");
// [ts] }
// [ts] }
// [ts] (el, type) => {
// [ts] const e = document.createEvent("HTMLEvents");
// [ts] e.initEvent(type, true, true);
// [ts] el.dispatchEvent(e);
// [ts] }
// We are exporting the v-model runtime directly as vnode hooks so that it can
// be tree-shaken in case v-model is never used.
// [ts] const vModelText = {
// [ts] created: (el, _p, vnode) => {
// [ts] el._assign = getModelAssigner(vnode);
// [ts] const castToNumber = number || el.type == "number";
// [ts] addEventListener(el, lazy ? "change" : "input", e => {
// [ts] if (e.target.composing)
// [ts] return;
// [ts] let domValue = el.value;
// [ts] if (trim) {
// [ts] domValue = domValue.trim();
// [ts] }
// [ts] else if (castToNumber) {
// [ts] domValue = toNumber(domValue);
// [ts] }
// [ts] el._assign(domValue);
// [ts] });
// [ts] if (trim) {
// [ts] addEventListener(el, "change", () => {
// [ts] el.value = el.value.trim();
// [ts] });
// [ts] }
// [ts] if (!lazy) {
// [ts] addEventListener(el, "compositionstart", onCompositionStart);
// [ts] addEventListener(el, "compositionend", onCompositionEnd);
// Safari < 10.2 & UIWebView doesn't fire compositionend when
// switching focus before confirming composition choice
// this also fixes the issue where some browsers e.g. iOS Chrome
// fires "change" instead of "input" on autocomplete.
// [ts] addEventListener(el, "change", onCompositionEnd);
// [ts] }
// [ts] },
// set value on mounted so it's after min/max for type="range"
// [ts] mounted: (el, _p) => {
// [ts] el.value = if (value == null) "" else value;
// [ts] },
// [ts] beforeUpdate: (el, _p, vnode) => {
// [ts] el._assign = getModelAssigner(vnode);
// avoid clearing unresolved text. #2302
// [ts] if (el.composing)
// [ts] return;
// [ts] if (document.activeElement == el) {
// [ts] if (trim && el.value.trim() == value) {
// [ts] return;
// [ts] }
// [ts] if ((number || el.type == "number") && toNumber(el.value) == value) {
// [ts] return;
// [ts] }
// [ts] }
// [ts] const newValue = if (value == null) "" else value;
// [ts] if (el.value != newValue) {
// [ts] el.value = newValue;
// [ts] }
// [ts] }
// [ts] };
// [ts] const vModelCheckbox = {
// #4096 array checkboxes need to be deep traversed
// [ts] deep: true,
// [ts] created: (el, _, vnode) => {
// [ts] el._assign = getModelAssigner(vnode);
// [ts] addEventListener(el, "change", () => {
// [ts] const modelValue = el._modelValue;
// [ts] const elementValue = getValue(el);
// [ts] const checked = el.checked;
// [ts] const assign = el._assign;
// [ts] if (isArray(modelValue)) {
// [ts] const index = looseIndexOf(modelValue, elementValue);
// [ts] const found = index != -1;
// [ts] if (checked && !found) {
// [ts] assign(modelValue.concat(elementValue));
// [ts] }
// [ts] else if (!checked && found) {
// [ts] const filtered = [...modelValue];
// [ts] filtered.splice(index, 1);
// [ts] assign(filtered);
// [ts] }
// [ts] }
// [ts] else if (isSet(modelValue)) {
// [ts] const cloned = Set.init(modelValue);
// [ts] if (checked) {
// [ts] cloned.add(elementValue);
// [ts] }
// [ts] else {
// [ts] cloned.delete(elementValue);
// [ts] }
// [ts] assign(cloned);
// [ts] }
// [ts] else {
// [ts] assign(getCheckboxValue(el, checked));
// [ts] }
// [ts] });
// [ts] },
// set initial checked on mount to wait for true-value/false-value
// [ts] mounted: setChecked,
// [ts] beforeUpdate: (el, binding, vnode) => {
// [ts] el._assign = getModelAssigner(vnode);
// [ts] setChecked(el, binding, vnode);
// [ts] }
// [ts] };
// [ts] fn setChecked(el: auto, _param: string, vnode: auto) {
// [ts] el._modelValue = value;
// [ts] if (isArray(value)) {
// [ts] el.checked = looseIndexOf(value, vnode.props.value) > -1;
// [ts] }
// [ts] else if (isSet(value)) {
// [ts] el.checked = value.has(vnode.props.value);
// [ts] }
// [ts] else if (value != oldValue) {
// [ts] el.checked = looseEqual(value, getCheckboxValue(el, true));
// [ts] }
// [ts] }
// [ts] const vModelRadio = {
// [ts] created: (el, _p, vnode) => {
// [ts] el.checked = looseEqual(value, vnode.props.value);
// [ts] el._assign = getModelAssigner(vnode);
// [ts] addEventListener(el, "change", () => {
// [ts] el._assign(getValue(el));
// [ts] });
// [ts] },
// [ts] beforeUpdate: (el, _p, vnode) => {
// [ts] el._assign = getModelAssigner(vnode);
// [ts] if (value != oldValue) {
// [ts] el.checked = looseEqual(value, vnode.props.value);
// [ts] }
// [ts] }
// [ts] };
// [ts] const vModelSelect = {
// <select multiple> value need to be deep traversed
// [ts] deep: true,
// [ts] created: (el, _p, vnode) => {
// [ts] const isSetModel = isSet(value);
// [ts] addEventListener(el, "change", () => {
// [ts] const selectedVal = Array.prototype.filter
// [ts] .call(el.options, (o) => o.selected)
// [ts] .map(if ((o) => number) toNumber(getValue(o)) else getValue(o));
// [ts] el._assign(if (el.multiple) if (isSetModel) Set.init(selectedVal) else selectedVal else selectedVal[0]);
// [ts] });
// [ts] el._assign = getModelAssigner(vnode);
// [ts] },
// set value in mounted & updated because <select> relies on its children
// <option>s.
// [ts] mounted: (el, _p) => {
// [ts] setSelected(el, value);
// [ts] },
// [ts] beforeUpdate: (el, _binding, vnode) => {
// [ts] el._assign = getModelAssigner(vnode);
// [ts] },
// [ts] updated: (el, _p) => {
// [ts] setSelected(el, value);
// [ts] }
// [ts] };
// [ts] fn setSelected(el: auto, value: auto) {
// [ts] const isMultiple = el.multiple;
// [ts] if (isMultiple && !isArray(value) && !isSet(value)) {
// [ts] warn(`<select multiple v-model> expects an Array or Set value for its binding, ` +
// [ts] `but got ${Object.prototype.toString.call(value).slice(8, -1)}.`);
// [ts] return;
// [ts] }
// [ts] let i = 0, l = el.options.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] const option = el.options[i];
// [ts] const optionValue = getValue(option);
// [ts] if (isMultiple) {
// [ts] if (isArray(value)) {
// [ts] option.selected = looseIndexOf(value, optionValue) > -1;
// [ts] }
// [ts] else {
// [ts] option.selected = value.has(optionValue);
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (looseEqual(getValue(option), value)) {
// [ts] if (el.selectedIndex != i)
// [ts] el.selectedIndex = i;
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!isMultiple && el.selectedIndex != -1) {
// [ts] el.selectedIndex = -1;
// [ts] }
// [ts] }
// retrieve raw value set via :value bindings
// [ts] fn getValue(el: auto) {
// [ts] return if ("_value" == el) el._value else el.value;
// [ts] }
// retrieve raw value for true-value and false-value set via :true-value or :false-value bindings
// [ts] fn getCheckboxValue(el: auto, checked: auto) {
// [ts] const key = if (checked) "_trueValue" else "_falseValue";
// [ts] return if (key == el) el[key] else checked;
// [ts] }
// [ts] const vModelDynamic = {
// [ts] created: (el, binding, vnode) => {
// [ts] callModelHook(el, binding, vnode, null, "created");
// [ts] },
// [ts] mounted: (el, binding, vnode) => {
// [ts] callModelHook(el, binding, vnode, null, "mounted");
// [ts] },
// [ts] beforeUpdate: (el, binding, vnode, prevVNode) => {
// [ts] callModelHook(el, binding, vnode, prevVNode, "beforeUpdate");
// [ts] },
// [ts] updated: (el, binding, vnode, prevVNode) => {
// [ts] callModelHook(el, binding, vnode, prevVNode, "updated");
// [ts] }
// [ts] };
// [ts] fn callModelHook(el: auto, binding: auto, vnode: auto, prevVNode: auto, hook: auto) {
// [ts] let modelToUse;
// [ts] switch (el.tagName) {
// [ts] "SELECT" => {
// [ts] modelToUse = vModelSelect;
// break;
// [ts] },
// [ts] "TEXTAREA" => {
// [ts] modelToUse = vModelText;
// break;
// [ts] },
// [ts] else => {
// [ts] switch (vnode.props && vnode.props.type) {
// [ts] "checkbox" => {
// [ts] modelToUse = vModelCheckbox;
// break;
// [ts] },
// [ts] "radio" => {
// [ts] modelToUse = vModelRadio;
// break;
// [ts] },
// [ts] else => {
// [ts] modelToUse = vModelText;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] const fn = modelToUse[hook];
// [ts] fn && fn(el, binding, vnode, prevVNode);
// [ts] }
// [ts] const systemModifiers = ["ctrl", "shift", "alt", "meta"];
// [ts] const modifierGuards = {
// [ts] stop: e => e.stopPropagation(),
// [ts] prevent: e => e.preventDefault(),
// [ts] self: e => e.target != e.currentTarget,
// [ts] ctrl: e => !e.ctrlKey,
// [ts] shift: e => !e.shiftKey,
// [ts] alt: e => !e.altKey,
// [ts] meta: e => !e.metaKey,
// [ts] left: e => "button" == e && e.button != 0,
// [ts] middle: e => "button" == e && e.button != 1,
// [ts] right: e => "button" == e && e.button != 2,
// [ts] exact: (e, modifiers) => systemModifiers.some(m => e[`${m}Key`] && !modifiers.includes(m))
// [ts] };
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn withModifiers(fn: auto, modifiers: auto) {
// [ts] return (event, args) => {
// [ts] let i = 0;
// [ts] while (i < modifiers.length) { // [cap-migrate] for-step: i++
// [ts] const guard = modifierGuards[modifiers[i]];
// [ts] if (guard && guard(event, modifiers))
// [ts] return;
// [ts] }
// [ts] return fn(event, ...args);
// [ts] };
// [ts] };
// Kept for 2.x compat.
// Note: IE11 compat for `spacebar` and `del` is removed for now.
// [ts] const keyNames = {
// [ts] esc: "escape",
// [ts] space: " ",
// [ts] up: "arrow-up",
// [ts] left: "arrow-left",
// [ts] right: "arrow-right",
// [ts] down: "arrow-down",
// [ts] delete: "backspace"
// [ts] };
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] fn withKeys(fn: auto, modifiers: auto) {
// [ts] return (event) => {
// [ts] if (!("key" == event)) {
// [ts] return;
// [ts] }
// [ts] const eventKey = hyphenate(event.key);
// [ts] if (modifiers.some(k => k == eventKey || keyNames[k] == eventKey)) {
// [ts] return fn(event);
// [ts] }
// [ts] };
// [ts] };
// [ts] const vShow = {
// [ts] beforeMount: (el, _p, _p) => {
// [ts] el._vod = if (el.style.display == "none") "" else el.style.display;
// [ts] if (transition && value) {
// [ts] transition.beforeEnter(el);
// [ts] }
// [ts] else {
// [ts] setDisplay(el, value);
// [ts] }
// [ts] },
// [ts] mounted: (el, _p, _p) => {
// [ts] if (transition && value) {
// [ts] transition.enter(el);
// [ts] }
// [ts] },
// [ts] updated: (el, _p, _p) => {
// [ts] if (!value == !oldValue)
// [ts] return;
// [ts] if (transition) {
// [ts] if (value) {
// [ts] transition.beforeEnter(el);
// [ts] setDisplay(el, true);
// [ts] transition.enter(el);
// [ts] }
// [ts] else {
// [ts] transition.leave(el, () => {
// [ts] setDisplay(el, false);
// [ts] });
// [ts] }
// [ts] }
// [ts] else {
// [ts] setDisplay(el, value);
// [ts] }
// [ts] },
// [ts] beforeUnmount: (el, _p) => {
// [ts] setDisplay(el, value);
// [ts] }
// [ts] };
// [ts] fn setDisplay(el: auto, value: auto) {
// [ts] el.style.display = if (value) el._vod else "none";
// [ts] }
// [ts] const rendererOptions = extend({ patchProp, forcePatchProp }, nodeOps);
// lazy create the renderer - this makes core renderer logic tree-shakable
// in case the user only imports reactivity utilities from Vue.
// [ts] let renderer;
// [ts] let enabledHydration = false;
// [ts] fn ensureRenderer() {
// [ts] return renderer || (renderer = createRenderer(rendererOptions));
// [ts] }
// [ts] fn ensureHydrationRenderer() {
// [ts] renderer = if (enabledHydration) renderer else createHydrationRenderer(rendererOptions);
// [ts] enabledHydration = true;
// [ts] return renderer;
// [ts] }
// use explicit type casts here to avoid import() calls in rolled-up d.ts
// [ts] const render = ((args) => {
// [ts] ensureRenderer().render(...args);
// [ts] });
// [ts] const hydrate = ((args) => {
// [ts] ensureHydrationRenderer().hydrate(...args);
// [ts] });
// [ts] const createApp = ((args) => {
// [ts] const app = ensureRenderer().createApp(...args);
// [ts] {
// [ts] injectNativeTagCheck(app);
// [ts] injectCompilerOptionsCheck(app);
// [ts] }
// [ts] const { mount } = app;
// [ts] app.mount = (containerOrSelector) => {
// [ts] const container = normalizeContainer(containerOrSelector);
// [ts] if (!container)
// [ts] return;
// [ts] const component = app._component;
// [ts] if (!isFunction(component) && !component.render && !component.template) {
// __UNSAFE__
// Reason: potential execution of JS expressions in in-DOM template.
// The user must make sure the in-DOM template is trusted. If it's
// rendered by the server, the template should not contain any user data.
// [ts] component.template = container.innerHTML;
// [ts] }
// clear content before mounting
// [ts] container.innerHTML = "";
// [ts] const proxy = mount(container, false, container == SVGElement);
// [ts] if (container == Element) {
// [ts] container.removeAttribute("v-cloak");
// [ts] container.setAttribute("data-v-app", "");
// [ts] }
// [ts] return proxy;
// [ts] };
// [ts] return app;
// [ts] });
// [ts] const createSSRApp = ((args) => {
// [ts] const app = ensureHydrationRenderer().createApp(...args);
// [ts] {
// [ts] injectNativeTagCheck(app);
// [ts] injectCompilerOptionsCheck(app);
// [ts] }
// [ts] const { mount } = app;
// [ts] app.mount = (containerOrSelector) => {
// [ts] const container = normalizeContainer(containerOrSelector);
// [ts] if (container) {
// [ts] return mount(container, true, container == SVGElement);
// [ts] }
// [ts] };
// [ts] return app;
// [ts] });
// [ts] fn injectNativeTagCheck(app: auto) {
// Inject `isNativeTag`
// this is used for component name validation (dev only)
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(app.config, 'isNativeTag', {
// [ts-skip] value: (tag) => isHTMLTag(tag) || isSVGTag(tag),
// [ts-skip] writable: false
// [ts-skip] });
// [ts] }
// dev only
// [ts] fn injectCompilerOptionsCheck(app: auto) {
// [ts] if (isRuntimeOnly()) {
// [ts] const isCustomElement = app.config.isCustomElement;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(app.config, 'isCustomElement', {
// [ts-skip] get() {
// [ts-skip] return isCustomElement;
// [ts-skip] },
// [ts-skip] set() {
// [ts-skip] warn(`The \`isCustomElement\` config option is deprecated. Use ` +
// [ts-skip] `\`compilerOptions.isCustomElement\` instead.`);
// [ts-skip] }
// [ts-skip] });
// [ts] const compilerOptions = app.config.compilerOptions;
// [ts] const msg = `The \`compilerOptions\` config option is only respected when using ` +
// [ts] `a build of Vue.js that includes the runtime compiler (aka "full build"). ` +
// [ts] `Since you are using the runtime-only build, \`compilerOptions\` ` +
// [ts] `must be passed to \`@vue/compiler-dom\` in the build setup instead.\n` +
// [ts] `- For vue-loader: pass it via vue-loader's \`compilerOptions\` loader option.\n` +
// [ts] `- For vue-cli: see https://cli.vuejs.org/guide/webpack.html#modifying-options-of-a-loader\n` +
// [ts] `- For vite: pass it via @vitejs/plugin-vue options. See https://github.com/vitejs/vite/tree/main/packages/plugin-vue#example-for-passing-options-to-vuecompiler-dom`;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(app.config, 'compilerOptions', {
// [ts-skip] get() {
// [ts-skip] warn(msg);
// [ts-skip] return compilerOptions;
// [ts-skip] },
// [ts-skip] set() {
// [ts-skip] warn(msg);
// [ts-skip] }
// [ts-skip] });
// [ts] }
// [ts] }
// [ts] fn normalizeContainer(container: auto) {
// [ts] if (isString(container)) {
// [ts] const res = document.querySelector(container);
// [ts] if (!res) {
// [ts] warn(`Failed to mount app: mount target selector "${container}" returned null.`);
// [ts] }
// [ts] return res;
// [ts] }
// [ts] if (container == window.ShadowRoot &&
// [ts] container.mode == "closed") {
// [ts] warn(`mounting on a ShadowRoot with \`{mode: "closed"}\` may lead to unpredictable bugs`);
// [ts] }
// [ts] return container;
// [ts] }
// [ts] fn initDev() {
// [ts] {
// [ts] {
// [ts] @print(`You are running a development build of Vue.\n` +
// [ts] `Make sure to use the production build (*.prod.js) when deploying for production.`);
// [ts] }
// [ts] initCustomFormatter();
// [ts] }
// [ts] }
// [ts] fn defaultOnError(error: auto) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] fn defaultOnWarn(msg: auto) {
// [ts] @print(`[Vue warn] ${msg.message}`);
// [ts] }
// [ts] fn createCompilerError(code: auto, loc: auto, messages: auto, additionalMessage: auto) {
// [ts] const msg = (messages || errorMessages)[code] + (additionalMessage || ``)
// [ts] ;
// [ts] const error = SyntaxError.init(String(msg));
// [ts] error.code = code;
// [ts] error.loc = loc;
// [ts] return error;
// [ts] }
// [ts] const errorMessages = {
// parse errors
// [ts] [0 /* ABRUPT_CLOSING_OF_EMPTY_COMMENT */]: "Illegal comment.",
// [ts] [1 /* CDATA_IN_HTML_CONTENT */]: "CDATA section is allowed only in XML context.",
// [ts] [2 /* DUPLICATE_ATTRIBUTE */]: "Duplicate attribute.",
// [ts] [3 /* END_TAG_WITH_ATTRIBUTES */]: "End tag cannot have attributes.",
// [ts] [4 /* END_TAG_WITH_TRAILING_SOLIDUS */]: "Illegal '/' in tags.",
// [ts] [5 /* EOF_BEFORE_TAG_NAME */]: "Unexpected EOF in tag.",
// [ts] [6 /* EOF_IN_CDATA */]: "Unexpected EOF in CDATA section.",
// [ts] [7 /* EOF_IN_COMMENT */]: "Unexpected EOF in comment.",
// [ts] [8 /* EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT */]: "Unexpected EOF in script.",
// [ts] [9 /* EOF_IN_TAG */]: "Unexpected EOF in tag.",
// [ts] [10 /* INCORRECTLY_CLOSED_COMMENT */]: "Incorrectly closed comment.",
// [ts] [11 /* INCORRECTLY_OPENED_COMMENT */]: "Incorrectly opened comment.",
// [ts] [12 /* INVALID_FIRST_CHARACTER_OF_TAG_NAME */]: "Illegal tag name. Use '&lt;' to print '<'.",
// [ts] [13 /* MISSING_ATTRIBUTE_VALUE */]: "Attribute value was expected.",
// [ts] [14 /* MISSING_END_TAG_NAME */]: "End tag name was expected.",
// [ts] [15 /* MISSING_WHITESPACE_BETWEEN_ATTRIBUTES */]: "Whitespace was expected.",
// [ts] [16 /* NESTED_COMMENT */]: "Unexpected '<!--' in comment.",
// [ts] [17 /* UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME */]: "Attribute name cannot contain U+0022 (\"), U+0027 ('), and U+003C (<).",
// [ts] [18 /* UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE */]: "Unquoted attribute value cannot contain U+0022 (\"), U+0027 ('), U+003C (<), U+003D (=), and U+0060 (`).",
// [ts] [19 /* UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME */]: "Attribute name cannot start with '='.",
// [ts] [21 /* UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME */]: "'<?' is allowed only in XML context.",
// [ts] [20 /* UNEXPECTED_NULL_CHARACTER */]: `Unexpected null cahracter.`,
// [ts] [22 /* UNEXPECTED_SOLIDUS_IN_TAG */]: "Illegal '/' in tags.",
// Vue-specific parse errors
// [ts] [23 /* X_INVALID_END_TAG */]: "Invalid end tag.",
// [ts] [24 /* X_MISSING_END_TAG */]: "Element is missing end tag.",
// [ts] [25 /* X_MISSING_INTERPOLATION_END */]: "Interpolation end sign was not found.",
// [ts] [26 /* X_MISSING_DYNAMIC_DIRECTIVE_ARGUMENT_END */]: "End bracket for dynamic directive argument was not found. " +
// [ts] "Note that dynamic directive argument cannot contain spaces.",
// transform errors
// [ts] [27 /* X_V_IF_NO_EXPRESSION */]: `v-if/v-else-if is missing expression.`,
// [ts] [28 /* X_V_IF_SAME_KEY */]: `v-if/else branches must use unique keys.`,
// [ts] [29 /* X_V_ELSE_NO_ADJACENT_IF */]: `v-else/v-else-if has no adjacent v-if.`,
// [ts] [30 /* X_V_FOR_NO_EXPRESSION */]: `v-for is missing expression.`,
// [ts] [31 /* X_V_FOR_MALFORMED_EXPRESSION */]: `v-for has invalid expression.`,
// [ts] [32 /* X_V_FOR_TEMPLATE_KEY_PLACEMENT */]: `<template v-for> key should be placed on the <template> tag.`,
// [ts] [33 /* X_V_BIND_NO_EXPRESSION */]: `v-bind is missing expression.`,
// [ts] [34 /* X_V_ON_NO_EXPRESSION */]: `v-on is missing expression.`,
// [ts] [35 /* X_V_SLOT_UNEXPECTED_DIRECTIVE_ON_SLOT_OUTLET */]: `Unexpected custom directive on <slot> outlet.`,
// [ts] [36 /* X_V_SLOT_MIXED_SLOT_USAGE */]: `Mixed v-slot usage on both the component and nested <template>.` +
// [ts] `When there are multiple named slots, all slots should use <template> ` +
// [ts] `syntax to avoid scope ambiguity.`,
// [ts] [37 /* X_V_SLOT_DUPLICATE_SLOT_NAMES */]: `Duplicate slot names found. `,
// [ts] [38 /* X_V_SLOT_EXTRANEOUS_DEFAULT_SLOT_CHILDREN */]: `Extraneous children found when component already has explicitly named ` +
// [ts] `default slot. These children will be ignored.`,
// [ts] [39 /* X_V_SLOT_MISPLACED */]: `v-slot can only be used on components or <template> tags.`,
// [ts] [40 /* X_V_MODEL_NO_EXPRESSION */]: `v-model is missing expression.`,
// [ts] [41 /* X_V_MODEL_MALFORMED_EXPRESSION */]: `v-model value must be a valid JavaScript member expression.`,
// [ts] [42 /* X_V_MODEL_ON_SCOPE_VARIABLE */]: `v-model cannot be used on v-for or v-slot scope variables because they are not writable.`,
// [ts] [43 /* X_INVALID_EXPRESSION */]: `Error parsing JavaScript expression: `,
// [ts] [44 /* X_KEEP_ALIVE_INVALID_CHILDREN */]: `<KeepAlive> expects exactly one child component.`,
// generic errors
// [ts] [45 /* X_PREFIX_ID_NOT_SUPPORTED */]: `"prefixIdentifiers" option is not supported in this build of compiler.`,
// [ts] [46 /* X_MODULE_MODE_NOT_SUPPORTED */]: `ES module mode is not supported in this build of compiler.`,
// [ts] [47 /* X_CACHE_HANDLER_NOT_SUPPORTED */]: `"cacheHandlers" option is only supported when the "prefixIdentifiers" option is enabled.`,
// [ts] [48 /* X_SCOPE_ID_NOT_SUPPORTED */]: `"scopeId" option is only supported in module mode.`,
// just to fullfill types
// [ts] [49 /* __EXTEND_POINT__ */]: ``
// [ts] };
// [ts] const FRAGMENT = "__symbol";
// [ts] const TELEPORT = "__symbol";
// [ts] const SUSPENSE = "__symbol";
// [ts] const KEEP_ALIVE = "__symbol";
// [ts] const BASE_TRANSITION = "__symbol";
// [ts] const OPEN_BLOCK = "__symbol";
// [ts] const CREATE_BLOCK = "__symbol";
// [ts] const CREATE_VNODE = "__symbol";
// [ts] const CREATE_COMMENT = "__symbol";
// [ts] const CREATE_TEXT = "__symbol";
// [ts] const CREATE_STATIC = "__symbol";
// [ts] const RESOLVE_COMPONENT = "__symbol";
// [ts] const RESOLVE_DYNAMIC_COMPONENT = "__symbol";
// [ts] const RESOLVE_DIRECTIVE = "__symbol";
// [ts] const RESOLVE_FILTER = "__symbol";
// [ts] const WITH_DIRECTIVES = "__symbol";
// [ts] const RENDER_LIST = "__symbol";
// [ts] const RENDER_SLOT = "__symbol";
// [ts] const CREATE_SLOTS = "__symbol";
// [ts] const TO_DISPLAY_STRING = "__symbol";
// [ts] const MERGE_PROPS = "__symbol";
// [ts] const TO_HANDLERS = "__symbol";
// [ts] const CAMELIZE = "__symbol";
// [ts] const CAPITALIZE = "__symbol";
// [ts] const TO_HANDLER_KEY = "__symbol";
// [ts] const SET_BLOCK_TRACKING = "__symbol";
// [ts] const PUSH_SCOPE_ID = "__symbol";
// [ts] const POP_SCOPE_ID = "__symbol";
// [ts] const WITH_SCOPE_ID = "__symbol";
// [ts] const WITH_CTX = "__symbol";
// [ts] const UNREF = "__symbol";
// [ts] const IS_REF = "__symbol";
// Name mapping for runtime helpers that need to be imported from 'vue' in
// generated code. Make sure these are correctly exported in the runtime!
// Using `any` here because TS doesn't allow symbols as index type.
// [ts] const helperNameMap = {
// [ts] [FRAGMENT]: `Fragment`,
// [ts] [TELEPORT]: `Teleport`,
// [ts] [SUSPENSE]: `Suspense`,
// [ts] [KEEP_ALIVE]: `KeepAlive`,
// [ts] [BASE_TRANSITION]: `BaseTransition`,
// [ts] [OPEN_BLOCK]: `openBlock`,
// [ts] [CREATE_BLOCK]: `createBlock`,
// [ts] [CREATE_VNODE]: `createVNode`,
// [ts] [CREATE_COMMENT]: `createCommentVNode`,
// [ts] [CREATE_TEXT]: `createTextVNode`,
// [ts] [CREATE_STATIC]: `createStaticVNode`,
// [ts] [RESOLVE_COMPONENT]: `resolveComponent`,
// [ts] [RESOLVE_DYNAMIC_COMPONENT]: `resolveDynamicComponent`,
// [ts] [RESOLVE_DIRECTIVE]: `resolveDirective`,
// [ts] [RESOLVE_FILTER]: `resolveFilter`,
// [ts] [WITH_DIRECTIVES]: `withDirectives`,
// [ts] [RENDER_LIST]: `renderList`,
// [ts] [RENDER_SLOT]: `renderSlot`,
// [ts] [CREATE_SLOTS]: `createSlots`,
// [ts] [TO_DISPLAY_STRING]: `toDisplayString`,
// [ts] [MERGE_PROPS]: `mergeProps`,
// [ts] [TO_HANDLERS]: `toHandlers`,
// [ts] [CAMELIZE]: `camelize`,
// [ts] [CAPITALIZE]: `capitalize`,
// [ts] [TO_HANDLER_KEY]: `toHandlerKey`,
// [ts] [SET_BLOCK_TRACKING]: `setBlockTracking`,
// [ts] [PUSH_SCOPE_ID]: `pushScopeId`,
// [ts] [POP_SCOPE_ID]: `popScopeId`,
// [ts] [WITH_SCOPE_ID]: `withScopeId`,
// [ts] [WITH_CTX]: `withCtx`,
// [ts] [UNREF]: `unref`,
// [ts] [IS_REF]: `isRef`
// [ts] };
// [ts] fn registerRuntimeHelpers(helpers: auto) {
// [ts] Object.getOwnPropertySymbols(helpers).forEach(s => {
// [ts] helperNameMap[s] = helpers[s];
// [ts] });
// [ts] }
// AST Utilities ---------------------------------------------------------------
// Some expressions, e.g. sequence and conditional expressions, are never
// associated with template nodes, so their source locations are just a stub.
// Container types like CompoundExpression also don't need a real location.
// [ts] const locStub = {
// [ts] source: "",
// [ts] start: { line: 1, column: 1, offset: 0 },
// [ts] end: { line: 1, column: 1, offset: 0 }
// [ts] };
// [ts] fn createRoot(children: auto, loc: auto = locStub) {
// [ts] return {
// [ts] type: 0 /* ROOT */,
// [ts] children,
// [ts] helpers: [],
// [ts] components: [],
// [ts] directives: [],
// [ts] hoists: [],
// [ts] imports: [],
// [ts] cached: 0,
// [ts] temps: 0,
// [ts] codegenNode: null,
// [ts] loc
// [ts] };
// [ts] }
// [ts] fn createVNodeCall(context: auto, tag: auto, props: auto, children: auto, patchFlag: auto, dynamicProps: auto, directives: auto, isBlock: auto = false, disableTracking: auto = false, loc: auto = locStub) {
// [ts] if (context) {
// [ts] if (isBlock) {
// [ts] context.helper(OPEN_BLOCK);
// [ts] context.helper(CREATE_BLOCK);
// [ts] }
// [ts] else {
// [ts] context.helper(CREATE_VNODE);
// [ts] }
// [ts] if (directives) {
// [ts] context.helper(WITH_DIRECTIVES);
// [ts] }
// [ts] }
// [ts] return {
// [ts] type: 13 /* VNODE_CALL */,
// [ts] tag,
// [ts] props,
// [ts] children,
// [ts] patchFlag,
// [ts] dynamicProps,
// [ts] directives,
// [ts] isBlock,
// [ts] disableTracking,
// [ts] loc
// [ts] };
// [ts] }
// [ts] fn createArrayExpression(elements: auto, loc: auto = locStub) {
// [ts] return {
// [ts] type: 17 /* JS_ARRAY_EXPRESSION */,
// [ts] loc,
// [ts] elements
// [ts] };
// [ts] }
// [ts] fn createObjectExpression(properties: auto, loc: auto = locStub) {
// [ts] return {
// [ts] type: 15 /* JS_OBJECT_EXPRESSION */,
// [ts] loc,
// [ts] properties
// [ts] };
// [ts] }
// [ts] fn createObjectProperty(key: auto, value: auto) {
// [ts] return {
// [ts] type: 16 /* JS_PROPERTY */,
// [ts] loc: locStub,
// [ts] key: if (isString(key)) createSimpleExpression(key, true) else key,
// [ts] value
// [ts] };
// [ts] }
// [ts] fn createSimpleExpression(content: auto, isStatic: auto, loc: auto = locStub, constType: auto = 0 /* NOT_CONSTANT */) {
// [ts] return {
// [ts] type: 4 /* SIMPLE_EXPRESSION */,
// [ts] loc,
// [ts] content,
// [ts] isStatic,
// [ts] constType: if (isStatic) 3 /* CAN_STRINGIFY */ else constType
// [ts] };
// [ts] }
// [ts] fn createCompoundExpression(children: auto, loc: auto = locStub) {
// [ts] return {
// [ts] type: 8 /* COMPOUND_EXPRESSION */,
// [ts] loc,
// [ts] children
// [ts] };
// [ts] }
// [ts] fn createCallExpression(callee: auto, args: auto = [], loc: auto = locStub) {
// [ts] return {
// [ts] type: 14 /* JS_CALL_EXPRESSION */,
// [ts] loc,
// [ts] callee,
// [ts] arguments: args
// [ts] };
// [ts] }
// [ts] fn createFunctionExpression(params: auto, returns: auto = null, newline: auto = false, isSlot: auto = false, loc: auto = locStub) {
// [ts] return {
// [ts] type: 18 /* JS_FUNCTION_EXPRESSION */,
// [ts] params,
// [ts] returns,
// [ts] newline,
// [ts] isSlot,
// [ts] loc
// [ts] };
// [ts] }
// [ts] fn createConditionalExpression(test: auto, consequent: auto, alternate: auto, newline: auto = true) {
// [ts] return {
// [ts] type: 19 /* JS_CONDITIONAL_EXPRESSION */,
// [ts] test,
// [ts] consequent,
// [ts] alternate,
// [ts] newline,
// [ts] loc: locStub
// [ts] };
// [ts] }
// [ts] fn createCacheExpression(index: auto, value: auto, isVNode: auto = false) {
// [ts] return {
// [ts] type: 20 /* JS_CACHE_EXPRESSION */,
// [ts] index,
// [ts] value,
// [ts] isVNode,
// [ts] loc: locStub
// [ts] };
// [ts] }
// [ts] fn isStaticExp(p: auto) { return p.type == 4 /* SIMPLE_EXPRESSION */ && p.isStatic; }
// [ts] fn isBuiltInType(tag: auto, expected: auto) { return tag == expected || tag == hyphenate(expected); }
// [ts] fn isCoreComponent(tag: auto) {
// [ts] if (isBuiltInType(tag, "Teleport")) {
// [ts] return TELEPORT;
// [ts] }
// [ts] else if (isBuiltInType(tag, "Suspense")) {
// [ts] return SUSPENSE;
// [ts] }
// [ts] else if (isBuiltInType(tag, "KeepAlive")) {
// [ts] return KEEP_ALIVE;
// [ts] }
// [ts] else if (isBuiltInType(tag, "BaseTransition")) {
// [ts] return BASE_TRANSITION;
// [ts] }
// [ts] }
// [ts] const nonIdentifierRE = "^\\d|[^\\$\\w]";
// [ts] fn isSimpleIdentifier(name: auto) { return !nonIdentifierRE.test(name); }
// [ts] const validFirstIdentCharRE = "[A-Za-z_$\\xA0-\\uFFFF]";
// [ts] const validIdentCharRE = "[\\.\\?\\w$\\xA0-\\uFFFF]";
// [ts] const whitespaceRE = "\\s+[.[]\\s*|\\s*[.[]\\s+";
// [ts] /**
// [ts] * Simple lexer to check if an expression is a member expression. This is
// [ts] * lax and only checks validity at the root level (i.e. does not validate exps
// [ts] * inside square brackets), but it's ok since these are only used on template
// [ts] * expressions and false positives are invalid expressions in the first place.
// [ts] */
// [ts] fn isMemberExpression(path: auto) {
// remove whitespaces around . or [ first
// [ts] path = path.trim().replace(whitespaceRE, s => s.trim());
// [ts] let state = 0 /* inMemberExp */;
// [ts] let stateStack = [];
// [ts] let currentOpenBracketCount = 0;
// [ts] let currentOpenParensCount = 0;
// [ts] let currentStringType = null;
// [ts] let i = 0;
// [ts] while (i < path.length) { // [cap-migrate] for-step: i++
// [ts] const char = path.charAt(i);
// [ts] switch (state) {
// [ts] 0 /* inMemberExp */ => {
// [ts] if (char == "[") {
// [ts] stateStack.push(state);
// [ts] state = 1 /* inBrackets */;
// [ts] currentOpenBracketCount++;
// [ts] }
// [ts] else if (char == "(") {
// [ts] stateStack.push(state);
// [ts] state = 2 /* inParens */;
// [ts] currentOpenParensCount++;
// [ts] }
// [ts] else if (!(if (i == 0) validFirstIdentCharRE else validIdentCharRE).test(char)) {
// [ts] return false;
// [ts] }
// break;
// [ts] },
// [ts] 1 /* inBrackets */ => {
// [ts] if (char == `'` || char == `"` || char == "`") {
// [ts] stateStack.push(state);
// [ts] state = 3 /* inString */;
// [ts] currentStringType = char;
// [ts] }
// [ts] else if (char == `[`) {
// [ts] currentOpenBracketCount++;
// [ts] }
// [ts] else if (char == `]`) {
// [ts] if (!--currentOpenBracketCount) {
// [ts] state = stateStack.pop();
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] 2 /* inParens */ => {
// [ts] if (char == `'` || char == `"` || char == "`") {
// [ts] stateStack.push(state);
// [ts] state = 3 /* inString */;
// [ts] currentStringType = char;
// [ts] }
// [ts] else if (char == `(`) {
// [ts] currentOpenParensCount++;
// [ts] }
// [ts] else if (char == `)`) {
// if the exp ends as a call then it should not be considered valid
// [ts] if (i == path.length - 1) {
// [ts] return false;
// [ts] }
// [ts] if (!--currentOpenParensCount) {
// [ts] state = stateStack.pop();
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] 3 /* inString */ => {
// [ts] if (char == currentStringType) {
// [ts] state = stateStack.pop();
// [ts] currentStringType = null;
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] return !currentOpenBracketCount && !currentOpenParensCount;
// [ts] };
// [ts] fn getInnerRange(loc: auto, offset: auto, length: auto) {
// [ts] const source = loc.source.substr(offset, length);
// [ts] const newLoc = {
// [ts] source,
// [ts] start: advancePositionWithClone(loc.start, loc.source, offset),
// [ts] end: loc.end
// [ts] };
// [ts] if (length != null) {
// [ts] newLoc.end = advancePositionWithClone(loc.start, loc.source, offset + length);
// [ts] }
// [ts] return newLoc;
// [ts] }
// [ts] fn advancePositionWithClone(pos: auto, source: auto, numberOfCharacters: auto = source.length) {
// [ts] return advancePositionWithMutation(extend({}, pos), source, numberOfCharacters);
// [ts] }
// advance by mutation without cloning (for performance reasons), since this
// gets called a lot in the parser
// [ts] fn advancePositionWithMutation(pos: auto, source: auto, numberOfCharacters: auto = source.length) {
// [ts] let linesCount = 0;
// [ts] let lastNewLinePos = -1;
// [ts] let i = 0;
// [ts] while (i < numberOfCharacters) { // [cap-migrate] for-step: i++
// [ts] if (source.charCodeAt(i) == 10 /* newline char code */) {
// [ts] linesCount++;
// [ts] lastNewLinePos = i;
// [ts] }
// [ts] }
// [ts] pos.offset += numberOfCharacters;
// [ts] pos.line += linesCount;
// [ts] pos.column =
// [ts] if (lastNewLinePos == -1) pos.column + numberOfCharacters else numberOfCharacters - lastNewLinePos;
// [ts] return pos;
// [ts] }
// [ts] fn assert(condition: auto, msg: auto) {
// [ts] /* istanbul ignore if */
// [ts] if (!condition) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(msg || `unexpected compiler condition`);)
// [ts] }
// [ts] }
// [ts] fn findDir(node: auto, name: auto, allowEmpty: auto = false) {
// [ts] let i = 0;
// [ts] while (i < node.props.length) { // [cap-migrate] for-step: i++
// [ts] const p = node.props[i];
// [ts] if (p.type == 7 /* DIRECTIVE */ &&
// [ts] (allowEmpty || p.exp) &&
// [ts] (if (isString(name)) p.name == name else name.test(p.name))) {
// [ts] return p;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn findProp(node: auto, name: auto, dynamicOnly: auto = false, allowEmpty: auto = false) {
// [ts] let i = 0;
// [ts] while (i < node.props.length) { // [cap-migrate] for-step: i++
// [ts] const p = node.props[i];
// [ts] if (p.type == 6 /* ATTRIBUTE */) {
// [ts] if (dynamicOnly)
// [ts] continue;
// [ts] if (p.name == name && (p.value || allowEmpty)) {
// [ts] return p;
// [ts] }
// [ts] }
// [ts] else if (p.name == "bind" &&
// [ts] (p.exp || allowEmpty) &&
// [ts] isBindKey: (p.arg, name) => {
// [ts] return p;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isBindKey(arg: auto, name: auto) {
// [ts] return !!(arg && isStaticExp(arg) && arg.content == name);
// [ts] }
// [ts] fn hasDynamicKeyVBind(node: auto) {
// [ts] return node.props.some(p => p.type == 7 /* DIRECTIVE */ &&
// [ts] p.name == "bind" &&
// [ts] (!p.arg || // v-bind="obj"
// [ts] p.arg.type != 4 /* SIMPLE_EXPRESSION */ || // v-bind:[_ctx.foo]
// [ts] !p.arg.isStatic) // v-bind:[foo]
// [ts] );
// [ts] }
// [ts] fn isText(node: auto) {
// [ts] return node.type == 5 /* INTERPOLATION */ || node.type == 2 /* TEXT */;
// [ts] }
// [ts] fn isVSlot(p: auto) {
// [ts] return p.type == 7 /* DIRECTIVE */ && p.name == "slot";
// [ts] }
// [ts] fn isTemplateNode(node: auto) {
// [ts] return (node.type == 1 /* ELEMENT */ && node.tagType == 3 /* TEMPLATE */);
// [ts] }
// [ts] fn isSlotOutlet(node: auto) {
// [ts] return node.type == 1 /* ELEMENT */ && node.tagType == 2 /* SLOT */;
// [ts] }
// [ts] fn injectProp(node: auto, prop: auto, context: auto) {
// [ts] let propsWithInjection;
// [ts] const props = if (node.type == 13 /* VNODE_CALL */) node.props else node.arguments[2];
// [ts] if (props == null || isString(props)) {
// [ts] propsWithInjection = createObjectExpression([prop]);
// [ts] }
// [ts] else if (props.type == 14 /* JS_CALL_EXPRESSION */) {
// merged props... add ours
// only inject key to object literal if it's the first argument so that
// if doesn't override user provided keys
// [ts] const first = props.arguments[0];
// [ts] if (!isString(first) && first.type == 15 /* JS_OBJECT_EXPRESSION */) {
// [ts] first.properties.unshift(prop);
// [ts] }
// [ts] else {
// [ts] if (props.callee == TO_HANDLERS) {
// #2366
// [ts] propsWithInjection = createCallExpression(context.helper(MERGE_PROPS), [
// [ts] createObjectExpression([prop]),
// [ts] props
// [ts] ]);
// [ts] }
// [ts] else {
// [ts] props.arguments.unshift(createObjectExpression([prop]));
// [ts] }
// [ts] }
// [ts] !propsWithInjection && (propsWithInjection = props);
// [ts] }
// [ts] else if (props.type == 15 /* JS_OBJECT_EXPRESSION */) {
// [ts] let alreadyExists = false;
// check existing key to avoid overriding user provided keys
// [ts] if (prop.key.type == 4 /* SIMPLE_EXPRESSION */) {
// [ts] const propKeyName = prop.key.content;
// [ts] alreadyExists = props.properties.some(p => p.key.type == 4 /* SIMPLE_EXPRESSION */ &&
// [ts] p.key.content == propKeyName);
// [ts] }
// [ts] if (!alreadyExists) {
// [ts] props.properties.unshift(prop);
// [ts] }
// [ts] propsWithInjection = props;
// [ts] }
// [ts] else {
// single v-bind with expression, return a merged replacement
// [ts] propsWithInjection = createCallExpression(context.helper(MERGE_PROPS), [
// [ts] createObjectExpression([prop]),
// [ts] props
// [ts] ]);
// [ts] }
// [ts] if (node.type == 13 /* VNODE_CALL */) {
// [ts] node.props = propsWithInjection;
// [ts] }
// [ts] else {
// [ts] node.arguments[2] = propsWithInjection;
// [ts] }
// [ts] }
// [ts] fn toValidAssetId(name: auto, type: auto) {
// [ts] return `_${type}_${name.replace("[^\\w]", "_")}`;
// [ts] }
// [ts] const deprecationData$1 = {
// [ts] ["COMPILER_IS_ON_ELEMENT" /* COMPILER_IS_ON_ELEMENT */]: {
// [ts] message: `Platform-native elements with "is" prop will no longer be ` +
// [ts] `treated as components in Vue 3 unless the "is" value is explicitly ` +
// [ts] `prefixed with "vue:".`,
// [ts] link: `https://v3.vuejs.org/guide/migration/custom-elements-interop.html`
// [ts] },
// [ts] ["COMPILER_V_BIND_SYNC" /* COMPILER_V_BIND_SYNC */]: {
// [ts] message: key => `.sync modifier for v-bind has been removed. Use v-model with ` +
// [ts] `argument instead. \`v-bind:${key}.sync\` should be changed to ` +
// [ts] `\`v-model:${key}\`.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/v-model.html`
// [ts] },
// [ts] ["COMPILER_V_BIND_PROP" /* COMPILER_V_BIND_PROP */]: {
// [ts] message: `.prop modifier for v-bind has been removed and no longer necessary. ` +
// [ts] `Vue 3 will automatically set a binding as DOM property when appropriate.`
// [ts] },
// [ts] ["COMPILER_V_BIND_OBJECT_ORDER" /* COMPILER_V_BIND_OBJECT_ORDER */]: {
// [ts] message: `v-bind="obj" usage is now order sensitive and behaves like JavaScript ` +
// [ts] `object spread: it will now overwrite an existing non-mergeable attribute ` +
// [ts] `that appears before v-bind in the case of conflict. ` +
// [ts] `To retain 2.x behavior, move v-bind to make it the first attribute. ` +
// [ts] `You can also suppress this warning if the usage is intended.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/v-bind.html`
// [ts] },
// [ts] ["COMPILER_V_ON_NATIVE" /* COMPILER_V_ON_NATIVE */]: {
// [ts] message: `.native modifier for v-on has been removed as is no longer necessary.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/v-on-native-modifier-removed.html`
// [ts] },
// [ts] ["COMPILER_V_IF_V_FOR_PRECEDENCE" /* COMPILER_V_IF_V_FOR_PRECEDENCE */]: {
// [ts] message: `v-if / v-for precedence when used on the same element has changed ` +
// [ts] `in Vue 3: v-if now takes higher precedence and will no longer have ` +
// [ts] `access to v-for scope variables. It is best to avoid the ambiguity ` +
// [ts] `with <template> tags or use a computed property that filters v-for ` +
// [ts] `data source.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/v-if-v-for.html`
// [ts] },
// [ts] ["COMPILER_V_FOR_REF" /* COMPILER_V_FOR_REF */]: {
// [ts] message: `Ref usage on v-for no longer creates array ref values in Vue 3. ` +
// [ts] `Consider using function refs or refactor to avoid ref usage altogether.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/array-refs.html`
// [ts] },
// [ts] ["COMPILER_NATIVE_TEMPLATE" /* COMPILER_NATIVE_TEMPLATE */]: {
// [ts] message: `<template> with no special directives will render as a native template ` +
// [ts] `element instead of its inner content in Vue 3.`
// [ts] },
// [ts] ["COMPILER_INLINE_TEMPLATE" /* COMPILER_INLINE_TEMPLATE */]: {
// [ts] message: `"inline-template" has been removed in Vue 3.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/inline-template-attribute.html`
// [ts] },
// [ts] ["COMPILER_FILTER" /* COMPILER_FILTERS */]: {
// [ts] message: `filters have been removed in Vue 3. ` +
// [ts] `The "|" symbol will be treated as native JavaScript bitwise OR operator. ` +
// [ts] `Use method calls or computed properties instead.`,
// [ts] link: `https://v3.vuejs.org/guide/migration/filters.html`
// [ts] }
// [ts] };
// [ts] fn getCompatValue(key: auto, context: auto) {
// [ts] const config = if (context.options) context.options.compatConfig else context.compatConfig;
// [ts] const value = config && config[key];
// [ts] if (key == "MODE") {
// [ts] return value || 3; // compiler defaults to v3 behavior
// [ts] }
// [ts] else {
// [ts] return value;
// [ts] }
// [ts] }
// [ts] (key, context) => {
// [ts] const mode = getCompatValue("MODE", context);
// [ts] const value = getCompatValue(key, context);
// in v3 mode, only enable if explicitly set to true
// otherwise enable for any non-false value
// [ts] return if (mode == 3) value == true else value != false;
// [ts] }
// [ts] fn checkCompatEnabled(key: auto, context: auto, loc: auto, args: auto) {
// [ts] const enabled = isCompatEnabled$1(key, context);
// [ts] if (enabled) {
// [ts] warnDeprecation$1(key, context, loc, ...args);
// [ts] }
// [ts] return enabled;
// [ts] }
// [ts] (key, context, loc, args) => {
// [ts] const val = getCompatValue(key, context);
// [ts] if (val == "suppress-warning") {
// [ts] return;
// [ts] }
// [ts] const { message, link } = deprecationData$1[key];
// [ts] const msg = `(deprecation ${key}) ${if (@typeName(message) == "function") message(...args) else message}${if (link) `\n  Details: ${link}` else ``}`;
// [ts] const err = SyntaxError.init(msg);
// [ts] err.code = key;
// [ts] if (loc)
// [ts] err.loc = loc;
// [ts] context.onWarn(err);
// [ts] }
// The default decoder only provides escapes for characters reserved as part of
// the template syntax, and is only used if the custom renderer did not provide
// a platform-specific decoder.
// [ts] const decodeRE = "&(gt|lt|amp|apos|quot);";
// [ts] const decodeMap = {
// [ts] gt: ">",
// [ts] lt: "<",
// [ts] amp: "&",
// [ts] apos: "'",
// [ts] quot: "\""
// [ts] };
// [ts] const defaultParserOptions = {
// [ts] delimiters: [`{{`, `}}`],
// [ts] getNamespace: () => 0 /* HTML */,
// [ts] getTextMode: () => 0 /* DATA */,
// [ts] isVoidTag: NO,
// [ts] isPreTag: NO,
// [ts] isCustomElement: NO,
// [ts] decodeEntities: (rawText) => rawText.replace(decodeRE, (_unused, p1) => decodeMap[p1]),
// [ts] onError: defaultOnError,
// [ts] onWarn: defaultOnWarn,
// [ts] comments: false
// [ts] };
// [ts] fn baseParse(content: auto, options: auto = {}) {
// [ts] const context = createParserContext(content, options);
// [ts] const start = getCursor(context);
// [ts] return createRoot(parseChildren(context, 0 /* DATA */, []), getSelection(context, start));
// [ts] }
// [ts] fn createParserContext(content: auto, rawOptions: auto) {
// [ts] const options = extend({}, defaultParserOptions);
// [ts] for (const key in rawOptions) {
// @ts-ignore
// [ts] options[key] = rawOptions[key] || defaultParserOptions[key];
// [ts] }
// [ts] return {
// [ts] options,
// [ts] column: 1,
// [ts] line: 1,
// [ts] offset: 0,
// [ts] originalSource: content,
// [ts] source: content,
// [ts] inPre: false,
// [ts] inVPre: false,
// [ts] onWarn: options.onWarn
// [ts] };
// [ts] }
// [ts] fn parseChildren(context: auto, mode: auto, ancestors: auto) {
// [ts] const parent = last(ancestors);
// [ts] const ns = if (parent) parent.ns else 0 /* HTML */;
// [ts] const nodes = [];
// [ts] while (!isEnd(context, mode, ancestors)) {
// [ts] const s = context.source;
// [ts] let node = null;
// [ts] if (mode == 0 /* DATA */ || mode == 1 /* RCDATA */) {
// [ts] if (!context.inVPre && startsWith(s, context.options.delimiters[0])) {
// '{{'
// [ts] node = parseInterpolation(context, mode);
// [ts] }
// [ts] else if (mode == 0 /* DATA */ && s[0] == "<") {
// https://html.spec.whatwg.org/multipage/parsing.html#tag-open-state
// [ts] if (s.length == 1) {
// [ts] emitError(context, 5 /* EOF_BEFORE_TAG_NAME */, 1);
// [ts] }
// [ts] else if (s[1] == "!") {
// https://html.spec.whatwg.org/multipage/parsing.html#markup-declaration-open-state
// [ts] if (startsWith(s, "<!--")) {
// [ts] node = parseComment(context);
// [ts] }
// [ts] else if (startsWith(s, "<!DOCTYPE")) {
// Ignore DOCTYPE by a limitation.
// [ts] node = parseBogusComment(context);
// [ts] }
// [ts] else if (startsWith(s, "<![CDATA[")) {
// [ts] if (ns != 0 /* HTML */) {
// [ts] node = parseCDATA(context, ancestors);
// [ts] }
// [ts] else {
// [ts] emitError(context, 1 /* CDATA_IN_HTML_CONTENT */);
// [ts] node = parseBogusComment(context);
// [ts] }
// [ts] }
// [ts] else {
// [ts] emitError(context, 11 /* INCORRECTLY_OPENED_COMMENT */);
// [ts] node = parseBogusComment(context);
// [ts] }
// [ts] }
// [ts] else if (s[1] == "/") {
// https://html.spec.whatwg.org/multipage/parsing.html#end-tag-open-state
// [ts] if (s.length == 2) {
// [ts] emitError(context, 5 /* EOF_BEFORE_TAG_NAME */, 2);
// [ts] }
// [ts] else if (s[2] == ">") {
// [ts] emitError(context, 14 /* MISSING_END_TAG_NAME */, 2);
// [ts] advanceBy(context, 3);
// [ts] continue;
// [ts] }
// [ts] else if ("[a-z]".test(s[2])) {
// [ts] emitError(context, 23 /* X_INVALID_END_TAG */);
// [ts] parseTag(context, 1 /* End */, parent);
// [ts] continue;
// [ts] }
// [ts] else {
// [ts] emitError(context, 12 /* INVALID_FIRST_CHARACTER_OF_TAG_NAME */, 2);
// [ts] node = parseBogusComment(context);
// [ts] }
// [ts] }
// [ts] else if ("[a-z]".test(s[1])) {
// [ts] node = parseElement(context, ancestors);
// [ts] }
// [ts] else if (s[1] == "?") {
// [ts] emitError(context, 21 /* UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME */, 1);
// [ts] node = parseBogusComment(context);
// [ts] }
// [ts] else {
// [ts] emitError(context, 12 /* INVALID_FIRST_CHARACTER_OF_TAG_NAME */, 1);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!node) {
// [ts] node = parseText(context, mode);
// [ts] }
// [ts] if (isArray(node)) {
// [ts] let i = 0;
// [ts] while (i < node.length) { // [cap-migrate] for-step: i++
// [ts] pushNode(nodes, node[i]);
// [ts] }
// [ts] }
// [ts] else {
// [ts] pushNode(nodes, node);
// [ts] }
// [ts] }
// Whitespace handling strategy like v2
// [ts] let removedWhitespace = false;
// [ts] if (mode != 2 /* RAWTEXT */ && mode != 1 /* RCDATA */) {
// [ts] const preserve = context.options.whitespace == "preserve";
// [ts] let i = 0;
// [ts] while (i < nodes.length) { // [cap-migrate] for-step: i++
// [ts] const node = nodes[i];
// [ts] if (!context.inPre && node.type == 2 /* TEXT */) {
// [ts] if (!"[^\\t\\r\\n\\f ]".test(node.content)) {
// [ts] const prev = nodes[i - 1];
// [ts] const next = nodes[i + 1];
// Remove if:
// - the whitespace is the first or last node, or:
// - (condense mode) the whitespace is adjacent to a comment, or:
// - (condense mode) the whitespace is between two elements AND contains newline
// [ts] if (!prev ||
// [ts] !next ||
// [ts] (!preserve &&
// [ts] (prev.type == 3 /* COMMENT */ ||
// [ts] next.type == 3 /* COMMENT */ ||
// [ts] (prev.type == 1 /* ELEMENT */ &&
// [ts] next.type == 1 /* ELEMENT */ &&
// [ts] "[\\r\\n]".test(node.content))))) {
// [ts] removedWhitespace = true;
// [ts] nodes[i] = null;
// [ts] }
// [ts] else {
// Otherwise, the whitespace is condensed into a single space
// [ts] node.content = " ";
// [ts] }
// [ts] }
// [ts] else if (!preserve) {
// in condense mode, consecutive whitespaces in text are condensed
// down to a single space.
// [ts] node.content = node.content.replace("[\\t\\r\\n\\f ]+", " ");
// [ts] }
// [ts] }
// [ts] }
// [ts] if (context.inPre && parent && context.options.isPreTag(parent.tag)) {
// remove leading newline per html spec
// https://html.spec.whatwg.org/multipage/grouping-content.html#the-pre-element
// [ts] const first = nodes[0];
// [ts] if (first && first.type == 2 /* TEXT */) {
// [ts] first.content = first.content.replace("^\\r?\\n", "");
// [ts] }
// [ts] }
// [ts] }
// [ts] return if (removedWhitespace) nodes.filter(Boolean) else nodes;
// [ts] }
// [ts] fn pushNode(nodes: auto, node: auto) {
// [ts] if (node.type == 2 /* TEXT */) {
// [ts] const prev = last(nodes);
// Merge if both this and the previous node are text and those are
// consecutive. This happens for cases like "a < b".
// [ts] if (prev &&
// [ts] prev.type == 2 /* TEXT */ &&
// [ts] prev.loc.end.offset == node.loc.start.offset) {
// [ts] prev.content += node.content;
// [ts] prev.loc.end = node.loc.end;
// [ts] prev.loc.source += node.loc.source;
// [ts] return;
// [ts] }
// [ts] }
// [ts] nodes.push(node);
// [ts] }
// [ts] fn parseCDATA(context: auto, ancestors: auto) {
// [ts] advanceBy(context, 9);
// [ts] const nodes = parseChildren(context, 3 /* CDATA */, ancestors);
// [ts] if (context.source.length == 0) {
// [ts] emitError(context, 6 /* EOF_IN_CDATA */);
// [ts] }
// [ts] else {
// [ts] advanceBy(context, 3);
// [ts] }
// [ts] return nodes;
// [ts] }
// [ts] fn parseComment(context: auto) {
// [ts] const start = getCursor(context);
// [ts] let content;
// Regular comment.
// [ts] const match_val = "--(\\!)?>".exec(context.source);
// [ts] if (!match_val) {
// [ts] content = context.source.slice(4);
// [ts] advanceBy(context, context.source.length);
// [ts] emitError(context, 7 /* EOF_IN_COMMENT */);
// [ts] }
// [ts] else {
// [ts] if (match_val.index <= 3) {
// [ts] emitError(context, 0 /* ABRUPT_CLOSING_OF_EMPTY_COMMENT */);
// [ts] }
// [ts] if (match_val[1]) {
// [ts] emitError(context, 10 /* INCORRECTLY_CLOSED_COMMENT */);
// [ts] }
// [ts] content = context.source.slice(4, match_val.index);
// Advancing with reporting nested comments.
// [ts] const s = context.source.slice(0, match_val.index);
// [ts] let prevIndex = 1, nestedIndex = 0;
// [ts] while ((nestedIndex = s.indexOf("<!--", prevIndex)) != -1) {
// [ts] advanceBy(context, nestedIndex - prevIndex + 1);
// [ts] if (nestedIndex + 4 < s.length) {
// [ts] emitError(context, 16 /* NESTED_COMMENT */);
// [ts] }
// [ts] prevIndex = nestedIndex + 1;
// [ts] }
// [ts] advanceBy(context, match_val.index + match_val[0].length - prevIndex + 1);
// [ts] }
// [ts] return {
// [ts] type: 3 /* COMMENT */,
// [ts] content,
// [ts] loc: getSelection(context, start)
// [ts] };
// [ts] }
// [ts] fn parseBogusComment(context: auto) {
// [ts] const start = getCursor(context);
// [ts] const contentStart = if (context.source[1] == "?") 1 else 2;
// [ts] let content;
// [ts] const closeIndex = context.source.indexOf(">");
// [ts] if (closeIndex == -1) {
// [ts] content = context.source.slice(contentStart);
// [ts] advanceBy(context, context.source.length);
// [ts] }
// [ts] else {
// [ts] content = context.source.slice(contentStart, closeIndex);
// [ts] advanceBy(context, closeIndex + 1);
// [ts] }
// [ts] return {
// [ts] type: 3 /* COMMENT */,
// [ts] content,
// [ts] loc: getSelection(context, start)
// [ts] };
// [ts] }
// [ts] fn parseElement(context: auto, ancestors: auto) {
// Start tag.
// [ts] const wasInPre = context.inPre;
// [ts] const wasInVPre = context.inVPre;
// [ts] const parent = last(ancestors);
// [ts] const element = parseTag(context, 0 /* Start */, parent);
// [ts] const isPreBoundary = context.inPre && !wasInPre;
// [ts] const isVPreBoundary = context.inVPre && !wasInVPre;
// [ts] if (element.isSelfClosing || context.options.isVoidTag(element.tag)) {
// #4030 self-closing <pre> tag
// [ts] if (context.options.isPreTag(element.tag)) {
// [ts] context.inPre = false;
// [ts] }
// [ts] return element;
// [ts] }
// Children.
// [ts] ancestors.push(element);
// [ts] const mode = context.options.getTextMode(element, parent);
// [ts] const children = parseChildren(context, mode, ancestors);
// [ts] ancestors.pop();
// [ts] element.children = children;
// End tag.
// [ts] if (startsWithEndTagOpen(context.source, element.tag)) {
// [ts] parseTag(context, 1 /* End */, parent);
// [ts] }
// [ts] else {
// [ts] emitError(context, 24 /* X_MISSING_END_TAG */, 0, element.loc.start);
// [ts] if (context.source.length == 0 && element.tag.toLowerCase() == "script") {
// [ts] const first = children[0];
// [ts] if (first && startsWith(first.loc.source, "<!--")) {
// [ts] emitError(context, 8 /* EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT */);
// [ts] }
// [ts] }
// [ts] }
// [ts] element.loc = getSelection(context, element.loc.start);
// [ts] if (isPreBoundary) {
// [ts] context.inPre = false;
// [ts] }
// [ts] if (isVPreBoundary) {
// [ts] context.inVPre = false;
// [ts] }
// [ts] return element;
// [ts] }
// [ts] const isSpecialTemplateDirective = /*_priv___PURE__*/ makeMap(`if,else,else-if,for,slot`);
// [ts] fn parseTag(context: auto, type: auto, parent: auto) {
// Tag open.
// [ts] const start = getCursor(context);
// [ts] const match_val = "^<\\/?([a-z][^\\t\\r\\n\\f />]*)".exec(context.source);
// [ts] const tag = match_val[1];
// [ts] const ns = context.options.getNamespace(tag, parent);
// [ts] advanceBy(context, match_val[0].length);
// [ts] advanceSpaces(context);
// save current state in case we need to re-parse attributes with v-pre
// [ts] const cursor = getCursor(context);
// [ts] const currentSource = context.source;
// check <pre> tag
// [ts] const isPreTag = context.options.isPreTag(tag);
// [ts] if (isPreTag) {
// [ts] context.inPre = true;
// [ts] }
// Attributes.
// [ts] let props = parseAttributes(context, type);
// check v-pre
// [ts] if (type == 0 /* Start */ &&
// [ts] !context.inVPre &&
// [ts] props.some(p => p.type == 7 /* DIRECTIVE */ && p.name == "pre")) {
// [ts] context.inVPre = true;
// reset context
// [ts] extend(context, cursor);
// [ts] context.source = currentSource;
// re-parse attrs and filter out v-pre itself
// [ts] props = parseAttributes(context, type).filter(p => p.name != "v-pre");
// [ts] }
// Tag close.
// [ts] let isSelfClosing = false;
// [ts] if (context.source.length == 0) {
// [ts] emitError(context, 9 /* EOF_IN_TAG */);
// [ts] }
// [ts] else {
// [ts] isSelfClosing = startsWith(context.source, "/>");
// [ts] if (type == 1 /* End */ && isSelfClosing) {
// [ts] emitError(context, 4 /* END_TAG_WITH_TRAILING_SOLIDUS */);
// [ts] }
// [ts] advanceBy(if (context, isSelfClosing) 2 else 1);
// [ts] }
// [ts] if (type == 1 /* End */) {
// [ts] return;
// [ts] }
// [ts] let tagType = 0 /* ELEMENT */;
// [ts] if (!context.inVPre) {
// [ts] if (tag == "slot") {
// [ts] tagType = 2 /* SLOT */;
// [ts] }
// [ts] else if (tag == "template") {
// [ts] if (props.some(p => p.type == 7 /* DIRECTIVE */ && isSpecialTemplateDirective(p.name))) {
// [ts] tagType = 3 /* TEMPLATE */;
// [ts] }
// [ts] }
// [ts] else if (isComponent(tag, props, context)) {
// [ts] tagType = 1 /* COMPONENT */;
// [ts] }
// [ts] }
// [ts] return {
// [ts] type: 1 /* ELEMENT */,
// [ts] ns,
// [ts] tag,
// [ts] tagType,
// [ts] props,
// [ts] isSelfClosing,
// [ts] children: [],
// [ts] loc: getSelection(context, start),
// [ts] codegenNode: null // to be created during transform phase
// [ts] };
// [ts] }
// [ts] fn isComponent(tag: auto, props: auto, context: auto) {
// [ts] const options = context.options;
// [ts] if (options.isCustomElement(tag)) {
// [ts] return false;
// [ts] }
// [ts] if (tag == "component" ||
// [ts] "^[A-Z]".test(tag) ||
// [ts] isCoreComponent(tag) ||
// [ts] (options.isBuiltInComponent && options.isBuiltInComponent(tag)) ||
// [ts] (options.isNativeTag && !options.isNativeTag(tag))) {
// [ts] return true;
// [ts] }
// at this point the tag should be a native tag, but check for potential "is"
// casting
// [ts] let i = 0;
// [ts] while (i < props.length) { // [cap-migrate] for-step: i++
// [ts] const p = props[i];
// [ts] if (p.type == 6 /* ATTRIBUTE */) {
// [ts] if (p.name == "is" && p.value) {
// [ts] if (p.value.content.startsWith("vue:")) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// directive
// v-is (TODO Deprecate)
// [ts] if (p.name == "is") {
// [ts] return true;
// [ts] }
// [ts] else if (
// :is on plain element - only treat as component in compat mode
// [ts] p.name == "bind" &&
// [ts] isBindKey(p.arg, "is") &&
// [ts] false &&
// [ts] checkCompatEnabled: ("COMPILER_IS_ON_ELEMENT" /* COMPILER_IS_ON_ELEMENT */, context, p.loc) => {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn parseAttributes(context: auto, type: auto) {
// [ts] const props = [];
// [ts] const attributeNames = Set.init();
// [ts] while (context.source.length > 0 &&
// [ts] !startsWith(context.source, ">") &&
// [ts] !startsWith(context.source, "/>")) {
// [ts] if (startsWith(context.source, "/")) {
// [ts] emitError(context, 22 /* UNEXPECTED_SOLIDUS_IN_TAG */);
// [ts] advanceBy(context, 1);
// [ts] advanceSpaces(context);
// [ts] continue;
// [ts] }
// [ts] if (type == 1 /* End */) {
// [ts] emitError(context, 3 /* END_TAG_WITH_ATTRIBUTES */);
// [ts] }
// [ts] const attr = parseAttribute(context, attributeNames);
// [ts] if (type == 0 /* Start */) {
// [ts] props.push(attr);
// [ts] }
// [ts] if ("^[^\\t\\r\\n\\f />]".test(context.source)) {
// [ts] emitError(context, 15 /* MISSING_WHITESPACE_BETWEEN_ATTRIBUTES */);
// [ts] }
// [ts] advanceSpaces(context);
// [ts] }
// [ts] return props;
// [ts] }
// [ts] fn parseAttribute(context: auto, nameSet: auto) {
// Name.
// [ts] const start = getCursor(context);
// [ts] const match_val = "^[^\\t\\r\\n\\f />][^\\t\\r\\n\\f />=]*".exec(context.source);
// [ts] const name = match_val[0];
// [ts] if (nameSet.has(name)) {
// [ts] emitError(context, 2 /* DUPLICATE_ATTRIBUTE */);
// [ts] }
// [ts] nameSet.add(name);
// [ts] if (name[0] == "=") {
// [ts] emitError(context, 19 /* UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME */);
// [ts] }
// [ts] {
// [ts] const pattern = "[\"'<]";
// [ts] let m;
// [ts] while ((m = pattern.exec(name))) {
// [ts] emitError(context, 17 /* UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME */, m.index);
// [ts] }
// [ts] }
// [ts] advanceBy(context, name.length);
// Value
// [ts] let value = null;
// [ts] if ("^[\\t\\r\\n\\f ]*=".test(context.source)) {
// [ts] advanceSpaces(context);
// [ts] advanceBy(context, 1);
// [ts] advanceSpaces(context);
// [ts] value = parseAttributeValue(context);
// [ts] if (!value) {
// [ts] emitError(context, 13 /* MISSING_ATTRIBUTE_VALUE */);
// [ts] }
// [ts] }
// [ts] const loc = getSelection(context, start);
// [ts] if (!context.inVPre && "^(v-|:|@|#)".test(name)) {
// [ts] const match_val = "(?:^v-([a-z0-9-]+))?(?:(?::|^@|^#)(\\[[^\\]]+\\]|[^\\.]+))?(.+)?$".exec(name);
// [ts] let dirName = match_val[1] ||
// [ts] (if (startsWith(name, ":")) "bind" else if (startsWith(name, "@")) "on" else "slot");
// [ts] let arg;
// [ts] if (match_val[2]) {
// [ts] const isSlot = dirName == "slot";
// [ts] const startOffset = name.lastIndexOf(match_val[2]);
// [ts] const loc = getSelection(context, getNewPosition(context, start, startOffset), getNewPosition(context, start, startOffset + match_val[2].length + ((isSlot && match_val[3]) || "").length));
// [ts] let content = match_val[2];
// [ts] let isStatic = true;
// [ts] if (content.startsWith("[")) {
// [ts] isStatic = false;
// [ts] if (!content.endsWith("]")) {
// [ts] emitError(context, 26 /* X_MISSING_DYNAMIC_DIRECTIVE_ARGUMENT_END */);
// [ts] }
// [ts] content = content.substr(1, content.length - 2);
// [ts] }
// [ts] else if (isSlot) {
// #1241 special case for v-slot: vuetify relies extensively on slot
// names containing dots. v-slot doesn't have any modifiers and Vue 2.x
// supports such usage so we are keeping it consistent with 2.x.
// [ts] content += match_val[3] || "";
// [ts] }
// [ts] arg = {
// [ts] type: 4 /* SIMPLE_EXPRESSION */,
// [ts] content,
// [ts] isStatic,
// [ts] constType: if (isStatic) 3 /* CAN_STRINGIFY */ else 0 /* NOT_CONSTANT */,
// [ts] loc
// [ts] };
// [ts] }
// [ts] if (value && value.isQuoted) {
// [ts] const valueLoc = value.loc;
// [ts] valueLoc.start.offset++;
// [ts] valueLoc.start.column++;
// [ts] valueLoc.end = advancePositionWithClone(valueLoc.start, value.content);
// [ts] valueLoc.source = valueLoc.source.slice(1, -1);
// [ts] }
// [ts] const modifiers = if (match_val[3]) match_val[3].substr(1).split(".") else [];
// [ts] return {
// [ts] type: 7 /* DIRECTIVE */,
// [ts] name: dirName,
// [ts] exp: value && {
// [ts] type: 4 /* SIMPLE_EXPRESSION */,
// [ts] content: value.content,
// [ts] isStatic: false,
// Treat as non-constant by default. This can be potentially set to
// other values by `transformExpression` to make it eligible for hoisting.
// [ts] constType: 0 /* NOT_CONSTANT */,
// [ts] loc: value.loc
// [ts] },
// [ts] arg,
// [ts] modifiers,
// [ts] loc
// [ts] };
// [ts] }
// [ts] return {
// [ts] type: 6 /* ATTRIBUTE */,
// [ts] name,
// [ts] value: value && {
// [ts] type: 2 /* TEXT */,
// [ts] content: value.content,
// [ts] loc: value.loc
// [ts] },
// [ts] loc
// [ts] };
// [ts] }
// [ts] fn parseAttributeValue(context: auto) {
// [ts] const start = getCursor(context);
// [ts] let content;
// [ts] const quote = context.source[0];
// [ts] const isQuoted = quote == `"` || quote == `'`;
// [ts] if (isQuoted) {
// Quoted value.
// [ts] advanceBy(context, 1);
// [ts] const endIndex = context.source.indexOf(quote);
// [ts] if (endIndex == -1) {
// [ts] content = parseTextData(context, context.source.length, 4 /* ATTRIBUTE_VALUE */);
// [ts] }
// [ts] else {
// [ts] content = parseTextData(context, endIndex, 4 /* ATTRIBUTE_VALUE */);
// [ts] advanceBy(context, 1);
// [ts] }
// [ts] }
// [ts] else {
// Unquoted
// [ts] const match_val = "^[^\\t\\r\\n\\f >]+".exec(context.source);
// [ts] if (!match_val) {
// [ts] return null;
// [ts] }
// [ts] const unexpectedChars = "[\"'<=`]";
// [ts] let m;
// [ts] while ((m = unexpectedChars.exec(match_val[0]))) {
// [ts] emitError(context, 18 /* UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE */, m.index);
// [ts] }
// [ts] content = parseTextData(context, match_val[0].length, 4 /* ATTRIBUTE_VALUE */);
// [ts] }
// [ts] return { content, isQuoted, loc: getSelection(context, start) };
// [ts] }
// [ts] fn parseInterpolation(context: auto, mode: auto) {
// [ts] const [open, close] = context.options.delimiters;
// [ts] const closeIndex = context.source.indexOf(close, open.length);
// [ts] if (closeIndex == -1) {
// [ts] emitError(context, 25 /* X_MISSING_INTERPOLATION_END */);
// [ts] return null;
// [ts] }
// [ts] const start = getCursor(context);
// [ts] advanceBy(context, open.length);
// [ts] const innerStart = getCursor(context);
// [ts] const innerEnd = getCursor(context);
// [ts] const rawContentLength = closeIndex - open.length;
// [ts] const rawContent = context.source.slice(0, rawContentLength);
// [ts] const preTrimContent = parseTextData(context, rawContentLength, mode);
// [ts] const content = preTrimContent.trim();
// [ts] const startOffset = preTrimContent.indexOf(content);
// [ts] if (startOffset > 0) {
// [ts] advancePositionWithMutation(innerStart, rawContent, startOffset);
// [ts] }
// [ts] const endOffset = rawContentLength - (preTrimContent.length - content.length - startOffset);
// [ts] advancePositionWithMutation(innerEnd, rawContent, endOffset);
// [ts] advanceBy(context, close.length);
// [ts] return {
// [ts] type: 5 /* INTERPOLATION */,
// [ts] content: {
// [ts] type: 4 /* SIMPLE_EXPRESSION */,
// [ts] isStatic: false,
// Set `isConstant` to false by default and will decide in transformExpression
// [ts] constType: 0 /* NOT_CONSTANT */,
// [ts] content,
// [ts] loc: getSelection(context, innerStart, innerEnd)
// [ts] },
// [ts] loc: getSelection(context, start)
// [ts] };
// [ts] }
// [ts] fn parseText(context: auto, mode: auto) {
// [ts] const endTokens = ["<", context.options.delimiters[0]];
// [ts] if (mode == 3 /* CDATA */) {
// [ts] endTokens.push("]]>");
// [ts] }
// [ts] let endIndex = context.source.length;
// [ts] let i = 0;
// [ts] while (i < endTokens.length) { // [cap-migrate] for-step: i++
// [ts] const index = context.source.indexOf(endTokens[i], 1);
// [ts] if (index != -1 && endIndex > index) {
// [ts] endIndex = index;
// [ts] }
// [ts] }
// [ts] const start = getCursor(context);
// [ts] const content = parseTextData(context, endIndex, mode);
// [ts] return {
// [ts] type: 2 /* TEXT */,
// [ts] content,
// [ts] loc: getSelection(context, start)
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Get text data with a given length from the current location.
// [ts] * This translates HTML entities in the text data.
// [ts] */
// [ts] fn parseTextData(context: auto, length: auto, mode: auto) {
// [ts] const rawText = context.source.slice(0, length);
// [ts] advanceBy(context, length);
// [ts] if (mode == 2 /* RAWTEXT */ ||
// [ts] mode == 3 /* CDATA */ ||
// [ts] rawText.indexOf("&") == -1) {
// [ts] return rawText;
// [ts] }
// [ts] else {
// DATA or RCDATA containing "&"". Entity decoding required.
// [ts] return context.options.decodeEntities(rawText, mode == 4 /* ATTRIBUTE_VALUE */);
// [ts] }
// [ts] }
// [ts] fn getCursor(context: auto) {
// [ts] const { column, line, offset } = context;
// [ts] return { column, line, offset };
// [ts] }
// [ts] fn getSelection(context: auto, start: auto, end: auto) {
// [ts] end = end || getCursor(context);
// [ts] return {
// [ts] start,
// [ts] end,
// [ts] source: context.originalSource.slice(start.offset, end.offset)
// [ts] };
// [ts] }
// [ts] fn last(xs: auto) {
// [ts] return xs[xs.length - 1];
// [ts] }
// [ts] fn startsWith(source: auto, searchString: auto) {
// [ts] return source.startsWith(searchString);
// [ts] }
// [ts] fn advanceBy(context: auto, numberOfCharacters: auto) {
// [ts] const { source } = context;
// [ts] advancePositionWithMutation(context, source, numberOfCharacters);
// [ts] context.source = source.slice(numberOfCharacters);
// [ts] }
// [ts] fn advanceSpaces(context: auto) {
// [ts] const match_val = "^[\\t\\r\\n\\f ]+".exec(context.source);
// [ts] if (match_val) {
// [ts] advanceBy(context, match_val[0].length);
// [ts] }
// [ts] }
// [ts] fn getNewPosition(context: auto, start: auto, numberOfCharacters: auto) {
// [ts] return advancePositionWithClone(start, context.originalSource.slice(start.offset, numberOfCharacters), numberOfCharacters);
// [ts] }
// [ts] fn emitError(context: auto, code: auto, offset: auto, loc: auto = getCursor(context)) {
// [ts] if (offset) {
// [ts] loc.offset += offset;
// [ts] loc.column += offset;
// [ts] }
// [ts] context.options.onError(createCompilerError(code, {
// [ts] start: loc,
// [ts] end: loc,
// [ts] source: ""
// [ts] }));
// [ts] }
// [ts] fn isEnd(context: auto, mode: auto, ancestors: auto) {
// [ts] const s = context.source;
// [ts] switch (mode) {
// [ts] 0 /* DATA */ => {
// [ts] if (startsWith(s, "</")) {
// TODO: probably bad performance
// [ts] let i = ancestors.length - 1;
// [ts] while (i >= 0) { // [cap-migrate] for-step: --i
// [ts] if (startsWithEndTagOpen(s, ancestors[i].tag)) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] 1 /* RCDATA */, 2 /* RAWTEXT */ => { {; },
// [ts] const parent = last(ancestors);
// [ts] if (parent && startsWithEndTagOpen(s, parent.tag)) {
// [ts] return true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] case 3 /* CDATA */:
// [ts] if (startsWith(s, "]]>")) {
// [ts] return true;
// [ts] }
// [ts] break;
// [ts] }
// [ts] return !s;
// [ts] }
// [ts] fn startsWithEndTagOpen(source: auto, tag: auto) {
// [ts] return (startsWith(source, "</") &&
// [ts] source.substr(2, tag.length).toLowerCase() == tag.toLowerCase() &&
// [ts] "[\\t\\r\\n\\f />]".test(source[2 + tag.length] || ">"));
// [ts] }
// [ts] fn hoistStatic(root: auto, context: auto) {
// [ts] walk(root, context,
// Root node is unfortunately non-hoistable due to potential parent
// fallthrough attributes.
// [ts] isSingleElementRoot(root, root.children[0]));
// [ts] }
// [ts] fn isSingleElementRoot(root: auto, child: auto) {
// [ts] const { children } = root;
// [ts] return (children.length == 1 &&
// [ts] child.type == 1 /* ELEMENT */ &&
// [ts] !isSlotOutlet(child));
// [ts] }
// [ts] fn walk(node: auto, context: auto, doNotHoistNode: auto = false) {
// [ts] let hasHoistedNode = false;
// Some transforms, e.g. transformAssetUrls from @vue/compiler-sfc, replaces
// static bindings with expressions. These expressions are guaranteed to be
// constant so they are still eligible for hoisting, but they are only
// available at runtime and therefore cannot be evaluated ahead of time.
// This is only a concern for pre-stringification (via transformHoist by
// @vue/compiler-dom), but doing it here allows us to perform only one full
// walk of the AST and allow `stringifyStatic` to stop walking as soon as its
// stringficiation threshold is met.
// [ts] let canStringify = true;
// [ts] const { children } = node;
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// only plain elements & text calls are eligible for hoisting.
// [ts] if (child.type == 1 /* ELEMENT */ &&
// [ts] child.tagType == 0 /* ELEMENT */) {
// [ts] const constantType = if (doNotHoistNode) 0 /* NOT_CONSTANT */ else getConstantType(child, context);
// [ts] if (constantType > 0 /* NOT_CONSTANT */) {
// [ts] if (constantType < 3 /* CAN_STRINGIFY */) {
// [ts] canStringify = false;
// [ts] }
// [ts] if (constantType >= 2 /* CAN_HOIST */) {
// [ts] child.codegenNode.patchFlag =
// [ts] -1 /* HOISTED */ + (` /* HOISTED */` );
// [ts] child.codegenNode = context.hoist(child.codegenNode);
// [ts] hasHoistedNode = true;
// [ts] continue;
// [ts] }
// [ts] }
// [ts] else {
// node may contain dynamic children, but its props may be eligible for
// hoisting.
// [ts] const codegenNode = child.codegenNode;
// [ts] if (codegenNode.type == 13 /* VNODE_CALL */) {
// [ts] const flag = getPatchFlag(codegenNode);
// [ts] if ((!flag ||
// [ts] flag == 512 /* NEED_PATCH */ ||
// [ts] flag == 1 /* TEXT */) &&
// [ts] getGeneratedPropsConstantType(child, context) >=
// [ts] 2 /* CAN_HOIST */) {
// [ts] const props = getNodeProps(child);
// [ts] if (props) {
// [ts] codegenNode.props = context.hoist(props);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (child.type == 12 /* TEXT_CALL */) {
// [ts] const contentType = getConstantType(child.content, context);
// [ts] if (contentType > 0) {
// [ts] if (contentType < 3 /* CAN_STRINGIFY */) {
// [ts] canStringify = false;
// [ts] }
// [ts] if (contentType >= 2 /* CAN_HOIST */) {
// [ts] child.codegenNode = context.hoist(child.codegenNode);
// [ts] hasHoistedNode = true;
// [ts] }
// [ts] }
// [ts] }
// walk further
// [ts] if (child.type == 1 /* ELEMENT */) {
// [ts] const isComponent = child.tagType == 1 /* COMPONENT */;
// [ts] if (isComponent) {
// [ts] context.scopes.vSlot++;
// [ts] }
// [ts] walk(child, context);
// [ts] if (isComponent) {
// [ts] context.scopes.vSlot--;
// [ts] }
// [ts] }
// [ts] else if (child.type == 11 /* FOR */) {
// Do not hoist v-for single child because it has to be a block
// [ts] walk(child, context, child.children.length == 1);
// [ts] }
// [ts] else if (child.type == 9 /* IF */) {
// [ts] let i = 0;
// [ts] while (i < child.branches.length) { // [cap-migrate] for-step: i++
// Do not hoist v-if single child because it has to be a block
// [ts] walk(child.branches[i], context, child.branches[i].children.length == 1);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (canStringify && hasHoistedNode && context.transformHoist) {
// [ts] context.transformHoist(children, context, node);
// [ts] }
// [ts] }
// [ts] fn getConstantType(node: auto, context: auto) {
// [ts] const { constantCache } = context;
// [ts] switch (node.type) {
// [ts] 1 /* ELEMENT */ => {
// [ts] if (node.tagType != 0 /* ELEMENT */) {
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] const cached = constantCache.get(node);
// [ts] if (cached != null) {
// [ts] return cached;
// [ts] }
// [ts] const codegenNode = node.codegenNode;
// [ts] if (codegenNode.type != 13 /* VNODE_CALL */) {
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] const flag = getPatchFlag(codegenNode);
// [ts] if (!flag) {
// [ts] let returnType = 3 /* CAN_STRINGIFY */;
// Element itself has no patch flag. However we still need to check:
// 1. Even for a node with no patch flag, it is possible for it to contain
// non-hoistable expressions that refers to scope variables, e.g. compiler
// injected keys or cached event handlers. Therefore we need to always
// check the codegenNode's props to be sure.
// [ts] const generatedPropsType = getGeneratedPropsConstantType(node, context);
// [ts] if (generatedPropsType == 0 /* NOT_CONSTANT */) {
// [ts] constantCache.set(node, 0 /* NOT_CONSTANT */);
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] if (generatedPropsType < returnType) {
// [ts] returnType = generatedPropsType;
// [ts] }
// 2. its children.
// [ts] let i = 0;
// [ts] while (i < node.children.length) { // [cap-migrate] for-step: i++
// [ts] const childType = getConstantType(node.children[i], context);
// [ts] if (childType == 0 /* NOT_CONSTANT */) {
// [ts] constantCache.set(node, 0 /* NOT_CONSTANT */);
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] if (childType < returnType) {
// [ts] returnType = childType;
// [ts] }
// [ts] }
// 3. if the type is not already CAN_SKIP_PATCH which is the lowest non-0
// type, check if any of the props can cause the type to be lowered
// we can skip can_patch because it's guaranteed by the absence of a
// patchFlag.
// [ts] if (returnType > 1 /* CAN_SKIP_PATCH */) {
// [ts] let i = 0;
// [ts] while (i < node.props.length) { // [cap-migrate] for-step: i++
// [ts] const p = node.props[i];
// [ts] if (p.type == 7 /* DIRECTIVE */ && p.name == "bind" && p.exp) {
// [ts] const expType = getConstantType(p.exp, context);
// [ts] if (expType == 0 /* NOT_CONSTANT */) {
// [ts] constantCache.set(node, 0 /* NOT_CONSTANT */);
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] if (expType < returnType) {
// [ts] returnType = expType;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// only svg/foreignObject could be block here, however if they are
// static then they don't need to be blocks since there will be no
// nested updates.
// [ts] if (codegenNode.isBlock) {
// [ts] context.removeHelper(OPEN_BLOCK);
// [ts] context.removeHelper(CREATE_BLOCK);
// [ts] codegenNode.isBlock = false;
// [ts] context.helper(CREATE_VNODE);
// [ts] }
// [ts] constantCache.set(node, returnType);
// [ts] return returnType;
// [ts] }
// [ts] else {
// [ts] constantCache.set(node, 0 /* NOT_CONSTANT */);
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] },
// [ts] 2 /* TEXT */, 3 /* COMMENT */ => {
// [ts] return 3 /* CAN_STRINGIFY */;
// [ts] },
// [ts] 9 /* IF */, 11 /* FOR */, 10 /* IF_BRANCH */ => {
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] },
// [ts] 5 /* INTERPOLATION */, 12 /* TEXT_CALL */ => {
// [ts] return getConstantType(node.content, context);
// [ts] },
// [ts] 4 /* SIMPLE_EXPRESSION */ => {
// [ts] return node.constType;
// [ts] },
// [ts] 8 /* COMPOUND_EXPRESSION */ => {
// [ts] let returnType = 3 /* CAN_STRINGIFY */;
// [ts] let i = 0;
// [ts] while (i < node.children.length) { // [cap-migrate] for-step: i++
// [ts] const child = node.children[i];
// [ts] if (isString(child) || isSymbol(child)) {
// [ts] continue;
// [ts] }
// [ts] const childType = getConstantType(child, context);
// [ts] if (childType == 0 /* NOT_CONSTANT */) {
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] else if (childType < returnType) {
// [ts] returnType = childType;
// [ts] }
// [ts] }
// [ts] return returnType;
// [ts] },
// [ts] else => {
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getGeneratedPropsConstantType(node: auto, context: auto) {
// [ts] let returnType = 3 /* CAN_STRINGIFY */;
// [ts] const props = getNodeProps(node);
// [ts] if (props && props.type == 15 /* JS_OBJECT_EXPRESSION */) {
// [ts] const { properties } = props;
// [ts] let i = 0;
// [ts] while (i < properties.length) { // [cap-migrate] for-step: i++
// [ts] const { key, value } = properties[i];
// [ts] const keyType = getConstantType(key, context);
// [ts] if (keyType == 0 /* NOT_CONSTANT */) {
// [ts] return keyType;
// [ts] }
// [ts] if (keyType < returnType) {
// [ts] returnType = keyType;
// [ts] }
// [ts] if (value.type != 4 /* SIMPLE_EXPRESSION */) {
// [ts] return 0 /* NOT_CONSTANT */;
// [ts] }
// [ts] const valueType = getConstantType(value, context);
// [ts] if (valueType == 0 /* NOT_CONSTANT */) {
// [ts] return valueType;
// [ts] }
// [ts] if (valueType < returnType) {
// [ts] returnType = valueType;
// [ts] }
// [ts] }
// [ts] }
// [ts] return returnType;
// [ts] }
// [ts] fn getNodeProps(node: auto) {
// [ts] const codegenNode = node.codegenNode;
// [ts] if (codegenNode.type == 13 /* VNODE_CALL */) {
// [ts] return codegenNode.props;
// [ts] }
// [ts] }
// [ts] fn getPatchFlag(node: auto) {
// [ts] const flag = node.patchFlag;
// [ts] return if (flag) parseInt(flag, 10) else null;
// [ts] }
// [ts] fn createTransformContext(root: auto, _param: string) {
// [ts] const nameMatch = filename.replace("\\?.*$", "").match("([^/\\\\]+)\\.\\w+$");
// [ts] const context = {
// options
// [ts] selfName: nameMatch && capitalize(camelize(nameMatch[1])),
// [ts] prefixIdentifiers,
// [ts] hoistStatic,
// [ts] cacheHandlers,
// [ts] nodeTransforms,
// [ts] directiveTransforms,
// [ts] transformHoist,
// [ts] isBuiltInComponent,
// [ts] isCustomElement,
// [ts] expressionPlugins,
// [ts] scopeId,
// [ts] slotted,
// [ts] ssr,
// [ts] ssrCssVars,
// [ts] bindingMetadata,
// [ts] inline,
// [ts] isTS,
// [ts] onError,
// [ts] onWarn,
// [ts] compatConfig,
// state
// [ts] root,
// [ts] helpers: Map.init(),
// [ts] components: Set.init(),
// [ts] directives: Set.init(),
// [ts] hoists: [],
// [ts] imports: [],
// [ts] constantCache: Map.init(),
// [ts] temps: 0,
// [ts] cached: 0,
// [ts] identifiers: Object.create(null),
// [ts] scopes: {
// [ts] vFor: 0,
// [ts] vSlot: 0,
// [ts] vPre: 0,
// [ts] vOnce: 0
// [ts] },
// [ts] parent: null,
// [ts] currentNode: root,
// [ts] childIndex: 0,
// methods
// [ts] helper: (name) => {
// [ts] const count = context.helpers.get(name) || 0;
// [ts] context.helpers.set(name, count + 1);
// [ts] return name;
// [ts] },
// [ts] removeHelper: (name) => {
// [ts] const count = context.helpers.get(name);
// [ts] if (count) {
// [ts] const currentCount = count - 1;
// [ts] if (!currentCount) {
// [ts] context.helpers.delete(name);
// [ts] }
// [ts] else {
// [ts] context.helpers.set(name, currentCount);
// [ts] }
// [ts] }
// [ts] },
// [ts] helperString: (name) => {
// [ts] return `_${helperNameMap[context.helper(name)]}`;
// [ts] },
// [ts] replaceNode: (node) => {
// [ts] /* istanbul ignore if */
// [ts] {
// [ts] if (!context.currentNode) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Node being replaced is already removed.`);)
// [ts] }
// [ts] if (!context.parent) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Cannot replace root node.`);)
// [ts] }
// [ts] }
// [ts] context.parent.children[context.childIndex] = context.currentNode = node;
// [ts] },
// [ts] removeNode: (node) => {
// [ts] if (!context.parent) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Cannot remove root node.`);)
// [ts] }
// [ts] const list = context.parent.children;
// [ts] const removalIndex = if (node) list.indexOf(node) else if (context.currentNode) context.childIndex else -1;
// [ts] /* istanbul ignore if */
// [ts] if (removalIndex < 0) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`node being removed is not a child of current parent`);)
// [ts] }
// [ts] if (!node || node == context.currentNode) {
// current node removed
// [ts] context.currentNode = null;
// [ts] context.onNodeRemoved();
// [ts] }
// [ts] else {
// sibling node removed
// [ts] if (context.childIndex > removalIndex) {
// [ts] context.childIndex--;
// [ts] context.onNodeRemoved();
// [ts] }
// [ts] }
// [ts] context.parent.children.splice(removalIndex, 1);
// [ts] },
// [ts] onNodeRemoved: () => { },
// [ts] addIdentifiers: (exp) => {
// [ts] },
// [ts] removeIdentifiers: (exp) => {
// [ts] },
// [ts] hoist: (exp) => {
// [ts] context.hoists.push(exp);
// [ts] const identifier = createSimpleExpression(`_hoisted_${context.hoists.length}`, false, exp.loc, 2 /* CAN_HOIST */);
// [ts] identifier.hoisted = exp;
// [ts] return identifier;
// [ts] },
// [ts] cache: (exp, isVNode) => {
// [ts] return createCacheExpression(++context.cached, exp, isVNode);
// [ts] }
// [ts] };
// [ts] return context;
// [ts] }
// [ts] fn transform(root: auto, options: auto) {
// [ts] const context = createTransformContext(root, options);
// [ts] traverseNode(root, context);
// [ts] if (options.hoistStatic) {
// [ts] hoistStatic(root, context);
// [ts] }
// [ts] if (!options.ssr) {
// [ts] createRootCodegen(root, context);
// [ts] }
// finalize meta information
// [ts] root.helpers = [...context.helpers.keys()];
// [ts] root.components = [...context.components];
// [ts] root.directives = [...context.directives];
// [ts] root.imports = context.imports;
// [ts] root.hoists = context.hoists;
// [ts] root.temps = context.temps;
// [ts] root.cached = context.cached;
// [ts] }
// [ts] fn createRootCodegen(root: auto, context: auto) {
// [ts] const { helper, removeHelper } = context;
// [ts] const { children } = root;
// [ts] if (children.length == 1) {
// [ts] const child = children[0];
// if the single child is an element, turn it into a block.
// [ts] if (isSingleElementRoot(root, child) && child.codegenNode) {
// single element root is never hoisted so codegenNode will never be
// SimpleExpressionNode
// [ts] const codegenNode = child.codegenNode;
// [ts] if (codegenNode.type == 13 /* VNODE_CALL */) {
// [ts] if (!codegenNode.isBlock) {
// [ts] removeHelper(CREATE_VNODE);
// [ts] codegenNode.isBlock = true;
// [ts] helper(OPEN_BLOCK);
// [ts] helper(CREATE_BLOCK);
// [ts] }
// [ts] }
// [ts] root.codegenNode = codegenNode;
// [ts] }
// [ts] else {
// - single <slot/>, IfNode, ForNode: already blocks.
// - single text node: always patched.
// root codegen falls through via genNode()
// [ts] root.codegenNode = child;
// [ts] }
// [ts] }
// [ts] else if (children.length > 1) {
// root has multiple nodes - return a fragment block.
// [ts] let patchFlag = 64 /* STABLE_FRAGMENT */;
// [ts] let patchFlagText = PatchFlagNames[64 /* STABLE_FRAGMENT */];
// check if the fragment actually contains a single valid child with
// the rest being comments
// [ts] if (children.filter(c => c.type != 3 /* COMMENT */).length == 1) {
// [ts] patchFlag |= 2048 /* DEV_ROOT_FRAGMENT */;
// [ts] patchFlagText += `, ${PatchFlagNames[2048 /* DEV_ROOT_FRAGMENT */]}`;
// [ts] }
// [ts] root.codegenNode = createVNodeCall(context, helper(FRAGMENT), null, root.children, patchFlag + (` /* ${patchFlagText} */` ), null, null, true);
// [ts] }
// [ts] else { ; }
// [ts] }
// [ts] fn traverseChildren(parent: auto, context: auto) {
// [ts] let i = 0;
// [ts] const nodeRemoved = () => {
// [ts] i--;
// [ts] };
// [ts] while (i < parent.children.length) { // [cap-migrate] for-step: i++
// [ts] const child = parent.children[i];
// [ts] if (isString(child))
// [ts] continue;
// [ts] context.parent = parent;
// [ts] context.childIndex = i;
// [ts] context.onNodeRemoved = nodeRemoved;
// [ts] traverseNode(child, context);
// [ts] }
// [ts] }
// [ts] fn traverseNode(node: auto, context: auto) {
// [ts] context.currentNode = node;
// apply transform plugins
// [ts] const { nodeTransforms } = context;
// [ts] const exitFns = [];
// [ts] let i = 0;
// [ts] while (i < nodeTransforms.length) { // [cap-migrate] for-step: i++
// [ts] const onExit = nodeTransforms[i](node, context);
// [ts] if (onExit) {
// [ts] if (isArray(onExit)) {
// [ts] exitFns.push(...onExit);
// [ts] }
// [ts] else {
// [ts] exitFns.push(onExit);
// [ts] }
// [ts] }
// [ts] if (!context.currentNode) {
// node was removed
// [ts] return;
// [ts] }
// [ts] else {
// node may have been replaced
// [ts] node = context.currentNode;
// [ts] }
// [ts] }
// [ts] switch (node.type) {
// [ts] 3 /* COMMENT */ => {
// [ts] if (!context.ssr) {
// inject import for the Comment symbol, which is needed for creating
// comment nodes with `createVNode`
// [ts] context.helper(CREATE_COMMENT);
// [ts] }
// break;
// [ts] },
// no need to traverse, but we need to inject toString helper
// [ts] 5 /* INTERPOLATION */ => {
// [ts] if (!context.ssr) {
// [ts] context.helper(TO_DISPLAY_STRING);
// [ts] }
// break;
// for container types, further traverse downwards
// [ts] },
// [ts] let i = 0;
// [ts] while (i < node.branches.length) { // [cap-migrate] for-step: i++
// [ts] traverseNode(node.branches[i], context);
// [ts] }
// [ts] 9 /* IF */ => {
// [ts] break;
// [ts] },
// [ts] 10 /* IF_BRANCH */, 11 /* FOR */, 1 /* ELEMENT */, 0 /* ROOT */ => {
// [ts] traverseChildren(node, context);
// break;
// [ts] }
// [ts] }
// exit transforms
// [ts] context.currentNode = node;
// [ts] let i = exitFns.length;
// [ts] while (i--) {
// [ts] exitFns[i]();
// [ts] }
// [ts] }
// [ts] fn createStructuralDirectiveTransform(name: auto, fn: auto) {
// [ts] const matches = if (isString(name)) (n) => n == name else (n) => name.test(n);
// [ts] return (node, context) => {
// [ts] if (node.type == 1 /* ELEMENT */) {
// [ts] const { props } = node;
// structural directive transforms are not concerned with slots
// as they are handled separately in vSlot.ts
// [ts] if (node.tagType == 3 /* TEMPLATE */ && props.some(isVSlot)) {
// [ts] return;
// [ts] }
// [ts] const exitFns = [];
// [ts] let i = 0;
// [ts] while (i < props.length) { // [cap-migrate] for-step: i++
// [ts] const prop = props[i];
// [ts] if (prop.type == 7 /* DIRECTIVE */ && matches(prop.name)) {
// structural directives are removed to avoid infinite recursion
// also we remove them *before* applying so that it can further
// traverse itself in case it moves the node around
// [ts] props.splice(i, 1);
// [ts] i--;
// [ts] const onExit = fn(node, prop, context);
// [ts] if (onExit)
// [ts] exitFns.push(onExit);
// [ts] }
// [ts] }
// [ts] return exitFns;
// [ts] }
// [ts] };
// [ts] }
// [ts] const PURE_ANNOTATION = `/*#__PURE__*/`;
// [ts] fn createCodegenContext(ast: auto, _param: string) {
// [ts] const context = {
// [ts] mode,
// [ts] prefixIdentifiers,
// [ts] sourceMap,
// [ts] filename,
// [ts] scopeId,
// [ts] optimizeImports,
// [ts] runtimeGlobalName,
// [ts] runtimeModuleName,
// [ts] ssr,
// [ts] isTS,
// [ts] source: ast.loc.source,
// [ts] code: ``,
// [ts] column: 1,
// [ts] line: 1,
// [ts] offset: 0,
// [ts] indentLevel: 0,
// [ts] pure: false,
// [ts] map: null,
// [ts] helper: (key) => {
// [ts] return `_${helperNameMap[key]}`;
// [ts] },
// [ts] push: (code, node) => {
// [ts] context.code += code;
// [ts] },
// [ts] indent: () => {
// [ts] newline(++context.indentLevel);
// [ts] },
// [ts] deindent: (withoutNewLine) => {
// [ts] if (withoutNewLine) {
// [ts] --context.indentLevel;
// [ts] }
// [ts] else {
// [ts] newline(--context.indentLevel);
// [ts] }
// [ts] },
// [ts] newline: () => {
// [ts] newline(context.indentLevel);
// [ts] }
// [ts] };
// [ts] fn newline(n: auto) {
// [ts] context.push("\n" + `  `.repeat(n));
// [ts] }
// [ts] return context;
// [ts] }
// [ts] fn generate(ast: auto, options: auto = {}) {
// [ts] const context = createCodegenContext(ast, options);
// [ts] if (options.onContextCreated)
// [ts] options.onContextCreated(context);
// [ts] const { mode, push, prefixIdentifiers, indent, deindent, newline, scopeId, ssr } = context;
// [ts] const hasHelpers = ast.helpers.length > 0;
// [ts] const useWithBlock = !prefixIdentifiers && mode != "module";
// preambles
// in setup() inline mode, the preamble is generated in a sub context
// and returned separately.
// [ts] const preambleContext = context;
// [ts] {
// [ts] genFunctionPreamble(ast, preambleContext);
// [ts] }
// enter render function
// [ts] const functionName = if (ssr) `ssrRender` else `render`;
// [ts] const args = if (ssr) ["_ctx", "_push", "_parent", "_attrs"] else ["_ctx", "_cache"];
// [ts] const signature = args.join(", ");
// [ts] {
// [ts] push(`function ${functionName}(${signature}) {`);
// [ts] }
// [ts] indent();
// [ts] if (useWithBlock) {
// [ts] push(`with (_ctx) {`);
// [ts] indent();
// function mode const declarations should be inside with block
// also they should be renamed to avoid collision with user properties
// [ts] if (hasHelpers) {
// [ts] push(`const { ${ast.helpers
// [ts] .map(s => `${helperNameMap[s]}: _${helperNameMap[s]}`)
// [ts] .join(', ')} } = _Vue`);
// [ts] push(`\n`);
// [ts] newline();
// [ts] }
// [ts] }
// generate asset resolution statements
// [ts] if (ast.components.length) {
// [ts] genAssets(ast.components, "component", context);
// [ts] if (ast.directives.length || ast.temps > 0) {
// [ts] newline();
// [ts] }
// [ts] }
// [ts] if (ast.directives.length) {
// [ts] genAssets(ast.directives, "directive", context);
// [ts] if (ast.temps > 0) {
// [ts] newline();
// [ts] }
// [ts] }
// [ts] if (ast.temps > 0) {
// [ts] push(`let `);
// [ts] let i = 0;
// [ts] while (i < ast.temps) { // [cap-migrate] for-step: i++
// [ts] push(`${if (i > 0) `, ` else ``}_temp${i}`);
// [ts] }
// [ts] }
// [ts] if (ast.components.length || ast.directives.length || ast.temps) {
// [ts] push(`\n`);
// [ts] newline();
// [ts] }
// generate the VNode tree expression
// [ts] if (!ssr) {
// [ts] push(`return `);
// [ts] }
// [ts] if (ast.codegenNode) {
// [ts] genNode(ast.codegenNode, context);
// [ts] }
// [ts] else {
// [ts] push(`null`);
// [ts] }
// [ts] if (useWithBlock) {
// [ts] deindent();
// [ts] push(`}`);
// [ts] }
// [ts] deindent();
// [ts] push(`}`);
// [ts] return {
// [ts] ast,
// [ts] code: context.code,
// [ts] preamble: ``,
// SourceMapGenerator does have toJSON() method but it's not in the types
// [ts] map: if (context.map) context.map.toJSON() else null
// [ts] };
// [ts] }
// [ts] fn genFunctionPreamble(ast: auto, context: auto) {
// [ts] const { ssr, prefixIdentifiers, push, newline, runtimeModuleName, runtimeGlobalName } = context;
// [ts] const VueBinding = runtimeGlobalName;
// [ts] const aliasHelper = (s) => `${helperNameMap[s]}: _${helperNameMap[s]}`;
// Generate const declaration for helpers
// In prefix mode, we place the const declaration at top so it's done
// only once; But if we not prefixing, we place the declaration inside the
// with block so it doesn't incur the `in` check cost for every helper access.
// [ts] if (ast.helpers.length > 0) {
// [ts] {
// "with" mode.
// save Vue in a separate variable to avoid collision
// [ts] push(`const _Vue = ${VueBinding}\n`);
// in "with" mode, helpers are declared inside the with block to avoid
// has check cost, but hoists are lifted out of the function - we need
// to provide the helper here.
// [ts] if (ast.hoists.length) {
// [ts] const staticHelpers = [
// [ts] CREATE_VNODE,
// [ts] CREATE_COMMENT,
// [ts] CREATE_TEXT,
// [ts] CREATE_STATIC
// [ts] ]
// [ts] .filter(helper => ast.helpers.includes(helper))
// [ts] .map(aliasHelper)
// [ts] .join(", ");
// [ts] push(`const { ${staticHelpers} } = _Vue\n`);
// [ts] }
// [ts] }
// [ts] }
// [ts] genHoists(ast.hoists, context);
// [ts] newline();
// [ts] push(`return `);
// [ts] }
// [ts] fn genAssets(assets: auto, type: auto, _param: string) {
// [ts] const resolver = helper(if (type == "component") RESOLVE_COMPONENT else RESOLVE_DIRECTIVE);
// [ts] let i = 0;
// [ts] while (i < assets.length) { // [cap-migrate] for-step: i++
// [ts] let id = assets[i];
// potential component implicit self-reference inferred from SFC filename
// [ts] const maybeSelfReference = id.endsWith("__self");
// [ts] if (maybeSelfReference) {
// [ts] id = id.slice(0, -6);
// [ts] }
// [ts] push(`const ${toValidAssetId(id, type)} = ${resolver}(${JSON.stringify(id)}${if (maybeSelfReference) `, true` else ``})${if (isTS) `!` else ``}`);
// [ts] if (i < assets.length - 1) {
// [ts] newline();
// [ts] }
// [ts] }
// [ts] }
// [ts] fn genHoists(hoists: auto, context: auto) {
// [ts] if (!hoists.length) {
// [ts] return;
// [ts] }
// [ts] context.pure = true;
// [ts] const { push, newline, helper, scopeId, mode } = context;
// [ts] newline();
// [ts] hoists.forEach((exp, i) => {
// [ts] if (exp) {
// [ts] push(`const _hoisted_${i + 1} = `);
// [ts] genNode(exp, context);
// [ts] newline();
// [ts] }
// [ts] });
// [ts] context.pure = false;
// [ts] }
// [ts] (n) => {
// [ts] return (isString(n) ||
// [ts] n.type == 4 /* SIMPLE_EXPRESSION */ ||
// [ts] n.type == 2 /* TEXT */ ||
// [ts] n.type == 5 /* INTERPOLATION */ ||
// [ts] n.type == 8 /* COMPOUND_EXPRESSION */);
// [ts] }
// [ts] fn genNodeListAsArray(nodes: auto, context: auto) {
// [ts] const multilines = nodes.length > 3 ||
// [ts] (nodes.some(n => isArray(n) || !isText$1(n)));
// [ts] context.push(`[`);
// [ts] multilines && context.indent();
// [ts] genNodeList(nodes, context, multilines);
// [ts] multilines && context.deindent();
// [ts] context.push(`]`);
// [ts] }
// [ts] fn genNodeList(nodes: auto, context: auto, multilines: auto = false, comma: auto = true) {
// [ts] const { push, newline } = context;
// [ts] let i = 0;
// [ts] while (i < nodes.length) { // [cap-migrate] for-step: i++
// [ts] const node = nodes[i];
// [ts] if (isString(node)) {
// [ts] push(node);
// [ts] }
// [ts] else if (isArray(node)) {
// [ts] genNodeListAsArray(node, context);
// [ts] }
// [ts] else {
// [ts] genNode(node, context);
// [ts] }
// [ts] if (i < nodes.length - 1) {
// [ts] if (multilines) {
// [ts] comma && push(",");
// [ts] newline();
// [ts] }
// [ts] else {
// [ts] comma && push(", ");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn genNode(node: auto, context: auto) {
// [ts] if (isString(node)) {
// [ts] context.push(node);
// [ts] return;
// [ts] }
// [ts] if (isSymbol(node)) {
// [ts] context.push(context.helper(node));
// [ts] return;
// [ts] }
// [ts] switch (node.type) {
// [ts] 1 /* ELEMENT */, 9 /* IF */, 11 /* FOR */ => {
// [ts] assert(node.codegenNode != null, `Codegen node is missing for element/if/for node. ` +
// [ts] `Apply appropriate transforms first.`);
// [ts] genNode(node.codegenNode, context);
// break;
// [ts] },
// [ts] 2 /* TEXT */ => {
// [ts] genText(node, context);
// break;
// [ts] },
// [ts] 4 /* SIMPLE_EXPRESSION */ => {
// [ts] genExpression(node, context);
// break;
// [ts] },
// [ts] 5 /* INTERPOLATION */ => {
// [ts] genInterpolation(node, context);
// break;
// [ts] },
// [ts] 12 /* TEXT_CALL */ => {
// [ts] genNode(node.codegenNode, context);
// break;
// [ts] },
// [ts] 8 /* COMPOUND_EXPRESSION */ => {
// [ts] genCompoundExpression(node, context);
// break;
// [ts] },
// [ts] 3 /* COMMENT */ => {
// [ts] genComment(node, context);
// break;
// [ts] },
// [ts] 13 /* VNODE_CALL */ => {
// [ts] genVNodeCall(node, context);
// break;
// [ts] },
// [ts] 14 /* JS_CALL_EXPRESSION */ => {
// [ts] genCallExpression(node, context);
// break;
// [ts] },
// [ts] 15 /* JS_OBJECT_EXPRESSION */ => {
// [ts] genObjectExpression(node, context);
// break;
// [ts] },
// [ts] 17 /* JS_ARRAY_EXPRESSION */ => {
// [ts] genArrayExpression(node, context);
// break;
// [ts] },
// [ts] 18 /* JS_FUNCTION_EXPRESSION */ => {
// [ts] genFunctionExpression(node, context);
// break;
// [ts] },
// [ts] 19 /* JS_CONDITIONAL_EXPRESSION */ => {
// [ts] genConditionalExpression(node, context);
// break;
// [ts] },
// [ts] 20 /* JS_CACHE_EXPRESSION */ => {
// [ts] genCacheExpression(node, context);
// break;
// SSR only types
// [ts] },
// [ts] 21 /* JS_BLOCK_STATEMENT */ => {
// [ts] break;
// [ts] },
// [ts] 22 /* JS_TEMPLATE_LITERAL */ => {
// [ts] break;
// [ts] },
// [ts] 23 /* JS_IF_STATEMENT */ => {
// [ts] break;
// [ts] },
// [ts] 24 /* JS_ASSIGNMENT_EXPRESSION */ => {
// [ts] break;
// [ts] },
// [ts] 25 /* JS_SEQUENCE_EXPRESSION */ => {
// [ts] break;
// [ts] },
// [ts] 26 /* JS_RETURN_STATEMENT */ => {
// [ts] break;
// [ts] /* istanbul ignore next */
// [ts] },
// noop
// [ts] 10 /* IF_BRANCH */ => {
// [ts] break;
// [ts] },
// [ts] else => {
// [ts] {
// [ts] assert(false, `unhandled codegen node type: ${node.type}`);
// make sure we exhaust all possible types
// [ts] const exhaustiveCheck = node;
// [ts] return exhaustiveCheck;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn genText(node: auto, context: auto) {
// [ts] context.push(JSON.stringify(node.content), node);
// [ts] }
// [ts] fn genExpression(node: auto, context: auto) {
// [ts] const { content, isStatic } = node;
// [ts] context.push(if (isStatic) JSON.stringify(content) else content, node);
// [ts] }
// [ts] fn genInterpolation(node: auto, context: auto) {
// [ts] const { push, helper, pure } = context;
// [ts] if (pure)
// [ts] push(PURE_ANNOTATION);
// [ts] push(`${helper(TO_DISPLAY_STRING)}(`);
// [ts] genNode(node.content, context);
// [ts] push(`)`);
// [ts] }
// [ts] fn genCompoundExpression(node: auto, context: auto) {
// [ts] let i = 0;
// [ts] while (i < node.children.length) { // [cap-migrate] for-step: i++
// [ts] const child = node.children[i];
// [ts] if (isString(child)) {
// [ts] context.push(child);
// [ts] }
// [ts] else {
// [ts] genNode(child, context);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn genExpressionAsPropertyKey(node: auto, context: auto) {
// [ts] const { push } = context;
// [ts] if (node.type == 8 /* COMPOUND_EXPRESSION */) {
// [ts] push(`[`);
// [ts] genCompoundExpression(node, context);
// [ts] push(`]`);
// [ts] }
// [ts] else if (node.isStatic) {
// only quote keys if necessary
// [ts] const text = if (isSimpleIdentifier(node.content)) node.content else JSON.stringify(node.content);
// [ts] push(text, node);
// [ts] }
// [ts] else {
// [ts] push(`[${node.content}]`, node);
// [ts] }
// [ts] }
// [ts] fn genComment(node: auto, context: auto) {
// [ts] const { push, helper, pure } = context;
// [ts] if (pure) {
// [ts] push(PURE_ANNOTATION);
// [ts] }
// [ts] push(`${helper(CREATE_COMMENT)}(${JSON.stringify(node.content)})`, node);
// [ts] }
// [ts] fn genVNodeCall(node: auto, context: auto) {
// [ts] const { push, helper, pure } = context;
// [ts] const { tag, props, children, patchFlag, dynamicProps, directives, isBlock, disableTracking } = node;
// [ts] if (directives) {
// [ts] push(helper(WITH_DIRECTIVES) + `(`);
// [ts] }
// [ts] if (isBlock) {
// [ts] push(`(${helper(OPEN_BLOCK)}(${if (disableTracking) `true` else ``}), `);
// [ts] }
// [ts] if (pure) {
// [ts] push(PURE_ANNOTATION);
// [ts] }
// [ts] push(helper(if (isBlock) CREATE_BLOCK else CREATE_VNODE) + `(`, node);
// [ts] genNodeList(genNullableArgs([tag, props, children, patchFlag, dynamicProps]), context);
// [ts] push(`)`);
// [ts] if (isBlock) {
// [ts] push(`)`);
// [ts] }
// [ts] if (directives) {
// [ts] push(`, `);
// [ts] genNode(directives, context);
// [ts] push(`)`);
// [ts] }
// [ts] }
// [ts] fn genNullableArgs(args: auto) {
// [ts] let i = args.length;
// [ts] while (i--) {
// [ts] if (args[i] != null)
// [ts] break;
// [ts] }
// [ts] return args.slice(0, i + 1).map(arg => arg || `null`);
// [ts] }
// JavaScript
// [ts] fn genCallExpression(node: auto, context: auto) {
// [ts] const { push, helper, pure } = context;
// [ts] const callee = if (isString(node.callee)) node.callee else helper(node.callee);
// [ts] if (pure) {
// [ts] push(PURE_ANNOTATION);
// [ts] }
// [ts] push(callee + `(`, node);
// [ts] genNodeList(node.arguments, context);
// [ts] push(`)`);
// [ts] }
// [ts] fn genObjectExpression(node: auto, context: auto) {
// [ts] const { push, indent, deindent, newline } = context;
// [ts] const { properties } = node;
// [ts] if (!properties.length) {
// [ts] push(`{}`, node);
// [ts] return;
// [ts] }
// [ts] const multilines = properties.length > 1 ||
// [ts] (properties.some(p => p.value.type != 4 /* SIMPLE_EXPRESSION */));
// [ts] push(if (multilines) `{` else `{ `);
// [ts] multilines && indent();
// [ts] let i = 0;
// [ts] while (i < properties.length) { // [cap-migrate] for-step: i++
// [ts] const { key, value } = properties[i];
// key
// [ts] genExpressionAsPropertyKey(key, context);
// [ts] push(`: `);
// value
// [ts] genNode(value, context);
// [ts] if (i < properties.length - 1) {
// will only reach this if it's multilines
// [ts] push(`,`);
// [ts] newline();
// [ts] }
// [ts] }
// [ts] multilines && deindent();
// [ts] push(if (multilines) `}` else ` }`);
// [ts] }
// [ts] fn genArrayExpression(node: auto, context: auto) {
// [ts] genNodeListAsArray(node.elements, context);
// [ts] }
// [ts] fn genFunctionExpression(node: auto, context: auto) {
// [ts] const { push, indent, deindent, scopeId, mode } = context;
// [ts] const { params, returns, body, newline, isSlot } = node;
// [ts] if (isSlot) {
// wrap slot functions with owner context
// [ts] push(`_${helperNameMap[WITH_CTX]}(`);
// [ts] }
// [ts] push(`(`, node);
// [ts] if (isArray(params)) {
// [ts] genNodeList(params, context);
// [ts] }
// [ts] else if (params) {
// [ts] genNode(params, context);
// [ts] }
// [ts] push(`) => `);
// [ts] if (newline || body) {
// [ts] push(`{`);
// [ts] indent();
// [ts] }
// [ts] if (returns) {
// [ts] if (newline) {
// [ts] push(`return `);
// [ts] }
// [ts] if (isArray(returns)) {
// [ts] genNodeListAsArray(returns, context);
// [ts] }
// [ts] else {
// [ts] genNode(returns, context);
// [ts] }
// [ts] }
// [ts] else if (body) {
// [ts] genNode(body, context);
// [ts] }
// [ts] if (newline || body) {
// [ts] deindent();
// [ts] push(`}`);
// [ts] }
// [ts] if (isSlot) {
// [ts] push(`)`);
// [ts] }
// [ts] }
// [ts] fn genConditionalExpression(node: auto, context: auto) {
// [ts] const { test, consequent, alternate, newline: needNewline } = node;
// [ts] const { push, indent, deindent, newline } = context;
// [ts] if (test.type == 4 /* SIMPLE_EXPRESSION */) {
// [ts] const needsParens = !isSimpleIdentifier(test.content);
// [ts] needsParens && push(`(`);
// [ts] genExpression(test, context);
// [ts] needsParens && push(`)`);
// [ts] }
// [ts] else {
// [ts] push(`(`);
// [ts] genNode(test, context);
// [ts] push(`)`);
// [ts] }
// [ts] needNewline && indent();
// [ts] context.indentLevel++;
// [ts] needNewline || push(` `);
// [ts] push(`? `);
// [ts] genNode(consequent, context);
// [ts] context.indentLevel--;
// [ts] needNewline && newline();
// [ts] needNewline || push(` `);
// [ts] push(`: `);
// [ts] const isNested = alternate.type == 19 /* JS_CONDITIONAL_EXPRESSION */;
// [ts] if (!isNested) {
// [ts] context.indentLevel++;
// [ts] }
// [ts] genNode(alternate, context);
// [ts] if (!isNested) {
// [ts] context.indentLevel--;
// [ts] }
// [ts] needNewline && deindent(true /* without newline */);
// [ts] }
// [ts] fn genCacheExpression(node: auto, context: auto) {
// [ts] const { push, helper, indent, deindent, newline } = context;
// [ts] push(`_cache[${node.index}] || (`);
// [ts] if (node.isVNode) {
// [ts] indent();
// [ts] push(`${helper(SET_BLOCK_TRACKING)}(-1),`);
// [ts] newline();
// [ts] }
// [ts] push(`_cache[${node.index}] = `);
// [ts] genNode(node.value, context);
// [ts] if (node.isVNode) {
// [ts] push(`,`);
// [ts] newline();
// [ts] push(`${helper(SET_BLOCK_TRACKING)}(1),`);
// [ts] newline();
// [ts] push(`_cache[${node.index}]`);
// [ts] deindent();
// [ts] }
// [ts] push(`)`);
// [ts] }
// these keywords should not appear inside expressions, but operators like
// typeof, instanceof and in are allowed
// [ts] const prohibitedKeywordRE = RegExp.init("\\b" +
// [ts] ("do,if,for,let,new,try,var,case,else,with,await,break,catch,class,const," +
// [ts] "super,throw,while,yield,delete,export,import,return,switch,default," +
// [ts] "extends,finally,continue,debugger,function,arguments,typeof,void")
// [ts] .split(",")
// [ts] .join("\\b|\\b") +
// [ts] "\\b");
// strip strings in expressions
// [ts] const stripStringRE = "'(?:[^'\\\\]|\\\\.)*'|\"(?:[^\"\\\\]|\\\\.)*\"|`(?:[^`\\\\]|\\\\.)*\\$\\{|\\}(?:[^`\\\\]|\\\\.)*`|`(?:[^`\\\\]|\\\\.)*`";
// [ts] /**
// [ts] * Validate a non-prefixed expression.
// [ts] * This is only called when using the in-browser runtime compiler since it
// [ts] * doesn't prefix expressions.
// [ts] */
// [ts] fn validateBrowserExpression(node: auto, context: auto, asParams: auto = false, asRawStatements: auto = false) {
// [ts] const exp = node.content;
// empty expressions are validated per-directive since some directives
// do allow empty expressions.
// [ts] if (!exp.trim()) {
// [ts] return;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: new Function(asRawStatements
// [ts] if () ` ${exp} ` else `return ${if (asParams) `(${exp}) => {}` else `(${exp})`}`);
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ? (catch (e) {)
// [ts] let message = e.message;
// [ts] const keywordMatch = exp
// [ts] .replace(stripStringRE, "")
// [ts] .match(prohibitedKeywordRE);
// [ts] if (keywordMatch) {
// [ts] message = `avoid using JavaScript keyword as property name: "${keywordMatch[0]}"`;
// [ts] }
// [ts] context.onError(createCompilerError(43 /* X_INVALID_EXPRESSION */, node.loc, null, message));
// [ts] }
// [ts] }
// [ts] fn transformExpression(node: auto, context: auto) {
// [ts] if (node.type == 5 /* INTERPOLATION */) {
// [ts] node.content = processExpression(node.content, context);
// [ts] }
// [ts] else if (node.type == 1 /* ELEMENT */) {
// handle directives on element
// [ts] let i = 0;
// [ts] while (i < node.props.length) { // [cap-migrate] for-step: i++
// [ts] const dir = node.props[i];
// do not process for v-on & v-for since they are special handled
// [ts] if (dir.type == 7 /* DIRECTIVE */ && dir.name != "for") {
// [ts] const exp = dir.exp;
// [ts] const arg = dir.arg;
// do not process exp if this is v-on:arg - we need special handling
// for wrapping inline statements.
// [ts] if (exp &&
// [ts] exp.type == 4 /* SIMPLE_EXPRESSION */ &&
// [ts] !(dir.name == "on" && arg)) {
// [ts] dir.exp = processExpression(exp, context,
// slot args must be processed as function params
// [ts] dir.name == "slot");
// [ts] }
// [ts] if (arg && arg.type == 4 /* SIMPLE_EXPRESSION */ && !arg.isStatic) {
// [ts] dir.arg = processExpression(arg, context);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// Important: since this function uses Node.js only dependencies, it should
// always be used with a leading !true check so that it can be
// tree-shaken from the browser build.
// [ts] fn processExpression(node: auto, context: auto, // some expressions like v-slot props & v-for aliases should be parsed as // function params asParams: auto = false, // v-on handler values may contain multiple statements asRawStatements: auto = false) {
// [ts] {
// [ts] {
// simple in-browser validation (same logic in 2.x)
// [ts] validateBrowserExpression(node, context, asParams, asRawStatements);
// [ts] }
// [ts] return node;
// [ts] }
// [ts] }
// [ts] const transformIf = createStructuralDirectiveTransform("^(if|else|else-if)$", (node, dir, context) => {
// [ts] return processIf(node, dir, context, (ifNode, branch, isRoot) => {
// #1587: We need to dynamically increment the key based on the current
// node's sibling nodes, since chained v-if/else branches are
// rendered at the same depth
// [ts] const siblings = context.parent.children;
// [ts] let i = siblings.indexOf(ifNode);
// [ts] let key = 0;
// [ts] while (i-- >= 0) {
// [ts] const sibling = siblings[i];
// [ts] if (sibling && sibling.type == 9 /* IF */) {
// [ts] key += sibling.branches.length;
// [ts] }
// [ts] }
// Exit callback. Complete the codegenNode when all children have been
// transformed.
// [ts] return () => {
// [ts] if (isRoot) {
// [ts] ifNode.codegenNode = createCodegenNodeForBranch(branch, key, context);
// [ts] }
// [ts] else {
// attach this branch's codegen node to the v-if root.
// [ts] const parentCondition = getParentCondition(ifNode.codegenNode);
// [ts] parentCondition.alternate = createCodegenNodeForBranch(branch, key + ifNode.branches.length - 1, context);
// [ts] }
// [ts] };
// [ts] });
// [ts] });
// target-agnostic transform used for both Client and SSR
// [ts] fn processIf(node: auto, dir: auto, context: auto, processCodegen: auto) {
// [ts] if (dir.name != "else" &&
// [ts] (!dir.exp || !dir.exp.content.trim())) {
// [ts] const loc = if (dir.exp) dir.exp.loc else node.loc;
// [ts] context.onError(createCompilerError(27 /* X_V_IF_NO_EXPRESSION */, dir.loc));
// [ts] dir.exp = createSimpleExpression(`true`, false, loc);
// [ts] }
// [ts] if (dir.exp) {
// [ts] validateBrowserExpression(dir.exp, context);
// [ts] }
// [ts] if (dir.name == "if") {
// [ts] const branch = createIfBranch(node, dir);
// [ts] const ifNode = {
// [ts] type: 9 /* IF */,
// [ts] loc: node.loc,
// [ts] branches: [branch]
// [ts] };
// [ts] context.replaceNode(ifNode);
// [ts] if (processCodegen) {
// [ts] return processCodegen(ifNode, branch, true);
// [ts] }
// [ts] }
// [ts] else {
// locate the adjacent v-if
// [ts] const siblings = context.parent.children;
// [ts] const comments = [];
// [ts] let i = siblings.indexOf(node);
// [ts] while (i-- >= -1) {
// [ts] const sibling = siblings[i];
// [ts] if (sibling && sibling.type == 3 /* COMMENT */) {
// [ts] context.removeNode(sibling);
// [ts] comments.unshift(sibling);
// [ts] continue;
// [ts] }
// [ts] if (sibling &&
// [ts] sibling.type == 2 /* TEXT */ &&
// [ts] !sibling.content.trim().length) {
// [ts] context.removeNode(sibling);
// [ts] continue;
// [ts] }
// [ts] if (sibling && sibling.type == 9 /* IF */) {
// move the node to the if node's branches
// [ts] context.removeNode();
// [ts] const branch = createIfBranch(node, dir);
// [ts] if (comments.length &&
// #3619 ignore comments if the v-if is direct child of <transition>
// [ts] !(context.parent &&
// [ts] context.parent.type == 1 /* ELEMENT */ &&
// [ts] isBuiltInType: (context.parent.tag, 'transition') => {
// [ts] branch.children = [...comments, ...branch.children];
// [ts] }
// check if user is forcing same key on different branches
// [ts] {
// [ts] const key = branch.userKey;
// [ts] if (key) {
// [ts] sibling.branches.forEach((_p) => {
// [ts] if (isSameKey(userKey, key)) {
// [ts] context.onError(createCompilerError(28 /* X_V_IF_SAME_KEY */, branch.userKey.loc));
// [ts] }
// [ts] });
// [ts] }
// [ts] }
// [ts] sibling.branches.push(branch);
// [ts] const onExit = processCodegen && processCodegen(sibling, branch, false);
// since the branch was removed, it will not be traversed.
// make sure to traverse here.
// [ts] traverseNode(branch, context);
// call on exit
// [ts] if (onExit)
// [ts] onExit();
// make sure to reset currentNode after traversal to indicate this
// node has been removed.
// [ts] context.currentNode = null;
// [ts] }
// [ts] else {
// [ts] context.onError(createCompilerError(29 /* X_V_ELSE_NO_ADJACENT_IF */, node.loc));
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn createIfBranch(node: auto, dir: auto) {
// [ts] return {
// [ts] type: 10 /* IF_BRANCH */,
// [ts] loc: node.loc,
// [ts] condition: if (dir.name == "else") null else dir.exp,
// [ts] children: if (node.tagType == 3 /* TEMPLATE */ && !findDir(node, "for")) node.children else [node],
// [ts] userKey: findProp(node, `key`)
// [ts] };
// [ts] }
// [ts] fn createCodegenNodeForBranch(branch: auto, keyIndex: auto, context: auto) {
// [ts] if (branch.condition) {
// [ts] return createConditionalExpression(branch.condition, createChildrenCodegenNode(branch, keyIndex, context),
// make sure to pass in asBlock: true so that the comment node call
// closes the current block.
// [ts] createCallExpression(context.helper(CREATE_COMMENT), [
// [ts] "\"v-if\"" ,
// [ts] "true"
// [ts] ]));
// [ts] }
// [ts] else {
// [ts] return createChildrenCodegenNode(branch, keyIndex, context);
// [ts] }
// [ts] }
// [ts] fn createChildrenCodegenNode(branch: auto, keyIndex: auto, context: auto) {
// [ts] const { helper, removeHelper } = context;
// [ts] const keyProperty = createObjectProperty(`key`, createSimpleExpression(`${keyIndex}`, false, locStub, 2 /* CAN_HOIST */));
// [ts] const { children } = branch;
// [ts] const firstChild = children[0];
// [ts] const needFragmentWrapper = children.length != 1 || firstChild.type != 1 /* ELEMENT */;
// [ts] if (needFragmentWrapper) {
// [ts] if (children.length == 1 && firstChild.type == 11 /* FOR */) {
// optimize away nested fragments when child is a ForNode
// [ts] const vnodeCall = firstChild.codegenNode;
// [ts] injectProp(vnodeCall, keyProperty, context);
// [ts] return vnodeCall;
// [ts] }
// [ts] else {
// [ts] let patchFlag = 64 /* STABLE_FRAGMENT */;
// [ts] let patchFlagText = PatchFlagNames[64 /* STABLE_FRAGMENT */];
// check if the fragment actually contains a single valid child with
// the rest being comments
// [ts] if (children.filter(c => c.type != 3 /* COMMENT */).length == 1) {
// [ts] patchFlag |= 2048 /* DEV_ROOT_FRAGMENT */;
// [ts] patchFlagText += `, ${PatchFlagNames[2048 /* DEV_ROOT_FRAGMENT */]}`;
// [ts] }
// [ts] return createVNodeCall(context, helper(FRAGMENT), createObjectExpression([keyProperty]), children, patchFlag + (` /* ${patchFlagText} */` ), null, null, true, false, branch.loc);
// [ts] }
// [ts] }
// [ts] else {
// [ts] const vnodeCall = firstChild
// [ts] .codegenNode;
// Change createVNode to createBlock.
// [ts] if (vnodeCall.type == 13 /* VNODE_CALL */ && !vnodeCall.isBlock) {
// [ts] removeHelper(CREATE_VNODE);
// [ts] vnodeCall.isBlock = true;
// [ts] helper(OPEN_BLOCK);
// [ts] helper(CREATE_BLOCK);
// [ts] }
// inject branch key
// [ts] injectProp(vnodeCall, keyProperty, context);
// [ts] return vnodeCall;
// [ts] }
// [ts] }
// [ts] fn isSameKey(a: auto, b: auto) {
// [ts] if (!a || a.type != b.type) {
// [ts] return false;
// [ts] }
// [ts] if (a.type == 6 /* ATTRIBUTE */) {
// [ts] if (a.value.content != b.value.content) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] else {
// directive
// [ts] const exp = a.exp;
// [ts] const branchExp = b.exp;
// [ts] if (exp.type != branchExp.type) {
// [ts] return false;
// [ts] }
// [ts] if (exp.type != 4 /* SIMPLE_EXPRESSION */ ||
// [ts] (exp.isStatic != branchExp.isStatic ||
// [ts] exp.content != branchExp.content)) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn getParentCondition(node: auto) {
// [ts] while (true) {
// [ts] if (node.type == 19 /* JS_CONDITIONAL_EXPRESSION */) {
// [ts] if (node.alternate.type == 19 /* JS_CONDITIONAL_EXPRESSION */) {
// [ts] node = node.alternate;
// [ts] }
// [ts] else {
// [ts] return node;
// [ts] }
// [ts] }
// [ts] else if (node.type == 20 /* JS_CACHE_EXPRESSION */) {
// [ts] node = node.value;
// [ts] }
// [ts] }
// [ts] }
// [ts] const transformFor = createStructuralDirectiveTransform("for", (node, dir, context) => {
// [ts] const { helper, removeHelper } = context;
// [ts] return processFor(node, dir, context, forNode => {
// create the loop render function expression now, and add the
// iterator on exit after all children have been traversed
// [ts] const renderExp = createCallExpression(helper(RENDER_LIST), [
// [ts] forNode.source
// [ts] ]);
// [ts] const keyProp = findProp(node, `key`);
// [ts] const keyProperty = if (keyProp) createObjectProperty(if (`key`, keyProp.type == 6 /* ATTRIBUTE */) createSimpleExpression(keyProp.value.content, true) else keyProp.exp) else null;
// [ts] const isStableFragment = forNode.source.type == 4 /* SIMPLE_EXPRESSION */ &&
// [ts] forNode.source.constType > 0 /* NOT_CONSTANT */;
// [ts] const fragmentFlag = if (isStableFragment) 64 /* STABLE_FRAGMENT */ else if (keyProp) 128 /* KEYED_FRAGMENT */ else 256 /* UNKEYED_FRAGMENT */;
// [ts] forNode.codegenNode = createVNodeCall(context, helper(FRAGMENT), null, renderExp, fragmentFlag +
// [ts] (` /* ${PatchFlagNames[fragmentFlag]} */` ), null, null, true /* isBlock */, !isStableFragment /* disableTracking */, node.loc);
// [ts] return () => {
// finish the codegen now that all children have been traversed
// [ts] let childBlock;
// [ts] const isTemplate = isTemplateNode(node);
// [ts] const { children } = forNode;
// check <template v-for> key placement
// [ts] if (isTemplate) {
// [ts] node.children.some(c => {
// [ts] if (c.type == 1 /* ELEMENT */) {
// [ts] const key = findProp(c, "key");
// [ts] if (key) {
// [ts] context.onError(createCompilerError(32 /* X_V_FOR_TEMPLATE_KEY_PLACEMENT */, key.loc));
// [ts] return true;
// [ts] }
// [ts] }
// [ts] });
// [ts] }
// [ts] const needFragmentWrapper = children.length != 1 || children[0].type != 1 /* ELEMENT */;
// [ts] const slotOutlet = if (isSlotOutlet(node)) node else isTemplate &&
// [ts] node.children.length == 1 &&
// [ts] isSlotOutlet(node.children[0]) ? node.children[0] // api-extractor somehow fails to infer this : null;
// [ts] if (slotOutlet) {
// <slot v-for="..."> or <template v-for="..."><slot/></template>
// [ts] childBlock = slotOutlet.codegenNode;
// [ts] if (isTemplate && keyProperty) {
// <template v-for="..." :key="..."><slot/></template>
// we need to inject the key to the renderSlot() call.
// the props for renderSlot is passed as the 3rd argument.
// [ts] injectProp(childBlock, keyProperty, context);
// [ts] }
// [ts] }
// [ts] else if (needFragmentWrapper) {
// <template v-for="..."> with text or multi-elements
// should generate a fragment block for each loop
// [ts] childBlock = createVNodeCall(context, helper(FRAGMENT), keyProperty ? createObjectExpression([keyProperty]) : null, node.children, 64 /* STABLE_FRAGMENT */ +
// [ts] (` /* ${PatchFlagNames[64 /* STABLE_FRAGMENT */]} */`
// [ts] ), null, null, true);
// [ts] }
// [ts] else {
// Normal element v-for. Directly use the child's codegenNode
// but mark it as a block.
// [ts] childBlock = children[0]
// [ts] .codegenNode;
// [ts] if (isTemplate && keyProperty) {
// [ts] injectProp(childBlock, keyProperty, context);
// [ts] }
// [ts] if (childBlock.isBlock != !isStableFragment) {
// [ts] if (childBlock.isBlock) {
// switch from block to vnode
// [ts] removeHelper(OPEN_BLOCK);
// [ts] removeHelper(CREATE_BLOCK);
// [ts] }
// [ts] else {
// switch from vnode to block
// [ts] removeHelper(CREATE_VNODE);
// [ts] }
// [ts] }
// [ts] childBlock.isBlock = !isStableFragment;
// [ts] if (childBlock.isBlock) {
// [ts] helper(OPEN_BLOCK);
// [ts] helper(CREATE_BLOCK);
// [ts] }
// [ts] else {
// [ts] helper(CREATE_VNODE);
// [ts] }
// [ts] }
// [ts] renderExp.arguments.push(createFunctionExpression(createForLoopParams(forNode.parseResult), childBlock, true /* force newline */));
// [ts] };
// [ts] });
// [ts] });
// target-agnostic transform used for both Client and SSR
// [ts] fn processFor(node: auto, dir: auto, context: auto, processCodegen: auto) {
// [ts] if (!dir.exp) {
// [ts] context.onError(createCompilerError(30 /* X_V_FOR_NO_EXPRESSION */, dir.loc));
// [ts] return;
// [ts] }
// [ts] const parseResult = parseForExpression(
// can only be simple expression because vFor transform is applied
// before expression transform.
// [ts] dir.exp, context);
// [ts] if (!parseResult) {
// [ts] context.onError(createCompilerError(31 /* X_V_FOR_MALFORMED_EXPRESSION */, dir.loc));
// [ts] return;
// [ts] }
// [ts] const { addIdentifiers, removeIdentifiers, scopes } = context;
// [ts] const { source, value, key, index } = parseResult;
// [ts] const forNode = {
// [ts] type: 11 /* FOR */,
// [ts] loc: dir.loc,
// [ts] source,
// [ts] valueAlias: value,
// [ts] keyAlias: key,
// [ts] objectIndexAlias: index,
// [ts] parseResult,
// [ts] children: if (isTemplateNode(node)) node.children else [node]
// [ts] };
// [ts] context.replaceNode(forNode);
// bookkeeping
// [ts] scopes.vFor++;
// [ts] const onExit = processCodegen && processCodegen(forNode);
// [ts] return () => {
// [ts] scopes.vFor--;
// [ts] if (onExit)
// [ts] onExit();
// [ts] };
// [ts] }
// [ts] const forAliasRE = "([\\s\\S]*?)\\s+(?:in|of)\\s+([\\s\\S]*)";
// This regex doesn't cover the case if key or index aliases have destructuring,
// but those do not make sense in the first place, so this works in practice.
// [ts] const forIteratorRE = ",([^,\\}\\]]*)(?:,([^,\\}\\]]*))?$";
// [ts] const stripParensRE = "^\\(|\\)$";
// [ts] fn parseForExpression(input: auto, context: auto) {
// [ts] const loc = input.loc;
// [ts] const exp = input.content;
// [ts] const inMatch = exp.match(forAliasRE);
// [ts] if (!inMatch)
// [ts] return;
// [ts] const [, LHS, RHS] = inMatch;
// [ts] const result = {
// [ts] source: createAliasExpression(loc, RHS.trim(), exp.indexOf(RHS, LHS.length)),
// [ts] value: null,
// [ts] key: null,
// [ts] index: null
// [ts] };
// [ts] {
// [ts] validateBrowserExpression(result.source, context);
// [ts] }
// [ts] let valueContent = LHS.trim()
// [ts] .replace(stripParensRE, "")
// [ts] .trim();
// [ts] const trimmedOffset = LHS.indexOf(valueContent);
// [ts] const iteratorMatch = valueContent.match(forIteratorRE);
// [ts] if (iteratorMatch) {
// [ts] valueContent = valueContent.replace(forIteratorRE, "").trim();
// [ts] const keyContent = iteratorMatch[1].trim();
// [ts] let keyOffset;
// [ts] if (keyContent) {
// [ts] keyOffset = exp.indexOf(keyContent, trimmedOffset + valueContent.length);
// [ts] result.key = createAliasExpression(loc, keyContent, keyOffset);
// [ts] {
// [ts] validateBrowserExpression(result.key, context, true);
// [ts] }
// [ts] }
// [ts] if (iteratorMatch[2]) {
// [ts] const indexContent = iteratorMatch[2].trim();
// [ts] if (indexContent) {
// [ts] result.index = createAliasExpression(loc, indexContent, exp.indexOf(if (indexContent, result.key) keyOffset + keyContent.length else trimmedOffset + valueContent.length));
// [ts] {
// [ts] validateBrowserExpression(result.index, context, true);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (valueContent) {
// [ts] result.value = createAliasExpression(loc, valueContent, trimmedOffset);
// [ts] {
// [ts] validateBrowserExpression(result.value, context, true);
// [ts] }
// [ts] }
// [ts] return result;
// [ts] }
// [ts] fn createAliasExpression(range: auto, content: auto, offset: auto) {
// [ts] return createSimpleExpression(content, false, getInnerRange(range, offset, content.length));
// [ts] }
// [ts] fn createForLoopParams(_param: string) {
// [ts] const params = [];
// [ts] if (value) {
// [ts] params.push(value);
// [ts] }
// [ts] if (key) {
// [ts] if (!value) {
// [ts] params.push(createSimpleExpression(`_`, false));
// [ts] }
// [ts] params.push(key);
// [ts] }
// [ts] if (index) {
// [ts] if (!key) {
// [ts] if (!value) {
// [ts] params.push(createSimpleExpression(`_`, false));
// [ts] }
// [ts] params.push(createSimpleExpression(`__`, false));
// [ts] }
// [ts] params.push(index);
// [ts] }
// [ts] return params;
// [ts] }
// [ts] const defaultFallback = createSimpleExpression(`undefined`, false);
// A NodeTransform that:
// 1. Tracks scope identifiers for scoped slots so that they don't get prefixed
//    by transformExpression. This is only applied in non-browser builds with
//    { prefixIdentifiers: true }.
// 2. Track v-slot depths so that we know a slot is inside another slot.
//    Note the exit callback is executed before buildSlots() on the same node,
//    so only nested slots see positive numbers.
// [ts] fn trackSlotScopes(node: auto, context: auto) {
// [ts] if (node.type == 1 /* ELEMENT */ &&
// [ts] (node.tagType == 1 /* COMPONENT */ ||
// [ts] node.tagType == 3 /* TEMPLATE */)) {
// We are only checking non-empty v-slot here
// since we only care about slots that introduce scope variables.
// [ts] const vSlot = findDir(node, "slot");
// [ts] if (vSlot) {
// [ts] vSlot.exp;
// [ts] context.scopes.vSlot++;
// [ts] return () => {
// [ts] context.scopes.vSlot--;
// [ts] };
// [ts] }
// [ts] }
// [ts] };
// [ts] fn buildClientSlotFn(props: auto, children: auto, loc: auto) { return createFunctionExpression(if (props, children, false /* newline */, true /* isSlot */, children.length) children[0].loc else loc); }
// Instead of being a DirectiveTransform, v-slot processing is called during
// transformElement to build the slots object for a component.
// [ts] fn buildSlots(node: auto, context: auto, buildSlotFn: auto = buildClientSlotFn) {
// [ts] context.helper(WITH_CTX);
// [ts] const { children, loc } = node;
// [ts] const slotsProperties = [];
// [ts] const dynamicSlots = [];
// If the slot is inside a v-for or another v-slot, force it to be dynamic
// since it likely uses a scope variable.
// [ts] let hasDynamicSlots = context.scopes.vSlot > 0 || context.scopes.vFor > 0;
// 1. Check for slot with slotProps on component itself.
//    <Comp v-slot="{ prop }"/>
// [ts] const onComponentSlot = findDir(node, "slot", true);
// [ts] if (onComponentSlot) {
// [ts] const { arg, exp } = onComponentSlot;
// [ts] if (arg && !isStaticExp(arg)) {
// [ts] hasDynamicSlots = true;
// [ts] }
// [ts] slotsProperties.push(createObjectProperty(arg || createSimpleExpression("default", true), buildSlotFn(exp, children, loc)));
// [ts] }
// 2. Iterate through children and check for template slots
//    <template v-slot:foo="{ prop }">
// [ts] let hasTemplateSlots = false;
// [ts] let hasNamedDefaultSlot = false;
// [ts] const implicitDefaultChildren = [];
// [ts] const seenSlotNames = Set.init();
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const slotElement = children[i];
// [ts] let slotDir;
// [ts] if (!isTemplateNode(slotElement) ||
// [ts] !(slotDir = findDir(slotElement, "slot", true))) {
// not a <template v-slot>, skip.
// [ts] if (slotElement.type != 3 /* COMMENT */) {
// [ts] implicitDefaultChildren.push(slotElement);
// [ts] }
// [ts] continue;
// [ts] }
// [ts] if (onComponentSlot) {
// already has on-component slot - this is incorrect usage.
// [ts] context.onError(createCompilerError(36 /* X_V_SLOT_MIXED_SLOT_USAGE */, slotDir.loc));
// [ts] break;
// [ts] }
// [ts] hasTemplateSlots = true;
// [ts] const { children: slotChildren, loc: slotLoc } = slotElement;
// [ts] const { arg: slotName = createSimpleExpression(`default`, true), exp: slotProps, loc: dirLoc } = slotDir;
// check if name is dynamic.
// [ts] let staticSlotName;
// [ts] if (isStaticExp(slotName)) {
// [ts] staticSlotName = if (slotName) slotName.content else `default`;
// [ts] }
// [ts] else {
// [ts] hasDynamicSlots = true;
// [ts] }
// [ts] const slotFunction = buildSlotFn(slotProps, slotChildren, slotLoc);
// check if this slot is conditional (v-if/v-for)
// [ts] let vIf;
// [ts] let vElse;
// [ts] let vFor;
// [ts] if ((vIf = findDir(slotElement, "if"))) {
// [ts] hasDynamicSlots = true;
// [ts] dynamicSlots.push(createConditionalExpression(vIf.exp, buildDynamicSlot(slotName, slotFunction), defaultFallback));
// [ts] }
// [ts] else if ((vElse = findDir(slotElement, "^else(-if)?$", true /* allowEmpty */))) {
// find adjacent v-if
// [ts] let j = i;
// [ts] let prev;
// [ts] while (j--) {
// [ts] prev = children[j];
// [ts] if (prev.type != 3 /* COMMENT */) {
// [ts] break;
// [ts] }
// [ts] }
// [ts] if (prev && isTemplateNode(prev) && findDir(prev, "if")) {
// remove node
// [ts] children.splice(i, 1);
// [ts] i--;
// attach this slot to previous conditional
// [ts] let conditional = dynamicSlots[dynamicSlots.length - 1];
// [ts] while (conditional.alternate.type == 19 /* JS_CONDITIONAL_EXPRESSION */) {
// [ts] conditional = conditional.alternate;
// [ts] }
// [ts] conditional.alternate = if (vElse.exp) createConditionalExpression(vElse.exp, buildDynamicSlot(slotName, slotFunction), defaultFallback) else buildDynamicSlot(slotName, slotFunction);
// [ts] }
// [ts] else {
// [ts] context.onError(createCompilerError(29 /* X_V_ELSE_NO_ADJACENT_IF */, vElse.loc));
// [ts] }
// [ts] }
// [ts] else if ((vFor = findDir(slotElement, "for"))) {
// [ts] hasDynamicSlots = true;
// [ts] const parseResult = vFor.parseResult ||
// [ts] parseForExpression(vFor.exp, context);
// [ts] if (parseResult) {
// Render the dynamic slots as an array and add it to the createSlot()
// args. The runtime knows how to handle it appropriately.
// [ts] dynamicSlots.push(createCallExpression(context.helper(RENDER_LIST), [
// [ts] parseResult.source,
// [ts] createFunctionExpression(createForLoopParams(parseResult), buildDynamicSlot(slotName, slotFunction), true /* force newline */)
// [ts] ]));
// [ts] }
// [ts] else {
// [ts] context.onError(createCompilerError(31 /* X_V_FOR_MALFORMED_EXPRESSION */, vFor.loc));
// [ts] }
// [ts] }
// [ts] else {
// check duplicate static names
// [ts] if (staticSlotName) {
// [ts] if (seenSlotNames.has(staticSlotName)) {
// [ts] context.onError(createCompilerError(37 /* X_V_SLOT_DUPLICATE_SLOT_NAMES */, dirLoc));
// [ts] continue;
// [ts] }
// [ts] seenSlotNames.add(staticSlotName);
// [ts] if (staticSlotName == "default") {
// [ts] hasNamedDefaultSlot = true;
// [ts] }
// [ts] }
// [ts] slotsProperties.push(createObjectProperty(slotName, slotFunction));
// [ts] }
// [ts] }
// [ts] if (!onComponentSlot) {
// [ts] const buildDefaultSlotProperty = (props, children) => {
// [ts] const fn = buildSlotFn(props, children, loc);
// [ts] return createObjectProperty(`default`, fn);
// [ts] };
// [ts] if (!hasTemplateSlots) {
// implicit default slot (on component)
// [ts] slotsProperties.push(buildDefaultSlotProperty(null, children));
// [ts] }
// [ts] else if (implicitDefaultChildren.length &&
// #3766
// with whitespace: 'preserve', whitespaces between slots will end up in
// implicitDefaultChildren. Ignore if all implicit children are whitespaces.
// [ts] implicitDefaultChildren.some(node => isNonWhitespaceContent(node))) {
// implicit default slot (mixed with named slots)
// [ts] if (hasNamedDefaultSlot) {
// [ts] context.onError(createCompilerError(38 /* X_V_SLOT_EXTRANEOUS_DEFAULT_SLOT_CHILDREN */, implicitDefaultChildren[0].loc));
// [ts] }
// [ts] else {
// [ts] slotsProperties.push(buildDefaultSlotProperty(null, implicitDefaultChildren));
// [ts] }
// [ts] }
// [ts] }
// [ts] const slotFlag = if (hasDynamicSlots) 2 /* DYNAMIC */ else if (hasForwardedSlots(node.children)) 3 /* FORWARDED */ else 1 /* STABLE */;
// [ts] let slots = createObjectExpression(slotsProperties.concat(createObjectProperty(`_`,
// 2 = compiled but dynamic = can skip normalization, but must run diff
// 1 = compiled and static = can skip normalization AND diff as optimized
// [ts] createSimpleExpression(slotFlag + (` /* ${slotFlagsText[slotFlag]} */` ), false))), loc);
// [ts] if (dynamicSlots.length) {
// [ts] slots = createCallExpression(context.helper(CREATE_SLOTS), [
// [ts] slots,
// [ts] createArrayExpression(dynamicSlots)
// [ts] ]);
// [ts] }
// [ts] return {
// [ts] slots,
// [ts] hasDynamicSlots
// [ts] };
// [ts] }
// [ts] fn buildDynamicSlot(name: auto, fn: auto) {
// [ts] return createObjectExpression([
// [ts] createObjectProperty(`name`, name),
// [ts] createObjectProperty(`fn`, fn)
// [ts] ]);
// [ts] }
// [ts] fn hasForwardedSlots(children: auto) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// [ts] switch (child.type) {
// [ts] 1 /* ELEMENT */ => {
// [ts] if (child.tagType == 2 /* SLOT */ ||
// [ts] ((child.tagType == 0 /* ELEMENT */ ||
// [ts] child.tagType == 3 /* TEMPLATE */) &&
// [ts] hasForwardedSlots: (child.children) => {
// [ts] return true;
// [ts] }
// break;
// [ts] },
// [ts] 9 /* IF */ => {
// [ts] if (hasForwardedSlots(child.branches))
// [ts] return true;
// break;
// [ts] },
// [ts] 10 /* IF_BRANCH */, 11 /* FOR */ => {
// [ts] if (hasForwardedSlots(child.children))
// [ts] return true;
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn isNonWhitespaceContent(node: auto) {
// [ts] if (node.type != 2 /* TEXT */ && node.type != 12 /* TEXT_CALL */)
// [ts] return true;
// [ts] return if (node.type == 2 /* TEXT */) !!node.content.trim() else isNonWhitespaceContent(node.content);
// [ts] }
// some directive transforms (e.g. v-model) may return a symbol for runtime
// import, which should be used instead of a resolveDirective call.
// [ts] const directiveImportMap = WeakMap.init();
// generate a JavaScript AST for this element's codegen
// [ts] fn transformElement(node: auto, context: auto) {
// perform the work on exit, after all child expressions have been
// processed and merged.
// [ts] return () => {
// [ts] node = context.currentNode;
// [ts] if (!(node.type == 1 /* ELEMENT */ &&
// [ts] (node.tagType == 0 /* ELEMENT */ ||
// [ts] node.tagType == 1 /* COMPONENT */))) {
// [ts] return;
// [ts] }
// [ts] const { tag, props } = node;
// [ts] const isComponent = node.tagType == 1 /* COMPONENT */;
// The goal of the transform is to create a codegenNode implementing the
// VNodeCall interface.
// [ts] let vnodeTag = if (isComponent) resolveComponentType(node, context) else `"${tag}"`;
// [ts] const isDynamicComponent = isObject(vnodeTag) && vnodeTag.callee == RESOLVE_DYNAMIC_COMPONENT;
// [ts] let vnodeProps;
// [ts] let vnodeChildren;
// [ts] let vnodePatchFlag;
// [ts] let patchFlag = 0;
// [ts] let vnodeDynamicProps;
// [ts] let dynamicPropNames;
// [ts] let vnodeDirectives;
// [ts] let shouldUseBlock =
// dynamic component may resolve to plain elements
// [ts] isDynamicComponent ||
// [ts] vnodeTag == TELEPORT ||
// [ts] vnodeTag == SUSPENSE ||
// [ts] (!isComponent &&
// <svg> and <foreignObject> must be forced into blocks so that block
// updates inside get proper isSVG flag at runtime. (#639, #643)
// This is technically web-specific, but splitting the logic out of core
// leads to too much unnecessary complexity.
// [ts] (tag == "svg" ||
// [ts] tag == "foreignObject" ||
// #938: elements with dynamic keys should be forced into blocks
// [ts] findProp(node, "key", true)));
// props
// [ts] if (props.length > 0) {
// [ts] const propsBuildResult = buildProps(node, context);
// [ts] vnodeProps = propsBuildResult.props;
// [ts] patchFlag = propsBuildResult.patchFlag;
// [ts] dynamicPropNames = propsBuildResult.dynamicPropNames;
// [ts] const directives = propsBuildResult.directives;
// [ts] vnodeDirectives =
// [ts] if (directives && directives.length) createArrayExpression(directives.map(dir => buildDirectiveArgs(dir, context))) else null;
// [ts] }
// children
// [ts] if (node.children.length > 0) {
// [ts] if (vnodeTag == KEEP_ALIVE) {
// Although a built-in component, we compile KeepAlive with raw children
// instead of slot functions so that it can be used inside Transition
// or other Transition-wrapping HOCs.
// To ensure correct updates with block optimizations, we need to:
// 1. Force keep-alive into a block. This avoids its children being
//    collected by a parent block.
// [ts] shouldUseBlock = true;
// 2. Force keep-alive to always be updated, since it uses raw children.
// [ts] patchFlag |= 1024 /* DYNAMIC_SLOTS */;
// [ts] if (node.children.length > 1) {
// [ts] context.onError(createCompilerError(44 /* X_KEEP_ALIVE_INVALID_CHILDREN */, {
// [ts] start: node.children[0].loc.start,
// [ts] end: node.children[node.children.length - 1].loc.end,
// [ts] source: ""
// [ts] }));
// [ts] }
// [ts] }
// [ts] const shouldBuildAsSlots = isComponent &&
// Teleport is not a real component and has dedicated runtime handling
// [ts] vnodeTag != TELEPORT &&
// explained above.
// [ts] vnodeTag != KEEP_ALIVE;
// [ts] if (shouldBuildAsSlots) {
// [ts] const { slots, hasDynamicSlots } = buildSlots(node, context);
// [ts] vnodeChildren = slots;
// [ts] if (hasDynamicSlots) {
// [ts] patchFlag |= 1024 /* DYNAMIC_SLOTS */;
// [ts] }
// [ts] }
// [ts] else if (node.children.length == 1 && vnodeTag != TELEPORT) {
// [ts] const child = node.children[0];
// [ts] const type = child.type;
// check for dynamic text children
// [ts] const hasDynamicTextChild = type == 5 /* INTERPOLATION */ ||
// [ts] type == 8 /* COMPOUND_EXPRESSION */;
// [ts] if (hasDynamicTextChild &&
// [ts] getConstantType: (child, context) => {
// [ts] patchFlag |= 1 /* TEXT */;
// [ts] }
// pass directly if the only child is a text node
// (plain / interpolation / expression)
// [ts] if (hasDynamicTextChild || type == 2 /* TEXT */) {
// [ts] vnodeChildren = child;
// [ts] }
// [ts] else {
// [ts] vnodeChildren = node.children;
// [ts] }
// [ts] }
// [ts] else {
// [ts] vnodeChildren = node.children;
// [ts] }
// [ts] }
// patchFlag & dynamicPropNames
// [ts] if (patchFlag != 0) {
// [ts] {
// [ts] if (patchFlag < 0) {
// special flags (negative and mutually exclusive)
// [ts] vnodePatchFlag = patchFlag + ` /* ${PatchFlagNames[patchFlag]} */`;
// [ts] }
// [ts] else {
// bitwise flags
// [ts] const flagNames = Object.keys(PatchFlagNames)
// [ts] .map(Number)
// [ts] .filter(n => n > 0 && patchFlag & n)
// [ts] .map(n => PatchFlagNames[n])
// [ts] .join(`, `);
// [ts] vnodePatchFlag = patchFlag + ` /* ${flagNames} */`;
// [ts] }
// [ts] }
// [ts] if (dynamicPropNames && dynamicPropNames.length) {
// [ts] vnodeDynamicProps = stringifyDynamicPropNames(dynamicPropNames);
// [ts] }
// [ts] }
// [ts] node.codegenNode = createVNodeCall(context, vnodeTag, vnodeProps, vnodeChildren, vnodePatchFlag, vnodeDynamicProps, vnodeDirectives, !!shouldUseBlock, false /* disableTracking */, node.loc);
// [ts] };
// [ts] };
// [ts] fn resolveComponentType(node: auto, context: auto, ssr: auto = false) {
// [ts] let { tag } = node;
// 1. dynamic component
// [ts] const isExplicitDynamic = isComponentTag(tag);
// [ts] const isProp = findProp(node, "is");
// [ts] if (isProp) {
// [ts] if (isExplicitDynamic ||
// [ts] (false )) {
// [ts] const exp = if (isProp.type == 6 /* ATTRIBUTE */) isProp.value && createSimpleExpression(isProp.value.content, true) else isProp.exp;
// [ts] if (exp) {
// [ts] return createCallExpression(context.helper(RESOLVE_DYNAMIC_COMPONENT), [
// [ts] exp
// [ts] ]);
// [ts] }
// [ts] }
// [ts] else if (isProp.type == 6 /* ATTRIBUTE */ &&
// [ts] isProp.value.content.startsWith("vue:")) {
// <button is="vue:xxx">
// if not <component>, only is value that starts with "vue:" will be
// treated as component by the parse phase and reach here, unless it's
// compat mode where all is values are considered components
// [ts] tag = isProp.value.content.slice(4);
// [ts] }
// [ts] }
// 1.5 v-is (TODO: Deprecate)
// [ts] const isDir = !isExplicitDynamic && findDir(node, "is");
// [ts] if (isDir && isDir.exp) {
// [ts] return createCallExpression(context.helper(RESOLVE_DYNAMIC_COMPONENT), [
// [ts] isDir.exp
// [ts] ]);
// [ts] }
// 2. built-in components (Teleport, Transition, KeepAlive, Suspense...)
// [ts] const builtIn = isCoreComponent(tag) || context.isBuiltInComponent(tag);
// [ts] if (builtIn) {
// built-ins are simply fallthroughs / have special handling during ssr
// so we don't need to import their runtime equivalents
// [ts] if (!ssr)
// [ts] context.helper(builtIn);
// [ts] return builtIn;
// [ts] }
// 5. user component (resolve)
// [ts] context.helper(RESOLVE_COMPONENT);
// [ts] context.components.add(tag);
// [ts] return toValidAssetId(tag, `component`);
// [ts] }
// [ts] fn buildProps(node: auto, context: auto, props: auto = node.props, ssr: auto = false) {
// [ts] const { tag, loc: elementLoc } = node;
// [ts] const isComponent = node.tagType == 1 /* COMPONENT */;
// [ts] let properties = [];
// [ts] const mergeArgs = [];
// [ts] const runtimeDirectives = [];
// patchFlag analysis
// [ts] let patchFlag = 0;
// [ts] let hasRef = false;
// [ts] let hasClassBinding = false;
// [ts] let hasStyleBinding = false;
// [ts] let hasHydrationEventBinding = false;
// [ts] let hasDynamicKeys = false;
// [ts] let hasVnodeHook = false;
// [ts] const dynamicPropNames = [];
// [ts] const analyzePatchFlag = (_p) => {
// [ts] if (isStaticExp(key)) {
// [ts] const name = key.content;
// [ts] const isEventHandler = isOn(name);
// [ts] if (!isComponent &&
// [ts] isEventHandler &&
// omit the flag for click handlers because hydration gives click
// dedicated fast path.
// [ts] name.toLowerCase() != "onclick" &&
// omit v-model handlers
// [ts] name != "onUpdate:modelValue" &&
// omit onVnodeXXX hooks
// [ts] !isReservedProp(name)) {
// [ts] hasHydrationEventBinding = true;
// [ts] }
// [ts] if (isEventHandler && isReservedProp(name)) {
// [ts] hasVnodeHook = true;
// [ts] }
// [ts] if (value.type == 20 /* JS_CACHE_EXPRESSION */ ||
// [ts] ((value.type == 4 /* SIMPLE_EXPRESSION */ ||
// [ts] value.type == 8 /* COMPOUND_EXPRESSION */) &&
// [ts] getConstantType: (value, context) => {
// skip if the prop is a cached handler or has constant value
// [ts] return;
// [ts] }
// [ts] if (name == "ref") {
// [ts] hasRef = true;
// [ts] }
// [ts] else if (name == "class" && !isComponent) {
// [ts] hasClassBinding = true;
// [ts] }
// [ts] else if (name == "style" && !isComponent) {
// [ts] hasStyleBinding = true;
// [ts] }
// [ts] else if (name != "key" && !dynamicPropNames.includes(name)) {
// [ts] dynamicPropNames.push(name);
// [ts] }
// [ts] }
// [ts] else {
// [ts] hasDynamicKeys = true;
// [ts] }
// [ts] };
// [ts] let i = 0;
// [ts] while (i < props.length) { // [cap-migrate] for-step: i++
// static attribute
// [ts] const prop = props[i];
// [ts] if (prop.type == 6 /* ATTRIBUTE */) {
// [ts] const { loc, name, value } = prop;
// [ts] let isStatic = true;
// [ts] if (name == "ref") {
// [ts] hasRef = true;
// [ts] }
// skip is on <component>, or is="vue:xxx"
// [ts] if (name == "is" &&
// [ts] (isComponentTag(tag) ||
// [ts] (value && value.content.startsWith("vue:")) ||
// [ts] (false ))) {
// [ts] continue;
// [ts] }
// [ts] properties.push(createObjectProperty(createSimpleExpression(name, true, getInnerRange(loc, 0, name.length)), createSimpleExpression(if (value) value.content else if ("", isStatic, value) value.loc else loc)));
// [ts] }
// [ts] else {
// directives
// [ts] const { name, arg, exp, loc } = prop;
// [ts] const isVBind = name == "bind";
// [ts] const isVOn = name == "on";
// skip v-slot - it is handled by its dedicated transform.
// [ts] if (name == "slot") {
// [ts] if (!isComponent) {
// [ts] context.onError(createCompilerError(39 /* X_V_SLOT_MISPLACED */, loc));
// [ts] }
// [ts] continue;
// [ts] }
// skip v-once - it is handled by its dedicated transform.
// [ts] if (name == "once") {
// [ts] continue;
// [ts] }
// skip v-is and :is on <component>
// [ts] if (name == "is" ||
// [ts] (isVBind &&
// [ts] isBindKey(arg, "is") &&
// [ts] (isComponentTag(tag) ||
// [ts] (false )))) {
// [ts] continue;
// [ts] }
// skip v-on in SSR compilation
// [ts] if (isVOn && ssr) {
// [ts] continue;
// [ts] }
// special case for v-bind and v-on with no argument
// [ts] if (!arg && (isVBind || isVOn)) {
// [ts] hasDynamicKeys = true;
// [ts] if (exp) {
// [ts] if (properties.length) {
// [ts] mergeArgs.push(createObjectExpression(dedupeProperties(properties), elementLoc));
// [ts] properties = [];
// [ts] }
// [ts] if (isVBind) {
// [ts] mergeArgs.push(exp);
// [ts] }
// [ts] else {
// v-on="obj" -> toHandlers(obj)
// [ts] mergeArgs.push({
// [ts] type: 14 /* JS_CALL_EXPRESSION */,
// [ts] loc,
// [ts] callee: context.helper(TO_HANDLERS),
// [ts] arguments: [exp]
// [ts] });
// [ts] }
// [ts] }
// [ts] else {
// [ts] context.onError(createCompilerError(if (isVBind) 33 /* X_V_BIND_NO_EXPRESSION */ else 34 /* X_V_ON_NO_EXPRESSION */, loc));
// [ts] }
// [ts] continue;
// [ts] }
// [ts] const directiveTransform = context.directiveTransforms[name];
// [ts] if (directiveTransform) {
// has built-in directive transform.
// [ts] const { props, needRuntime } = directiveTransform(prop, node, context);
// [ts] !ssr && props.forEach(analyzePatchFlag);
// [ts] properties.push(...props);
// [ts] if (needRuntime) {
// [ts] runtimeDirectives.push(prop);
// [ts] if (isSymbol(needRuntime)) {
// [ts] directiveImportMap.set(prop, needRuntime);
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// no built-in transform, this is a user custom directive.
// [ts] runtimeDirectives.push(prop);
// [ts] }
// [ts] }
// [ts] }
// [ts] let propsExpression = null;
// has v-bind="object" or v-on="object", wrap with mergeProps
// [ts] if (mergeArgs.length) {
// [ts] if (properties.length) {
// [ts] mergeArgs.push(createObjectExpression(dedupeProperties(properties), elementLoc));
// [ts] }
// [ts] if (mergeArgs.length > 1) {
// [ts] propsExpression = createCallExpression(context.helper(MERGE_PROPS), mergeArgs, elementLoc);
// [ts] }
// [ts] else {
// single v-bind with nothing else - no need for a mergeProps call
// [ts] propsExpression = mergeArgs[0];
// [ts] }
// [ts] }
// [ts] else if (properties.length) {
// [ts] propsExpression = createObjectExpression(dedupeProperties(properties), elementLoc);
// [ts] }
// patchFlag analysis
// [ts] if (hasDynamicKeys) {
// [ts] patchFlag |= 16 /* FULL_PROPS */;
// [ts] }
// [ts] else {
// [ts] if (hasClassBinding) {
// [ts] patchFlag |= 2 /* CLASS */;
// [ts] }
// [ts] if (hasStyleBinding) {
// [ts] patchFlag |= 4 /* STYLE */;
// [ts] }
// [ts] if (dynamicPropNames.length) {
// [ts] patchFlag |= 8 /* PROPS */;
// [ts] }
// [ts] if (hasHydrationEventBinding) {
// [ts] patchFlag |= 32 /* HYDRATE_EVENTS */;
// [ts] }
// [ts] }
// [ts] if ((patchFlag == 0 || patchFlag == 32 /* HYDRATE_EVENTS */) &&
// [ts] (hasRef || hasVnodeHook || runtimeDirectives.length > 0)) {
// [ts] patchFlag |= 512 /* NEED_PATCH */;
// [ts] }
// [ts] return {
// [ts] props: propsExpression,
// [ts] directives: runtimeDirectives,
// [ts] patchFlag,
// [ts] dynamicPropNames
// [ts] };
// [ts] }
// Dedupe props in an object literal.
// Literal duplicated attributes would have been warned during the parse phase,
// however, it's possible to encounter duplicated `onXXX` handlers with different
// modifiers. We also need to merge static and dynamic class / style attributes.
// - onXXX handlers / style: merge into array
// - class: merge into single expression with concatenation
// [ts] fn dedupeProperties(properties: auto) {
// [ts] const knownProps = Map.init();
// [ts] const deduped = [];
// [ts] let i = 0;
// [ts] while (i < properties.length) { // [cap-migrate] for-step: i++
// [ts] const prop = properties[i];
// dynamic keys are always allowed
// [ts] if (prop.key.type == 8 /* COMPOUND_EXPRESSION */ || !prop.key.isStatic) {
// [ts] deduped.push(prop);
// [ts] continue;
// [ts] }
// [ts] const name = prop.key.content;
// [ts] const existing = knownProps.get(name);
// [ts] if (existing) {
// [ts] if (name == "style" || name == "class" || name.startsWith("on")) {
// [ts] mergeAsArray$1(existing, prop);
// [ts] }
// unexpected duplicate, should have emitted error during parse
// [ts] }
// [ts] else {
// [ts] knownProps.set(name, prop);
// [ts] deduped.push(prop);
// [ts] }
// [ts] }
// [ts] return deduped;
// [ts] }
// [ts] (existing, incoming) => {
// [ts] if (existing.value.type == 17 /* JS_ARRAY_EXPRESSION */) {
// [ts] existing.value.elements.push(incoming.value);
// [ts] }
// [ts] else {
// [ts] existing.value = createArrayExpression([existing.value, incoming.value], existing.loc);
// [ts] }
// [ts] }
// [ts] fn buildDirectiveArgs(dir: auto, context: auto) {
// [ts] const dirArgs = [];
// [ts] const runtime = directiveImportMap.get(dir);
// [ts] if (runtime) {
// built-in directive with runtime
// [ts] dirArgs.push(context.helperString(runtime));
// [ts] }
// [ts] else {
// [ts] {
// inject statement for resolving directive
// [ts] context.helper(RESOLVE_DIRECTIVE);
// [ts] context.directives.add(dir.name);
// [ts] dirArgs.push(toValidAssetId(dir.name, `directive`));
// [ts] }
// [ts] }
// [ts] const { loc } = dir;
// [ts] if (dir.exp)
// [ts] dirArgs.push(dir.exp);
// [ts] if (dir.arg) {
// [ts] if (!dir.exp) {
// [ts] dirArgs.push(`void 0`);
// [ts] }
// [ts] dirArgs.push(dir.arg);
// [ts] }
// [ts] if (Object.keys(dir.modifiers).length) {
// [ts] if (!dir.arg) {
// [ts] if (!dir.exp) {
// [ts] dirArgs.push(`void 0`);
// [ts] }
// [ts] dirArgs.push(`void 0`);
// [ts] }
// [ts] const trueExpression = createSimpleExpression(`true`, false, loc);
// [ts] dirArgs.push(createObjectExpression(dir.modifiers.map(modifier => createObjectProperty(modifier, trueExpression)), loc));
// [ts] }
// [ts] return createArrayExpression(dirArgs, dir.loc);
// [ts] }
// [ts] fn stringifyDynamicPropNames(props: auto) {
// [ts] let propsNamesString = `[`;
// [ts] let i = 0, l = props.length;
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] propsNamesString += JSON.stringify(props[i]);
// [ts] if (i < l - 1)
// [ts] propsNamesString += ", ";
// [ts] }
// [ts] return propsNamesString + `]`;
// [ts] }
// [ts] fn isComponentTag(tag: auto) {
// [ts] return tag[0].toLowerCase() + tag.slice(1) == "component";
// [ts] }
// [ts] fn transformSlotOutlet(node: auto, context: auto) {
// [ts] if (isSlotOutlet(node)) {
// [ts] const { children, loc } = node;
// [ts] const { slotName, slotProps } = processSlotOutlet(node, context);
// [ts] const slotArgs = [
// [ts] if (context.prefixIdentifiers) `_ctx.$slots` else `$slots`,
// [ts] slotName
// [ts] ];
// [ts] if (slotProps) {
// [ts] slotArgs.push(slotProps);
// [ts] }
// [ts] if (children.length) {
// [ts] if (!slotProps) {
// [ts] slotArgs.push(`{}`);
// [ts] }
// [ts] slotArgs.push(createFunctionExpression([], children, false, false, loc));
// [ts] }
// [ts] if (context.scopeId && !context.slotted) {
// [ts] if (!slotProps) {
// [ts] slotArgs.push(`{}`);
// [ts] }
// [ts] if (!children.length) {
// [ts] slotArgs.push(`undefined`);
// [ts] }
// [ts] slotArgs.push(`true`);
// [ts] }
// [ts] node.codegenNode = createCallExpression(context.helper(RENDER_SLOT), slotArgs, loc);
// [ts] }
// [ts] };
// [ts] fn processSlotOutlet(node: auto, context: auto) {
// [ts] let slotName = `"default"`;
// [ts] let slotProps = null;
// [ts] const nonNameProps = [];
// [ts] let i = 0;
// [ts] while (i < node.props.length) { // [cap-migrate] for-step: i++
// [ts] const p = node.props[i];
// [ts] if (p.type == 6 /* ATTRIBUTE */) {
// [ts] if (p.value) {
// [ts] if (p.name == "name") {
// [ts] slotName = JSON.stringify(p.value.content);
// [ts] }
// [ts] else {
// [ts] p.name = camelize(p.name);
// [ts] nonNameProps.push(p);
// [ts] }
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (p.name == "bind" && isBindKey(p.arg, "name")) {
// [ts] if (p.exp)
// [ts] slotName = p.exp;
// [ts] }
// [ts] else {
// [ts] if (p.name == "bind" && p.arg && isStaticExp(p.arg)) {
// [ts] p.arg.content = camelize(p.arg.content);
// [ts] }
// [ts] nonNameProps.push(p);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (nonNameProps.length > 0) {
// [ts] const { props, directives } = buildProps(node, context, nonNameProps);
// [ts] slotProps = props;
// [ts] if (directives.length) {
// [ts] context.onError(createCompilerError(35 /* X_V_SLOT_UNEXPECTED_DIRECTIVE_ON_SLOT_OUTLET */, directives[0].loc));
// [ts] }
// [ts] }
// [ts] return {
// [ts] slotName,
// [ts] slotProps
// [ts] };
// [ts] }
// [ts] const fnExpRE = "^\\s*([\\w$_]+|\\([^)]*?\\))\\s*=>|^\\s*function(?:\\s+[\\w$]+)?\\s*\\(";
// [ts] fn transformOn(dir: auto, node: auto, context: auto, augmentor: auto) {
// [ts] const { loc, modifiers, arg } = dir;
// [ts] if (!dir.exp && !modifiers.length) {
// [ts] context.onError(createCompilerError(34 /* X_V_ON_NO_EXPRESSION */, loc));
// [ts] }
// [ts] let eventName;
// [ts] if (arg.type == 4 /* SIMPLE_EXPRESSION */) {
// [ts] if (arg.isStatic) {
// [ts] const rawName = arg.content;
// for all event listeners, auto convert it to camelCase. See issue #2249
// [ts] eventName = createSimpleExpression(toHandlerKey(camelize(rawName)), true, arg.loc);
// [ts] }
// [ts] else {
// #2388
// [ts] eventName = createCompoundExpression([
// [ts] `${context.helperString(TO_HANDLER_KEY)}(`,
// [ts] arg,
// [ts] `)`
// [ts] ]);
// [ts] }
// [ts] }
// [ts] else {
// already a compound expression.
// [ts] eventName = arg;
// [ts] eventName.children.unshift(`${context.helperString(TO_HANDLER_KEY)}(`);
// [ts] eventName.children.push(`)`);
// [ts] }
// handler processing
// [ts] let exp = dir.exp;
// [ts] if (exp && !exp.content.trim()) {
// [ts] exp = null;
// [ts] }
// [ts] let shouldCache = context.cacheHandlers && !exp;
// [ts] if (exp) {
// [ts] const isMemberExp = isMemberExpression(exp.content);
// [ts] const isInlineStatement = !(isMemberExp || fnExpRE.test(exp.content));
// [ts] const hasMultipleStatements = exp.content.includes(`;`);
// [ts] {
// [ts] validateBrowserExpression(exp, context, false, hasMultipleStatements);
// [ts] }
// [ts] if (isInlineStatement || (shouldCache && isMemberExp)) {
// wrap inline statement in a function expression
// [ts] exp = createCompoundExpression([
// [ts] `${if (isInlineStatement) `$event` else `${``}(...args)`} => ${if (hasMultipleStatements) `{` else `(`}`,
// [ts] exp,
// [ts] if (hasMultipleStatements) `}` else `)`
// [ts] ]);
// [ts] }
// [ts] }
// [ts] let ret = {
// [ts] props: [
// [ts] createObjectProperty(eventName, exp || createSimpleExpression(`() => {}`, false, loc))
// [ts] ]
// [ts] };
// apply extended compiler augmentor
// [ts] if (augmentor) {
// [ts] ret = augmentor(ret);
// [ts] }
// [ts] if (shouldCache) {
// cache handlers so that it's always the same handler being passed down.
// this avoids unnecessary re-renders when users use inline handlers on
// components.
// [ts] ret.props[0].value = context.cache(ret.props[0].value);
// [ts] }
// [ts] return ret;
// [ts] };
// v-bind without arg is handled directly in ./transformElements.ts due to it affecting
// codegen for the entire props object. This transform here is only for v-bind
// *with* args.
// [ts] const transformBind = (dir, _node, context) => {
// [ts] const { exp, modifiers, loc } = dir;
// [ts] const arg = dir.arg;
// [ts] if (arg.type != 4 /* SIMPLE_EXPRESSION */) {
// [ts] arg.children.unshift(`(`);
// [ts] arg.children.push(`) || ""`);
// [ts] }
// [ts] else if (!arg.isStatic) {
// [ts] arg.content = `${arg.content} || ""`;
// [ts] }
// .prop is no longer necessary due to new patch behavior
// .sync is replaced by v-model:arg
// [ts] if (modifiers.includes("camel")) {
// [ts] if (arg.type == 4 /* SIMPLE_EXPRESSION */) {
// [ts] if (arg.isStatic) {
// [ts] arg.content = camelize(arg.content);
// [ts] }
// [ts] else {
// [ts] arg.content = `${context.helperString(CAMELIZE)}(${arg.content})`;
// [ts] }
// [ts] }
// [ts] else {
// [ts] arg.children.unshift(`${context.helperString(CAMELIZE)}(`);
// [ts] arg.children.push(`)`);
// [ts] }
// [ts] }
// [ts] if (!exp ||
// [ts] (exp.type == 4 /* SIMPLE_EXPRESSION */ && !exp.content.trim())) {
// [ts] context.onError(createCompilerError(33 /* X_V_BIND_NO_EXPRESSION */, loc));
// [ts] return {
// [ts] props: [createObjectProperty(arg, createSimpleExpression("", true, loc))]
// [ts] };
// [ts] }
// [ts] return {
// [ts] props: [createObjectProperty(arg, exp)]
// [ts] };
// [ts] };
// Merge adjacent text nodes and expressions into a single expression
// e.g. <div>abc {{ d }} {{ e }}</div> should have a single expression node as child.
// [ts] const transformText = (node, context) => {
// [ts] if (node.type == 0 /* ROOT */ ||
// [ts] node.type == 1 /* ELEMENT */ ||
// [ts] node.type == 11 /* FOR */ ||
// [ts] node.type == 10 /* IF_BRANCH */) {
// perform the transform on node exit so that all expressions have already
// been processed.
// [ts] return () => {
// [ts] const children = node.children;
// [ts] let currentContainer = null;
// [ts] let hasText = false;
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// [ts] if (isText(child)) {
// [ts] hasText = true;
// [ts] let j = i + 1;
// [ts] while (j < children.length) { // [cap-migrate] for-step: j++
// [ts] const next = children[j];
// [ts] if (isText(next)) {
// [ts] if (!currentContainer) {
// [ts] currentContainer = children[i] = {
// [ts] type: 8 /* COMPOUND_EXPRESSION */,
// [ts] loc: child.loc,
// [ts] children: [child]
// [ts] };
// [ts] }
// merge adjacent text node into current
// [ts] currentContainer.children.push(` + `, next);
// [ts] children.splice(j, 1);
// [ts] j--;
// [ts] }
// [ts] else {
// [ts] currentContainer = null;
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!hasText ||
// if this is a plain element with a single text child, leave it
// as-is since the runtime has dedicated fast path for this by directly
// setting textContent of the element.
// for component root it's always normalized anyway.
// [ts] (children.length == 1 &&
// [ts] (node.type == 0 /* ROOT */ ||
// [ts] (node.type == 1 /* ELEMENT */ &&
// [ts] node.tagType == 0 /* ELEMENT */ &&
// #3756
// custom directives can potentially add DOM elements arbitrarily,
// we need to avoid setting textContent of the element at runtime
// to avoid accidentally overwriting the DOM elements added
// by the user through custom directives.
// [ts] !node.props.find(p => p.type == 7 /* DIRECTIVE */ &&
// [ts] !context.directiveTransforms[p.name]) &&
// in compat mode, <template> tags with no special directives
// will be rendered as a fragment so its children must be
// converted into vnodes.
// [ts] !(false ))))) {
// [ts] return;
// [ts] }
// pre-convert text nodes into createTextVNode(text) calls to avoid
// runtime normalization.
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] const child = children[i];
// [ts] if (isText(child) || child.type == 8 /* COMPOUND_EXPRESSION */) {
// [ts] const callArgs = [];
// createTextVNode defaults to single whitespace, so if it is a
// single space the code could be an empty call to save bytes.
// [ts] if (child.type != 2 /* TEXT */ || child.content != " ") {
// [ts] callArgs.push(child);
// [ts] }
// mark dynamic text with flag so it gets patched inside a block
// [ts] if (!context.ssr &&
// [ts] getConstantType: (child, context) => {
// [ts] callArgs.push(1 /* TEXT */ +
// [ts] (` /* ${PatchFlagNames[1 /* TEXT */]} */` ));
// [ts] }
// [ts] children[i] = {
// [ts] type: 12 /* TEXT_CALL */,
// [ts] content: child,
// [ts] loc: child.loc,
// [ts] codegenNode: createCallExpression(context.helper(CREATE_TEXT), callArgs)
// [ts] };
// [ts] }
// [ts] }
// [ts] };
// [ts] }
// [ts] };
// [ts] const seen = WeakSet.init();
// [ts] const transformOnce = (node, context) => {
// [ts] if (node.type == 1 /* ELEMENT */ && findDir(node, "once", true)) {
// [ts] if (seen.has(node)) {
// [ts] return;
// [ts] }
// [ts] seen.add(node);
// [ts] context.helper(SET_BLOCK_TRACKING);
// [ts] return () => {
// [ts] const cur = context.currentNode;
// [ts] if (cur.codegenNode) {
// [ts] cur.codegenNode = context.cache(cur.codegenNode, true /* isVNode */);
// [ts] }
// [ts] };
// [ts] }
// [ts] };
// [ts] const transformModel = (dir, node, context) => {
// [ts] const { exp, arg } = dir;
// [ts] if (!exp) {
// [ts] context.onError(createCompilerError(40 /* X_V_MODEL_NO_EXPRESSION */, dir.loc));
// [ts] return createTransformProps();
// [ts] }
// [ts] const rawExp = exp.loc.source;
// [ts] const expString = if (exp.type == 4 /* SIMPLE_EXPRESSION */) exp.content else rawExp;
// im SFC <script setup> inline mode, the exp may have been transformed into
// _unref(exp)
// [ts] context.bindingMetadata[rawExp];
// [ts] const maybeRef = !true    /* SETUP_CONST */;
// [ts] if (!expString.trim() || (!isMemberExpression(expString) && !maybeRef)) {
// [ts] context.onError(createCompilerError(41 /* X_V_MODEL_MALFORMED_EXPRESSION */, exp.loc));
// [ts] return createTransformProps();
// [ts] }
// [ts] const propName = if (arg) arg else createSimpleExpression("modelValue", true);
// [ts] const eventName = if (arg) if (isStaticExp(arg)) `onUpdate:${arg.content}` else createCompoundExpression(["\"onUpdate:\" + ", arg]) else `onUpdate:modelValue`;
// [ts] let assignmentExp;
// [ts] const eventArg = if (context.isTS) `($event: any)` else `$event`;
// [ts] {
// [ts] assignmentExp = createCompoundExpression([
// [ts] `${eventArg} => (`,
// [ts] exp,
// [ts] ` = $event)`
// [ts] ]);
// [ts] }
// [ts] const props = [
// modelValue: foo
// [ts] createObjectProperty(propName, dir.exp),
// "onUpdate:modelValue": $event => (foo = $event)
// [ts] createObjectProperty(eventName, assignmentExp)
// [ts] ];
// modelModifiers: { foo: true, "bar-baz": true }
// [ts] if (dir.modifiers.length && node.tagType == 1 /* COMPONENT */) {
// [ts] const modifiers = dir.modifiers
// [ts] .map(m => (if (isSimpleIdentifier(m)) m else JSON.stringify(m)) + `: true`)
// [ts] .join(`, `);
// [ts] const modifiersKey = if (arg) if (isStaticExp(arg)) `${arg.content}Modifiers` else createCompoundExpression([arg, " + \"Modifiers\""]) else `modelModifiers`;
// [ts] props.push(createObjectProperty(modifiersKey, createSimpleExpression(`{ ${modifiers} }`, false, dir.loc, 2 /* CAN_HOIST */)));
// [ts] }
// [ts] return createTransformProps(props);
// [ts] };
// [ts] fn createTransformProps(props: auto = []) {
// [ts] return { props };
// [ts] }
// [ts] fn getBaseTransformPreset(prefixIdentifiers: auto) {
// [ts] return [
// [ts] [
// [ts] transformOnce,
// [ts] transformIf,
// [ts] transformFor,
// [ts] ...([]),
// [ts] ...([transformExpression]
// [ts] ),
// [ts] transformSlotOutlet,
// [ts] transformElement,
// [ts] trackSlotScopes,
// [ts] transformText
// [ts] ],
// [ts] {
// [ts] on: transformOn,
// [ts] bind: transformBind,
// [ts] model: transformModel
// [ts] }
// [ts] ];
// [ts] }
// we name it `baseCompile` so that higher order compilers like
// @vue/compiler-dom can export `compile` while re-exporting everything else.
// [ts] fn baseCompile(template: auto, options: auto = {}) {
// [ts] const onError = options.onError || defaultOnError;
// [ts] const isModuleMode = options.mode == "module";
// [ts] /* istanbul ignore if */
// [ts] {
// [ts] if (options.prefixIdentifiers == true) {
// [ts] onError(createCompilerError(45 /* X_PREFIX_ID_NOT_SUPPORTED */));
// [ts] }
// [ts] else if (isModuleMode) {
// [ts] onError(createCompilerError(46 /* X_MODULE_MODE_NOT_SUPPORTED */));
// [ts] }
// [ts] }
// [ts] const prefixIdentifiers = !true ;
// [ts] if (options.cacheHandlers) {
// [ts] onError(createCompilerError(47 /* X_CACHE_HANDLER_NOT_SUPPORTED */));
// [ts] }
// [ts] if (options.scopeId && !isModuleMode) {
// [ts] onError(createCompilerError(48 /* X_SCOPE_ID_NOT_SUPPORTED */));
// [ts] }
// [ts] const ast = if (isString(template)) baseParse(template, options) else template;
// [ts] const [nodeTransforms, directiveTransforms] = getBaseTransformPreset();
// [ts] transform(ast, extend({}, options, {
// [ts] prefixIdentifiers,
// [ts] nodeTransforms: [
// [ts] ...nodeTransforms,
// [ts] ...(options.nodeTransforms || []) // user transforms
// [ts] ],
// [ts] directiveTransforms: extend({}, directiveTransforms, options.directiveTransforms || {} // user transforms
// [ts] )
// [ts] }));
// [ts] return generate(ast, extend({}, options, {
// [ts] prefixIdentifiers
// [ts] }));
// [ts] }
// [ts] const noopDirectiveTransform = () => ({ props: [] });
// [ts] const V_MODEL_RADIO = "__symbol";
// [ts] const V_MODEL_CHECKBOX = "__symbol";
// [ts] const V_MODEL_TEXT = "__symbol";
// [ts] const V_MODEL_SELECT = "__symbol";
// [ts] const V_MODEL_DYNAMIC = "__symbol";
// [ts] const V_ON_WITH_MODIFIERS = "__symbol";
// [ts] const V_ON_WITH_KEYS = "__symbol";
// [ts] const V_SHOW = "__symbol";
// [ts] const TRANSITION$1 = "__symbol";
// [ts] const TRANSITION_GROUP = "__symbol";
// [ts] registerRuntimeHelpers({
// [ts] [V_MODEL_RADIO]: `vModelRadio`,
// [ts] [V_MODEL_CHECKBOX]: `vModelCheckbox`,
// [ts] [V_MODEL_TEXT]: `vModelText`,
// [ts] [V_MODEL_SELECT]: `vModelSelect`,
// [ts] [V_MODEL_DYNAMIC]: `vModelDynamic`,
// [ts] [V_ON_WITH_MODIFIERS]: `withModifiers`,
// [ts] [V_ON_WITH_KEYS]: `withKeys`,
// [ts] [V_SHOW]: `vShow`,
// [ts] [TRANSITION$1]: `Transition`,
// [ts] [TRANSITION_GROUP]: `TransitionGroup`
// [ts] });
// [ts] /* eslint-disable no-restricted-globals */
// [ts] let decoder;
// [ts] fn decodeHtmlBrowser(raw: auto, asAttr: auto = false) {
// [ts] if (!decoder) {
// [ts] decoder = document.createElement("div");
// [ts] }
// [ts] if (asAttr) {
// [ts] decoder.innerHTML = `<div foo="${raw.replace(/"/g, '&quot;')}">`;
// [ts] return decoder.children[0].getAttribute("foo");
// [ts] }
// [ts] else {
// [ts] decoder.innerHTML = raw;
// [ts] return decoder.textContent;
// [ts] }
// [ts] }
// [ts] const isRawTextContainer = /*_priv___PURE__*/ makeMap("style,iframe,script,noscript", true);
// [ts] const parserOptions = {
// [ts] isVoidTag,
// [ts] isNativeTag: tag => isHTMLTag(tag) || isSVGTag(tag),
// [ts] isPreTag: tag => tag == "pre",
// [ts] decodeEntities: decodeHtmlBrowser ,
// [ts] isBuiltInComponent: (tag) => {
// [ts] if (isBuiltInType(tag, `Transition`)) {
// [ts] return TRANSITION$1;
// [ts] }
// [ts] else if (isBuiltInType(tag, `TransitionGroup`)) {
// [ts] return TRANSITION_GROUP;
// [ts] }
// [ts] },
// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction-dispatcher
// [ts] getNamespace: (tag, parent) => {
// [ts] let ns = if (parent) parent.ns else 0 /* HTML */;
// [ts] if (parent && ns == 2 /* MATH_ML */) {
// [ts] if (parent.tag == "annotation-xml") {
// [ts] if (tag == "svg") {
// [ts] return 1 /* SVG */;
// [ts] }
// [ts] if (parent.props.some(a => a.type == 6 /* ATTRIBUTE */ &&
// [ts] a.name == "encoding" &&
// [ts] a.value != null &&
// [ts] (a.value.content == "text/html" ||
// [ts] a.value.content == "application/xhtml+xml"))) {
// [ts] ns = 0 /* HTML */;
// [ts] }
// [ts] }
// [ts] else if ("^m(?:[ions]|text)$".test(parent.tag) &&
// [ts] tag != "mglyph" &&
// [ts] tag != "malignmark") {
// [ts] ns = 0 /* HTML */;
// [ts] }
// [ts] }
// [ts] else if (parent && ns == 1 /* SVG */) {
// [ts] if (parent.tag == "foreignObject" ||
// [ts] parent.tag == "desc" ||
// [ts] parent.tag == "title") {
// [ts] ns = 0 /* HTML */;
// [ts] }
// [ts] }
// [ts] if (ns == 0 /* HTML */) {
// [ts] if (tag == "svg") {
// [ts] return 1 /* SVG */;
// [ts] }
// [ts] if (tag == "math") {
// [ts] return 2 /* MATH_ML */;
// [ts] }
// [ts] }
// [ts] return ns;
// [ts] },
// https://html.spec.whatwg.org/multipage/parsing.html#parsing-html-fragments
// [ts] getTextMode: (_p) => {
// [ts] if (ns == 0 /* HTML */) {
// [ts] if (tag == "textarea" || tag == "title") {
// [ts] return 1 /* RCDATA */;
// [ts] }
// [ts] if (isRawTextContainer(tag)) {
// [ts] return 2 /* RAWTEXT */;
// [ts] }
// [ts] }
// [ts] return 0 /* DATA */;
// [ts] }
// [ts] };
// Parse inline CSS strings for static style attributes into an object.
// This is a NodeTransform since it works on the static `style` attribute and
// converts it into a dynamic equivalent:
// style="color: red" -> :style='{ "color": "red" }'
// It is then processed by `transformElement` and included in the generated
// props.
// [ts] const transformStyle = node => {
// [ts] if (node.type == 1 /* ELEMENT */) {
// [ts] node.props.forEach((p, i) => {
// [ts] if (p.type == 6 /* ATTRIBUTE */ && p.name == "style" && p.value) {
// replace p with an expression node
// [ts] node.props[i] = {
// [ts] type: 7 /* DIRECTIVE */,
// [ts] name: `bind`,
// [ts] arg: createSimpleExpression(`style`, true, p.loc),
// [ts] exp: parseInlineCSS(p.value.content, p.loc),
// [ts] modifiers: [],
// [ts] loc: p.loc
// [ts] };
// [ts] }
// [ts] });
// [ts] }
// [ts] };
// [ts] const parseInlineCSS = (cssText, loc) => {
// [ts] const normalized = parseStringStyle(cssText);
// [ts] return createSimpleExpression(JSON.stringify(normalized), false, loc, 3 /* CAN_STRINGIFY */);
// [ts] };
// [ts] fn createDOMCompilerError(code: auto, loc: auto) {
// [ts] return createCompilerError(code, loc, DOMErrorMessages );
// [ts] }
// [ts] const DOMErrorMessages = {
// [ts] [49 /* X_V_HTML_NO_EXPRESSION */]: `v-html is missing expression.`,
// [ts] [50 /* X_V_HTML_WITH_CHILDREN */]: `v-html will override element children.`,
// [ts] [51 /* X_V_TEXT_NO_EXPRESSION */]: `v-text is missing expression.`,
// [ts] [52 /* X_V_TEXT_WITH_CHILDREN */]: `v-text will override element children.`,
// [ts] [53 /* X_V_MODEL_ON_INVALID_ELEMENT */]: `v-model can only be used on <input>, <textarea> and <select> elements.`,
// [ts] [54 /* X_V_MODEL_ARG_ON_ELEMENT */]: `v-model argument is not supported on plain elements.`,
// [ts] [55 /* X_V_MODEL_ON_FILE_INPUT_ELEMENT */]: `v-model cannot be used on file inputs since they are read-only. Use a v-on:change listener instead.`,
// [ts] [56 /* X_V_MODEL_UNNECESSARY_VALUE */]: `Unnecessary value binding used alongside v-model. It will interfere with v-model's behavior.`,
// [ts] [57 /* X_V_SHOW_NO_EXPRESSION */]: `v-show is missing expression.`,
// [ts] [58 /* X_TRANSITION_INVALID_CHILDREN */]: `<Transition> expects exactly one child element or component.`,
// [ts] [59 /* X_IGNORED_SIDE_EFFECT_TAG */]: `Tags with side effect (<script> and <style>) are ignored in client component templates.`
// [ts] };
// [ts] const transformVHtml = (dir, node, context) => {
// [ts] const { exp, loc } = dir;
// [ts] if (!exp) {
// [ts] context.onError(createDOMCompilerError(49 /* X_V_HTML_NO_EXPRESSION */, loc));
// [ts] }
// [ts] if (node.children.length) {
// [ts] context.onError(createDOMCompilerError(50 /* X_V_HTML_WITH_CHILDREN */, loc));
// [ts] node.children.length = 0;
// [ts] }
// [ts] return {
// [ts] props: [
// [ts] createObjectProperty(createSimpleExpression(`innerHTML`, true, loc), exp || createSimpleExpression("", true))
// [ts] ]
// [ts] };
// [ts] };
// [ts] const transformVText = (dir, node, context) => {
// [ts] const { exp, loc } = dir;
// [ts] if (!exp) {
// [ts] context.onError(createDOMCompilerError(51 /* X_V_TEXT_NO_EXPRESSION */, loc));
// [ts] }
// [ts] if (node.children.length) {
// [ts] context.onError(createDOMCompilerError(52 /* X_V_TEXT_WITH_CHILDREN */, loc));
// [ts] node.children.length = 0;
// [ts] }
// [ts] return {
// [ts] props: [
// [ts] createObjectProperty(if (createSimpleExpression(`textContent`, true), exp) createCallExpression(context.helperString(TO_DISPLAY_STRING), [exp], loc) else createSimpleExpression("", true))
// [ts] ]
// [ts] };
// [ts] };
// [ts] const transformModel$1 = (dir, node, context) => {
// [ts] const baseResult = transformModel(dir, node, context);
// base transform has errors OR component v-model (only need props)
// [ts] if (!baseResult.props.length || node.tagType == 1 /* COMPONENT */) {
// [ts] return baseResult;
// [ts] }
// [ts] if (dir.arg) {
// [ts] context.onError(createDOMCompilerError(54 /* X_V_MODEL_ARG_ON_ELEMENT */, dir.arg.loc));
// [ts] }
// [ts] fn checkDuplicatedValue() {
// [ts] const value = findProp(node, "value");
// [ts] if (value) {
// [ts] context.onError(createDOMCompilerError(56 /* X_V_MODEL_UNNECESSARY_VALUE */, value.loc));
// [ts] }
// [ts] }
// [ts] const { tag } = node;
// [ts] const isCustomElement = context.isCustomElement(tag);
// [ts] if (tag == "input" ||
// [ts] tag == "textarea" ||
// [ts] tag == "select" ||
// [ts] isCustomElement) {
// [ts] let directiveToUse = V_MODEL_TEXT;
// [ts] let isInvalidType = false;
// [ts] if (tag == "input" || isCustomElement) {
// [ts] const type = findProp(node, `type`);
// [ts] if (type) {
// [ts] if (type.type == 7 /* DIRECTIVE */) {
// :type="foo"
// [ts] directiveToUse = V_MODEL_DYNAMIC;
// [ts] }
// [ts] else if (type.value) {
// [ts] switch (type.value.content) {
// [ts] "radio" => {
// [ts] directiveToUse = V_MODEL_RADIO;
// break;
// [ts] },
// [ts] "checkbox" => {
// [ts] directiveToUse = V_MODEL_CHECKBOX;
// break;
// [ts] },
// [ts] "file" => {
// [ts] isInvalidType = true;
// [ts] context.onError(createDOMCompilerError(55 /* X_V_MODEL_ON_FILE_INPUT_ELEMENT */, dir.loc));
// break;
// [ts] },
// [ts] else => {
// text type
// [ts] checkDuplicatedValue();
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] else if (hasDynamicKeyVBind(node)) {
// element has bindings with dynamic keys, which can possibly contain
// "type".
// [ts] directiveToUse = V_MODEL_DYNAMIC;
// [ts] }
// [ts] else {
// text type
// [ts] checkDuplicatedValue();
// [ts] }
// [ts] }
// [ts] else if (tag == "select") {
// [ts] directiveToUse = V_MODEL_SELECT;
// [ts] }
// [ts] else {
// textarea
// [ts] checkDuplicatedValue();
// [ts] }
// inject runtime directive
// by returning the helper symbol via needRuntime
// the import will replaced a resolveDirective call.
// [ts] if (!isInvalidType) {
// [ts] baseResult.needRuntime = context.helper(directiveToUse);
// [ts] }
// [ts] }
// [ts] else {
// [ts] context.onError(createDOMCompilerError(53 /* X_V_MODEL_ON_INVALID_ELEMENT */, dir.loc));
// [ts] }
// native vmodel doesn't need the `modelValue` props since they are also
// passed to the runtime as `binding.value`. removing it reduces code size.
// [ts] baseResult.props = baseResult.props.filter(p => !(p.key.type == 4 /* SIMPLE_EXPRESSION */ &&
// [ts] p.key.content == "modelValue"));
// [ts] return baseResult;
// [ts] };
// [ts] const isEventOptionModifier = /*_priv___PURE__*/ makeMap(`passive,once,capture`);
// [ts] const isNonKeyModifier = /*_priv___PURE__*/ makeMap(
// event propagation management
// [ts] `stop,prevent,self,`   +
// system modifiers + exact
// [ts] `ctrl,shift,alt,meta,exact,` +
// mouse
// [ts] `middle`);
// left & right could be mouse or key modifiers based on event type
// [ts] const maybeKeyModifier = /*_priv___PURE__*/ makeMap("left,right");
// [ts] const isKeyboardEvent = /*_priv___PURE__*/ makeMap(`onkeyup,onkeydown,onkeypress`, true);
// [ts] const resolveModifiers = (key, modifiers, context, loc) => {
// [ts] const keyModifiers = [];
// [ts] const nonKeyModifiers = [];
// [ts] const eventOptionModifiers = [];
// [ts] let i = 0;
// [ts] while (i < modifiers.length) { // [cap-migrate] for-step: i++
// [ts] const modifier = modifiers[i];
// [ts] if (isEventOptionModifier(modifier)) {
// eventOptionModifiers: modifiers for addEventListener() options,
// e.g. .passive & .capture
// [ts] eventOptionModifiers.push(modifier);
// [ts] }
// [ts] else {
// runtimeModifiers: modifiers that needs runtime guards
// [ts] if (maybeKeyModifier(modifier)) {
// [ts] if (isStaticExp(key)) {
// [ts] if (isKeyboardEvent(key.content)) {
// [ts] keyModifiers.push(modifier);
// [ts] }
// [ts] else {
// [ts] nonKeyModifiers.push(modifier);
// [ts] }
// [ts] }
// [ts] else {
// [ts] keyModifiers.push(modifier);
// [ts] nonKeyModifiers.push(modifier);
// [ts] }
// [ts] }
// [ts] else {
// [ts] if (isNonKeyModifier(modifier)) {
// [ts] nonKeyModifiers.push(modifier);
// [ts] }
// [ts] else {
// [ts] keyModifiers.push(modifier);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return {
// [ts] keyModifiers,
// [ts] nonKeyModifiers,
// [ts] eventOptionModifiers
// [ts] };
// [ts] };
// [ts] const transformClick = (key, event) => {
// [ts] const isStaticClick = isStaticExp(key) && key.content.toLowerCase() == "onclick";
// [ts] return if (isStaticClick) createSimpleExpression(event, true) else key.type != 4 /* SIMPLE_EXPRESSION */ ? createCompoundExpression([
// [ts] `(`,
// [ts] key,
// [ts] `) === "onClick" ? "${event}" : (`,
// [ts] key,
// [ts] `)`
// [ts] ]) : key;
// [ts] };
// [ts] const transformOn$1 = (dir, node, context) => {
// [ts] return transformOn(dir, node, context, baseResult => {
// [ts] const { modifiers } = dir;
// [ts] if (!modifiers.length)
// [ts] return baseResult;
// [ts] let { key, value: handlerExp } = baseResult.props[0];
// [ts] const { keyModifiers, nonKeyModifiers, eventOptionModifiers } = resolveModifiers(key, modifiers, context, dir.loc);
// normalize click.right and click.middle since they don't actually fire
// [ts] if (nonKeyModifiers.includes("right")) {
// [ts] key = transformClick(key, `onContextmenu`);
// [ts] }
// [ts] if (nonKeyModifiers.includes("middle")) {
// [ts] key = transformClick(key, `onMouseup`);
// [ts] }
// [ts] if (nonKeyModifiers.length) {
// [ts] handlerExp = createCallExpression(context.helper(V_ON_WITH_MODIFIERS), [
// [ts] handlerExp,
// [ts] JSON.stringify(nonKeyModifiers)
// [ts] ]);
// [ts] }
// [ts] if (keyModifiers.length &&
// if event name is dynamic, always wrap with keys guard
// [ts] (!isStaticExp(key) || isKeyboardEvent(key.content))) {
// [ts] handlerExp = createCallExpression(context.helper(V_ON_WITH_KEYS), [
// [ts] handlerExp,
// [ts] JSON.stringify(keyModifiers)
// [ts] ]);
// [ts] }
// [ts] if (eventOptionModifiers.length) {
// [ts] const modifierPostfix = eventOptionModifiers.map(capitalize).join("");
// [ts] key = if (isStaticExp(key)) createSimpleExpression(`${key.content}${modifierPostfix}`, true) else createCompoundExpression([`(`, key, `) + "${modifierPostfix}"`]);
// [ts] }
// [ts] return {
// [ts] props: [createObjectProperty(key, handlerExp)]
// [ts] };
// [ts] });
// [ts] };
// [ts] const transformShow = (dir, node, context) => {
// [ts] const { exp, loc } = dir;
// [ts] if (!exp) {
// [ts] context.onError(createDOMCompilerError(57 /* X_V_SHOW_NO_EXPRESSION */, loc));
// [ts] }
// [ts] return {
// [ts] props: [],
// [ts] needRuntime: context.helper(V_SHOW)
// [ts] };
// [ts] };
// [ts] const warnTransitionChildren = (node, context) => {
// [ts] if (node.type == 1 /* ELEMENT */ &&
// [ts] node.tagType == 1 /* COMPONENT */) {
// [ts] const component = context.isBuiltInComponent(node.tag);
// [ts] if (component == TRANSITION$1) {
// [ts] return () => {
// [ts] if (node.children.length && hasMultipleChildren(node)) {
// [ts] context.onError(createDOMCompilerError(58 /* X_TRANSITION_INVALID_CHILDREN */, {
// [ts] start: node.children[0].loc.start,
// [ts] end: node.children[node.children.length - 1].loc.end,
// [ts] source: ""
// [ts] }));
// [ts] }
// [ts] };
// [ts] }
// [ts] }
// [ts] };
// [ts] fn hasMultipleChildren(node: auto) {
// #1352 filter out potential comment nodes.
// [ts] const children = (node.children = node.children.filter(c => c.type != 3 /* COMMENT */));
// [ts] const child = children[0];
// [ts] return (children.length != 1 ||
// [ts] child.type == 11 /* FOR */ ||
// [ts] (child.type == 9 /* IF */ && child.branches.some(hasMultipleChildren)));
// [ts] }
// [ts] const ignoreSideEffectTags = (node, context) => {
// [ts] if (node.type == 1 /* ELEMENT */ &&
// [ts] node.tagType == 0 /* ELEMENT */ &&
// [ts] (node.tag == "script" || node.tag == "style")) {
// [ts] context.onError(createDOMCompilerError(59 /* X_IGNORED_SIDE_EFFECT_TAG */, node.loc));
// [ts] context.removeNode();
// [ts] }
// [ts] };
// [ts] const DOMNodeTransforms = [
// [ts] transformStyle,
// [ts] ...([warnTransitionChildren] )
// [ts] ];
// [ts] const DOMDirectiveTransforms = {
// [ts] cloak: noopDirectiveTransform,
// [ts] html: transformVHtml,
// [ts] text: transformVText,
// [ts] model: transformModel$1,
// [ts] on: transformOn$1,
// [ts] show: transformShow
// [ts] };
// [ts] (template, options) => {
// [ts] return baseCompile(template, extend({}, parserOptions, options, {
// [ts] nodeTransforms: [
// ignore <script> and <tag>
// this is not put inside DOMNodeTransforms because that list is used
// by compiler-ssr to generate vnode fallback branches
// [ts] ignoreSideEffectTags,
// [ts] ...DOMNodeTransforms,
// [ts] ...(options.nodeTransforms || [])
// [ts] ],
// [ts] directiveTransforms: extend({}, DOMDirectiveTransforms, options.directiveTransforms || {}),
// [ts] transformHoist: null
// [ts] }));
// [ts] }
// This entry is the "full-build" that includes both the runtime
// [ts] {
// [ts] initDev();
// [ts] }
// [ts] const compileCache = Object.create(null);
// [ts] fn compileToFunction(template: auto, options: auto) {
// [ts] if (!isString(template)) {
// [ts] if (template.nodeType) {
// [ts] template = template.innerHTML;
// [ts] }
// [ts] else {
// [ts] warn(`invalid template option: `, template);
// [ts] return NOOP;
// [ts] }
// [ts] }
// [ts] const key = template;
// [ts] const cached = compileCache[key];
// [ts] if (cached) {
// [ts] return cached;
// [ts] }
// [ts] if (template[0] == "#") {
// [ts] const el = document.querySelector(template);
// [ts] if (!el) {
// [ts] warn(`Template element not found or is empty: ${template}`);
// [ts] }
// __UNSAFE__
// Reason: potential execution of JS expressions in in-DOM template.
// The user must make sure the in-DOM template is trusted. If it's rendered
// by the server, the template should not contain any user data.
// [ts] template = if (el) el.innerHTML else ``;
// [ts] }
// [ts] const { code } = compile$1(template, extend({
// [ts] hoistStatic: true,
// [ts] onError: onError ,
// [ts] onWarn: e => onError(e, true)
// [ts] }, options));
// [ts] fn onError(err: auto, asWarning: auto = false) {
// [ts] const message = if (asWarning) err.message else `Template compilation error: ${err.message}`;
// [ts] const codeFrame = err.loc &&
// [ts] generateCodeFrame(template, err.loc.start.offset, err.loc.end.offset);
// [ts] warn(if (codeFrame) `${message}\n${codeFrame}` else message);
// [ts] }
// The wildcard import results in a huge object with every export
// with keys that cannot be mangled, and can be quite heavy size-wise.
// In the global build we know `Vue` is available globally so we can avoid
// the wildcard object.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const render = (new Function(code)()
// [ts] );
// [ts] render._rc = true;
// [ts] return (compileCache[key] = render);
// [ts] }
// [ts] registerRuntimeCompiler(compileToFunction);
// [ts] exports.BaseTransition = BaseTransition;
// [ts] exports.Comment = Comment$1;
// [ts] exports.Fragment = Fragment;
// [ts] exports.KeepAlive = KeepAlive;
// [ts] exports.Static = Static;
// [ts] exports.Suspense = Suspense;
// [ts] exports.Teleport = Teleport;
// [ts] exports.Text = Text;
// [ts] exports.Transition = Transition;
// [ts] exports.TransitionGroup = TransitionGroup;
// [ts] exports.callWithAsyncErrorHandling = callWithAsyncErrorHandling;
// [ts] exports.callWithErrorHandling = callWithErrorHandling;
// [ts] exports.camelize = camelize;
// [ts] exports.capitalize = capitalize;
// [ts] exports.cloneVNode = cloneVNode;
// [ts] exports.compatUtils = compatUtils;
// [ts] exports.compile = compileToFunction;
// [ts] exports.computed = computed$1;
// [ts] exports.createApp = createApp;
// [ts] exports.createBlock = createBlock;
// [ts] exports.createCommentVNode = createCommentVNode;
// [ts] exports.createHydrationRenderer = createHydrationRenderer;
// [ts] exports.createRenderer = createRenderer;
// [ts] exports.createSSRApp = createSSRApp;
// [ts] exports.createSlots = createSlots;
// [ts] exports.createStaticVNode = createStaticVNode;
// [ts] exports.createTextVNode = createTextVNode;
// [ts] exports.createVNode = createVNode;
// [ts] exports.customRef = customRef;
// [ts] exports.defineAsyncComponent = defineAsyncComponent;
// [ts] exports.defineComponent = defineComponent;
// [ts] exports.defineEmit = defineEmit;
// [ts] exports.defineEmits = defineEmits;
// [ts] exports.defineExpose = defineExpose;
// [ts] exports.defineProps = defineProps;
// [ts] exports.getCurrentInstance = getCurrentInstance;
// [ts] exports.getTransitionRawChildren = getTransitionRawChildren;
// [ts] exports.h = h;
// [ts] exports.handleError = handleError;
// [ts] exports.hydrate = hydrate;
// [ts] exports.initCustomFormatter = initCustomFormatter;
// [ts] exports.inject = inject;
// [ts] exports.isProxy = isProxy;
// [ts] exports.isReactive = isReactive;
// [ts] exports.isReadonly = isReadonly;
// [ts] exports.isRef = isRef;
// [ts] exports.isRuntimeOnly = isRuntimeOnly;
// [ts] exports.isVNode = isVNode;
// [ts] exports.markRaw = markRaw;
// [ts] exports.mergeDefaults = mergeDefaults;
// [ts] exports.mergeProps = mergeProps;
// [ts] exports.nextTick = nextTick;
// [ts] exports.onActivated = onActivated;
// [ts] exports.onBeforeMount = onBeforeMount;
// [ts] exports.onBeforeUnmount = onBeforeUnmount;
// [ts] exports.onBeforeUpdate = onBeforeUpdate;
// [ts] exports.onDeactivated = onDeactivated;
// [ts] exports.onErrorCaptured = onErrorCaptured;
// [ts] exports.onMounted = onMounted;
// [ts] exports.onRenderTracked = onRenderTracked;
// [ts] exports.onRenderTriggered = onRenderTriggered;
// [ts] exports.onServerPrefetch = onServerPrefetch;
// [ts] exports.onUnmounted = onUnmounted;
// [ts] exports.onUpdated = onUpdated;
// [ts] exports.openBlock = openBlock;
// [ts] exports.popScopeId = popScopeId;
// [ts] exports.provide = provide;
// [ts] exports.proxyRefs = proxyRefs;
// [ts] exports.pushScopeId = pushScopeId;
// [ts] exports.queuePostFlushCb = queuePostFlushCb;
// [ts] exports.reactive = reactive;
// [ts] exports.readonly = readonly;
// [ts] exports.ref = ref;
// [ts] exports.registerRuntimeCompiler = registerRuntimeCompiler;
// [ts] exports.render = render;
// [ts] exports.renderList = renderList;
// [ts] exports.renderSlot = renderSlot;
// [ts] exports.resolveComponent = resolveComponent;
// [ts] exports.resolveDirective = resolveDirective;
// [ts] exports.resolveDynamicComponent = resolveDynamicComponent;
// [ts] exports.resolveFilter = resolveFilter;
// [ts] exports.resolveTransitionHooks = resolveTransitionHooks;
// [ts] exports.setBlockTracking = setBlockTracking;
// [ts] exports.setDevtoolsHook = setDevtoolsHook;
// [ts] exports.setTransitionHooks = setTransitionHooks;
// [ts] exports.shallowReactive = shallowReactive;
// [ts] exports.shallowReadonly = shallowReadonly;
// [ts] exports.shallowRef = shallowRef;
// [ts] exports.ssrContextKey = ssrContextKey;
// [ts] exports.ssrUtils = ssrUtils;
// [ts] exports.toDisplayString = toDisplayString;
// [ts] exports.toHandlerKey = toHandlerKey;
// [ts] exports.toHandlers = toHandlers;
// [ts] exports.toRaw = toRaw;
// [ts] exports.toRef = toRef;
// [ts] exports.toRefs = toRefs;
// [ts] exports.transformVNodeArgs = transformVNodeArgs;
// [ts] exports.triggerRef = triggerRef;
// [ts] exports.unref = unref;
// [ts] exports.useAttrs = useAttrs;
// [ts] exports.useContext = useContext;
// [ts] exports.useCssModule = useCssModule;
// [ts] exports.useCssVars = useCssVars;
// [ts] exports.useSSRContext = useSSRContext;
// [ts] exports.useSlots = useSlots;
// [ts] exports.useTransitionState = useTransitionState;
// [ts] exports.vModelCheckbox = vModelCheckbox;
// [ts] exports.vModelDynamic = vModelDynamic;
// [ts] exports.vModelRadio = vModelRadio;
// [ts] exports.vModelSelect = vModelSelect;
// [ts] exports.vModelText = vModelText;
// [ts] exports.vShow = vShow;
// [ts] exports.version = version;
// [ts] exports.warn = warn;
// [ts] exports.watch = watch;
// [ts] exports.watchEffect = watchEffect;
// [ts] exports.withAsyncContext = withAsyncContext;
// [ts] exports.withCtx = withCtx;
// [ts] exports.withDefaults = withDefaults;
// [ts] exports.withDirectives = withDirectives;
// [ts] exports.withKeys = withKeys;
// [ts] exports.withModifiers = withModifiers;
// [ts] exports.withScopeId = withScopeId;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(exports, '__esModule', { value: true });
// [ts] return exports;
// [ts] }({}));
