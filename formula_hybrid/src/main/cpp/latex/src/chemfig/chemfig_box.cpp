#include "chemfig_box.h"
#include "chemfig_constants.h"
#include "fonts/fonts.h"
#include <cmath>
#include <algorithm>

namespace tex {

namespace {
    using namespace chemfig;

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
        if (seg.layout) seg.layout->getBounds(bounds);
        float scale = (type != SEG_NORMAL) ? SUBSCRIPT_SCALE : 1.0f;
        seg.width = (bounds.w + bounds.x + 0.6f) * textScale * scale;
        seg.height = bounds.h * textScale * scale;
        seg.ascent = -bounds.y * textScale * scale;
        return seg;
    }

    std::wstring resolveLatexCommands(const std::wstring& text) {
        std::wstring result;
        size_t i = 0;
        size_t n = text.size();

        while (i < n) {
            if (text[i] == L'\\' && i + 1 < n) {
                size_t cmdStart = i;
                i++;
                std::wstring cmdName;
                while (i < n && text[i] >= L'a' && text[i] <= L'z') {
                    cmdName += text[i];
                    i++;
                }

                if (cmdName == L"ominus") {
                    result += L'\u2296';
                } else if (cmdName == L"oplus") {
                    result += L'\u2295';
                } else if (cmdName == L"cdot") {
                    result += L'\u00B7';
                } else if (cmdName == L"circ") {
                    result += L'\u2218';
                } else if (cmdName == L"bullet") {
                    result += L'\u2219';
                } else if (cmdName == L"times") {
                    result += L'\u00D7';
                } else if (cmdName == L"chemabove" || cmdName == L"chembelow") {
                    auto parseBraceArg = [&](std::wstring& arg) -> bool {
                        if (i >= n || text[i] != L'{') return false;
                        i++;
                        int depth = 1;
                        while (i < n && depth > 0) {
                            if (text[i] == L'{') depth++;
                            else if (text[i] == L'}') {
                                depth--;
                                if (depth == 0) { i++; break; }
                            }
                            if (depth > 0) { arg += text[i]; i++; }
                        }
                        return true;
                    };
                    std::wstring baseArg, labelArg;
                    if (parseBraceArg(baseArg) && parseBraceArg(labelArg)) {
                        std::wstring resolvedBase = resolveLatexCommands(baseArg);
                        std::wstring resolvedLabel = resolveLatexCommands(labelArg);
                        result += resolvedBase;
                        if (cmdName == L"chemabove") {
                            result += L'^';
                            result += resolvedLabel;
                        } else {
                            result += L'_';
                            result += resolvedLabel;
                        }
                    }
                } else if (cmdName == L"scriptstyle" || cmdName == L"scriptscriptstyle" ||
                           cmdName == L"displaystyle" || cmdName == L"textstyle") {
                } else if (cmdName == L"vphantom" || cmdName == L"hphantom" || cmdName == L"phantom") {
                    if (i < n && text[i] == L'{') {
                        i++;
                        int depth = 1;
                        while (i < n && depth > 0) {
                            if (text[i] == L'{') depth++;
                            else if (text[i] == L'}') {
                                depth--;
                                if (depth == 0) { i++; break; }
                            }
                            i++;
                        }
                    }
                } else {
                    result += text.substr(cmdStart, i - cmdStart);
                }
            } else {
                result += text[i];
                i++;
            }
        }
        return result;
    }

    std::vector<TextSegment> splitTextToSegments(const std::wstring& rawText,
                                                  const sptr<Font>& font, float textScale) {
        std::wstring text = resolveLatexCommands(rawText);
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
        if (text.find_first_of(L"_^|") != std::wstring::npos) return true;
        if (text.find(L'\\') != std::wstring::npos) return true;
        return false;
    }

    float computeSegmentsTotalWidth(const std::vector<TextSegment>& segments) {
        float total = 0;
        for (size_t i = 0; i < segments.size(); i++) {
            bool isScriptPair = false;
            if (i + 1 < segments.size()) {
                TextSegmentType nextType = segments[i + 1].type;
                if ((segments[i].type == SEG_SUPERSCRIPT && nextType == SEG_SUBSCRIPT) ||
                    (segments[i].type == SEG_SUBSCRIPT && nextType == SEG_SUPERSCRIPT)) {
                    isScriptPair = true;
                    total += std::max(segments[i].width, segments[i + 1].width);
                    i++;
                }
            }
            if (!isScriptPair) {
                total += segments[i].width;
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
        if (layout) layout->getBounds(bounds);
        float textW = (bounds.w + bounds.x + 0.6f) * textScale;
        float textH = bounds.h * textScale;
        return {textW / 2, textH / 2, -bounds.y * textScale, textH};
    }

    int findAnchorIndex(const std::vector<Anchor>& anchors, int atomIndex) {
        int idx = 0;
        while (idx < static_cast<int>(anchors.size()) && atomIndex > anchors[idx].atomIndex) {
            idx++;
        }
        return idx;
    }
}

ChemfigBox::ChemfigBox(const Molecule& mol, color c, const sptr<Font>& font, float sizeFactor)
    : _molecule(mol), _color(c), _font(font), _sizeFactor(sizeFactor), _layoutsBuilt(false) {
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
        _textBoundsMaxX = 0;
        _textBoundsMaxY = 0;
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
    _textBoundsMaxX = maxX;
    _textBoundsMaxY = maxY;
}

float ChemfigBox::computeAnchorYOffset(int atomIndex) const {
    if (_molecule.rings.size() > 0) return 0.0f;

    bool hasAngleControl = false;
    for (const auto& b : _molecule.bonds) {
        if (b.params.hasAngle) {
            hasAngleControl = true;
            break;
        }
    }
    if (hasAngleControl) return 0.0f;
    if (_molecule.anchors.empty()) return 0.0f;

    int anchorIdx = findAnchorIndex(_molecule.anchors, atomIndex);
    return (_textBoundsMinY * 0.5f) * anchorIdx;
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

        float anchorsYOffset = computeAnchorYOffset(i);

        layout.x = atom.position.x * scale + offsetX;
        layout.y = atom.position.y * scale + offsetY - anchorsYOffset;

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
            if (layout.layout) layout.layout->getBounds(bounds);
            float textW = (bounds.w + bounds.x + 0.6f) * textScale;
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
    _layoutsBuilt = true;
}

void ChemfigBox::drawMolecule(Graphics2D& g2, float x, float y) {
    if (_molecule.atoms.empty()) return;

    float scale = BOND_SCALE;
    float offsetX = x - _textBoundsMinX * scale + PADDING * scale;
    float offsetY = y - _textBoundsMinY * scale - (_textBoundsMaxY - _textBoundsMinY) * scale / 2;

    int bondCount = 0;
    for (const auto& bond : _molecule.bonds) {
        if (!_molecule.isValidAtomIndex(bond.fromAtom)) continue;
        if (!_molecule.isValidAtomIndex(bond.toAtom)) continue;

        const AtomNode& from = _molecule.atoms[bond.fromAtom];
        const AtomNode& to = _molecule.atoms[bond.toAtom];

        float fromX = from.position.x * scale + offsetX;
        float toX = to.position.x * scale + offsetX;
        float fromY = 0.0f;
        float toY = 0.0f;

        float anchorYOffset = 0.0f;
        float anchorYFromOffset = 0.0f;
        float anchorYToOffset = 0.0f;

        bool hasAngleControl = bond.params.hasAngle;

        if (_molecule.rings.empty() && !hasAngleControl && !_molecule.anchors.empty()) {
            if (!bond.isHook) {
                anchorYOffset = computeAnchorYOffset(bondCount);
            } else {
                anchorYFromOffset = computeAnchorYOffset(bond.fromAtom);
                anchorYToOffset = computeAnchorYOffset(bond.toAtom);
            }
        }

        fromY = from.position.y * scale + offsetY - anchorYOffset - anchorYFromOffset;
        toY = to.position.y * scale + offsetY - anchorYOffset - anchorYToOffset;

        float dx = toX - fromX;
        float dy = toY - fromY;
        float bondLen = std::sqrt(dx * dx + dy * dy);

        if (bondLen > EPSILON) {
            float dirX = dx / bondLen;
            float dirY = dy / bondLen;

            float shortenFrom = 0.0f;
            float shortenTo = 0.0f;

            auto itFrom = _atomTextBounds.find(bond.fromAtom);
            if (itFrom != _atomTextBounds.end()) {
                shortenFrom = computeShortening(dirX, dirY, itFrom->second.halfW, itFrom->second.halfH);
            }

            auto itTo = _atomTextBounds.find(bond.toAtom);
            if (itTo != _atomTextBounds.end()) {
                shortenTo = computeShortening(-dirX, -dirY, itTo->second.halfW, itTo->second.halfH);
            }

            float totalShorten = shortenFrom + shortenTo;
            float minVisibleLen = TEXT_BOND_GAP * 2.0f;
            if (totalShorten + minVisibleLen > bondLen && totalShorten > EPSILON) {
                float ratio = (bondLen - minVisibleLen) / totalShorten;
                if (ratio < 0.0f) ratio = 0.0f;
                shortenFrom *= ratio;
                shortenTo *= ratio;
            }

            fromX += dirX * shortenFrom;
            fromY += dirY * shortenFrom;
            toX -= dirX * shortenTo;
            toY -= dirY * shortenTo;

            if (bond.params.hasOffset) {
                float startGap = bond.params.offsetStart * scale * 0.06f;
                float endGap = bond.params.offsetEnd * scale * 0.06f;
                fromX += dirX * startGap;
                fromY += dirY * startGap;
                toX -= dirX * endGap;
                toY -= dirY * endGap;
            }
        }

        ChemPoint bondRingCenter = _molecule.center();
        bool bondInRing = false;
        if (bond.ringIndex >= 0 && bond.ringIndex < static_cast<int>(_molecule.rings.size())) {
            bondRingCenter = _molecule.rings[bond.ringIndex].center;
            bondInRing = true;
        }
        ChemPoint scaledCenter(bondRingCenter.x * scale + offsetX, bondRingCenter.y * scale + offsetY);
        BondRenderer::drawBond(g2, bond.type, ChemPoint(fromX, fromY), ChemPoint(toX, toY),
                               scale, bond.params, scaledCenter, bondInRing);
        bondCount++;
    }

    for (const auto& layout : _atomLayouts) {
        float baseScale = 0.1f * _sizeFactor;
        if (baseScale < EPSILON) baseScale = EPSILON;

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
                if (seg.layout) seg.layout->draw(g2, 0, 0);
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
            if (layout.layout) layout.layout->draw(g2, 0, 0);
            g2.scale(1.f / baseScale, 1.f / baseScale);
            g2.translate(-drawX, -drawY);
        }
    }

    for (const auto& ring : _molecule.rings) {
        if (ring.hasInnerCircle && ring.sides > 0 && ring.radius > EPSILON) {
            float cx = ring.center.x * scale + offsetX;
            float cy = ring.center.y * scale + offsetY;
            float apothem = ring.radius * scale * std::cos(CHEM_PI / ring.sides);
            float r = apothem * 0.75f;
            if (r > EPSILON) {
                g2.drawEllipse(cx - r, cy + r, r * 2, r * 2, r, r, BOND_LINE_WIDTH * scale, 0.0f);
            }
        }
    }

    float textScale = 0.1f * _sizeFactor;
    float chargeRadius = 0.55f * scale * textScale * 10;
    
    for (int i = 0; i < static_cast<int>(_molecule.atoms.size()); i++) {
        const auto& atom = _molecule.atoms[i];
        if (atom.charges.empty()) continue;
        
        float atomX = atom.position.x * scale + offsetX;
        float atomY = atom.position.y * scale + offsetY;
        
        for (const auto& charge : atom.charges) {
            float angleRad = charge.angle * CHEM_PI / 180.0f;
            float dist = chargeRadius;
            if (charge.distance > 0) {
                dist = charge.distance * scale * textScale * 2.5f;
            }
            float dx = dist * std::cos(angleRad);
            float dy = -dist * std::sin(angleRad);
            float chargeX = atomX + dx;
            float chargeY = atomY + dy;
            
            if (charge.mark == L"\"") {
                float rectW = 0.45f * scale * textScale * 10;
                float rectH = 0.12f * scale * textScale * 10;
                float ca = std::cos(angleRad);
                float sa = std::sin(angleRad);
                float hw = rectW / 2;
                float hh = rectH / 2;
                float x1 = chargeX - hh * ca + hw * sa;
                float y1 = chargeY + hh * sa + hw * ca;
                float x2 = chargeX + hh * ca + hw * sa;
                float y2 = chargeY - hh * sa + hw * ca;
                float x3 = chargeX + hh * ca - hw * sa;
                float y3 = chargeY - hh * sa - hw * ca;
                float x4 = chargeX - hh * ca - hw * sa;
                float y4 = chargeY + hh * sa - hw * ca;
                g2.drawLine(x1, y1, x2, y2);
                g2.drawLine(x2, y2, x3, y3);
                g2.drawLine(x3, y3, x4, y4);
                g2.drawLine(x4, y4, x1, y1);
            } else if (charge.mark == L"\\|" || charge.mark == L"|") {
                float lineLen = 0.5f * scale * textScale * 10;
                float perpAngle = CHEM_PI / 2 - angleRad;
                float dx1 = lineLen / 2 * std::cos(perpAngle);
                float dy1 = lineLen / 2 * std::sin(perpAngle);
                g2.drawLine(chargeX - dx1, chargeY - dy1, chargeX + dx1, chargeY + dy1);
            } else if (charge.mark == L"\\:" || charge.mark == L":") {
                float dotSize = 0.12f * scale * textScale * 10;
                float dotSpacing = 0.18f * scale * textScale * 10;
                float perpAngle = CHEM_PI / 2 - angleRad;
                float ca = std::cos(perpAngle);
                float sa = std::sin(perpAngle);
                float offsetX1 = -dotSpacing / 2 * ca;
                float offsetY1 = -dotSpacing / 2 * sa;
                g2.fillRoundRect(chargeX + offsetX1 - dotSize / 2, chargeY + offsetY1 - dotSize / 2, dotSize, dotSize, dotSize / 2, dotSize / 2);
                float offsetX2 = dotSpacing / 2 * ca;
                float offsetY2 = dotSpacing / 2 * sa;
                g2.fillRoundRect(chargeX + offsetX2 - dotSize / 2, chargeY + offsetY2 - dotSize / 2, dotSize, dotSize, dotSize / 2, dotSize / 2);
            } else if (charge.mark == L"\\." || charge.mark == L".") {
                float dotSize = 0.12f * scale * textScale * 10;
                g2.fillRoundRect(chargeX - dotSize / 2, chargeY - dotSize / 2, dotSize, dotSize, dotSize / 2, dotSize / 2);
            } else if (charge.mark == L"\\oplus") {
                float radius = 0.20f * scale * textScale * 10;
                g2.drawEllipse(chargeX - radius, chargeY + radius, radius * 2, radius * 2, radius, radius, BOND_LINE_WIDTH * scale, 0.0f);
                float lineLen = radius * 0.8f;
                g2.drawLine(chargeX - lineLen, chargeY, chargeX + lineLen, chargeY);
                g2.drawLine(chargeX, chargeY - lineLen, chargeX, chargeY + lineLen);
            } else if (charge.mark == L"\\ominus") {
                float radius = 0.20f * scale * textScale * 10;
                g2.drawEllipse(chargeX - radius, chargeY + radius, radius * 2, radius * 2, radius, radius, BOND_LINE_WIDTH * scale, 0.0f);
                float lineLen = radius * 0.8f;
                g2.drawLine(chargeX - lineLen, chargeY, chargeX + lineLen, chargeY);
            } else {
                auto chargeLayout = TextLayout::create(charge.mark, _font);
                if (chargeLayout) {
                    Rect bounds;
                    chargeLayout->getBounds(bounds);
                    float scriptScale = charge.isScriptStyle ? 0.5f : 1.0f;
                    float scaledTextScale = textScale * scriptScale;
                    float chargeW = (bounds.w + bounds.x + 0.6f) * scaledTextScale;
                    float chargeH = bounds.h * scaledTextScale;
                    float chargeOffsetY = -bounds.y * scaledTextScale;
                    
                    g2.translate(chargeX - chargeW / 2, chargeY - chargeH / 2 + chargeOffsetY);
                    g2.scale(scaledTextScale, scaledTextScale);
                    chargeLayout->draw(g2, 0, 0);
                    g2.scale(1.f / scaledTextScale, 1.f / scaledTextScale);
                    g2.translate(-(chargeX - chargeW / 2), -(chargeY - chargeH / 2 + chargeOffsetY));
                }
            }
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
    float offsetY = y - _textBoundsMinY * scale - (_textBoundsMaxY - _textBoundsMinY) * scale / 2;

    if (!_layoutsBuilt) {
        buildAtomLayouts(offsetX, offsetY, scale);
    }

    drawMolecule(g2, x, y);

    g2.setStroke(oldStroke);
    g2.setColor(oldColor);
}

int ChemfigBox::getLastFontId() {
    return -1;
}

} // namespace tex
