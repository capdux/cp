// Migrated from TypeScript (assets/reading-list/react_15.7.0.js) via cap from -ts

 // TODO(cap-migrate): refine TS statement
 // [ts] /**
 // [ts] * React v15.7.0
 // [ts] */
 // [ts] ((f) => {if(@typeName(exports)=="object"&&@typeName(module)!="undefined"){module.exports=f()}else if(@typeName(define)=="function"&&define.amd){define([],f)}else{let g;if(@typeName(window)!="undefined"){g=window}else if(@typeName(global)!="undefined"){g=global}else if(@typeName(self)!="undefined"){g=self}else{g=this}g.React = f()}})(() => {let define,module,exports;return ((t, n, r) => {(o, u) => {if(!n[o]){if(!t[o]){let a=@typeName(require)=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);let f=Error.init("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}let l=n[o]={exports:{}};t[o][0].call(l.exports,(e) => {let n=t[o][1][e];return s(if (n) n else e)},l,l.exports,e,t,n,r)}return n[o].exports}let i=@typeName(require)=="function"&&require;for(let o=0;o<r.length;o++)s(r[o]);return s})({1:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] /**
 // [ts] * Escape and wrap key so it is safe to use as a reactid
 // [ts] *
 // [ts] * @param {string} key to be escaped.
 // [ts] * @return {string} the escaped key.
 // [ts] */
 // [ts] fn escape(key: auto) {
 // [ts] let escapeRegex = "[=:]";
 // [ts] let escaperLookup = {
 // [ts] "=": "=0",
 // [ts] ":": "=2"
 // [ts] };
 // [ts] let escapedString = ("" + key).replace(escapeRegex, (match_val) => {
 // [ts] return escaperLookup[match_val];
 // [ts] });
 // [ts] return "$" + escapedString;
 // [ts] }
 // [ts] /**
 // [ts] * Unescape and unwrap key for human-readable display
 // [ts] *
 // [ts] * @param {string} key to unescape.
 // [ts] * @return {string} the unescaped key.
 // [ts] */
 // [ts] fn unescape(key: auto) {
 // [ts] let unescapeRegex = "(=0|=2)";
 // [ts] let unescaperLookup = {
 // [ts] "=0": "=",
 // [ts] "=2": ":"
 // [ts] };
 // [ts] let keySubstring = if (key[0] == "." && key[1] == "$") key.substring(2) else key.substring(1);
 // [ts] return ("" + keySubstring).replace(unescapeRegex, (match_val) => {
 // [ts] return unescaperLookup[match_val];
 // [ts] });
 // [ts] }
 // [ts] let KeyEscapeUtils = {
 // [ts] escape: escape,
 // [ts] unescape: unescape
 // [ts] };
 // [ts] module.exports = KeyEscapeUtils;
 // [ts] },{}],2:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _prodInvariant = _dereq_(25);
 // [ts] let invariant = _dereq_(30);
 // [ts] /**
 // [ts] * Static poolers. Several custom versions for each potential number of
 // [ts] * arguments. A completely generic pooler is easy to implement, but would
 // [ts] * require accessing the `arguments` object. In each of these, `this` refers to
 // [ts] * the Class itself, not an instance. If any others are needed, simply add them
 // [ts] * here, or in their own files.
 // [ts] */
 // [ts] let oneArgumentPooler = (copyFieldsFrom) => {
 // [ts] let Klass = this;
 // [ts] if (Klass.instancePool.length) {
 // [ts] let instance = Klass.instancePool.pop();
 // [ts] Klass.call(instance, copyFieldsFrom);
 // [ts] return instance;
 // [ts] } else {
 // [ts] return Klass.init(copyFieldsFrom);
 // [ts] }
 // [ts] };
 // [ts] let twoArgumentPooler = (a1, a2) => {
 // [ts] let Klass = this;
 // [ts] if (Klass.instancePool.length) {
 // [ts] let instance = Klass.instancePool.pop();
 // [ts] Klass.call(instance, a1, a2);
 // [ts] return instance;
 // [ts] } else {
 // [ts] return Klass.init(a1, a2);
 // [ts] }
 // [ts] };
 // [ts] let threeArgumentPooler = (a1, a2, a3) => {
 // [ts] let Klass = this;
 // [ts] if (Klass.instancePool.length) {
 // [ts] let instance = Klass.instancePool.pop();
 // [ts] Klass.call(instance, a1, a2, a3);
 // [ts] return instance;
 // [ts] } else {
 // [ts] return Klass.init(a1, a2, a3);
 // [ts] }
 // [ts] };
 // [ts] let fourArgumentPooler = (a1, a2, a3, a4) => {
 // [ts] let Klass = this;
 // [ts] if (Klass.instancePool.length) {
 // [ts] let instance = Klass.instancePool.pop();
 // [ts] Klass.call(instance, a1, a2, a3, a4);
 // [ts] return instance;
 // [ts] } else {
 // [ts] return Klass.init(a1, a2, a3, a4);
 // [ts] }
 // [ts] };
 // [ts] let standardReleaser = (instance) => {
 // [ts] let Klass = this;
 // [ts] if (!(instance == Klass)) if ("development" != "production") invariant(false, "Trying to release an instance into a pool of a different type.") else _prodInvariant("25") else void 0;
 // [ts] instance.destructor();
 // [ts] if (Klass.instancePool.length < Klass.poolSize) {
 // [ts] Klass.instancePool.push(instance);
 // [ts] }
 // [ts] };
 // [ts] let DEFAULT_POOL_SIZE = 10;
 // [ts] let DEFAULT_POOLER = oneArgumentPooler;
 // [ts] /**
 // [ts] * Augments `CopyConstructor` to be a poolable class, augmenting only the class
 // [ts] * itself (statically) not adding any prototypical fields. Any CopyConstructor
 // [ts] * you give this may have a `poolSize` property, and will look for a
 // [ts] * prototypical `destructor` on instances.
 // [ts] *
 // [ts] * @param {Function} CopyConstructor Constructor that can be used to reset.
 // [ts] * @param {Function} pooler Customizable pooler.
 // [ts] */
 // [ts] let addPoolingTo = (CopyConstructor, pooler) => {
 // Casting as any so that flow ignores the actual implementation and trusts
 // it to match the type we declared
 // [ts] let NewKlass = CopyConstructor;
 // [ts] NewKlass.instancePool = [];
 // [ts] NewKlass.getPooled = pooler || DEFAULT_POOLER;
 // [ts] if (!NewKlass.poolSize) {
 // [ts] NewKlass.poolSize = DEFAULT_POOL_SIZE;
 // [ts] }
 // [ts] NewKlass.release = standardReleaser;
 // [ts] return NewKlass;
 // [ts] };
 // [ts] let PooledClass = {
 // [ts] addPoolingTo: addPoolingTo,
 // [ts] oneArgumentPooler: oneArgumentPooler,
 // [ts] twoArgumentPooler: twoArgumentPooler,
 // [ts] threeArgumentPooler: threeArgumentPooler,
 // [ts] fourArgumentPooler: fourArgumentPooler
 // [ts] };
 // [ts] module.exports = PooledClass;
 // [ts] },{"25":25,"30":30}],3:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _assign = _dereq_(32);
 // [ts] let ReactBaseClasses = _dereq_(4);
 // [ts] let ReactChildren = _dereq_(5);
 // [ts] let ReactDOMFactories = _dereq_(8);
 // [ts] let ReactElement = _dereq_(9);
 // [ts] let ReactPropTypes = _dereq_(14);
 // [ts] let ReactVersion = _dereq_(17);
 // [ts] let createReactClass = _dereq_(20);
 // [ts] let onlyChild = _dereq_(24);
 // [ts] let createElement = ReactElement.createElement;
 // [ts] let createFactory = ReactElement.createFactory;
 // [ts] let cloneElement = ReactElement.cloneElement;
 // [ts] if ("development" != "production") {
 // [ts] let lowPriorityWarning = _dereq_(23);
 // [ts] let canDefineProperty = _dereq_(18);
 // [ts] let ReactElementValidator = _dereq_(11);
 // [ts] let didWarnPropTypesDeprecated = false;
 // [ts] createElement = ReactElementValidator.createElement;
 // [ts] createFactory = ReactElementValidator.createFactory;
 // [ts] cloneElement = ReactElementValidator.cloneElement;
 // [ts] }
 // [ts] let __spread = _assign;
 // [ts] let createMixin = (mixin) => {
 // [ts] return mixin;
 // [ts] };
 // [ts] if ("development" != "production") {
 // [ts] let warnedForSpread = false;
 // [ts] let warnedForCreateMixin = false;
 // [ts] __spread = () => {
 // [ts] lowPriorityWarning(warnedForSpread, "React.__spread is deprecated and should not be used. Use " + "Object.assign directly or another helper function with similar " + "semantics. You may be seeing this warning due to your compiler. " + "See https://fb.me/react-spread-deprecation for more details.");
 // [ts] warnedForSpread = true;
 // [ts] return _assign.apply(null, arguments);
 // [ts] };
 // [ts] createMixin = (mixin) => {
 // [ts] lowPriorityWarning(warnedForCreateMixin, "React.createMixin is deprecated and should not be used. " + "In React v16.0, it will be removed. " + "You can use this mixin directly instead. " + "See https://fb.me/createmixin-was-never-implemented for more info.");
 // [ts] warnedForCreateMixin = true;
 // [ts] return mixin;
 // [ts] };
 // [ts] }
 // [ts] let React = {
 // Modern
 // [ts] Children: {
 // [ts] map: ReactChildren.map,
 // [ts] forEach: ReactChildren.forEach,
 // [ts] count: ReactChildren.count,
 // [ts] toArray: ReactChildren.toArray,
 // [ts] only: onlyChild
 // [ts] },
 // [ts] Component: ReactBaseClasses.Component,
 // [ts] PureComponent: ReactBaseClasses.PureComponent,
 // [ts] createElement: createElement,
 // [ts] cloneElement: cloneElement,
 // [ts] isValidElement: ReactElement.isValidElement,
 // Classic
 // [ts] PropTypes: ReactPropTypes,
 // [ts] createClass: createReactClass,
 // [ts] createFactory: createFactory,
 // [ts] createMixin: createMixin,
 // This looks DOM specific but these are actually isomorphic helpers
 // since they are just generating DOM strings.
 // [ts] DOM: ReactDOMFactories,
 // [ts] version: ReactVersion,
 // Deprecated hook for JSX spread, don't use this for anything.
 // [ts] __spread: __spread
 // [ts] };
 // [ts] if ("development" != "production") {
 // [ts] let warnedForCreateClass = false;
 // [ts] if (canDefineProperty) {
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(React, 'PropTypes', {
 // [ts-skip] get: function () {
 // [ts-skip] lowPriorityWarning(didWarnPropTypesDeprecated, 'Accessing PropTypes via the main React package is deprecated,' + ' and will be removed in  React v16.0.' + ' Use the latest available v15.* prop-types package from npm instead.' + ' For info on usage, compatibility, migration and more, see ' + 'https://fb.me/prop-types-docs');
 // [ts-skip] didWarnPropTypesDeprecated = true;
 // [ts-skip] return ReactPropTypes;
 // [ts-skip] }
 // [ts-skip] });
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(React, 'createClass', {
 // [ts-skip] get: function () {
 // [ts-skip] lowPriorityWarning(warnedForCreateClass, 'Accessing createClass via the main React package is deprecated,' + ' and will be removed in React v16.0.' + " Use a plain JavaScript class instead. If you're not yet " + 'ready to migrate, create-react-class v15.* is available ' + 'on npm as a temporary, drop-in replacement. ' + 'For more info see https://fb.me/react-create-class');
 // [ts-skip] warnedForCreateClass = true;
 // [ts-skip] return createReactClass;
 // [ts-skip] }
 // [ts-skip] });
 // [ts] }
 // React.DOM factories are deprecated. Wrap these methods so that
 // invocations of the React.DOM namespace and alert users to switch
 // to the `react-dom-factories` package.
 // [ts] React.DOM = {};
 // [ts] let warnedForFactories = false;
 // [ts] Object.keys(ReactDOMFactories).forEach((factory) => {
 // [ts] React.DOM[factory] = () => {
 // [ts] if (!warnedForFactories) {
 // [ts] lowPriorityWarning(false, "Accessing factories like React.DOM.%s has been deprecated " + "and will be removed in v16.0+. Use the " + "react-dom-factories package instead. " + " Version 1.0 provides a drop-in replacement." + " For more info, see https://fb.me/react-dom-factories", factory);
 // [ts] warnedForFactories = true;
 // [ts] }
 // [ts] return ReactDOMFactories[factory].apply(ReactDOMFactories, arguments);
 // [ts] };
 // [ts] });
 // [ts] }
 // [ts] module.exports = React;
 // [ts] },{"11":11,"14":14,"17":17,"18":18,"20":20,"23":23,"24":24,"32":32,"4":4,"5":5,"8":8,"9":9}],4:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _prodInvariant = _dereq_(25),
 // [ts] _assign = _dereq_(32);
 // [ts] let ReactNoopUpdateQueue = _dereq_(12);
 // [ts] let canDefineProperty = _dereq_(18);
 // [ts] let emptyObject = _dereq_(29);
 // [ts] let invariant = _dereq_(30);
 // [ts] let lowPriorityWarning = _dereq_(23);
 // [ts] /**
 // [ts] * Base class helpers for the updating state of a component.
 // [ts] */
 // [ts] fn ReactComponent(props: auto, context: auto, updater: auto) {
 // [ts] _.props = props;
 // [ts] _.context = context;
 // [ts] _.refs = emptyObject;
 // We initialize the default updater but the real one gets injected by the
 // renderer.
 // [ts] _.updater = updater || ReactNoopUpdateQueue;
 // [ts] }
 // [ts] ReactComponent.prototype.isReactComponent = {};
 // [ts] /**
 // [ts] * Sets a subset of the state. Always use this to mutate
 // [ts] * state. You should treat `this.state` as immutable.
 // [ts] *
 // [ts] * There is no guarantee that `this.state` will be immediately updated, so
 // [ts] * accessing `this.state` after calling this method may return the old value.
 // [ts] *
 // [ts] * There is no guarantee that calls to `setState` will run synchronously,
 // [ts] * as they may eventually be batched together.  You can provide an optional
 // [ts] * callback that will be executed when the call to setState is actually
 // [ts] * completed.
 // [ts] *
 // [ts] * When a function is provided to setState, it will be called at some point in
 // [ts] * the future (not synchronously). It will be called with the up to date
 // [ts] * component arguments (state, props, context). These values can be different
 // [ts] * from this.* because your function may be called after receiveProps but before
 // [ts] * shouldComponentUpdate, and this new state, props, and context will not yet be
 // [ts] * assigned to this.
 // [ts] *
 // [ts] * @param {object|function} partialState Next partial state or function to
 // [ts] *        produce next partial state to be merged with current state.
 // [ts] * @param {?function} callback Called after state is updated.
 // [ts] * @final
 // [ts] * @protected
 // [ts] */
 // [ts] ReactComponent.prototype.setState = (partialState, callback) => {
 // [ts] if (!(@typeName(partialState) == "object" || @typeName(partialState) == "function" || partialState == null)) if ("development" != "production") invariant(false, "setState(...): takes an object of state variables to update or a function which returns an object of state variables.") else _prodInvariant("85") else void 0;
 // [ts] _.updater.enqueueSetState(this, partialState);
 // [ts] if (callback) {
 // [ts] _.updater.enqueueCallback(this, callback, "setState");
 // [ts] }
 // [ts] };
 // [ts] /**
 // [ts] * Forces an update. This should only be invoked when it is known with
 // [ts] * certainty that we are **not** in a DOM transaction.
 // [ts] *
 // [ts] * You may want to call this when you know that some deeper aspect of the
 // [ts] * component's state has changed but `setState` was not called.
 // [ts] *
 // [ts] * This will not invoke `shouldComponentUpdate`, but it will invoke
 // [ts] * `componentWillUpdate` and `componentDidUpdate`.
 // [ts] *
 // [ts] * @param {?function} callback Called after update is complete.
 // [ts] * @final
 // [ts] * @protected
 // [ts] */
 // [ts] ReactComponent.prototype.forceUpdate = (callback) => {
 // [ts] _.updater.enqueueForceUpdate(this);
 // [ts] if (callback) {
 // [ts] _.updater.enqueueCallback(this, callback, "forceUpdate");
 // [ts] }
 // [ts] };
 // [ts] /**
 // [ts] * Deprecated APIs. These APIs used to exist on classic React classes but since
 // [ts] * we would like to deprecate them, we're not going to move them over to this
 // [ts] * modern base class. Instead, we define a getter that warns if it's accessed.
 // [ts] */
 // [ts] if ("development" != "production") {
 // [ts] let deprecatedAPIs = {
 // [ts] isMounted: ["isMounted", "Instead, make sure to clean up subscriptions and pending requests in " + "componentWillUnmount to prevent memory leaks."],
 // [ts] replaceState: ["replaceState", "Refactor your code to use setState instead (see " + "https://github.com/facebook/react/issues/3236)."]
 // [ts] };
 // [ts] let defineDeprecationWarning = (methodName, info) => {
 // [ts] if (canDefineProperty) {
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(ReactComponent.prototype, methodName, {
 // [ts-skip] get: function () {
 // [ts-skip] lowPriorityWarning(false, '%s(...) is deprecated in plain JavaScript React classes. %s', info[0], info[1]);
 // [ts-skip] return undefined;
 // [ts-skip] }
 // [ts-skip] });
 // [ts] }
 // [ts] };
 // [ts] for (let fnName in deprecatedAPIs) {
 // [ts] if (deprecatedAPIs.hasOwnProperty(fnName)) {
 // [ts] defineDeprecationWarning(fnName, deprecatedAPIs[fnName]);
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] /**
 // [ts] * Base class helpers for the updating state of a component.
 // [ts] */
 // [ts] fn ReactPureComponent(props: auto, context: auto, updater: auto) {
 // Duplicated from ReactComponent.
 // [ts] _.props = props;
 // [ts] _.context = context;
 // [ts] _.refs = emptyObject;
 // We initialize the default updater but the real one gets injected by the
 // renderer.
 // [ts] _.updater = updater || ReactNoopUpdateQueue;
 // [ts] }
 // [ts] fn ComponentDummy() {}
 // [ts] ComponentDummy.prototype = ReactComponent.prototype;
 // [ts] ReactPureComponent.prototype = ComponentDummy.init();
 // [ts] ReactPureComponent.prototype.constructor = ReactPureComponent;
 // Avoid an extra prototype jump for these methods.
 // [ts] _assign(ReactPureComponent.prototype, ReactComponent.prototype);
 // [ts] ReactPureComponent.prototype.isPureReactComponent = true;
 // [ts] module.exports = {
 // [ts] Component: ReactComponent,
 // [ts] PureComponent: ReactPureComponent
 // [ts] };
 // [ts] },{"12":12,"18":18,"23":23,"25":25,"29":29,"30":30,"32":32}],5:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let PooledClass = _dereq_(2);
 // [ts] let ReactElement = _dereq_(9);
 // [ts] let emptyFunction = _dereq_(28);
 // [ts] let traverseAllChildren = _dereq_(26);
 // [ts] let twoArgumentPooler = PooledClass.twoArgumentPooler;
 // [ts] let fourArgumentPooler = PooledClass.fourArgumentPooler;
 // [ts] let userProvidedKeyEscapeRegex = "\\/+";
 // [ts] fn escapeUserProvidedKey(text: auto) {
 // [ts] return ("" + text).replace(userProvidedKeyEscapeRegex, "$&/");
 // [ts] }
 // [ts] /**
 // [ts] * PooledClass representing the bookkeeping associated with performing a child
 // [ts] * traversal. Allows avoiding binding callbacks.
 // [ts] *
 // [ts] * @constructor ForEachBookKeeping
 // [ts] * @param {!function} forEachFunction Function to perform traversal with.
 // [ts] * @param {?*} forEachContext Context to perform context with.
 // [ts] */
 // [ts] fn ForEachBookKeeping(forEachFunction: auto, forEachContext: auto) {
 // [ts] _.func = forEachFunction;
 // [ts] _.context = forEachContext;
 // [ts] _.count = 0;
 // [ts] }
 // [ts] ForEachBookKeeping.prototype.destructor = () => {
 // [ts] _.func = null;
 // [ts] _.context = null;
 // [ts] _.count = 0;
 // [ts] };
 // [ts] PooledClass.addPoolingTo(ForEachBookKeeping, twoArgumentPooler);
 // [ts] fn forEachSingleChild(bookKeeping: auto, child: auto, name: auto) {
 // [ts] let func = bookKeeping.func,
 // [ts] context = bookKeeping.context;
 // [ts] func.call(context, child, bookKeeping.count++);
 // [ts] }
 // [ts] /**
 // [ts] * Iterates through children that are typically specified as `props.children`.
 // [ts] *
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.children.foreach
 // [ts] *
 // [ts] * The provided forEachFunc(child, index) will be called for each
 // [ts] * leaf child.
 // [ts] *
 // [ts] * @param {?*} children Children tree container.
 // [ts] * @param {function(*, int)} forEachFunc
 // [ts] * @param {*} forEachContext Context for forEachContext.
 // [ts] */
 // [ts] fn forEachChildren(children: auto, forEachFunc: auto, forEachContext: auto) {
 // [ts] if (children == null) {
 // [ts] return children;
 // [ts] }
 // [ts] let traverseContext = ForEachBookKeeping.getPooled(forEachFunc, forEachContext);
 // [ts] traverseAllChildren(children, forEachSingleChild, traverseContext);
 // [ts] ForEachBookKeeping.release(traverseContext);
 // [ts] }
 // [ts] /**
 // [ts] * PooledClass representing the bookkeeping associated with performing a child
 // [ts] * mapping. Allows avoiding binding callbacks.
 // [ts] *
 // [ts] * @constructor MapBookKeeping
 // [ts] * @param {!*} mapResult Object containing the ordered map of results.
 // [ts] * @param {!function} mapFunction Function to perform mapping with.
 // [ts] * @param {?*} mapContext Context to perform mapping with.
 // [ts] */
 // [ts] fn MapBookKeeping(mapResult: auto, keyPrefix: auto, mapFunction: auto, mapContext: auto) {
 // [ts] _.result = mapResult;
 // [ts] _.keyPrefix = keyPrefix;
 // [ts] _.func = mapFunction;
 // [ts] _.context = mapContext;
 // [ts] _.count = 0;
 // [ts] }
 // [ts] MapBookKeeping.prototype.destructor = () => {
 // [ts] _.result = null;
 // [ts] _.keyPrefix = null;
 // [ts] _.func = null;
 // [ts] _.context = null;
 // [ts] _.count = 0;
 // [ts] };
 // [ts] PooledClass.addPoolingTo(MapBookKeeping, fourArgumentPooler);
 // [ts] fn mapSingleChildIntoContext(bookKeeping: auto, child: auto, childKey: auto) {
 // [ts] let result = bookKeeping.result,
 // [ts] keyPrefix = bookKeeping.keyPrefix,
 // [ts] func = bookKeeping.func,
 // [ts] context = bookKeeping.context;
 // [ts] let mappedChild = func.call(context, child, bookKeeping.count++);
 // [ts] if (Array.isArray(mappedChild)) {
 // [ts] mapIntoWithKeyPrefixInternal(mappedChild, result, childKey, emptyFunction.thatReturnsArgument);
 // [ts] } else if (mappedChild != null) {
 // [ts] if (ReactElement.isValidElement(mappedChild)) {
 // [ts] mappedChild = ReactElement.cloneAndReplaceKey(mappedChild,
 // Keep both the (mapped) and old keys if they differ, just as
 // traverseAllChildren used to do for objects as children
 // [ts] keyPrefix + (if (mappedChild.key && (!child || child.key != mappedChild.key)) escapeUserProvidedKey(mappedChild.key) + "/" else "") + childKey);
 // [ts] }
 // [ts] result.push(mappedChild);
 // [ts] }
 // [ts] }
 // [ts] fn mapIntoWithKeyPrefixInternal(children: auto, array: auto, prefix: auto, func: auto, context: auto) {
 // [ts] let escapedPrefix = "";
 // [ts] if (prefix != null) {
 // [ts] escapedPrefix = escapeUserProvidedKey(prefix) + "/";
 // [ts] }
 // [ts] let traverseContext = MapBookKeeping.getPooled(array, escapedPrefix, func, context);
 // [ts] traverseAllChildren(children, mapSingleChildIntoContext, traverseContext);
 // [ts] MapBookKeeping.release(traverseContext);
 // [ts] }
 // [ts] /**
 // [ts] * Maps children that are typically specified as `props.children`.
 // [ts] *
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.children.map
 // [ts] *
 // [ts] * The provided mapFunction(child, key, index) will be called for each
 // [ts] * leaf child.
 // [ts] *
 // [ts] * @param {?*} children Children tree container.
 // [ts] * @param {function(*, int)} func The map function.
 // [ts] * @param {*} context Context for mapFunction.
 // [ts] * @return {object} Object containing the ordered map of results.
 // [ts] */
 // [ts] fn mapChildren(children: auto, func: auto, context: auto) {
 // [ts] if (children == null) {
 // [ts] return children;
 // [ts] }
 // [ts] let result = [];
 // [ts] mapIntoWithKeyPrefixInternal(children, result, null, func, context);
 // [ts] return result;
 // [ts] }
 // [ts] fn forEachSingleChildDummy(traverseContext: auto, child: auto, name: auto) {
 // [ts] return null;
 // [ts] }
 // [ts] /**
 // [ts] * Count the number of children that are typically specified as
 // [ts] * `props.children`.
 // [ts] *
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.children.count
 // [ts] *
 // [ts] * @param {?*} children Children tree container.
 // [ts] * @return {number} The number of children.
 // [ts] */
 // [ts] fn countChildren(children: auto, context: auto) {
 // [ts] return traverseAllChildren(children, forEachSingleChildDummy, null);
 // [ts] }
 // [ts] /**
 // [ts] * Flatten a children object (typically specified as `props.children`) and
 // [ts] * return an array with appropriately re-keyed children.
 // [ts] *
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.children.toarray
 // [ts] */
 // [ts] fn toArray(children: auto) {
 // [ts] let result = [];
 // [ts] mapIntoWithKeyPrefixInternal(children, result, null, emptyFunction.thatReturnsArgument);
 // [ts] return result;
 // [ts] }
 // [ts] let ReactChildren = {
 // [ts] forEach: forEachChildren,
 // [ts] map: mapChildren,
 // [ts] mapIntoWithKeyPrefixInternal: mapIntoWithKeyPrefixInternal,
 // [ts] count: countChildren,
 // [ts] toArray: toArray
 // [ts] };
 // [ts] module.exports = ReactChildren;
 // [ts] },{"2":2,"26":26,"28":28,"9":9}],6:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2016-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _prodInvariant = _dereq_(25);
 // [ts] let ReactCurrentOwner = _dereq_(7);
 // [ts] let invariant = _dereq_(30);
 // [ts] let warning = _dereq_(31);
 // [ts] fn isNative(fn: auto) {
 // Based on isNative() from Lodash
 // [ts] let funcToString = Function.prototype.toString;
 // [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
 // [ts] let reIsNative = RegExp("^" + funcToString
 // Take an example native function source for comparison
 // [ts] .call(hasOwnProperty
 // Strip regex characters so we can use it for regex
 // [ts] ).replace("[\\\\^$.*+?()[\\]{}|]", "\\$&"
 // Remove hasOwnProperty from the template to make it generic
 // [ts] ).replace("hasOwnProperty|(function).*?(?=\\\\\\()| for .+?(?=\\\\\\])", "$1.*?") + "$");
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // [ts] let source = funcToString.call(fn);
 // [ts] return reIsNative.test(source);
 // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] let canUseCollections =
 // Array.from
 // [ts] @typeName(Array.from) == "function" &&
 // Map
 // [ts] @typeName(Map) == "function" && isNative(Map) &&
 // Map.prototype.keys
 // [ts] Map.prototype != null && @typeName(Map.prototype.keys) == "function" && isNative(Map.prototype.keys) &&
 // Set
 // [ts] @typeName(Set) == "function" && isNative(Set) &&
 // Set.prototype.keys
 // [ts] Set.prototype != null && @typeName(Set.prototype.keys) == "function" && isNative(Set.prototype.keys);
 // [ts] let setItem;
 // [ts] let getItem;
 // [ts] let removeItem;
 // [ts] let getItemIDs;
 // [ts] let addRoot;
 // [ts] let removeRoot;
 // [ts] let getRootIDs;
 // [ts] if (canUseCollections) {
 // [ts] let itemMap = Map.init();
 // [ts] let rootIDSet = Set.init();
 // [ts] setItem = (id, item) => {
 // [ts] itemMap.set(id, item);
 // [ts] };
 // [ts] getItem = (id) => {
 // [ts] return itemMap.get(id);
 // [ts] };
 // [ts] removeItem = (id) => {
 // [ts] itemMap["delete"](id);
 // [ts] };
 // [ts] getItemIDs = () => {
 // [ts] return Array.from(itemMap.keys());
 // [ts] };
 // [ts] addRoot = (id) => {
 // [ts] rootIDSet.add(id);
 // [ts] };
 // [ts] removeRoot = (id) => {
 // [ts] rootIDSet["delete"](id);
 // [ts] };
 // [ts] getRootIDs = () => {
 // [ts] return Array.from(rootIDSet.keys());
 // [ts] };
 // [ts] } else {
 // [ts] let itemByKey = {};
 // [ts] let rootByKey = {};
 // Use non-numeric keys to prevent V8 performance issues:
 // https://github.com/facebook/react/pull/7232
 // [ts] let getKeyFromID = (id) => {
 // [ts] return "." + id;
 // [ts] };
 // [ts] let getIDFromKey = (key) => {
 // [ts] return parseInt(key.substr(1), 10);
 // [ts] };
 // [ts] setItem = (id, item) => {
 // [ts] let key = getKeyFromID(id);
 // [ts] itemByKey[key] = item;
 // [ts] };
 // [ts] getItem = (id) => {
 // [ts] let key = getKeyFromID(id);
 // [ts] return itemByKey[key];
 // [ts] };
 // [ts] removeItem = (id) => {
 // [ts] let key = getKeyFromID(id);
 // [ts] delete itemByKey[key];
 // [ts] };
 // [ts] getItemIDs = () => {
 // [ts] return Object.keys(itemByKey).map(getIDFromKey);
 // [ts] };
 // [ts] addRoot = (id) => {
 // [ts] let key = getKeyFromID(id);
 // [ts] rootByKey[key] = true;
 // [ts] };
 // [ts] removeRoot = (id) => {
 // [ts] let key = getKeyFromID(id);
 // [ts] delete rootByKey[key];
 // [ts] };
 // [ts] getRootIDs = () => {
 // [ts] return Object.keys(rootByKey).map(getIDFromKey);
 // [ts] };
 // [ts] }
 // [ts] let unmountedIDs = [];
 // [ts] fn purgeDeep(id: auto) {
 // [ts] let item = getItem(id);
 // [ts] if (item) {
 // [ts] let childIDs = item.childIDs;
 // [ts] removeItem(id);
 // [ts] childIDs.forEach(purgeDeep);
 // [ts] }
 // [ts] }
 // [ts] fn describeComponentFrame(name: auto, source: auto, ownerName: auto) {
 // [ts] return "\n    in " + (name || "Unknown") + (if (source) " (at " + source.fileName.replace("^.*[\\\\\\/]", "") + ":" + source.lineNumber + ")" else if (ownerName) " (created by " + ownerName + ")" else "");
 // [ts] }
 // [ts] fn getDisplayName(element: auto) {
 // [ts] if (element == null) {
 // [ts] return "#empty";
 // [ts] } else if (@typeName(element) == "string" || @typeName(element) == "number") {
 // [ts] return "#text";
 // [ts] } else if (@typeName(element.type) == "string") {
 // [ts] return element.type;
 // [ts] } else {
 // [ts] return element.type.displayName || element.type.name || "Unknown";
 // [ts] }
 // [ts] }
 // [ts] fn describeID(id: auto) {
 // [ts] let name = ReactComponentTreeHook.getDisplayName(id);
 // [ts] let element = ReactComponentTreeHook.getElement(id);
 // [ts] let ownerID = ReactComponentTreeHook.getOwnerID(id);
 // [ts] let ownerName;
 // [ts] if (ownerID) {
 // [ts] ownerName = ReactComponentTreeHook.getDisplayName(ownerID);
 // [ts] }
 // [ts] if ("development" != "production") warning(element, "ReactComponentTreeHook: Missing React element for debugID %s when " + "building stack", id) else void 0;
 // [ts] return describeComponentFrame(name, element && element._source, ownerName);
 // [ts] }
 // [ts] let ReactComponentTreeHook = {
 // [ts] onSetChildren: (id, nextChildIDs) => {
 // [ts] let item = getItem(id);
 // [ts] if (!item) if ("development" != "production") invariant(false, "Item must have been set") else _prodInvariant("144") else void 0;
 // [ts] item.childIDs = nextChildIDs;
 // [ts] let i = 0;
 // [ts] while (i < nextChildIDs.length) { // [cap-migrate] for-step: i++
 // [ts] let nextChildID = nextChildIDs[i];
 // [ts] let nextChild = getItem(nextChildID);
 // [ts] if (!nextChild) if ("development" != "production") invariant(false, "Expected hook events to fire for the child before its parent includes it in onSetChildren().") else _prodInvariant("140") else void 0;
 // [ts] if (!(nextChild.childIDs != null || @typeName(nextChild.element) != "object" || nextChild.element == null)) if ("development" != "production") invariant(false, "Expected onSetChildren() to fire for a container child before its parent includes it in onSetChildren().") else _prodInvariant("141") else void 0;
 // [ts] if (!nextChild.isMounted) if ("development" != "production") invariant(false, "Expected onMountComponent() to fire for the child before its parent includes it in onSetChildren().") else _prodInvariant("71") else void 0;
 // [ts] if (nextChild.parentID == null) {
 // [ts] nextChild.parentID = id;
 // TODO: This shouldn't be necessary but mounting a new root during in
 // componentWillMount currently causes not-yet-mounted components to
 // be purged from our tree data so their parent id is missing.
 // [ts] }
 // [ts] if (!(nextChild.parentID == id)) if ("development" != "production") invariant(false, "Expected onBeforeMountComponent() parent and onSetChildren() to be consistent (%s has parents %s and %s).", nextChildID, nextChild.parentID, id) else _prodInvariant("142", nextChildID, nextChild.parentID, id) else void 0;
 // [ts] }
 // [ts] },
 // [ts] onBeforeMountComponent: (id, element, parentID) => {
 // [ts] let item = {
 // [ts] element: element,
 // [ts] parentID: parentID,
 // [ts] text: null,
 // [ts] childIDs: [],
 // [ts] isMounted: false,
 // [ts] updateCount: 0
 // [ts] };
 // [ts] setItem(id, item);
 // [ts] },
 // [ts] onBeforeUpdateComponent: (id, element) => {
 // [ts] let item = getItem(id);
 // [ts] if (!item || !item.isMounted) {
 // We may end up here as a result of setState() in componentWillUnmount().
 // In this case, ignore the element.
 // [ts] return;
 // [ts] }
 // [ts] item.element = element;
 // [ts] },
 // [ts] onMountComponent: (id) => {
 // [ts] let item = getItem(id);
 // [ts] if (!item) if ("development" != "production") invariant(false, "Item must have been set") else _prodInvariant("144") else void 0;
 // [ts] item.isMounted = true;
 // [ts] let isRoot = item.parentID == 0;
 // [ts] if (isRoot) {
 // [ts] addRoot(id);
 // [ts] }
 // [ts] },
 // [ts] onUpdateComponent: (id) => {
 // [ts] let item = getItem(id);
 // [ts] if (!item || !item.isMounted) {
 // We may end up here as a result of setState() in componentWillUnmount().
 // In this case, ignore the element.
 // [ts] return;
 // [ts] }
 // [ts] item.updateCount++;
 // [ts] },
 // [ts] onUnmountComponent: (id) => {
 // [ts] let item = getItem(id);
 // [ts] if (item) {
 // We need to check if it exists.
 // `item` might not exist if it is inside an error boundary, and a sibling
 // error boundary child threw while mounting. Then this instance never
 // got a chance to mount, but it still gets an unmounting event during
 // the error boundary cleanup.
 // [ts] item.isMounted = false;
 // [ts] let isRoot = item.parentID == 0;
 // [ts] if (isRoot) {
 // [ts] removeRoot(id);
 // [ts] }
 // [ts] }
 // [ts] unmountedIDs.push(id);
 // [ts] },
 // [ts] purgeUnmountedComponents: () => {
 // [ts] if (ReactComponentTreeHook._preventPurging) {
 // Should only be used for testing.
 // [ts] return;
 // [ts] }
 // [ts] let i = 0;
 // [ts] while (i < unmountedIDs.length) { // [cap-migrate] for-step: i++
 // [ts] let id = unmountedIDs[i];
 // [ts] purgeDeep(id);
 // [ts] }
 // [ts] unmountedIDs.length = 0;
 // [ts] },
 // [ts] isMounted: (id) => {
 // [ts] let item = getItem(id);
 // [ts] return if (item) item.isMounted else false;
 // [ts] },
 // [ts] getCurrentStackAddendum: (topElement) => {
 // [ts] let info = "";
 // [ts] if (topElement) {
 // [ts] let name = getDisplayName(topElement);
 // [ts] let owner = topElement._owner;
 // [ts] info += describeComponentFrame(name, topElement._source, owner && owner.getName());
 // [ts] }
 // [ts] let currentOwner = ReactCurrentOwner.current;
 // [ts] let id = currentOwner && currentOwner._debugID;
 // [ts] info += ReactComponentTreeHook.getStackAddendumByID(id);
 // [ts] return info;
 // [ts] },
 // [ts] getStackAddendumByID: (id) => {
 // [ts] let info = "";
 // [ts] while (id) {
 // [ts] info += describeID(id);
 // [ts] id = ReactComponentTreeHook.getParentID(id);
 // [ts] }
 // [ts] return info;
 // [ts] },
 // [ts] getChildIDs: (id) => {
 // [ts] let item = getItem(id);
 // [ts] return if (item) item.childIDs else [];
 // [ts] },
 // [ts] getDisplayName: (id) => {
 // [ts] let element = ReactComponentTreeHook.getElement(id);
 // [ts] if (!element) {
 // [ts] return null;
 // [ts] }
 // [ts] return getDisplayName(element);
 // [ts] },
 // [ts] getElement: (id) => {
 // [ts] let item = getItem(id);
 // [ts] return if (item) item.element else null;
 // [ts] },
 // [ts] getOwnerID: (id) => {
 // [ts] let element = ReactComponentTreeHook.getElement(id);
 // [ts] if (!element || !element._owner) {
 // [ts] return null;
 // [ts] }
 // [ts] return element._owner._debugID;
 // [ts] },
 // [ts] getParentID: (id) => {
 // [ts] let item = getItem(id);
 // [ts] return if (item) item.parentID else null;
 // [ts] },
 // [ts] getSource: (id) => {
 // [ts] let item = getItem(id);
 // [ts] let element = if (item) item.element else null;
 // [ts] let source = if (element != null) element._source else null;
 // [ts] return source;
 // [ts] },
 // [ts] getText: (id) => {
 // [ts] let element = ReactComponentTreeHook.getElement(id);
 // [ts] if (@typeName(element) == "string") {
 // [ts] return element;
 // [ts] } else if (@typeName(element) == "number") {
 // [ts] return "" + element;
 // [ts] } else {
 // [ts] return null;
 // [ts] }
 // [ts] },
 // [ts] getUpdateCount: (id) => {
 // [ts] let item = getItem(id);
 // [ts] return if (item) item.updateCount else 0;
 // [ts] },
 // [ts] getRootIDs: getRootIDs,
 // [ts] getRegisteredIDs: getItemIDs,
 // [ts] pushNonStandardWarningStack: (isCreatingElement, currentSource) => {
 // [ts] if (@typeName(console.reactStack) != "function") {
 // [ts] return;
 // [ts] }
 // [ts] let stack = [];
 // [ts] let currentOwner = ReactCurrentOwner.current;
 // [ts] let id = currentOwner && currentOwner._debugID;
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // [ts] if (isCreatingElement) {
 // [ts] stack.push({
 // [ts] name: if (id) ReactComponentTreeHook.getDisplayName(id) else null,
 // [ts] fileName: if (currentSource) currentSource.fileName else null,
 // [ts] lineNumber: if (currentSource) currentSource.lineNumber else null
 // [ts] });
 // [ts] }
 // [ts] while (id) {
 // [ts] let element = ReactComponentTreeHook.getElement(id);
 // [ts] let parentID = ReactComponentTreeHook.getParentID(id);
 // [ts] let ownerID = ReactComponentTreeHook.getOwnerID(id);
 // [ts] let ownerName = if (ownerID) ReactComponentTreeHook.getDisplayName(ownerID) else null;
 // [ts] let source = element && element._source;
 // [ts] stack.push({
 // [ts] name: ownerName,
 // [ts] fileName: if (source) source.fileName else null,
 // [ts] lineNumber: if (source) source.lineNumber else null
 // [ts] });
 // [ts] id = parentID;
 // [ts] }
 // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
 // Internal state is messed up.
 // Stop building the stack (it's just a nice to have).
 // [ts] }
 // [ts] console.reactStack(stack);
 // [ts] },
 // [ts] popNonStandardWarningStack: () => {
 // [ts] if (@typeName(console.reactStackEnd) != "function") {
 // [ts] return;
 // [ts] }
 // [ts] console.reactStackEnd();
 // [ts] }
 // [ts] };
 // [ts] module.exports = ReactComponentTreeHook;
 // [ts] },{"25":25,"30":30,"31":31,"7":7}],7:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] /**
 // [ts] * Keeps track of the current owner.
 // [ts] *
 // [ts] * The current owner is the component who should own any components that are
 // [ts] * currently being constructed.
 // [ts] */
 // [ts] let ReactCurrentOwner = {
 // [ts] /**
 // [ts] * @internal
 // [ts] * @type {ReactComponent}
 // [ts] */
 // [ts] current: null
 // [ts] };
 // [ts] module.exports = ReactCurrentOwner;
 // [ts] },{}],8:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let ReactElement = _dereq_(9);
 // [ts] /**
 // [ts] * Create a factory that creates HTML tag elements.
 // [ts] *
 // [ts] * @private
 // [ts] */
 // [ts] let createDOMFactory = ReactElement.createFactory;
 // [ts] if ("development" != "production") {
 // [ts] let ReactElementValidator = _dereq_(11);
 // [ts] createDOMFactory = ReactElementValidator.createFactory;
 // [ts] }
 // [ts] /**
 // [ts] * Creates a mapping from supported HTML tags to `ReactDOMComponent` classes.
 // [ts] *
 // [ts] * @public
 // [ts] */
 // [ts] let ReactDOMFactories = {
 // [ts] a: createDOMFactory("a"),
 // [ts] abbr: createDOMFactory("abbr"),
 // [ts] address: createDOMFactory("address"),
 // [ts] area: createDOMFactory("area"),
 // [ts] article: createDOMFactory("article"),
 // [ts] aside: createDOMFactory("aside"),
 // [ts] audio: createDOMFactory("audio"),
 // [ts] b: createDOMFactory("b"),
 // [ts] base: createDOMFactory("base"),
 // [ts] bdi: createDOMFactory("bdi"),
 // [ts] bdo: createDOMFactory("bdo"),
 // [ts] big: createDOMFactory("big"),
 // [ts] blockquote: createDOMFactory("blockquote"),
 // [ts] body: createDOMFactory("body"),
 // [ts] br: createDOMFactory("br"),
 // [ts] button: createDOMFactory("button"),
 // [ts] canvas: createDOMFactory("canvas"),
 // [ts] caption: createDOMFactory("caption"),
 // [ts] cite: createDOMFactory("cite"),
 // [ts] code: createDOMFactory("code"),
 // [ts] col: createDOMFactory("col"),
 // [ts] colgroup: createDOMFactory("colgroup"),
 // [ts] data: createDOMFactory("data"),
 // [ts] datalist: createDOMFactory("datalist"),
 // [ts] dd: createDOMFactory("dd"),
 // [ts] del: createDOMFactory("del"),
 // [ts] details: createDOMFactory("details"),
 // [ts] dfn: createDOMFactory("dfn"),
 // [ts] dialog: createDOMFactory("dialog"),
 // [ts] div: createDOMFactory("div"),
 // [ts] dl: createDOMFactory("dl"),
 // [ts] dt: createDOMFactory("dt"),
 // [ts] em: createDOMFactory("em"),
 // [ts] embed: createDOMFactory("embed"),
 // [ts] fieldset: createDOMFactory("fieldset"),
 // [ts] figcaption: createDOMFactory("figcaption"),
 // [ts] figure: createDOMFactory("figure"),
 // [ts] footer: createDOMFactory("footer"),
 // [ts] form: createDOMFactory("form"),
 // [ts] h1: createDOMFactory("h1"),
 // [ts] h2: createDOMFactory("h2"),
 // [ts] h3: createDOMFactory("h3"),
 // [ts] h4: createDOMFactory("h4"),
 // [ts] h5: createDOMFactory("h5"),
 // [ts] h6: createDOMFactory("h6"),
 // [ts] head: createDOMFactory("head"),
 // [ts] header: createDOMFactory("header"),
 // [ts] hgroup: createDOMFactory("hgroup"),
 // [ts] hr: createDOMFactory("hr"),
 // [ts] html: createDOMFactory("html"),
 // [ts] i: createDOMFactory("i"),
 // [ts] iframe: createDOMFactory("iframe"),
 // [ts] img: createDOMFactory("img"),
 // [ts] input: createDOMFactory("input"),
 // [ts] ins: createDOMFactory("ins"),
 // [ts] kbd: createDOMFactory("kbd"),
 // [ts] keygen: createDOMFactory("keygen"),
 // [ts] label: createDOMFactory("label"),
 // [ts] legend: createDOMFactory("legend"),
 // [ts] li: createDOMFactory("li"),
 // [ts] link: createDOMFactory("link"),
 // [ts] main: createDOMFactory("main"),
 // [ts] map: createDOMFactory("map"),
 // [ts] mark: createDOMFactory("mark"),
 // [ts] menu: createDOMFactory("menu"),
 // [ts] menuitem: createDOMFactory("menuitem"),
 // [ts] meta: createDOMFactory("meta"),
 // [ts] meter: createDOMFactory("meter"),
 // [ts] nav: createDOMFactory("nav"),
 // [ts] noscript: createDOMFactory("noscript"),
 // [ts] object: createDOMFactory("object"),
 // [ts] ol: createDOMFactory("ol"),
 // [ts] optgroup: createDOMFactory("optgroup"),
 // [ts] option: createDOMFactory("option"),
 // [ts] output: createDOMFactory("output"),
 // [ts] p: createDOMFactory("p"),
 // [ts] param: createDOMFactory("param"),
 // [ts] picture: createDOMFactory("picture"),
 // [ts] pre: createDOMFactory("pre"),
 // [ts] progress: createDOMFactory("progress"),
 // [ts] q: createDOMFactory("q"),
 // [ts] rp: createDOMFactory("rp"),
 // [ts] rt: createDOMFactory("rt"),
 // [ts] ruby: createDOMFactory("ruby"),
 // [ts] s: createDOMFactory("s"),
 // [ts] samp: createDOMFactory("samp"),
 // [ts] script: createDOMFactory("script"),
 // [ts] section: createDOMFactory("section"),
 // [ts] select: createDOMFactory("select"),
 // [ts] small: createDOMFactory("small"),
 // [ts] source: createDOMFactory("source"),
 // [ts] span: createDOMFactory("span"),
 // [ts] strong: createDOMFactory("strong"),
 // [ts] style: createDOMFactory("style"),
 // [ts] sub: createDOMFactory("sub"),
 // [ts] summary: createDOMFactory("summary"),
 // [ts] sup: createDOMFactory("sup"),
 // [ts] table: createDOMFactory("table"),
 // [ts] tbody: createDOMFactory("tbody"),
 // [ts] td: createDOMFactory("td"),
 // [ts] textarea: createDOMFactory("textarea"),
 // [ts] tfoot: createDOMFactory("tfoot"),
 // [ts] th: createDOMFactory("th"),
 // [ts] thead: createDOMFactory("thead"),
 // [ts] time: createDOMFactory("time"),
 // [ts] title: createDOMFactory("title"),
 // [ts] tr: createDOMFactory("tr"),
 // [ts] track: createDOMFactory("track"),
 // [ts] u: createDOMFactory("u"),
 // [ts] ul: createDOMFactory("ul"),
 // [ts] "var": createDOMFactory("var"),
 // [ts] video: createDOMFactory("video"),
 // [ts] wbr: createDOMFactory("wbr"),
 // SVG
 // [ts] circle: createDOMFactory("circle"),
 // [ts] clipPath: createDOMFactory("clipPath"),
 // [ts] defs: createDOMFactory("defs"),
 // [ts] ellipse: createDOMFactory("ellipse"),
 // [ts] g: createDOMFactory("g"),
 // [ts] image: createDOMFactory("image"),
 // [ts] line: createDOMFactory("line"),
 // [ts] linearGradient: createDOMFactory("linearGradient"),
 // [ts] mask: createDOMFactory("mask"),
 // [ts] path: createDOMFactory("path"),
 // [ts] pattern: createDOMFactory("pattern"),
 // [ts] polygon: createDOMFactory("polygon"),
 // [ts] polyline: createDOMFactory("polyline"),
 // [ts] radialGradient: createDOMFactory("radialGradient"),
 // [ts] rect: createDOMFactory("rect"),
 // [ts] stop: createDOMFactory("stop"),
 // [ts] svg: createDOMFactory("svg"),
 // [ts] text: createDOMFactory("text"),
 // [ts] tspan: createDOMFactory("tspan")
 // [ts] };
 // [ts] module.exports = ReactDOMFactories;
 // [ts] },{"11":11,"9":9}],9:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2014-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _assign = _dereq_(32);
 // [ts] let ReactCurrentOwner = _dereq_(7);
 // [ts] let warning = _dereq_(31);
 // [ts] let canDefineProperty = _dereq_(18);
 // [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
 // [ts] let REACT_ELEMENT_TYPE = _dereq_(10);
 // [ts] let RESERVED_PROPS = {
 // [ts] key: true,
 // [ts] ref: true,
 // [ts] __self: true,
 // [ts] __source: true
 // [ts] };
 // [ts] let specialPropKeyWarningShown, specialPropRefWarningShown;
 // [ts] fn hasValidRef(config: auto) {
 // [ts] if ("development" != "production") {
 // [ts] if (hasOwnProperty.call(config, "ref")) {
 // [ts] let getter = Object.getOwnPropertyDescriptor(config, "ref").get;
 // [ts] if (getter && getter.isReactWarning) {
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] return config.ref != null;
 // [ts] }
 // [ts] fn hasValidKey(config: auto) {
 // [ts] if ("development" != "production") {
 // [ts] if (hasOwnProperty.call(config, "key")) {
 // [ts] let getter = Object.getOwnPropertyDescriptor(config, "key").get;
 // [ts] if (getter && getter.isReactWarning) {
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] return config.key != null;
 // [ts] }
 // [ts] fn defineKeyPropWarningGetter(props: auto, displayName: auto) {
 // [ts] let warnAboutAccessingKey = () => {
 // [ts] if (!specialPropKeyWarningShown) {
 // [ts] specialPropKeyWarningShown = true;
 // [ts] if ("development" != "production") warning(false, "%s: `key` is not a prop. Trying to access it will result " + "in `undefined` being returned. If you need to access the same " + "value within the child component, you should pass it as a different " + "prop. (https://fb.me/react-special-props)", displayName) else void 0;
 // [ts] }
 // [ts] };
 // [ts] warnAboutAccessingKey.isReactWarning = true;
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(props, 'key', {
 // [ts-skip] get: warnAboutAccessingKey,
 // [ts-skip] configurable: true
 // [ts-skip] });
 // [ts] }
 // [ts] fn defineRefPropWarningGetter(props: auto, displayName: auto) {
 // [ts] let warnAboutAccessingRef = () => {
 // [ts] if (!specialPropRefWarningShown) {
 // [ts] specialPropRefWarningShown = true;
 // [ts] if ("development" != "production") warning(false, "%s: `ref` is not a prop. Trying to access it will result " + "in `undefined` being returned. If you need to access the same " + "value within the child component, you should pass it as a different " + "prop. (https://fb.me/react-special-props)", displayName) else void 0;
 // [ts] }
 // [ts] };
 // [ts] warnAboutAccessingRef.isReactWarning = true;
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(props, 'ref', {
 // [ts-skip] get: warnAboutAccessingRef,
 // [ts-skip] configurable: true
 // [ts-skip] });
 // [ts] }
 // [ts] /**
 // [ts] * Factory method to create a new React element. This no longer adheres to
 // [ts] * the class pattern, so do not use new to call it. Also, no instanceof check
 // [ts] * will work. Instead test $$typeof field against Symbol.for('react.element') to check
 // [ts] * if something is a React Element.
 // [ts] *
 // [ts] * @param {*} type
 // [ts] * @param {*} key
 // [ts] * @param {string|object} ref
 // [ts] * @param {*} self A *temporary* helper to detect places where `this` is
 // [ts] * different from the `owner` when React.createElement is called, so that we
 // [ts] * can warn. We want to get rid of owner and replace string `ref`s with arrow
 // [ts] * functions, and as long as `this` and owner are the same, there will be no
 // [ts] * change in behavior.
 // [ts] * @param {*} source An annotation object (added by a transpiler or otherwise)
 // [ts] * indicating filename, line number, and/or other information.
 // [ts] * @param {*} owner
 // [ts] * @param {*} props
 // [ts] * @internal
 // [ts] */
 // [ts] let ReactElement = (type, key, ref, self, source, owner, props) => {
 // [ts] let element = {
 // This tag allow us to uniquely identify this as a React Element
 // [ts] $$typeof: REACT_ELEMENT_TYPE,
 // Built-in properties that belong on the element
 // [ts] type: type,
 // [ts] key: key,
 // [ts] ref: ref,
 // [ts] props: props,
 // Record the component responsible for creating this element.
 // [ts] _owner: owner
 // [ts] };
 // [ts] if ("development" != "production") {
 // The validation flag is currently mutative. We put it on
 // an external backing store so that we can freeze the whole object.
 // This can be replaced with a WeakMap once they are implemented in
 // commonly used development environments.
 // [ts] element._store = {};
 // To make comparing ReactElements easier for testing purposes, we make
 // the validation flag non-enumerable (where possible, which should
 // include every environment we run tests in), so the test framework
 // ignores it.
 // [ts] if (canDefineProperty) {
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(element._store, 'validated', {
 // [ts-skip] configurable: false,
 // [ts-skip] enumerable: false,
 // [ts-skip] writable: true,
 // [ts-skip] value: false
 // [ts-skip] });
 // self and source are DEV only properties.
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(element, '_self', {
 // [ts-skip] configurable: false,
 // [ts-skip] enumerable: false,
 // [ts-skip] writable: false,
 // [ts-skip] value: self
 // [ts-skip] });
 // Two elements created in two different places should be considered
 // equal for testing purposes and therefore we hide it from enumeration.
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(element, '_source', {
 // [ts-skip] configurable: false,
 // [ts-skip] enumerable: false,
 // [ts-skip] writable: false,
 // [ts-skip] value: source
 // [ts-skip] });
 // [ts] } else {
 // [ts] element._store.validated = false;
 // [ts] element._self = self;
 // [ts] element._source = source;
 // [ts] }
 // [ts] if (Object.freeze) {
 // [ts] Object.freeze(element.props);
 // [ts] Object.freeze(element);
 // [ts] }
 // [ts] }
 // [ts] return element;
 // [ts] };
 // [ts] /**
 // [ts] * Create and return a new ReactElement of the given type.
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.createelement
 // [ts] */
 // [ts] ReactElement.createElement = (type, config, children) => {
 // [ts] let propName;
 // Reserved names are extracted
 // [ts] let props = {};
 // [ts] let key = null;
 // [ts] let ref = null;
 // [ts] let self = null;
 // [ts] let source = null;
 // [ts] if (config != null) {
 // [ts] if (hasValidRef(config)) {
 // [ts] ref = config.ref;
 // [ts] }
 // [ts] if (hasValidKey(config)) {
 // [ts] key = "" + config.key;
 // [ts] }
 // [ts] self = if (config.__self == null) null else config.__self;
 // [ts] source = if (config.__source == null) null else config.__source;
 // Remaining properties are added to a new props object
 // [ts] for (propName in config) {
 // [ts] if (hasOwnProperty.call(config, propName) && !RESERVED_PROPS.hasOwnProperty(propName)) {
 // [ts] props[propName] = config[propName];
 // [ts] }
 // [ts] }
 // [ts] }
 // Children can be more than one argument, and those are transferred onto
 // the newly allocated props object.
 // [ts] let childrenLength = arguments.length - 2;
 // [ts] if (childrenLength == 1) {
 // [ts] props.children = children;
 // [ts] } else if (childrenLength > 1) {
 // [ts] let childArray = Array(childrenLength);
 // [ts] let i = 0;
 // [ts] while (i < childrenLength) { // [cap-migrate] for-step: i++
 // [ts] childArray[i] = arguments[i + 2];
 // [ts] }
 // [ts] if ("development" != "production") {
 // [ts] if (Object.freeze) {
 // [ts] Object.freeze(childArray);
 // [ts] }
 // [ts] }
 // [ts] props.children = childArray;
 // [ts] }
 // Resolve default props
 // [ts] if (type && type.defaultProps) {
 // [ts] let defaultProps = type.defaultProps;
 // [ts] for (propName in defaultProps) {
 // [ts] if (props[propName] == null) {
 // [ts] props[propName] = defaultProps[propName];
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] if ("development" != "production") {
 // [ts] if (key || ref) {
 // [ts] if (@typeName(props.$$typeof) == "undefined" || props.$$typeof != REACT_ELEMENT_TYPE) {
 // [ts] let displayName = if (@typeName(type) == "function") type.displayName || type.name || "Unknown" else type;
 // [ts] if (key) {
 // [ts] defineKeyPropWarningGetter(props, displayName);
 // [ts] }
 // [ts] if (ref) {
 // [ts] defineRefPropWarningGetter(props, displayName);
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] return ReactElement(type, key, ref, self, source, ReactCurrentOwner.current, props);
 // [ts] };
 // [ts] /**
 // [ts] * Return a function that produces ReactElements of a given type.
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.createfactory
 // [ts] */
 // [ts] ReactElement.createFactory = (type) => {
 // [ts] let factory = ReactElement.createElement.bind(null, type);
 // Expose the type on the factory and the prototype so that it can be
 // easily accessed on elements. E.g. `<Foo />.type === Foo`.
 // This should not be named `constructor` since this may not be the function
 // that created the element, and it may not even be a constructor.
 // Legacy hook TODO: Warn if this is accessed
 // [ts] factory.type = type;
 // [ts] return factory;
 // [ts] };
 // [ts] ReactElement.cloneAndReplaceKey = (oldElement, newKey) => {
 // [ts] let newElement = ReactElement(oldElement.type, newKey, oldElement.ref, oldElement._self, oldElement._source, oldElement._owner, oldElement.props);
 // [ts] return newElement;
 // [ts] };
 // [ts] /**
 // [ts] * Clone and return a new ReactElement using element as the starting point.
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.cloneelement
 // [ts] */
 // [ts] ReactElement.cloneElement = (element, config, children) => {
 // [ts] let propName;
 // Original props are copied
 // [ts] let props = _assign({}, element.props);
 // Reserved names are extracted
 // [ts] let key = element.key;
 // [ts] let ref = element.ref;
 // Self is preserved since the owner is preserved.
 // [ts] let self = element._self;
 // Source is preserved since cloneElement is unlikely to be targeted by a
 // transpiler, and the original source is probably a better indicator of the
 // true owner.
 // [ts] let source = element._source;
 // Owner will be preserved, unless ref is overridden
 // [ts] let owner = element._owner;
 // [ts] if (config != null) {
 // [ts] if (hasValidRef(config)) {
 // Silently steal the ref from the parent.
 // [ts] ref = config.ref;
 // [ts] owner = ReactCurrentOwner.current;
 // [ts] }
 // [ts] if (hasValidKey(config)) {
 // [ts] key = "" + config.key;
 // [ts] }
 // Remaining properties override existing props
 // [ts] let defaultProps;
 // [ts] if (element.type && element.type.defaultProps) {
 // [ts] defaultProps = element.type.defaultProps;
 // [ts] }
 // [ts] for (propName in config) {
 // [ts] if (hasOwnProperty.call(config, propName) && !RESERVED_PROPS.hasOwnProperty(propName)) {
 // [ts] if (config[propName] == null && defaultProps != null) {
 // Resolve default props
 // [ts] props[propName] = defaultProps[propName];
 // [ts] } else {
 // [ts] props[propName] = config[propName];
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // Children can be more than one argument, and those are transferred onto
 // the newly allocated props object.
 // [ts] let childrenLength = arguments.length - 2;
 // [ts] if (childrenLength == 1) {
 // [ts] props.children = children;
 // [ts] } else if (childrenLength > 1) {
 // [ts] let childArray = Array(childrenLength);
 // [ts] let i = 0;
 // [ts] while (i < childrenLength) { // [cap-migrate] for-step: i++
 // [ts] childArray[i] = arguments[i + 2];
 // [ts] }
 // [ts] props.children = childArray;
 // [ts] }
 // [ts] return ReactElement(element.type, key, ref, self, source, owner, props);
 // [ts] };
 // [ts] /**
 // [ts] * Verifies the object is a ReactElement.
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.isvalidelement
 // [ts] * @param {?object} object
 // [ts] * @return {boolean} True if `object` is a valid component.
 // [ts] * @final
 // [ts] */
 // [ts] ReactElement.isValidElement = (object) => {
 // [ts] return @typeName(object) == "object" && object != null && object.$$typeof == REACT_ELEMENT_TYPE;
 // [ts] };
 // [ts] module.exports = ReactElement;
 // [ts] },{"10":10,"18":18,"31":31,"32":32,"7":7}],10:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2014-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // The Symbol used to tag the ReactElement type. If there is no native Symbol
 // nor polyfill, then a plain number is used for performance.
 // [ts] let REACT_ELEMENT_TYPE = @typeName(Symbol) == "function" && Symbol["for"] && Symbol["for"]("react.element") || 0xeac7;
 // [ts] module.exports = REACT_ELEMENT_TYPE;
 // [ts] },{}],11:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2014-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] /**
 // [ts] * ReactElementValidator provides a wrapper around a element factory
 // [ts] * which validates the props passed to the element. This is intended to be
 // [ts] * used only in DEV and could be replaced by a static type checker for languages
 // [ts] * that support it.
 // [ts] */
 // [ts] "use strict";
 // [ts] let ReactCurrentOwner = _dereq_(7);
 // [ts] let ReactComponentTreeHook = _dereq_(6);
 // [ts] let ReactElement = _dereq_(9);
 // [ts] let checkReactTypeSpec = _dereq_(19);
 // [ts] let canDefineProperty = _dereq_(18);
 // [ts] let getIteratorFn = _dereq_(21);
 // [ts] let warning = _dereq_(31);
 // [ts] let lowPriorityWarning = _dereq_(23);
 // [ts] fn getDeclarationErrorAddendum() {
 // [ts] if (ReactCurrentOwner.current) {
 // [ts] let name = ReactCurrentOwner.current.getName();
 // [ts] if (name) {
 // [ts] return " Check the render method of `" + name + "`.";
 // [ts] }
 // [ts] }
 // [ts] return "";
 // [ts] }
 // [ts] fn getSourceInfoErrorAddendum(elementProps: auto) {
 // [ts] if (elementProps != null && elementProps != null && elementProps.__source != null) {
 // [ts] let source = elementProps.__source;
 // [ts] let fileName = source.fileName.replace("^.*[\\\\\\/]", "");
 // [ts] let lineNumber = source.lineNumber;
 // [ts] return " Check your code at " + fileName + ":" + lineNumber + ".";
 // [ts] }
 // [ts] return "";
 // [ts] }
 // [ts] /**
 // [ts] * Warn if there's no key explicitly set on dynamic arrays of children or
 // [ts] * object keys are not valid. This allows us to keep track of children between
 // [ts] * updates.
 // [ts] */
 // [ts] let ownerHasKeyUseWarning = {};
 // [ts] fn getCurrentComponentErrorInfo(parentType: auto) {
 // [ts] let info = getDeclarationErrorAddendum();
 // [ts] if (!info) {
 // [ts] let parentName = if (@typeName(parentType) == "string") parentType else parentType.displayName || parentType.name;
 // [ts] if (parentName) {
 // [ts] info = " Check the top-level render call using <" + parentName + ">.";
 // [ts] }
 // [ts] }
 // [ts] return info;
 // [ts] }
 // [ts] /**
 // [ts] * Warn if the element doesn't have an explicit key assigned to it.
 // [ts] * This element is in an array. The array could grow and shrink or be
 // [ts] * reordered. All children that haven't already been validated are required to
 // [ts] * have a "key" property assigned to it. Error statuses are cached so a warning
 // [ts] * will only be shown once.
 // [ts] *
 // [ts] * @internal
 // [ts] * @param {ReactElement} element Element that requires a key.
 // [ts] * @param {*} parentType element's parent's type.
 // [ts] */
 // [ts] fn validateExplicitKey(element: auto, parentType: auto) {
 // [ts] if (!element._store || element._store.validated || element.key != null) {
 // [ts] return;
 // [ts] }
 // [ts] element._store.validated = true;
 // [ts] let memoizer = ownerHasKeyUseWarning.uniqueKey || (ownerHasKeyUseWarning.uniqueKey = {});
 // [ts] let currentComponentErrorInfo = getCurrentComponentErrorInfo(parentType);
 // [ts] if (memoizer[currentComponentErrorInfo]) {
 // [ts] return;
 // [ts] }
 // [ts] memoizer[currentComponentErrorInfo] = true;
 // Usually the current owner is the offender, but if it accepts children as a
 // property, it may be the creator of the child that's responsible for
 // assigning it a key.
 // [ts] let childOwner = "";
 // [ts] if (element && element._owner && element._owner != ReactCurrentOwner.current) {
 // Give the component that originally created this child.
 // [ts] childOwner = " It was passed a child from " + element._owner.getName() + ".";
 // [ts] }
 // [ts] if ("development" != "production") warning(false, "Each child in an array or iterator should have a unique \"key\" prop." + "%s%s See https://fb.me/react-warning-keys for more information.%s", currentComponentErrorInfo, childOwner, ReactComponentTreeHook.getCurrentStackAddendum(element)) else void 0;
 // [ts] }
 // [ts] /**
 // [ts] * Ensure that every element either is passed in a static location, in an
 // [ts] * array with an explicit keys property defined, or in an object literal
 // [ts] * with valid key property.
 // [ts] *
 // [ts] * @internal
 // [ts] * @param {ReactNode} node Statically passed child of any type.
 // [ts] * @param {*} parentType node's parent's type.
 // [ts] */
 // [ts] fn validateChildKeys(node: auto, parentType: auto) {
 // [ts] if (@typeName(node) != "object") {
 // [ts] return;
 // [ts] }
 // [ts] if (Array.isArray(node)) {
 // [ts] let i = 0;
 // [ts] while (i < node.length) { // [cap-migrate] for-step: i++
 // [ts] let child = node[i];
 // [ts] if (ReactElement.isValidElement(child)) {
 // [ts] validateExplicitKey(child, parentType);
 // [ts] }
 // [ts] }
 // [ts] } else if (ReactElement.isValidElement(node)) {
 // This element was passed in a valid location.
 // [ts] if (node._store) {
 // [ts] node._store.validated = true;
 // [ts] }
 // [ts] } else if (node) {
 // [ts] let iteratorFn = getIteratorFn(node);
 // Entry iterators provide implicit keys.
 // [ts] if (iteratorFn) {
 // [ts] if (iteratorFn != node.entries) {
 // [ts] let iterator = iteratorFn.call(node);
 // [ts] let step;
 // [ts] while (!(step = iterator.next()).done) {
 // [ts] if (ReactElement.isValidElement(step.value)) {
 // [ts] validateExplicitKey(step.value, parentType);
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] /**
 // [ts] * Given an element, validate that its props follow the propTypes definition,
 // [ts] * provided by the type.
 // [ts] *
 // [ts] * @param {ReactElement} element
 // [ts] */
 // [ts] fn validatePropTypes(element: auto) {
 // [ts] let componentClass = element.type;
 // [ts] if (@typeName(componentClass) != "function") {
 // [ts] return;
 // [ts] }
 // [ts] let name = componentClass.displayName || componentClass.name;
 // [ts] if (componentClass.propTypes) {
 // [ts] checkReactTypeSpec(componentClass.propTypes, element.props, "prop", name, element, null);
 // [ts] }
 // [ts] if (@typeName(componentClass.getDefaultProps) == "function") {
 // [ts] if ("development" != "production") warning(componentClass.getDefaultProps.isReactClassApproved, "getDefaultProps is only used on classic React.createClass " + "definitions. Use a static property named `defaultProps` instead.") else void 0;
 // [ts] }
 // [ts] }
 // [ts] let ReactElementValidator = {
 // [ts] createElement: (type, props, children) => {
 // [ts] let validType = @typeName(type) == "string" || @typeName(type) == "function";
 // We warn in this case but don't throw. We expect the element creation to
 // succeed and there will likely be errors in render.
 // [ts] if (!validType) {
 // [ts] if (@typeName(type) != "function" && @typeName(type) != "string") {
 // [ts] let info = "";
 // [ts] if (type == null || @typeName(type) == "object" && type != null && Object.keys(type).length == 0) {
 // [ts] info += " You likely forgot to export your component from the file " + "it's defined in.";
 // [ts] }
 // [ts] let sourceInfo = getSourceInfoErrorAddendum(props);
 // [ts] if (sourceInfo) {
 // [ts] info += sourceInfo;
 // [ts] } else {
 // [ts] info += getDeclarationErrorAddendum();
 // [ts] }
 // [ts] info += ReactComponentTreeHook.getCurrentStackAddendum();
 // [ts] let currentSource = if (props != null && props != null && props.__source != null) props.__source else null;
 // [ts] ReactComponentTreeHook.pushNonStandardWarningStack(true, currentSource);
 // [ts] if ("development" != "production") warning(if (false, "React.createElement: type is invalid -- expected a string (for " + "built-in components) or a class/function (for composite " + "components) but got: %s.%s", type == null) type else @typeName(type), info) else void 0;
 // [ts] ReactComponentTreeHook.popNonStandardWarningStack();
 // [ts] }
 // [ts] }
 // [ts] let element = ReactElement.createElement.apply(this, arguments);
 // The result can be nullish if a mock or a custom function is used.
 // TODO: Drop this when these are no longer allowed as the type argument.
 // [ts] if (element == null) {
 // [ts] return element;
 // [ts] }
 // Skip key warning if the type isn't valid since our key validation logic
 // doesn't expect a non-string/function type and can throw confusing errors.
 // We don't want exception behavior to differ between dev and prod.
 // (Rendering will throw with a helpful message and as soon as the type is
 // fixed, the key warnings will appear.)
 // [ts] if (validType) {
 // [ts] let i = 2;
 // [ts] while (i < arguments.length) { // [cap-migrate] for-step: i++
 // [ts] validateChildKeys(arguments[i], type);
 // [ts] }
 // [ts] }
 // [ts] validatePropTypes(element);
 // [ts] return element;
 // [ts] },
 // [ts] createFactory: (type) => {
 // [ts] let validatedFactory = ReactElementValidator.createElement.bind(null, type);
 // Legacy hook TODO: Warn if this is accessed
 // [ts] validatedFactory.type = type;
 // [ts] if ("development" != "production") {
 // [ts] if (canDefineProperty) {
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(validatedFactory, 'type', {
 // [ts-skip] enumerable: false,
 // [ts-skip] get: function () {
 // [ts-skip] lowPriorityWarning(false, 'Factory.type is deprecated. Access the class directly ' + 'before passing it to createFactory.');
 // [ts-skip] Object.defineProperty(this, 'type', {
 // [ts-skip] value: type
 // [ts-skip] });
 // [ts-skip] return type;
 // [ts-skip] }
 // [ts-skip] });
 // [ts] }
 // [ts] }
 // [ts] return validatedFactory;
 // [ts] },
 // [ts] cloneElement: (element, props, children) => {
 // [ts] let newElement = ReactElement.cloneElement.apply(this, arguments);
 // [ts] let i = 2;
 // [ts] while (i < arguments.length) { // [cap-migrate] for-step: i++
 // [ts] validateChildKeys(arguments[i], newElement.type);
 // [ts] }
 // [ts] validatePropTypes(newElement);
 // [ts] return newElement;
 // [ts] }
 // [ts] };
 // [ts] module.exports = ReactElementValidator;
 // [ts] },{"18":18,"19":19,"21":21,"23":23,"31":31,"6":6,"7":7,"9":9}],12:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2015-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let warning = _dereq_(31);
 // [ts] fn warnNoop(publicInstance: auto, callerName: auto) {
 // [ts] if ("development" != "production") {
 // [ts] let constructor = publicInstance.constructor;
 // [ts] if ("development" != "production") warning(false, "%s(...): Can only update a mounted or mounting component. " + "This usually means you called %s() on an unmounted component. " + "This is a no-op. Please check the code for the %s component.", callerName, callerName, constructor && (constructor.displayName || constructor.name) || "ReactClass") else void 0;
 // [ts] }
 // [ts] }
 // [ts] /**
 // [ts] * This is the abstract API for an update queue.
 // [ts] */
 // [ts] let ReactNoopUpdateQueue = {
 // [ts] /**
 // [ts] * Checks whether or not this composite component is mounted.
 // [ts] * @param {ReactClass} publicInstance The instance we want to test.
 // [ts] * @return {boolean} True if mounted, false otherwise.
 // [ts] * @protected
 // [ts] * @final
 // [ts] */
 // [ts] isMounted: (publicInstance) => {
 // [ts] return false;
 // [ts] },
 // [ts] /**
 // [ts] * Enqueue a callback that will be executed after all the pending updates
 // [ts] * have processed.
 // [ts] *
 // [ts] * @param {ReactClass} publicInstance The instance to use as `this` context.
 // [ts] * @param {?function} callback Called after state is updated.
 // [ts] * @internal
 // [ts] */
 // [ts] enqueueCallback: (publicInstance, callback) => {},
 // [ts] /**
 // [ts] * Forces an update. This should only be invoked when it is known with
 // [ts] * certainty that we are **not** in a DOM transaction.
 // [ts] *
 // [ts] * You may want to call this when you know that some deeper aspect of the
 // [ts] * component's state has changed but `setState` was not called.
 // [ts] *
 // [ts] * This will not invoke `shouldComponentUpdate`, but it will invoke
 // [ts] * `componentWillUpdate` and `componentDidUpdate`.
 // [ts] *
 // [ts] * @param {ReactClass} publicInstance The instance that should rerender.
 // [ts] * @internal
 // [ts] */
 // [ts] enqueueForceUpdate: (publicInstance) => {
 // [ts] warnNoop(publicInstance, "forceUpdate");
 // [ts] },
 // [ts] /**
 // [ts] * Replaces all of the state. Always use this or `setState` to mutate state.
 // [ts] * You should treat `this.state` as immutable.
 // [ts] *
 // [ts] * There is no guarantee that `this.state` will be immediately updated, so
 // [ts] * accessing `this.state` after calling this method may return the old value.
 // [ts] *
 // [ts] * @param {ReactClass} publicInstance The instance that should rerender.
 // [ts] * @param {object} completeState Next state.
 // [ts] * @internal
 // [ts] */
 // [ts] enqueueReplaceState: (publicInstance, completeState) => {
 // [ts] warnNoop(publicInstance, "replaceState");
 // [ts] },
 // [ts] /**
 // [ts] * Sets a subset of the state. This only exists because _pendingState is
 // [ts] * internal. This provides a merging strategy that is not available to deep
 // [ts] * properties which is confusing. TODO: Expose pendingState or don't use it
 // [ts] * during the merge.
 // [ts] *
 // [ts] * @param {ReactClass} publicInstance The instance that should rerender.
 // [ts] * @param {object} partialState Next partial state to be merged with state.
 // [ts] * @internal
 // [ts] */
 // [ts] enqueueSetState: (publicInstance, partialState) => {
 // [ts] warnNoop(publicInstance, "setState");
 // [ts] }
 // [ts] };
 // [ts] module.exports = ReactNoopUpdateQueue;
 // [ts] },{"31":31}],13:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let ReactPropTypeLocationNames = {};
 // [ts] if ("development" != "production") {
 // [ts] ReactPropTypeLocationNames = {
 // [ts] prop: "prop",
 // [ts] context: "context",
 // [ts] childContext: "child context"
 // [ts] };
 // [ts] }
 // [ts] module.exports = ReactPropTypeLocationNames;
 // [ts] },{}],14:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _require = _dereq_(9),
 // [ts] isValidElement = _require.isValidElement;
 // [ts] let factory = _dereq_(34);
 // [ts] module.exports = factory(isValidElement);
 // [ts] },{"34":34,"9":9}],15:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let ReactPropTypesSecret = "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED";
 // [ts] module.exports = ReactPropTypesSecret;
 // [ts] },{}],16:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _assign = _dereq_(32);
 // [ts] let React = _dereq_(3);
 // `version` will be added here by the React module.
 // [ts] let ReactUMDEntry = _assign(React, {
 // [ts] __SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED: {
 // [ts] ReactCurrentOwner: _dereq_(7)
 // [ts] }
 // [ts] });
 // [ts] if ("development" != "production") {
 // [ts] _assign(ReactUMDEntry.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED, {
 // ReactComponentTreeHook should not be included in production.
 // [ts] ReactComponentTreeHook: _dereq_(6),
 // [ts] getNextDebugID: _dereq_(22)
 // [ts] });
 // [ts] }
 // [ts] module.exports = ReactUMDEntry;
 // [ts] },{"22":22,"3":3,"32":32,"6":6,"7":7}],17:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] module.exports = "15.7.0";
 // [ts] },{}],18:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let canDefineProperty = false;
 // [ts] if ("development" != "production") {
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // $FlowFixMe https://github.com/facebook/flow/issues/285
 // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty({}, 'x', { get: function () {} });
 // [ts] canDefineProperty = true;
 // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {)
 // IE will fail on defineProperty
 // [ts] }
 // [ts] }
 // [ts] module.exports = canDefineProperty;
 // [ts] },{}],19:[(_dereq_, module, exports) => {
 // [ts] ((process) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _prodInvariant = _dereq_(25);
 // [ts] let ReactPropTypeLocationNames = _dereq_(13);
 // [ts] let ReactPropTypesSecret = _dereq_(15);
 // [ts] let invariant = _dereq_(30);
 // [ts] let warning = _dereq_(31);
 // [ts] let ReactComponentTreeHook;
 // [ts] if (@typeName(process) != "undefined" && process.env && "development" == "test") {
 // Temporary hack.
 // Inline requires don't work well with Jest:
 // https://github.com/facebook/react/issues/7240
 // Remove the inline requires when we don't need them anymore:
 // https://github.com/facebook/react/pull/7178
 // [ts] ReactComponentTreeHook = _dereq_(6);
 // [ts] }
 // [ts] let loggedTypeFailures = {};
 // [ts] /**
 // [ts] * Assert that the values match with the type specs.
 // [ts] * Error messages are memorized and will only be shown once.
 // [ts] *
 // [ts] * @param {object} typeSpecs Map of name to a ReactPropType
 // [ts] * @param {object} values Runtime values that need to be type-checked
 // [ts] * @param {string} location e.g. "prop", "context", "child context"
 // [ts] * @param {string} componentName Name of the component for error messages.
 // [ts] * @param {?object} element The React element that is being type-checked
 // [ts] * @param {?number} debugID The React component instance that is being type-checked
 // [ts] * @private
 // [ts] */
 // [ts] fn checkReactTypeSpec(typeSpecs: auto, values: auto, location: auto, componentName: auto, element: auto, debugID: auto) {
 // [ts] for (let typeSpecName in typeSpecs) {
 // [ts] if (typeSpecs.hasOwnProperty(typeSpecName)) {
 // [ts] let error;
 // Prop type validation may throw. In case they do, we don't want to
 // fail the render phase where it didn't fail before. So we log it.
 // After these have been cleaned up, we'll let them throw.
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // This is intentionally an invariant that gets caught. It's the same
 // behavior as without this statement except with a better message.
 // [ts] if (!(@typeName(typeSpecs[typeSpecName]) == "function")) if ("development" != "production") invariant(false, "%s: %s type `%s` is invalid; it must be a function, usually from React.PropTypes.", componentName || "React class", ReactPropTypeLocationNames[location], typeSpecName) else _prodInvariant("84", componentName || "React class", ReactPropTypeLocationNames[location], typeSpecName) else void 0;
 // [ts] error = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, ReactPropTypesSecret);
 // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
 // [ts] error = ex;
 // [ts] }
 // [ts] if ("development" != "production") warning(!error || error == Error, "%s: type specification of %s `%s` is invalid; the type checker " + "function must return `null` or an `Error` but returned a %s. " + "You may have forgotten to pass an argument to the type checker " + "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " + "shape all require an argument).", componentName || "React class", ReactPropTypeLocationNames[location], typeSpecName, @typeName(error)) else void 0;
 // [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
 // Only monitor this failure once because there tends to be a lot of the
 // same error.
 // [ts] loggedTypeFailures[error.message] = true;
 // [ts] let componentStackInfo = "";
 // [ts] if ("development" != "production") {
 // [ts] if (!ReactComponentTreeHook) {
 // [ts] ReactComponentTreeHook = _dereq_(6);
 // [ts] }
 // [ts] if (debugID != null) {
 // [ts] componentStackInfo = ReactComponentTreeHook.getStackAddendumByID(debugID);
 // [ts] } else if (element != null) {
 // [ts] componentStackInfo = ReactComponentTreeHook.getCurrentStackAddendum(element);
 // [ts] }
 // [ts] }
 // [ts] if ("development" != "production") warning(false, "Failed %s type: %s%s", location, error.message, componentStackInfo) else void 0;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] module.exports = checkReactTypeSpec;
 // [ts] }).call(this,null)
 // [ts] },{"13":13,"15":15,"25":25,"30":30,"31":31,"6":6}],20:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _require = _dereq_(4),
 // [ts] Component = _require.Component;
 // [ts] let _require2 = _dereq_(9),
 // [ts] isValidElement = _require2.isValidElement;
 // [ts] let ReactNoopUpdateQueue = _dereq_(12);
 // [ts] let factory = _dereq_(27);
 // [ts] module.exports = factory(Component, isValidElement, ReactNoopUpdateQueue);
 // [ts] },{"12":12,"27":27,"4":4,"9":9}],21:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] /* global Symbol */
 // [ts] let ITERATOR_SYMBOL = @typeName(Symbol) == "function" && "__symbol_iterator";
 // [ts] let FAUX_ITERATOR_SYMBOL = "@@iterator"; // Before Symbol spec.
 // [ts] /**
 // [ts] * Returns the iterator method function contained on the iterable object.
 // [ts] *
 // [ts] * Be sure to invoke the function with the iterable as context:
 // [ts] *
 // [ts] *     var iteratorFn = getIteratorFn(myIterable);
 // [ts] *     if (iteratorFn) {
 // [ts] *       var iterator = iteratorFn.call(myIterable);
 // [ts] *       ...
 // [ts] *     }
 // [ts] *
 // [ts] * @param {?object} maybeIterable
 // [ts] * @return {?function}
 // [ts] */
 // [ts] fn getIteratorFn(maybeIterable: auto) {
 // [ts] let iteratorFn = maybeIterable && (ITERATOR_SYMBOL && maybeIterable[ITERATOR_SYMBOL] || maybeIterable[FAUX_ITERATOR_SYMBOL]);
 // [ts] if (@typeName(iteratorFn) == "function") {
 // [ts] return iteratorFn;
 // [ts] }
 // [ts] }
 // [ts] module.exports = getIteratorFn;
 // [ts] },{}],22:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let nextDebugID = 1;
 // [ts] fn getNextDebugID() {
 // [ts] return nextDebugID++;
 // [ts] }
 // [ts] module.exports = getNextDebugID;
 // [ts] },{}],23:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2014-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] /**
 // [ts] * Forked from fbjs/warning:
 // [ts] * https://github.com/facebook/fbjs/blob/e66ba20ad5be433eb54423f2b097d829324d9de6/packages/fbjs/src/__forks__/warning.js
 // [ts] *
 // [ts] * Only change is we use console.warn instead of console.error,
 // [ts] * and do nothing when 'console' is not supported.
 // [ts] * This really simplifies the code.
 // [ts] * ---
 // [ts] * Similar to invariant but only logs a warning if the condition is not met.
 // [ts] * This can be used to log issues in development environments in critical
 // [ts] * paths. Removing the logging code for production environments will keep the
 // [ts] * same logic and follow the same code paths.
 // [ts] */
 // [ts] let lowPriorityWarning = () => {};
 // [ts] if ("development" != "production") {
 // [ts] let printWarning = (format) => {
 // [ts] let _len = arguments.length, args = Array(if (_len > 1) _len - 1 else 0), _key = 1;
 // [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
 // [ts] args[_key - 1] = arguments[_key];
 // [ts] }
 // [ts] let argIndex = 0;
 // [ts] let message = "Warning: " + format.replace("%s", () => {
 // [ts] return args[argIndex++];
 // [ts] });
 // [ts] if (@typeName(console) != "undefined") {
 // [ts] @print(message);
 // [ts] }
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // --- Welcome to debugging React ---
 // This error was thrown as a convenience so that you can use this stack
 // to find the callsite that caused this warning to fire.
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
 // [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
 // [ts] };
 // [ts] lowPriorityWarning = (condition, format) => {
 // [ts] if (format == null) {
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('`warning(condition, format, ...args)` requires a warning ' + 'message argument');)
 // [ts] }
 // [ts] if (!condition) {
 // [ts] let _len2 = arguments.length, args = Array(if (_len2 > 2) _len2 - 2 else 0), _key2 = 2;
 // [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
 // [ts] args[_key2 - 2] = arguments[_key2];
 // [ts] }
 // [ts] printWarning.apply(null, [format].concat(args));
 // [ts] }
 // [ts] };
 // [ts] }
 // [ts] module.exports = lowPriorityWarning;
 // [ts] },{}],24:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _prodInvariant = _dereq_(25);
 // [ts] let ReactElement = _dereq_(9);
 // [ts] let invariant = _dereq_(30);
 // [ts] /**
 // [ts] * Returns the first child in a collection of children and verifies that there
 // [ts] * is only one child in the collection.
 // [ts] *
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.children.only
 // [ts] *
 // [ts] * The current implementation of this function assumes that a single child gets
 // [ts] * passed without a wrapper, but the purpose of this helper function is to
 // [ts] * abstract away the particular structure of children.
 // [ts] *
 // [ts] * @param {?object} children Child collection structure.
 // [ts] * @return {ReactElement} The first and only `ReactElement` contained in the
 // [ts] * structure.
 // [ts] */
 // [ts] fn onlyChild(children: auto) {
 // [ts] if (!ReactElement.isValidElement(children)) if ("development" != "production") invariant(false, "React.Children.only expected to receive a single React element child.") else _prodInvariant("143") else void 0;
 // [ts] return children;
 // [ts] }
 // [ts] module.exports = onlyChild;
 // [ts] },{"25":25,"30":30,"9":9}],25:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] /**
 // [ts] * WARNING: DO NOT manually require this module.
 // [ts] * This is a replacement for `invariant(...)` used by the error code system
 // [ts] * and will _only_ be required by the corresponding babel pass.
 // [ts] * It always throws.
 // [ts] */
 // [ts] fn reactProdInvariant(code: auto) {
 // [ts] let argCount = arguments.length - 1;
 // [ts] let message = "Minified React error #" + code + "; visit " + "http://facebook.github.io/react/docs/error-decoder.html?invariant=" + code;
 // [ts] let argIdx = 0;
 // [ts] while (argIdx < argCount) { // [cap-migrate] for-step: argIdx++
 // [ts] message += "&args[]=" + encodeURIComponent(arguments[argIdx + 1]);
 // [ts] }
 // [ts] message += " for the full message or use the non-minified dev environment" + " for full errors and additional helpful warnings.";
 // [ts] let error = Error.init(message);
 // [ts] error.name = "Invariant Violation";
 // [ts] error.framesToPop = 1; // we don't care about reactProdInvariant's own frame
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
 // [ts] }
 // [ts] module.exports = reactProdInvariant;
 // [ts] },{}],26:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] *
 // [ts] * This source code is licensed under the MIT license found in the
 // [ts] * LICENSE file in the root directory of this source tree.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _prodInvariant = _dereq_(25);
 // [ts] let ReactCurrentOwner = _dereq_(7);
 // [ts] let REACT_ELEMENT_TYPE = _dereq_(10);
 // [ts] let getIteratorFn = _dereq_(21);
 // [ts] let invariant = _dereq_(30);
 // [ts] let KeyEscapeUtils = _dereq_(1);
 // [ts] let warning = _dereq_(31);
 // [ts] let SEPARATOR = ".";
 // [ts] let SUBSEPARATOR = ":";
 // [ts] /**
 // [ts] * This is inlined from ReactElement since this file is shared between
 // [ts] * isomorphic and renderers. We could extract this to a
 // [ts] *
 // [ts] */
 // [ts] /**
 // [ts] * TODO: Test that a single child and an array with one item have the same key
 // [ts] * pattern.
 // [ts] */
 // [ts] let didWarnAboutMaps = false;
 // [ts] /**
 // [ts] * Generate a key string that identifies a component within a set.
 // [ts] *
 // [ts] * @param {*} component A component that could contain a manual key.
 // [ts] * @param {number} index Index that is used if a manual key is not provided.
 // [ts] * @return {string}
 // [ts] */
 // [ts] fn getComponentKey(component: auto, index: auto) {
 // Do some typechecking here since we call this blindly. We want to ensure
 // that we don't block potential future ES APIs.
 // [ts] if (component && @typeName(component) == "object" && component.key != null) {
 // Explicit key
 // [ts] return KeyEscapeUtils.escape(component.key);
 // [ts] }
 // Implicit key determined by the index in the set
 // [ts] return index.toString(36);
 // [ts] }
 // [ts] /**
 // [ts] * @param {?*} children Children tree container.
 // [ts] * @param {!string} nameSoFar Name of the key path so far.
 // [ts] * @param {!function} callback Callback to invoke with each child found.
 // [ts] * @param {?*} traverseContext Used to pass information throughout the traversal
 // [ts] * process.
 // [ts] * @return {!number} The number of children in this subtree.
 // [ts] */
 // [ts] fn traverseAllChildrenImpl(children: auto, nameSoFar: auto, callback: auto, traverseContext: auto) {
 // [ts] let type = @typeName(children);
 // [ts] if (type == "undefined" || type == "boolean") {
 // All of the above are perceived as null.
 // [ts] children = null;
 // [ts] }
 // [ts] if (children == null || type == "string" || type == "number" ||
 // The following is inlined from ReactElement. This means we can optimize
 // some checks. React Fiber also inlines this logic for similar purposes.
 // [ts] type == "object" && children.$$typeof == REACT_ELEMENT_TYPE) {
 // [ts] callback(traverseContext, children,
 // If it's the only child, treat the name as if it was wrapped in an array
 // so that it's consistent if the number of children grows.
 // [ts] if (nameSoFar == "") SEPARATOR + getComponentKey(children, 0) else nameSoFar);
 // [ts] return 1;
 // [ts] }
 // [ts] let child;
 // [ts] let nextName;
 // [ts] let subtreeCount = 0; // Count of children found in the current subtree.
 // [ts] let nextNamePrefix = if (nameSoFar == "") SEPARATOR else nameSoFar + SUBSEPARATOR;
 // [ts] if (Array.isArray(children)) {
 // [ts] let i = 0;
 // [ts] while (i < children.length) { // [cap-migrate] for-step: i++
 // [ts] child = children[i];
 // [ts] nextName = nextNamePrefix + getComponentKey(child, i);
 // [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
 // [ts] }
 // [ts] } else {
 // [ts] let iteratorFn = getIteratorFn(children);
 // [ts] if (iteratorFn) {
 // [ts] let iterator = iteratorFn.call(children);
 // [ts] let step;
 // [ts] if (iteratorFn != children.entries) {
 // [ts] let ii = 0;
 // [ts] while (!(step = iterator.next()).done) {
 // [ts] child = step.value;
 // [ts] nextName = nextNamePrefix + getComponentKey(child, ii++);
 // [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
 // [ts] }
 // [ts] } else {
 // [ts] if ("development" != "production") {
 // [ts] let mapsAsChildrenAddendum = "";
 // [ts] if (ReactCurrentOwner.current) {
 // [ts] let mapsAsChildrenOwnerName = ReactCurrentOwner.current.getName();
 // [ts] if (mapsAsChildrenOwnerName) {
 // [ts] mapsAsChildrenAddendum = " Check the render method of `" + mapsAsChildrenOwnerName + "`.";
 // [ts] }
 // [ts] }
 // [ts] if ("development" != "production") warning(didWarnAboutMaps, "Using Maps as children is not yet fully supported. It is an " + "experimental feature that might be removed. Convert it to a " + "sequence / iterable of keyed ReactElements instead.%s", mapsAsChildrenAddendum) else void 0;
 // [ts] didWarnAboutMaps = true;
 // [ts] }
 // Iterator will provide entry [k,v] tuples rather than values.
 // [ts] while (!(step = iterator.next()).done) {
 // [ts] let entry = step.value;
 // [ts] if (entry) {
 // [ts] child = entry[1];
 // [ts] nextName = nextNamePrefix + KeyEscapeUtils.escape(entry[0]) + SUBSEPARATOR + getComponentKey(child, 0);
 // [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] } else if (type == "object") {
 // [ts] let addendum = "";
 // [ts] if ("development" != "production") {
 // [ts] addendum = " If you meant to render a collection of children, use an array " + "instead or wrap the object using createFragment(object) from the " + "React add-ons.";
 // [ts] if (children._isReactElement) {
 // [ts] addendum = " It looks like you're using an element created by a different " + "version of React. Make sure to use only one copy of React.";
 // [ts] }
 // [ts] if (ReactCurrentOwner.current) {
 // [ts] let name = ReactCurrentOwner.current.getName();
 // [ts] if (name) {
 // [ts] addendum += " Check the render method of `" + name + "`.";
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] let childrenString = String(children);
 // [ts] if (!false) if ("development" != "production") invariant(if (false, "Objects are not valid as a React child (found: %s).%s", childrenString == "[object Object]") "object with keys {" + Object.keys(children).join(", ") + "}" else childrenString, addendum) else _prodInvariant(if ("31", childrenString == "[object Object]") "object with keys {" + Object.keys(children).join(", ") + "}" else childrenString, addendum) else void 0;
 // [ts] }
 // [ts] }
 // [ts] return subtreeCount;
 // [ts] }
 // [ts] /**
 // [ts] * Traverses children that are typically specified as `props.children`, but
 // [ts] * might also be specified through attributes:
 // [ts] *
 // [ts] * - `traverseAllChildren(this.props.children, ...)`
 // [ts] * - `traverseAllChildren(this.props.leftPanelChildren, ...)`
 // [ts] *
 // [ts] * The `traverseContext` is an optional argument that is passed through the
 // [ts] * entire traversal. It can be used to store accumulations or anything else that
 // [ts] * the callback might find relevant.
 // [ts] *
 // [ts] * @param {?*} children Children tree object.
 // [ts] * @param {!function} callback To invoke upon traversing each child.
 // [ts] * @param {?*} traverseContext Context for traversal.
 // [ts] * @return {!number} The number of children in this subtree.
 // [ts] */
 // [ts] fn traverseAllChildren(children: auto, callback: auto, traverseContext: auto) {
 // [ts] if (children == null) {
 // [ts] return 0;
 // [ts] }
 // [ts] return traverseAllChildrenImpl(children, "", callback, traverseContext);
 // [ts] }
 // [ts] module.exports = traverseAllChildren;
 // [ts] },{"1":1,"10":10,"21":21,"25":25,"30":30,"31":31,"7":7}],27:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let _assign = _dereq_(32);
 // [ts] let emptyObject = _dereq_(29);
 // [ts] let _invariant = _dereq_(30);
 // [ts] if ("development" != "production") {
 // [ts] let warning = _dereq_(31);
 // [ts] }
 // [ts] let MIXINS_KEY = "mixins";
 // Helper function to allow the creation of anonymous functions which do not
 // have .name set to the name of the variable being assigned to.
 // [ts] fn identity(fn: auto) {
 // [ts] return fn;
 // [ts] }
 // [ts] let ReactPropTypeLocationNames;
 // [ts] if ("development" != "production") {
 // [ts] ReactPropTypeLocationNames = {
 // [ts] prop: "prop",
 // [ts] context: "context",
 // [ts] childContext: "child context"
 // [ts] };
 // [ts] } else {
 // [ts] ReactPropTypeLocationNames = {};
 // [ts] }
 // [ts] fn factory(ReactComponent: auto, isValidElement: auto, ReactNoopUpdateQueue: auto) {
 // [ts] /**
 // [ts] * Policies that describe methods in `ReactClassInterface`.
 // [ts] */
 // [ts] let injectedMixins = [];
 // [ts] /**
 // [ts] * Composite components are higher-level components that compose other composite
 // [ts] * or host components.
 // [ts] *
 // [ts] * To create a new type of `ReactClass`, pass a specification of
 // [ts] * your new class to `React.createClass`. The only requirement of your class
 // [ts] * specification is that you implement a `render` method.
 // [ts] *
 // [ts] *   var MyComponent = React.createClass({
 // [ts] *     render: function() {
 // [ts] *       return <div>Hello World</div>;
 // [ts] *     }
 // [ts] *   });
 // [ts] *
 // [ts] * The class specification supports a specific protocol of methods that have
 // [ts] * special meaning (e.g. `render`). See `ReactClassInterface` for
 // [ts] * more the comprehensive protocol. Any other properties and methods in the
 // [ts] * class specification will be available on the prototype.
 // [ts] *
 // [ts] * @interface ReactClassInterface
 // [ts] * @internal
 // [ts] */
 // [ts] let ReactClassInterface = {
 // [ts] /**
 // [ts] * An array of Mixin objects to include when defining your component.
 // [ts] *
 // [ts] * @type {array}
 // [ts] * @optional
 // [ts] */
 // [ts] mixins: "DEFINE_MANY",
 // [ts] /**
 // [ts] * An object containing properties and methods that should be defined on
 // [ts] * the component's constructor instead of its prototype (static methods).
 // [ts] *
 // [ts] * @type {object}
 // [ts] * @optional
 // [ts] */
 // [ts] statics: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Definition of prop types for this component.
 // [ts] *
 // [ts] * @type {object}
 // [ts] * @optional
 // [ts] */
 // [ts] propTypes: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Definition of context types for this component.
 // [ts] *
 // [ts] * @type {object}
 // [ts] * @optional
 // [ts] */
 // [ts] contextTypes: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Definition of context types this component sets for its children.
 // [ts] *
 // [ts] * @type {object}
 // [ts] * @optional
 // [ts] */
 // [ts] childContextTypes: "DEFINE_MANY",
 // ==== Definition methods ====
 // [ts] /**
 // [ts] * Invoked when the component is mounted. Values in the mapping will be set on
 // [ts] * `this.props` if that prop is not specified (i.e. using an `in` check).
 // [ts] *
 // [ts] * This method is invoked before `getInitialState` and therefore cannot rely
 // [ts] * on `this.state` or use `this.setState`.
 // [ts] *
 // [ts] * @return {object}
 // [ts] * @optional
 // [ts] */
 // [ts] getDefaultProps: "DEFINE_MANY_MERGED",
 // [ts] /**
 // [ts] * Invoked once before the component is mounted. The return value will be used
 // [ts] * as the initial value of `this.state`.
 // [ts] *
 // [ts] *   getInitialState: function() {
 // [ts] *     return {
 // [ts] *       isOn: false,
 // [ts] *       fooBaz: new BazFoo()
 // [ts] *     }
 // [ts] *   }
 // [ts] *
 // [ts] * @return {object}
 // [ts] * @optional
 // [ts] */
 // [ts] getInitialState: "DEFINE_MANY_MERGED",
 // [ts] /**
 // [ts] * @return {object}
 // [ts] * @optional
 // [ts] */
 // [ts] getChildContext: "DEFINE_MANY_MERGED",
 // [ts] /**
 // [ts] * Uses props from `this.props` and state from `this.state` to render the
 // [ts] * structure of the component.
 // [ts] *
 // [ts] * No guarantees are made about when or how often this method is invoked, so
 // [ts] * it must not have side effects.
 // [ts] *
 // [ts] *   render: function() {
 // [ts] *     var name = this.props.name;
 // [ts] *     return <div>Hello, {name}!</div>;
 // [ts] *   }
 // [ts] *
 // [ts] * @return {ReactComponent}
 // [ts] * @required
 // [ts] */
 // [ts] render: "DEFINE_ONCE",
 // ==== Delegate methods ====
 // [ts] /**
 // [ts] * Invoked when the component is initially created and about to be mounted.
 // [ts] * This may have side effects, but any external subscriptions or data created
 // [ts] * by this method must be cleaned up in `componentWillUnmount`.
 // [ts] *
 // [ts] * @optional
 // [ts] */
 // [ts] componentWillMount: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Invoked when the component has been mounted and has a DOM representation.
 // [ts] * However, there is no guarantee that the DOM node is in the document.
 // [ts] *
 // [ts] * Use this as an opportunity to operate on the DOM when the component has
 // [ts] * been mounted (initialized and rendered) for the first time.
 // [ts] *
 // [ts] * @param {DOMElement} rootNode DOM element representing the component.
 // [ts] * @optional
 // [ts] */
 // [ts] componentDidMount: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Invoked before the component receives new props.
 // [ts] *
 // [ts] * Use this as an opportunity to react to a prop transition by updating the
 // [ts] * state using `this.setState`. Current props are accessed via `this.props`.
 // [ts] *
 // [ts] *   componentWillReceiveProps: function(nextProps, nextContext) {
 // [ts] *     this.setState({
 // [ts] *       likesIncreasing: nextProps.likeCount > this.props.likeCount
 // [ts] *     });
 // [ts] *   }
 // [ts] *
 // [ts] * NOTE: There is no equivalent `componentWillReceiveState`. An incoming prop
 // [ts] * transition may cause a state change, but the opposite is not true. If you
 // [ts] * need it, you are probably looking for `componentWillUpdate`.
 // [ts] *
 // [ts] * @param {object} nextProps
 // [ts] * @optional
 // [ts] */
 // [ts] componentWillReceiveProps: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Invoked while deciding if the component should be updated as a result of
 // [ts] * receiving new props, state and/or context.
 // [ts] *
 // [ts] * Use this as an opportunity to `return false` when you're certain that the
 // [ts] * transition to the new props/state/context will not require a component
 // [ts] * update.
 // [ts] *
 // [ts] *   shouldComponentUpdate: function(nextProps, nextState, nextContext) {
 // [ts] *     return !equal(nextProps, this.props) ||
 // [ts] *       !equal(nextState, this.state) ||
 // [ts] *       !equal(nextContext, this.context);
 // [ts] *   }
 // [ts] *
 // [ts] * @param {object} nextProps
 // [ts] * @param {?object} nextState
 // [ts] * @param {?object} nextContext
 // [ts] * @return {boolean} True if the component should update.
 // [ts] * @optional
 // [ts] */
 // [ts] shouldComponentUpdate: "DEFINE_ONCE",
 // [ts] /**
 // [ts] * Invoked when the component is about to update due to a transition from
 // [ts] * `this.props`, `this.state` and `this.context` to `nextProps`, `nextState`
 // [ts] * and `nextContext`.
 // [ts] *
 // [ts] * Use this as an opportunity to perform preparation before an update occurs.
 // [ts] *
 // [ts] * NOTE: You **cannot** use `this.setState()` in this method.
 // [ts] *
 // [ts] * @param {object} nextProps
 // [ts] * @param {?object} nextState
 // [ts] * @param {?object} nextContext
 // [ts] * @param {ReactReconcileTransaction} transaction
 // [ts] * @optional
 // [ts] */
 // [ts] componentWillUpdate: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Invoked when the component's DOM representation has been updated.
 // [ts] *
 // [ts] * Use this as an opportunity to operate on the DOM when the component has
 // [ts] * been updated.
 // [ts] *
 // [ts] * @param {object} prevProps
 // [ts] * @param {?object} prevState
 // [ts] * @param {?object} prevContext
 // [ts] * @param {DOMElement} rootNode DOM element representing the component.
 // [ts] * @optional
 // [ts] */
 // [ts] componentDidUpdate: "DEFINE_MANY",
 // [ts] /**
 // [ts] * Invoked when the component is about to be removed from its parent and have
 // [ts] * its DOM representation destroyed.
 // [ts] *
 // [ts] * Use this as an opportunity to deallocate any external resources.
 // [ts] *
 // [ts] * NOTE: There is no `componentDidUnmount` since your component will have been
 // [ts] * destroyed by that point.
 // [ts] *
 // [ts] * @optional
 // [ts] */
 // [ts] componentWillUnmount: "DEFINE_MANY",
 // ==== Advanced methods ====
 // [ts] /**
 // [ts] * Updates the component's currently mounted DOM representation.
 // [ts] *
 // [ts] * By default, this implements React's rendering and reconciliation algorithm.
 // [ts] * Sophisticated clients may wish to override this.
 // [ts] *
 // [ts] * @param {ReactReconcileTransaction} transaction
 // [ts] * @internal
 // [ts] * @overridable
 // [ts] */
 // [ts] updateComponent: "OVERRIDE_BASE"
 // [ts] };
 // [ts] /**
 // [ts] * Mapping from class specification keys to special processing functions.
 // [ts] *
 // [ts] * Although these are declared like instance properties in the specification
 // [ts] * when defining classes using `React.createClass`, they are actually static
 // [ts] * and are accessible on the constructor instead of the prototype. Despite
 // [ts] * being static, they must be defined outside of the "statics" key under
 // [ts] * which all other static methods are defined.
 // [ts] */
 // [ts] let RESERVED_SPEC_KEYS = {
 // [ts] displayName: (Constructor, displayName) => {
 // [ts] Constructor.displayName = displayName;
 // [ts] },
 // [ts] mixins: (Constructor, mixins) => {
 // [ts] if (mixins) {
 // [ts] let i = 0;
 // [ts] while (i < mixins.length) { // [cap-migrate] for-step: i++
 // [ts] mixSpecIntoComponent(Constructor, mixins[i]);
 // [ts] }
 // [ts] }
 // [ts] },
 // [ts] childContextTypes: (Constructor, childContextTypes) => {
 // [ts] if ("development" != "production") {
 // [ts] validateTypeDef(Constructor, childContextTypes, "childContext");
 // [ts] }
 // [ts] Constructor.childContextTypes = _assign(
 // [ts] {},
 // [ts] Constructor.childContextTypes,
 // [ts] childContextTypes
 // [ts] );
 // [ts] },
 // [ts] contextTypes: (Constructor, contextTypes) => {
 // [ts] if ("development" != "production") {
 // [ts] validateTypeDef(Constructor, contextTypes, "context");
 // [ts] }
 // [ts] Constructor.contextTypes = _assign(
 // [ts] {},
 // [ts] Constructor.contextTypes,
 // [ts] contextTypes
 // [ts] );
 // [ts] },
 // [ts] /**
 // [ts] * Special case getDefaultProps which should move into statics but requires
 // [ts] * automatic merging.
 // [ts] */
 // [ts] getDefaultProps: (Constructor, getDefaultProps) => {
 // [ts] if (Constructor.getDefaultProps) {
 // [ts] Constructor.getDefaultProps = createMergedResultFunction(
 // [ts] Constructor.getDefaultProps,
 // [ts] getDefaultProps
 // [ts] );
 // [ts] } else {
 // [ts] Constructor.getDefaultProps = getDefaultProps;
 // [ts] }
 // [ts] },
 // [ts] propTypes: (Constructor, propTypes) => {
 // [ts] if ("development" != "production") {
 // [ts] validateTypeDef(Constructor, propTypes, "prop");
 // [ts] }
 // [ts] Constructor.propTypes = _assign({}, Constructor.propTypes, propTypes);
 // [ts] },
 // [ts] statics: (Constructor, statics) => {
 // [ts] mixStaticSpecIntoComponent(Constructor, statics);
 // [ts] },
 // [ts] autobind: () => {}
 // [ts] };
 // [ts] fn validateTypeDef(Constructor: auto, typeDef: auto, location: auto) {
 // [ts] for (let propName in typeDef) {
 // [ts] if (typeDef.hasOwnProperty(propName)) {
 // use a warning instead of an _invariant so components
 // don't show up in prod but only in __DEV__
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] @typeName(typeDef[propName]) == "function",
 // [ts] "%s: %s type `%s` is invalid; it must be a function, usually from " +
 // [ts] "React.PropTypes.",
 // [ts] Constructor.displayName || "ReactClass",
 // [ts] ReactPropTypeLocationNames[location],
 // [ts] propName
 // [ts] );
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] fn validateMethodOverride(isAlreadyDefined: auto, name: auto) {
 // [ts] let specPolicy = if (ReactClassInterface.hasOwnProperty(name)) ReactClassInterface[name] else null;
 // Disallow overriding of base class methods unless explicitly allowed.
 // [ts] if (ReactClassMixin.hasOwnProperty(name)) {
 // [ts] _invariant(
 // [ts] specPolicy == "OVERRIDE_BASE",
 // [ts] "ReactClassInterface: You are attempting to override " +
 // [ts] "`%s` from your class specification. Ensure that your method names " +
 // [ts] "do not overlap with React methods.",
 // [ts] name
 // [ts] );
 // [ts] }
 // Disallow defining methods more than once unless explicitly allowed.
 // [ts] if (isAlreadyDefined) {
 // [ts] _invariant(
 // [ts] specPolicy == "DEFINE_MANY" || specPolicy == "DEFINE_MANY_MERGED",
 // [ts] "ReactClassInterface: You are attempting to define " +
 // [ts] "`%s` on your component more than once. This conflict may be due " +
 // [ts] "to a mixin.",
 // [ts] name
 // [ts] );
 // [ts] }
 // [ts] }
 // [ts] /**
 // [ts] * Mixin helper which handles policy validation and reserved
 // [ts] * specification keys when building React classes.
 // [ts] */
 // [ts] fn mixSpecIntoComponent(Constructor: auto, spec: auto) {
 // [ts] if (!spec) {
 // [ts] if ("development" != "production") {
 // [ts] let typeofSpec = @typeName(spec);
 // [ts] let isMixinValid = typeofSpec == "object" && spec != null;
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] isMixinValid,
 // [ts] "%s: You're attempting to include a mixin that is either null " +
 // [ts] "or not an object. Check the mixins included by the component, " +
 // [ts] "as well as any mixins they include themselves. " +
 // [ts] "Expected object but got %s.",
 // [ts] Constructor.displayName || "ReactClass",
 // [ts] if (spec == null) null else typeofSpec
 // [ts] );
 // [ts] }
 // [ts] }
 // [ts] return;
 // [ts] }
 // [ts] _invariant(
 // [ts] @typeName(spec) != "function",
 // [ts] "ReactClass: You're attempting to " +
 // [ts] "use a component class or function as a mixin. Instead, just use a " +
 // [ts] "regular object."
 // [ts] );
 // [ts] _invariant(
 // [ts] !isValidElement(spec),
 // [ts] "ReactClass: You're attempting to " +
 // [ts] "use a component as a mixin. Instead, just use a regular object."
 // [ts] );
 // [ts] let proto = Constructor.prototype;
 // [ts] let autoBindPairs = proto.__reactAutoBindPairs;
 // By handling mixins before any other properties, we ensure the same
 // chaining order is applied to methods with DEFINE_MANY policy, whether
 // mixins are listed before or after these methods in the spec.
 // [ts] if (spec.hasOwnProperty(MIXINS_KEY)) {
 // [ts] RESERVED_SPEC_KEYS.mixins(Constructor, spec.mixins);
 // [ts] }
 // [ts] for (let name in spec) {
 // [ts] if (!spec.hasOwnProperty(name)) {
 // [ts] continue;
 // [ts] }
 // [ts] if (name == MIXINS_KEY) {
 // We have already handled mixins in a special case above.
 // [ts] continue;
 // [ts] }
 // [ts] let property = spec[name];
 // [ts] let isAlreadyDefined = proto.hasOwnProperty(name);
 // [ts] validateMethodOverride(isAlreadyDefined, name);
 // [ts] if (RESERVED_SPEC_KEYS.hasOwnProperty(name)) {
 // [ts] RESERVED_SPEC_KEYS[name](Constructor, property);
 // [ts] } else {
 // Setup methods on prototype:
 // The following member methods should not be automatically bound:
 // 1. Expected ReactClass methods (in the "interface").
 // 2. Overridden methods (that were mixed in).
 // [ts] let isReactClassMethod = ReactClassInterface.hasOwnProperty(name);
 // [ts] let isFunction = @typeName(property) == "function";
 // [ts] let shouldAutoBind =
 // [ts] isFunction &&
 // [ts] !isReactClassMethod &&
 // [ts] !isAlreadyDefined &&
 // [ts] spec.autobind != false;
 // [ts] if (shouldAutoBind) {
 // [ts] autoBindPairs.push(name, property);
 // [ts] proto[name] = property;
 // [ts] } else {
 // [ts] if (isAlreadyDefined) {
 // [ts] let specPolicy = ReactClassInterface[name];
 // These cases should already be caught by validateMethodOverride.
 // [ts] _invariant(
 // [ts] isReactClassMethod &&
 // [ts] (specPolicy == "DEFINE_MANY_MERGED" ||
 // [ts] specPolicy == "DEFINE_MANY"),
 // [ts] "ReactClass: Unexpected spec policy %s for key %s " +
 // [ts] "when mixing in component specs.",
 // [ts] specPolicy,
 // [ts] name
 // [ts] );
 // For methods which are defined more than once, call the existing
 // methods before calling the new property, merging if appropriate.
 // [ts] if (specPolicy == "DEFINE_MANY_MERGED") {
 // [ts] proto[name] = createMergedResultFunction(proto[name], property);
 // [ts] } else if (specPolicy == "DEFINE_MANY") {
 // [ts] proto[name] = createChainedFunction(proto[name], property);
 // [ts] }
 // [ts] } else {
 // [ts] proto[name] = property;
 // [ts] if ("development" != "production") {
 // Add verbose displayName to the function, which helps when looking
 // at profiling tools.
 // [ts] if (@typeName(property) == "function" && spec.displayName) {
 // [ts] proto[name].displayName = spec.displayName + "_" + name;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] fn mixStaticSpecIntoComponent(Constructor: auto, statics: auto) {
 // [ts] if (!statics) {
 // [ts] return;
 // [ts] }
 // [ts] for (let name in statics) {
 // [ts] let property = statics[name];
 // [ts] if (!statics.hasOwnProperty(name)) {
 // [ts] continue;
 // [ts] }
 // [ts] let isReserved = name == RESERVED_SPEC_KEYS;
 // [ts] _invariant(
 // [ts] !isReserved,
 // [ts] "ReactClass: You are attempting to define a reserved " +
 // [ts] "property, `%s`, that shouldn't be on the \"statics\" key. Define it " +
 // [ts] "as an instance property instead; it will still be accessible on the " +
 // [ts] "constructor.",
 // [ts] name
 // [ts] );
 // [ts] let isInherited = name == Constructor;
 // [ts] _invariant(
 // [ts] !isInherited,
 // [ts] "ReactClass: You are attempting to define " +
 // [ts] "`%s` on your component more than once. This conflict may be " +
 // [ts] "due to a mixin.",
 // [ts] name
 // [ts] );
 // [ts] Constructor[name] = property;
 // [ts] }
 // [ts] }
 // [ts] /**
 // [ts] * Merge two objects, but throw if both contain the same key.
 // [ts] *
 // [ts] * @param {object} one The first object, which is mutated.
 // [ts] * @param {object} two The second object
 // [ts] * @return {object} one after it has been mutated to contain everything in two.
 // [ts] */
 // [ts] fn mergeIntoWithNoDuplicateKeys(one: auto, two: auto) {
 // [ts] _invariant(
 // [ts] one && two && @typeName(one) == "object" && @typeName(two) == "object",
 // [ts] "mergeIntoWithNoDuplicateKeys(): Cannot merge non-objects."
 // [ts] );
 // [ts] for (let key in two) {
 // [ts] if (two.hasOwnProperty(key)) {
 // [ts] _invariant(
 // [ts] one[key] == null,
 // [ts] "mergeIntoWithNoDuplicateKeys(): " +
 // [ts] "Tried to merge two objects with the same key: `%s`. This conflict " +
 // [ts] "may be due to a mixin; in particular, this may be caused by two " +
 // [ts] "getInitialState() or getDefaultProps() methods returning objects " +
 // [ts] "with clashing keys.",
 // [ts] key
 // [ts] );
 // [ts] one[key] = two[key];
 // [ts] }
 // [ts] }
 // [ts] return one;
 // [ts] }
 // [ts] /**
 // [ts] * Creates a function that invokes two functions and merges their return values.
 // [ts] *
 // [ts] * @param {function} one Function to invoke first.
 // [ts] * @param {function} two Function to invoke second.
 // [ts] * @return {function} Function that invokes the two argument functions.
 // [ts] * @private
 // [ts] */
 // [ts] fn createMergedResultFunction(one: auto, two: auto) {
 // [ts] return () => {
 // [ts] let a = one.apply(this, arguments);
 // [ts] let b = two.apply(this, arguments);
 // [ts] if (a == null) {
 // [ts] return b;
 // [ts] } else if (b == null) {
 // [ts] return a;
 // [ts] }
 // [ts] let c = {};
 // [ts] mergeIntoWithNoDuplicateKeys(c, a);
 // [ts] mergeIntoWithNoDuplicateKeys(c, b);
 // [ts] return c;
 // [ts] };
 // [ts] }
 // [ts] /**
 // [ts] * Creates a function that invokes two functions and ignores their return vales.
 // [ts] *
 // [ts] * @param {function} one Function to invoke first.
 // [ts] * @param {function} two Function to invoke second.
 // [ts] * @return {function} Function that invokes the two argument functions.
 // [ts] * @private
 // [ts] */
 // [ts] fn createChainedFunction(one: auto, two: auto) {
 // [ts] return () => {
 // [ts] one.apply(this, arguments);
 // [ts] two.apply(this, arguments);
 // [ts] };
 // [ts] }
 // [ts] /**
 // [ts] * Binds a method to the component.
 // [ts] *
 // [ts] * @param {object} component Component whose method is going to be bound.
 // [ts] * @param {function} method Method to be bound.
 // [ts] * @return {function} The bound method.
 // [ts] */
 // [ts] fn bindAutoBindMethod(component: auto, method: auto) {
 // [ts] let boundMethod = method.bind(component);
 // [ts] if ("development" != "production") {
 // [ts] boundMethod.__reactBoundContext = component;
 // [ts] boundMethod.__reactBoundMethod = method;
 // [ts] boundMethod.__reactBoundArguments = null;
 // [ts] let componentName = component.constructor.displayName;
 // [ts] let _bind = boundMethod.bind;
 // [ts] boundMethod.bind = (newThis) => {
 // [ts] for (
 // [ts] let _len = arguments.length,
 // [ts] args = Array(if (_len > 1) _len - 1 else 0),
 // [ts] _key = 1;
 // [ts] _key < _len;
 // [ts] _key++
 // [ts] ) {
 // [ts] args[_key - 1] = arguments[_key];
 // [ts] }
 // User is trying to bind() an autobound method; we effectively will
 // ignore the value of "this" that the user is trying to use, so
 // let's warn.
 // [ts] if (newThis != component && newThis != null) {
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] false,
 // [ts] "bind(): React component methods may only be bound to the " +
 // [ts] "component instance. See %s",
 // [ts] componentName
 // [ts] );
 // [ts] }
 // [ts] } else if (!args.length) {
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] false,
 // [ts] "bind(): You are binding a component method to the component. " +
 // [ts] "React does this for you automatically in a high-performance " +
 // [ts] "way, so you can safely remove this call. See %s",
 // [ts] componentName
 // [ts] );
 // [ts] }
 // [ts] return boundMethod;
 // [ts] }
 // [ts] let reboundMethod = _bind.apply(boundMethod, arguments);
 // [ts] reboundMethod.__reactBoundContext = component;
 // [ts] reboundMethod.__reactBoundMethod = method;
 // [ts] reboundMethod.__reactBoundArguments = args;
 // [ts] return reboundMethod;
 // [ts] };
 // [ts] }
 // [ts] return boundMethod;
 // [ts] }
 // [ts] /**
 // [ts] * Binds all auto-bound methods in a component.
 // [ts] *
 // [ts] * @param {object} component Component whose method is going to be bound.
 // [ts] */
 // [ts] fn bindAutoBindMethods(component: auto) {
 // [ts] let pairs = component.__reactAutoBindPairs;
 // [ts] let i = 0;
 // [ts] while (i < pairs.length) { // [cap-migrate] for-step: i += 2
 // [ts] let autoBindKey = pairs[i];
 // [ts] let method = pairs[i + 1];
 // [ts] component[autoBindKey] = bindAutoBindMethod(component, method);
 // [ts] }
 // [ts] }
 // [ts] let IsMountedPreMixin = {
 // [ts] componentDidMount: () => {
 // [ts] _.__isMounted = true;
 // [ts] }
 // [ts] };
 // [ts] let IsMountedPostMixin = {
 // [ts] componentWillUnmount: () => {
 // [ts] _.__isMounted = false;
 // [ts] }
 // [ts] };
 // [ts] /**
 // [ts] * Add more to the ReactClass base class. These are all legacy features and
 // [ts] * therefore not already part of the modern ReactComponent.
 // [ts] */
 // [ts] let ReactClassMixin = {
 // [ts] /**
 // [ts] * TODO: This will be deprecated because state should always keep a consistent
 // [ts] * type signature and the only use case for this, is to avoid that.
 // [ts] */
 // [ts] replaceState: (newState, callback) => {
 // [ts] _.updater.enqueueReplaceState(this, newState, callback);
 // [ts] },
 // [ts] /**
 // [ts] * Checks whether or not this composite component is mounted.
 // [ts] * @return {boolean} True if mounted, false otherwise.
 // [ts] * @protected
 // [ts] * @final
 // [ts] */
 // [ts] isMounted: () => {
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] _.__didWarnIsMounted,
 // [ts] "%s: isMounted is deprecated. Instead, make sure to clean up " +
 // [ts] "subscriptions and pending requests in componentWillUnmount to " +
 // [ts] "prevent memory leaks.",
 // [ts] (_.constructor && _.constructor.displayName) ||
 // [ts] _.name ||
 // [ts] "Component"
 // [ts] );
 // [ts] _.__didWarnIsMounted = true;
 // [ts] }
 // [ts] return !!_.__isMounted;
 // [ts] }
 // [ts] };
 // [ts] let ReactClassComponent = () => {};
 // [ts] _assign(
 // [ts] ReactClassComponent.prototype,
 // [ts] ReactComponent.prototype,
 // [ts] ReactClassMixin
 // [ts] );
 // [ts] /**
 // [ts] * Creates a composite component class given a class specification.
 // [ts] * See https://facebook.github.io/react/docs/top-level-api.html#react.createclass
 // [ts] *
 // [ts] * @param {object} spec Class specification (which must define `render`).
 // [ts] * @return {function} Component constructor function.
 // [ts] * @public
 // [ts] */
 // [ts] fn createClass(spec: auto) {
 // To keep our warnings more understandable, we'll use a little hack here to
 // ensure that Constructor.name !== 'Constructor'. This makes sure we don't
 // unnecessarily identify a class without displayName as 'Constructor'.
 // [ts] let Constructor = identity((props, context, updater) => {
 // This constructor gets overridden by mocks. The argument is used
 // by mocks to assert on what gets mounted.
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] this == Constructor,
 // [ts] "Something is calling a React component directly. Use a factory or " +
 // [ts] "JSX instead. See: https://fb.me/react-legacyfactory"
 // [ts] );
 // [ts] }
 // Wire up auto-binding
 // [ts] if (_.__reactAutoBindPairs.length) {
 // [ts] bindAutoBindMethods(this);
 // [ts] }
 // [ts] _.props = props;
 // [ts] _.context = context;
 // [ts] _.refs = emptyObject;
 // [ts] _.updater = updater || ReactNoopUpdateQueue;
 // [ts] _.state = null;
 // ReactClasses doesn't have constructors. Instead, they use the
 // getInitialState and componentWillMount methods for initialization.
 // [ts] let initialState = if (_.getInitialState) _.getInitialState() else null;
 // [ts] if ("development" != "production") {
 // We allow auto-mocks to proceed as if they're returning null.
 // [ts] if (
 // [ts] initialState == null &&
 // [ts] _.getInitialState._isMockFunction
 // [ts] ) {
 // This is probably bad practice. Consider warning here and
 // deprecating this convenience.
 // [ts] initialState = null;
 // [ts] }
 // [ts] }
 // [ts] _invariant(
 // [ts] @typeName(initialState) == "object" && !Array.isArray(initialState),
 // [ts] "%s.getInitialState(): must return an object or null",
 // [ts] Constructor.displayName || "ReactCompositeComponent"
 // [ts] );
 // [ts] _.state = initialState;
 // [ts] });
 // [ts] Constructor.prototype = ReactClassComponent.init();
 // [ts] Constructor.prototype.constructor = Constructor;
 // [ts] Constructor.prototype.__reactAutoBindPairs = [];
 // [ts] injectedMixins.forEach(mixSpecIntoComponent.bind(null, Constructor));
 // [ts] mixSpecIntoComponent(Constructor, IsMountedPreMixin);
 // [ts] mixSpecIntoComponent(Constructor, spec);
 // [ts] mixSpecIntoComponent(Constructor, IsMountedPostMixin);
 // Initialize the defaultProps property after all mixins have been merged.
 // [ts] if (Constructor.getDefaultProps) {
 // [ts] Constructor.defaultProps = Constructor.getDefaultProps();
 // [ts] }
 // [ts] if ("development" != "production") {
 // This is a tag to indicate that the use of these method names is ok,
 // since it's used with createClass. If it's not, then it's likely a
 // mistake so we'll warn you to use the static property, property
 // initializer or constructor respectively.
 // [ts] if (Constructor.getDefaultProps) {
 // [ts] Constructor.getDefaultProps.isReactClassApproved = {};
 // [ts] }
 // [ts] if (Constructor.prototype.getInitialState) {
 // [ts] Constructor.prototype.getInitialState.isReactClassApproved = {};
 // [ts] }
 // [ts] }
 // [ts] _invariant(
 // [ts] Constructor.prototype.render,
 // [ts] "createClass(...): Class specification must implement a `render` method."
 // [ts] );
 // [ts] if ("development" != "production") {
 // [ts] warning(
 // [ts] !Constructor.prototype.componentShouldUpdate,
 // [ts] "%s has a method called " +
 // [ts] "componentShouldUpdate(). Did you mean shouldComponentUpdate()? " +
 // [ts] "The name is phrased as a question because the function is " +
 // [ts] "expected to return a value.",
 // [ts] spec.displayName || "A component"
 // [ts] );
 // [ts] warning(
 // [ts] !Constructor.prototype.componentWillRecieveProps,
 // [ts] "%s has a method called " +
 // [ts] "componentWillRecieveProps(). Did you mean componentWillReceiveProps()?",
 // [ts] spec.displayName || "A component"
 // [ts] );
 // [ts] }
 // Reduce time spent doing lookups by setting these on the prototype.
 // [ts] for (let methodName in ReactClassInterface) {
 // [ts] if (!Constructor.prototype[methodName]) {
 // [ts] Constructor.prototype[methodName] = null;
 // [ts] }
 // [ts] }
 // [ts] return Constructor;
 // [ts] }
 // [ts] return createClass;
 // [ts] }
 // [ts] module.exports = factory;
 // [ts] },{"29":29,"30":30,"31":31,"32":32}],28:[(_dereq_, module, exports) => {
 // [ts] "use strict";
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] *
 // [ts] *
 // [ts] */
 // [ts] fn makeEmptyFunction(arg: auto) {
 // [ts] return () => {
 // [ts] return arg;
 // [ts] };
 // [ts] }
 // [ts] /**
 // [ts] * This function accepts and discards inputs; it has no side effects. This is
 // [ts] * primarily useful idiomatically for overridable function endpoints which
 // [ts] * always need to be callable, since JS lacks a null-call idiom ala Cocoa.
 // [ts] */
 // [ts] let emptyFunction = () => {};
 // [ts] emptyFunction.thatReturns = makeEmptyFunction;
 // [ts] emptyFunction.thatReturnsFalse = makeEmptyFunction(false);
 // [ts] emptyFunction.thatReturnsTrue = makeEmptyFunction(true);
 // [ts] emptyFunction.thatReturnsNull = makeEmptyFunction(null);
 // [ts] emptyFunction.thatReturnsThis = () => {
 // [ts] return this;
 // [ts] };
 // [ts] emptyFunction.thatReturnsArgument = (arg) => {
 // [ts] return arg;
 // [ts] };
 // [ts] module.exports = emptyFunction;
 // [ts] },{}],29:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let emptyObject = {};
 // [ts] if ("development" != "production") {
 // [ts] Object.freeze(emptyObject);
 // [ts] }
 // [ts] module.exports = emptyObject;
 // [ts] },{}],30:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright (c) 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] /**
 // [ts] * Use invariant() to assert state which your program assumes to be true.
 // [ts] *
 // [ts] * Provide sprintf-style format (only %s is supported) and arguments
 // [ts] * to provide information about what broke and what you were
 // [ts] * expecting.
 // [ts] *
 // [ts] * The invariant message will be stripped in production, but the invariant
 // [ts] * will remain to ensure logic does not differ in production.
 // [ts] */
 // [ts] let validateFormat = (format) => {};
 // [ts] if ("development" != "production") {
 // [ts] validateFormat = (format) => {
 // [ts] if (format == null) {
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('invariant requires an error message argument');)
 // [ts] }
 // [ts] };
 // [ts] }
 // [ts] fn invariant(condition: auto, format: auto, a: auto, b: auto, c: auto, d: auto, e: auto, f: auto) {
 // [ts] validateFormat(format);
 // [ts] if (!condition) {
 // [ts] let error;
 // [ts] if (format == null) {
 // [ts] error = Error.init("Minified exception occurred; use the non-minified dev environment " + "for the full error message and additional helpful warnings.");
 // [ts] } else {
 // [ts] let args = [a, b, c, d, e, f];
 // [ts] let argIndex = 0;
 // [ts] error = Error.init(format.replace("%s", () => {
 // [ts] return args[argIndex++];
 // [ts] }));
 // [ts] error.name = "Invariant Violation";
 // [ts] }
 // [ts] error.framesToPop = 1; // we don't care about invariant's own frame
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
 // [ts] }
 // [ts] }
 // [ts] module.exports = invariant;
 // [ts] },{}],31:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright 2014-2015, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] *
 // [ts] */
 // [ts] "use strict";
 // [ts] let emptyFunction = _dereq_(28);
 // [ts] /**
 // [ts] * Similar to invariant but only logs a warning if the condition is not met.
 // [ts] * This can be used to log issues in development environments in critical
 // [ts] * paths. Removing the logging code for production environments will keep the
 // [ts] * same logic and follow the same code paths.
 // [ts] */
 // [ts] let warning = emptyFunction;
 // [ts] if ("development" != "production") {
 // [ts] (() => {
 // [ts] let printWarning = (format) => {
 // [ts] let _len = arguments.length, args = Array(if (_len > 1) _len - 1 else 0), _key = 1;
 // [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
 // [ts] args[_key - 1] = arguments[_key];
 // [ts] }
 // [ts] let argIndex = 0;
 // [ts] let message = "Warning: " + format.replace("%s", () => {
 // [ts] return args[argIndex++];
 // [ts] });
 // [ts] if (@typeName(console) != "undefined") {
 // [ts] @print(message);
 // [ts] }
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // --- Welcome to debugging React ---
 // This error was thrown as a convenience so that you can use this stack
 // to find the callsite that caused this warning to fire.
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
 // [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
 // [ts] };
 // [ts] warning = (condition, format) => {
 // [ts] if (format == null) {
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('`warning(condition, format, ...args)` requires a warning ' + 'message argument');)
 // [ts] }
 // [ts] if (format.indexOf("Failed Composite propType: ") == 0) {
 // [ts] return; // Ignore CompositeComponent proptype check.
 // [ts] }
 // [ts] if (!condition) {
 // [ts] let _len2 = arguments.length, args = Array(if (_len2 > 2) _len2 - 2 else 0), _key2 = 2;
 // [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
 // [ts] args[_key2 - 2] = arguments[_key2];
 // [ts] }
 // [ts] printWarning.apply(null, [format].concat(args));
 // [ts] }
 // [ts] };
 // [ts] })();
 // [ts] }
 // [ts] module.exports = warning;
 // [ts] },{"28":28}],32:[(_dereq_, module, exports) => {
 // [ts] /*
 // [ts] object-assign
 // [ts] (c) Sindre Sorhus
 // [ts] @license MIT
 // [ts] */
 // [ts] "use strict";
 // [ts] /* eslint-disable no-unused-vars */
 // [ts] let getOwnPropertySymbols = Object.getOwnPropertySymbols;
 // [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
 // [ts] let propIsEnumerable = Object.prototype.propertyIsEnumerable;
 // [ts] fn toObject(val: auto) {
 // [ts] if (val == null || val == null) {
 // [ts] return; // TODO(cap-migrate): throw -> error return (throw new TypeError('Object.assign cannot be called with null or undefined');)
 // [ts] }
 // [ts] return Object(val);
 // [ts] }
 // [ts] fn shouldUseNative() {
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // [ts] if (!Object.assign) {
 // [ts] return false;
 // [ts] }
 // Detect buggy property enumeration order in older V8 versions.
 // https://bugs.chromium.org/p/v8/issues/detail?id=4118
 // [ts] let test1 = String.init("abc");  // eslint-disable-line no-new-wrappers
 // [ts] test1[5] = "de";
 // [ts] if (Object.getOwnPropertyNames(test1)[0] == "5") {
 // [ts] return false;
 // [ts] }
 // https://bugs.chromium.org/p/v8/issues/detail?id=3056
 // [ts] let test2 = {};
 // [ts] let i = 0;
 // [ts] while (i < 10) { // [cap-migrate] for-step: i++
 // [ts] test2["_" + String.fromCharCode(i)] = i;
 // [ts] }
 // [ts] let order2 = Object.getOwnPropertyNames(test2).map((n) => {
 // [ts] return test2[n];
 // [ts] });
 // [ts] if (order2.join("") != "0123456789") {
 // [ts] return false;
 // [ts] }
 // https://bugs.chromium.org/p/v8/issues/detail?id=3056
 // [ts] let test3 = {};
 // [ts] "abcdefghijklmnopqrst".split("").forEach((letter) => {
 // [ts] test3[letter] = letter;
 // [ts] });
 // [ts] if (Object.keys(Object.assign({}, test3)).join("") !=
 // [ts] "abcdefghijklmnopqrst") {
 // [ts] return false;
 // [ts] }
 // [ts] return true;
 // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
 // We don't expect any of the above to throw, but better to be safe.
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] module.exports = if (shouldUseNative()) Object.assign else (target, source) => {
 // [ts] let from;
 // [ts] let to = toObject(target);
 // [ts] let symbols;
 // [ts] let s = 1;
 // [ts] while (s < arguments.length) { // [cap-migrate] for-step: s++
 // [ts] from = Object(arguments[s]);
 // [ts] for (let key in from) {
 // [ts] if (hasOwnProperty.call(from, key)) {
 // [ts] to[key] = from[key];
 // [ts] }
 // [ts] }
 // [ts] if (getOwnPropertySymbols) {
 // [ts] symbols = getOwnPropertySymbols(from);
 // [ts] let i = 0;
 // [ts] while (i < symbols.length) { // [cap-migrate] for-step: i++
 // [ts] if (propIsEnumerable.call(from, symbols[i])) {
 // [ts] to[symbols[i]] = from[symbols[i]];
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] return to;
 // [ts] };
 // [ts] },{}],33:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] */
 // [ts] "use strict";
 // [ts] if ("development" != "production") {
 // [ts] let invariant = _dereq_(30);
 // [ts] let warning = _dereq_(31);
 // [ts] let ReactPropTypesSecret = _dereq_(36);
 // [ts] let loggedTypeFailures = {};
 // [ts] }
 // [ts] /**
 // [ts] * Assert that the values match with the type specs.
 // [ts] * Error messages are memorized and will only be shown once.
 // [ts] *
 // [ts] * @param {object} typeSpecs Map of name to a ReactPropType
 // [ts] * @param {object} values Runtime values that need to be type-checked
 // [ts] * @param {string} location e.g. "prop", "context", "child context"
 // [ts] * @param {string} componentName Name of the component for error messages.
 // [ts] * @param {?Function} getStack Returns the component stack.
 // [ts] * @private
 // [ts] */
 // [ts] fn checkPropTypes(typeSpecs: auto, values: auto, location: auto, componentName: auto, getStack: auto) {
 // [ts] if ("development" != "production") {
 // [ts] for (let typeSpecName in typeSpecs) {
 // [ts] if (typeSpecs.hasOwnProperty(typeSpecName)) {
 // [ts] let error;
 // Prop type validation may throw. In case they do, we don't want to
 // fail the render phase where it didn't fail before. So we log it.
 // After these have been cleaned up, we'll let them throw.
 // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
 // This is intentionally an invariant that gets caught. It's the same
 // behavior as without this statement except with a better message.
 // [ts] invariant(@typeName(typeSpecs[typeSpecName]) == "function", "%s: %s type `%s` is invalid; it must be a function, usually from " + "React.PropTypes.", componentName || "React class", location, typeSpecName);
 // [ts] error = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, ReactPropTypesSecret);
 // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
 // [ts] error = ex;
 // [ts] }
 // [ts] warning(!error || error == Error, "%s: type specification of %s `%s` is invalid; the type checker " + "function must return `null` or an `Error` but returned a %s. " + "You may have forgotten to pass an argument to the type checker " + "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " + "shape all require an argument).", componentName || "React class", location, typeSpecName, @typeName(error));
 // [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
 // Only monitor this failure once because there tends to be a lot of the
 // same error.
 // [ts] loggedTypeFailures[error.message] = true;
 // [ts] let stack = if (getStack) getStack() else "";
 // [ts] warning(if (false, "Failed %s type: %s%s", location, error.message, stack != null) stack else "");
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] module.exports = checkPropTypes;
 // [ts] },{"30":30,"31":31,"36":36}],34:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] */
 // [ts] "use strict";
 // React 15.5 references this module, and assumes PropTypes are still callable in production.
 // Therefore we re-export development-only version with all the PropTypes checks here.
 // However if one is migrating to the `prop-types` npm library, they will go through the
 // `index.js` entry point, and it will branch depending on the environment.
 // [ts] let factory = _dereq_(35);
 // [ts] module.exports = (isValidElement) => {
 // It is still allowed in 15.5.
 // [ts] let throwOnDirectAccess = false;
 // [ts] return factory(isValidElement, throwOnDirectAccess);
 // [ts] };
 // [ts] },{"35":35}],35:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] */
 // [ts] "use strict";
 // [ts] let emptyFunction = _dereq_(28);
 // [ts] let invariant = _dereq_(30);
 // [ts] let warning = _dereq_(31);
 // [ts] let ReactPropTypesSecret = _dereq_(36);
 // [ts] let checkPropTypes = _dereq_(33);
 // [ts] module.exports = (isValidElement, throwOnDirectAccess) => {
 // [ts] /* global Symbol */
 // [ts] let ITERATOR_SYMBOL = @typeName(Symbol) == "function" && "__symbol_iterator";
 // [ts] let FAUX_ITERATOR_SYMBOL = "@@iterator"; // Before Symbol spec.
 // [ts] /**
 // [ts] * Returns the iterator method function contained on the iterable object.
 // [ts] *
 // [ts] * Be sure to invoke the function with the iterable as context:
 // [ts] *
 // [ts] *     var iteratorFn = getIteratorFn(myIterable);
 // [ts] *     if (iteratorFn) {
 // [ts] *       var iterator = iteratorFn.call(myIterable);
 // [ts] *       ...
 // [ts] *     }
 // [ts] *
 // [ts] * @param {?object} maybeIterable
 // [ts] * @return {?function}
 // [ts] */
 // [ts] fn getIteratorFn(maybeIterable: auto) {
 // [ts] let iteratorFn = maybeIterable && (ITERATOR_SYMBOL && maybeIterable[ITERATOR_SYMBOL] || maybeIterable[FAUX_ITERATOR_SYMBOL]);
 // [ts] if (@typeName(iteratorFn) == "function") {
 // [ts] return iteratorFn;
 // [ts] }
 // [ts] }
 // [ts] /**
 // [ts] * Collection of methods that allow declaration and validation of props that are
 // [ts] * supplied to React components. Example usage:
 // [ts] *
 // [ts] *   var Props = require('ReactPropTypes');
 // [ts] *   var MyArticle = React.createClass({
 // [ts] *     propTypes: {
 // [ts] *       // An optional string prop named "description".
 // [ts] *       description: Props.string,
 // [ts] *
 // [ts] *       // A required enum prop named "category".
 // [ts] *       category: Props.oneOf(['News','Photos']).isRequired,
 // [ts] *
 // [ts] *       // A prop named "dialog" that requires an instance of Dialog.
 // [ts] *       dialog: Props.instanceOf(Dialog).isRequired
 // [ts] *     },
 // [ts] *     render: function() { ... }
 // [ts] *   });
 // [ts] *
 // [ts] * A more formal specification of how these methods are used:
 // [ts] *
 // [ts] *   type := array|bool|func|object|number|string|oneOf([...])|instanceOf(...)
 // [ts] *   decl := ReactPropTypes.{type}(.isRequired)?
 // [ts] *
 // [ts] * Each and every declaration produces a function with the same signature. This
 // [ts] * allows the creation of custom validation functions. For example:
 // [ts] *
 // [ts] *  var MyLink = React.createClass({
 // [ts] *    propTypes: {
 // [ts] *      // An optional string or URI prop named "href".
 // [ts] *      href: function(props, propName, componentName) {
 // [ts] *        var propValue = props[propName];
 // [ts] *        if (propValue != null && typeof propValue !== 'string' &&
 // [ts] *            !(propValue instanceof URI)) {
 // [ts] *          return new Error(
 // [ts] *            'Expected a string or an URI for ' + propName + ' in ' +
 // [ts] *            componentName
 // [ts] *          );
 // [ts] *        }
 // [ts] *      }
 // [ts] *    },
 // [ts] *    render: function() {...}
 // [ts] *  });
 // [ts] *
 // [ts] * @internal
 // [ts] */
 // [ts] let ANONYMOUS = "<<anonymous>>";
 // Important!
 // Keep this list in sync with production version in `./factoryWithThrowingShims.js`.
 // [ts] let ReactPropTypes = {
 // [ts] array: createPrimitiveTypeChecker("array"),
 // [ts] bool: createPrimitiveTypeChecker("boolean"),
 // [ts] func: createPrimitiveTypeChecker("function"),
 // [ts] number: createPrimitiveTypeChecker("number"),
 // [ts] object: createPrimitiveTypeChecker("object"),
 // [ts] string: createPrimitiveTypeChecker("string"),
 // [ts] symbol: createPrimitiveTypeChecker("symbol"),
 // [ts] any: createAnyTypeChecker(),
 // [ts] arrayOf: createArrayOfTypeChecker,
 // [ts] element: createElementTypeChecker(),
 // [ts] instanceOf: createInstanceTypeChecker,
 // [ts] node: createNodeChecker(),
 // [ts] objectOf: createObjectOfTypeChecker,
 // [ts] oneOf: createEnumTypeChecker,
 // [ts] oneOfType: createUnionTypeChecker,
 // [ts] shape: createShapeTypeChecker
 // [ts] };
 // [ts] /**
 // [ts] * inlined Object.is polyfill to avoid requiring consumers ship their own
 // [ts] * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/is
 // [ts] */
 // [ts] /*eslint-disable no-self-compare*/
 // [ts] fn is(x: auto, y: auto) {
 // SameValue algorithm
 // [ts] if (x == y) {
 // Steps 1-5, 7-10
 // Steps 6.b-6.e: +0 != -0
 // [ts] return x != 0 || 1 / x == 1 / y;
 // [ts] } else {
 // Step 6.a: NaN == NaN
 // [ts] return x != x && y != y;
 // [ts] }
 // [ts] }
 // [ts] /*eslint-enable no-self-compare*/
 // [ts] /**
 // [ts] * We use an Error-like object for backward compatibility as people may call
 // [ts] * PropTypes directly and inspect their output. However, we don't use real
 // [ts] * Errors anymore. We don't inspect their stack anyway, and creating them
 // [ts] * is prohibitively expensive if they are created too often, such as what
 // [ts] * happens in oneOfType() for any type before the one that matched.
 // [ts] */
 // [ts] fn PropTypeError(message: auto) {
 // [ts] _.message = message;
 // [ts] _.stack = "";
 // [ts] }
 // Make `instanceof Error` still work for returned errors.
 // [ts] PropTypeError.prototype = Error.prototype;
 // [ts] fn createChainableTypeChecker(validate: auto) {
 // [ts] if ("development" != "production") {
 // [ts] let manualPropTypeCallCache = {};
 // [ts] let manualPropTypeWarningCount = 0;
 // [ts] }
 // [ts] fn checkType(isRequired: auto, props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto, secret: auto) {
 // [ts] componentName = componentName || ANONYMOUS;
 // [ts] propFullName = propFullName || propName;
 // [ts] if (secret != ReactPropTypesSecret) {
 // [ts] if (throwOnDirectAccess) {
 // New behavior only for users of `prop-types` package
 // [ts] invariant(
 // [ts] false,
 // [ts] "Calling PropTypes validators directly is not supported by the `prop-types` package. " +
 // [ts] "Use `PropTypes.checkPropTypes()` to call them. " +
 // [ts] "Read more at http://fb.me/use-check-prop-types"
 // [ts] );
 // [ts] } else if ("development" != "production" && @typeName(console) != "undefined") {
 // Old behavior for people using React.PropTypes
 // [ts] let cacheKey = componentName + ":" + propName;
 // [ts] if (
 // [ts] !manualPropTypeCallCache[cacheKey] &&
 // Avoid spamming the console because they are often not actionable except for lib authors
 // [ts] manualPropTypeWarningCount < 3
 // [ts] ) {
 // [ts] warning(
 // [ts] false,
 // [ts] "You are manually calling a React.PropTypes validation " +
 // [ts] "function for the `%s` prop on `%s`. This is deprecated " +
 // [ts] "and will throw in the standalone `prop-types` package. " +
 // [ts] "You may be seeing this warning due to a third-party PropTypes " +
 // [ts] "library. See https://fb.me/react-warning-dont-call-proptypes " + "for details.",
 // [ts] propFullName,
 // [ts] componentName
 // [ts] );
 // [ts] manualPropTypeCallCache[cacheKey] = true;
 // [ts] manualPropTypeWarningCount++;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] if (props[propName] == null) {
 // [ts] if (isRequired) {
 // [ts] if (props[propName] == null) {
 // [ts] return PropTypeError.init("The " + location + " `" + propFullName + "` is marked as required " + ("in `" + componentName + "`, but its value is `null`."));
 // [ts] }
 // [ts] return PropTypeError.init("The " + location + " `" + propFullName + "` is marked as required in " + ("`" + componentName + "`, but its value is `undefined`."));
 // [ts] }
 // [ts] return null;
 // [ts] } else {
 // [ts] return validate(props, propName, componentName, location, propFullName);
 // [ts] }
 // [ts] }
 // [ts] let chainedCheckType = checkType.bind(null, false);
 // [ts] chainedCheckType.isRequired = checkType.bind(null, true);
 // [ts] return chainedCheckType;
 // [ts] }
 // [ts] fn createPrimitiveTypeChecker(expectedType: auto) {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto, secret: auto) {
 // [ts] let propValue = props[propName];
 // [ts] let propType = getPropType(propValue);
 // [ts] if (propType != expectedType) {
 // `propValue` being instance of, say, date/regexp, pass the 'object'
 // check, but we can offer a more precise error message here rather than
 // 'of type `object`'.
 // [ts] let preciseType = getPreciseType(propValue);
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + preciseType + "` supplied to `" + componentName + "`, expected ") + ("`" + expectedType + "`."));
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createAnyTypeChecker() {
 // [ts] return createChainableTypeChecker(emptyFunction.thatReturnsNull);
 // [ts] }
 // [ts] fn createArrayOfTypeChecker(typeChecker: auto) {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] if (@typeName(typeChecker) != "function") {
 // [ts] return PropTypeError.init("Property `" + propFullName + "` of component `" + componentName + "` has invalid PropType notation inside arrayOf.");
 // [ts] }
 // [ts] let propValue = props[propName];
 // [ts] if (!Array.isArray(propValue)) {
 // [ts] let propType = getPropType(propValue);
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + propType + "` supplied to `" + componentName + "`, expected an array."));
 // [ts] }
 // [ts] let i = 0;
 // [ts] while (i < propValue.length) { // [cap-migrate] for-step: i++
 // [ts] let error = typeChecker(propValue, i, componentName, location, propFullName + "[" + i + "]", ReactPropTypesSecret);
 // [ts] if (error == Error) {
 // [ts] return error;
 // [ts] }
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createElementTypeChecker() {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] let propValue = props[propName];
 // [ts] if (!isValidElement(propValue)) {
 // [ts] let propType = getPropType(propValue);
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + propType + "` supplied to `" + componentName + "`, expected a single ReactElement."));
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createInstanceTypeChecker(expectedClass: auto) {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] if (!(props[propName] == expectedClass)) {
 // [ts] let expectedClassName = expectedClass.name || ANONYMOUS;
 // [ts] let actualClassName = getClassName(props[propName]);
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + actualClassName + "` supplied to `" + componentName + "`, expected ") + ("instance of `" + expectedClassName + "`."));
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createEnumTypeChecker(expectedValues: auto) {
 // [ts] if (!Array.isArray(expectedValues)) {
 // [ts] if ("development" != "production") warning(false, "Invalid argument supplied to oneOf, expected an instance of array.") else void 0;
 // [ts] return emptyFunction.thatReturnsNull;
 // [ts] }
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] let propValue = props[propName];
 // [ts] let i = 0;
 // [ts] while (i < expectedValues.length) { // [cap-migrate] for-step: i++
 // [ts] if (is(propValue, expectedValues[i])) {
 // [ts] return null;
 // [ts] }
 // [ts] }
 // [ts] let valuesString = JSON.stringify(expectedValues);
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of value `" + propValue + "` " + ("supplied to `" + componentName + "`, expected one of " + valuesString + "."));
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createObjectOfTypeChecker(typeChecker: auto) {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] if (@typeName(typeChecker) != "function") {
 // [ts] return PropTypeError.init("Property `" + propFullName + "` of component `" + componentName + "` has invalid PropType notation inside objectOf.");
 // [ts] }
 // [ts] let propValue = props[propName];
 // [ts] let propType = getPropType(propValue);
 // [ts] if (propType != "object") {
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + propType + "` supplied to `" + componentName + "`, expected an object."));
 // [ts] }
 // [ts] for (let key in propValue) {
 // [ts] if (propValue.hasOwnProperty(key)) {
 // [ts] let error = typeChecker(propValue, key, componentName, location, propFullName + "." + key, ReactPropTypesSecret);
 // [ts] if (error == Error) {
 // [ts] return error;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createUnionTypeChecker(arrayOfTypeCheckers: auto) {
 // [ts] if (!Array.isArray(arrayOfTypeCheckers)) {
 // [ts] if ("development" != "production") warning(false, "Invalid argument supplied to oneOfType, expected an instance of array.") else void 0;
 // [ts] return emptyFunction.thatReturnsNull;
 // [ts] }
 // [ts] let i = 0;
 // [ts] while (i < arrayOfTypeCheckers.length) { // [cap-migrate] for-step: i++
 // [ts] let checker = arrayOfTypeCheckers[i];
 // [ts] if (@typeName(checker) != "function") {
 // [ts] warning(
 // [ts] false,
 // [ts] "Invalid argument supplid to oneOfType. Expected an array of check functions, but " +
 // [ts] "received %s at index %s.",
 // [ts] getPostfixForTypeWarning(checker),
 // [ts] i
 // [ts] );
 // [ts] return emptyFunction.thatReturnsNull;
 // [ts] }
 // [ts] }
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] let i = 0;
 // [ts] while (i < arrayOfTypeCheckers.length) { // [cap-migrate] for-step: i++
 // [ts] let checker = arrayOfTypeCheckers[i];
 // [ts] if (checker(props, propName, componentName, location, propFullName, ReactPropTypesSecret) == null) {
 // [ts] return null;
 // [ts] }
 // [ts] }
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` supplied to " + ("`" + componentName + "`."));
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createNodeChecker() {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] if (!isNode(props[propName])) {
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` supplied to " + ("`" + componentName + "`, expected a ReactNode."));
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn createShapeTypeChecker(shapeTypes: auto) {
 // [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
 // [ts] let propValue = props[propName];
 // [ts] let propType = getPropType(propValue);
 // [ts] if (propType != "object") {
 // [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type `" + propType + "` " + ("supplied to `" + componentName + "`, expected `object`."));
 // [ts] }
 // [ts] for (let key in shapeTypes) {
 // [ts] let checker = shapeTypes[key];
 // [ts] if (!checker) {
 // [ts] continue;
 // [ts] }
 // [ts] let error = checker(propValue, key, componentName, location, propFullName + "." + key, ReactPropTypesSecret);
 // [ts] if (error) {
 // [ts] return error;
 // [ts] }
 // [ts] }
 // [ts] return null;
 // [ts] }
 // [ts] return createChainableTypeChecker(validate);
 // [ts] }
 // [ts] fn isNode(propValue: auto) {
 // [ts] switch (@typeName(propValue)) {
 // [ts] "number", "string", "undefined" => {
 // [ts] return true;
 // [ts] },
 // [ts] "boolean" => {
 // [ts] return !propValue;
 // [ts] },
 // [ts] "object" => {
 // [ts] if (Array.isArray(propValue)) {
 // [ts] return propValue.every(isNode);
 // [ts] }
 // [ts] if (propValue == null || isValidElement(propValue)) {
 // [ts] return true;
 // [ts] }
 // [ts] let iteratorFn = getIteratorFn(propValue);
 // [ts] if (iteratorFn) {
 // [ts] let iterator = iteratorFn.call(propValue);
 // [ts] let step;
 // [ts] if (iteratorFn != propValue.entries) {
 // [ts] while (!(step = iterator.next()).done) {
 // [ts] if (!isNode(step.value)) {
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] } else {
 // Iterator will provide entry [k,v] tuples rather than values.
 // [ts] while (!(step = iterator.next()).done) {
 // [ts] let entry = step.value;
 // [ts] if (entry) {
 // [ts] if (!isNode(entry[1])) {
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] } else {
 // [ts] return false;
 // [ts] }
 // [ts] return true;
 // [ts] },
 // [ts] else => {
 // [ts] return false;
 // [ts] }
 // [ts] }
 // [ts] }
 // [ts] fn isSymbol(propType: auto, propValue: auto) {
 // Native Symbol.
 // [ts] if (propType == "symbol") {
 // [ts] return true;
 // [ts] }
 // 19.4.3.5 Symbol.prototype[@@toStringTag] === 'Symbol'
 // [ts] if (propValue["@@toStringTag"] == "Symbol") {
 // [ts] return true;
 // [ts] }
 // Fallback for non-spec compliant Symbols which are polyfilled.
 // [ts] if (@typeName(Symbol) == "function" && propValue == Symbol) {
 // [ts] return true;
 // [ts] }
 // [ts] return false;
 // [ts] }
 // Equivalent of `typeof` but with special handling for array and regexp.
 // [ts] fn getPropType(propValue: auto) {
 // [ts] let propType = @typeName(propValue);
 // [ts] if (Array.isArray(propValue)) {
 // [ts] return "array";
 // [ts] }
 // [ts] if (propValue == RegExp) {
 // Old webkits (at least until Android 4.0) return 'function' rather than
 // 'object' for typeof a RegExp. We'll normalize this here so that /bla/
 // passes PropTypes.object.
 // [ts] return "object";
 // [ts] }
 // [ts] if (isSymbol(propType, propValue)) {
 // [ts] return "symbol";
 // [ts] }
 // [ts] return propType;
 // [ts] }
 // This handles more types than `getPropType`. Only used for error messages.
 // See `createPrimitiveTypeChecker`.
 // [ts] fn getPreciseType(propValue: auto) {
 // [ts] if (@typeName(propValue) == "undefined" || propValue == null) {
 // [ts] return "" + propValue;
 // [ts] }
 // [ts] let propType = getPropType(propValue);
 // [ts] if (propType == "object") {
 // [ts] if (propValue == Date) {
 // [ts] return "date";
 // [ts] } else if (propValue == RegExp) {
 // [ts] return "regexp";
 // [ts] }
 // [ts] }
 // [ts] return propType;
 // [ts] }
 // Returns a string that is postfixed to a warning about an invalid type.
 // For example, "undefined" or "of type array"
 // [ts] fn getPostfixForTypeWarning(value: auto) {
 // [ts] let type = getPreciseType(value);
 // [ts] switch (type) {
 // [ts] "array", "object" => {
 // [ts] return "an " + type;
 // [ts] },
 // [ts] "boolean", "date", "regexp" => {
 // [ts] return "a " + type;
 // [ts] },
 // [ts] else => {
 // [ts] return type;
 // [ts] }
 // [ts] }
 // [ts] }
 // Returns class name of the object, if any.
 // [ts] fn getClassName(propValue: auto) {
 // [ts] if (!propValue.constructor || !propValue.constructor.name) {
 // [ts] return ANONYMOUS;
 // [ts] }
 // [ts] return propValue.constructor.name;
 // [ts] }
 // [ts] ReactPropTypes.checkPropTypes = checkPropTypes;
 // [ts] ReactPropTypes.PropTypes = ReactPropTypes;
 // [ts] return ReactPropTypes;
 // [ts] };
 // [ts] },{"28":28,"30":30,"31":31,"33":33,"36":36}],36:[(_dereq_, module, exports) => {
 // [ts] /**
 // [ts] * Copyright 2013-present, Facebook, Inc.
 // [ts] * All rights reserved.
 // [ts] *
 // [ts] * This source code is licensed under the BSD-style license found in the
 // [ts] * LICENSE file in the root directory of this source tree. An additional grant
 // [ts] * of patent rights can be found in the PATENTS file in the same directory.
 // [ts] */
 // [ts] "use strict";
 // [ts] let ReactPropTypesSecret = "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED";
 // [ts] module.exports = ReactPropTypesSecret;
 // [ts] },{}]},{},[16])(16)
 // [ts] });
