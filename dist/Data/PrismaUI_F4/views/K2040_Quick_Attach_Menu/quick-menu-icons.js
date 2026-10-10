// Optional presentation-only PNG/WebP icon resources supplied by a trusted native bridge.
// No source paths, SWFs, SVG data, or network URLs are accepted.
(function(global) {
  "use strict";

  var CLASS_NAME = /^[a-z][a-z0-9_]*\.[a-z][a-z0-9_]*$/;
  var MAX_URI_LENGTH = 262144;
  var PNG_PREFIX = "data:image/png;base64,";
  var WEBP_PREFIX = "data:image/webp;base64,";

  function sourceFor(iconAssets, iconClass) {
    if (!iconAssets || typeof iconAssets !== "object" ||
        typeof iconClass !== "string" || !CLASS_NAME.test(iconClass) ||
        !Object.prototype.hasOwnProperty.call(iconAssets, iconClass)) return null;

    var value = iconAssets[iconClass];
    if (typeof value !== "string" || value.length > MAX_URI_LENGTH) return null;
    var prefix = value.indexOf(PNG_PREFIX) === 0 ? PNG_PREFIX :
      (value.indexOf(WEBP_PREFIX) === 0 ? WEBP_PREFIX : null);
    if (!prefix) return null;
    var payload = value.slice(prefix.length);
    if (!payload || payload.length % 4 !== 0 || !/^[A-Za-z0-9+/]*={0,2}$/.test(payload)) return null;
    // Reject arbitrary data disguised as images. Decoder validation remains browser-owned.
    if (prefix === PNG_PREFIX && payload.indexOf("iVBORw0KGgo") !== 0) return null;
    if (prefix === WEBP_PREFIX && payload.indexOf("UklGR") !== 0) return null;
    return value;
  }

  function createIcon(iconAssets, iconClass) {
    var source = sourceFor(iconAssets, iconClass);
    if (!source) return null;

    var shell = document.createElement("span");
    shell.className = "qm-asset-icon";
    shell.setAttribute("aria-hidden", "true");
    if ("webkitMaskImage" in shell.style || "maskImage" in shell.style) {
      shell.className += " qm-asset-icon-mask";
      shell.style.webkitMaskImage = 'url("' + source + '")';
      shell.style.maskImage = 'url("' + source + '")';
    } else {
      var fallback = document.createElement("img");
      fallback.className = "qm-asset-icon-img";
      fallback.alt = "";
      fallback.src = source;
      shell.appendChild(fallback);
    }
    return shell;
  }

  global.K2040IconResources = { sourceFor: sourceFor, createIcon: createIcon };
})(window);
