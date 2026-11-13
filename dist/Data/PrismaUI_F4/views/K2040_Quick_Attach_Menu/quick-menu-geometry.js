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
      top: 50 + offset * 16.5,
      left: 45 + (2 - distance) * 12,
      scale: 1.02 - distance * .055
    };
  }


  function clockOptionSlice(offset) {
    offset = clamp(Math.round(Number(offset) || 0), -2, 2);
    var angle = offset * 20;
    // Stay close to the category ring; the previous 270px depth made the
    // individual sectors look like long, oversized blades.
    return { angle: angle, start: angle - 9.8, end: angle + 9.8,
      innerRadius: 234, outerRadius: 402 };
  }

  function shortenCategoryWords(label, category, enabled) {
    var original = String(label || "").trim();
    var categoryName = String(category || "").trim();
    if (!enabled || !original || !categoryName) return original;
    // Match the category's full phrase, plus singular/plural versions of its
    // final word. This handles Sights/Sight, Magazine/Magazines and compounds
    // like Barrel Attachments/Barrel Attachment without substring removal.
    var words = categoryName.split(/\s+/);
    var last = words.pop();
    var counterpart;
    if (/ies$/i.test(last) && last.length > 4) counterpart = last.slice(0, -3) + "y";
    else if (/(?:ches|shes|sses|xes|zes)$/i.test(last)) counterpart = last.slice(0, -2);
    else if (/s$/i.test(last) && !/ss$/i.test(last)) counterpart = last.slice(0, -1);
    else if (/[bcdfghjklmnpqrstvwxz]y$/i.test(last)) counterpart = last.slice(0, -1) + "ies";
    else if (/(?:s|x|z|ch|sh)$/i.test(last)) counterpart = last + "es";
    else counterpart = last + "s";
    var phrases = [categoryName, words.concat(counterpart).join(" ")];
    var escaped = phrases.sort(function(a, b) { return b.length - a.length; }).map(function(phrase) {
      return Array.from(phrase).map(function(character) {
        return /[\p{L}\p{N}\s]/u.test(character) ? character : "\\" + character;
      }).join("").replace(/\s+/g, "\\s+");
    }).join("|");
    var pattern = new RegExp("(^|[^\\p{L}\\p{N}])(?:" + escaped + ")(?=$|[^\\p{L}\\p{N}])", "giu");
    var shortened = original.replace(pattern, "$1")
      .replace(/\s+/g, " ").replace(/^[\s_/:;\-,]+|[\s_/:;\-,]+$/g, "").trim();
    // Reset options such as "No Material" should remain meaningful rather
    // than being reduced to an isolated "No" or "None".
    if (/^(?:no|none|without)$/iu.test(shortened)) return original;
    return shortened || original;
  }

  function fitClockLabel(value, width, options) {
    options = options || {};
    var label = String(value || "").trim();
    width = Math.max(42, Number(width) || 200);
    var maximum = clamp(Number(options.baseSize) || 15, 11, 18);
    var minimum = clamp(Number(options.minSize) || 11, 10, maximum);
    var measure = options.measure;
    var size;
    for (size = maximum; size >= minimum - .01; size -= .5) {
      if (textWidth(label, size, measure) <= width) return { lines:[label], fontSize:size };
    }
    var words = label.split(/\s+/);
    for (size = maximum; size >= minimum - .01; size -= .5) {
      var best = null;
      for (var i = 1; i < words.length; i += 1) {
        var left = words.slice(0, i).join(" ");
        var right = words.slice(i).join(" ");
        var widest = Math.max(textWidth(left, size, measure), textWidth(right, size, measure));
        if (widest <= width && (!best || widest < best.widest))
          best = { lines:[left,right], fontSize:size, widest:widest };
      }
      if (best) return { lines:best.lines, fontSize:best.fontSize };
    }
    var halfway = Math.max(1, Math.ceil(words.length / 2));
    var first = words.slice(0, halfway).join(" ");
    var last = words.slice(halfway).join(" ");
    if (!last) return { lines:[truncateLine(label, width, minimum, measure)], fontSize:minimum };
    return { lines:[truncateLine(first, width, minimum, measure),
      truncateLine(last, width, minimum, measure)], fontSize:minimum };
  }

  global.K2040QuickMenuGeometry = {
    fitWheelLabel: fitWheelLabel,
    wheelArcWidth: wheelArcWidth,
    orbitOptionWindow: orbitOptionWindow,
    orbitCategoryWindow: orbitCategoryWindow,
    orbitRowPosition: orbitRowPosition,
    clockOptionSlice: clockOptionSlice,
    shortenCategoryWords: shortenCategoryWords,
    fitClockLabel: fitClockLabel
  };
})(window);
