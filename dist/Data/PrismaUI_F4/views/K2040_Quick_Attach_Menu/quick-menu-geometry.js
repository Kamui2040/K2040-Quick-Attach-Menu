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


  function fitReadableWheelLabel(value, width, height, settings) {
    settings = settings || {};
    var text = String(value || "").trim();
    width = Math.max(20, Number(width) || 20);
    height = Math.max(15, Number(height) || 15);
    var base = clamp(Number(settings.baseSize) || 14, 11, 26);
    var minimum = clamp(Number(settings.minSize) || 12, 11, base);
    var measure = settings.measure;
    var words = text.split(/\s+/);
    var size, i, left, right, result, widest;
    // Prefer an unbroken, readable label. At most two complete-word lines.
    for (size = base; size >= minimum - .01; size -= .5) {
      if (size * 1.25 <= height && textWidth(text, size, measure) <= width)
        return { lines:[text], fontSize:size, truncated:false };
      if (size * 2.45 > height || words.length < 2) continue;
      var best = null;
      for (i = 1; i < words.length; i += 1) {
        left = words.slice(0, i).join(" ");
        right = words.slice(i).join(" ");
        widest = Math.max(textWidth(left, size, measure), textWidth(right, size, measure));
        if (widest <= width && (!best || widest < best.widest))
          best = { widest:widest, lines:[left,right], fontSize:size, truncated:false };
      }
      if (best) return best;
    }
    // Do not reduce font size indefinitely or stack single letters vertically.
    // Truncation is a fallback for a physically too narrow slice only.
    size = minimum;
    result = { lines:[truncateLine(text, width, size, measure)], fontSize:size, truncated:true };
    if (words.length > 1 && height >= size * 2.45) {
      var shortest = null;
      for (i = 1; i < words.length; i += 1) {
        left = words.slice(0, i).join(" ");
        right = words.slice(i).join(" ");
        widest = Math.max(textWidth(left, size, measure), textWidth(right, size, measure));
        if (!shortest || widest < shortest.widest)
          shortest = { widest:widest, lines:[left,right] };
      }
      if (shortest) result.lines = shortest.lines.map(function(line) {
        return textWidth(line, size, measure) > width ?
          truncateLine(line, width, size, measure) : line;
      });
    }
    return result;
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


  function clockCategoryAngle(index, count) {
    // The option fan and its parent wedge share one angular origin.
    // Keep the midpoint independent of the visual category border inset.
    count = Math.max(1, Math.floor(Number(count) || 1));
    return -90 + (Number(index) + .5) * 360 / count;
  }

  function clockReadableLabelAngle(angle) {
    // Radial labels on the left side must not render upside-down.
    var normalized = ((Number(angle) % 360) + 360) % 360;
    return normalized > 90 && normalized < 270 ? angle + 180 : angle;
  }

  function clockOptionSlice(offset, lineCount, centerAngle) {
    // Offsets may now extend around the full clock, not just -2 through +2.
    offset = Math.round(Number(offset) || 0);
    var angle = Number.isFinite(Number(centerAngle)) && centerAngle !== undefined ?
      Number(centerAngle) : offset * 15;
    var halfSweep = Number(lineCount) === 1 ? 4.25 : 6.75;
    return { angle: angle, start: angle - halfSweep, end: angle + halfSweep,
      innerRadius: 234, outerRadius: 462 };
  }

  function clockOptionWindow(lineCounts, selectedIndex) {
    // Show all attachments that physically fit in one revolution. The
    // selected attachment remains centered on the active parent category;
    // its neighbours expand around either side until the available 360°
    // circumference is occupied. The quarter-degree seam prevents the
    // first and last edges from ever drawing on top of each other.
    var count = (lineCounts || []).length;
    if (!count) return [];
    selectedIndex = wrap(Math.floor(Number(selectedIndex) || 0), count);
    var gap = .25;
    var circumference = 360 - gap;
    function span(index) {
      return Number(lineCounts[index]) === 1 ? 8.5 : 13.5;
    }
    var used = {};
    used[selectedIndex] = true;
    var selectedSpan = span(selectedIndex);
    var occupied = selectedSpan;
    var leftExtent = selectedSpan / 2;
    var rightExtent = selectedSpan / 2;
    var leftOffset = 0;
    var rightOffset = 0;
    var rows = [{ index:selectedIndex, offset:0 }];
    while (rows.length < count) {
      // Favor the less-occupied side so the visible fan stays balanced,
      // but try the other side if the next entry would not fit.
      var directions = leftExtent <= rightExtent ? [-1, 1] : [1, -1];
      var added = false;
      for (var side = 0; side < directions.length; side += 1) {
        var direction = directions[side];
        var offset = direction < 0 ? leftOffset - 1 : rightOffset + 1;
        var index = wrap(selectedIndex + offset, count);
        if (used[index]) continue;
        var nextSpan = span(index);
        if (occupied + nextSpan + gap > circumference + 1e-8) continue;
        rows.push({ index:index, offset:offset });
        used[index] = true;
        occupied += nextSpan + gap;
        if (direction < 0) {
          leftOffset = offset;
          leftExtent += nextSpan + gap;
        } else {
          rightOffset = offset;
          rightExtent += nextSpan + gap;
        }
        added = true;
        break;
      }
      if (!added) break;
    }
    return rows.sort(function(a, b) { return a.offset - b.offset; });
  }

  function clockPackedOptionSlices(rows, categoryAngle) {
    // Each group of child slices fans out from the center of its selected
    // parent category, instead of always appearing on the right (0°).
    // Offsets remain relative to the selected child, so mixed one/two-line
    // spacing and keyboard/controller navigation do not change.
    var anchor = Number.isFinite(Number(categoryAngle)) &&
      categoryAngle !== undefined ? Number(categoryAngle) : 0;
    // Thin sectors must stay close together even with mixed one/two-line
    // labels. A fixed 15° pitch left broad triangular holes between them.
    // Navigation still uses row.offset; only visual angles are packed.
    var sorted = (rows || []).map(function(row, index) {
      return { offset: row.offset, lineCount: row.lineCount, index: index,
        span: Number(row.lineCount) === 1 ? 8.5 : 13.5 };
    }).sort(function(a, b) { return a.offset - b.offset; });
    if (!sorted.length) return [];
    var center = sorted.findIndex(function(row) { return row.offset === 0; });
    if (center < 0) center = Math.floor(sorted.length / 2);
    var gap = .25;
    sorted[center].angle = anchor;
    var i;
    for (i = center + 1; i < sorted.length; i += 1) {
      sorted[i].angle = sorted[i - 1].angle +
        (sorted[i - 1].span + sorted[i].span) / 2 + gap;
    }
    for (i = center - 1; i >= 0; i -= 1) {
      sorted[i].angle = sorted[i + 1].angle -
        (sorted[i + 1].span + sorted[i].span) / 2 - gap;
    }
    return sorted.sort(function(a, b) { return a.index - b.index; }).map(function(row) {
      return clockOptionSlice(row.offset, row.lineCount, row.angle);
    });
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
    fitReadableWheelLabel: fitReadableWheelLabel,
    fitWheelLabel: fitWheelLabel,
    wheelArcWidth: wheelArcWidth,
    orbitOptionWindow: orbitOptionWindow,
    orbitCategoryWindow: orbitCategoryWindow,
    orbitRowPosition: orbitRowPosition,
    clockCategoryAngle: clockCategoryAngle,
    clockReadableLabelAngle: clockReadableLabelAngle,
    clockOptionSlice: clockOptionSlice,
    clockOptionWindow: clockOptionWindow,
    clockPackedOptionSlices: clockPackedOptionSlices,
    shortenCategoryWords: shortenCategoryWords,
    fitClockLabel: fitClockLabel
  };
})(window);
