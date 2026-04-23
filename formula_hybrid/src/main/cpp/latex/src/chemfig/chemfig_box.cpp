#include "chemfig_box.h"
#include "fonts/fonts.h"
#include <cmath>
#include <algorithm>

namespace tex {

namespace {
    constexpr float PADDING = 0.3f;
    constexpr float BOND_SCALE = 1.0f;
    constexpr float TEXT_BOND_GAP = 0.12f;
    constexpr float BOND_LINE_WIDTH = 0.055f;
    constexpr float SUBSCRIPT_SCALE = 0.7f;
    constexpr float SUPERSCRIPT_RISE = 0.35f;
    constexpr float EPSILON = 0.0001f;
    constexpr float CHEMFIG_PI = 3.14159265358979f;

    float computeShortening(float dirX, float dirY, float halfW, float halfH) {
        float tx = (std::abs(dirX) > EPSILON) ? halfW / std::abs(dirX) : 1e6f;
        float ty = (std::abs(dirY) > EPSILON) ? halfH / std::abs(dirY) : 1e6f;
        return std::min(tx, ty) + TEXT_BOND_GAP;
    }

    TextSegment buildSegment(const std::wstring& text, TextSegmentType type,
                             const sptr<Font>& font, float textScale) {
        TextSegment seg;
        seg.text = text;
        seg.type = type;
        seg.layout = TextLayout::create(text, font);
        Rect bounds;
        seg.layout->getBounds(bounds);
        float scale = (type != SEG_NORMAL) ? SUBSCRIPT_SCALE : 1.0f;
        seg.width = (bounds.w + bounds.x + 0.4f) * textScale * scale;
        seg.height = bounds.h * textScale * scale;
        seg.ascent = -bounds.y * textScale * scale;
        return seg;
    }

    std::vector<TextSegment> splitTextToSegments(const std::wstring& text,
                                                  const sptr<Font>& font, float textScale) {
        std::vector<TextSegment> segments;
        std::wstring current;
        TextSegmentType currentType = SEG_NORMAL;

        auto flushCurrent = [&]() {
            if (current.empty()) return;
            segments.push_back(buildSegment(current, currentType, font, textScale));
            current.clear();
        };

        for (wchar_t ch : text) {
            if (ch == L'|') {
                flushCurrent();
                currentType = SEG_NORMAL;
            } else if (ch == L'_' || ch == L'^') {
                flushCurrent();
                currentType = (ch == L'_') ? SEG_SUBSCRIPT : SEG_SUPERSCRIPT;
            } else {
                current += ch;
            }
        }
        flushCurrent();
        return segments;
    }

    bool hasSubOrSuper(const std::wstring& text) {
        return text.find_first_of(L"_^|") != std::wstring::npos;
    }

    float computeSegmentsTotalWidth(const std::vector<TextSegment>& segments) {
        float total = 0;
        for (size_t i = 0; i < segments.size(); i++) {
            const auto& seg = segments[i];
            bool isScriptPair = false;
            if (i + 1 < segments.size()) {
                TextSegmentType nextType = segments[i + 1].type;
                if ((seg.type == SEG_SUPERSCRIPT && nextType == SEG_SUBSCRIPT) ||
                    (seg.type == SEG_SUBSCRIPT && nextType == SEG_SUPERSCRIPT)) {
                    isScriptPair = true;
                    total += std::max(seg.width, segments[i + 1].width);
                    i++;
                }
            }
            if (!isScriptPair) {
                total += seg.width;
            }
        }
        return total;
    }

    struct TextMetrics {
        float halfW;
        float halfH;
        float normalAscent;
        float normalHeight;
    };

    TextMetrics computeTextMetrics(const std::vector<TextSegment>& segments) {
        TextMetrics tm = {0, 0, 0, 0};
        float totalW = computeSegmentsTotalWidth(segments);
        tm.halfW = totalW / 2;

        for (const auto& seg : segments) {
            if (seg.type == SEG_NORMAL) {
                tm.normalAscent = seg.ascent;
                tm.normalHeight = seg.height;
                break;
            }
        }

        float baselineOffset = -tm.normalHeight / 2 + tm.normalAscent;
        float topExtent = baselineOffset - tm.normalAscent;
        float bottomExtent = baselineOffset + (tm.normalHeight - tm.normalAscent);

        for (const auto& seg : segments) {
            if (seg.type == SEG_SUBSCRIPT) {
                float subBaseline = baselineOffset - tm.normalAscent / 2 + seg.ascent;
                float subBottom = subBaseline + (seg.height - seg.ascent);
                bottomExtent = std::max(bottomExtent, subBottom);
            } else if (seg.type == SEG_SUPERSCRIPT) {
                float subTop = baselineOffset - SUPERSCRIPT_RISE * tm.normalHeight - seg.ascent;
                topExtent = std::min(topExtent, subTop);
            }
        }
        tm.halfH = (bottomExtent - topExtent) / 2;
        return tm;
    }

    TextMetrics computeSimpleTextMetrics(const std::wstring& text,
                                          const sptr<Font>& font, float textScale) {
        auto layout = TextLayout::create(text, font);
        Rect bounds;
        layout->getBounds(bounds);
        float textW = (bounds.w + bounds.x + 0.4f) * textScale;
        float textH = bounds.h * textScale;
        return {textW / 2, textH / 2, -bounds.y * textScale, textH};
    }
}

ChemfigBox::ChemfigBox(const Molecule& mol, color c, const sptr<Font>& font, float sizeFactor)
    : _molecule(mol), _color(c), _font(font), _sizeFactor(sizeFactor) {
    calculateLayout();
}

void ChemfigBox::calculateLayout() {
    if (_molecule.atoms.empty()) {
        _width = PADDING * 2;
        _height = PADDING;
        _depth = PADDING;
        _foreground = _color;
        _textBoundsMinX = 0;
        _textBoundsMinY = 0;
        return;
    }

    _molecule.calculateBounds();
    float textScale = 0.1f * _sizeFactor;

    float minX = _molecule.minX;
    float maxX = _molecule.maxX;
    float minY = _molecule.minY;
    float maxY = _molecule.maxY;

    for (const auto& atom : _molecule.atoms) {
        if (atom.text.empty()) continue;

        TextMetrics tm;
        if (hasSubOrSuper(atom.text)) {
            auto segments = splitTextToSegments(atom.text, _font, textScale);
            tm = computeTextMetrics(segments);
        } else {
            tm = computeSimpleTextMetrics(atom.text, _font, textScale);
        }

        minX = std::min(minX, atom.position.x - tm.halfW);
        maxX = std::max(maxX, atom.position.x + tm.halfW);
        minY = std::min(minY, atom.position.y - tm.halfH);
        maxY = std::max(maxY, atom.position.y + tm.halfH);
    }

    float w = (maxX - minX) * BOND_SCALE;
    float h = (maxY - minY) * BOND_SCALE;
    _width = w + 2 * PADDING;
    _height = h / 2 + PADDING;
    _depth = h / 2 + PADDING;
    _foreground = _color;
    _textBoundsMinX = minX;
    _textBoundsMinY = minY;
}

void ChemfigBox::buildAtomLayouts(float offsetX, float offsetY, float scale) {
    _atomLayouts.clear();
    _atomTextBounds.clear();
    float textScale = 0.1f * _sizeFactor;

    for (int i = 0; i < static_cast<int>(_molecule.atoms.size()); i++) {
        const auto& atom = _molecule.atoms[i];
        if (atom.text.empty()) continue;

        AtomLayout layout;
        layout.text = atom.text;
        layout.x = atom.position.x * scale + offsetX;
        layout.y = atom.position.y * scale + offsetY;
        layout.normalAscent = 0;
        layout.normalHeight = 0;

        TextMetrics tm;
        if (hasSubOrSuper(atom.text)) {
            layout.segments = splitTextToSegments(atom.text, _font, textScale);
            tm = computeTextMetrics(layout.segments);
            layout.textOffsetX = -tm.halfW;
            layout.normalAscent = tm.normalAscent;
            layout.normalHeight = tm.normalHeight;
            layout.textOffsetY = -tm.normalHeight / 2 + tm.normalAscent;
            layout.layout = TextLayout::create(L"", _font);
        } else {
            layout.layout = TextLayout::create(atom.text, _font);
            Rect bounds;
            layout.layout->getBounds(bounds);
            float textW = (bounds.w + bounds.x + 0.4f) * textScale;
            float textH = bounds.h * textScale;
            float textY = -bounds.y * textScale;
            layout.textOffsetX = -textW / 2;
            layout.textOffsetY = -textH / 2 + textY;
            tm.halfW = textW / 2;
            tm.halfH = textH / 2;
        }

        _atomTextBounds[i] = {tm.halfW, tm.halfH};
        _atomLayouts.push_back(layout);
    }
}

void ChemfigBox::drawMolecule(Graphics2D& g2, float x, float y) {
    if (_molecule.atoms.empty()) return;

    ChemPoint ringCenter = _molecule.center();
    bool hasRing = !_molecule.rings.empty();
    float scale = BOND_SCALE;
    float offsetX = x - _textBoundsMinX * scale + PADDING * scale;
    float offsetY = y - _textBoundsMinY * scale - (_molecule.maxY - _textBoundsMinY) * scale / 2;

    for (const auto& bond : _molecule.bonds) {
        if (bond.fromAtom < 0 || bond.fromAtom >= static_cast<int>(_molecule.atoms.size())) continue;
        if (bond.toAtom < 0 || bond.toAtom >= static_cast<int>(_molecule.atoms.size())) continue;

        const AtomNode& from = _molecule.atoms[bond.fromAtom];
        const AtomNode& to = _molecule.atoms[bond.toAtom];

        float fromX = from.position.x * scale + offsetX;
        float fromY = from.position.y * scale + offsetY;
        float toX = to.position.x * scale + offsetX;
        float toY = to.position.y * scale + offsetY;

        float dx = toX - fromX;
        float dy = toY - fromY;
        float bondLen = std::sqrt(dx * dx + dy * dy);

        if (bondLen > EPSILON) {
            float dirX = dx / bondLen;
            float dirY = dy / bondLen;

            auto itFrom = _atomTextBounds.find(bond.fromAtom);
            if (itFrom != _atomTextBounds.end()) {
                float shorten = computeShortening(dirX, dirY, itFrom->second.halfW, itFrom->second.halfH);
                fromX += dirX * shorten;
                fromY += dirY * shorten;
            }

            auto itTo = _atomTextBounds.find(bond.toAtom);
            if (itTo != _atomTextBounds.end()) {
                float shorten = computeShortening(-dirX, -dirY, itTo->second.halfW, itTo->second.halfH);
                toX -= dirX * shorten;
                toY -= dirY * shorten;
            }
        }

        ChemPoint bondRingCenter = ringCenter;
        bool bondInRing = hasRing;
        if (bond.ringIndex >= 0 && bond.ringIndex < static_cast<int>(_molecule.rings.size())) {
            bondRingCenter = _molecule.rings[bond.ringIndex].center;
            bondInRing = true;
        }
        ChemPoint scaledCenter(bondRingCenter.x * scale + offsetX, bondRingCenter.y * scale + offsetY);
        BondRenderer::drawBond(g2, bond.type, ChemPoint(fromX, fromY), ChemPoint(toX, toY),
                               scale, bond.params, scaledCenter, bondInRing);
    }

    for (const auto& layout : _atomLayouts) {
        float baseScale = 0.1f * _sizeFactor;

        if (!layout.segments.empty()) {
            float baseX = layout.x + layout.textOffsetX;
            float baselineY = layout.y + layout.textOffsetY;
            float xCursor = 0;
            float prevNormalAscent = layout.normalAscent;

            for (size_t i = 0; i < layout.segments.size(); i++) {
                const auto& seg = layout.segments[i];
                float segX = baseX + xCursor;
                float segY = baselineY;

                if (seg.type == SEG_SUBSCRIPT) {
                    segY = baselineY - prevNormalAscent / 2 + seg.ascent;
                } else if (seg.type == SEG_SUPERSCRIPT) {
                    segY = baselineY - SUPERSCRIPT_RISE * layout.normalHeight;
                }

                float s = baseScale * ((seg.type != SEG_NORMAL) ? SUBSCRIPT_SCALE : 1.0f);
                g2.translate(segX, segY);
                g2.scale(s, s);
                seg.layout->draw(g2, 0, 0);
                g2.scale(1.f / s, 1.f / s);
                g2.translate(-segX, -segY);

                if (seg.type == SEG_NORMAL) {
                    prevNormalAscent = seg.ascent;
                }

                bool isScriptPair = false;
                if (i + 1 < layout.segments.size()) {
                    TextSegmentType nextType = layout.segments[i + 1].type;
                    if ((seg.type == SEG_SUPERSCRIPT && nextType == SEG_SUBSCRIPT) ||
                        (seg.type == SEG_SUBSCRIPT && nextType == SEG_SUPERSCRIPT)) {
                        isScriptPair = true;
                    }
                }

                if (!isScriptPair) {
                    xCursor += seg.width;
                }
            }
        } else {
            float drawX = layout.x + layout.textOffsetX;
            float drawY = layout.y + layout.textOffsetY;
            g2.translate(drawX, drawY);
            g2.scale(baseScale, baseScale);
            layout.layout->draw(g2, 0, 0);
            g2.scale(1.f / baseScale, 1.f / baseScale);
            g2.translate(-drawX, -drawY);
        }
    }

    for (const auto& ring : _molecule.rings) {
        if (ring.hasInnerCircle) {
            float cx = ring.center.x * scale + offsetX;
            float cy = ring.center.y * scale + offsetY;
            float apothem = ring.radius * scale * std::cos(CHEMFIG_PI / ring.sides);
            float r = apothem * 0.75f;
            g2.drawEllipse(cx - r, cy + r, r * 2, r * 2, r, r, BOND_LINE_WIDTH * scale, 0.0f);
        }
    }
}

void ChemfigBox::draw(Graphics2D& g2, float x, float y) {
    if (_molecule.atoms.empty()) return;

    color oldColor = g2.getColor();
    if (!istrans(_color)) g2.setColor(_color);

    const Stroke& oldStroke = g2.getStroke();
    g2.setStroke(Stroke(BOND_LINE_WIDTH, CAP_ROUND, JOIN_ROUND));

    float scale = BOND_SCALE;
    float offsetX = x - _textBoundsMinX * scale + PADDING * scale;
    float offsetY = y - _textBoundsMinY * scale - (_molecule.maxY - _textBoundsMinY) * scale / 2;
    buildAtomLayouts(offsetX, offsetY, scale);

    drawMolecule(g2, x, y);

    g2.setStroke(oldStroke);
    g2.setColor(oldColor);
}

int ChemfigBox::getLastFontId() {
    return -1;
}

} // namespace tex
