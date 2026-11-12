// Geometry and text fitting shared by Radial, Hybrid, and Orbit.
// Pure calculations: no game APIs and no borrowed UI artwork.
(function(global) {
  "use strict";

  function clamp(value, low, high) {
    return Math.max(low, Math.min(high, value));
  }

  function estimateWidth(value, fontSize) {
    return Array.from(value).reduce(function(total, character) {
      var code = character.codePointAt(0);
      var wide = code > 0x2E80 || code >= 0x1F000;
      return total + fontSize * (wide ? 1 : /[ilI.,'!| ]/.test(character) ? .31 : .56);
    }, 0);
  }

  function textWidth(text, size, measure) {
    if (typeof measure === "function") {
      var result = Number(measure(text, size));
      if (isFinite(result) && result >= 0) return result;
    }
    return estimateWidth(text, size);
  }

  function wrapText(text, width, size, measure) {
    var words = String(text || "").trim().split(/\s+/);
    var lines = [];
    var current = "";
    function addWord(word) {
      var together = current ? current + " " + word : word;
      if (textWidth(together, size, measure) <= width) {
        current = together;
        return;
      }
      if (current) {
        lines.push(current);
        current = "";
      }
      // A long unbroken name must not overflow across adjacent wedges.
      var characters = Array.from(word);
      var part = "";
      for (var i = 0; i < characters.length; i += 1) {
        var candidate = part + characters[i];
        if (textWidth(candidate, size, measure) > width && part) {
          lines.push(part);
          part = characters[i];
        } else {
          part = candidate;
        }
      }
      current = part;
    }
    if (words[0]) words.forEach(addWord);
    if (current) lines.push(current);
    return lines.length ? lines : [""];
  }

  function truncateLine(line, width, size, measure) {
    var suffix = "…";
    var characters = Array.from(line);
    while (characters.length && textWidth(characters.join("") + suffix, size, measure) > width)
      characters.pop();
    return characters.join("").trimEnd() + suffix;
  }

  function fitWheelLabel(value, width, height, settings) {
    settings = settings || {};
    var text = String(value || "").trim();
    width = Math.max(16, Number(width) || 16);
    height = Math.max(12, Number(height) || 12);
    var base = clamp(Number(settings.baseSize) || 13.5, 9, 20);
    var minimum = clamp(Number(settings.minSize) || 10, 9, base);
    var measure = settings.measure;
    var lines = [text];
    var size = base;
    var maxLines = 3;

    while (size >= minimum - .01) {
      maxLines = clamp(Math.floor(height / (size * 1.16)), 1, 3);
      lines = wrapText(text, width, size, measure);
      if (lines.length <= maxLines &&
          lines.every(function(line) { return textWidth(line, size, measure) <= width; }))
        return { lines: lines, fontSize: size, truncated: false };
      size -= .5;
    }
    size = minimum;
    maxLines = clamp(Math.floor(height / (size * 1.16)), 1, 3);
    lines = wrapText(text, width, size, measure);
    var truncated = lines.length > maxLines;
    if (truncated) {
      var visible = lines.slice(0, maxLines);
      // Keep as much of the final readable line as fits, not a blind fixed
      // character-count prefix.
      visible[maxLines - 1] = truncateLine(visible[maxLines - 1], width, size, measure);
      lines = visible;
    }
    lines = lines.map(function(line) {
      return textWidth(line, size, measure) <= width ? line : truncateLine(line, width, size, measure);
    });
    return { lines: lines, fontSize: size, truncated: truncated };
  }

  function wheelArcWidth(innerRadius, outerRadius, spanDegrees) {
    var inner = Math.max(0, Number(innerRadius) || 0);
    var outer = Math.max(inner, Number(outerRadius) || 0);
    var span = clamp(Math.abs(Number(spanDegrees) || 0), 0, 180);
    var conservativeRadius = inner + .24 * (outer - inner);
    // The chord is narrower than the arc length. Fitting to the chord
    // leaves safe space at the boundaries of narrow curved segments.
    return Math.max(16, 2 * conservativeRadius * Math.sin(span * Math.PI / 360) - 14);
  }

  function wrap(index, count) {
    return (index % count + count) % count;
  }

  function orbitCategoryWindow(count, selectedIndex, limit) {
    count = Math.max(0, Math.floor(Number(count) || 0));
    if (!count) return [];
    limit = clamp(Math.floor(Number(limit) || 8), 1, count);
    if (count <= limit) return Array.from({ length: count }, function(_, i) { return i; });
    var start = wrap(Math.floor(Number(selectedIndex) || 0) - Math.floor(limit / 2), count);
    return Array.from({ length: limit }, function(_, i) { return wrap(start + i, count); });
  }

  function orbitOptionWindow(count, selectedIndex, radius) {
    count = Math.max(0, Math.floor(Number(count) || 0));
    if (!count) return [];
    selectedIndex = wrap(Math.floor(Number(selectedIndex) || 0), count);
    radius = clamp(Math.floor(Number(radius) || 3), 1, 5);
    var rows = [];
    if (count >= radius * 2 + 1) {
      for (var delta = -radius; delta <= radius; delta += 1)
        rows.push({ index: wrap(selectedIndex + delta, count), offset: delta });
      return rows;
    }
    for (var i = 0; i < count; i += 1) {
      var distance = wrap(i - selectedIndex, count);
      if (distance > count / 2) distance -= count;
      rows.push({ index: i, offset: distance });
    }
    return rows.sort(function(a, b) { return a.offset - b.offset; });
  }

  function orbitRowPosition(offset) {
    offset = clamp(Math.round(Number(offset) || 0), -3, 3);
    var distance = Math.abs(offset);
    return {
      top: 50 + offset * 12.5,
      left: 29 + (3 - distance) * 15,
      scale: 1.11 - distance * .065,
      opacity: 1 - distance * .12
    };
  }

  global.K2040QuickMenuGeometry = {
    fitWheelLabel: fitWheelLabel,
    wheelArcWidth: wheelArcWidth,
    orbitOptionWindow: orbitOptionWindow,
    orbitCategoryWindow: orbitCategoryWindow,
    orbitRowPosition: orbitRowPosition
  };
})(window);
