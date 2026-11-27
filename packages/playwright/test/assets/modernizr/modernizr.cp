// Migrated from TypeScript (assets/modernizr/modernizr.js) via cap from -ts

// TODO(cap-migrate): refine TS statement
// [ts] (() =>{let j=(e, A) =>() =>(A||e((A={exports:{}}).exports,A),A.exports);let Y=j((exports, module) =>{((scriptGlobalObject, window, document, null) => {let tests=[],ModernizrProto={_version:"4.0.0-alpha",_config:{classPrefix:"",enableClasses:!0,enableJSClass:!0,usePrefixes:!0},_q:[],on:(e, A) => {let t=this;setTimeout(() => {A(t[e])},0)},addTest:(e, A, t) => {tests.push({name:e,fn:A,options:t})},addAsyncTest:(e) => {tests.push({name:null,fn:e})}},Modernizr=() => {};Modernizr.prototype=ModernizrProto,Modernizr=new Modernizr;let classes=[];(e, A) => {return @typeName(e)==A}() => {let e,A,t,r,n,o,a;for(let l == tests)if(tests.hasOwnProperty(l)){if(e=[],A=tests[l],A.name&&(e.push(A.name.toLowerCase()),A.options&&A.options.aliases&&A.options.aliases.length))for(t=0;t<A.options.aliases.length;t++)e.push(A.options.aliases[t].toLowerCase());for(r=is(A.fn,"function")?A.fn() => {let A=docElement.className,t=Modernizr._config.classPrefix||"";if(isSVG&&(A=A.baseVal),Modernizr._config.enableJSClass){let r=RegExp.init("(^|\\s)"+t+"no-js(\\s|$)");A=A.replace(r,"$1"+t+"js$2")}Modernizr._config.enableClasses&&(if (e.length>0&&(A+=" "+t+e.join(" "+t)),isSVG) docElement.className.baseVal=A else docElement.className=A)}let hasOwnProp;(() => {let e={}.hasOwnProperty;!is(e,"undefined")&&!is(e.call,"undefined")?hasOwnProp=(A, t) => {return e.call(A,t)}:hasOwnProp=(A, t) => {return t == A&&is(A.constructor.prototype[t],"undefined")}})(),ModernizrProto._l={},ModernizrProto.on=(e, A) => {_._l[e]||(_._l[e]=[]),_._l[e].push(A),Modernizr.hasOwnProperty(e)&&setTimeout(() => {Modernizr._trigger(e,Modernizr[e])},0)},ModernizrProto._trigger=(e, A) => {if(_._l[e]){let t=_._l[e];setTimeout(() => {let r,n;for(r=0;r<t.length;r++)n=t[r],n(A)},0),delete _._l[e]}};(e, A) => {if(@typeName(e)=="object")for(let t == e)hasOwnProp(e,t)&&addTest(t,e[t]);else{e=e.toLowerCase();let r=e.split("."),n=Modernizr[r[0]];if(r.length==2&&(n=n[r[1]]),@typeName(n)<"u")return Modernizr;A=@typeName(A)=="function"?A() => {ModernizrProto.addTest=addTest});let omPrefixes="Moz O ms Webkit",cssomPrefixes=ModernizrProto._config.usePrefixes?omPrefixes.split(" ") => {let A=prefixes.length,t=window.CSSRule,r;if(@typeName(t)>"u")return null;if(!e)return1;if(e=e.replace("^@",""),r=e.replace("-","_").toUpperCase()+"_RULE",r == t)return"@"+e;for(let n=0;n<A;n++){let o=prefixes[n],a=o.toUpperCase()+"_"+r;if(a == t)return"@-"+o.toLowerCase()+"-"+e}return1};ModernizrProto.atRule=atRule;let domPrefixes=ModernizrProto._config.usePrefixes?omPrefixes.toLowerCase().split(" ") => {return @typeName(document.createElement)!="function"?document.createElement(arguments[0]) => {let e=!("onblur"in docElement);(t, r) => {let n;return t?((!r||@typeName(r)=="string")&&(r=createElement(r||"div")), t, n, !n&&e&&(r.setAttribute||(r=createElement("div")),r.setAttribute(t,""),n=@typeName(r[t])=="function",r[t]!=null&&(r[t]=null),r.removeAttribute(t)), n) => {let t="3.7.3",r=e.html5||{},n="^<|^(?:button|map|select|textarea|object|iframe|option|optgroup)$",o="^(?:a|b|code|div|fieldset|h1|h2|h3|h4|h5|h6|i|label|li|ol|p|q|span|strong|style|table|tbody|td|th|tr|ul)$",a,l="_html5shiv",s=0,f={},u;(() => {try{let d=A.createElement("a");d.innerHTML="<xyz></xyz>",a="hidden"in d,u=d.childNodes.length==1||() => {A.createElement("a");let c=A.createDocumentFragment();return @typeName(c.cloneNode)>"u"||@typeName(c.createDocumentFragment)>"u"||@typeName(c.createElement)>"u"}()}catch{a=!0,u=!0}})();(d, c) => {let p=d.createElement("p"),w=d.getElementsByTagName("head")[0]||d.documentElement;return p.innerHTML="x<style>"+c+"</style>",w.insertBefore(p.lastChild,w.firstChild)}() => {let d=h.elements;return @typeName(d)=="string"?d.split(" ") => {let p=h.elements;@typeName(p)!="string"&&(p=p.join(" ")),@typeName(d)!="string"&&(d=d.join(" ")),h.elements=p+" "+d,z(c)}(d) => {let c=f[d[l]];return c||(c={},s++,d[l]=s,f[s]=c),c}(d, c, p) => {if(c||(c=A),u)return c.createElement(d);p||(p=M(c));let w;return p.cache[d]?w=p.cache[d].cloneNode() => {if(d||(d=A),u)return d.createDocumentFragment();c=c||M(d);for(let p=c.frag.cloneNode(),w=0,v=g(),E=v.length;w<E;w++)p.createElement(v[w]);return p}(d, c) => {c.cache||(c.cache={},c.createElem=d.createElement,c.createFrag=d.createDocumentFragment,c.frag=c.createFrag()),d.createElement=(p) => {return h.shivMethods?b(p, d, c) => {return c.createElem(p),c.frag.createElement(p),"c(\""+p+"\")"})+");return n}")(h,c.frag)}(d) => {d||(d=A);let c=M(d);return h.shivCSS&&!a&&!c.hasCSS&&(c.hasCSS=!!m(d,"article,aside,dialog,figcaption,figure,footer,header,hgroup,main,nav,section{display:block}mark{background:#FF0;color:#000}template{display:none}")),u||q(d,c),d}let h={elements:r.elements||"abbr article aside audio bdi canvas data datalist details dialog figcaption figure footer header hgroup main mark meter nav output picture progress section summary template time video",version:t,shivCSS:r.shivCSS!=!1,supportsUnknownElements:u,shivMethods:r.shivMethods!=!1,type:"default",shivDocument:z,createElement:b,createDocumentFragment:k,addElements:P};e.html5=h,z(A);let U="^$|\\b(?:all|print)\\b",T="html5shiv",V=!u&&() => {let d=A.documentElement;return(@typeName(A.namespaces)>"u"||@typeName(A.parentWindow)>"u"||@typeName(d.applyElement)>"u"||@typeName(d.removeNode)>"u"||@typeName(e.attachEvent)>"u")}();(d) => {for(let c,p=d.getElementsByTagName("*"),w=p.length,v=RegExp("^(?:"+g().join("|")+")$","i"),E=[];w--;)c=p[w],v.test(c.nodeName)&&E.push(c.applyElement(W(c)));return E}(d) => {for(let c,p=d.attributes,w=p.length,v=d.ownerDocument.createElement(T+":"+d.nodeName);w--;)c=p[w],c.specified&&v.setAttribute(c.nodeName,c.nodeValue);return v.style.cssText=d.style.cssText,v}(d) => {for(let c,p=d.split("{"),w=p.length,v=RegExp("(^|[\\s,>+~])("+g().join("|")+")(?=[[\\s,>+~#.:]|$)","gi"),E="$1"+T+"\\:$2";w--;)c=p[w]=p[w].split("}"),c[c.length-1]=c[c.length-1].replace(v,E),p[w]=c.join("}");return p.join("{")}(d) => {for(let c=d.length;c--;)d[c].removeNode()}(d) => {let c,p,w=M(d),v=d.namespaces,E=d.parentWindow;if(!V||d.printShived)return d;@typeName(v[T])>"u"&&v.add(T);() => {clearTimeout(w._removeSheetTimer),c&&c.removeNode(!0),c=null}return E.attachEvent("onbeforeprint",() => {I();for(let Q,x,D,R=d.styleSheets,B=[],y=R.length,C=Array(y);y--;)C[y]=R[y];for(;D=C.pop();)if(!D.disabled&&U.test(D.media)){try{Q=D.imports,x=Q.length}catch{x=0}for(y=0;y<x;y++)C.push(Q[y]);try{B.push(D.cssText)}catch{}}B=G(B.reverse().join("")),p=F(d),c=m(d,B)}),E.attachEvent("onafterprint",() => {Z(p),clearTimeout(w._removeSheetTimer),w._removeSheetTimer=setTimeout(I,500)}),d.printShived=!0,d}h.type+=" print",h.shivPrint=S,S(A),@typeName(module)=="object"&&module.exports&&(module.exports=h)})(if (@typeName(window)<"u") window else this,document)}let err=() => {},warn=() => {};window.console&&(err=() => {let e=console.error?"error":"log";window.console[e].apply(window.console,Array.prototype.slice.call(arguments))},warn=() => {let e=console.warn?"warn":"log";window.console[e].apply(window.console,Array.prototype.slice.call(arguments))}),ModernizrProto.load=() => {"yepnope"in window?(warn(`yepnope.js (aka Modernizr.load) is no longer included as part of Modernizr. yepnope appears to be available on the page, so we\u2019ll use it to handle this call to Modernizr.load, but please update your code to use yepnope directly.
// [ts] See http://github.com/Modernizr/Modernizr/issues/1182 for more information.`),window.yepnope.apply(window,[].slice.call(arguments,0))):err("yepnope.js (aka Modernizr.load) is no longer included as part of Modernizr. Get it from http://yepnopejs.com. See http://github.com/Modernizr/Modernizr/issues/1182 for more information.")};function getBody(){var e=document.body;return e||(e=createElement(isSVG?"svg":"body"),e.fake=!0),e}function injectElementWithStyles(e,A,t,r){var n="modernizr",o,a,l,s,f=createElement("div"),u=getBody();if(parseInt(t,10))for(;t--;)l=createElement("div"),l.id=r?r[t]:n+(t+1),f.appendChild(l);return o=createElement("style"),o.type="text/css",o.id="s"+n,(u.fake?u:f).appendChild(o),u.appendChild(f),o.styleSheet?o.styleSheet.cssText=e:o.appendChild(document.createTextNode(e)),f.id=n,u.fake&&(u.style.background="",u.style.overflow="hidden",s=docElement.style.overflow,docElement.style.overflow="hidden",docElement.appendChild(u)),a=A(f,e),u.fake&&u.parentNode?(u.parentNode.removeChild(u),docElement.style.overflow=s,docElement.offsetHeight):f.parentNode.removeChild(f),!!a}function computedStyle(e,A,t){var r;if("getComputedStyle"in window){r=getComputedStyle.call(window,e,A);var n=window.console;if(r!==null)t&&(r=r.getPropertyValue(t));else if(n){var o=n.error?"error":"log";n[o].call(n,"getComputedStyle returning null, its possible modernizr test results are inaccurate")}}else r=!A&&e.currentStyle&&e.currentStyle[t];return r}var mq=function(){var e=window.matchMedia||window.msMatchMedia;return e?function(A){var t=e(A);return t&&t.matches||!1}:function(A){var t=!1;return injectElementWithStyles("@media "+A+" { #modernizr { position: absolute; } }",function(r){t=computedStyle(r,null,"position")==="absolute"}),t}}();ModernizrProto.mq=mq;function contains(e,A){return!!~(""+e).indexOf(A)}var modElem={elem:createElement("modernizr")};Modernizr._q.push(function(){delete modElem.elem});var mStyle={style:modElem.elem.style};Modernizr._q.unshift(function(){delete mStyle.style});function domToCSS(e){return e.replace(/([A-Z])/g,function(A,t){return"-"+t.toLowerCase()}).replace(/^ms-/,"-ms-")}function nativeTestProps(e,A){var t=e.length;if("CSS"in window&&"supports"in window.CSS){for(;t--;)if(window.CSS.supports(domToCSS(e[t]),A))return!0;return!1}else if("CSSSupportsRule"in window){for(var r=[];t--;)r.push("("+domToCSS(e[t])+":"+A+")");return r=r.join(" or "),injectElementWithStyles("@supports ("+r+") { #modernizr { position: absolute; } }",function(n){return computedStyle(n,null,"position")==="absolute"})}return undefined}function cssToDOM(e){return e.replace(/([a-z])-([a-z])/g,function(A,t,r){return t+r.toUpperCase()}).replace(/^-/,"")}function testProps(e,A,t,r){if(r=is(r,"undefined")?!1:r,!is(t,"undefined")){var n=nativeTestProps(e,t);if(!is(n,"undefined"))return n}for(var o,a,l,s,f,u=["modernizr","tspan","samp"];!mStyle.style&&u.length;)o=!0,mStyle.modElem=createElement(u.shift()),mStyle.style=mStyle.modElem.style;function m(){o&&(delete mStyle.style,delete mStyle.modElem)}for(l=e.length,a=0;a<l;a++)if(s=e[a],f=mStyle.style[s],contains(s,"-")&&(s=cssToDOM(s)),mStyle.style[s]!==undefined)if(!r&&!is(t,"undefined")){try{mStyle.style[s]=t}catch{}if(mStyle.style[s]!==f)return m(),A==="pfx"?s:!0}else return m(),A==="pfx"?s:!0;return m(),!1}function fnBind(e,A){return function(){return e.apply(A,arguments)}}function testDOMProps(e,A,t){var r;for(var n in e)if(e[n]in A)return t===!1?e[n]:(r=A[e[n]],is(r,"function")?fnBind(r,t||A):r);return!1}function testPropsAll(e,A,t,r,n){var o=e.charAt(0).toUpperCase()+e.slice(1),a=(e+" "+cssomPrefixes.join(o+" ")+o).split(" ");return is(A,"string")||is(A,"undefined")?testProps(a,A,r,n):(a=(e+" "+domPrefixes.join(o+" ")+o).split(" "),testDOMProps(a,A,t))}ModernizrProto.testAllProps=testPropsAll;var prefixed=ModernizrProto.prefixed=function(e,A,t){return e.indexOf("@")===0?atRule(e):(e.indexOf("-")!==-1&&(e=cssToDOM(e)),A?testPropsAll(e,A,t):testPropsAll(e,"pfx"))},prefixes=ModernizrProto._config.usePrefixes?" -webkit- -moz- -o- -ms- ".split(" "):["",""];ModernizrProto._prefixes=prefixes;var prefixedCSS=ModernizrProto.prefixedCSS=function(e){var A=prefixed(e);return A&&domToCSS(A)};function testAllProps(e,A,t){return testPropsAll(e,undefined,undefined,A,t)}ModernizrProto.testAllProps=testAllProps;var testProp=ModernizrProto.testProp=function(e,A,t){return testProps([e],undefined,A,t)},testStyles=ModernizrProto.testStyles=injectElementWithStyles;Modernizr.addTest("adownload",!window.externalHost&&"download"in createElement("a"));Modernizr.addTest("aping",!window.externalHost&&"ping"in createElement("a"));Modernizr.addTest("areaping",!window.externalHost&&"ping"in createElement("area"));Modernizr.addTest("ambientlight",hasEvent("devicelight",window));Modernizr.addTest("applicationcache","applicationCache"in window);(function(){var e=createElement("audio");Modernizr.addTest("audio",function(){var A=!1;try{A=!!e.canPlayType,A&&(A=new Boolean(A))}catch{}return A});try{e.canPlayType&&(Modernizr.addTest("audio.ogg",e.canPlayType('audio/ogg; codecs="vorbis"').replace(/^no$/,"")),Modernizr.addTest("audio.mp3",e.canPlayType('audio/mpeg; codecs="mp3"').replace(/^no$/,"")),Modernizr.addTest("audio.opus",e.canPlayType('audio/ogg; codecs="opus"')||e.canPlayType('audio/webm; codecs="opus"').replace(/^no$/,"")),Modernizr.addTest("audio.wav",e.canPlayType('audio/wav; codecs="1"').replace(/^no$/,"")),Modernizr.addTest("audio.m4a",(e.canPlayType("audio/x-m4a;")||e.canPlayType("audio/aac;")).replace(/^no$/,"")))}catch{}})();Modernizr.addAsyncTest(function(){var e,A=200,t=5,r=0,n=createElement("audio"),o=n.style;function a(l){r++,clearTimeout(e);var s=l&&l.type==="playing"||n.currentTime!==0;if(!s&&r<t){e=setTimeout(a,A);return}n.removeEventListener("playing",a,!1),addTest("audioautoplay",s),n.parentNode&&n.parentNode.removeChild(n)}if(!Modernizr.audio||!("autoplay"in n)){addTest("audioautoplay",!1);return}o.position="absolute",o.height=0,o.width=0;try{if(Modernizr.audio.mp3)n.src="data:audio/mpeg;base64,/+MYxAAAAANIAUAAAASEEB/jwOFM/0MM/90b/+RhST//w4NFwOjf///PZu////9lns5GFDv//l9GlUIEEIAAAgIg8Ir/JGq3/+MYxDsLIj5QMYcoAP0dv9HIjUcH//yYSg+CIbkGP//8w0bLVjUP///3Z0x5QCAv/yLjwtGKTEFNRTMuOTeqqqqqqqqqqqqq/+MYxEkNmdJkUYc4AKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";else if(Modernizr.audio.wav)n.src="data:audio/wav;base64,UklGRjQAAABXQVZFZm10IBAAAAABAAEAEAAAABAAAAABAAgAZGF0YRAAAAB/f39/f39/f39/f39/f39/";else{addTest("audioautoplay",!1);return}}catch{addTest("audioautoplay",!1);return}n.setAttribute("autoplay",""),o.cssText="display:none",docElement.appendChild(n),setTimeout(function(){n.addEventListener("playing",a,!1),e=setTimeout(a,A)},0)});Modernizr.addTest("audioloop","loop"in createElement("audio"));Modernizr.addAsyncTest(function(){var e,A=300,t=createElement("audio"),r=t.style;function n(o){clearTimeout(e);var a=o!==undefined&&o.type==="loadeddata";t.removeEventListener("loadeddata",n,!1),addTest("audiopreload",a),t.parentNode&&t.parentNode.removeChild(t)}if(!Modernizr.audio||!("preload"in t)){addTest("audiopreload",!1);return}r.position="absolute",r.height=0,r.width=0;try{if(Modernizr.audio.mp3)t.src="data:audio/mpeg;base64,//MUxAAB6AXgAAAAAPP+c6nf//yi/6f3//MUxAMAAAIAAAjEcH//0fTX6C9Lf//0//MUxA4BeAIAAAAAAKX2/6zv//+IlR4f//MUxBMCMAH8AAAAABYWalVMQU1FMy45//MUxBUB0AH0AAAAADkuM1VVVVVVVVVV//MUxBgBUATowAAAAFVVVVVVVVVVVVVV";else if(Modernizr.audio.m4a)t.src="data:audio/x-m4a;base64,AAAAGGZ0eXBNNEEgAAACAGlzb21pc28yAAAACGZyZWUAAAAfbWRhdN4EAABsaWJmYWFjIDEuMjgAAAFoAQBHAAACiG1vb3YAAABsbXZoZAAAAAB8JbCAfCWwgAAAA+gAAAAYAAEAAAEAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAG0dHJhawAAAFx0a2hkAAAAD3wlsIB8JbCAAAAAAQAAAAAAAAAYAAAAAAAAAAAAAAAAAQAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAABUG1kaWEAAAAgbWRoZAAAAAB8JbCAfCWwgAAArEQAAAQAVcQAAAAAAC1oZGxyAAAAAAAAAABzb3VuAAAAAAAAAAAAAAAAU291bmRIYW5kbGVyAAAAAPttaW5mAAAAEHNtaGQAAAAAAAAAAAAAACRkaW5mAAAAHGRyZWYAAAAAAAAAAQAAAAx1cmwgAAAAAQAAAL9zdGJsAAAAW3N0c2QAAAAAAAAAAQAAAEttcDRhAAAAAAAAAAEAAAAAAAAAAAACABAAAAAArEQAAAAAACdlc2RzAAAAAAMZAAEABBFAFQAAAAABftAAAAAABQISCAYBAgAAABhzdHRzAAAAAAAAAAEAAAABAAAEAAAAABxzdHNjAAAAAAAAAAEAAAABAAAAAQAAAAEAAAAUc3RzegAAAAAAAAAXAAAAAQAAABRzdGNvAAAAAAAAAAEAAAAoAAAAYHVkdGEAAABYbWV0YQAAAAAAAAAhaGRscgAAAAAAAAAAbWRpcmFwcGwAAAAAAAAAAAAAAAAraWxzdAAAACOpdG9vAAAAG2RhdGEAAAABAAAAAExhdmY1Mi42NC4y";else if(Modernizr.audio.ogg)t.src="data:audio/ogg;base64,T2dnUwACAAAAAAAAAAD/QwAAAAAAAM2LVKsBHgF2b3JiaXMAAAAAAUSsAAAAAAAAgLsAAAAAAAC4AU9nZ1MAAAAAAAAAAAAA/0MAAAEAAADmvOe6Dy3/////////////////MgN2b3JiaXMdAAAAWGlwaC5PcmcgbGliVm9yYmlzIEkgMjAwNzA2MjIAAAAAAQV2b3JiaXMfQkNWAQAAAQAYY1QpRplS0kqJGXOUMUaZYpJKiaWEFkJInXMUU6k515xrrLm1IIQQGlNQKQWZUo5SaRljkCkFmVIQS0kldBI6J51jEFtJwdaYa4tBthyEDZpSTCnElFKKQggZU4wpxZRSSkIHJXQOOuYcU45KKEG4nHOrtZaWY4updJJK5yRkTEJIKYWSSgelU05CSDWW1lIpHXNSUmpB6CCEEEK2IIQNgtCQVQAAAQDAQBAasgoAUAAAEIqhGIoChIasAgAyAAAEoCiO4iiOIzmSY0kWEBqyCgAAAgAQAADAcBRJkRTJsSRL0ixL00RRVX3VNlVV9nVd13Vd13UgNGQVAAABAEBIp5mlGiDCDGQYCA1ZBQAgAAAARijCEANCQ1YBAAABAABiKDmIJrTmfHOOg2Y5aCrF5nRwItXmSW4q5uacc845J5tzxjjnnHOKcmYxaCa05pxzEoNmKWgmtOacc57E5kFrqrTmnHPGOaeDcUYY55xzmrTmQWo21uaccxa0pjlqLsXmnHMi5eZJbS7V5pxzzjnnnHPOOeecc6oXp3NwTjjnnHOi9uZabkIX55xzPhmne3NCOOecc84555xzzjnnnHOC0JBVAAAQAABBGDaGcacgSJ+jgRhFiGnIpAfdo8MkaAxyCqlHo6ORUuoglFTGSSmdIDRkFQAACAAAIYQUUkghhRRSSCGFFFKIIYYYYsgpp5yCCiqppKKKMsoss8wyyyyzzDLrsLPOOuwwxBBDDK20EktNtdVYY62555xrDtJaaa211koppZRSSikIDVkFAIAAABAIGWSQQUYhhRRSiCGmnHLKKaigAkJDVgEAgAAAAgAAADzJc0RHdERHdERHdERHdETHczxHlERJlERJtEzL1ExPFVXVlV1b1mXd9m1hF3bd93Xf93Xj14VhWZZlWZZlWZZlWZZlWZZlWYLQkFUAAAgAAIAQQgghhRRSSCGlGGPMMeegk1BCIDRkFQAACAAgAAAAwFEcxXEkR3IkyZIsSZM0S7M8zdM8TfREURRN01RFV3RF3bRF2ZRN13RN2XRVWbVdWbZt2dZtX5Zt3/d93/d93/d93/d93/d1HQgNWQUASAAA6EiOpEiKpEiO4ziSJAGhIasAABkAAAEAKIqjOI7jSJIkSZakSZ7lWaJmaqZneqqoAqEhqwAAQAAAAQAAAAAAKJriKabiKaLiOaIjSqJlWqKmaq4om7Lruq7ruq7ruq7ruq7ruq7ruq7ruq7ruq7ruq7ruq7ruq7rui4QGrIKAJAAANCRHMmRHEmRFEmRHMkBQkNWAQAyAAACAHAMx5AUybEsS9M8zdM8TfRET/RMTxVd0QVCQ1YBAIAAAAIAAAAAADAkw1IsR3M0SZRUS7VUTbVUSxVVT1VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTVN0zRNIDRkJQAABADAYo3B5SAhJSXl3hDCEJOeMSYhtV4hBJGS3jEGFYOeMqIMct5C4xCDHggNWREARAEAAMYgxxBzyDlHqZMSOeeodJQa5xyljlJnKcWYYs0oldhSrI1zjlJHraOUYiwtdpRSjanGAgAAAhwAAAIshEJDVgQAUQAAhDFIKaQUYow5p5xDjCnnmHOGMeYcc44556B0UirnnHROSsQYc445p5xzUjonlXNOSiehAACAAAcAgAALodCQFQFAnACAQZI8T/I0UZQ0TxRFU3RdUTRd1/I81fRMU1U90VRVU1Vt2VRVWZY8zzQ901RVzzRV1VRVWTZVVZZFVdVt03V123RV3ZZt2/ddWxZ2UVVt3VRd2zdV1/Zd2fZ9WdZ1Y/I8VfVM03U903Rl1XVtW3VdXfdMU5ZN15Vl03Vt25VlXXdl2fc103Rd01Vl2XRd2XZlV7ddWfZ903WF35VlX1dlWRh2XfeFW9eV5XRd3VdlVzdWWfZ9W9eF4dZ1YZk8T1U903RdzzRdV3VdX1dd19Y105Rl03Vt2VRdWXZl2fddV9Z1zzRl2XRd2zZdV5ZdWfZ9V5Z13XRdX1dlWfhVV/Z1WdeV4dZt4Tdd1/dVWfaFV5Z14dZ1Ybl1XRg+VfV9U3aF4XRl39eF31luXTiW0XV9YZVt4VhlWTl+4ViW3feVZXRdX1ht2RhWWRaGX/id5fZ943h1XRlu3efMuu8Mx++k+8rT1W1jmX3dWWZfd47hGDq/8OOpqq+brisMpywLv+3rxrP7vrKMruv7qiwLvyrbwrHrvvP8vrAso+z6wmrLwrDatjHcvm4sv3Acy2vryjHrvlG2dXxfeArD83R1XXlmXcf2dXTjRzh+ygAAgAEHAIAAE8pAoSErAoA4AQCPJImiZFmiKFmWKIqm6LqiaLqupGmmqWmeaVqaZ5qmaaqyKZquLGmaaVqeZpqap5mmaJqua5qmrIqmKcumasqyaZqy7LqybbuubNuiacqyaZqybJqmLLuyq9uu7Oq6pFmmqXmeaWqeZ5qmasqyaZquq3meanqeaKqeKKqqaqqqraqqLFueZ5qa6KmmJ4qqaqqmrZqqKsumqtqyaaq2bKqqbbuq7Pqybeu6aaqybaqmLZuqatuu7OqyLNu6L2maaWqeZ5qa55mmaZqybJqqK1uep5qeKKqq5ommaqqqLJumqsqW55mqJ4qq6omea5qqKsumatqqaZq2bKqqLZumKsuubfu+68qybqqqbJuqauumasqybMu+78qq7oqmKcumqtqyaaqyLduy78uyrPuiacqyaaqybaqqLsuybRuzbPu6aJqybaqmLZuqKtuyLfu6LNu678qub6uqrOuyLfu67vqucOu6MLyybPuqrPq6K9u6b+sy2/Z9RNOUZVM1bdtUVVl2Zdn2Zdv2fdE0bVtVVVs2TdW2ZVn2fVm2bWE0Tdk2VVXWTdW0bVmWbWG2ZeF2Zdm3ZVv2ddeVdV/XfePXZd3murLty7Kt+6qr+rbu+8Jw667wCgAAGHAAAAgwoQwUGrISAIgCAACMYYwxCI1SzjkHoVHKOecgZM5BCCGVzDkIIZSSOQehlJQy5yCUklIIoZSUWgshlJRSawUAABQ4AAAE2KApsThAoSErAYBUAACD41iW55miatqyY0meJ4qqqaq27UiW54miaaqqbVueJ4qmqaqu6+ua54miaaqq6+q6aJqmqaqu67q6Lpqiqaqq67qyrpumqqquK7uy7Oumqqqq68quLPvCqrquK8uybevCsKqu68qybNu2b9y6ruu+7/vCka3rui78wjEMRwEA4AkOAEAFNqyOcFI0FlhoyEoAIAMAgDAGIYMQQgYhhJBSSiGllBIAADDgAAAQYEIZKDRkRQAQJwAAGEMppJRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkgppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkqppJRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoplVJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSSimllFJKKaWUUkoppZRSCgCQinAAkHowoQwUGrISAEgFAACMUUopxpyDEDHmGGPQSSgpYsw5xhyUklLlHIQQUmktt8o5CCGk1FJtmXNSWosx5hgz56SkFFvNOYdSUoux5ppr7qS0VmuuNedaWqs115xzzbm0FmuuOdecc8sx15xzzjnnGHPOOeecc84FAOA0OACAHtiwOsJJ0VhgoSErAYBUAAACGaUYc8456BBSjDnnHIQQIoUYc845CCFUjDnnHHQQQqgYc8w5CCGEkDnnHIQQQgghcw466CCEEEIHHYQQQgihlM5BCCGEEEooIYQQQgghhBA6CCGEEEIIIYQQQgghhFJKCCGEEEIJoZRQAABggQMAQIANqyOcFI0FFhqyEgAAAgCAHJagUs6EQY5Bjw1BylEzDUJMOdGZYk5qMxVTkDkQnXQSGWpB2V4yCwAAgCAAIMAEEBggKPhCCIgxAABBiMwQCYVVsMCgDBoc5gHAA0SERACQmKBIu7iALgNc0MVdB0IIQhCCWBxAAQk4OOGGJ97whBucoFNU6iAAAAAAAAwA4AEA4KAAIiKaq7C4wMjQ2ODo8AgAAAAAABYA+AAAOD6AiIjmKiwuMDI0Njg6PAIAAAAAAAAAAICAgAAAAAAAQAAAAICAT2dnUwAE7AwAAAAAAAD/QwAAAgAAADuydfsFAQEBAQEACg4ODg==";else if(Modernizr.audio.wav)t.src="data:audio/wav;base64,UklGRvwZAABXQVZFZm10IBAAAAABAAEARKwAAIhYAQACABAAZGF0YdgZAAAAAAEA/v8CAP//AAABAP////8DAPz/BAD9/wEAAAAAAAAAAAABAP7/AgD//wAAAQD//wAAAQD//wAAAQD+/wIA//8AAAAAAAD//wIA/v8BAAAA//8BAAAA//8BAP//AQAAAP//AQD//wEAAAD//wEA//8BAP//AQD//wEA//8BAP//AQD+/wMA/f8DAP3/AgD+/wIA/////wMA/f8CAP7/AgD+/wMA/f8CAP7/AgD//wAAAAAAAAAAAQD+/wIA/v8CAP7/AwD9/wIA/v8BAAEA/v8CAP7/AQAAAAAAAAD//wEAAAD//wIA/f8DAP7/AQD//wEAAAD//wEA//8CAP7/AQD//wIA/v8CAP7/AQAAAAAAAAD//wEAAAAAAAAA//8BAP//AgD9/wQA+/8FAPz/AgAAAP//AgD+/wEAAAD//wIA/v8CAP3/BAD8/wQA/P8DAP7/AwD8/wQA/P8DAP7/AQAAAAAA//8BAP//AgD+/wEAAAD//wIA/v8BAP//AQD//wEAAAD//wEA//8BAAAAAAAAAP//AgD+/wEAAAAAAAAAAAD//wEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAP//AgD+/wIA/v8BAP//AQABAP7/AQD//wIA/v8CAP3/AwD/////AgD9/wMA/v8BAP//AQAAAP//AQD//wEA//8BAP//AAABAP//AAABAP//AQD//wAAAAACAP3/AwD9/wIA//8BAP//AQD//wEA//8BAP//AgD9/wMA/v8AAAIA/f8CAAAA/v8EAPv/BAD9/wIAAAD+/wQA+v8HAPr/BAD+/wEAAAD//wIA/f8EAPz/BAD7/wUA/P8EAPz/AwD+/wEAAAD//wEAAAAAAP//AgD8/wUA+/8FAPz/AwD9/wIA//8AAAEA/v8CAP//AQD//wAAAAABAP//AgD9/wMA/f8EAPz/AwD+/wAAAwD7/wUA/P8DAP7/AQAAAP//AgD+/wEAAQD+/wIA/v8BAAEA/v8CAP7/AQAAAP//AgD9/wMA/f8DAP7/AgD+/wEAAAAAAAEA//8AAAEA/v8DAP3/AgD//wEA//8BAP7/AwD9/wMA/v8BAP//AQAAAP//AgD9/wMA/v8BAP//AQAAAP//AgD+/wEAAQD+/wIA/////wIA//8AAAEA/f8DAP//AAABAP////8DAP3/AwD+/wEA//8BAP//AQAAAAAA//8BAP//AQD//wEA//8BAP//AAAAAAEA//8BAP7/AgD//wEA//8AAAAAAAAAAAAAAAD//wIA/v8BAAAA//8BAAEA/v8BAAAA//8DAPz/AwD+/wIA/v8CAP3/AwD+/wEAAAD//wEA//8BAAAA//8BAAAA/v8EAPv/BAD+/wAAAAABAP7/AgD//wAAAAABAP7/AgD//wAAAAAAAAAAAAABAP3/BAD8/wQA/f8BAAAAAAABAP7/AgD+/wIA/v8CAP7/AgD+/wIA/v8BAAAAAAD//wIA/f8DAP7/AAABAP//AAACAPz/BAD9/wIA//8AAP//AwD9/wMA/P8EAP3/AwD9/wIA//8BAP//AQD+/wMA/f8DAP7/AAABAP//AQAAAP//AQD//wIA/f8DAP7/AQAAAP//AQAAAAAA//8CAP7/AQABAP7/AgD+/wEAAQD+/wIA/v8CAP////8CAP7/AgD//wAAAAABAP7/AwD9/wIAAAD+/wMA/f8CAP//AQD+/wMA/f8CAP//AAACAPz/BQD6/wUA/v///wIA/v8CAP3/BAD7/wYA+v8FAPz/AwD/////AgD+/wEAAAD//wEAAAD//wIA/f8DAP7/AQAAAP//AgD//wAA//8BAAAAAAAAAP//AQD//wEA//8AAAIA/f8DAP3/AgAAAP//AQD//wEA//8AAAEA//8BAP////8CAP//AAABAP3/BAD9/wIA/v8BAAEA//8BAP7/AgD//wEA//8AAAEA//8BAP//AAAAAAEA//8BAP7/AgD//wEA//8AAAAAAQD+/wIA/v8BAAAAAAD//wIA/v8BAAAAAAAAAAAAAQD+/wMA/f8CAP//AQD//wIA/f8DAP7/AQD//wEA//8CAP7/AAABAP7/AwD9/wMA/v8AAAEA//8BAAAAAAD//wIA/v8BAAAA//8CAP7/AgD+/wEA//8CAP7/AgD//wAAAAAAAAAAAQD//wEA/v8DAPz/BQD8/wIA//8AAAEAAAD//wEA//8BAP//AQAAAAAA//8BAP//AgD+/wEAAAAAAP//AQD+/wMA/////wEA/v8CAP//AQD//wEA//8AAAEA//8BAAAA/v8EAPz/AwD+/wEAAAAAAAAA//8CAP7/AQD//wEA//8BAP//AAABAP7/AwD9/wIA//8BAP//AQD//wEA//8AAAEA/v8EAPv/BAD9/wIA//8BAP7/AwD9/wIA//8AAAEA//8BAP//AQD//wAAAQD//wEAAAD+/wMA/v8AAAIA/f8DAP7/AQD//wAAAQD+/wMA/f8CAP//AAABAP7/AgD+/wMA/f8CAP7/AQABAP7/AgD+/wIA/v8CAP7/AwD8/wMA//8AAAEA//8AAAAAAAABAP//AQD//wAAAQD//wIA/f8DAP3/AwD+/wAAAgD9/wIA//8AAAEAAAD+/wMA/P8FAPv/BAD9/wIA//8AAP//AgD+/wIA/v8BAAAAAAD//wEAAAAAAP//AQD//wEA//8BAP//AAABAP7/AwD9/wIA//8BAP//AAABAP//AQD//wAAAQD//wEA//8BAP//AAABAAAA//8BAP7/AwD9/wMA/f8DAP3/AgD//wEA//8BAP7/AgD//wAAAgD8/wQA/f8CAP//AQD+/wMA/f8CAP7/AgD//wAAAAAAAAAAAAABAP7/AwD9/wIA/v8DAP3/AwD9/wIA/v8DAPz/BQD7/wQA/f8CAP7/AwD9/wMA/f8CAP//AQAAAP7/AwD+/wEA//8AAAEAAAAAAP//AAABAP//AQAAAP7/AwD9/wMA/f8CAP//AQD//wEA//8AAAIA/f8CAAAA//8BAAAA//8BAAAA/v8EAPv/BAD9/wIA//8AAAEA/v8CAP//AAABAP//AAABAP//AAABAP7/AwD8/wQA/f8CAAAA/v8DAP3/AwD9/wMA/v8BAAAA//8BAAAA//8CAP7/AQAAAAAAAAAAAAAA//8CAP7/AgD+/wIA/v8CAP7/AgD//wAAAQD//wAAAQD//wAAAQD//wAAAQD+/wIA//8AAAAAAQD+/wMA/f8CAP//AQD//wEA//8AAAEA/v8DAP3/AgD//wAAAAABAP7/AwD9/wIA//8AAAEA/v8DAP3/AgD//wAAAAABAP7/AwD8/wMA/v8CAP//AAD//wIA/v8CAP7/AQABAP7/AQAAAP//AgD/////AQD//wEAAAD//wEA/v8EAPv/BAD9/wMA/v8BAAAA//8BAAEA/P8GAPr/BQD8/wMA/v8BAAAA//8CAP7/AQABAP3/BAD7/wYA+/8EAPz/AwD//wEA//8BAP7/BAD8/wMA/v8AAAIA/v8BAAAA//8BAAAA//8BAAAA//8CAP3/AwD+/wAAAgD8/wUA/P8DAP7/AAABAAAAAAD//wEAAAD//wIA/f8DAP7/AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAEA/f8EAPz/AwD/////AgD+/wIA/f8DAP7/AgD+/wEA//8CAP7/AQD//wEAAAAAAP//AQAAAP//AgD9/wMA/v8BAAAA//8BAP//AQAAAP//AAACAP3/BAD7/wQA/v8BAAAA//8BAP//AQAAAP//AQAAAP7/BAD7/wUA+/8EAP3/AgD//wAAAQD+/wIA//8AAAEA/v8CAP//AQD+/wEAAAAAAAAAAAD//wEA//8CAP3/AwD9/wIA//8AAAAAAAAAAAAA//8BAP//AgD+/wEA//8CAP7/AQAAAP//AgD/////AgD/////AgD+/wIA//8AAP//AQABAP7/AgD9/wMA/v8CAP////8BAAAAAAAAAAAA//8CAP////8DAPz/AwD+/wEAAAAAAP//AQD//wEAAAD//wEAAAD+/wQA+/8FAPz/AgAAAP//AgD9/wMA/v8BAAAAAAD//wEAAAD//wIA/v8BAAAAAAD//wIA/v8BAAAA//8BAAAA//8CAP7/AQD//wEA//8BAAAA//8BAP//AAABAP//AQAAAP7/AgD//wEA//8AAAAAAQD+/wMA/P8EAP7///8DAPz/BQD8/wEAAQD+/wMA/v8AAAEA//8BAP//AQD//wEA/v8CAP//AQD//wAAAAABAAAA//8BAP//AQAAAAAA//8BAP//AgD+/wAAAQD//wIA/f8CAP//AQAAAP7/AwD9/wMA/v8BAP//AAABAP//AgD9/wIA//8BAAAA//8BAAAA//8CAP3/AwD+/wEAAAD+/wQA/P8DAP7/AAACAP7/AQAAAP//AQAAAP//AQAAAP//AgD9/wIAAAD//wIA/f8DAP7/AQD//wEA//8CAP7/AQD//wAAAQD//wEA//8AAAAAAQD//wEAAAD9/wUA+/8FAPz/AgD//wAAAQD//wAAAQD+/wMA/f8BAAEA/v8CAP7/AgD+/wIA/v8BAAAAAAAAAAAAAAD//wIA/v8CAP////8CAP7/AgD+/wIA/v8CAP7/AQAAAP//AQAAAP//AQD//wAAAQD//wAAAQD+/wMA/f8CAAAA/v8DAP3/AgAAAP//AQAAAP7/AwD9/wMA/v8BAP//AQD//wEAAAD+/wMA/f8CAAAA/v8CAP//AAAAAAEA//8AAAEA/v8DAP3/AwD9/wIA//8BAP//AgD8/wQA/v8BAAAA/v8CAP//AQD//wAAAAAAAAEA/f8EAPz/BAD9/wIA//8AAAAAAAABAP//AAAAAAAAAAABAP3/BAD9/wIA/v8BAAEA//8AAAAA//8CAP7/AgD9/wQA+/8FAPv/BQD8/wMA/f8DAP3/AwD+/wAAAgD9/wMA/f8CAAAA/v8EAPv/BQD7/wUA/P8DAP///v8DAP3/BAD8/wMA/f8DAP7/AQD//wEAAAD//wEA/v8CAAAA/v8CAP7/AgD//wAAAAAAAAAAAQD+/wIA//8AAAEA/v8DAPz/BAD9/wIA//8AAP//AgD//wEA/v8BAAAAAQD//wAAAAAAAAEA//8AAAEA//8BAP//AAABAP//AQD+/wIA/v8DAPz/BAD8/wQA/f8BAAAAAQD+/wMA/P8DAP//AAAAAAAAAAD//wMA+/8FAP3/AQABAP3/BAD8/wMA/v8BAAAA//8CAP3/AwD+/wEAAQD9/wMA/f8EAPz/BAD7/wQA/v8BAAEA/f8DAP7/AQAAAP//AgD+/wEAAAD//wIA/v8CAP7/AgD+/wEAAQD//wEA/v8CAP7/BAD7/wQA/f8CAAAA//8AAAAAAAABAP//AQD+/wEAAQD+/wMA/f8BAAEA/v8DAPz/AwD/////AwD8/wQA/P8DAP7/AgD//wAA//8BAAAAAAAAAP//AgD+/wEAAAD//wIA/v8BAAAA//8CAP3/AgD//wAAAQD+/wIA/v8BAAAA//8CAP7/AgD+/wEA//8CAP3/BAD7/wQA/v8BAAAA//8AAAEAAAD//wIA/f8DAP7/AgD+/wIA/v8CAP7/AgD+/wEAAAAAAP//AgD9/wMA/v8BAP//AgD9/wMA/v8AAAEA//8BAP//AQD//wEA//8AAAEA/v8EAPz/AgD//wAAAQAAAP//AAABAP//AQD//wEAAAD//wEA//8BAAEA/f8DAP7/AQABAP3/AwD+/wIA/////wEAAAAAAAAAAAD//wIA/v8CAP////8CAP7/AgD//wAA//8CAP3/BAD9/wAAAgD9/wMA/v8BAP//AQAAAP//AQAAAP//AgD9/wMA/f8EAPz/AwD+/wEAAAAAAAAAAAD//wIA/f8EAP3/AAABAAAA//8CAP7/AQAAAP//AQAAAAAA//8BAP//AQAAAP//AQAAAP//AQAAAP//AgD9/wMA/v8BAP//AQAAAP//AQD//wIA/v8CAP3/BAD9/wEAAAD//wEAAQD9/wMA/f8CAAAA/v8DAP3/AgD//wAAAQD+/wIA/v8CAP7/AQAAAP//AgD+/wEAAAAAAP//AwD7/wUA/f8BAAEA/v8BAAEA/v8DAP3/AgD//wEA//8BAP//AQD//wEA//8CAP3/BAD7/wQA/////wIA/v8AAAIA/v8CAP3/BAD7/wUA/P8DAP3/AwD9/wMA/v8AAAIA/v8CAP7/AgD+/wIA//8AAAEA/v8CAP7/AgD//wAAAAD//wEAAAAAAAAA//8BAP7/BAD7/wUA/P8CAAAA//8BAP//AQAAAP//AgD9/wMA/v8BAAAA//8BAAAA//8CAP3/AwD+/wEA//8CAP3/AwD+/wAAAwD8/wIAAAD//wIA/////wIA/v8CAP7/AgD+/wEAAAAAAAAAAAAAAP//AgD+/wIA//8AAAAA//8CAP7/AgD+/wEA//8CAP3/AwD9/wMA/v8BAP7/AwD9/wMA/f8CAP//AQD+/wIA//8BAP//AQD+/wMA/v8BAAAA//8BAAAA//8CAP7/AQAAAP//AgD+/wIA/v8CAP//AAAAAAEA//8BAP//AAABAAAA//8BAP//AQD//wEA//8BAP//AQAAAP//AQD//wEAAAD//wIA/f8CAAAA//8BAAAA//8BAP//AAABAP//AQD//wAAAAAAAAEA/v8CAP//AQD//wAAAAABAP7/AwD9/wIAAAD+/wIA//8BAP//AgD9/wMA/f8DAP7/AgD+/wEAAAAAAAEA/v8CAP7/AgD//wAAAAAAAAAAAAAAAP//AgD/////AgD9/wQA/f8BAAAAAAAAAAEA/f8DAP////8DAP3/AQABAP7/AgD//wAAAQD+/wMA/f8CAP7/AQABAP7/AwD7/wYA+v8FAP3/AQABAP7/AgD+/wMA/f8CAP7/AwD+/wEA//8BAP//AQAAAP7/BQD5/wcA+v8FAPz/AwD+/wIA/v8BAAAA//8DAPv/BQD8/wMA/////wEAAAAAAAAAAAD//wIA/f8DAP7/AQAAAP//AQAAAP//AgD+/wIA/v8BAAEA/f8EAPz/AwD+/wEA//8CAP7/AQD//wEA//8CAP7/AQAAAP//AgD+/wEAAAAAAAAAAAAAAAAAAAD//wIA/f8EAPz/AwD+/wEA//8CAP7/AgD+/wEAAQD+/wEAAQD+/wIA/////wIA//8AAAAAAAAAAAAAAAD//wEAAAAAAP//AgD9/wMA/v8BAP//AQAAAP//AQD//wEA//8BAP//AQD//wEA//8BAP//AQAAAP7/AwD9/wMA/v8BAP7/AwD9/wMA/v8BAP//AAABAP//AQD//wAAAAABAP//AAAAAAAAAQD//wEA/v8CAAAA/v8EAPv/BAD9/wIAAAD+/wMA/P8DAP//AAAAAP//AQD//wIA/f8DAP3/AwD9/wMA/v8BAAAA//8BAAAA//8CAP3/AwD9/wQA+/8FAPv/BQD8/wMA/v8BAAAA//8BAP//AgD+/wEAAAD//wIA/v8BAAEA/f8DAP3/AgAAAP//AQD//wAAAQD//wEA//8BAP//AQD//wEA/v8DAP3/AgAAAP7/AwD9/wIAAAD//wEAAAD//wIA/f8DAP7/AgD9/wQA+/8FAPz/AgAAAP//AgD9/wIA//8BAP//AQD//wEA//8BAP//AQD//wIA/f8DAP3/AgD//wAAAQD+/wIA/v8BAAEA/v8CAP7/AgD+/wMA/P8DAP//AAABAP7/AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEA/v8CAP3/BAD8/wMA/v8BAAAAAAD//wEAAAAAAAAAAAD//wEAAAAAAAAA//8BAP//AgD+/wEA//8CAP3/AwD9/wMA/f8EAPv/BAD+/wAAAQD//wEA//8BAP//AAABAP//AQD//wEAAAD//wEA//8BAP//AgD9/wMA/v8AAAIA/f8DAP7/AAACAP3/AwD+/wEA//8BAP//AQAAAP//AQAAAP7/AwD9/wMA/v8AAAEA//8BAP//AAAAAAEA//8AAAEA/v8CAP//AAAAAAEA/v8DAPz/BAD9/wEAAQD+/wEAAQD9/wQA/P8DAP7/AQAAAAAAAAAAAAAAAAAAAAAAAQD+/wIA/////wIA/v8BAAAA//8BAP//AQD//wEA//8BAAAA/v8EAPz/AwD///7/BAD8/wMA/////wIA/v8CAP////8CAP7/AgD+/wIA/v8CAP////8CAP7/AwD9/wIA/v8CAP//AAABAP7/AwD9/wEAAQD+/wMA/f8CAP//AAAAAAEA/v8DAPz/BAD9/wIA/v8CAP7/AgD//wAAAAD//wIA/v8CAP7/AQAAAAAA//8CAP7/AgD+/wIA/v8CAP7/AwD8/wUA+v8GAPv/AwD//wAAAAAAAAAA//8DAPv/BQD9/wAAAgD9/wMA/v8BAP//AQAAAP//AgD9/wMA/v8BAAAA//8BAAAAAAAAAP//AQAAAAAAAAD//wEA//8CAP3/AwD+/wAAAgD+/wEAAAD//wIA/v8CAP7/AgD/////AwD8/wUA/P8CAP//AQD//wIA/f8DAP3/AwD+/wAAAQD+/wMA/f8DAP3/AgD//wAAAQD//wEA//8BAP7/AwD+/wEA//8AAAEA//8CAPz/BAD9/wIA//8AAAEA/v8DAPz/BAD9/wIA//8AAAEA/v8CAP7/AgD//wEA/f8EAPz/BAD+////AgD//wAAAQD//wAAAQD//wEA//8BAP7/AwD+/wEA";else{addTest("audiopreload",!1);return}}catch{addTest("audiopreload",!1);return}t.setAttribute("preload","auto"),t.style.cssText="display:none",docElement.appendChild(t),setTimeout(function(){t.addEventListener("loadeddata",n,!1),e=setTimeout(n,A)},0)});Modernizr.addTest("webaudio",function(){var e="webkitAudioContext"in window,A="AudioContext"in window;return Modernizr._config.usePrefixes&&e||A});Modernizr.addTest("batteryapi",!!prefixed("battery",navigator)||!!prefixed("getBattery",navigator),{aliases:["battery-api"]});Modernizr.addTest("lowbattery",function(){var e=.2,A=prefixed("battery",navigator);return!!(A&&!A.charging&&A.level<=e)});Modernizr.addTest("blobconstructor",function(){try{return!!new Blob}catch{return!1}},{aliases:["blob-constructor"]});Modernizr.addTest("broadcastchannel","BroadcastChannel"in window);Modernizr.addTest("canvas",function(){var e=createElement("canvas");return!!(e.getContext&&e.getContext("2d"))});Modernizr.addTest("canvasblending",function(){if(Modernizr.canvas===!1)return!1;var e=createElement("canvas").getContext("2d");try{e.globalCompositeOperation="screen"}catch{}return e.globalCompositeOperation==="screen"});var canvas=createElement("canvas");Modernizr.addTest("todataurljpeg",function(){var e=!1;try{e=!!Modernizr.canvas&&canvas.toDataURL("image/jpeg").indexOf("data:image/jpeg")===0}catch{}return e}),Modernizr.addTest("todataurlpng",function(){var e=!1;try{e=!!Modernizr.canvas&&canvas.toDataURL("image/png").indexOf("data:image/png")===0}catch{}return e}),Modernizr.addTest("todataurlwebp",function(){var e=!1;try{e=!!Modernizr.canvas&&canvas.toDataURL("image/webp").indexOf("data:image/webp")===0}catch{}return e});Modernizr.addTest("canvaswinding",function(){if(Modernizr.canvas===!1)return!1;var e=createElement("canvas").getContext("2d");return e.rect(0,0,10,10),e.rect(2,2,6,6),e.isPointInPath(5,5,"evenodd")===!1});Modernizr.addTest("canvastext",function(){return Modernizr.canvas===!1?!1:typeof createElement("canvas").getContext("2d").fillText=="function"});Modernizr.addAsyncTest(function(){var e,A=["read","readText","write","writeText"];if(navigator.clipboard){addTest("clipboard",!0);for(var t=0;t<A.length;t++)navigator.clipboard[A[t]]?e=!0:e=!1,addTest("clipboard."+A[t].toLowerCase(),e)}else addTest("clipboard",!1)});Modernizr.addTest("contenteditable",function(){if("contentEditable"in docElement){var e=createElement("div");return e.contentEditable=!0,e.contentEditable==="true"}});Modernizr.addTest("contextmenu","contextMenu"in docElement&&"HTMLMenuItemElement"in window);Modernizr.addTest("cors","XMLHttpRequest"in window&&"withCredentials"in new XMLHttpRequest);var crypto=prefixed("crypto",window);Modernizr.addTest("crypto",!!prefixed("subtle",crypto));var crypto=prefixed("crypto",window),supportsGetRandomValues;if(crypto&&"getRandomValues"in crypto&&"Uint32Array"in window){var array=new Uint32Array(10),values=crypto.getRandomValues(array);supportsGetRandomValues=values&&is(values[0],"number")}Modernizr.addTest("getrandomvalues",!!supportsGetRandomValues);Modernizr.addTest("cssall","all"in docElement.style);Modernizr.addTest("cssanimations",testAllProps("animationName","a",!0));Modernizr.addTest("appearance",testAllProps("appearance"));Modernizr.addTest("aspectratio",function(){if(typeof CSS!="object"&&typeof CSS.supports=="function")return CSS.supports("aspect-ratio","1 / 1");var e=createElement("p"),A=e.style;return"aspectRatio"in A?(A.cssText="aspect-ratio:1 / 1",e.remove(),A.aspectRatio==="1 / 1"):(e.remove(),!1)});Modernizr.addTest("backdropfilter",testAllProps("backdropFilter"));Modernizr.addTest("backgroundblendmode",prefixed("backgroundBlendMode","text"));Modernizr.addTest("backgroundcliptext",function(){return testAllProps("backgroundClip","text")});Modernizr.addTest("bgpositionshorthand",function(){var e=createElement("a"),A=e.style,t="right 10px bottom 10px";return A.cssText="background-position: "+t+";",A.backgroundPosition===t});Modernizr.addTest("bgpositionxy",function(){return testAllProps("backgroundPositionX","3px",!0)&&testAllProps("backgroundPositionY","5px",!0)});Modernizr.addTest("bgrepeatround",testAllProps("backgroundRepeat","round")),Modernizr.addTest("bgrepeatspace",testAllProps("backgroundRepeat","space"));Modernizr.addTest("backgroundsize",testAllProps("backgroundSize","100%",!0));Modernizr.addTest("bgsizecover",testAllProps("backgroundSize","cover"));Modernizr.addTest("borderimage",testAllProps("borderImage","url() 1",!0));Modernizr.addTest("borderradius",testAllProps("borderRadius","0px",!0));Modernizr.addTest("boxdecorationbreak",testAllProps("boxDecorationBreak","slice"));Modernizr.addTest("boxshadow",testAllProps("boxShadow","1px 1px",!0));Modernizr.addTest("boxsizing",testAllProps("boxSizing","border-box",!0)&&(document.documentMode===undefined||document.documentMode>7));Modernizr.addTest("csscalc",function(){var e="width:",A="calc(10px);",t=createElement("a");return t.style.cssText=e+prefixes.join(A+e),!!t.style.length});Modernizr.addTest("checked",function(){return testStyles("#modernizr {position:absolute} #modernizr input {margin-left:10px} #modernizr :checked {margin-left:20px;display:block}",function(e){var A=createElement("input");return A.setAttribute("type","checkbox"),A.setAttribute("checked","checked"),e.appendChild(A),A.offsetLeft===20})});Modernizr.addTest("csschunit",function(){var e=modElem.elem.style,A;try{e.fontSize="3ch",A=e.fontSize.indexOf("ch")!==-1}catch{A=!1}return A});(function(){Modernizr.addTest("csscolumns",function(){var n=!1,o=testAllProps("columnCount");try{n=!!o,n&&(n=new Boolean(n))}catch{}return n});for(var e=["Width","Span","Fill","Gap","Rule","RuleColor","RuleStyle","RuleWidth","BreakBefore","BreakAfter","BreakInside"],A,t,r=0;r<e.length;r++)A=e[r].toLowerCase(),t=testAllProps("column"+e[r]),(A==="breakbefore"||A==="breakafter"||A==="breakinside")&&(t=t||testAllProps(e[r])),Modernizr.addTest("csscolumns."+A,t)})();Modernizr.addTest("cssgridlegacy",testAllProps("grid-columns","10px",!0)),Modernizr.addTest("cssgrid",testAllProps("grid-template-rows","none",!0));Modernizr.addTest("cubicbezierrange",function(){var e=createElement("a");return e.style.cssText=prefixes.join("transition-timing-function:cubic-bezier(1,0,0,1.1); "),!!e.style.length});var supportsFn=window.CSS&&window.CSS.supports.bind(window.CSS)||window.supportsCSS;Modernizr.addTest("customproperties",!!supportsFn&&(supportsFn("--f:0")||supportsFn("--f",0)));Modernizr.addTest("displayrunin",testAllProps("display","run-in"),{aliases:["display-runin"]});testStyles("#modernizr{display: table; direction: ltr}#modernizr div{display: table-cell; padding: 10px}",function(e){var A,t=e.childNodes;A=t[0].offsetLeft<t[1].offsetLeft,Modernizr.addTest("displaytable",A,{aliases:["display-table"]})},2);Modernizr.addTest("ellipsis",testAllProps("textOverflow","ellipsis"));var CSS=window.CSS;Modernizr.addTest("cssescape",CSS?typeof CSS.escape=="function":!1);Modernizr.addTest("cssexunit",function(){var e=modElem.elem.style,A;try{e.fontSize="3ex",A=e.fontSize.indexOf("ex")!==-1}catch{A=!1}return A});var newSyntax="CSS"in window&&"supports"in window.CSS,oldSyntax="supportsCSS"in window;Modernizr.addTest("supports",newSyntax||oldSyntax);Modernizr.addTest("cssfilters",function(){if(Modernizr.supports)return testAllProps("filter","blur(2px)");var e=createElement("a");return e.style.cssText=prefixes.join("filter:blur(2px); "),!!e.style.length&&(document.documentMode===undefined||document.documentMode>9)});Modernizr.addTest("flexbox",testAllProps("flexBasis","1px",!0));Modernizr.addTest("flexboxlegacy",testAllProps("boxDirection","reverse",!0));Modernizr.addTest("flexboxtweener",testAllProps("flexAlign","end",!0));Modernizr.addTest("flexgap",function(){var e=createElement("div");e.style.display="flex",e.style.flexDirection="column",e.style.rowGap="1px",e.appendChild(createElement("div")),e.appendChild(createElement("div")),docElement.appendChild(e);var A=e.scrollHeight===1;return e.parentNode.removeChild(e),A});Modernizr.addTest("flexwrap",testAllProps("flexWrap","wrap",!0));Modernizr.addTest("focusvisible",function(){try{document.querySelector(":focus-visible")}catch{return!1}return!0});Modernizr.addTest("focuswithin",function(){try{document.querySelector(":focus-within")}catch{return!1}return!0});Modernizr.addTest("fontDisplay",testProp("font-display"));var unsupportedUserAgent=function(){var e=navigator.userAgent,A=e.match(/w(eb)?osbrowser/gi),t=e.match(/windows phone/gi)&&e.match(/iemobile\/([0-9])+/gi)&&parseFloat(RegExp.$1)>=9;return A||t}();unsupportedUserAgent?Modernizr.addTest("fontface",!1):testStyles('@font-face {font-family:"font";src:url("https://")}',function(e,A){var t=document.getElementById("smodernizr"),r=t.sheet||t.styleSheet,n=r?r.cssRules&&r.cssRules[0]?r.cssRules[0].cssText:r.cssText||"":"",o=/src/i.test(n)&&n.indexOf(A.split(" ")[0])===0;Modernizr.addTest("fontface",o)});testStyles('#modernizr{font:0/0 a}#modernizr:after{content:":)";visibility:hidden;font:7px/1 a}',function(e){Modernizr.addTest("generatedcontent",e.offsetHeight>=6)});Modernizr.addTest("cssgradients",function(){for(var e="background-image:",A="gradient(linear,left top,right bottom,from(#9f9),to(white));",t="",r,n=0,o=prefixes.length-1;n<o;n++)r=n===0?"to ":"",t+=e+prefixes[n]+"linear-gradient("+r+"left top, #9f9, white);";Modernizr._config.usePrefixes&&(t+=e+"-webkit-"+A);var a=createElement("a"),l=a.style;return l.cssText=t,(""+l.backgroundImage).indexOf("gradient")>-1});Modernizr.addTest("hairline",function(){return testStyles("#modernizr {border:.5px solid transparent}",function(e){return e.offsetHeight===1})});Modernizr.addTest("hsla",function(){var e=createElement("a").style;return e.cssText="background-color:hsla(120,40%,100%,.5)",contains(e.backgroundColor,"rgba")||contains(e.backgroundColor,"hsla")});Modernizr.addAsyncTest(function(){var e=300;setTimeout(A,e);function A(){if(!document.body&&!document.getElementsByTagName("body")[0]){setTimeout(A,e);return}function t(){try{var o=createElement("div"),a=createElement("span"),l=o.style,s=0,f=0,u=!1,m=document.body.firstElementChild||document.body.firstChild;return o.lang="en",o.appendChild(a),a.innerHTML="Bacon ipsum dolor sit amet jerky velit in culpa hamburger et. Laborum dolor proident, enim dolore duis commodo et strip steak. Salami anim et, veniam consectetur dolore qui tenderloin jowl velit sirloin. Et ad culpa, fatback cillum jowl ball tip ham hock nulla short ribs pariatur aute. Pig pancetta ham bresaola, ut boudin nostrud commodo flank esse cow tongue culpa. Pork belly bresaola enim pig, ea consectetur nisi. Fugiat officia turkey, ea cow jowl pariatur ullamco proident do laborum velit sausage. Magna biltong sint tri-tip commodo sed bacon, esse proident aliquip. Ullamco ham sint fugiat, velit in enim sed mollit nulla cow ut adipisicing nostrud consectetur. Proident dolore beef ribs, laborum nostrud meatball ea laboris rump cupidatat labore culpa. Shankle minim beef, velit sint cupidatat fugiat tenderloin pig et ball tip. Ut cow fatback salami, bacon ball tip et in shank strip steak bresaola. In ut pork belly sed mollit tri-tip magna culpa veniam, short ribs qui in andouille ham consequat. Dolore bacon t-bone, velit short ribs enim strip steak nulla. Voluptate labore ut, biltong swine irure jerky. Cupidatat excepteur aliquip salami dolore. Ball tip strip steak in pork dolor. Ad in esse biltong. Dolore tenderloin exercitation ad pork loin t-bone, dolore in chicken ball tip qui pig. Ut culpa tongue, sint ribeye dolore ex shank voluptate hamburger. Jowl et tempor, boudin pork chop labore ham hock drumstick consectetur tri-tip elit swine meatball chicken ground round. Proident shankle mollit dolore. Shoulder ut duis t-bone quis reprehenderit. Meatloaf dolore minim strip steak, laboris ea aute bacon beef ribs elit shank in veniam drumstick qui. Ex laboris meatball cow tongue pork belly. Ea ball tip reprehenderit pig, sed fatback boudin dolore flank aliquip laboris eu quis. Beef ribs duis beef, cow corned beef adipisicing commodo nisi deserunt exercitation. Cillum dolor t-bone spare ribs, ham hock est sirloin. Brisket irure meatloaf in, boudin pork belly sirloin ball tip. Sirloin sint irure nisi nostrud aliqua. Nostrud nulla aute, enim officia culpa ham hock. Aliqua reprehenderit dolore sunt nostrud sausage, ea boudin pork loin ut t-bone ham tempor. Tri-tip et pancetta drumstick laborum. Ham hock magna do nostrud in proident. Ex ground round fatback, venison non ribeye in.",document.body.insertBefore(o,m),l.cssText="position:absolute;top:0;left:0;width:5em;text-align:justify;text-justify:newspaper;",s=a.offsetHeight,f=a.offsetWidth,l.cssText="position:absolute;top:0;left:0;width:5em;text-align:justify;text-justify:newspaper;"+prefixes.join("hyphens:auto; "),u=a.offsetHeight!==s||a.offsetWidth!==f,document.body.removeChild(o),o.removeChild(a),u}catch{return!1}}function r(o,a){try{var l=createElement("div"),s=createElement("span"),f=l.style,u=0,m=!1,g=!1,P=!1,M=document.body.firstElementChild||document.body.firstChild;return f.cssText="position:absolute;top:0;left:0;overflow:visible;width:1.25em;",l.appendChild(s),document.body.insertBefore(l,M),s.innerHTML="mm",u=s.offsetHeight,s.innerHTML="m"+o+"m",g=s.offsetHeight>u,a?(s.innerHTML="m<br />m",u=s.offsetWidth,s.innerHTML="m"+o+"m",P=s.offsetWidth>u):P=!0,g===!0&&P===!0&&(m=!0),document.body.removeChild(l),l.removeChild(s),m}catch{return!1}}function n(o){try{var a=createElement("input"),l=createElement("div"),s="lebowski",f=!1,u,m=document.body.firstElementChild||document.body.firstChild;a.style.cssText="position:fixed;top:0;",l.style.cssText="position:fixed;top:0;",l.innerHTML=s+o+s,document.body.insertBefore(l,m),document.body.insertBefore(a,l),a.setSelectionRange?(a.focus(),a.setSelectionRange(0,0)):a.createTextRange&&(u=a.createTextRange(),u.collapse(!0),u.moveEnd("character",0),u.moveStart("character",0),u.select());try{window.find?f=window.find(s+s):(u=window.self.document.body.createTextRange(),f=u.findText(s+s))}catch{f=!1}return document.body.removeChild(l),document.body.removeChild(a),f}catch{return!1}}addTest("csshyphens",function(){if(!testAllProps("hyphens","auto",!0))return!1;try{return t()}catch{return!1}}),addTest("softhyphens",function(){try{return r("&#173;",!0)&&r("&#8203;",!1)}catch{return!1}}),addTest("softhyphensfind",function(){try{return n("&#173;")&&n("&#8203;")}catch{return!1}})}});Modernizr.addTest("cssinvalid",function(){return testStyles("#modernizr input{height:0;border:0;padding:0;margin:0;width:10px} #modernizr input:invalid{width:50px}",function(e){var A=createElement("input");return A.required=!0,e.appendChild(A),A.clientWidth>10})});testStyles("#modernizr div {width:100px} #modernizr :last-child{width:200px;display:block}",function(e){Modernizr.addTest("lastchild",e.lastChild.offsetWidth>e.firstChild.offsetWidth)},2);Modernizr.addTest("cssmask",testAllProps("maskRepeat","repeat-x",!0));Modernizr.addTest("mediaqueries",mq("only all"));Modernizr.addTest("multiplebgs",function(){var e=createElement("a").style;return e.cssText="background:url(https://),url(https://),red url(https://)",/(url\s*\(.*?){3}/.test(e.background)});testStyles("#modernizr div {width:1px} #modernizr div:nth-child(2n) {width:2px;}",function(e){var A=e.getElementsByTagName("div"),t=A[0].offsetWidth===A[2].offsetWidth&&A[1].offsetWidth===A[3].offsetWidth&&A[0].offsetWidth!==A[1].offsetWidth;Modernizr.addTest("nthchild",t)},4);Modernizr.addTest("objectfit",!!prefixed("objectFit"),{aliases:["object-fit"]});Modernizr.addTest("opacity",function(){var e=createElement("a").style;return e.cssText=prefixes.join("opacity:.55;"),/^0.55$/.test(e.opacity)});Modernizr.addTest("overflowscrolling",testAllProps("overflowScrolling","touch",!0));Modernizr.addTest("csspointerevents",function(){var e=createElement("a").style;return e.cssText="pointer-events:auto",e.pointerEvents==="auto"});Modernizr.addTest("csspositionsticky",function(){var e="position:",A="sticky",t=createElement("a"),r=t.style;return r.cssText=e+prefixes.join(A+";"+e).slice(0,-e.length),r.position.indexOf(A)!==-1});Modernizr.addTest("csspseudoanimations",function(){var e=!1;if(!Modernizr.cssanimations)return e;var A=["@",prefixes.join("keyframes csspseudoanimations { from { font-size: 10px; } }@").replace(/\@$/,""),'#modernizr:before { content:" "; font-size:5px;',prefixes.join("animation:csspseudoanimations 1ms infinite;"),"}"].join("");return testStyles(A,function(t){e=computedStyle(t,":before","font-size")==="10px"}),e});Modernizr.addTest("csstransitions",testAllProps("transition","all",!0));Modernizr.addTest("csspseudotransitions",function(){var e=!1;if(!Modernizr.csstransitions)return e;var A='#modernizr:before { content:" "; font-size:5px;'+prefixes.join("transition:0s 100s;")+"}#modernizr.trigger:before { font-size:10px; }";return testStyles(A,function(t){computedStyle(t,":before","font-size"),t.className+="trigger",e=computedStyle(t,":before","font-size")==="5px"}),e});Modernizr.addTest("cssreflections",testAllProps("boxReflect","above",!0));Modernizr.addTest("regions",function(){if(isSVG)return!1;var e=prefixed("flowFrom"),A=prefixed("flowInto"),t=!1;if(!e||!A)return t;var r=createElement("iframe"),n=createElement("div"),o=createElement("div"),a=createElement("div"),l="modernizr_flow_for_regions_check";o.innerText="M",n.style.cssText="top: 150px; left: 150px; padding: 0px;",a.style.cssText="width: 50px; height: 50px; padding: 42px;",a.style[e]=l,n.appendChild(o),n.appendChild(a),docElement.appendChild(n);var s,f,u=o.getBoundingClientRect();return o.style[A]=l,s=o.getBoundingClientRect(),f=parseInt(s.left-u.left,10),docElement.removeChild(n),f===42?t=!0:(docElement.appendChild(r),u=r.getBoundingClientRect(),r.style[A]=l,s=r.getBoundingClientRect(),u.height>0&&u.height!==s.height&&s.height===0&&(t=!0)),o=a=n=r=undefined,t});Modernizr.addTest("cssremunit",function(){var e=createElement("a").style;try{e.fontSize="3rem"}catch{}return/rem/.test(e.fontSize)});Modernizr.addTest("cssresize",testAllProps("resize","both",!0));Modernizr.addTest("rgba",function(){var e=createElement("a").style;return e.cssText="background-color:rgba(150,255,150,.5)",(""+e.backgroundColor).indexOf("rgba")>-1});testStyles("#modernizr{overflow: scroll; width: 40px; height: 40px; }#"+prefixes.join("scrollbar{width:10px} #modernizr::").split("#").slice(1).join("#")+"scrollbar{width:10px}",function(e){Modernizr.addTest("cssscrollbar","scrollWidth"in e&&e.scrollWidth===30)});Modernizr.addTest("scrollsnappoints",testAllProps("scrollSnapType"));Modernizr.addTest("shapes",testAllProps("shapeOutside","content-box",!0));Modernizr.addTest("siblinggeneral",function(){return testStyles("#modernizr div {width:100px} #modernizr div ~ div {width:200px;display:block}",function(e){return e.lastChild.offsetWidth===200},2)});testStyles("#modernizr{position: absolute; top: -10em; visibility:hidden; font: normal 10px arial;}#subpixel{float: left; font-size: 33.3333%;}",function(e){var A=e.firstChild;A.innerHTML="This is a text written in Arial",Modernizr.addTest("subpixelfont",computedStyle(A,null,"width")!=="44px")},1,["subpixel"]);Modernizr.addTest("target",function(){var e=window.document;if(!("querySelectorAll"in e))return!1;try{return e.querySelectorAll(":target"),!0}catch{return!1}});Modernizr.addTest("textalignlast",testAllProps("textAlignLast"));(function(){Modernizr.addTest("textdecoration",function(){var n=!1,o=testAllProps("textDecoration");try{n=!!o,n&&(n=new Boolean(n))}catch{}return n});for(var e=["Line","Style","Color","Skip","SkipInk"],A,t,r=0;r<e.length;r++)A=e[r].toLowerCase(),t=testAllProps("textDecoration"+e[r]),Modernizr.addTest("textdecoration."+A,t)})();Modernizr.addTest("textshadow",testProp("textShadow","1px 1px"));Modernizr.addTest("csstransforms",function(){return navigator.userAgent.indexOf("Android 2.")===-1&&testAllProps("transform","scale(1)",!0)});Modernizr.addTest("csstransforms3d",function(){return!!testAllProps("perspective","1px",!0)});Modernizr.addTest("csstransformslevel2",function(){return testAllProps("translate","45px",!0)});Modernizr.addTest("preserve3d",function(){var e,A,t=window.CSS,r=!1;return t&&t.supports&&t.supports("(transform-style: preserve-3d)")?!0:(e=createElement("a"),A=createElement("a"),e.style.cssText="display: block; transform-style: preserve-3d; transform-origin: right; transform: rotateY(40deg);",A.style.cssText="display: block; width: 9px; height: 1px; background: #000; transform-origin: right; transform: rotateY(40deg);",e.appendChild(A),docElement.appendChild(e),r=A.getBoundingClientRect(),docElement.removeChild(e),r=r.width&&r.width<4,r)});Modernizr.addTest("userselect",testAllProps("userSelect","none",!0));Modernizr.addTest("cssvalid",function(){return testStyles("#modernizr input{height:0;border:0;padding:0;margin:0;width:10px} #modernizr input:valid{width:50px}",function(e){var A=createElement("input");return e.appendChild(A),A.clientWidth>10})});Modernizr.addTest("variablefonts",testAllProps("fontVariationSettings"));testStyles("#modernizr { height: 50vh; max-height: 10px; }",function(e){var A=parseInt(computedStyle(e,null,"height"),10);Modernizr.addTest("cssvhunit",A===10)});function roundedEquals(e,A){return e-1===A||e===A||e+1===A}testStyles("#modernizr1{width: 50vmax}#modernizr2{width:50px;height:50px;overflow:scroll}#modernizr3{position:fixed;top:0;left:0;bottom:0;right:0}",function(e){var A=e.childNodes[2],t=e.childNodes[1],r=e.childNodes[0],n=parseInt((t.offsetWidth-t.clientWidth)/2,10),o=r.clientWidth/100,a=r.clientHeight/100,l=parseInt(Math.max(o,a)*50,10),s=parseInt(computedStyle(A,null,"width"),10);Modernizr.addTest("cssvmaxunit",roundedEquals(l,s)||roundedEquals(l,s-n))},3);testStyles("#modernizr1{width: 50vm;width:50vmin}#modernizr2{width:50px;height:50px;overflow:scroll}#modernizr3{position:fixed;top:0;left:0;bottom:0;right:0}",function(e){var A=e.childNodes[2],t=e.childNodes[1],r=e.childNodes[0],n=parseInt((t.offsetWidth-t.clientWidth)/2,10),o=r.clientWidth/100,a=r.clientHeight/100,l=parseInt(Math.min(o,a)*50,10),s=parseInt(computedStyle(A,null,"width"),10);Modernizr.addTest("cssvminunit",roundedEquals(l,s)||roundedEquals(l,s-n))},3);testStyles("#modernizr { width: 50vw; }",function(e){var A=parseInt(window.innerWidth/2,10),t=parseInt(computedStyle(e,null,"width"),10);Modernizr.addTest("cssvwunit",roundedEquals(t,A))});Modernizr.addTest("willchange","willChange"in docElement.style);Modernizr.addTest("wrapflow",function(){var e=prefixed("wrapFlow");if(!e||isSVG)return!1;var A=e.replace(/([A-Z])/g,function(a,l){return"-"+l.toLowerCase()}).replace(/^ms-/,"-ms-"),t=createElement("div"),r=createElement("div"),n=createElement("span");r.style.cssText="position: absolute; left: 50px; width: 100px; height: 20px;"+A+":end;",n.innerText="X",t.appendChild(r),t.appendChild(n),docElement.appendChild(t);var o=n.offsetLeft;return docElement.removeChild(t),r=n=t=undefined,o===150});Modernizr.addTest("customelements","customElements"in window);Modernizr.addTest("customprotocolhandler",function(){if(!navigator.registerProtocolHandler)return!1;try{navigator.registerProtocolHandler("thisShouldFail")}catch(e){return e instanceof TypeError}return!1});Modernizr.addTest("dart",!!prefixed("startDart",navigator));Modernizr.addTest("dataview",typeof DataView<"u"&&"getFloat64"in DataView.prototype);Modernizr.addTest("classlist","classList"in docElement);Modernizr.addTest("createelementattrs",function(){try{return createElement('<input name="test" />').getAttribute("name")==="test"}catch{return!1}},{aliases:["createelement-attrs"]});Modernizr.addTest("dataset",function(){var e=createElement("div");return e.setAttribute("data-a-b","c"),!!(e.dataset&&e.dataset.aB==="c")});Modernizr.addTest("documentfragment",function(){return"createDocumentFragment"in document&&"appendChild"in docElement});Modernizr.addTest("hidden","hidden"in createElement("a"));Modernizr.addTest("intersectionobserver","IntersectionObserver"in window);Modernizr.addTest("microdata","getItems"in document);Modernizr.addTest("mutationobserver",!!window.MutationObserver||!!window.WebKitMutationObserver);Modernizr.addTest("passiveeventlisteners",function(){var e=!1;try{var A=Object.defineProperty({},"passive",{get:function(){e=!0}}),t=function(){};window.addEventListener("testPassiveEventSupport",t,A),window.removeEventListener("testPassiveEventSupport",t,A)}catch{}return e});Modernizr.addTest("shadowroot","attachShadow"in createElement("div"));Modernizr.addTest("shadowrootlegacy","createShadowRoot"in createElement("div"));Modernizr.addTest("bdi",function(){var e=createElement("div"),A=createElement("bdi");A.innerHTML="&#1573;",e.appendChild(A),docElement.appendChild(e);var t=computedStyle(A,null,"direction")==="rtl";return docElement.removeChild(e),t});Modernizr.addTest("details",function(){var e=createElement("details"),A;return"open"in e?(testStyles("#modernizr details{display:block}",function(t){t.appendChild(e),e.innerHTML="<summary>a</summary>b",A=e.offsetHeight,e.open=!0,A=A!==e.offsetHeight}),A):!1});Modernizr.addTest("outputelem","value"in createElement("output"));Modernizr.addTest("picture","HTMLPictureElement"in window);Modernizr.addTest("progressbar",createElement("progress").max!==undefined),Modernizr.addTest("meter",createElement("meter").max!==undefined);Modernizr.addTest("ruby",function(){var e=createElement("ruby"),A=createElement("rt"),t=createElement("rp");if(e.appendChild(t),e.appendChild(A),docElement.appendChild(e),computedStyle(t,null,"display")==="none"||computedStyle(e,null,"display")==="ruby"&&computedStyle(A,null,"display")==="ruby-text"||computedStyle(t,null,"fontSize")==="6pt"&&computedStyle(A,null,"fontSize")==="6pt")return r(),!0;return r(),!1;function r(){docElement.removeChild(e),e=null,A=null,t=null}});Modernizr.addTest("template","content"in createElement("template"));Modernizr.addTest("time","valueAsDate"in createElement("time"));Modernizr.addTest("texttrackapi",typeof createElement("video").addTextTrack=="function"),Modernizr.addTest("track","kind"in createElement("track"));Modernizr.addTest("unknownelements",function(){var e=createElement("a");return e.innerHTML="<xyz></xyz>",e.childNodes.length===1});Modernizr.addTest("emoji",function(){if(!Modernizr.canvastext)return!1;var e=createElement("canvas"),A=e.getContext("2d"),t=A.webkitBackingStorePixelRatio||A.mozBackingStorePixelRatio||A.msBackingStorePixelRatio||A.oBackingStorePixelRatio||A.backingStorePixelRatio||1,r=12*t;return A.fillStyle="#f00",A.textBaseline="top",A.font="32px Arial",A.fillText("\u{1F428}",0,0),A.getImageData(r,r,1,1).data[0]!==0});Modernizr.addTest("es5array",function(){return!!(Array.prototype&&Array.prototype.every&&Array.prototype.filter&&Array.prototype.forEach&&Array.prototype.indexOf&&Array.prototype.lastIndexOf&&Array.prototype.map&&Array.prototype.some&&Array.prototype.reduce&&Array.prototype.reduceRight&&Array.isArray)});Modernizr.addTest("es5date",function(){var e="2013-04-12T06:06:37.307Z",A=!1;try{A=!!Date.parse(e)}catch{}return!!(Date.now&&Date.prototype&&Date.prototype.toISOString&&Date.prototype.toJSON&&A)});Modernizr.addTest("es5function",function(){return!!(Function.prototype&&Function.prototype.bind)});Modernizr.addTest("es5object",function(){return!!(Object.keys&&Object.create&&Object.getPrototypeOf&&Object.getOwnPropertyNames&&Object.isSealed&&Object.isFrozen&&Object.isExtensible&&Object.getOwnPropertyDescriptor&&Object.defineProperty&&Object.defineProperties&&Object.seal&&Object.freeze&&Object.preventExtensions)});Modernizr.addTest("strictmode",function(){"use strict";return!this}());Modernizr.addTest("es5string",function(){return!!(String.prototype&&String.prototype.trim)});Modernizr.addTest("json","JSON"in window&&"parse"in JSON&&"stringify"in JSON);Modernizr.addTest("es5syntax",function(){var value,obj,stringAccess,getter,setter,reservedWords,zeroWidthChars;try{return stringAccess=eval('"foobar"[3] === "b"'),getter=eval("({ get x(){ return 1 } }).x === 1"),eval("({ set x(v){ value = v; } }).x = 1"),setter=value===1,eval("obj = ({ if: 1 })"),reservedWords=obj.if===1,zeroWidthChars=eval("_\u200C\u200D = true"),stringAccess&&getter&&setter&&reservedWords&&zeroWidthChars}catch(e){return!1}});Modernizr.addTest("es5undefined",function(){var e,A;try{A=window.undefined,window.undefined=12345,e=typeof window.undefined>"u",window.undefined=A}catch{return!1}return e});Modernizr.addTest("es5",function(){return!!(Modernizr.es5array&&Modernizr.es5date&&Modernizr.es5function&&Modernizr.es5object&&Modernizr.strictmode&&Modernizr.es5string&&Modernizr.json&&Modernizr.es5syntax&&Modernizr.es5undefined)});Modernizr.addTest("es6array",!!(Array.prototype&&Array.prototype.copyWithin&&Array.prototype.fill&&Array.prototype.find&&Array.prototype.findIndex&&Array.prototype.keys&&Array.prototype.entries&&Array.prototype.values&&Array.from&&Array.of));Modernizr.addTest("arrow",function(){try{eval("()=>{}")}catch(e){return!1}return!0});Modernizr.addTest("es6class",function(){try{eval("class A{}")}catch(e){return!1}return!0});Modernizr.addTest("es6collections",!!(window.Map&&window.Set&&window.WeakMap&&window.WeakSet));Modernizr.addTest("generators",function(){try{new Function("function* test() {}")()}catch{return!1}return!0});Modernizr.addTest("es6math",!!(Math&&Math.clz32&&Math.cbrt&&Math.imul&&Math.sign&&Math.log10&&Math.log2&&Math.log1p&&Math.expm1&&Math.cosh&&Math.sinh&&Math.tanh&&Math.acosh&&Math.asinh&&Math.atanh&&Math.hypot&&Math.trunc&&Math.fround));Modernizr.addTest("es6number",!!(Number.isFinite&&Number.isInteger&&Number.isSafeInteger&&Number.isNaN&&Number.parseInt&&Number.parseFloat&&Number.isInteger(Number.MAX_SAFE_INTEGER)&&Number.isInteger(Number.MIN_SAFE_INTEGER)&&Number.isFinite(Number.EPSILON)));Modernizr.addTest("es6object",!!(Object.assign&&Object.is&&Object.setPrototypeOf));Modernizr.addTest("promises",function(){return"Promise"in window&&"resolve"in window.Promise&&"reject"in window.Promise&&"all"in window.Promise&&"race"in window.Promise&&function(){var e;return new window.Promise(function(A){e=A}),typeof e=="function"}()});Modernizr.addTest("restparameters",function(){try{eval("function f(...rest) {}")}catch(e){return!1}return!0});Modernizr.addTest("spreadarray",function(){try{eval("(function f(){})(...[1])")}catch(e){return!1}return!0});Modernizr.addTest("stringtemplate",function(){try{return eval("(function(){var a=1; return `-${a}-`;})()")==="-1-"}catch(e){return!1}});Modernizr.addTest("es6string",!!(String.fromCodePoint&&String.raw&&String.prototype.codePointAt&&String.prototype.repeat&&String.prototype.startsWith&&String.prototype.endsWith&&String.prototype.includes));Modernizr.addTest("es6symbol",!!(typeof Symbol=="function"&&Symbol.for&&Symbol.hasInstance&&Symbol.isConcatSpreadable&&Symbol.iterator&&Symbol.keyFor&&Symbol.match&&Symbol.prototype&&Symbol.replace&&Symbol.search&&Symbol.species&&Symbol.split&&Symbol.toPrimitive&&Symbol.toStringTag&&Symbol.unscopables));Modernizr.addTest("es7array",!!(Array.prototype&&Array.prototype.includes));Modernizr.addTest("restdestructuringarray",function(){try{eval("var [...rest]=[1]")}catch(e){return!1}return!0}),Modernizr.addTest("restdestructuringobject",function(){try{eval("var {...rest}={a:1}")}catch(e){return!1}return!0});Modernizr.addTest("spreadobject",function(){try{eval("var a={...{b:1}}")}catch(e){return!1}return!0});Modernizr.addTest("es8object",!!(Object.entries&&Object.values));Modernizr.addTest("customevent","CustomEvent"in window&&typeof window.CustomEvent=="function");Modernizr.addTest("devicemotion","DeviceMotionEvent"in window),Modernizr.addTest("deviceorientation","DeviceOrientationEvent"in window);Modernizr.addTest("eventlistener","addEventListener"in window);Modernizr.addTest("forcetouch",function(){return hasEvent(prefixed("mouseforcewillbegin",window,!1),window)?MouseEvent.WEBKIT_FORCE_AT_MOUSE_DOWN&&MouseEvent.WEBKIT_FORCE_AT_FORCE_MOUSE_DOWN:!1});Modernizr.addTest("hashchange",function(){return hasEvent("hashchange",window)===!1?!1:document.documentMode===undefined||document.documentMode>7});Modernizr.addTest("oninput",function(){var e=createElement("input"),A;if(e.setAttribute("oninput","return"),e.style.cssText="position:fixed;top:0;",hasEvent("oninput",docElement)||typeof e.oninput=="function")return!0;try{var t=document.createEvent("KeyboardEvent");A=!1;var r=function(n){A=!0,n.preventDefault(),n.stopPropagation()};t.initKeyEvent("keypress",!0,!0,window,!1,!1,!1,!1,0,"e".charCodeAt(0)),docElement.appendChild(e),e.addEventListener("input",r,!1),e.focus(),e.dispatchEvent(t),e.removeEventListener("input",r,!1),docElement.removeChild(e)}catch{A=!1}return A});var domPrefixesAll=[""].concat(domPrefixes);ModernizrProto._domPrefixesAll=domPrefixesAll;Modernizr.addTest("pointerevents",function(){for(var e=0,A=domPrefixesAll.length;e<A;e++)if(hasEvent(domPrefixesAll[e]+"pointerdown"))return!0;return!1});Modernizr.addAsyncTest(function(){var e,A=300;function t(){clearTimeout(e),window.removeEventListener("deviceproximity",t),addTest("proximity",!0)}"ondeviceproximity"in window&&"onuserproximity"in window?(window.addEventListener("deviceproximity",t),e=setTimeout(function(){window.removeEventListener("deviceproximity",t),addTest("proximity",!1)},A)):addTest("proximity",!1)});Modernizr.addTest("filereader",!!(window.File&&window.FileList&&window.FileReader));Modernizr.addTest("filesystem",!!prefixed("requestFileSystem",window));Modernizr.addAsyncTest(function(){var e=function(f){docElement.contains(f)||docElement.appendChild(f)},A=function(f){f.fake&&f.parentNode&&f.parentNode.removeChild(f)},t=function(f,u){var m=!!f;if(m&&(m=new Boolean(m),m.blocked=f==="blocked"),addTest("flash",function(){return m}),u&&a.contains(u)){for(;u.parentNode!==a;)u=u.parentNode;a.removeChild(u)}},r,n;try{n="ActiveXObject"in window&&"Pan"in new window.ActiveXObject("ShockwaveFlash.ShockwaveFlash")}catch{}if(r=!("plugins"in navigator&&"Shockwave Flash"in navigator.plugins||n),r||isSVG)t(!1);else{var o=createElement("embed"),a=getBody(),l,s;if(o.type="application/x-shockwave-flash",a.appendChild(o),!("Pan"in o)&&!n){e(a),t("blocked",o),A(a);return}l=function(){if(e(a),!docElement.contains(a))return a=document.body||a,o=createElement("embed"),o.type="application/x-shockwave-flash",a.appendChild(o),setTimeout(l,1e3);docElement.contains(o)?(s=o.style.cssText,t(s!==""?"blocked":!0,o)):t("blocked"),A(a)},setTimeout(l,10)}});Modernizr.addTest("fullscreen",!!(prefixed("exitFullscreen",document,!1)||prefixed("cancelFullScreen",document,!1)));Modernizr.addTest("gamepads",!!prefixed("getGamepads",navigator));Modernizr.addTest("geolocation","geolocation"in navigator);Modernizr.addTest("hiddenscroll",function(){return testStyles("#modernizr {width:100px;height:100px;overflow:scroll}",function(e){return e.offsetWidth===e.clientWidth})});Modernizr.addTest("history",function(){var e=navigator.userAgent;return!e||(e.indexOf("Android 2.")!==-1||e.indexOf("Android 4.0")!==-1)&&e.indexOf("Mobile Safari")!==-1&&e.indexOf("Chrome")===-1&&e.indexOf("Windows Phone")===-1&&location.protocol!=="file:"?!1:window.history&&"pushState"in window.history});Modernizr.addTest("htmlimports","import"in createElement("link"));Modernizr.addTest("ie8compat",!window.addEventListener&&!!document.documentMode&&document.documentMode===7);Modernizr.addTest("sandbox","sandbox"in createElement("iframe"));Modernizr.addTest("seamless","seamless"in createElement("iframe"));Modernizr.addTest("srcdoc","srcdoc"in createElement("iframe"));Modernizr.addAsyncTest(function(){if(!Modernizr.canvas)return!1;var e=new Image,A=createElement("canvas"),t=A.getContext("2d");e.onload=function(){addTest("apng",function(){return typeof A.getContext>"u"?!1:(t.drawImage(e,0,0),t.getImageData(0,0,1,1).data[3]===0)})},e.src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAACGFjVEwAAAABAAAAAcMq2TYAAAANSURBVAiZY2BgYPgPAAEEAQB9ssjfAAAAGmZjVEwAAAAAAAAAAQAAAAEAAAAAAAAAAAD6A+gBAbNU+2sAAAARZmRBVAAAAAEImWNgYGBgAAAABQAB6MzFdgAAAABJRU5ErkJggg=="});Modernizr.addAsyncTest(function(){var e=new Image;e.onload=e.onerror=function(){addTest("avif",e.width===1)},e.src="data:image/avif;base64,AAAAIGZ0eXBhdmlmAAAAAGF2aWZtaWYxbWlhZk1BMUIAAAEcbWV0YQAAAAAAAABIaGRscgAAAAAAAAAAcGljdAAAAAAAAAAAAAAAAGNhdmlmIC0gaHR0cHM6Ly9naXRodWIuY29tL2xpbmstdS9jYXZpZgAAAAAeaWxvYwAAAAAEQAABAAEAAAAAAUQAAQAAABcAAAAqaWluZgEAAAAAAAABAAAAGmluZmUCAAAAAAEAAGF2MDFJbWFnZQAAAAAOcGl0bQAAAAAAAQAAAHJpcHJwAAAAUmlwY28AAAAQcGFzcAAAAAEAAAABAAAAFGlzcGUAAAAAAAAAAQAAAAEAAAAQcGl4aQAAAAADCAgIAAAAFmF2MUOBAAwACggYAAYICGgIIAAAABhpcG1hAAAAAAAAAAEAAQUBAoMDhAAAAB9tZGF0CggYAAYICGgIIBoFHiAAAEQiBACwDoA="});Modernizr.addTest("imgcrossorigin","crossOrigin"in createElement("img"));Modernizr.addAsyncTest(function(){var e=new Image;e.onerror=function(){addTest("exiforientation",!1,{aliases:["exif-orientation"]})},e.onload=function(){addTest("exiforientation",e.width!==2,{aliases:["exif-orientation"]})},e.src="data:image/jpeg;base64,/9j/4AAQSkZJRgABAQEAYABgAAD/4QAiRXhpZgAASUkqAAgAAAABABIBAwABAAAABgASAAAAAAD/2wBDAAEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQH/2wBDAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQH/wAARCAABAAIDASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD+/iiiigD/2Q=="});Modernizr.addAsyncTest(function(){var e=new Image;e.onload=e.onerror=function(){addTest("jpeg2000",e.width===1)},e.src="data:image/jp2;base64,/0//UQAyAAAAAAABAAAAAgAAAAAAAAAAAAAABAAAAAQAAAAAAAAAAAAEBwEBBwEBBwEBBwEB/1IADAAAAAEAAAQEAAH/XAAEQED/ZAAlAAFDcmVhdGVkIGJ5IE9wZW5KUEVHIHZlcnNpb24gMi4wLjD/kAAKAAAAAABYAAH/UwAJAQAABAQAAf9dAAUBQED/UwAJAgAABAQAAf9dAAUCQED/UwAJAwAABAQAAf9dAAUDQED/k8+kEAGvz6QQAa/PpBABr994EAk//9k="});Modernizr.addAsyncTest(function(){var e=new Image;e.onload=e.onerror=function(){addTest("jpegxr",e.width===1,{aliases:["jpeg-xr"]})},e.src="data:image/vnd.ms-photo;base64,SUm8AQgAAAAFAAG8AQAQAAAASgAAAIC8BAABAAAAAQAAAIG8BAABAAAAAQAAAMC8BAABAAAAWgAAAMG8BAABAAAAHwAAAAAAAAAkw91vA07+S7GFPXd2jckNV01QSE9UTwAZAYBxAAAAABP/gAAEb/8AAQAAAQAAAA=="});Modernizr.addTest("lazyloading","loading"in HTMLImageElement.prototype);Modernizr.addAsyncTest(function(){var e,A,t,r=createElement("img"),n="sizes"in r;!n&&"srcset"in r?(A="data:image/gif;base64,R0lGODlhAgABAPAAAP///wAAACH5BAAAAAAALAAAAAACAAEAAAICBAoAOw==",e="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==",t=function(){addTest("sizes",r.width===2)},r.onload=t,r.onerror=t,r.setAttribute("sizes","9px"),r.srcset=e+" 1w,"+A+" 8w",r.src=e):addTest("sizes",n)});Modernizr.addTest("srcset","srcset"in createElement("img"));Modernizr.addAsyncTest(function(){var e=new Image;e.onerror=function(){addTest("webpalpha",!1,{aliases:["webp-alpha"]})},e.onload=function(){addTest("webpalpha",e.width===1,{aliases:["webp-alpha"]})},e.src="data:image/webp;base64,UklGRkoAAABXRUJQVlA4WAoAAAAQAAAAAAAAAAAAQUxQSAwAAAABBxAR/Q9ERP8DAABWUDggGAAAADABAJ0BKgEAAQADADQlpAADcAD++/1QAA=="});Modernizr.addAsyncTest(function(){var e=new Image;e.onerror=function(){addTest("webpanimation",!1,{aliases:["webp-animation"]})},e.onload=function(){addTest("webpanimation",e.width===1,{aliases:["webp-animation"]})},e.src="data:image/webp;base64,UklGRlIAAABXRUJQVlA4WAoAAAASAAAAAAAAAAAAQU5JTQYAAAD/////AABBTk1GJgAAAAAAAAAAAAAAAAAAAGQAAABWUDhMDQAAAC8AAAAQBxAREYiI/gcA"});Modernizr.addAsyncTest(function(){var e=new Image;e.onerror=function(){addTest("webplossless",!1,{aliases:["webp-lossless"]})},e.onload=function(){addTest("webplossless",e.width===1,{aliases:["webp-lossless"]})},e.src="data:image/webp;base64,UklGRh4AAABXRUJQVlA4TBEAAAAvAAAAAAfQ//73v/+BiOh/AAA="});Modernizr.addAsyncTest(function(){var e=[{uri:"data:image/webp;base64,UklGRiQAAABXRUJQVlA4IBgAAAAwAQCdASoBAAEAAwA0JaQAA3AA/vuUAAA=",name:"webp"},{uri:"data:image/webp;base64,UklGRkoAAABXRUJQVlA4WAoAAAAQAAAAAAAAAAAAQUxQSAwAAAABBxAR/Q9ERP8DAABWUDggGAAAADABAJ0BKgEAAQADADQlpAADcAD++/1QAA==",name:"webp.alpha"},{uri:"data:image/webp;base64,UklGRlIAAABXRUJQVlA4WAoAAAASAAAAAAAAAAAAQU5JTQYAAAD/////AABBTk1GJgAAAAAAAAAAAAAAAAAAAGQAAABWUDhMDQAAAC8AAAAQBxAREYiI/gcA",name:"webp.animation"},{uri:"data:image/webp;base64,UklGRh4AAABXRUJQVlA4TBEAAAAvAAAAAAfQ//73v/+BiOh/AAA=",name:"webp.lossless"}],A=e.shift();function t(r,n,o){var a=new Image;function l(s){var f=s&&s.type==="load"?a.width===1:!1,u=r==="webp";addTest(r,u&&f?new Boolean(f):f),o&&o(s)}a.onerror=l,a.onload=l,a.src=n}t(A.name,A.uri,function(r){if(r&&r.type==="load")for(var n=0;n<e.length;n++)t(e[n].name,e[n].uri)})});Modernizr.addTest("capture","capture"in createElement("input"));Modernizr.addTest("fileinput",function(){var e=navigator.userAgent;if(e.match(/(Android (1.0|1.1|1.5|1.6|2.0|2.1))|(Windows Phone (OS 7|8.0))|(XBLWP)|(ZuneWP)|(w(eb)?OSBrowser)|(webOS)|(Kindle\/(1.0|2.0|2.5|3.0))/)||e.match(/\swv\).+(chrome)\/([\w\.]+)/i))return!1;var A=createElement("input");return A.type="file",!A.disabled});Modernizr.addTest("fileinputdirectory",function(){var e=createElement("input"),A="directory";e.type="file";for(var t=0,r=domPrefixesAll.length;t<r;t++)if(domPrefixesAll[t]+A in e)return!0;return!1});Modernizr.addTest("inputformaction","formAction"in createElement("input"),{aliases:["input-formaction"]});Modernizr.addTest("formattribute",function(){var e=createElement("form"),A=createElement("input"),t=createElement("div"),r="formtest"+new Date().getTime(),n,o=!1;e.id=r;try{A.setAttribute("form",r)}catch{document.createAttribute&&(n=document.createAttribute("form"),n.nodeValue=r,A.setAttributeNode(n))}return t.appendChild(e),t.appendChild(A),docElement.appendChild(t),o=e.elements&&e.elements.length===1&&A.form===e,t.parentNode.removeChild(t),o});Modernizr.addTest("inputformenctype","formEnctype"in createElement("input"),{aliases:["input-formenctype"]});Modernizr.addTest("inputformmethod","formMethod"in createElement("input"));Modernizr.addTest("inputformnovalidate","formNoValidate"in createElement("input"),{aliases:["input-formnovalidate"]});Modernizr.addTest("inputformtarget","formTarget"in createElement("input"),{aliases:["input-formtarget"]});var inputElem=createElement("input");var inputattrs="autocomplete autofocus list placeholder max min multiple pattern required step".split(" "),attrs={};Modernizr.input=function(e){for(var A=0,t=e.length;A<t;A++)attrs[e[A]]=e[A]in inputElem;return attrs.list&&(attrs.list=!!(createElement("datalist")&&window.HTMLDataListElement)),attrs}(inputattrs);(function(){for(var e=["search","tel","url","email","datetime","date","month","week","time","datetime-local","number","range","color"],A="1)",t,r,n,o=0;o<e.length;o++)inputElem.setAttribute("type",t=e[o]),n=inputElem.type!=="text"&&"style"in inputElem,n&&(inputElem.value=A,inputElem.style.cssText="position:absolute;visibility:hidden;",/^range$/.test(t)&&inputElem.style.WebkitAppearance!==undefined?(docElement.appendChild(inputElem),r=document.defaultView,n=r.getComputedStyle&&r.getComputedStyle(inputElem,null).WebkitAppearance!=="textfield"&&inputElem.offsetHeight!==0,docElement.removeChild(inputElem)):/^(search|tel)$/.test(t)||(/^(url|email)$/.test(t)?n=inputElem.checkValidity&&inputElem.checkValidity()===!1:n=inputElem.value!==A)),Modernizr.addTest("inputtypes."+t,!!n)})();Modernizr.addTest("formvalidation",function(){var e=createElement("form");if(!("checkValidity"in e)||!("addEventListener"in e))return!1;if("reportValidity"in e)return!0;var A=!1,t;return Modernizr.formvalidationapi=!0,e.addEventListener("submit",function(r){(!window.opera||window.operamini)&&r.preventDefault(),r.stopPropagation()},!1),e.innerHTML='<input name="modTest" required="required" /><button></button>',testStyles("#modernizr form{position:absolute;top:-99999em}",function(r){r.appendChild(e),t=e.getElementsByTagName("input")[0],t.addEventListener("invalid",function(n){A=!0,n.preventDefault(),n.stopPropagation()},!1),Modernizr.formvalidationmessage=!!t.validationMessage,e.getElementsByTagName("button")[0].click()}),A});Modernizr.addTest("localizednumber",function(){if(!Modernizr.inputtypes.number||!Modernizr.formvalidation)return!1;var e=getBody(),A=createElement("div"),t=e.firstElementChild||e.firstChild,r;e.insertBefore(A,t),A.innerHTML='<input type="number" value="1.0" step="0.1" style="position: fixed; top: 0;" />';var n=A.childNodes[0];e.appendChild(A),n.focus();try{document.execCommand("SelectAll",!1),document.execCommand("InsertText",!1,"1,1")}catch{}return r=n.type==="number"&&n.valueAsNumber===1.1&&n.checkValidity(),e.removeChild(A),e.fake&&e.parentNode&&e.parentNode.removeChild(e),r});Modernizr.addTest("inputsearchevent",hasEvent("search"));Modernizr.addTest("placeholder","placeholder"in createElement("input")&&"placeholder"in createElement("textarea"));Modernizr.addTest("requestautocomplete",!!prefixed("requestAutocomplete",createElement("form")));Modernizr.addTest("intl",!!prefixed("Intl",window));Modernizr.addTest("ligatures",testAllProps("fontFeatureSettings",'"liga" 1'));Modernizr.addTest("olreversed","reversed"in createElement("ol"));Modernizr.addTest("mathml",function(){var e;return testStyles("#modernizr{position:absolute;display:inline-block}",function(A){A.innerHTML+="<math><mfrac><mi>xx</mi><mi>yy</mi></mfrac></math>",e=A.offsetHeight>A.offsetWidth}),e});Modernizr.addTest("mediasource","MediaSource"in window);Modernizr.addTest("hovermq",mq("(hover)"));Modernizr.addTest("pointermq",mq("(pointer:coarse),(pointer:fine),(pointer:none)"));Modernizr.addTest("messagechannel","MessageChannel"in window);Modernizr.addTest("beacon","sendBeacon"in navigator);Modernizr.addTest("effectiveType",function(){var e=navigator.connection||{effectiveType:0};return e.effectiveType!==0});Modernizr.addTest("lowbandwidth",function(){var e=navigator.connection||{type:0,effectiveType:0};return e.type===3||e.type===4||/^[23]g$/.test(e.effectiveType)});Modernizr.addTest("eventsource","EventSource"in window);Modernizr.addTest("fetch","fetch"in window);var testXhrType=function(e){if(typeof XMLHttpRequest>"u")return!1;var A=new XMLHttpRequest;A.open("get","/",!0);try{A.responseType=e}catch{return!1}return"response"in A&&A.responseType===e};Modernizr.addTest("xhrresponsetypearraybuffer",testXhrType("arraybuffer"));Modernizr.addTest("xhrresponsetypeblob",testXhrType("blob"));Modernizr.addTest("xhrresponsetypedocument",testXhrType("document"));Modernizr.addTest("xhrresponsetypejson",testXhrType("json"));Modernizr.addTest("xhrresponsetypetext",testXhrType("text"));Modernizr.addTest("xhrresponsetype",function(){if(typeof XMLHttpRequest>"u")return!1;var e=new XMLHttpRequest;return e.open("get","/",!0),"response"in e}());Modernizr.addTest("xhr2","XMLHttpRequest"in window&&"withCredentials"in new XMLHttpRequest);Modernizr.addTest("notification",function(){if(!window.Notification||!window.Notification.requestPermission)return!1;if(window.Notification.permission==="granted")return!0;try{new window.Notification("")}catch(e){if(e.name==="TypeError")return!1}return!0});Modernizr.addTest("pagevisibility",!!prefixed("hidden",document,!1));Modernizr.addTest("performance",!!prefixed("performance",window));Modernizr.addTest("pointerlock",!!prefixed("exitPointerLock",document));var bool=!0;try{window.postMessage({toString:function(){bool=!1}},"*")}catch(e){}Modernizr.addTest("postmessage",new Boolean("postMessage"in window)),Modernizr.addTest("postmessage.structuredclones",bool);Modernizr.addTest("proxy","Proxy"in window);Modernizr.addTest("queryselector","querySelector"in document&&"querySelectorAll"in document);Modernizr.addTest("prefetch",function(){if(document.documentMode===11)return!0;var e=createElement("link").relList;return!e||!e.supports?!1:e.supports("prefetch")});Modernizr.addTest("requestanimationframe",!!prefixed("requestAnimationFrame",window),{aliases:["raf"]});Modernizr.addTest("scriptasync","async"in createElement("script"));Modernizr.addTest("scriptdefer","defer"in createElement("script"));Modernizr.addTest("scrolltooptions",function(){var e=getBody(),A=window.pageYOffset,t=e.clientHeight<=window.innerHeight;if(t){var r=createElement("div");r.style.height=window.innerHeight-e.clientHeight+1+"px",r.style.display="block",e.appendChild(r)}window.scrollTo({top:1});var n=window.pageYOffset!==0;return t&&e.removeChild(r),window.scrollTo(0,A),n});Modernizr.addTest("serviceworker","serviceWorker"in navigator);Modernizr.addTest("speechrecognition",function(){try{return!!prefixed("SpeechRecognition",window)}catch{return!1}});Modernizr.addTest("speechsynthesis",function(){try{return"SpeechSynthesisUtterance"in window}catch{return!1}});Modernizr.addTest("cookies",function(){try{document.cookie="cookietest=1";var e=document.cookie.indexOf("cookietest=")!==-1;return document.cookie="cookietest=1; expires=Thu, 01-Jan-1970 00:00:01 GMT",e}catch{return!1}});Modernizr.addAsyncTest(function(){var e;try{e=prefixed("indexedDB",window)}catch{}if(e){var A="modernizr-"+Math.random(),t;try{t=e.open(A)}catch{addTest("indexeddb",!1);return}t.onerror=function(r){t.error&&(t.error.name==="InvalidStateError"||t.error.name==="UnknownError")?(addTest("indexeddb",!1),r.preventDefault()):(addTest("indexeddb",!0),detectDeleteDatabase(e,A))},t.onsuccess=function(){addTest("indexeddb",!0),detectDeleteDatabase(e,A)}}else addTest("indexeddb",!1)});function detectDeleteDatabase(e,A){var t=e.deleteDatabase(A);t.onsuccess=function(){addTest("indexeddb.deletedatabase",!0)},t.onerror=function(){addTest("indexeddb.deletedatabase",!1)}}Modernizr.addAsyncTest(function(){var e,A="detect-blob-support",t=!1,r,n,o;try{e=prefixed("indexedDB",window)}catch{}if(!(Modernizr.indexeddb&&Modernizr.indexeddb.deletedatabase))return!1;try{e.deleteDatabase(A).onsuccess=function(){r=e.open(A,1),r.onupgradeneeded=function(){r.result.createObjectStore("store")},r.onsuccess=function(){n=r.result;try{o=n.transaction("store","readwrite").objectStore("store").put(new Blob,"key"),o.onsuccess=function(){t=!0},o.onerror=function(){t=!1}}catch{t=!1}finally{addTest("indexeddbblob",t),n.close(),e.deleteDatabase(A)}}}}catch{addTest("indexeddbblob",!1)}});Modernizr.addAsyncTest(function(){Modernizr.on("indexeddb",function(e){e&&addTest("indexeddb2","getAll"in IDBIndex.prototype)})});Modernizr.addTest("localstorage",function(){var e="modernizr";try{return localStorage.setItem(e,e),localStorage.removeItem(e),!0}catch{return!1}});Modernizr.addTest("quotamanagement",function(){var e=prefixed("temporaryStorage",navigator),A=prefixed("persistentStorage",navigator);return!!(e&&A)});Modernizr.addTest("sessionstorage",function(){var e="modernizr";try{return sessionStorage.setItem(e,e),sessionStorage.removeItem(e),!0}catch{return!1}});Modernizr.addTest("userdata",!!createElement("div").addBehavior);Modernizr.addTest("websqldatabase","openDatabase"in window);Modernizr.addTest("stylescoped","scoped"in createElement("style"));Modernizr.addTest("svg",!!document.createElementNS&&!!document.createElementNS("http://www.w3.org/2000/svg","svg").createSVGRect);Modernizr.addTest("svgasimg",document.implementation.hasFeature("http://www.w3.org/TR/SVG11/feature#Image","1.1"));var toStringFn={}.toString;Modernizr.addTest("svgclippaths",function(){return!!document.createElementNS&&/SVGClipPath/.test(toStringFn.call(document.createElementNS("http://www.w3.org/2000/svg","clipPath")))});Modernizr.addTest("svgfilters",function(){var e=!1;try{e="SVGFEColorMatrixElement"in window&&SVGFEColorMatrixElement.SVG_FECOLORMATRIX_TYPE_SATURATE===2}catch{}return e});Modernizr.addTest("svgforeignobject",function(){return!!document.createElementNS&&/SVGForeignObject/.test(toStringFn.call(document.createElementNS("http://www.w3.org/2000/svg","foreignObject")))});Modernizr.addTest("inlinesvg",function(){var e=createElement("div");return e.innerHTML="<svg/>",(typeof SVGRect<"u"&&e.firstChild&&e.firstChild.namespaceURI)==="http://www.w3.org/2000/svg"});Modernizr.addTest("smil",function(){return!!document.createElementNS&&/SVGAnimate/.test(toStringFn.call(document.createElementNS("http://www.w3.org/2000/svg","animate")))});Modernizr.addTest("textareamaxlength","maxLength"in createElement("textarea"));Modernizr.addTest("textencoder",!!(window.TextEncoder&&window.TextEncoder.prototype.encode)),Modernizr.addTest("textdecoder",!!(window.TextDecoder&&window.TextDecoder.prototype.decode));Modernizr.addTest("typedarrays","ArrayBuffer"in window);Modernizr.addTest("unicoderange",function(){return testStyles('@font-face{font-family:"unicodeRange";src:local("Arial");unicode-range:U+0020,U+002E}#modernizr span{font-size:20px;display:inline-block;font-family:"unicodeRange",monospace}#modernizr .mono{font-family:monospace}',function(e){for(var A=[".",".","m","m"],t=0;t<A.length;t++){var r=createElement("span");r.innerHTML=A[t],r.className=t%2?"mono":"",e.appendChild(r),A[t]=r.clientWidth}return A[0]!==A[1]&&A[2]===A[3]})});var url=prefixed("URL",window,!1);url=url&&window[url],Modernizr.addTest("bloburls",url&&"revokeObjectURL"in url&&"createObjectURL"in url);Modernizr.addAsyncTest(function(){navigator.userAgent.indexOf("MSIE 7.")!==-1&&setTimeout(function(){Modernizr.addTest("datauri",new Boolean(!1))},10);var e=new Image;e.onerror=function(){Modernizr.addTest("datauri",new Boolean(!1))},e.onload=function(){e.width===1&&e.height===1?A():Modernizr.addTest("datauri",new Boolean(!1))},e.src="data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==";function A(){var t=new Image;t.onerror=function(){Modernizr.addTest("datauri",new Boolean(!0)),Modernizr.addTest("datauri.over32kb",!1)},t.onload=function(){Modernizr.addTest("datauri",new Boolean(!0)),Modernizr.addTest("datauri.over32kb",t.width===1&&t.height===1)};for(var r="R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==";r.length<33e3;)r=`\r
// [ts] `+r;t.src="data:image/gif;base64,"+r}});Modernizr.addTest("urlparser",function(){var e;try{return e=new URL("http://modernizr.com/"),e.href==="http://modernizr.com/"}catch{return!1}});Modernizr.addTest("urlsearchparams","URLSearchParams"in window);Modernizr.addTest("vibrate",!!prefixed("vibrate",navigator));(function(){var e=createElement("video");Modernizr.addTest("video",function(){var A=!1;try{A=!!e.canPlayType,A&&(A=new Boolean(A))}catch{}return A});try{e.canPlayType&&(Modernizr.addTest("video.ogg",e.canPlayType('video/ogg; codecs="theora"').replace(/^no$/,"")),Modernizr.addTest("video.h264",e.canPlayType('video/mp4; codecs="avc1.42E01E"').replace(/^no$/,"")),Modernizr.addTest("video.h265",e.canPlayType('video/mp4; codecs="hev1"').replace(/^no$/,"")),Modernizr.addTest("video.webm",e.canPlayType('video/webm; codecs="vp8, vorbis"').replace(/^no$/,"")),Modernizr.addTest("video.vp9",e.canPlayType('video/webm; codecs="vp9"').replace(/^no$/,"")),Modernizr.addTest("video.hls",e.canPlayType('application/x-mpegURL; codecs="avc1.42E01E"').replace(/^no$/,"")),Modernizr.addTest("video.av1",e.canPlayType('video/mp4; codecs="av01"').replace(/^no$/,"")))}catch{}})();Modernizr.addAsyncTest(function(){var e,A=200,t=5,r=0,n=createElement("video"),o=n.style;function a(l){r++,clearTimeout(e);var s=l&&l.type==="playing"||n.currentTime!==0;if(!s&&r<t){e=setTimeout(a,A);return}n.removeEventListener("playing",a,!1),addTest("videoautoplay",s),n.parentNode&&n.parentNode.removeChild(n)}if(!Modernizr.video||!("autoplay"in n)){addTest("videoautoplay",!1);return}o.position="absolute",o.height=0,o.width=0;try{if(Modernizr.video.ogg)n.src="data:video/ogg;base64,T2dnUwACAAAAAAAAAABmnCATAAAAAHDEixYBKoB0aGVvcmEDAgEAAQABAAAQAAAQAAAAAAAFAAAAAQAAAAAAAAAAAGIAYE9nZ1MAAAAAAAAAAAAAZpwgEwEAAAACrA7TDlj///////////////+QgXRoZW9yYSsAAABYaXBoLk9yZyBsaWJ0aGVvcmEgMS4xIDIwMDkwODIyIChUaHVzbmVsZGEpAQAAABoAAABFTkNPREVSPWZmbXBlZzJ0aGVvcmEtMC4yOYJ0aGVvcmG+zSj3uc1rGLWpSUoQc5zmMYxSlKQhCDGMYhCEIQhAAAAAAAAAAAAAEW2uU2eSyPxWEvx4OVts5ir1aKtUKBMpJFoQ/nk5m41mUwl4slUpk4kkghkIfDwdjgajQYC8VioUCQRiIQh8PBwMhgLBQIg4FRba5TZ5LI/FYS/Hg5W2zmKvVoq1QoEykkWhD+eTmbjWZTCXiyVSmTiSSCGQh8PB2OBqNBgLxWKhQJBGIhCHw8HAyGAsFAiDgUCw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDw8PDAwPEhQUFQ0NDhESFRUUDg4PEhQVFRUOEBETFBUVFRARFBUVFRUVEhMUFRUVFRUUFRUVFRUVFRUVFRUVFRUVEAwLEBQZGxwNDQ4SFRwcGw4NEBQZHBwcDhATFhsdHRwRExkcHB4eHRQYGxwdHh4dGxwdHR4eHh4dHR0dHh4eHRALChAYKDM9DAwOExo6PDcODRAYKDlFOA4RFh0zV1A+EhYlOkRtZ00YIzdAUWhxXDFATldneXhlSFxfYnBkZ2MTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTExMTEhIVGRoaGhoSFBYaGhoaGhUWGRoaGhoaGRoaGhoaGhoaGhoaGhoaGhoaGhoaGhoaGhoaGhoaGhoaGhoaGhoaGhESFh8kJCQkEhQYIiQkJCQWGCEkJCQkJB8iJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQkJCQREhgvY2NjYxIVGkJjY2NjGBo4Y2NjY2MvQmNjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjY2NjFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRUVFRISEhUXGBkbEhIVFxgZGxwSFRcYGRscHRUXGBkbHB0dFxgZGxwdHR0YGRscHR0dHhkbHB0dHR4eGxwdHR0eHh4REREUFxocIBERFBcaHCAiERQXGhwgIiUUFxocICIlJRcaHCAiJSUlGhwgIiUlJSkcICIlJSUpKiAiJSUlKSoqEBAQFBgcICgQEBQYHCAoMBAUGBwgKDBAFBgcICgwQEAYHCAoMEBAQBwgKDBAQEBgICgwQEBAYIAoMEBAQGCAgAfF5cdH1e3Ow/L66wGmYnfIUbwdUTe3LMRbqON8B+5RJEvcGxkvrVUjTMrsXYhAnIwe0dTJfOYbWrDYyqUrz7dw/JO4hpmV2LsQQvkUeGq1BsZLx+cu5iV0e0eScJ91VIQYrmqfdVSK7GgjOU0oPaPOu5IcDK1mNvnD+K8LwS87f8Jx2mHtHnUkTGAurWZlNQa74ZLSFH9oF6FPGxzLsjQO5Qe0edcpttd7BXBSqMCL4k/4tFrHIPuEQ7m1/uIWkbDMWVoDdOSuRQ9286kvVUlQjzOE6VrNguN4oRXYGkgcnih7t13/9kxvLYKQezwLTrO44sVmMPgMqORo1E0sm1/9SludkcWHwfJwTSybR4LeAz6ugWVgRaY8mV/9SluQmtHrzsBtRF/wPY+X0JuYTs+ltgrXAmlk10xQHmTu9VSIAk1+vcvU4ml2oNzrNhEtQ3CysNP8UeR35wqpKUBdGdZMSjX4WVi8nJpdpHnbhzEIdx7mwf6W1FKAiucMXrWUWVjyRf23chNtR9mIzDoT/6ZLYailAjhFlZuvPtSeZ+2oREubDoWmT3TguY+JHPdRVSLKxfKH3vgNqJ/9emeEYikGXDFNzaLjvTeGAL61mogOoeG3y6oU4rW55ydoj0lUTSR/mmRhPmF86uwIfzp3FtiufQCmppaHDlGE0r2iTzXIw3zBq5hvaTldjG4CPb9wdxAme0SyedVKczJ9AtYbgPOzYKJvZZImsN7ecrxWZg5dR6ZLj/j4qpWsIA+vYwE+Tca9ounMIsrXMB4Stiib2SPQtZv+FVIpfEbzv8ncZoLBXc3YBqTG1HsskTTotZOYTG+oVUjLk6zhP8bg4RhMUNtfZdO7FdpBuXzhJ5Fh8IKlJG7wtD9ik8rWOJxy6iQ3NwzBpQ219mlyv+FLicYs2iJGSE0u2txzed++D61ZWCiHD/cZdQVCqkO2gJpdpNaObhnDfAPrT89RxdWFZ5hO3MseBSIlANppdZNIV/Rwe5eLTDvkfWKzFnH+QJ7m9QWV1KdwnuIwTNtZdJMoXBf74OhRnh2t+OTGL+AVUnIkyYY+QG7g9itHXyF3OIygG2s2kud679ZWKqSFa9n3IHD6MeLv1lZ0XyduRhiDRtrNnKoyiFVLcBm0ba5Yy3fQkDh4XsFE34isVpOzpa9nR8iCpS4HoxG2rJpnRhf3YboVa1PcRouh5LIJv/uQcPNd095ickTaiGBnWLKVWRc0OnYTSyex/n2FofEPnDG8y3PztHrzOLK1xo6RAml2k9owKajOC0Wr4D5x+3nA0UEhK2m198wuBHF3zlWWVKWLN1CHzLClUfuoYBcx4b1llpeBKmbayaR58njtE9onD66lUcsg0Spm2snsb+8HaJRn4dYcLbCuBuYwziB8/5U1C1DOOz2gZjSZtrLJk6vrLF3hwY4Io9xuT/ruUFRSBkNtUzTOWhjh26irLEPx4jPZL3Fo3QrReoGTTM21xYTT9oFdhTUIvjqTkfkvt0bzgVUjq/hOYY8j60IaO/0AzRBtqkTS6R5ellZd5uKdzzhb8BFlDdAcrwkE0rbXTOPB+7Y0FlZO96qFL4Ykg21StJs8qIW7h16H5hGiv8V2Cflau7QVDepTAHa6Lgt6feiEvJDM21StJsmOH/hynURrKxvUpQ8BH0JF7BiyG2qZpnL/7AOU66gt+reLEXY8pVOCQvSsBtqZTNM8bk9ohRcwD18o/WVkbvrceVKRb9I59IEKysjBeTMmmbA21xu/6iHadLRxuIzkLpi8wZYmmbbWi32RVAUjruxWlJ//iFxE38FI9hNKOoCdhwf5fDe4xZ81lgREhK2m1j78vW1CqkuMu/AjBNK210kzRUX/B+69cMMUG5bYrIeZxVSEZISmkzbXOi9yxwIfPgdsov7R71xuJ7rFcACjG/9PzApqFq7wEgzNJm2suWESPuwrQvejj7cbnQxMkxpm21lUYJL0fKmogPPqywn7e3FvB/FCNxPJ85iVUkCE9/tLKx31G4CgNtWTTPFhMvlu8G4/TrgaZttTChljfNJGgOT2X6EqpETy2tYd9cCBI4lIXJ1/3uVUllZEJz4baqGF64yxaZ+zPLYwde8Uqn1oKANtUrSaTOPHkhvuQP3bBlEJ/LFe4pqQOHUI8T8q7AXx3fLVBgSCVpMba55YxN3rv8U1Dv51bAPSOLlZWebkL8vSMGI21lJmmeVxPRwFlZF1CpqCN8uLwymaZyjbXHCRytogPN3o/n74CNykfT+qqRv5AQlHcRxYrC5KvGmbbUwmZY/29BvF6C1/93x4WVglXDLFpmbapmF89HKTogRwqqSlGbu+oiAkcWFbklC6Zhf+NtTLFpn8oWz+HsNRVSgIxZWON+yVyJlE5tq/+GWLTMutYX9ekTySEQPLVNQQ3OfycwJBM0zNtZcse7CvcKI0V/zh16Dr9OSA21MpmmcrHC+6pTAPHPwoit3LHHqs7jhFNRD6W8+EBGoSEoaZttTCZljfduH/fFisn+dRBGAZYtMzbVMwvul/T/crK1NQh8gN0SRRa9cOux6clC0/mDLFpmbarmF8/e6CopeOLCNW6S/IUUg3jJIYiAcDoMcGeRbOvuTPjXR/tyo79LK3kqqkbxkkMRAOB0GODPItnX3Jnxro/25Ud+llbyVVSN4ySGIgHA6DHBnkWzr7kz410f7cqO/Syt5KqpFVJwn6gBEvBM0zNtZcpGOEPiysW8vvRd2R0f7gtjhqUvXL+gWVwHm4XJDBiMpmmZtrLfPwd/IugP5+fKVSysH1EXreFAcEhelGmbbUmZY4Xdo1vQWVnK19P4RuEnbf0gQnR+lDCZlivNM22t1ESmopPIgfT0duOfQrsjgG4tPxli0zJmF5trdL1JDUIUT1ZXSqQDeR4B8mX3TrRro/2McGeUvLtwo6jIEKMkCUXWsLyZROd9P/rFYNtXPBli0z398iVUlVKAjFlY437JXImUTm2r/4ZYtMy61hf16RPJIU9nZ1MABAwAAAAAAAAAZpwgEwIAAABhp658BScAAAAAAADnUFBQXIDGXLhwtttNHDhw5OcpQRMETBEwRPduylKVB0HRdF0A";else if(Modernizr.video.h264)n.src="data:video/mp4;base64,AAAAIGZ0eXBpc29tAAACAGlzb21pc28yYXZjMW1wNDEAAAAIZnJlZQAAAs1tZGF0AAACrgYF//+q3EXpvebZSLeWLNgg2SPu73gyNjQgLSBjb3JlIDE0OCByMjYwMSBhMGNkN2QzIC0gSC4yNjQvTVBFRy00IEFWQyBjb2RlYyAtIENvcHlsZWZ0IDIwMDMtMjAxNSAtIGh0dHA6Ly93d3cudmlkZW9sYW4ub3JnL3gyNjQuaHRtbCAtIG9wdGlvbnM6IGNhYmFjPTEgcmVmPTMgZGVibG9jaz0xOjA6MCBhbmFseXNlPTB4MzoweDExMyBtZT1oZXggc3VibWU9NyBwc3k9MSBwc3lfcmQ9MS4wMDowLjAwIG1peGVkX3JlZj0xIG1lX3JhbmdlPTE2IGNocm9tYV9tZT0xIHRyZWxsaXM9MSA4eDhkY3Q9MSBjcW09MCBkZWFkem9uZT0yMSwxMSBmYXN0X3Bza2lwPTEgY2hyb21hX3FwX29mZnNldD0tMiB0aHJlYWRzPTEgbG9va2FoZWFkX3RocmVhZHM9MSBzbGljZWRfdGhyZWFkcz0wIG5yPTAgZGVjaW1hdGU9MSBpbnRlcmxhY2VkPTAgYmx1cmF5X2NvbXBhdD0wIGNvbnN0cmFpbmVkX2ludHJhPTAgYmZyYW1lcz0zIGJfcHlyYW1pZD0yIGJfYWRhcHQ9MSBiX2JpYXM9MCBkaXJlY3Q9MSB3ZWlnaHRiPTEgb3Blbl9nb3A9MCB3ZWlnaHRwPTIga2V5aW50PTI1MCBrZXlpbnRfbWluPTEwIHNjZW5lY3V0PTQwIGludHJhX3JlZnJlc2g9MCByY19sb29rYWhlYWQ9NDAgcmM9Y3JmIG1idHJlZT0xIGNyZj0yMy4wIHFjb21wPTAuNjAgcXBtaW49MCBxcG1heD02OSBxcHN0ZXA9NCBpcF9yYXRpbz0xLjQwIGFxPTE6MS4wMACAAAAAD2WIhAA3//728P4FNjuZQQAAAu5tb292AAAAbG12aGQAAAAAAAAAAAAAAAAAAAPoAAAAZAABAAABAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAACGHRyYWsAAABcdGtoZAAAAAMAAAAAAAAAAAAAAAEAAAAAAAAAZAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAEAAAAAAAAAAAAAAAAAAEAAAAAAAgAAAAIAAAAAACRlZHRzAAAAHGVsc3QAAAAAAAAAAQAAAGQAAAAAAAEAAAAAAZBtZGlhAAAAIG1kaGQAAAAAAAAAAAAAAAAAACgAAAAEAFXEAAAAAAAtaGRscgAAAAAAAAAAdmlkZQAAAAAAAAAAAAAAAFZpZGVvSGFuZGxlcgAAAAE7bWluZgAAABR2bWhkAAAAAQAAAAAAAAAAAAAAJGRpbmYAAAAcZHJlZgAAAAAAAAABAAAADHVybCAAAAABAAAA+3N0YmwAAACXc3RzZAAAAAAAAAABAAAAh2F2YzEAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAgACAEgAAABIAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAY//8AAAAxYXZjQwFkAAr/4QAYZ2QACqzZX4iIhAAAAwAEAAADAFA8SJZYAQAGaOvjyyLAAAAAGHN0dHMAAAAAAAAAAQAAAAEAAAQAAAAAHHN0c2MAAAAAAAAAAQAAAAEAAAABAAAAAQAAABRzdHN6AAAAAAAAAsUAAAABAAAAFHN0Y28AAAAAAAAAAQAAADAAAABidWR0YQAAAFptZXRhAAAAAAAAACFoZGxyAAAAAAAAAABtZGlyYXBwbAAAAAAAAAAAAAAAAC1pbHN0AAAAJal0b28AAAAdZGF0YQAAAAEAAAAATGF2ZjU2LjQwLjEwMQ==";else{addTest("videoautoplay",!1);return}}catch{addTest("videoautoplay",!1);return}n.setAttribute("autoplay",""),o.cssText="display:none",docElement.appendChild(n),setTimeout(function(){n.addEventListener("playing",a,!1),e=setTimeout(a,A)},0)});Modernizr.addTest("videocrossorigin","crossOrigin"in createElement("video"));Modernizr.addTest("videoloop","loop"in createElement("video"));Modernizr.addTest("videopreload","preload"in createElement("video"));Modernizr.addTest("vml",function(){var e=createElement("div"),A=!1,t;return isSVG||(e.innerHTML='<v:shape id="vml_flag1" adj="1" />',t=e.firstChild,"style"in t&&(t.style.behavior="url(#default#VML)"),A=t?typeof t.adj=="object":!0),A});Modernizr.addTest("webintents",!!prefixed("startActivity",navigator));Modernizr.addTest("webanimations","animate"in createElement("div"));Modernizr.addTest("publicKeyCredential",function(){return!!window.PublicKeyCredential});Modernizr.addTest("webgl",function(){return"WebGLRenderingContext"in window});Modernizr.addAsyncTest(function(){if(Modernizr.webglextensions=!1,!!Modernizr.webgl){var e,A,t;try{e=createElement("canvas"),A=e.getContext("webgl")||e.getContext("experimental-webgl"),t=A.getSupportedExtensions()}catch{return}A!==undefined&&(Modernizr.webglextensions=new Boolean(!0));for(var r=-1,n=t.length;++r<n;)Modernizr.webglextensions[t[r]]=!0;e=undefined}});Modernizr.addTest("peerconnection",!!prefixed("RTCPeerConnection",window));Modernizr.addTest("datachannel",function(){if(!Modernizr.peerconnection)return!1;for(var e=0,A=domPrefixesAll.length;e<A;e++){var t=window[domPrefixesAll[e]+"RTCPeerConnection"];if(t)try{var r=new t({});return"createDataChannel"in r}catch{}}return!1});Modernizr.addTest("getUserMedia","mediaDevices"in navigator&&"getUserMedia"in navigator.mediaDevices);Modernizr.addTest("mediastream",typeof MediaRecorder<"u");var supports=!1;try{supports="WebSocket"in window&&window.WebSocket.CLOSING===2}catch(e){}Modernizr.addTest("websockets",supports);Modernizr.addTest("websocketsbinary",function(){var e=location.protocol==="https:"?"wss":"ws",A;if("WebSocket"in window){if(A="binaryType"in WebSocket.prototype,A)return A;try{return!!new WebSocket(e+"://.").binaryType}catch{}}return!1});Modernizr.addTest("atobbtoa","atob"in window&&"btoa"in window,{aliases:["atob-btoa"]});Modernizr.addTest("framed",window.location!==top.location);Modernizr.addTest("matchmedia",!!prefixed("matchMedia",window));Modernizr.addTest("pushmanager","PushManager"in window);Modernizr.addTest("resizeobserver","ResizeObserver"in window);Modernizr.addTest("workertypeoption",function(){if("Worker"in window){var e=!1,A={get type(){return e=!0,"module"}},t="var message='hello'",r=new Blob([t],{type:"text/javascript"}),n=URL.createObjectURL(r);try{return new Worker(n,A).terminate(),e}catch{return!1}}else return!1});Modernizr.addAsyncTest(function(){try{var e=window.BlobBuilder,A=window.URL;Modernizr._config.usePrefix&&(e=e||window.MozBlobBuilder||window.WebKitBlobBuilder||window.MSBlobBuilder||window.OBlobBuilder,A=A||window.MozURL||window.webkitURL||window.MSURL||window.OURL);var t="Modernizr",r,n,o,a,l,s="this.onmessage=function(e){postMessage(e.data)}";try{r=new Blob([s],{type:"text/javascript"})}catch{}r||(n=new e,n.append(s),r=n.getBlob()),a=A.createObjectURL(r),o=new Worker(a),o.onmessage=function(m){addTest("blobworkers",t===m.data),u()},o.onerror=f,l=setTimeout(f,200),o.postMessage(t)}catch{f()}function f(){addTest("blobworkers",!1),u()}function u(){a&&A.revokeObjectURL(a),o&&o.terminate(),l&&clearTimeout(l)}});Modernizr.addAsyncTest(function(){try{var e="Modernizr",A=new Worker("data:text/javascript;base64,dGhpcy5vbm1lc3NhZ2U9ZnVuY3Rpb24oZSl7cG9zdE1lc3NhZ2UoZS5kYXRhKX0=");A.onmessage=function(t){A.terminate(),addTest("dataworkers",e===t.data),A=null},A.onerror=function(){addTest("dataworkers",!1),A=null},setTimeout(function(){addTest("dataworkers",!1)},200),A.postMessage(e)}catch{setTimeout(function(){addTest("dataworkers",!1)},0)}});Modernizr.addTest("sharedworkers","SharedWorker"in window);Modernizr.addTest("webworkers","Worker"in window);Modernizr.addAsyncTest(function(){var e=!!(Modernizr.blobconstructor&&Modernizr.bloburls&&Modernizr.webworkers&&Modernizr.typedarrays);if(!e)return addTest("transferables",!1);try{var A,t='var hello = "world"',r=new Blob([t],{type:"text/javascript"}),n=URL.createObjectURL(r),o=new Worker(n),a;o.onerror=l,a=setTimeout(l,200),A=new ArrayBuffer(1),o.postMessage(A,[A]),addTest("transferables",A.byteLength===0),s()}catch{l()}function l(){addTest("transferables",!1),s()}function s(){n&&URL.revokeObjectURL(n),o&&o.terminate(),a&&clearTimeout(a)}});Modernizr.addTest("xdomainrequest","XDomainRequest"in window),testRunner(),setClasses(classes),delete ModernizrProto.addTest,delete ModernizrProto.addAsyncTest;for(var i=0;i<Modernizr._q.length;i++)Modernizr._q[i]();scriptGlobalObject.Modernizr=Modernizr})(window,window,document)});Y();})();
// [ts] /*!
// [ts] * modernizr v4.0.0-alpha
// [ts] * Build https://modernizr.com/download?-adownload-ambientlight-aping-apng-appearance-applicationcache-areaping-arrow-aspectratio-atobbtoa-audio-audioautoplay-audioloop-audiopreload-avif-backdropfilter-backgroundblendmode-backgroundcliptext-backgroundsize-batteryapi-bdi-beacon-bgpositionshorthand-bgpositionxy-bgrepeatspace_bgrepeatround-bgsizecover-blobconstructor-bloburls-blobworkers-borderimage-borderradius-boxdecorationbreak-boxshadow-boxsizing-broadcastchannel-canvas-canvasblending-canvastext-canvaswinding-capture-checked-classlist-clipboard-connectioneffectivetype-contenteditable-contextmenu-cookies-cors-createelementattrs_createelement_attrs-cryptography-cssall-cssanimations-csscalc-csschunit-csscolumns-cssescape-cssexunit-cssfilters-cssgradients-cssgrid_cssgridlegacy-csshyphens_softhyphens_softhyphensfind-cssinvalid-cssmask-csspointerevents-csspositionsticky-csspseudoanimations-csspseudotransitions-cssreflections-cssremunit-cssresize-cssscrollbar-csstransforms-csstransforms3d-csstransformslevel2-csstransitions-cssvalid-cssvhunit-cssvmaxunit-cssvminunit-cssvwunit-cubicbezierrange-customelements-customevent-customproperties-customprotocolhandler-dart-datachannel-dataset-datauri-dataview-dataworkers-details-devicemotion_deviceorientation-directory-display_runin-displaytable-documentfragment-ellipsis-emoji-es5-es5array-es5date-es5function-es5object-es5string-es5syntax-es5undefined-es6array-es6class-es6collections-es6math-es6number-es6object-es6string-es6symbol-es7array-es8object-eventlistener-eventsource-exiforientation-fetch-fileinput-filereader-filesystem-flash-flexbox-flexboxlegacy-flexboxtweener-flexgap-flexwrap-focusvisible-focuswithin-fontdisplay-fontface-forcetouch-formattribute-formvalidation-framed-fullscreen-gamepads-generatedcontent-generators-geolocation-getrandomvalues-getusermedia-hairline-hashchange-hidden-hiddenscroll-history-hovermq-hsla-htmlimports-ie8compat-imgcrossorigin-indexeddb-indexeddb2-indexeddbblob-inlinesvg-input-inputformaction-inputformenctype-inputformmethod-inputformnovalidate-inputformtarget-inputsearchevent-inputtypes-intersectionobserver-intl-jpeg2000-jpegxr-json-lastchild-lazyloading-ligatures-localizednumber-localstorage-lowbandwidth-lowbattery-matchmedia-mathml-mediaqueries-mediarecorder-mediasource-messagechannel-microdata-multiplebgs-mutationobserver-notification-nthchild-objectfit-olreversed-oninput-opacity-outputelem-overflowscrolling-pagevisibility-passiveeventlisteners-peerconnection-performance-picture-placeholder-pointerevents-pointerlock-pointermq-postmessage-prefetch-preserve3d-progressbar_meter-promises-proximity-proxy-publickeycredential-pushmanager-queryselector-quotamanagement-regions-requestanimationframe-requestautocomplete-resizeobserver-restdestructuringarray_restdestructuringobject-restparameters-rgba-ruby-sandbox-scriptasync-scriptdefer-scrollsnappoints-scrolltooptions-seamless-serviceworker-sessionstorage-shadowroot-shadowrootlegacy-shapes-sharedworkers-siblinggeneral-sizes-smil-speechrecognition-speechsynthesis-spreadarray-spreadobject-srcdoc-srcset-strictmode-stringtemplate-stylescoped-subpixelfont-supports-svg-svgasimg-svgclippaths-svgfilters-svgforeignobject-target-template-textalignlast-textareamaxlength-textdecoration-textencoder_textdecoder-textshadow-texttrackapi_track-time-todataurljpeg_todataurlpng_todataurlwebp-transferables-typedarrays-unicoderange-unknownelements-urlparser-urlsearchparams-userdata-userselect-variablefonts-vibrate-video-videoautoplay-videocrossorigin-videoloop-videopreload-vml-webanimations-webaudio-webgl-webglextensions-webintents-webp-webpalpha-webpanimation-webplossless_webp_lossless-websockets-websocketsbinary-websqldatabase-webworkers-willchange-workertypeoption-wrapflow-xdomainrequest-xhr2-xhrresponsetype-xhrresponsetypearraybuffer-xhrresponsetypeblob-xhrresponsetypedocument-xhrresponsetypejson-xhrresponsetypetext-addtest-atrule-domprefixes-hasevent-load-mq-prefixed-prefixedcss-prefixes-printshiv-setclasses-testallprops-testprop-teststyles-dontmin
// [ts] *
// [ts] * Copyright (c)
// [ts] *  Faruk Ates
// [ts] *  Paul Irish
// [ts] *  Alex Sexton
// [ts] *  Ryan Seddon
// [ts] *  Patrick Kettner
// [ts] *  Stu Cox
// [ts] *  Richard Herrera
// [ts] *  Veeck
// [ts] * MIT License
// [ts] */
// [ts] /**
// [ts] * @preserve HTML5 Shiv 3.7.3 | @afarkas @jdalton @jon_neal @rem | MIT/GPL2 Licensed
// [ts] */
// [ts] /*!
// [ts] {
// [ts] "name": "a[download] Attribute",
// [ts] "property": "adownload",
// [ts] "caniuse": "download",
// [ts] "tags": ["media", "attribute"],
// [ts] "builderAliases": ["a_download"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://developers.whatwg.org/links.html#downloading-resources"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "a[ping] Attribute",
// [ts] "property": "aping",
// [ts] "caniuse": "ping",
// [ts] "tags": ["media", "attribute"],
// [ts] "builderAliases": ["a_ping"],
// [ts] "authors": ["Hélio Correia (@heliocorreia)"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/dev/links.html#ping"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "area[ping] Attribute",
// [ts] "property": "areaping",
// [ts] "caniuse": "ping",
// [ts] "tags": ["media", "attribute"],
// [ts] "builderAliases": ["area_ping"],
// [ts] "authors": ["Hélio Correia (@heliocorreia)"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/dev/links.html#ping"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Ambient Light Events",
// [ts] "property": "ambientlight",
// [ts] "caniuse": "ambient-light",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/ambient-light/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Application Cache",
// [ts] "property": "applicationcache",
// [ts] "caniuse": "offline-apps",
// [ts] "tags": ["storage", "offline"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/docs/HTML/Using_the_application_cache"
// [ts] }],
// [ts] "polyfills": ["html5gears"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "HTML5 Audio Element",
// [ts] "property": "audio",
// [ts] "caniuse": "audio",
// [ts] "tags": ["html5", "audio", "media"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/En/Media_formats_supported_by_the_audio_and_video_elements"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Audio Autoplay",
// [ts] "property": "audioautoplay",
// [ts] "authors": ["Jordy van Dortmont"],
// [ts] "tags": ["audio"],
// [ts] "async": true
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Audio Loop Attribute",
// [ts] "property": "audioloop",
// [ts] "tags": ["audio", "media"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Audio Preload",
// [ts] "property": "audiopreload",
// [ts] "tags": ["audio", "media"],
// [ts] "async": true,
// [ts] "warnings": ["This test is very large – only include it if you absolutely need it"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Web Audio API",
// [ts] "property": "webaudio",
// [ts] "caniuse": "audio-api",
// [ts] "polyfills": ["dynamicaudiojs", "audiolibjs"],
// [ts] "tags": ["audio", "media"],
// [ts] "builderAliases": ["audio_webaudio_api"],
// [ts] "authors": ["Addy Osmani"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://webaudio.github.io/web-audio-api/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Battery API",
// [ts] "property": "batteryapi",
// [ts] "aliases": ["battery-api"],
// [ts] "builderAliases": ["battery_api"],
// [ts] "tags": ["battery", "device", "media"],
// [ts] "authors": ["Paul Sayre", "Alex Bradley (@abrad1212)"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/DOM/window.navigator.mozBattery"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Low Battery Level",
// [ts] "property": "lowbattery",
// [ts] "tags": ["battery", "hardware", "mobile"],
// [ts] "builderAliases": ["battery_level"],
// [ts] "authors": ["Paul Sayre"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Navigator/battery"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Blob constructor",
// [ts] "property": "blobconstructor",
// [ts] "aliases": ["blob-constructor"],
// [ts] "builderAliases": ["blob_constructor"],
// [ts] "caniuse": "blobbuilder",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/FileAPI/#constructorBlob"
// [ts] }],
// [ts] "polyfills": ["blobjs"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Broadcast Channel",
// [ts] "property": "broadcastchannel",
// [ts] "authors": ["Alex Neises (@AlexNeises)"],
// [ts] "caniuse": "broadcastchannel",
// [ts] "tags": ["performance", "broadcastchannel"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/BroadcastChannel"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Canvas",
// [ts] "property": "canvas",
// [ts] "caniuse": "canvas",
// [ts] "tags": ["canvas", "graphics"],
// [ts] "polyfills": ["excanvas", "slcanvas"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "canvas blending support",
// [ts] "property": "canvasblending",
// [ts] "caniuse": "canvas-blending",
// [ts] "tags": ["canvas"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://drafts.fxtf.org/compositing-1/"
// [ts] }, {
// [ts] "name": "Article",
// [ts] "href": "https://web.archive.org/web/20171003232921/http://blogs.adobe.com/webplatform/2013/01/28/blending-features-in-canvas/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "canvas.toDataURL type support",
// [ts] "property": ["todataurljpeg", "todataurlpng", "todataurlwebp"],
// [ts] "tags": ["canvas"],
// [ts] "builderAliases": ["canvas_todataurl_type"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/HTMLCanvasElement.toDataURL"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "canvas winding support",
// [ts] "property": "canvaswinding",
// [ts] "tags": ["canvas"],
// [ts] "notes": [{
// [ts] "name": "Article",
// [ts] "href": "https://web.archive.org/web/20170825024655/http://blogs.adobe.com/webplatform/2013/01/30/winding-rules-in-canvas/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Canvas text",
// [ts] "property": "canvastext",
// [ts] "caniuse": "canvas-text",
// [ts] "tags": ["canvas", "graphics"],
// [ts] "polyfills": ["canvastext"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Clipboard API",
// [ts] "property": "clipboard",
// [ts] "tags": ["clipboard"],
// [ts] "authors": ["Markel Ferro (@MarkelFe)"],
// [ts] "async": true,
// [ts] "warnings": ["It may return false in non-HTTPS connections as the API is only available in secure contexts"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs Clipboard Object",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Clipboard"
// [ts] }, {
// [ts] "name": "MDN Docs Clipboard API",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Clipboard_API"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Content Editable",
// [ts] "property": "contenteditable",
// [ts] "caniuse": "contenteditable",
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/interaction.html#contenteditable"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Context menus",
// [ts] "property": "contextmenu",
// [ts] "caniuse": "menu",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/html5/interactive-elements.html#context-menus"
// [ts] }, {
// [ts] "name": "thewebrocks.com Demo",
// [ts] "href": "http://thewebrocks.com/demos/context-menu/"
// [ts] }],
// [ts] "polyfills": ["jquery-contextmenu"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Cross-Origin Resource Sharing",
// [ts] "property": "cors",
// [ts] "caniuse": "cors",
// [ts] "authors": ["Theodoor van Donge"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/HTTP/Access_control_CORS"
// [ts] }],
// [ts] "polyfills": ["pmxdr", "ppx", "flxhr"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Web Cryptography",
// [ts] "property": "cryptography",
// [ts] "caniuse": "cryptography",
// [ts] "tags": ["crypto"],
// [ts] "authors": ["roblarsen"],
// [ts] "notes": [{
// [ts] "name": "W3C Editor's Draft Spec",
// [ts] "href": "https://www.w3.org/TR/WebCryptoAPI/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "getRandomValues",
// [ts] "property": "getrandomvalues",
// [ts] "caniuse": "getrandomvalues",
// [ts] "tags": ["crypto"],
// [ts] "authors": ["komachi"],
// [ts] "notes": [{
// [ts] "name": "W3C Editor’s Draft Spec",
// [ts] "href": "https://w3c.github.io/webcrypto/#Crypto-interface-methods"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "cssall",
// [ts] "property": "cssall",
// [ts] "notes": [{
// [ts] "name": "Spec",
// [ts] "href": "https://drafts.csswg.org/css-cascade/#all-shorthand"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Animations",
// [ts] "property": "cssanimations",
// [ts] "caniuse": "css-animation",
// [ts] "polyfills": ["transformie", "csssandpaper"],
// [ts] "tags": ["css"],
// [ts] "warnings": ["Android < 4 will pass this test, but can only animate a single property at a time"],
// [ts] "notes": [{
// [ts] "name": "Article: 'Dispelling the Android CSS animation myths'",
// [ts] "href": "https://web.archive.org/web/20180602074607/https://daneden.me/2011/12/14/putting-up-with-androids-bullshit/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Appearance",
// [ts] "property": "appearance",
// [ts] "caniuse": "css-appearance",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/-moz-appearance"
// [ts] }, {
// [ts] "name": "CSS-Tricks CSS Almanac: appearance",
// [ts] "href": "https://css-tricks.com/almanac/properties/a/appearance/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "aspectratio css property",
// [ts] "property": "aspectratio",
// [ts] "tags": ["css aspectratio", "aspect-ratio"],
// [ts] "builderAliases": ["aspectratio"],
// [ts] "caniuse":"mdn-css_properties_aspect-ratio",
// [ts] "authors": ["Debadutta Panda"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/aspect-ratio"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Backdrop Filter",
// [ts] "property": "backdropfilter",
// [ts] "authors": ["Brian Seward"],
// [ts] "tags": ["css"],
// [ts] "caniuse": "css-backdrop-filter",
// [ts] "notes": [{
// [ts] "name": "W3C Editor’s Draft Spec",
// [ts] "href": "https://drafts.fxtf.org/filters-2/#BackdropFilterProperty"
// [ts] }, {
// [ts] "name": "WebKit Blog introduction + Demo",
// [ts] "href": "https://www.webkit.org/blog/3632/introducing-backdrop-filters/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Background Blend Mode",
// [ts] "property": "backgroundblendmode",
// [ts] "caniuse": "css-backgroundblendmode",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "CSS Blend Modes could be the next big thing in Web Design",
// [ts] "href": "https://medium.com/@bennettfeely/css-blend-modes-could-be-the-next-big-thing-in-web-design-6b51bf53743a"
// [ts] }, {
// [ts] "name": "Demo",
// [ts] "href": "https://bennettfeely.com/gradients/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Background Clip Text",
// [ts] "property": "backgroundcliptext",
// [ts] "authors": ["ausi"],
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "CSS Tricks Article",
// [ts] "href": "https://css-tricks.com/image-under-text/"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/background-clip"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/199"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Background Position Shorthand",
// [ts] "property": "bgpositionshorthand",
// [ts] "caniuse": "css-background-offsets",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_backgroundposition_shorthand"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/CSS/background-position"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-background/#background-position"
// [ts] }, {
// [ts] "name": "Demo",
// [ts] "href": "https://jsfiddle.net/Blink/bBXvt/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Background Position XY",
// [ts] "property": "bgpositionxy",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_backgroundposition_xy"],
// [ts] "authors": ["Allan Lei", "Brandom Aaron"],
// [ts] "notes": [{
// [ts] "name": "Demo",
// [ts] "href": "https://jsfiddle.net/allanlei/R8AYS/"
// [ts] }, {
// [ts] "name": "Adapted From",
// [ts] "href": "https://github.com/brandonaaron/jquery-cssHooks/blob/master/bgpos.js"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Background Repeat",
// [ts] "property": ["bgrepeatspace", "bgrepeatround"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_backgroundrepeat"],
// [ts] "authors": ["Ryan Seddon"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/background-repeat"
// [ts] }, {
// [ts] "name": "Test Page",
// [ts] "href": "https://jsbin.com/uzesun/"
// [ts] }, {
// [ts] "name": "Demo",
// [ts] "href": "https://jsfiddle.net/ryanseddon/yMLTQ/6/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Background Size",
// [ts] "property": "backgroundsize",
// [ts] "tags": ["css"],
// [ts] "knownBugs": ["This will false positive in Opera Mini - https://github.com/Modernizr/Modernizr/issues/396"],
// [ts] "notes": [{
// [ts] "name": "Related Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/396"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Background Size Cover",
// [ts] "property": "bgsizecover",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_backgroundsizecover"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/CSS/background-size"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Border Image",
// [ts] "property": "borderimage",
// [ts] "caniuse": "border-image",
// [ts] "polyfills": ["css3pie"],
// [ts] "knownBugs": ["Android < 2.0 is true, but has a broken implementation"],
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Border Radius",
// [ts] "property": "borderradius",
// [ts] "caniuse": "border-radius",
// [ts] "polyfills": ["css3pie"],
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "Comprehensive Compat Chart",
// [ts] "href": "https://muddledramblings.com/table-of-css3-border-radius-compliance"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Box Decoration Break",
// [ts] "property": "boxdecorationbreak",
// [ts] "caniuse": "css-boxdecorationbreak",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/box-decoration-break"
// [ts] }, {
// [ts] "name": "Demo",
// [ts] "href": "https://jsbin.com/xojoro/edit?css,output"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Box Shadow",
// [ts] "property": "boxshadow",
// [ts] "caniuse": "css-boxshadow",
// [ts] "tags": ["css"],
// [ts] "knownBugs": [
// [ts] "WebOS false positives on this test.",
// [ts] "The Kindle Silk browser false positives"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Box Sizing",
// [ts] "property": "boxsizing",
// [ts] "caniuse": "css3-boxsizing",
// [ts] "polyfills": ["borderboxmodel", "boxsizingpolyfill", "borderbox"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_boxsizing"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/box-sizing"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/248"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Calc",
// [ts] "property": "csscalc",
// [ts] "caniuse": "calc",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_calc"],
// [ts] "authors": ["@calvein"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :checked pseudo-selector",
// [ts] "caniuse": "css-sel3",
// [ts] "property": "checked",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/pull/879"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Font ch Units",
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "property": "csschunit",
// [ts] "caniuse": "ch-unit",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-values/#font-relative-lengths"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Columns",
// [ts] "property": "csscolumns",
// [ts] "caniuse": "multicolumn",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Grid (old & new)",
// [ts] "property": ["cssgrid", "cssgridlegacy"],
// [ts] "authors": ["Faruk Ates"],
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "The new, standardized CSS Grid",
// [ts] "href": "https://www.w3.org/TR/css3-grid-layout/"
// [ts] }, {
// [ts] "name": "The _old_ CSS Grid (legacy)",
// [ts] "href": "https://www.w3.org/TR/2011/WD-css3-grid-layout-20110407/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Cubic Bezier Range",
// [ts] "property": "cubicbezierrange",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_cubicbezierrange"],
// [ts] "authors": ["@calvein"],
// [ts] "warnings": ["In old versions (pre-2013) cubic-bezier values can't be > 1 due to Webkit [bug #45761](https://bugs.webkit.org/show_bug.cgi?id=45761)"],
// [ts] "notes": [{
// [ts] "name": "Comprehensive Compat Chart",
// [ts] "href": "https://muddledramblings.com/table-of-css3-border-radius-compliance/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Custom Properties",
// [ts] "property": "customproperties",
// [ts] "caniuse": "css-variables",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_customproperties"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/--*"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://drafts.csswg.org/css-variables/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Display run-in",
// [ts] "property": "display-runin",
// [ts] "authors": ["alanhogan"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_displayrunin"],
// [ts] "notes": [{
// [ts] "name": "CSS Tricks Article",
// [ts] "href": "https://web.archive.org/web/20111204150927/http://css-tricks.com:80/596-run-in/"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/198"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Display table",
// [ts] "property": "displaytable",
// [ts] "caniuse": "css-table",
// [ts] "authors": ["scottjehl"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_displaytable"],
// [ts] "notes": [{
// [ts] "name": "Detects for all additional table display values",
// [ts] "href": "https://pastebin.com/Gk9PeVaQ"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS text-overflow ellipsis",
// [ts] "property": "ellipsis",
// [ts] "caniuse": "text-overflow",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS.escape()",
// [ts] "property": "cssescape",
// [ts] "polyfills": ["css-escape"],
// [ts] "tags": ["css", "cssom"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Font ex Units",
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "property": "cssexunit",
// [ts] "caniuse": "mdn-css_types_length_ex",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-values/#font-relative-lengths"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Supports",
// [ts] "property": "supports",
// [ts] "caniuse": "css-featurequeries",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_supports"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec (The @supports rule)",
// [ts] "href": "https://dev.w3.org/csswg/css3-conditional/#at-supports"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/648"
// [ts] }, {
// [ts] "name": "W3C Spec (The CSSSupportsRule interface)",
// [ts] "href": "https://dev.w3.org/csswg/css3-conditional/#the-csssupportsrule-interface"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Filters",
// [ts] "property": "cssfilters",
// [ts] "caniuse": "css-filters",
// [ts] "polyfills": ["polyfilter"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_filters"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/filter"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Flexbox",
// [ts] "property": "flexbox",
// [ts] "caniuse": "flexbox",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "The _new_ flexbox",
// [ts] "href": "https://www.w3.org/TR/css-flexbox-1/"
// [ts] }],
// [ts] "warnings": [
// [ts] "A `true` result for this detect does not imply that the `flex-wrap` property is supported; see the `flexwrap` detect."
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Flexbox (legacy)",
// [ts] "property": "flexboxlegacy",
// [ts] "tags": ["css"],
// [ts] "polyfills": ["flexie"],
// [ts] "notes": [{
// [ts] "name": "The _old_ flexbox",
// [ts] "href": "https://www.w3.org/TR/2009/WD-css3-flexbox-20090723/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Flexbox (tweener)",
// [ts] "property": "flexboxtweener",
// [ts] "tags": ["css"],
// [ts] "polyfills": ["flexie"],
// [ts] "notes": [{
// [ts] "name": "The _inbetween_ flexbox",
// [ts] "href": "https://www.w3.org/TR/2011/WD-css3-flexbox-20111129/"
// [ts] }],
// [ts] "warnings": ["This represents an old syntax, not the latest standard syntax."]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Flex Gap",
// [ts] "property": "flexgap",
// [ts] "caniuse": "flexbox-gap",
// [ts] "tags": ["css", "flexbox"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css-align-3/#gaps"
// [ts] }],
// [ts] "authors": ["Chris Smith (@chris13524)"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Flex Line Wrapping",
// [ts] "property": "flexwrap",
// [ts] "tags": ["css", "flexbox"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css-flexbox-1/"
// [ts] }],
// [ts] "warnings": [
// [ts] "Does not imply a modern implementation – see documentation."
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :focus-visible pseudo-selector",
// [ts] "caniuse": "css-focus-visible",
// [ts] "property": "focusvisible",
// [ts] "authors": ["@esaborit4code"],
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :focus-within pseudo-selector",
// [ts] "caniuse": "css-focus-within",
// [ts] "property": "focuswithin",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Font Display",
// [ts] "property": "fontdisplay",
// [ts] "authors": ["Patrick Kettner"],
// [ts] "caniuse": "css-font-rendering-controls",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://drafts.csswg.org/css-fonts-4/#font-display-desc"
// [ts] }, {
// [ts] "name": "`font-display` for the masses",
// [ts] "href": "https://css-tricks.com/font-display-masses/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "@font-face",
// [ts] "property": "fontface",
// [ts] "authors": ["Diego Perini", "Mat Marquis"],
// [ts] "tags": ["css"],
// [ts] "knownBugs": [
// [ts] "False Positive: WebOS https://github.com/Modernizr/Modernizr/issues/342",
// [ts] "False Positive: WP7 https://github.com/Modernizr/Modernizr/issues/538"
// [ts] ],
// [ts] "notes": [{
// [ts] "name": "@font-face detection routine by Diego Perini",
// [ts] "href": "http://javascript.nwbox.com/CSSSupport/"
// [ts] }, {
// [ts] "name": "Filament Group @font-face compatibility research",
// [ts] "href": "https://docs.google.com/presentation/d/1n4NyG4uPRjAA8zn_pSQ_Ket0RhcWC6QlZ6LMjKeECo0/edit#slide=id.p"
// [ts] }, {
// [ts] "name": "Filament Grunticon/@font-face device testing results",
// [ts] "href": "https://docs.google.com/spreadsheet/ccc?key=0Ag5_yGvxpINRdHFYeUJPNnZMWUZKR2ItMEpRTXZPdUE#gid=0"
// [ts] }, {
// [ts] "name": "CSS fonts on Android",
// [ts] "href": "https://stackoverflow.com/questions/3200069/css-fonts-on-android"
// [ts] }, {
// [ts] "name": "@font-face and Android",
// [ts] "href": "http://archivist.incutio.com/viewlist/css-discuss/115960"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Generated Content",
// [ts] "property": "generatedcontent",
// [ts] "tags": ["css"],
// [ts] "warnings": ["Android may not return correct height for anything below 7px in old versions #738"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-selectors/#gen-content"
// [ts] }, {
// [ts] "name": "MDN Docs on :before",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/::before"
// [ts] }, {
// [ts] "name": "MDN Docs on :after",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/::after"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Gradients",
// [ts] "caniuse": "css-gradients",
// [ts] "property": "cssgradients",
// [ts] "tags": ["css"],
// [ts] "knownBugs": ["False-positives on webOS (https://github.com/Modernizr/Modernizr/issues/202)"],
// [ts] "notes": [{
// [ts] "name": "Webkit Gradient Syntax",
// [ts] "href": "https://webkit.org/blog/175/introducing-css-gradients/"
// [ts] }, {
// [ts] "name": "Linear Gradient Syntax",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/linear-gradient"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://drafts.csswg.org/css-images-3/#gradients"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*! {
// [ts] "name": "CSS Hairline",
// [ts] "property": "hairline",
// [ts] "tags": ["css"],
// [ts] "authors": ["strarsis"],
// [ts] "notes": [{
// [ts] "name": "Blog post about CSS retina hairlines",
// [ts] "href": "http://dieulot.net/css-retina-hairline"
// [ts] }, {
// [ts] "name": "Derived from",
// [ts] "href": "https://gist.github.com/dieulot/520a49463f6058fbc8d1"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS HSLA Colors",
// [ts] "caniuse": "css3-colors",
// [ts] "property": "hsla",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Hyphens",
// [ts] "caniuse": "css-hyphens",
// [ts] "property": ["csshyphens", "softhyphens", "softhyphensfind"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_hyphens"],
// [ts] "async": true,
// [ts] "authors": ["David Newton"],
// [ts] "warnings": [
// [ts] "These tests currently require document.body to be present",
// [ts] "If loading Hyphenator.js via yepnope, be cautious of issue 158: https://github.com/mnater/hyphenator/issues/158",
// [ts] "This is very large – only include it if you absolutely need it"
// [ts] ],
// [ts] "notes": [{
// [ts] "name": "The Current State of Hyphenation on the Web.",
// [ts] "href": "https://davidnewton.ca/the-current-state-of-hyphenation-on-the-web"
// [ts] }, {
// [ts] "name": "Hyphenation Test Page",
// [ts] "href": "https://web.archive.org/web/20150319125549/http://davidnewton.ca/demos/hyphenation/test.html"
// [ts] }, {
// [ts] "name": "Hyphenation is Language Specific",
// [ts] "href": "https://code.google.com/p/hyphenator/source/diff?spec=svn975&r=975&format=side&path=/trunk/Hyphenator.js#sc_svn975_313"
// [ts] }, {
// [ts] "name": "Related Modernizr Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/312"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :invalid pseudo-class",
// [ts] "property": "cssinvalid",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/:invalid"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :last-child pseudo-selector",
// [ts] "caniuse": "css-sel3",
// [ts] "property": "lastchild",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_lastchild"],
// [ts] "notes": [{
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/pull/304"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Mask",
// [ts] "caniuse": "css-masks",
// [ts] "property": "cssmask",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_mask"],
// [ts] "notes": [{
// [ts] "name": "Webkit blog on CSS Masks",
// [ts] "href": "https://webkit.org/blog/181/css-masks/"
// [ts] }, {
// [ts] "name": "Safari Docs",
// [ts] "href": "https://developer.apple.com/library/archive/documentation/InternetWeb/Conceptual/SafariVisualEffectsProgGuide/Masks/Masks.html"
// [ts] }, {
// [ts] "name": "CSS SVG mask",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/mask"
// [ts] }, {
// [ts] "name": "Combine with clippaths for awesomeness",
// [ts] "href": "https://web.archive.org/web/20150508193041/http://generic.cx:80/for/webkit/test.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Media Queries",
// [ts] "caniuse": "css-mediaqueries",
// [ts] "property": "mediaqueries",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_mediaqueries"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Multiple Backgrounds",
// [ts] "caniuse": "multibackgrounds",
// [ts] "property": "multiplebgs",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :nth-child pseudo-selector",
// [ts] "caniuse": "css-sel3",
// [ts] "property": "nthchild",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/pull/685"
// [ts] }, {
// [ts] "name": "Sitepoint :nth-child documentation",
// [ts] "href": "https://www.sitepoint.com/atoz-css-screencast-nth-child/"
// [ts] }],
// [ts] "authors": ["@emilchristensen"],
// [ts] "knownBugs": ["Known false negative in Safari 3.1 and Safari 3.2.2"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Object Fit",
// [ts] "caniuse": "object-fit",
// [ts] "property": "objectfit",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_objectfit"],
// [ts] "notes": [{
// [ts] "name": "Opera Article on Object Fit",
// [ts] "href": "https://dev.opera.com/articles/css3-object-fit-object-position/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Opacity",
// [ts] "caniuse": "css-opacity",
// [ts] "property": "opacity",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Overflow Scrolling",
// [ts] "property": "overflowscrolling",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_overflow_scrolling"],
// [ts] "notes": [{
// [ts] "name": "Article on iOS overflow scrolling",
// [ts] "href": "https://css-tricks.com/snippets/css/momentum-scrolling-on-ios-overflow-elements/"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/-webkit-overflow-scrolling"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Pointer Events",
// [ts] "caniuse": "pointer-events",
// [ts] "property": "csspointerevents",
// [ts] "authors": ["ausi"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_pointerevents"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/pointer-events"
// [ts] }, {
// [ts] "name": "Test Project Page",
// [ts] "href": "https://ausi.github.com/Feature-detection-technique-for-pointer-events/"
// [ts] }, {
// [ts] "name": "Test Project Wiki",
// [ts] "href": "https://github.com/ausi/Feature-detection-technique-for-pointer-events/wiki"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/80"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS position: sticky",
// [ts] "property": "csspositionsticky",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_positionsticky"],
// [ts] "notes": [{
// [ts] "name": "Chrome bug report",
// [ts] "href": "https://bugs.chromium.org/p/chromium/issues/detail?id=322972"
// [ts] }],
// [ts] "warnings": ["using position:sticky on anything but top aligned elements is buggy in Chrome < 37 and iOS <=7+"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Generated Content Animations",
// [ts] "property": "csspseudoanimations",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Transitions",
// [ts] "property": "csstransitions",
// [ts] "caniuse": "css-transitions",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Generated Content Transitions",
// [ts] "property": "csspseudotransitions",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Reflections",
// [ts] "caniuse": "css-reflections",
// [ts] "property": "cssreflections",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Regions",
// [ts] "caniuse": "css-regions",
// [ts] "authors": ["Mihai Balan"],
// [ts] "property": "regions",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_regions"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-regions/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Font rem Units",
// [ts] "caniuse": "rem",
// [ts] "authors": ["nsfmc"],
// [ts] "property": "cssremunit",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_remunit"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-values/#relative0"
// [ts] }, {
// [ts] "name": "Font Size with rem by Jonathan Snook",
// [ts] "href": "https://snook.ca/archives/html_and_css/font-size-with-rem"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS UI Resize",
// [ts] "property": "cssresize",
// [ts] "caniuse": "css-resize",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_resize"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-ui/#resize"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/CSS/resize"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS rgba",
// [ts] "caniuse": "css3-colors",
// [ts] "property": "rgba",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "CSSTricks Tutorial",
// [ts] "href": "https://css-tricks.com/rgba-browser-support/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Stylable Scrollbars",
// [ts] "property": "cssscrollbar",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_scrollbars"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Scroll Snap Points",
// [ts] "property": "scrollsnappoints",
// [ts] "caniuse": "css-snappoints",
// [ts] "notes": [{
// [ts] "name": "Setting native-like scrolling offsets in CSS with Scrolling Snap Points",
// [ts] "href": "http://generatedcontent.org/post/66817675443/setting-native-like-scrolling-offsets-in-css-with"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Scroll_Snap_Points"
// [ts] }],
// [ts] "polyfills": ["scrollsnap"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Shapes",
// [ts] "property": "shapes",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css-shapes"
// [ts] }, {
// [ts] "name": "Examples from Adobe",
// [ts] "href": "https://web.archive.org/web/20171230010236/http://webplatform.adobe.com:80/shapes"
// [ts] }, {
// [ts] "name": "Examples from CSS-Tricks",
// [ts] "href": "https://css-tricks.com/examples/ShapesOfCSS/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS general sibling selector",
// [ts] "caniuse": "css-sel3",
// [ts] "property": "siblinggeneral",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/pull/889"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Subpixel Fonts",
// [ts] "property": "subpixelfont",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_subpixelfont"],
// [ts] "authors": ["@derSchepp", "@gerritvanaaken", "@rodneyrehm", "@yatil", "@ryanseddon"],
// [ts] "notes": [{
// [ts] "name": "Origin Test",
// [ts] "href": "https://github.com/gerritvanaaken/subpixeldetect"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :target pseudo-class",
// [ts] "caniuse": "css-sel3",
// [ts] "property": "target",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/:target"
// [ts] }],
// [ts] "authors": ["@zachleat"],
// [ts] "warnings": ["Opera Mini supports :target but doesn't update the hash for anchor links."]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS text-align-last",
// [ts] "property": "textalignlast",
// [ts] "caniuse": "css-text-align-last",
// [ts] "tags": ["css"],
// [ts] "warnings": ["IE does not support the 'start' or 'end' values."],
// [ts] "notes": [{
// [ts] "name": "Quirksmode",
// [ts] "href": "https://www.quirksmode.org/css/text/textalignlast.html"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/text-align-last"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS textDecoration",
// [ts] "property": "textdecoration",
// [ts] "caniuse": "text-decoration",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css-text-decor-3/#line-decoration"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS textshadow",
// [ts] "property": "textshadow",
// [ts] "caniuse": "css-textshadow",
// [ts] "tags": ["css"],
// [ts] "knownBugs": ["FF3.0 will false positive on this test"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Transforms",
// [ts] "property": "csstransforms",
// [ts] "caniuse": "transforms2d",
// [ts] "tags": ["css"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Transforms 3D",
// [ts] "property": "csstransforms3d",
// [ts] "caniuse": "transforms3d",
// [ts] "tags": ["css"],
// [ts] "knownBugs": [
// [ts] "Chrome may occasionally fail this test on some systems; more info: https://bugs.chromium.org/p/chromium/issues/detail?id=129004, however, the issue has since been closed (marked as fixed)."
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Transforms Level 2",
// [ts] "property": "csstransformslevel2",
// [ts] "authors": ["rupl"],
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "CSSWG Draft Spec",
// [ts] "href": "https://drafts.csswg.org/css-transforms-2/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS Transform Style preserve-3d",
// [ts] "property": "preserve3d",
// [ts] "authors": ["denyskoch", "aFarkas"],
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/transform-style"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/1748"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS user-select",
// [ts] "property": "userselect",
// [ts] "caniuse": "user-select-none",
// [ts] "authors": ["ryan seddon"],
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_userselect"],
// [ts] "notes": [{
// [ts] "name": "Related Modernizr Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/250"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS :valid pseudo-class",
// [ts] "property": "cssvalid",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/CSS/:valid"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Variable Open Type Fonts",
// [ts] "property": "variablefonts",
// [ts] "authors": ["Patrick Kettner"],
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "Variable fonts on the web",
// [ts] "href": "https://webkit.org/blog/7051/variable-fonts-on-the-web/"
// [ts] }, {
// [ts] "name": "Variable fonts for responsive design",
// [ts] "href": "https://alistapart.com/blog/post/variable-fonts-for-responsive-design"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS vh unit",
// [ts] "property": "cssvhunit",
// [ts] "caniuse": "viewport-units",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_vhunit"],
// [ts] "notes": [{
// [ts] "name": "Related Modernizr Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/572"
// [ts] }, {
// [ts] "name": "Similar JSFiddle",
// [ts] "href": "https://jsfiddle.net/FWeinb/etnYC/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS vmax unit",
// [ts] "property": "cssvmaxunit",
// [ts] "caniuse": "viewport-units",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_vmaxunit"],
// [ts] "notes": [{
// [ts] "name": "Related Modernizr Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/572"
// [ts] }, {
// [ts] "name": "JSFiddle Example",
// [ts] "href": "https://jsfiddle.net/glsee/JDsWQ/4/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS vmin unit",
// [ts] "property": "cssvminunit",
// [ts] "caniuse": "viewport-units",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_vminunit"],
// [ts] "notes": [{
// [ts] "name": "Related Modernizr Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/572"
// [ts] }, {
// [ts] "name": "JSFiddle Example",
// [ts] "href": "https://jsfiddle.net/glsee/JRmdq/8/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS vw unit",
// [ts] "property": "cssvwunit",
// [ts] "caniuse": "viewport-units",
// [ts] "tags": ["css"],
// [ts] "builderAliases": ["css_vwunit"],
// [ts] "notes": [{
// [ts] "name": "Related Modernizr Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/572"
// [ts] }, {
// [ts] "name": "JSFiddle Example",
// [ts] "href": "https://jsfiddle.net/FWeinb/etnYC/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "will-change",
// [ts] "property": "willchange",
// [ts] "caniuse": "will-change",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://drafts.csswg.org/css-will-change/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CSS wrap-flow",
// [ts] "property": "wrapflow",
// [ts] "tags": ["css"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/css3-exclusions"
// [ts] }, {
// [ts] "name": "Example by Louie Rootfield",
// [ts] "href": "https://webdesign.tutsplus.com/tutorials/css-exclusions--cms-28087"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Custom Elements API",
// [ts] "property": "customelements",
// [ts] "caniuse": "custom-elementsv1",
// [ts] "tags": ["customelements"],
// [ts] "polyfills": ["customelements"],
// [ts] "notes": [{
// [ts] "name": "Specs for Custom Elements",
// [ts] "href": "https://www.w3.org/TR/custom-elements/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Custom protocol handler",
// [ts] "property": "customprotocolhandler",
// [ts] "authors": ["Ben Schwarz"],
// [ts] "builderAliases": ["custom_protocol_handler"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/dev/system-state.html#custom-handlers"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/navigator.registerProtocolHandler"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Dart",
// [ts] "property": "dart",
// [ts] "authors": ["Theodoor van Donge"],
// [ts] "notes": [{
// [ts] "name": "Language website",
// [ts] "href": "https://www.dartlang.org/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "DataView",
// [ts] "property": "dataview",
// [ts] "authors": ["Addy Osmani"],
// [ts] "builderAliases": ["dataview_api"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/JavaScript_typed_arrays/DataView"
// [ts] }],
// [ts] "polyfills": ["jdataview"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "classList",
// [ts] "caniuse": "classlist",
// [ts] "property": "classlist",
// [ts] "tags": ["dom"],
// [ts] "builderAliases": ["dataview_api"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/DOM/element.classList"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "createElement with Attributes",
// [ts] "property": ["createelementattrs", "createelement-attrs"],
// [ts] "tags": ["dom"],
// [ts] "builderAliases": ["dom_createElement_attrs"],
// [ts] "authors": ["James A. Rosen"],
// [ts] "notes": [{
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/258"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "dataset API",
// [ts] "caniuse": "dataset",
// [ts] "property": "dataset",
// [ts] "tags": ["dom"],
// [ts] "builderAliases": ["dom_dataset"],
// [ts] "authors": ["@phiggins42"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Document Fragment",
// [ts] "property": "documentfragment",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/REC-DOM-Level-1/level-one-core.html#ID-B63ED1A3"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/DocumentFragment"
// [ts] }, {
// [ts] "name": "QuirksMode Compatibility Tables",
// [ts] "href": "https://www.quirksmode.org/m/w3c_core.html#t112"
// [ts] }],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "knownBugs": ["false-positive on Blackberry 9500, see QuirksMode note"],
// [ts] "tags": ["dom"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "[hidden] Attribute",
// [ts] "property": "hidden",
// [ts] "tags": ["dom"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/dev/interaction.html#the-hidden-attribute"
// [ts] }, {
// [ts] "name": "original implementation of detect code",
// [ts] "href": "https://github.com/aFarkas/html5shiv/blob/bf4fcc4/src/html5shiv.js#L38"
// [ts] }],
// [ts] "polyfills": ["html5shiv"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Intersection Observer",
// [ts] "property": "intersectionobserver",
// [ts] "caniuse": "intersectionobserver",
// [ts] "tags": ["dom"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/IntersectionObserver/"
// [ts] }, {
// [ts] "name": "IntersectionObserver polyfill",
// [ts] "href": "https://github.com/w3c/IntersectionObserver/tree/master/polyfill"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/docs/Web/API/Intersection_Observer_API"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "microdata",
// [ts] "property": "microdata",
// [ts] "tags": ["dom"],
// [ts] "builderAliases": ["dom_microdata"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/microdata/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "DOM4 MutationObserver",
// [ts] "property": "mutationobserver",
// [ts] "caniuse": "mutationobserver",
// [ts] "tags": ["dom"],
// [ts] "authors": ["Karel Sedláček (@ksdlck)"],
// [ts] "polyfills": ["mutationobservers"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/MutationObserver"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "property": "passiveeventlisteners",
// [ts] "caniuse": "passive-event-listener",
// [ts] "tags": ["dom"],
// [ts] "authors": ["Rick Byers"],
// [ts] "name": "Passive event listeners",
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://dom.spec.whatwg.org/#dom-addeventlisteneroptions-passive"
// [ts] }, {
// [ts] "name": "WICG explainer",
// [ts] "href": "https://github.com/WICG/EventListenerOptions/blob/gh-pages/explainer.md"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Shadow DOM API",
// [ts] "property": "shadowroot",
// [ts] "caniuse": "shadowdomv1",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/ShadowRoot"
// [ts] }],
// [ts] "authors": ["Kevin Coyle (@kevin-coyle-unipro)", "Pascal Lim (@pascalim)"],
// [ts] "tags": ["dom"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Shadow DOM API (Legacy)",
// [ts] "property": "shadowrootlegacy",
// [ts] "caniuse": "shadowdom",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Element/createShadowRoot"
// [ts] }],
// [ts] "authors": ["Kevin Coyle (@kevin-coyle-unipro)", "Pascal Lim (@pascalim)"],
// [ts] "tags": ["dom"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "bdi Element",
// [ts] "property": "bdi",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/HTML/Element/bdi"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "details Element",
// [ts] "caniuse": "details",
// [ts] "property": "details",
// [ts] "tags": ["elem"],
// [ts] "builderAliases": ["elem_details"],
// [ts] "authors": ["@mathias"],
// [ts] "notes": [{
// [ts] "name": "Mathias' Original",
// [ts] "href": "https://mathiasbynens.be/notes/html5-details-jquery#comment-35"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "output Element",
// [ts] "property": "outputelem",
// [ts] "tags": ["elem"],
// [ts] "builderAliases": ["elem_output"],
// [ts] "notes": [{
// [ts] "name": "WhatWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/form-elements.html#the-output-element"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "picture Element",
// [ts] "property": "picture",
// [ts] "tags": ["elem"],
// [ts] "authors": ["Scott Jehl", "Mat Marquis"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/embedded-content.html#embedded-content"
// [ts] }, {
// [ts] "name": "Relevant spec issue",
// [ts] "href": "https://github.com/ResponsiveImagesCG/picture-element/issues/87"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "progress Element",
// [ts] "caniuse": "progress",
// [ts] "property": ["progressbar", "meter"],
// [ts] "tags": ["elem"],
// [ts] "builderAliases": ["elem_progress_meter"],
// [ts] "authors": ["Stefan Wallin"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ruby, rp, rt Elements",
// [ts] "caniuse": "ruby",
// [ts] "property": "ruby",
// [ts] "tags": ["elem"],
// [ts] "builderAliases": ["elem_ruby"],
// [ts] "authors": ["Cătălin Mariș"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-ruby-element"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Template Tag",
// [ts] "property": "template",
// [ts] "caniuse": "template",
// [ts] "tags": ["elem"],
// [ts] "notes": [{
// [ts] "name": "HTML5Rocks Article",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/webcomponents/template/"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://web.archive.org/web/20171130222649/http://www.w3.org/TR/html5/scripting-1.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "time Element",
// [ts] "property": "time",
// [ts] "tags": ["elem"],
// [ts] "builderAliases": ["elem_time"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/text-level-semantics.html#the-time-element"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Track element and Timed Text Track",
// [ts] "property": ["texttrackapi", "track"],
// [ts] "tags": ["elem"],
// [ts] "builderAliases": ["elem_track"],
// [ts] "authors": ["Addy Osmani"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec (Track Element)",
// [ts] "href": "https://web.archive.org/web/20121119095019/http://www.w3.org/TR/html5/the-track-element.html#the-track-element"
// [ts] }, {
// [ts] "name": "W3C Spec (Track API)",
// [ts] "href": "https://web.archive.org/web/20121119094620/http://www.w3.org/TR/html5/media-elements.html#text-track-api"
// [ts] }],
// [ts] "warnings": ["While IE10 has implemented the track element, IE10 does not expose the underlying APIs to create timed text tracks by JS (really sad)"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Unknown Elements",
// [ts] "property": "unknownelements",
// [ts] "tags": ["elem"],
// [ts] "notes": [{
// [ts] "name": "The Story of the HTML5 Shiv",
// [ts] "href": "https://www.paulirish.com/2011/the-history-of-the-html5-shiv/"
// [ts] }, {
// [ts] "name": "original implementation of detect code",
// [ts] "href": "https://github.com/aFarkas/html5shiv/blob/bf4fcc4/src/html5shiv.js#L36"
// [ts] }],
// [ts] "polyfills": ["html5shiv"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Emoji",
// [ts] "property": "emoji"
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Array",
// [ts] "property": "es5array",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "polyfills": ["es5shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Date",
// [ts] "property": "es5date",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "polyfills": ["es5shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Function",
// [ts] "property": "es5function",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "polyfills": ["es5shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Object",
// [ts] "property": "es5object",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "polyfills": ["es5shim", "es5sham"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Strict Mode",
// [ts] "property": "strictmode",
// [ts] "caniuse": "use-strict",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "authors": ["@kangax"],
// [ts] "tags": ["es5"],
// [ts] "builderAliases": ["es5_strictmode"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 String",
// [ts] "property": "es5string",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "polyfills": ["es5shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "JSON",
// [ts] "property": "json",
// [ts] "caniuse": "json",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Glossary/JSON"
// [ts] }],
// [ts] "polyfills": ["json2"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Syntax",
// [ts] "property": "es5syntax",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }, {
// [ts] "name": "original implementation of detect code",
// [ts] "href": "https://kangax.github.io/compat-table/es5/"
// [ts] }],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "warnings": ["This detect uses `eval()`, so CSP may be a problem."],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5 Immutable Undefined",
// [ts] "property": "es5undefined",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }, {
// [ts] "name": "original implementation of detect code",
// [ts] "href": "https://kangax.github.io/compat-table/es5/"
// [ts] }],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES5",
// [ts] "property": "es5",
// [ts] "caniuse": "es5",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 5.1 Language Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/5.1/"
// [ts] }],
// [ts] "polyfills": ["es5shim", "es5sham"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es5"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Array",
// [ts] "property": "es6array",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/index.html"
// [ts] }, {
// [ts] "name": "Last ECMAScript specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/index.html"
// [ts] }],
// [ts] "polyfills": ["es6shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Arrow Functions",
// [ts] "property": "arrow",
// [ts] "authors": ["Vincent Riemer"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Class",
// [ts] "property": "es6class",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 language specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/#sec-class-definitions"
// [ts] }],
// [ts] "caniuse": "es6-class",
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es6"],
// [ts] "builderAliases": ["class"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Collections",
// [ts] "property": "es6collections",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/index.html"
// [ts] }, {
// [ts] "name": "Last ECMAScript specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/index.html"
// [ts] }],
// [ts] "polyfills": ["es6shim", "weakmap"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Generators",
// [ts] "property": "generators",
// [ts] "authors": ["Michael Kachanovskyi"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Math",
// [ts] "property": "es6math",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/index.html"
// [ts] }, {
// [ts] "name": "Last ECMAScript specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/index.html"
// [ts] }],
// [ts] "polyfills": ["es6shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Number",
// [ts] "property": "es6number",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/index.html"
// [ts] }, {
// [ts] "name": "Last ECMAScript specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/index.html"
// [ts] }],
// [ts] "polyfills": ["es6shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Object",
// [ts] "property": "es6object",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/index.html"
// [ts] }, {
// [ts] "name": "Last ECMAScript specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/index.html"
// [ts] }],
// [ts] "polyfills": ["es6shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Promises",
// [ts] "property": "promises",
// [ts] "caniuse": "promises",
// [ts] "polyfills": ["es6promises"],
// [ts] "authors": ["Krister Kari", "Jake Archibald"],
// [ts] "tags": ["es6"],
// [ts] "notes": [{
// [ts] "name": "The ES6 promises spec",
// [ts] "href": "https://github.com/domenic/promises-unwrapping"
// [ts] }, {
// [ts] "name": "Chromium dashboard - ES6 Promises",
// [ts] "href": "https://www.chromestatus.com/features/5681726336532480"
// [ts] }, {
// [ts] "name": "JavaScript Promises: an Introduction",
// [ts] "href": "https://developers.google.com/web/fundamentals/primers/promises/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Rest parameters",
// [ts] "property": "restparameters",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 language specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/#sec-function-definitions"
// [ts] }],
// [ts] "caniuse": "rest",
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Spread array",
// [ts] "property": "spreadarray",
// [ts] "notes": [{
// [ts] "name": "ECMAScript Specification",
// [ts] "href": "https://tc39.es/ecma262/#sec-array-initializer"
// [ts] },
// [ts] {
// [ts] "name": "Article",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax"
// [ts] }],
// [ts] "caniuse": "mdn-javascript_operators_spread_spread_in_arrays",
// [ts] "authors": ["dabretin"],
// [ts] "warnings": ["not for object literals (implemented in ES7)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Template Strings",
// [ts] "property": "stringtemplate",
// [ts] "caniuse": "template-literals",
// [ts] "builderAliases": ["templatestrings"],
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 draft specification",
// [ts] "href": "https://tc39wiki.calculist.org/es6/template-strings/"
// [ts] }],
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 String",
// [ts] "property": "es6string",
// [ts] "notes": [{
// [ts] "name": "ECMAScript 6 Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/index.html"
// [ts] }, {
// [ts] "name": "Last ECMAScript Specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/index.html"
// [ts] }],
// [ts] "polyfills": ["es6shim"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["es6"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES6 Symbol",
// [ts] "property": "es6symbol",
// [ts] "caniuse": "mdn-javascript_builtins_symbol",
// [ts] "notes": [{
// [ts] "name": "Official ECMAScript 6 specification",
// [ts] "href": "https://www.ecma-international.org/ecma-262/6.0/#sec-symbol-constructor"
// [ts] },{
// [ts] "name": "MDN web docs",
// [ts] "href": "https://developer.mozilla.org/de/docs/Web/JavaScript/Reference/Global_Objects/Symbol"
// [ts] }],
// [ts] "polyfills": ["es6symbol"],
// [ts] "authors": ["buhichan (@buhichan)"],
// [ts] "tags": ["es6","symbol"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES7 Array",
// [ts] "property": "es7array",
// [ts] "notes": [{
// [ts] "name": "ECMAScript array Specification",
// [ts] "href": "https://tc39.es/ecma262/#sec-array.prototype.includes"
// [ts] }],
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es7"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES7 Rest destructuring",
// [ts] "property": ["restdestructuringarray", "restdestructuringobject"],
// [ts] "caniuse" : "destructuring%20assignment",
// [ts] "notes": [{
// [ts] "name": "ECMAScript Destructuring Assignment Specification",
// [ts] "href": "https://tc39.es/ecma262/#sec-destructuring-assignment"
// [ts] }],
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es7"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES7 Spread object",
// [ts] "property": "spreadobject",
// [ts] "notes": [{
// [ts] "name": "ECMAScript array Specification",
// [ts] "href": "http://www.ecma-international.org/ecma-262/#sec-object-initializer"
// [ts] }],
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es7"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ES8 Object",
// [ts] "property": "es8object",
// [ts] "notes": [{
// [ts] "name": "ECMAScript specification: Object.entries",
// [ts] "href": "https://www.ecma-international.org/ecma-262/#sec-object.entries"
// [ts] }, {
// [ts] "name": "ECMAScript specification: Object.values",
// [ts] "href": "https://www.ecma-international.org/ecma-262/#sec-object.values"
// [ts] }],
// [ts] "caniuse": "object-entries,object-values",
// [ts] "authors": ["dabretin"],
// [ts] "tags": ["es8"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "CustomEvent",
// [ts] "property": "customevent",
// [ts] "tags": ["customevent"],
// [ts] "authors": ["Alberto Elias"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/DOM-Level-3-Events/#interface-CustomEvent"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/docs/Web/API/CustomEvent"
// [ts] }],
// [ts] "polyfills": ["eventlistener"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Orientation and Motion Events",
// [ts] "property": ["devicemotion", "deviceorientation"],
// [ts] "caniuse": "deviceorientation",
// [ts] "notes": [{
// [ts] "name": "W3C Editor's Draft Spec",
// [ts] "href": "https://w3c.github.io/deviceorientation/"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Detecting_device_orientation"
// [ts] }],
// [ts] "authors": ["Shi Chuan"],
// [ts] "tags": ["event"],
// [ts] "builderAliases": ["event_deviceorientation_motion"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Event Listener",
// [ts] "property": "eventlistener",
// [ts] "caniuse": "addeventlistener",
// [ts] "authors": ["Andrew Betts (@triblondon)"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/DOM-Level-2-Events/events.html#Events-Registration-interfaces"
// [ts] }],
// [ts] "polyfills": ["eventlistener"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Force Touch Events",
// [ts] "property": "forcetouch",
// [ts] "authors": ["Kraig Walker"],
// [ts] "notes": [{
// [ts] "name": "Responding to Force Touch Events from JavaScript",
// [ts] "href": "https://developer.apple.com/library/archive/documentation/AppleApplications/Conceptual/SafariJSProgTopics/RespondingtoForceTouchEventsfromJavaScript.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Hashchange event",
// [ts] "property": "hashchange",
// [ts] "caniuse": "hashchange",
// [ts] "tags": ["history"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/WindowEventHandlers/onhashchange"
// [ts] }],
// [ts] "polyfills": [
// [ts] "jquery-hashchange",
// [ts] "moo-historymanager",
// [ts] "jquery-ajaxy",
// [ts] "hasher",
// [ts] "shistory"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "onInput Event",
// [ts] "property": "oninput",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/GlobalEventHandlers.oninput"
// [ts] }, {
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/input.html#common-input-element-attributes"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/210"
// [ts] }],
// [ts] "authors": ["Patrick Kettner"],
// [ts] "tags": ["event"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "DOM Pointer Events API",
// [ts] "property": "pointerevents",
// [ts] "caniuse": "pointer",
// [ts] "tags": ["input"],
// [ts] "authors": ["Stu Cox"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec (Pointer Events)",
// [ts] "href": "https://www.w3.org/TR/pointerevents/"
// [ts] }, {
// [ts] "name": "W3C Spec (Pointer Events Level 2)",
// [ts] "href": "https://www.w3.org/TR/pointerevents2/"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/PointerEvent"
// [ts] }],
// [ts] "warnings": ["This property name now refers to W3C DOM PointerEvents: https://github.com/Modernizr/Modernizr/issues/548#issuecomment-12812099"],
// [ts] "polyfills": ["pep"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Proximity API",
// [ts] "property": "proximity",
// [ts] "authors": ["Cătălin Mariș"],
// [ts] "tags": ["events", "proximity"],
// [ts] "caniuse": "proximity",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Proximity_Events"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/proximity/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "File API",
// [ts] "property": "filereader",
// [ts] "caniuse": "fileapi",
// [ts] "notes": [{
// [ts] "name": "W3C Working Draft Spec",
// [ts] "href": "https://www.w3.org/TR/FileAPI/"
// [ts] }],
// [ts] "tags": ["file"],
// [ts] "builderAliases": ["file_api"],
// [ts] "knownBugs": ["Will fail in Safari 5 due to its lack of support for the standards defined FileReader object"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Filesystem API",
// [ts] "property": "filesystem",
// [ts] "caniuse": "filesystem",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/file-system-api/"
// [ts] }],
// [ts] "authors": ["Eric Bidelman (@ebidel)"],
// [ts] "tags": ["file"],
// [ts] "builderAliases": ["file_filesystem"],
// [ts] "knownBugs": ["The API will be present in Chrome incognito, but will throw an exception. See crbug.com/93417"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Flash",
// [ts] "property": "flash",
// [ts] "tags": ["flash"],
// [ts] "polyfills": ["shumway"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Fullscreen API",
// [ts] "property": "fullscreen",
// [ts] "caniuse": "fullscreen",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/API/Fullscreen"
// [ts] }],
// [ts] "polyfills": ["screenfulljs"],
// [ts] "builderAliases": ["fullscreen_api"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "GamePad API",
// [ts] "property": "gamepads",
// [ts] "caniuse": "gamepad",
// [ts] "authors": ["Eric Bidelman"],
// [ts] "tags": ["media"],
// [ts] "warnings": ["In new browsers it may return false in non-HTTPS connections"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/gamepad/"
// [ts] }, {
// [ts] "name": "HTML5 Rocks Tutorial",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/doodles/gamepad/#toc-featuredetect"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Geolocation API",
// [ts] "property": "geolocation",
// [ts] "caniuse": "geolocation",
// [ts] "tags": ["media"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/WebAPI/Using_geolocation"
// [ts] }],
// [ts] "polyfills": [
// [ts] "joshuabell-polyfill",
// [ts] "webshims",
// [ts] "geo-location-javascript",
// [ts] "geolocation-api-polyfill"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Hidden Scrollbar",
// [ts] "property": "hiddenscroll",
// [ts] "authors": ["Oleg Korsunsky"],
// [ts] "tags": ["overlay"],
// [ts] "notes": [{
// [ts] "name": "Overlay Scrollbar description",
// [ts] "href": "https://developer.apple.com/library/mac/releasenotes/MacOSX/WhatsNewInOSX/Articles/MacOSX10_7.html#//apple_ref/doc/uid/TP40010355-SW39"
// [ts] }, {
// [ts] "name": "Video example of overlay scrollbars",
// [ts] "href": "https://gfycat.com/FoolishMeaslyAtlanticsharpnosepuffer"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "History API",
// [ts] "property": "history",
// [ts] "caniuse": "history",
// [ts] "tags": ["history"],
// [ts] "authors": ["Hay Kranen", "Alexander Farkas"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/html51/browsers.html#the-history-interface"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/window.history"
// [ts] }],
// [ts] "polyfills": ["historyjs", "html5historyapi"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "HTML Imports",
// [ts] "property": "htmlimports",
// [ts] "tags": ["html", "import"],
// [ts] "polyfills": ["polymer-htmlimports"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/webcomponents/spec/imports/"
// [ts] }, {
// [ts] "name": "HTML Imports - #include for the web",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/webcomponents/imports/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "IE8 compat mode",
// [ts] "property": "ie8compat",
// [ts] "authors": ["Erich Ocean"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "iframe[sandbox] Attribute",
// [ts] "property": "sandbox",
// [ts] "caniuse": "iframe-sandbox",
// [ts] "tags": ["iframe"],
// [ts] "builderAliases": ["iframe_sandbox"],
// [ts] "notes": [
// [ts] {
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/embedded-content.html#attr-iframe-sandbox"
// [ts] }],
// [ts] "knownBugs": ["False-positive on Firefox < 29"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "iframe[seamless] Attribute",
// [ts] "property": "seamless",
// [ts] "tags": ["iframe"],
// [ts] "builderAliases": ["iframe_seamless"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/embedded-content.html#attr-iframe-seamless"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "iframe[srcdoc] Attribute",
// [ts] "property": "srcdoc",
// [ts] "caniuse": "iframe-srcdoc",
// [ts] "tags": ["iframe"],
// [ts] "builderAliases": ["iframe_srcdoc"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/embedded-content.html#attr-iframe-srcdoc"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Animated PNG",
// [ts] "async": true,
// [ts] "property": "apng",
// [ts] "caniuse": "apng",
// [ts] "tags": ["image"],
// [ts] "builderAliases": ["img_apng"],
// [ts] "notes": [{
// [ts] "name": "Wikipedia Article",
// [ts] "href": "https://en.wikipedia.org/wiki/APNG"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "AVIF",
// [ts] "async": true,
// [ts] "property": "avif",
// [ts] "caniuse": "avif",
// [ts] "tags": ["image"],
// [ts] "authors": ["Markel Ferro (@MarkelFe)"],
// [ts] "polyfills": ["avifjs"],
// [ts] "notes": [{
// [ts] "name": "Avif Spec",
// [ts] "href": "https://aomediacodec.github.io/av1-avif/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Image crossOrigin",
// [ts] "property": "imgcrossorigin",
// [ts] "tags": ["image"],
// [ts] "notes": [{
// [ts] "name": "Cross Domain Images and the Tainted Canvas",
// [ts] "href": "https://blog.codepen.io/2013/10/08/cross-domain-images-tainted-canvas/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "EXIF Orientation",
// [ts] "property": "exiforientation",
// [ts] "tags": ["image"],
// [ts] "builderAliases": ["exif_orientation"],
// [ts] "async": true,
// [ts] "authors": ["Paul Sayre"],
// [ts] "notes": [{
// [ts] "name": "Article by Dave Perrett",
// [ts] "href": "https://www.daveperrett.com/articles/2012/07/28/exif-orientation-handling-is-a-ghetto/"
// [ts] }, {
// [ts] "name": "Article by Calvin Hass",
// [ts] "href": "https://www.impulseadventure.com/photo/exif-orientation.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "JPEG 2000",
// [ts] "async": true,
// [ts] "aliases": ["jpeg-2000", "jpg2"],
// [ts] "property": "jpeg2000",
// [ts] "caniuse": "jpeg2000",
// [ts] "tags": ["image"],
// [ts] "authors": ["@eric_wvgg"],
// [ts] "notes": [{
// [ts] "name": "Wikipedia Article",
// [ts] "href": "https://en.wikipedia.org/wiki/JPEG_2000"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "JPEG XR (extended range)",
// [ts] "async": true,
// [ts] "aliases": ["jpeg-xr"],
// [ts] "property": "jpegxr",
// [ts] "tags": ["image"],
// [ts] "notes": [{
// [ts] "name": "Wikipedia Article",
// [ts] "href": "https://en.wikipedia.org/wiki/JPEG_XR"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "image and iframe native lazy loading",
// [ts] "property": "lazyloading",
// [ts] "caniuse": "loading-lazy-attr",
// [ts] "tags": ["image", "lazy", "loading"],
// [ts] "notes": [{
// [ts] "name": "Native image lazy-loading for the web",
// [ts] "href": "https://addyosmani.com/blog/lazy-loading/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "sizes attribute",
// [ts] "async": true,
// [ts] "property": "sizes",
// [ts] "tags": ["image"],
// [ts] "authors": ["Mat Marquis"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/embedded-content.html#the-img-element"
// [ts] }, {
// [ts] "name": "Srcset and sizes",
// [ts] "href": "https://ericportis.com/posts/2014/srcset-sizes/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "srcset attribute",
// [ts] "property": "srcset",
// [ts] "caniuse": "srcset",
// [ts] "tags": ["image"],
// [ts] "notes": [{
// [ts] "name": "Smashing Magazine Article",
// [ts] "href": "https://www.smashingmagazine.com/2013/08/webkit-implements-srcset-and-why-its-a-good-thing/"
// [ts] }, {
// [ts] "name": "Generate multi-resolution images for srcset with Grunt",
// [ts] "href": "https://addyosmani.com/blog/generate-multi-resolution-images-for-srcset-with-grunt/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Webp Alpha",
// [ts] "async": true,
// [ts] "property": "webpalpha",
// [ts] "aliases": ["webp-alpha"],
// [ts] "tags": ["image"],
// [ts] "authors": ["Krister Kari", "Rich Bradshaw", "Ryan Seddon", "Paul Irish"],
// [ts] "notes": [{
// [ts] "name": "WebP Info",
// [ts] "href": "https://developers.google.com/speed/webp/"
// [ts] }, {
// [ts] "name": "Article about WebP support",
// [ts] "href": "https://optimus.keycdn.com/support/webp-support/"
// [ts] }, {
// [ts] "name": "Chromium WebP announcement",
// [ts] "href": "https://blog.chromium.org/2011/11/lossless-and-transparency-encoding-in.html?m=1"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Webp Animation",
// [ts] "async": true,
// [ts] "property": "webpanimation",
// [ts] "aliases": ["webp-animation"],
// [ts] "tags": ["image"],
// [ts] "authors": ["Krister Kari", "Rich Bradshaw", "Ryan Seddon", "Paul Irish"],
// [ts] "notes": [{
// [ts] "name": "WebP Info",
// [ts] "href": "https://developers.google.com/speed/webp/"
// [ts] }, {
// [ts] "name": "Chromium blog - Chrome 32 Beta: Animated WebP images and faster Chrome for Android touch input",
// [ts] "href": "https://blog.chromium.org/2013/11/chrome-32-beta-animated-webp-images-and.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Webp Lossless",
// [ts] "async": true,
// [ts] "property": ["webplossless", "webp-lossless"],
// [ts] "tags": ["image"],
// [ts] "authors": ["@amandeep", "Rich Bradshaw", "Ryan Seddon", "Paul Irish"],
// [ts] "notes": [{
// [ts] "name": "Webp Info",
// [ts] "href": "https://developers.google.com/speed/webp/"
// [ts] }, {
// [ts] "name": "Webp Lossless Spec",
// [ts] "href": "https://developers.google.com/speed/webp/docs/webp_lossless_bitstream_specification"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Webp",
// [ts] "async": true,
// [ts] "property": "webp",
// [ts] "caniuse": "webp",
// [ts] "tags": ["image"],
// [ts] "builderAliases": ["img_webp"],
// [ts] "authors": ["Krister Kari", "@amandeep", "Rich Bradshaw", "Ryan Seddon", "Paul Irish"],
// [ts] "notes": [{
// [ts] "name": "Webp Info",
// [ts] "href": "https://developers.google.com/speed/webp/"
// [ts] }, {
// [ts] "name": "Chromium blog - Chrome 32 Beta: Animated WebP images and faster Chrome for Android touch input",
// [ts] "href": "https://blog.chromium.org/2013/11/chrome-32-beta-animated-webp-images-and.html"
// [ts] }, {
// [ts] "name": "Webp Lossless Spec",
// [ts] "href": "https://developers.google.com/speed/webp/docs/webp_lossless_bitstream_specification"
// [ts] }, {
// [ts] "name": "Article about WebP support",
// [ts] "href": "https://optimus.keycdn.com/support/webp-support/"
// [ts] }, {
// [ts] "name": "Chromium WebP announcement",
// [ts] "href": "https://blog.chromium.org/2011/11/lossless-and-transparency-encoding-in.html?m=1"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input[capture] Attribute",
// [ts] "property": "capture",
// [ts] "tags": ["video", "image", "audio", "media", "attribute"],
// [ts] "notes": [{
// [ts] "name": "W3C Draft Spec",
// [ts] "href": "https://www.w3.org/TR/html-media-capture/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input[file] Attribute",
// [ts] "property": "fileinput",
// [ts] "caniuse": "forms",
// [ts] "tags": ["file", "forms", "input"],
// [ts] "builderAliases": ["forms_fileinput"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input[directory] Attribute",
// [ts] "property": "directory",
// [ts] "authors": ["silverwind"],
// [ts] "tags": ["file", "input", "attribute"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input formaction",
// [ts] "property": "inputformaction",
// [ts] "aliases": ["input-formaction"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fs-formaction"
// [ts] }, {
// [ts] "name": "Wufoo demo",
// [ts] "href": "https://www.wufoo.com/html5/formaction-attribute/"
// [ts] }],
// [ts] "polyfills": ["webshims"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input[form] Attribute",
// [ts] "property": "formattribute",
// [ts] "tags": ["attribute", "forms", "input"],
// [ts] "builderAliases": ["forms_formattribute"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input formenctype",
// [ts] "property": "inputformenctype",
// [ts] "aliases": ["input-formenctype"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fs-formenctype"
// [ts] }, {
// [ts] "name": "Wufoo demo",
// [ts] "href": "https://www.wufoo.com/html5/formenctype-attribute/"
// [ts] }],
// [ts] "polyfills": ["html5formshim"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input formmethod",
// [ts] "property": "inputformmethod",
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fs-formmethod"
// [ts] }, {
// [ts] "name": "Wufoo demo",
// [ts] "href": "https://www.wufoo.com/html5/formmethod-attribute/"
// [ts] }],
// [ts] "polyfills": ["webshims"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input formnovalidate",
// [ts] "property": "inputformnovalidate",
// [ts] "aliases": ["input-formnovalidate"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fs-formnovalidate"
// [ts] }, {
// [ts] "name": "Wufoo demo",
// [ts] "href": "https://www.wufoo.com/html5/formnovalidate-attribute/"
// [ts] }],
// [ts] "polyfills": ["html5formshim"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input formtarget",
// [ts] "property": "inputformtarget",
// [ts] "aliases": ["input-formtarget"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fs-formtarget"
// [ts] }, {
// [ts] "name": "Wufoo demo",
// [ts] "href": "https://www.wufoo.com/html5/formtarget-attribute/"
// [ts] }],
// [ts] "polyfills": ["html5formshim"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Input attributes",
// [ts] "property": "input",
// [ts] "tags": ["forms"],
// [ts] "authors": ["Mike Taylor"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/input.html#input-type-attr-summary"
// [ts] }],
// [ts] "knownBugs": ["Some blackberry devices report false positive for input.multiple"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Form input types",
// [ts] "property": "inputtypes",
// [ts] "caniuse": "forms",
// [ts] "tags": ["forms"],
// [ts] "authors": ["Mike Taylor"],
// [ts] "polyfills": [
// [ts] "jquerytools",
// [ts] "webshims",
// [ts] "h5f",
// [ts] "webforms2",
// [ts] "nwxforms",
// [ts] "fdslider",
// [ts] "html5slider",
// [ts] "galleryhtml5forms",
// [ts] "jscolor",
// [ts] "html5formshim",
// [ts] "selectedoptionsjs",
// [ts] "formvalidationjs"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Form Validation",
// [ts] "property": "formvalidation",
// [ts] "tags": ["forms", "validation", "attribute"],
// [ts] "builderAliases": ["forms_validation"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input[type=\"number\"] Localization",
// [ts] "property": "localizednumber",
// [ts] "tags": ["forms", "localization", "attribute"],
// [ts] "authors": ["Peter Janes"],
// [ts] "notes": [{
// [ts] "name": "Webkit Bug Tracker Listing",
// [ts] "href": "https://bugs.webkit.org/show_bug.cgi?id=42484"
// [ts] }, {
// [ts] "name": "Based on This",
// [ts] "href": "https://trac.webkit.org/browser/trunk/LayoutTests/fast/forms/script-tests/input-number-keyoperation.js?rev=80096#L9"
// [ts] }],
// [ts] "knownBugs": ["Only ever returns true if the browser/OS is configured to use comma as a decimal separator. This is probably fine for most use cases."]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "input[search] search event",
// [ts] "property": "inputsearchevent",
// [ts] "tags": ["input","search"],
// [ts] "authors": ["Calvin Webster"],
// [ts] "notes": [{
// [ts] "name": "Wufoo demo",
// [ts] "href": "https://www.wufoo.com/html5/search-type/"
// [ts] }, {
// [ts] "name": "CSS Tricks",
// [ts] "href": "https://css-tricks.com/webkit-html5-search-inputs/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "placeholder attribute",
// [ts] "property": "placeholder",
// [ts] "tags": ["forms", "attribute"],
// [ts] "builderAliases": ["forms_placeholder"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "form#requestAutocomplete()",
// [ts] "property": "requestautocomplete",
// [ts] "tags": ["form", "forms", "requestAutocomplete", "payments"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://wiki.whatwg.org/wiki/RequestAutocomplete"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Internationalization API",
// [ts] "property": "intl",
// [ts] "caniuse": "internationalization",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Intl"
// [ts] }, {
// [ts] "name": "ECMAScript spec",
// [ts] "href": "https://www.ecma-international.org/ecma-402/1.0/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Font Ligatures",
// [ts] "property": "ligatures",
// [ts] "caniuse": "font-feature",
// [ts] "notes": [{
// [ts] "name": "Cross-browser Web Fonts",
// [ts] "href": "https://www.sitepoint.com/cross-browser-web-fonts-part-3/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Reverse Ordered Lists",
// [ts] "property": "olreversed",
// [ts] "notes": [{
// [ts] "name": "Impressive Webs article",
// [ts] "href": "https://www.impressivewebs.com/reverse-ordered-lists-html5/"
// [ts] }],
// [ts] "builderAliases": ["lists_reversed"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "MathML",
// [ts] "property": "mathml",
// [ts] "caniuse": "mathml",
// [ts] "authors": ["Addy Osmani", "Davide P. Cervone", "David Carlisle"],
// [ts] "knownBugs": ["Firefox < 4 will likely return a false, however it does support MathML inside XHTML documents"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/Math/"
// [ts] }],
// [ts] "polyfills": ["mathjax"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Media Source Extensions API",
// [ts] "caniuse": "mediasource",
// [ts] "property": "mediasource",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API"
// [ts] }],
// [ts] "builderAliases": ["media_source_extension_api"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Hover Media Query",
// [ts] "property": "hovermq",
// [ts] "tags": ["mediaquery"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Pointer Media Query",
// [ts] "property": "pointermq",
// [ts] "tags": ["mediaquery"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Message Channel",
// [ts] "property": "messagechannel",
// [ts] "authors": ["Raju Konga (@kongaraju)"],
// [ts] "caniuse": "channel-messaging",
// [ts] "tags": ["performance", "messagechannel"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/2011/WD-webmessaging-20110317/#message-channels"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Channel_Messaging_API/Using_channel_messaging"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Beacon API",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/navigator.sendBeacon"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/beacon/"
// [ts] }],
// [ts] "property": "beacon",
// [ts] "caniuse": "beacon",
// [ts] "tags": ["beacon", "network"],
// [ts] "authors": ["Cătălin Mariș"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Connection Effective Type",
// [ts] "notes": [{
// [ts] "name": "MDN documentation",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/NetworkInformation/effectiveType"
// [ts] }],
// [ts] "property": "connectioneffectivetype",
// [ts] "builderAliases": ["network_connection"],
// [ts] "tags": ["network"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Low Bandwidth Connection",
// [ts] "property": "lowbandwidth",
// [ts] "tags": ["network"],
// [ts] "builderAliases": ["network_connection"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Server Sent Events",
// [ts] "property": "eventsource",
// [ts] "tags": ["network"],
// [ts] "builderAliases": ["network_eventsource"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/server-sent-events.html#server-sent-events"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Fetch API",
// [ts] "property": "fetch",
// [ts] "tags": ["network"],
// [ts] "caniuse": "fetch",
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://fetch.spec.whatwg.org/"
// [ts] }],
// [ts] "polyfills": ["fetch"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XHR responseType='arraybuffer'",
// [ts] "property": "xhrresponsetypearraybuffer",
// [ts] "tags": ["network"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://xhr.spec.whatwg.org/#the-responsetype-attribute"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XHR responseType='blob'",
// [ts] "property": "xhrresponsetypeblob",
// [ts] "tags": ["network"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://xhr.spec.whatwg.org/#the-responsetype-attribute"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XHR responseType='document'",
// [ts] "property": "xhrresponsetypedocument",
// [ts] "tags": ["network"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://xhr.spec.whatwg.org/#the-responsetype-attribute"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XHR responseType='json'",
// [ts] "property": "xhrresponsetypejson",
// [ts] "tags": ["network"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://xhr.spec.whatwg.org/#the-responsetype-attribute"
// [ts] }, {
// [ts] "name": "Explanation of xhr.responseType='json'",
// [ts] "href": "https://mathiasbynens.be/notes/xhr-responsetype-json"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XHR responseType='text'",
// [ts] "property": "xhrresponsetypetext",
// [ts] "tags": ["network"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://xhr.spec.whatwg.org/#the-responsetype-attribute"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XHR responseType",
// [ts] "property": "xhrresponsetype",
// [ts] "tags": ["network"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://xhr.spec.whatwg.org/#the-responsetype-attribute"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XML HTTP Request Level 2 XHR2",
// [ts] "property": "xhr2",
// [ts] "caniuse": "xhr2",
// [ts] "tags": ["network"],
// [ts] "builderAliases": ["network_xhr2"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/XMLHttpRequest2/"
// [ts] }, {
// [ts] "name": "Details on Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/385"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Notification",
// [ts] "property": "notification",
// [ts] "caniuse": "notifications",
// [ts] "authors": ["Theodoor van Donge", "Hendrik Beskow"],
// [ts] "notes": [{
// [ts] "name": "HTML5 Rocks Tutorial",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/notifications/quick/"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/notifications/"
// [ts] }, {
// [ts] "name": "Changes in Chrome to Notifications API due to Service Worker Push Notifications",
// [ts] "href": "https://developers.google.com/web/updates/2015/05/Notifying-you-of-notificiation-changes"
// [ts] }],
// [ts] "knownBugs": ["Possibility of false-positive on Chrome for Android if permissions we're granted for a website prior to Chrome 44."],
// [ts] "polyfills": ["desktop-notify", "html5-notifications"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Page Visibility API",
// [ts] "property": "pagevisibility",
// [ts] "caniuse": "pagevisibility",
// [ts] "tags": ["performance"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/DOM/Using_the_Page_Visibility_API"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/2011/WD-page-visibility-20110602/"
// [ts] }, {
// [ts] "name": "HTML5 Rocks Tutorial",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/pagevisibility/intro/"
// [ts] }],
// [ts] "polyfills": ["visibilityjs", "visiblyjs", "jquery-visibility"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Navigation Timing API",
// [ts] "property": "performance",
// [ts] "caniuse": "nav-timing",
// [ts] "tags": ["performance"],
// [ts] "authors": ["Scott Murphy (@uxder)"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/navigation-timing/"
// [ts] }, {
// [ts] "name": "HTML5 Rocks Tutorial",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/webperformance/basics/"
// [ts] }],
// [ts] "polyfills": ["perfnow"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Pointer Lock API",
// [ts] "property": "pointerlock",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/API/Pointer_Lock_API"
// [ts] }],
// [ts] "builderAliases": ["pointerlock_api"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "postMessage",
// [ts] "property": "postmessage",
// [ts] "caniuse": "x-doc-messaging",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/webmessaging/#crossDocumentMessages"
// [ts] }],
// [ts] "polyfills": ["easyxdm", "postmessage-jquery"],
// [ts] "knownBugs": [
// [ts] "structuredclones - Android 2&3 can not send a structured clone of dates, filelists or regexps.",
// [ts] "Some old WebKit versions have bugs."
// [ts] ],
// [ts] "warnings": ["To be safe you should stick with object, array, number and pixeldata."]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Proxy Object",
// [ts] "property": "proxy",
// [ts] "caniuse": "proxy",
// [ts] "authors": ["Brock Beaudry"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Proxy"
// [ts] }],
// [ts] "polyfills": [
// [ts] "harmony-reflect"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "QuerySelector",
// [ts] "property": "queryselector",
// [ts] "caniuse": "queryselector",
// [ts] "tags": ["queryselector"],
// [ts] "authors": ["Andrew Betts (@triblondon)"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/selectors-api/#queryselectorall"
// [ts] }],
// [ts] "polyfills": ["css-selector-engine"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "rel=prefetch",
// [ts] "property": "prefetch",
// [ts] "caniuse": "link-rel-prefetch",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/resource-hints/#prefetch"
// [ts] }, {
// [ts] "name": "Related Github Issue",
// [ts] "href": "https://github.com/Modernizr/Modernizr/issues/2536"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "requestAnimationFrame",
// [ts] "property": "requestanimationframe",
// [ts] "aliases": ["raf"],
// [ts] "caniuse": "requestanimationframe",
// [ts] "tags": ["animation"],
// [ts] "authors": ["Addy Osmani"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/animation-timing/"
// [ts] }],
// [ts] "polyfills": ["raf"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "script[async]",
// [ts] "property": "scriptasync",
// [ts] "caniuse": "script-async",
// [ts] "tags": ["script"],
// [ts] "builderAliases": ["script_async"],
// [ts] "authors": ["Theodoor van Donge"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "script[defer]",
// [ts] "property": "scriptdefer",
// [ts] "caniuse": "script-defer",
// [ts] "tags": ["script"],
// [ts] "builderAliases": ["script_defer"],
// [ts] "authors": ["Theodoor van Donge"],
// [ts] "warnings": ["Browser implementation of the `defer` attribute vary: https://stackoverflow.com/questions/3952009/defer-attribute-chrome#answer-3982619"],
// [ts] "knownBugs": ["False positive in Opera 12"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "scrollToOptions dictionary",
// [ts] "property": "scrolltooptions",
// [ts] "caniuse": "mdn-api_scrolltooptions",
// [ts] "notes": [{
// [ts] "name": "MDN docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Window/scrollTo"
// [ts] }],
// [ts] "authors": ["Oliver Tušla (@asmarcz)", "Chris Smith (@chris13524)"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ServiceWorker API",
// [ts] "property": "serviceworker",
// [ts] "caniuse": "serviceworkers",
// [ts] "notes": [{
// [ts] "name": "ServiceWorkers Explained",
// [ts] "href": "https://github.com/slightlyoff/ServiceWorker/blob/master/explainer.md"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "property": "speechrecognition",
// [ts] "caniuse": "speech-recognition",
// [ts] "tags": ["input", "speech"],
// [ts] "authors": ["Cătălin Mariș"],
// [ts] "name": "Speech Recognition API",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/speech-api/speechapi.html#speechreco-section"
// [ts] }, {
// [ts] "name": "Introduction to the Web Speech API",
// [ts] "href": "https://developers.google.com/web/updates/2013/01/Voice-Driven-Web-Apps-Introduction-to-the-Web-Speech-API"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "property": "speechsynthesis",
// [ts] "caniuse": "speech-synthesis",
// [ts] "tags": ["input", "speech"],
// [ts] "authors": ["Cătălin Mariș"],
// [ts] "name": "Speech Synthesis API",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/speech-api/speechapi.html#tts-section"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Cookies",
// [ts] "property": "cookies",
// [ts] "tags": ["storage"],
// [ts] "authors": ["tauren"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "IndexedDB",
// [ts] "property": "indexeddb",
// [ts] "caniuse": "indexeddb",
// [ts] "tags": ["storage"],
// [ts] "polyfills": ["indexeddb"],
// [ts] "async": true
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "IndexedDB Blob",
// [ts] "property": "indexeddbblob",
// [ts] "tags": ["storage"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "IndexedDB 2.0",
// [ts] "property": "indexeddb2",
// [ts] "tags": ["storage"],
// [ts] "caniuse": "indexeddb2",
// [ts] "authors": ["Tan Zhen Yong (@Xenonym)"],
// [ts] "polyfills": ["indexeddb"],
// [ts] "async": true
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Local Storage",
// [ts] "property": "localstorage",
// [ts] "caniuse": "namevalue-storage",
// [ts] "tags": ["storage"],
// [ts] "polyfills": [
// [ts] "joshuabell-polyfill",
// [ts] "cupcake",
// [ts] "storagepolyfill",
// [ts] "amplifyjs",
// [ts] "yui-cacheoffline"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Quota Storage Management API",
// [ts] "property": "quotamanagement",
// [ts] "tags": ["storage"],
// [ts] "builderAliases": ["quota_management_api"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/quota-api/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Session Storage",
// [ts] "property": "sessionstorage",
// [ts] "tags": ["storage"],
// [ts] "polyfills": ["joshuabell-polyfill", "cupcake", "storagepolyfill"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "IE User Data API",
// [ts] "property": "userdata",
// [ts] "tags": ["storage"],
// [ts] "authors": ["@stereobooster"],
// [ts] "notes": [{
// [ts] "name": "MSDN Documentation",
// [ts] "href": "https://msdn.microsoft.com/en-us/library/ms531424.aspx"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Web SQL Database",
// [ts] "property": "websqldatabase",
// [ts] "caniuse": "sql-storage",
// [ts] "tags": ["storage"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "style[scoped]",
// [ts] "property": "stylescoped",
// [ts] "caniuse": "style-scoped",
// [ts] "tags": ["dom"],
// [ts] "builderAliases": ["style_scoped"],
// [ts] "authors": ["Cătălin Mariș"],
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://html.spec.whatwg.org/multipage/semantics.html#attr-style-scoped"
// [ts] }],
// [ts] "polyfills": ["scoped-styles"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "SVG",
// [ts] "property": "svg",
// [ts] "caniuse": "svg",
// [ts] "tags": ["svg"],
// [ts] "authors": ["Erik Dahlstrom"],
// [ts] "polyfills": [
// [ts] "svgweb",
// [ts] "raphael",
// [ts] "canvg",
// [ts] "svg-boilerplate",
// [ts] "sie",
// [ts] "fabricjs"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "SVG as an <img> tag source",
// [ts] "property": "svgasimg",
// [ts] "caniuse": "svg-img",
// [ts] "tags": ["svg"],
// [ts] "aliases": ["svgincss"],
// [ts] "authors": ["Chris Coyier"],
// [ts] "notes": [{
// [ts] "name": "HTML5 Spec",
// [ts] "href": "https://www.w3.org/TR/html5/embedded-content-0.html#the-img-element"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "SVG clip paths",
// [ts] "property": "svgclippaths",
// [ts] "tags": ["svg"],
// [ts] "notes": [{
// [ts] "name": "Demo",
// [ts] "href": "http://srufaculty.sru.edu/david.dailey/svg/newstuff/clipPath4.svg"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "SVG filters",
// [ts] "property": "svgfilters",
// [ts] "caniuse": "svg-filters",
// [ts] "tags": ["svg"],
// [ts] "builderAliases": ["svg_filters"],
// [ts] "authors": ["Erik Dahlstrom"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/SVG11/filters.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "SVG foreignObject",
// [ts] "property": "svgforeignobject",
// [ts] "tags": ["svg"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/SVG11/extend.html"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Inline SVG",
// [ts] "property": "inlinesvg",
// [ts] "caniuse": "svg-html5",
// [ts] "tags": ["svg"],
// [ts] "notes": [{
// [ts] "name": "Test page",
// [ts] "href": "https://paulirish.com/demo/inline-svg"
// [ts] }, {
// [ts] "name": "Test page and results",
// [ts] "href": "https://codepen.io/eltonmesquita/full/GgXbvo/"
// [ts] }],
// [ts] "polyfills": ["inline-svg-polyfill"],
// [ts] "knownBugs": ["False negative on some Chromia browsers."]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "SVG SMIL animation",
// [ts] "property": "smil",
// [ts] "caniuse": "svg-smil",
// [ts] "tags": ["svg"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/AudioVideo/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "textarea maxlength",
// [ts] "property": "textareamaxlength",
// [ts] "aliases": ["textarea-maxlength"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/HTML/Element/textarea"
// [ts] }],
// [ts] "polyfills": ["maxlength"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Text Encoding/Decoding",
// [ts] "property": ["textencoder", "textdecoder"],
// [ts] "caniuse" : "textencoder",
// [ts] "notes": [{
// [ts] "name": "MDN TextEncoder Doc",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/TextEncoder"
// [ts] }, {
// [ts] "name": "MDN TextDecoder Doc",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/TextDecoder"
// [ts] }],
// [ts] "authors": ["dabretin"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Typed arrays",
// [ts] "property": "typedarrays",
// [ts] "caniuse": "typedarrays",
// [ts] "tags": ["js"],
// [ts] "authors": ["Stanley Stuart (@fivetanley)"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/JavaScript/Typed_arrays"
// [ts] }, {
// [ts] "name": "Kronos spec",
// [ts] "href": "http://www.ecma-international.org/ecma-262/6.0/#sec-typedarray-objects"
// [ts] }],
// [ts] "polyfills": ["joshuabell-polyfill"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Unicode Range",
// [ts] "property": "unicoderange",
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/2013/CR-css-fonts-3-20131003/#descdef-unicode-range"
// [ts] }, {
// [ts] "name": "24 Way article",
// [ts] "href": "https://24ways.org/2011/creating-custom-font-stacks-with-unicode-range"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Blob URLs",
// [ts] "property": "bloburls",
// [ts] "caniuse": "bloburls",
// [ts] "notes": [{
// [ts] "name": "W3C Working Draft Spec",
// [ts] "href": "https://www.w3.org/TR/FileAPI/#creating-revoking"
// [ts] }],
// [ts] "tags": ["file", "url"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Data URI",
// [ts] "property": "datauri",
// [ts] "caniuse": "datauri",
// [ts] "tags": ["url"],
// [ts] "builderAliases": ["url_data_uri"],
// [ts] "async": true,
// [ts] "notes": [{
// [ts] "name": "Wikipedia article",
// [ts] "href": "https://en.wikipedia.org/wiki/Data_URI_scheme"
// [ts] }],
// [ts] "warnings": ["Support in Internet Explorer 8 is limited to images and linked resources like CSS files, not HTML files"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "URL parser",
// [ts] "property": "urlparser",
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://url.spec.whatwg.org/"
// [ts] }],
// [ts] "polyfills": ["urlparser"],
// [ts] "authors": ["Ron Waldon (@jokeyrhyme)"],
// [ts] "tags": ["url"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "property": "urlsearchparams",
// [ts] "caniuse": "urlsearchparams",
// [ts] "tags": ["querystring", "url"],
// [ts] "authors": ["Cătălin Mariș"],
// [ts] "name": "URLSearchParams API",
// [ts] "notes": [{
// [ts] "name": "WHATWG Spec",
// [ts] "href": "https://url.spec.whatwg.org/#interface-urlsearchparams"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/URLSearchParams"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Vibration API",
// [ts] "property": "vibrate",
// [ts] "caniuse": "vibration",
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en/DOM/window.navigator.mozVibrate"
// [ts] }, {
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/vibration/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "HTML5 Video",
// [ts] "property": "video",
// [ts] "caniuse": "video",
// [ts] "tags": ["html5", "video", "media"],
// [ts] "knownBugs": ["Without QuickTime, `Modernizr.video.h264` will be `undefined`; https://github.com/Modernizr/Modernizr/issues/546"],
// [ts] "polyfills": [
// [ts] "html5media",
// [ts] "mediaelementjs",
// [ts] "videojs",
// [ts] "leanbackplayer",
// [ts] "videoforeverybody"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Video Autoplay",
// [ts] "property": "videoautoplay",
// [ts] "tags": ["video"],
// [ts] "async": true,
// [ts] "warnings": ["This test is very large – only include it if you absolutely need it"],
// [ts] "knownBugs": ["crashes with an alert on iOS7 when added to homescreen"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Video crossOrigin",
// [ts] "property": "videocrossorigin",
// [ts] "caniuse": "cors",
// [ts] "authors": ["Florian Mailliet"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/HTML/CORS_settings_attributes"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Video Loop Attribute",
// [ts] "property": "videoloop",
// [ts] "tags": ["video", "media"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Video Preload Attribute",
// [ts] "property": "videopreload",
// [ts] "tags": ["video", "media"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "VML",
// [ts] "property": "vml",
// [ts] "tags": ["vml"],
// [ts] "authors": ["Craig Andrews (@candrews)"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/NOTE-VML"
// [ts] }, {
// [ts] "name": "MSDN Documentation",
// [ts] "href": "https://docs.microsoft.com/en-us/windows/desktop/VML/msdn-online-vml-introduction"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Web Intents",
// [ts] "property": "webintents",
// [ts] "authors": ["Eric Bidelman"],
// [ts] "notes": [{
// [ts] "name": "Web Intents project site",
// [ts] "href": "http://www.webintents.org/"
// [ts] }],
// [ts] "builderAliases": ["web_intents"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Web Animation API",
// [ts] "property": "webanimations",
// [ts] "caniuse": "web-animation",
// [ts] "tags": ["webanimations"],
// [ts] "polyfills": ["webanimationsjs"],
// [ts] "notes": [{
// [ts] "name": "Introducing Web Animations",
// [ts] "href": "https://birtles.wordpress.com/2013/06/26/introducing-web-animations/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "PublicKeyCredential",
// [ts] "notes": [
// [ts] {
// [ts] "name": "MDN Documentation",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/PublicKeyCredential"
// [ts] },
// [ts] {
// [ts] "name": "Google Developers solution",
// [ts] "href": "https://developers.google.com/web/updates/2018/03/webauthn-credential-management#the_solution"
// [ts] }
// [ts] ],
// [ts] "property": "publickeycredential",
// [ts] "tags": ["webauthn", "web authentication"],
// [ts] "authors": ["Eric Delia"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "WebGL",
// [ts] "property": "webgl",
// [ts] "caniuse": "webgl",
// [ts] "tags": ["webgl", "graphics"],
// [ts] "polyfills": ["jebgl", "cwebgl", "iewebgl"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "WebGL Extensions",
// [ts] "property": "webglextensions",
// [ts] "tags": ["webgl", "graphics"],
// [ts] "builderAliases": ["webgl_extensions"],
// [ts] "async": true,
// [ts] "authors": ["Ilmari Heikkinen"],
// [ts] "notes": [{
// [ts] "name": "Kronos extensions registry",
// [ts] "href": "https://www.khronos.org/registry/webgl/extensions/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "RTC Peer Connection",
// [ts] "property": "peerconnection",
// [ts] "caniuse": "rtcpeerconnection",
// [ts] "tags": ["webrtc"],
// [ts] "authors": ["Ankur Oberoi"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/webrtc/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "RTC Data Channel",
// [ts] "property": "datachannel",
// [ts] "notes": [{
// [ts] "name": "HTML5 Rocks Tutorial",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/webrtc/datachannels/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "getUserMedia",
// [ts] "property": "getusermedia",
// [ts] "caniuse": "stream",
// [ts] "tags": ["webrtc"],
// [ts] "authors": ["Eric Bidelman", "Masataka Yakura"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://w3c.github.io/mediacapture-main/#dom-mediadevices-getusermedia"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "MediaStream Recording API",
// [ts] "property": "mediarecorder",
// [ts] "caniuse": "mediarecorder",
// [ts] "tags": ["mediarecorder", "media"],
// [ts] "authors": ["Onkar Dahale"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/MediaStream_Recording_API"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "WebSockets Support",
// [ts] "property": "websockets",
// [ts] "authors": ["Phread (@fearphage)", "Mike Sherov (@mikesherov)", "Burak Yigit Kaya (@BYK)"],
// [ts] "caniuse": "websockets",
// [ts] "tags": ["html5"],
// [ts] "knownBugs": ["This test will reject any old version of WebSockets even if it is not prefixed such as in Safari 5.1"],
// [ts] "notes": [{
// [ts] "name": "CLOSING State and Spec",
// [ts] "href": "https://www.w3.org/TR/websockets/#the-websocket-interface"
// [ts] }],
// [ts] "polyfills": [
// [ts] "sockjs",
// [ts] "socketio",
// [ts] "websocketjs",
// [ts] "atmosphere",
// [ts] "graceful-websocket",
// [ts] "portal",
// [ts] "datachannel"
// [ts] ]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Binary WebSockets",
// [ts] "property": "websocketsbinary",
// [ts] "tags": ["websockets"],
// [ts] "builderAliases": ["websockets_binary"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Base 64 encoding/decoding",
// [ts] "property": "atobbtoa",
// [ts] "builderAliases": ["atob-btoa"],
// [ts] "caniuse": "atob-btoa",
// [ts] "tags": ["atob", "base64", "WindowBase64", "btoa"],
// [ts] "authors": ["Christian Ulbrich"],
// [ts] "notes": [{
// [ts] "name": "WindowBase64",
// [ts] "href": "https://www.w3.org/TR/html5/webappapis.html#windowbase64"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/atob"
// [ts] }],
// [ts] "polyfills": ["base64js"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Framed window",
// [ts] "property": "framed",
// [ts] "tags": ["window"],
// [ts] "builderAliases": ["window_framed"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "matchMedia",
// [ts] "property": "matchmedia",
// [ts] "caniuse": "matchmedia",
// [ts] "tags": ["matchmedia"],
// [ts] "authors": ["Alberto Elias"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://drafts.csswg.org/cssom-view/#the-mediaquerylist-interface"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Window.matchMedia"
// [ts] }],
// [ts] "polyfills": ["matchmediajs"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "PushManager",
// [ts] "property": "pushmanager",
// [ts] "caniuse": "mdn-api_pushmanager",
// [ts] "authors": ["Dawid Kulpa (@dawidkulpa)"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/PushManager"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "ResizeObserver",
// [ts] "property": "resizeobserver",
// [ts] "caniuse": "resizeobserver",
// [ts] "tags": ["ResizeObserver"],
// [ts] "authors": ["Christian Andersson"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/resize-observer/"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/ResizeObserver"
// [ts] }, {
// [ts] "name": "Web.dev Article",
// [ts] "href": "https://web.dev/resize-observer/"
// [ts] }, {
// [ts] "name": "Digital Ocean tutorial",
// [ts] "href": "https://www.digitalocean.com/community/tutorials/js-resize-observer"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "worker type option test",
// [ts] "property": "workertypeoption",
// [ts] "caniuse":"mdn-api_worker_worker_ecmascript_modules",
// [ts] "tags": ["web worker type options", "web worker"],
// [ts] "builderAliases": ["worker_type_options"],
// [ts] "authors": ["Debadutta Panda"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Worker/Worker"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Workers from Blob URIs",
// [ts] "property": "blobworkers",
// [ts] "tags": ["performance", "workers"],
// [ts] "builderAliases": ["workers_blobworkers"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/workers/"
// [ts] }],
// [ts] "warnings": ["This test may output garbage to console."],
// [ts] "authors": ["Jussi Kalliokoski"],
// [ts] "async": true
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Workers from Data URIs",
// [ts] "property": "dataworkers",
// [ts] "tags": ["performance", "workers"],
// [ts] "builderAliases": ["workers_dataworkers"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/workers/"
// [ts] }],
// [ts] "warnings": ["This test may output garbage to console."],
// [ts] "authors": ["Jussi Kalliokoski"],
// [ts] "async": true
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Shared Workers",
// [ts] "property": "sharedworkers",
// [ts] "caniuse": "sharedworkers",
// [ts] "tags": ["performance", "workers"],
// [ts] "builderAliases": ["workers_sharedworkers"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/workers/"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Web Workers",
// [ts] "property": "webworkers",
// [ts] "caniuse": "webworkers",
// [ts] "tags": ["performance", "workers"],
// [ts] "notes": [{
// [ts] "name": "W3C Spec",
// [ts] "href": "https://www.w3.org/TR/workers/"
// [ts] }, {
// [ts] "name": "HTML5 Rocks Tutorial",
// [ts] "href": "https://www.html5rocks.com/en/tutorials/workers/basics/"
// [ts] }, {
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Using_web_workers"
// [ts] }],
// [ts] "polyfills": ["fakeworker", "html5shims"]
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "Transferables Objects",
// [ts] "property": "transferables",
// [ts] "tags": ["performance", "workers"],
// [ts] "builderAliases": ["transferables"],
// [ts] "notes": [{
// [ts] "name": "Transferable Objects: Lightning Fast!",
// [ts] "href": "https://developers.google.com/web/updates/2011/12/Transferable-Objects-Lightning-Fast"
// [ts] }],
// [ts] "async": true
// [ts] }
// [ts] !*/
// [ts] /*!
// [ts] {
// [ts] "name": "XDomainRequest",
// [ts] "property": "xdomainrequest",
// [ts] "tags": ["cors", "xdomainrequest", "ie9", "ie8"],
// [ts] "authors": ["Ivan Pan (@hypotenuse)"],
// [ts] "notes": [{
// [ts] "name": "MDN Docs",
// [ts] "href": "https://developer.mozilla.org/en-US/docs/Web/API/XDomainRequest"
// [ts] }]
// [ts] }
// [ts] !*/
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// [ts] } // [cap-migrate] auto-closed brace
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace
