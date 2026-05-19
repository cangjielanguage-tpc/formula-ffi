#ifndef SCHEME_BOX_H_INCLUDED
#define SCHEME_BOX_H_INCLUDED

#include "scheme_types.h"
#include "arrow_renderer.h"
#include "scheme_config.h"
#include "atom/box.h"
#include "fonts/fonts.h"
#include "core/core.h"

namespace tex {

class SchemeBox : public Box {
private:
    ReactionScheme _scheme;
    color _color;
    sptr<Font> _font;
    float _sizeFactor;

    struct CompoundLayout {
        sptr<Box> box;
        float x, y;
        float width, height;
        float boxHeight, boxDepth;
        float rowMaxDepth;
        sptr<Box> nameBox;
        sptr<Box> numberBox;
        CompoundAnchorPos anchors;
        bool positioned;
        bool invisible;
        CompoundLayout() : x(0), y(0), width(0), height(0),
                           boxHeight(0), boxDepth(0), rowMaxDepth(0), positioned(false), invisible(false) {}
    };
    std::vector<CompoundLayout> _compoundLayouts;

    struct ArrowLayout {
        ChemPoint from, to;
        int arrowIndex;
        float yShift;
        sptr<Box> labelAboveBox;
        sptr<Box> labelBelowBox;
        ArrowLayout() : arrowIndex(-1), yShift(0.0f) {}
    };
    std::vector<ArrowLayout> _arrowLayouts;

    struct PlusLayout {
        float x, y;
        PlusLayout() : x(0), y(0) {}
    };
    std::vector<PlusLayout> _plusLayouts;

    struct MergeLayout {
        std::vector<ChemPoint> fromPoints;
        ChemPoint toPoint;
        MergeDirection direction;
        float segmentCoeff;
        int mergeIndex;
        MergeLayout() : direction(MERGE_RIGHT), segmentCoeff(0.5f), mergeIndex(-1) {}
    };
    std::vector<MergeLayout> _mergeLayouts;

    struct SubschemeLayout {
        int subschemeIndex;
        int firstCompound;
        int lastCompound;
        float minX, maxX, minY, maxY;
        float centerX, centerY;
        float width, height;
        CompoundAnchorPos anchors;
        sptr<Box> leftDelimBox;
        sptr<Box> rightDelimBox;
        float leftDelimWidth;
        float rightDelimWidth;
        SubschemeLayout() : subschemeIndex(-1), firstCompound(-1), lastCompound(-1),
                            minX(0), maxX(0), minY(0), maxY(0),
                            centerX(0), centerY(0), width(0), height(0),
                            leftDelimWidth(0), rightDelimWidth(0) {}
    };
    std::vector<SubschemeLayout> _subschemeLayouts;

    float _compoundGap;
    float _arrowLength;
    float _scale;

    void calculateLayout(TeXEnvironment& env);
    void calculateCompoundAnchors(CompoundLayout& cl);
    ChemPoint resolveArrowEndpoint(int compoundIdx, const ArrowRef& ref,
                                   const ArrowAnchor& anchor, float angle, bool isFrom,
                                   int subschemeIdx = -1);
    void drawArrows(Graphics2D& g2, float ox, float oy);
    void drawPlusSigns(Graphics2D& g2, float ox, float oy);
    void drawMerges(Graphics2D& g2, float ox, float oy);
    void drawCompoundNames(Graphics2D& g2, float ox, float oy);
    void drawCompoundNumbers(Graphics2D& g2, float ox, float oy);
    void drawSubschemeDelimiters(Graphics2D& g2, float ox, float oy);
    void drawDebug(Graphics2D& g2, float ox, float oy);

    sptr<Box> createLabelBox(const std::wstring& text, TeXEnvironment& env);

public:
    SchemeBox(const ReactionScheme& scheme, color c,
              const sptr<Font>& font, float sizeFactor,
              TeXEnvironment& env);

    void draw(Graphics2D& g2, float x, float y) override;
    int getLastFontId() override;
};

} // namespace tex

#endif // SCHEME_BOX_H_INCLUDED
