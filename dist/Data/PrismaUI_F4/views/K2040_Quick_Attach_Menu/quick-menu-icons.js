// Presentation-only icon resources. The native bridge reads separately installed
// FIS vectors in memory; no FIS file or derived artwork ships with this mod.
(function(global) {
  "use strict";

  var SVG_NS = "http://www.w3.org/2000/svg";
  var CLASS_NAME = /^[a-z][a-z0-9_]*\.[a-z][a-z0-9_]*$/;
  var VECTOR_PATH = /^[MLQmlq0-9\- ]+$/;
  var MAX_URI_LENGTH = 262144;
  var MAX_PATH_LENGTH = 32768;
  var PNG_PREFIX = "data:image/png;base64,";
  var WEBP_PREFIX = "data:image/webp;base64,";

  function validClass(iconClass) {
    return typeof iconClass === "string" && CLASS_NAME.test(iconClass);
  }

  function hasClass(collection, iconClass) {
    return collection && typeof collection === "object" && validClass(iconClass) &&
      Object.prototype.hasOwnProperty.call(collection, iconClass);
  }

  function sourceFor(iconAssets, iconClass) {
    if (!hasClass(iconAssets, iconClass)) return null;
    var value = iconAssets[iconClass];
    if (typeof value !== "string" || value.length > MAX_URI_LENGTH) return null;
    var prefix = value.indexOf(PNG_PREFIX) === 0 ? PNG_PREFIX :
      (value.indexOf(WEBP_PREFIX) === 0 ? WEBP_PREFIX : null);
    if (!prefix) return null;
    var payload = value.slice(prefix.length);
    if (!payload || payload.length % 4 !== 0 || !/^[A-Za-z0-9+/]*={0,2}$/.test(payload)) return null;
    if (prefix === PNG_PREFIX && payload.indexOf("iVBORw0KGgo") !== 0) return null;
    if (prefix === WEBP_PREFIX && payload.indexOf("UklGR") !== 0) return null;
    return value;
  }

  function safeNumber(value) {
    return typeof value === "number" && isFinite(value) && Math.abs(value) <= 10000000;
  }

  function vectorFor(iconVectors, iconClass) {
    if (!hasClass(iconVectors, iconClass)) return null;
    var icon = iconVectors[iconClass];
    if (!icon || typeof icon !== "object" || !Array.isArray(icon.bounds) ||
        icon.bounds.length !== 4 || !Array.isArray(icon.parts) ||
        !icon.parts.length || icon.parts.length > 16) return null;
    for (var i = 0; i < icon.bounds.length; i += 1) {
      if (!safeNumber(icon.bounds[i])) return null;
    }
    if (icon.bounds[2] <= 0 || icon.bounds[3] <= 0) return null;
    for (var j = 0; j < icon.parts.length; j += 1) {
      var part = icon.parts[j];
      if (!part || typeof part.d !== "string" || !part.d.length ||
          part.d.length > MAX_PATH_LENGTH || !VECTOR_PATH.test(part.d) ||
          !Array.isArray(part.matrix) || part.matrix.length !== 6) return null;
      for (var k = 0; k < part.matrix.length; k += 1) {
        if (!safeNumber(part.matrix[k])) return null;
      }
    }
    return icon;
  }

  function createIcon(iconAssets, iconClass, iconVectors) {
    var source = sourceFor(iconAssets, iconClass);
    var vector = source ? null : vectorFor(iconVectors, iconClass);
    if (!source && !vector) return null;

    var shell = document.createElement("span");
    shell.className = "qm-asset-icon";
    shell.setAttribute("aria-hidden", "true");
    if (source) {
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
    } else {
      var svg = document.createElementNS(SVG_NS, "svg");
      svg.className = "qm-asset-icon-svg";
      svg.setAttribute("viewBox", vector.bounds.join(" "));
      svg.setAttribute("preserveAspectRatio", "xMidYMid meet");
      for (var i = 0; i < vector.parts.length; i += 1) {
        var part = vector.parts[i];
        var group = document.createElementNS(SVG_NS, "g");
        group.setAttribute("transform", "matrix(" + part.matrix.join(" ") + ")");
        var path = document.createElementNS(SVG_NS, "path");
        path.setAttribute("d", part.d);
        path.setAttribute("fill", "currentColor");
        path.setAttribute("fill-rule", "evenodd");
        group.appendChild(path);
        svg.appendChild(group);
      }
      shell.appendChild(svg);
    }
    return shell;
  }

  global.K2040IconResources = {
    sourceFor: sourceFor,
    vectorFor: vectorFor,
    createIcon: createIcon
  };
})(window);
