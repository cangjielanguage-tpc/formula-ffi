#include "scheme_box.h"
#include "chemfig_constants.h"
#include "chemfig_box.h"
#include "core/formula.h"
#include "atom/atom_basic.h"
#include <cmath>
#include <algorithm>

namespace tex {

namespace {
    using namespace chemfig;
}

sptr<Box> SchemeBox::createLabelBox(const std::wstring& text, TeXEnvironment& env) {
    if (text.empty()) return nullptr;
    try {
        TeXFormula formula(text);
        return formula.createBox(env);
    } catch (...) {
        return nullptr;
    }
}

SchemeBox::SchemeBox(const ReactionScheme& scheme, color c,
                     const sptr<Font>& font, float sizeFactor,
                     TeXEnvironment& env)
    : _scheme(scheme), _color(c), _font(font), _sizeFactor(sizeFactor) {
    _compoundGap = SchemeConfig::instance().compoundSep;
    _arrowLength = DEFAULT_ARROW_LENGTH * SchemeConfig::instance().arrowCoeff;
    _scale = BOND_SCALE;
    calculateLayout(env);
}

void SchemeBox::calculateCompoundAnchors(CompoundLayout& cl) {
    cl.anchors = ReactionScheme::calculateCompoundAnchors(
        cl.x, cl.y, cl.width, cl.height, cl.boxHeight, cl.boxDepth);
}

ChemPoint SchemeBox::resolveArrowEndpoint(
    int compoundIdx,
    const ArrowRef& ref,
    const ArrowAnchor& anchor,
    float angle,
    bool isFrom) {
    if (compoundIdx < 0 || compoundIdx >= static_cast<int>(_compoundLayouts.size())) {
        return ChemPoint();
    }

    auto& cl = _compoundLayouts[compoundIdx];
    bool hasAngle = (std::abs(angle) > EPSILON);

    if (!anchor.anchorName.empty()) {
        return cl.anchors.getAnchor(anchor.anchorName);
    }

    if (!ref.anchorName.empty()) {
        ChemPoint testAnchor = cl.anchors.getAnchor(ref.anchorName);
        bool anchorFound = (std::abs(testAnchor.x - cl.anchors.center.x) > ANCHOR_EQUALITY_THRESHOLD ||
                            std::abs(testAnchor.y - cl.anchors.center.y) > ANCHOR_EQUALITY_THRESHOLD);
        if (anchorFound) {
            return testAnchor;
        }
        ChemPoint atomPos = _scheme.getAtomPosition(compoundIdx, ref.anchorName);
        if (atomPos.x != 0 || atomPos.y != 0) {
            return ChemPoint(cl.x + atomPos.x * _scale, cl.y + atomPos.y * _scale);
        }
        return isFrom ? cl.anchors.east : cl.anchors.west;
    }

    if (hasAngle) {
        float entryAngle = isFrom ? angle : (angle + 180.0f);
        return cl.anchors.getAnchor(std::to_wstring(static_cast<int>(entryAngle)));
    }

    return isFrom ? cl.anchors.east : cl.anchors.west;
}

void SchemeBox::calculateLayout(TeXEnvironment& env) {
    _compoundLayouts.clear();
    _arrowLayouts.clear();
    _plusLayouts.clear();
    _mergeLayouts.clear();

    _compoundLayouts.resize(_scheme.compoundCount());

    for (int i = 0; i < _scheme.compoundCount(); i++) {
        const auto& info = _scheme.compounds[i];
        sptr<Box> mbox(new ChemfigBox(info.molecule, _color, _font, _sizeFactor));

        CompoundLayout& layout = _compoundLayouts[i];
        layout.box = mbox;
        layout.width = mbox->_width;
        layout.height = mbox->_height + mbox->_depth;
        layout.boxHeight = mbox->_height;
        layout.boxDepth = mbox->_depth;

        if (!info.name.empty()) {
            layout.nameBox = createLabelBox(info.name, env);
        }
        if (info.number > 0) {
            std::wstring numText = L"(" + std::to_wstring(info.number) + L")";
            layout.numberBox = createLabelBox(numText, env);
        }
    }

    std::vector<std::vector<int>> rows;
    std::vector<int> currentRow;

    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first == ELEM_LINEBREAK) {
            if (!currentRow.empty()) {
                rows.push_back(currentRow);
                currentRow.clear();
            }
        } else if (elem.first == ELEM_COMPOUND) {
            currentRow.push_back(elem.second);
        }
    }
    if (!currentRow.empty()) {
        rows.push_back(currentRow);
    }
    if (rows.empty()) {
        for (int i = 0; i < _scheme.compoundCount(); i++) {
            rows.push_back({i});
        }
    }

    float lineSpacing = SchemeConfig::instance().lineSpacing;
    float currentY = 0.0f;

    for (size_t rowIdx = 0; rowIdx < rows.size(); rowIdx++) {
        const auto& row = rows[rowIdx];
        float currentX = 0.0f;
        float rowMaxHeight = 0.0f;

        for (int compoundIdx : row) {
            auto& layout = _compoundLayouts[compoundIdx];
            layout.x = currentX;
            layout.y = currentY;
            layout.positioned = true;
            currentX += layout.width + _compoundGap;
            if (layout.height > rowMaxHeight) rowMaxHeight = layout.height;
        }

        if (rowIdx < rows.size() - 1) {
            currentY += rowMaxHeight + lineSpacing;
        }
    }

    for (auto& cl : _compoundLayouts) {
        if (cl.positioned) {
            calculateCompoundAnchors(cl);
        }
    }

    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first == ELEM_ARROW) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.arrows.size())) continue;
            const ArrowElement& arrow = _scheme.arrows[elem.second];
            ArrowLayout alayout;
            alayout.arrowIndex = elem.second;

            int fromIdx = arrow.fromCompound;
            int toIdx = arrow.toCompound;

            if (_scheme.compoundCount() == 0) continue;
            if (fromIdx < 0) fromIdx = 0;
            if (toIdx < 0) toIdx = std::min(fromIdx + 1, _scheme.compoundCount() - 1);
            if (fromIdx >= _scheme.compoundCount()) fromIdx = _scheme.compoundCount() - 1;
            if (toIdx >= _scheme.compoundCount()) toIdx = _scheme.compoundCount() - 1;

            bool hasAngle = (std::abs(arrow.params.angle) > EPSILON);
            float angleRad = hasAngle ? arrow.params.angle * CHEM_PI / 180.0f : 0.0f;

            ChemPoint fromPos = resolveArrowEndpoint(
                fromIdx, arrow.params.fromRef, arrow.params.fromAnchor,
                arrow.params.angle, true);

            ChemPoint toPos = resolveArrowEndpoint(
                toIdx, arrow.params.toRef, arrow.params.toAnchor,
                arrow.params.angle, false);

            if (toIdx >= 0 && toIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& tl = _compoundLayouts[toIdx];
                if (hasAngle && !tl.positioned) {
                    float arrowLen = _arrowLength * arrow.params.lengthCoeff * _scale;
                    float centerX = fromPos.x + std::cos(angleRad) * arrowLen;
                    float centerY = fromPos.y - std::sin(angleRad) * arrowLen;
                    tl.x = centerX - tl.width * 0.5f;
                    tl.y = centerY - (tl.boxDepth - tl.boxHeight) * 0.5f;
                    tl.positioned = true;
                    calculateCompoundAnchors(tl);
                    float entryAngle = arrow.params.angle + 180.0f;
                    toPos = tl.anchors.getAnchor(
                        std::to_wstring(static_cast<int>(entryAngle)));
                }
            } else if (hasAngle) {
                float arrowLen = _arrowLength * arrow.params.lengthCoeff * _scale;
                toPos = ChemPoint(
                    fromPos.x + std::cos(angleRad) * arrowLen,
                    fromPos.y - std::sin(angleRad) * arrowLen
                );
            }

            float arrowLen = _arrowLength * arrow.params.lengthCoeff * _scale;
            if (std::abs(toPos.x - fromPos.x) < EPSILON &&
                std::abs(toPos.y - fromPos.y) < EPSILON) {
                toPos = ChemPoint(fromPos.x + arrowLen, fromPos.y);
            }

            alayout.from = fromPos;
            alayout.to = toPos;

            if (!arrow.params.yShiftRaw.empty()) {
                auto len = SpaceAtom::getLength(arrow.params.yShiftRaw);
                alayout.yShift = SpaceAtom::getSize(len.first, len.second, env);
            }

            if (!arrow.params.labelAbove.empty()) {
                alayout.labelAboveBox = createLabelBox(arrow.params.labelAbove, env);
            }
            if (!arrow.params.labelBelow.empty()) {
                alayout.labelBelowBox = createLabelBox(arrow.params.labelBelow, env);
            }

            _arrowLayouts.push_back(alayout);
        } else if (elem.first == ELEM_PLUS) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.pluses.size())) continue;
            PlusLayout playout;
            int afterIdx = _scheme.pluses[elem.second].afterCompound;
            if (afterIdx >= 0 && afterIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& cl = _compoundLayouts[afterIdx];
                playout.x = cl.x + cl.width + _compoundGap * 0.5f;
                playout.y = cl.y + (cl.boxDepth - cl.boxHeight) * 0.5f;
            }
            _plusLayouts.push_back(playout);
        } else if (elem.first == ELEM_MERGE) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.merges.size())) continue;
            const MergeElement& merge = _scheme.merges[elem.second];
            MergeLayout mlayout;
            mlayout.mergeIndex = elem.second;
            mlayout.direction = merge.direction;
            mlayout.segmentCoeff = merge.geometry.segmentCoeff;

            for (const auto& src : merge.sources) {
                int srcIdx = src.compoundIndex;
                if (srcIdx >= 0 && srcIdx < static_cast<int>(_compoundLayouts.size())) {
                    auto& cl = _compoundLayouts[srcIdx];
                    if (!src.anchorName.empty()) {
                        mlayout.fromPoints.push_back(cl.anchors.getAnchor(src.anchorName));
                    } else {
                        switch (merge.direction) {
                            case MERGE_RIGHT:
                            case MERGE_LEFT:
                                mlayout.fromPoints.push_back(cl.anchors.south);
                                break;
                            case MERGE_UP:
                            case MERGE_DOWN:
                                mlayout.fromPoints.push_back(cl.anchors.east);
                                break;
                        }
                    }
                }
            }

            int targetIdx = merge.target.compoundIndex;
            if (targetIdx >= 0 && targetIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& cl = _compoundLayouts[targetIdx];
                if (!merge.target.anchorName.empty()) {
                    mlayout.toPoint = cl.anchors.getAnchor(merge.target.anchorName);
                } else {
                    switch (merge.direction) {
                        case MERGE_RIGHT:
                        case MERGE_LEFT:
                            mlayout.toPoint = cl.anchors.north;
                            break;
                        case MERGE_UP:
                        case MERGE_DOWN:
                            mlayout.toPoint = cl.anchors.west;
                            break;
                    }
                }
            }

            _mergeLayouts.push_back(mlayout);
        }
    }

    float totalWidth = 0.0f;
    float maxBottom = 0.0f;
    float maxTop = 0.0f;

    for (const auto& cl : _compoundLayouts) {
        float right = cl.x + cl.width;
        if (right > totalWidth) totalWidth = right;

        float bottom = cl.y + cl.boxDepth;
        float top = cl.y - cl.boxHeight;
        if (bottom > maxBottom) maxBottom = bottom;
        if (top < maxTop) maxTop = top;

        if (cl.nameBox) {
            float nameBottom = cl.y + cl.boxDepth +
                               SchemeConfig::instance().nameOffset + cl.nameBox->_height + cl.nameBox->_depth;
            if (nameBottom > maxBottom) maxBottom = nameBottom;
        }
        if (cl.numberBox) {
            float numBottom = cl.y + cl.boxDepth +
                              SchemeConfig::instance().numOffset + cl.numberBox->_height + cl.numberBox->_depth;
            if (numBottom > maxBottom) maxBottom = numBottom;
        }
    }

    for (const auto& al : _arrowLayouts) {
        float minX = std::min(al.from.x, al.to.x);
        float maxX = std::max(al.from.x, al.to.x);
        float minY = std::min(al.from.y, al.to.y);
        float maxY = std::max(al.from.y, al.to.y);

        if (al.arrowIndex >= 0 && al.arrowIndex < static_cast<int>(_scheme.arrows.size())) {
            const auto& arrow = _scheme.arrows[al.arrowIndex];
            if (arrow.params.isCurved()) {
                ChemPoint ctrl = ArrowRenderer::computeCurveControlPoint(
                    al.from, al.to, arrow.params.curveHeight);
                if (ctrl.x < minX) minX = ctrl.x;
                if (ctrl.x > maxX) maxX = ctrl.x;
                if (ctrl.y < minY) minY = ctrl.y;
                if (ctrl.y > maxY) maxY = ctrl.y;
            }
        }

        if (maxX > totalWidth) totalWidth = maxX;
        if (minY < maxTop) maxTop = minY;
        if (maxY > maxBottom) maxBottom = maxY;

        if (al.labelAboveBox) {
            float labelMidY = (al.from.y + al.to.y) * 0.5f;
            float labelTop = labelMidY - LABEL_OFFSET - al.labelAboveBox->_depth - al.labelAboveBox->_height;
            if (labelTop < maxTop) maxTop = labelTop;
        }
        if (al.labelBelowBox) {
            float labelMidY = (al.from.y + al.to.y) * 0.5f;
            float labelBottom = labelMidY + LABEL_OFFSET + al.labelBelowBox->_height + al.labelBelowBox->_depth;
            if (labelBottom > maxBottom) maxBottom = labelBottom;
        }
    }

    for (const auto& pl : _plusLayouts) {
        float plusRight = pl.x + PLUS_SIGN_SIZE;
        if (plusRight > totalWidth) totalWidth = plusRight;
    }

    for (const auto& ml : _mergeLayouts) {
        for (const auto& fp : ml.fromPoints) {
            if (fp.x > totalWidth) totalWidth = fp.x;
            if (fp.y > maxBottom) maxBottom = fp.y;
            if (fp.y < maxTop) maxTop = fp.y;
        }
        if (ml.toPoint.x > totalWidth) totalWidth = ml.toPoint.x;
        if (ml.toPoint.y > maxBottom) maxBottom = ml.toPoint.y;
        if (ml.toPoint.y < maxTop) maxTop = ml.toPoint.y;
    }

    _width = totalWidth + 2 * PADDING;
    _height = (-maxTop) + PADDING;
    _depth = maxBottom + PADDING;
    _foreground = _color;
}

void SchemeBox::drawArrows(Graphics2D& g2, float ox, float oy) {
    ArrowStyle style;
    style.headLength = SchemeConfig::instance().arrowHeadLength;
    style.headWidth = SchemeConfig::instance().arrowHeadWidth;
    style.lineWidth = ARROW_LINE_WIDTH;
    style.doubleBondOffset = ARROW_DOUBLE_BOND_OFFSET;
    style.harpRadius = ARROW_HARP_RADIUS;
    style.dashLength = ARROW_DASH_LENGTH;
    style.dashGap = ARROW_DASH_GAP;

    for (const auto& al : _arrowLayouts) {
        if (al.arrowIndex < 0 || al.arrowIndex >= static_cast<int>(_scheme.arrows.size())) continue;
        const auto& arrow = _scheme.arrows[al.arrowIndex];

        ChemPoint fromPos(al.from.x + ox, al.from.y + oy - al.yShift);
        ChemPoint toPos(al.to.x + ox, al.to.y + oy - al.yShift);

        ArrowRenderer::drawArrow(g2, arrow.params.type,
                                 fromPos, toPos, _scale,
                                 arrow.params, style);

        if (al.labelAboveBox || al.labelBelowBox) {
            ChemPoint labelRef;
            if (arrow.params.isCurved()) {
                ChemPoint ctrl = ArrowRenderer::computeCurveControlPoint(
                    fromPos, toPos, arrow.params.curveHeight);
                float t = 0.5f;
                float t1 = 1.0f - t;
                labelRef = ChemPoint(
                    t1 * t1 * fromPos.x + 2.0f * t1 * t * ctrl.x + t * t * toPos.x,
                    t1 * t1 * fromPos.y + 2.0f * t1 * t * ctrl.y + t * t * toPos.y
                );
            } else {
                labelRef = ChemPoint(
                    (fromPos.x + toPos.x) * 0.5f,
                    (fromPos.y + toPos.y) * 0.5f
                );
            }

            if (al.labelAboveBox) {
                float lx = labelRef.x - al.labelAboveBox->_width * 0.5f;
                float ly = labelRef.y - LABEL_OFFSET - al.labelAboveBox->_depth;
                al.labelAboveBox->draw(g2, lx, ly);
            }
            if (al.labelBelowBox) {
                float lx = labelRef.x - al.labelBelowBox->_width * 0.5f;
                float ly = labelRef.y + LABEL_OFFSET + al.labelBelowBox->_height;
                al.labelBelowBox->draw(g2, lx, ly);
            }
        }
    }
}

void SchemeBox::drawPlusSigns(Graphics2D& g2, float ox, float oy) {
    color oldColor = g2.getColor();
    if (!istrans(_color)) g2.setColor(_color);

    float halfSize = PLUS_SIGN_SIZE * 0.5f;

    for (const auto& pl : _plusLayouts) {
        float px = pl.x + ox;
        float py = pl.y + oy;
        g2.drawLine(px - halfSize, py, px + halfSize, py);
        g2.drawLine(px, py - halfSize, px, py + halfSize);
    }

    g2.setColor(oldColor);
}

void SchemeBox::drawMerges(Graphics2D& g2, float ox, float oy) {
    color oldColor = g2.getColor();
    const Stroke& oldStroke = g2.getStroke();
    if (!istrans(_color)) g2.setColor(_color);

    g2.setStroke(Stroke(ARROW_LINE_WIDTH * _scale, CAP_ROUND, JOIN_ROUND));

    float headLength = SchemeConfig::instance().arrowHeadLength * _scale;
    float headWidth = SchemeConfig::instance().arrowHeadWidth * _scale;

    for (const auto& ml : _mergeLayouts) {
        if (ml.fromPoints.empty()) continue;

        ChemPoint target(ml.toPoint.x + ox, ml.toPoint.y + oy);

        for (const auto& fromPt : ml.fromPoints) {
            ChemPoint source(fromPt.x + ox, fromPt.y + oy);

            float dx = target.x - source.x;
            float dy = target.y - source.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < EPSILON) continue;

            float dirX = dx / len;
            float dirY = dy / len;
            ChemPoint perp(-dirY, dirX);

            float segLen = len * ml.segmentCoeff;
            float offset = len * MERGE_BEND_OFFSET_RATIO;
            float bendSign = (ml.direction == MERGE_RIGHT || ml.direction == MERGE_DOWN) ? 1.0f : -1.0f;
            bool isVertical = (ml.direction == MERGE_UP || ml.direction == MERGE_DOWN);

            if (isVertical) {
                float vertOffset = len * MERGE_BEND_OFFSET_RATIO * bendSign;
                ChemPoint bend(source.x, source.y + vertOffset);
                g2.drawLine(source.x, source.y, bend.x, bend.y);
                g2.drawLine(bend.x, bend.y, target.x, target.y);
            } else {
                ChemPoint midPoint(source.x + dirX * segLen, source.y + dirY * segLen);
                ChemPoint bend(midPoint.x + perp.x * offset * bendSign,
                               midPoint.y + perp.y * offset * bendSign);
                g2.drawLine(source.x, source.y, bend.x, bend.y);
                g2.drawLine(bend.x, bend.y, target.x, target.y);
            }

            ArrowRenderer::drawMergeArrowHead(g2, target, source, _scale, headLength, headWidth);
        }
    }

    g2.setStroke(oldStroke);
    g2.setColor(oldColor);
}

void SchemeBox::drawCompoundNames(Graphics2D& g2, float ox, float oy) {
    for (size_t i = 0; i < _compoundLayouts.size(); i++) {
        const auto& cl = _compoundLayouts[i];
        if (!cl.nameBox) continue;

        float nx = cl.x + ox + (cl.width - cl.nameBox->_width) * 0.5f;
        float ny = cl.y + oy + cl.boxDepth + SchemeConfig::instance().nameOffset + cl.nameBox->_height;
        cl.nameBox->draw(g2, nx, ny);
    }
}

void SchemeBox::drawCompoundNumbers(Graphics2D& g2, float ox, float oy) {
    for (size_t i = 0; i < _compoundLayouts.size(); i++) {
        const auto& cl = _compoundLayouts[i];
        if (!cl.numberBox) continue;

        float nx = cl.x + ox + cl.width + COMPOUND_NUMBER_GAP;
        float ny = cl.y + oy + cl.boxDepth + SchemeConfig::instance().numOffset + cl.numberBox->_height;
        cl.numberBox->draw(g2, nx, ny);
    }
}

void SchemeBox::drawDebug(Graphics2D& g2, float ox, float oy) {
    color oldColor = g2.getColor();
    g2.setColor(0xFF0000FF);

    for (const auto& cl : _compoundLayouts) {
        float x = cl.x + ox;
        float y = cl.y + oy;
        g2.drawLine(x, y - cl.boxHeight, x + cl.width, y - cl.boxHeight);
        g2.drawLine(x, y + cl.boxDepth, x + cl.width, y + cl.boxDepth);
        g2.drawLine(x, y - cl.boxHeight, x, y + cl.boxDepth);
        g2.drawLine(x + cl.width, y - cl.boxHeight, x + cl.width, y + cl.boxDepth);
    }

    g2.setColor(oldColor);
}

void SchemeBox::draw(Graphics2D& g2, float x, float y) {
    float ox = x + PADDING;
    float oy = y;

    for (const auto& cl : _compoundLayouts) {
        if (cl.box) {
            cl.box->draw(g2, cl.x + ox, cl.y + oy);
        }
    }

    drawArrows(g2, ox, oy);
    drawPlusSigns(g2, ox, oy);
    drawMerges(g2, ox, oy);
    drawCompoundNames(g2, ox, oy);
    drawCompoundNumbers(g2, ox, oy);

    if (SchemeConfig::instance().debugMode) {
        drawDebug(g2, ox, oy);
    }
}

int SchemeBox::getLastFontId() {
    if (!_compoundLayouts.empty() && _compoundLayouts[0].box) {
        return _compoundLayouts[0].box->getLastFontId();
    }
    return TeXFont::NO_FONT;
}

} // namespace tex
