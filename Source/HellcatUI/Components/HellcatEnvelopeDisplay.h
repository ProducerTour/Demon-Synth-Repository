#pragma once
#include <JuceHeader.h>
#include "../HellcatLookAndFeel.h"
#include <cmath>

class HellcatEnvelopeDisplay : public juce::Component,
                               private juce::Timer,
                               private juce::Label::Listener
{
public:
    enum class DragPoint { None, Attack, Decay, Sustain, Release };

    HellcatEnvelopeDisplay()
    {
        // One-shot timer to force repaint after component is laid out
        startTimer(100);

        // Editable value labels
        auto setupValueLabel = [this](juce::Label& label) {
            label.setJustificationType(juce::Justification::centred);
            label.setColour(juce::Label::textColourId, HellcatColors::textPrimary);
            label.setColour(juce::Label::textWhenEditingColourId, HellcatColors::textPrimary);
            label.setColour(juce::Label::backgroundWhenEditingColourId, HellcatColors::panelDark);
            label.setColour(juce::Label::outlineWhenEditingColourId, HellcatColors::hellcatRed);
            label.setEditable(true, true, false);
            label.addListener(this);
            addAndMakeVisible(label);
        };
        setupValueLabel(attackValueLabel);
        setupValueLabel(decayValueLabel);
        setupValueLabel(sustainValueLabel);
        setupValueLabel(releaseValueLabel);
        updateValueLabels();
    }

    ~HellcatEnvelopeDisplay() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();
        if (bounds.isEmpty()) return;

        // Reserve bottom area for ADSR values
        auto valueBounds = bounds.removeFromBottom(60);
        graphBounds = bounds;

        // Background with gradient
        juce::ColourGradient bgGradient(
            HellcatColors::background, 0, 0,
            juce::Colour(0xff0a0c0f), 0, static_cast<float>(graphBounds.getHeight()),
            false
        );
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(graphBounds.toFloat(), 8.0f);

        // Border
        g.setColour(HellcatColors::panelLight);
        g.drawRoundedRectangle(graphBounds.toFloat(), 8.0f, 1.0f);

        // Grid
        drawGrid(g, graphBounds.reduced(20));

        // Build path based on current bounds
        pathBounds = graphBounds.reduced(20).toFloat();
        if (pathBounds.getWidth() < 10 || pathBounds.getHeight() < 10) return;

        // Calculate touchpoint positions
        calculateTouchpoints();

        // Build curved envelope path
        juce::Path envPath = buildCurvedEnvelopePath();

        // Fill under curve
        juce::Path fillPath = envPath;
        fillPath.lineTo(pathBounds.getRight(), pathBounds.getBottom());
        fillPath.lineTo(pathBounds.getX(), pathBounds.getBottom());
        fillPath.closeSubPath();

        juce::ColourGradient fillGradient(
            HellcatColors::hellcatRed.withAlpha(0.2f),
            pathBounds.getCentreX(), pathBounds.getY(),
            HellcatColors::hellcatRed.withAlpha(0.0f),
            pathBounds.getCentreX(), pathBounds.getBottom(),
            false
        );
        g.setGradientFill(fillGradient);
        g.fillPath(fillPath);

        // Draw envelope line
        g.setColour(HellcatColors::hellcatRed);
        g.strokePath(envPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Glow effect
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
        g.strokePath(envPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Draw touchpoints with labels
        drawTouchpoint(g, attackPoint, currentDragPoint == DragPoint::Attack || hoveredPoint == DragPoint::Attack, "A");
        drawTouchpoint(g, decayPoint, currentDragPoint == DragPoint::Decay || hoveredPoint == DragPoint::Decay, "D");
        drawTouchpoint(g, sustainPoint, currentDragPoint == DragPoint::Sustain || hoveredPoint == DragPoint::Sustain, "S");
        drawTouchpoint(g, releasePoint, currentDragPoint == DragPoint::Release || hoveredPoint == DragPoint::Release, "R");

        // Draw drag value tooltip
        if (currentDragPoint != DragPoint::None)
            drawDragTooltip(g);

        // Draw ADSR values at bottom
        drawADSRValues(g, valueBounds);
    }

    void setADSR(float attack, float decay, float sustain, float release)
    {
        // Only update if not currently dragging or editing (to avoid fighting with user input)
        if (currentDragPoint == DragPoint::None && !isEditingLabel)
        {
            attackTime = attack;
            decayTime = decay;
            sustainLevel = sustain;
            releaseTime = release;
            updateValueLabels();
            repaint();
        }
    }

    void resized() override
    {
        repaint();
    }

    // Callbacks for when user drags envelope points
    std::function<void(float)> onAttackChanged;
    std::function<void(float)> onDecayChanged;
    std::function<void(float)> onSustainChanged;
    std::function<void(float)> onReleaseChanged;

    // Callbacks for curve/tension changes
    std::function<void(float)> onAttackCurveChanged;
    std::function<void(float)> onDecayCurveChanged;
    std::function<void(float)> onReleaseCurveChanged;

    // Set curve values from external source
    void setCurves(float attack, float decay, float release)
    {
        if (currentDragPoint == DragPoint::None)
        {
            attackCurve = attack;
            decayCurve = decay;
            releaseCurve = release;
            repaint();
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        currentDragPoint = getPointAt(e.getPosition().toFloat());
        if (currentDragPoint != DragPoint::None)
        {
            updateCursorForPoint(currentDragPoint);
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (currentDragPoint == DragPoint::None || pathBounds.isEmpty())
            return;

        auto pos = e.getPosition().toFloat();
        float width = pathBounds.getWidth();
        float height = pathBounds.getHeight();

        switch (currentDragPoint)
        {
            case DragPoint::Attack:
            {
                // Attack X controls time, Y controls curve tension
                float relX = juce::jlimit(0.0f, width * 0.3f, pos.x - pathBounds.getX());
                float newAttack = (relX / width) * 3.0f; // Scale to 0-3 seconds for this portion
                attackTime = juce::jlimit(0.001f, 10.0f, newAttack);
                if (onAttackChanged) onAttackChanged(attackTime);

                // Y position controls curve: top = more exponential (-6), bottom = more linear (0)
                float relY = (pos.y - pathBounds.getY()) / height;
                // Map Y (0=top, 1=bottom) to curve (-6 to 0)
                // Top of graph = strong exponential curve, bottom = linear
                float newCurve = juce::jlimit(-6.0f, 0.0f, -6.0f * (1.0f - relY));
                attackCurve = newCurve;
                if (onAttackCurveChanged) onAttackCurveChanged(attackCurve);
                break;
            }
            case DragPoint::Decay:
            {
                // Decay X controls time, Y controls both sustain level AND curve
                float attackWidth = (attackTime / (attackTime + decayTime + 0.4f + releaseTime)) * width;
                float relX = juce::jlimit(attackWidth + 5.0f, width * 0.6f, pos.x - pathBounds.getX()) - attackWidth;
                float newDecay = (relX / width) * 3.0f;
                decayTime = juce::jlimit(0.001f, 10.0f, newDecay);
                if (onDecayChanged) onDecayChanged(decayTime);

                // Y position controls curve: top = linear (0), bottom = more logarithmic (6)
                float relY = (pos.y - pathBounds.getY()) / height;
                float newCurve = juce::jlimit(0.0f, 6.0f, 6.0f * relY);
                decayCurve = newCurve;
                if (onDecayCurveChanged) onDecayCurveChanged(decayCurve);
                break;
            }
            case DragPoint::Sustain:
            {
                // Sustain controls Y position (level)
                float relY = pos.y - pathBounds.getY();
                float newSustain = 1.0f - juce::jlimit(0.0f, 1.0f, relY / height);
                sustainLevel = newSustain;
                if (onSustainChanged) onSustainChanged(sustainLevel);
                break;
            }
            case DragPoint::Release:
            {
                // Release X controls time, Y controls curve
                float sustainWidth = ((attackTime + decayTime + 0.4f) / (attackTime + decayTime + 0.4f + releaseTime)) * width;
                float relX = juce::jlimit(sustainWidth + 5.0f, width, pos.x - pathBounds.getX()) - sustainWidth;
                float newRelease = (relX / width) * 3.0f;
                releaseTime = juce::jlimit(0.001f, 10.0f, newRelease);
                if (onReleaseChanged) onReleaseChanged(releaseTime);

                // Y position controls curve: top = linear (0), bottom = more logarithmic (6)
                float relY = (pos.y - pathBounds.getY()) / height;
                float newCurve = juce::jlimit(0.0f, 6.0f, 6.0f * relY);
                releaseCurve = newCurve;
                if (onReleaseCurveChanged) onReleaseCurveChanged(releaseCurve);
                break;
            }
            default:
                break;
        }
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        currentDragPoint = DragPoint::None;
        setMouseCursor(juce::MouseCursor::NormalCursor);
        repaint();
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        auto newHovered = getPointAt(e.getPosition().toFloat());
        if (newHovered != hoveredPoint)
        {
            hoveredPoint = newHovered;
            updateCursorForPoint(hoveredPoint);
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        if (hoveredPoint != DragPoint::None)
        {
            hoveredPoint = DragPoint::None;
            setMouseCursor(juce::MouseCursor::NormalCursor);
            repaint();
        }
    }

private:
    void timerCallback() override
    {
        stopTimer();
        repaint();
    }

    // Apply exponential/logarithmic curve transformation
    // curve < 0: exponential (fast start, slow end) - typical for attack
    // curve > 0: logarithmic (slow start, fast end) - typical for decay/release
    // curve = 0: linear
    float applyCurve(float x, float curve) const
    {
        if (std::abs(curve) < 0.01f) return x; // Linear

        if (curve < 0)
        {
            // Exponential curve (concave) - rises quickly then levels off
            float expVal = std::exp(curve);
            return (1.0f - std::exp(curve * x)) / (1.0f - expVal);
        }
        else
        {
            // Logarithmic curve (convex) - starts slow then accelerates
            float expVal = std::exp(curve);
            return std::log(1.0f + (expVal - 1.0f) * x) / curve;
        }
    }

    // Build a curved path segment using multiple line segments to approximate the curve
    void addCurvedSegment(juce::Path& path,
                          juce::Point<float> start,
                          juce::Point<float> end,
                          float curve,
                          bool isRising) const
    {
        const int numSegments = 20; // More segments = smoother curve

        for (int i = 1; i <= numSegments; ++i)
        {
            float t = static_cast<float>(i) / numSegments;

            // Apply curve to the time parameter
            float curvedT = applyCurve(t, curve);

            // Interpolate position
            float x = start.x + (end.x - start.x) * t;
            float y;

            if (isRising)
            {
                // For attack: amplitude goes from 0 to 1
                y = start.y + (end.y - start.y) * curvedT;
            }
            else
            {
                // For decay/release: amplitude goes from 1 to target
                y = start.y + (end.y - start.y) * curvedT;
            }

            path.lineTo(x, y);
        }
    }

    juce::Path buildCurvedEnvelopePath() const
    {
        juce::Path path;

        juce::Point<float> startPoint(pathBounds.getX(), pathBounds.getBottom());

        path.startNewSubPath(startPoint.x, startPoint.y);

        // Attack segment: exponential curve (fast rise, then levels off)
        // Uses negative curve for concave shape
        addCurvedSegment(path, startPoint, attackPoint, attackCurve, true);

        // Decay segment: logarithmic curve (slow start, then drops)
        // Uses positive curve for convex shape
        addCurvedSegment(path, attackPoint, decayPoint, decayCurve, false);

        // Sustain segment: horizontal line
        path.lineTo(sustainPoint.x, sustainPoint.y);

        // Release segment: logarithmic curve (slow start, then drops)
        addCurvedSegment(path, sustainPoint, releasePoint, releaseCurve, false);

        return path;
    }

    void calculateTouchpoints()
    {
        float width = pathBounds.getWidth();
        float height = pathBounds.getHeight();

        float totalTime = attackTime + decayTime + 0.4f + releaseTime;
        float attackX = (attackTime / totalTime) * width;
        float decayX = attackX + (decayTime / totalTime) * width;
        float sustainX = decayX + (0.4f / totalTime) * width;

        attackPoint = { pathBounds.getX() + attackX, pathBounds.getY() };
        decayPoint = { pathBounds.getX() + decayX, pathBounds.getY() + (1.0f - sustainLevel) * height };
        sustainPoint = { pathBounds.getX() + sustainX, pathBounds.getY() + (1.0f - sustainLevel) * height };
        releasePoint = { pathBounds.getRight(), pathBounds.getBottom() };
    }

    void drawTouchpoint(juce::Graphics& g, juce::Point<float> point, bool highlighted, const juce::String& label)
    {
        float size = highlighted ? 14.0f : 10.0f;

        // Outer glow
        if (highlighted)
        {
            g.setColour(HellcatColors::hellcatRed.withAlpha(0.4f));
            g.fillEllipse(point.x - size, point.y - size, size * 2, size * 2);
        }

        // Main circle
        g.setColour(highlighted ? HellcatColors::redBright : HellcatColors::hellcatRed);
        g.fillEllipse(point.x - size / 2, point.y - size / 2, size, size);

        // Inner highlight
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillEllipse(point.x - size / 4, point.y - size / 4, size / 2, size / 2);

        // Point label
        g.setColour(highlighted ? HellcatColors::textPrimary : HellcatColors::textSecondary);
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(9.0f));
        else
            g.setFont(juce::Font(9.0f, juce::Font::bold));

        // Position label above the point, clamped within graph bounds
        float labelY = point.y - size - 12.0f;
        if (labelY < pathBounds.getY())
            labelY = point.y + size + 2.0f; // Below if too close to top
        g.drawText(label, juce::Rectangle<float>(point.x - 8, labelY, 16, 12).toNearestInt(),
                   juce::Justification::centred);
    }

    void drawDragTooltip(juce::Graphics& g)
    {
        juce::String text;
        juce::Point<float> pos;

        switch (currentDragPoint)
        {
            case DragPoint::Attack:
                text = juce::String(juce::roundToInt(attackTime * 1000)) + "ms";
                pos = attackPoint;
                break;
            case DragPoint::Decay:
                text = juce::String(juce::roundToInt(decayTime * 1000)) + "ms";
                pos = decayPoint;
                break;
            case DragPoint::Sustain:
                text = juce::String(juce::roundToInt(sustainLevel * 100)) + "%";
                pos = sustainPoint;
                break;
            case DragPoint::Release:
                text = juce::String(juce::roundToInt(releaseTime * 1000)) + "ms";
                pos = releasePoint;
                break;
            default: return;
        }

        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
            g.setFont(lf->getOrbitronFont(10.0f));
        else
            g.setFont(juce::Font(10.0f, juce::Font::bold));

        float textWidth = g.getCurrentFont().getStringWidthFloat(text) + 12.0f;
        float tooltipX = pos.x - textWidth / 2.0f;
        float tooltipY = pos.y - 32.0f;

        // Clamp within bounds
        tooltipX = juce::jlimit(pathBounds.getX(), pathBounds.getRight() - textWidth, tooltipX);
        if (tooltipY < pathBounds.getY())
            tooltipY = pos.y + 18.0f;

        auto tooltipBounds = juce::Rectangle<float>(tooltipX, tooltipY, textWidth, 18.0f);

        // Background
        g.setColour(HellcatColors::panelDark);
        g.fillRoundedRectangle(tooltipBounds, 4.0f);
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.6f));
        g.drawRoundedRectangle(tooltipBounds, 4.0f, 1.0f);

        // Text
        g.setColour(HellcatColors::textPrimary);
        g.drawText(text, tooltipBounds.toNearestInt(), juce::Justification::centred);
    }

    DragPoint getPointAt(juce::Point<float> pos) const
    {
        const float hitRadius = 15.0f;

        if (attackPoint.getDistanceFrom(pos) < hitRadius) return DragPoint::Attack;
        if (decayPoint.getDistanceFrom(pos) < hitRadius) return DragPoint::Decay;
        if (sustainPoint.getDistanceFrom(pos) < hitRadius) return DragPoint::Sustain;
        // Release point is at bottom-right corner — use larger radius since it's at the edge
        if (releasePoint.getDistanceFrom(pos) < hitRadius * 1.5f)
            return DragPoint::Release;

        return DragPoint::None;
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
    {
        g.setColour(HellcatColors::hellcatRed.withAlpha(0.1f));

        for (int i = 0; i <= 4; ++i)
        {
            float y = area.getY() + (area.getHeight() / 4.0f) * i;
            g.drawLine(static_cast<float>(area.getX()), y, static_cast<float>(area.getRight()), y, 1.0f);
        }

        for (int i = 0; i <= 5; ++i)
        {
            float x = area.getX() + (area.getWidth() / 5.0f) * i;
            g.drawLine(x, static_cast<float>(area.getY()), x, static_cast<float>(area.getBottom()), 1.0f);
        }
    }

    void drawADSRValues(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        bounds.reduce(10, 5);
        int colWidth = bounds.getWidth() / 4;

        const juce::String labels[] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        juce::Label* valueLabelPtrs[] = { &attackValueLabel, &decayValueLabel, &sustainValueLabel, &releaseValueLabel };

        // Set font on labels during paint (when LookAndFeel is available)
        if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
        {
            auto font = lf->getOrbitronBlackFont(16.0f);
            for (auto* vl : valueLabelPtrs)
                vl->setFont(font);
        }
        else
        {
            for (auto* vl : valueLabelPtrs)
                vl->setFont(juce::Font(16.0f, juce::Font::bold));
        }

        for (int i = 0; i < 4; ++i)
        {
            auto col = bounds.removeFromLeft(colWidth);

            // Header label text
            g.setColour(HellcatColors::textTertiary);
            if (auto* lf = dynamic_cast<HellcatLookAndFeel*>(&getLookAndFeel()))
                g.setFont(lf->getOrbitronFont(9.0f));
            else
                g.setFont(juce::Font(9.0f, juce::Font::bold));
            g.drawText(labels[i], col.removeFromTop(15), juce::Justification::centred);

            // Position the editable value label
            auto valueArea = col.removeFromTop(25);
            valueLabelPtrs[i]->setBounds(valueArea);

            // Red underline below the value label
            g.setColour(HellcatColors::hellcatRed);
            auto underlineBounds = valueArea.reduced(10, 0);
            g.fillRect(underlineBounds.getX(), underlineBounds.getBottom() + 2,
                      underlineBounds.getWidth(), 3);
        }
    }

    void updateCursorForPoint(DragPoint point)
    {
        switch (point)
        {
            case DragPoint::Sustain:
                setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
                break;
            case DragPoint::Attack:
            case DragPoint::Decay:
            case DragPoint::Release:
                setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                break;
            default:
                setMouseCursor(juce::MouseCursor::NormalCursor);
                break;
        }
    }

    void updateValueLabels()
    {
        attackValueLabel.setText(juce::String(juce::roundToInt(attackTime * 1000)) + "ms", juce::dontSendNotification);
        decayValueLabel.setText(juce::String(juce::roundToInt(decayTime * 1000)) + "ms", juce::dontSendNotification);
        sustainValueLabel.setText(juce::String(juce::roundToInt(sustainLevel * 100)) + "%", juce::dontSendNotification);
        releaseValueLabel.setText(juce::String(juce::roundToInt(releaseTime * 1000)) + "ms", juce::dontSendNotification);
    }

    void labelTextChanged(juce::Label* labelThatHasChanged) override
    {
        juce::String text = labelThatHasChanged->getText().trim();
        // Strip suffix
        text = text.replace("ms", "").replace("MS", "").replace("%", "").trim();
        float numericValue = text.getFloatValue();

        if (labelThatHasChanged == &attackValueLabel)
        {
            attackTime = juce::jlimit(0.001f, 10.0f, numericValue / 1000.0f);
            if (onAttackChanged) onAttackChanged(attackTime);
        }
        else if (labelThatHasChanged == &decayValueLabel)
        {
            decayTime = juce::jlimit(0.001f, 10.0f, numericValue / 1000.0f);
            if (onDecayChanged) onDecayChanged(decayTime);
        }
        else if (labelThatHasChanged == &sustainValueLabel)
        {
            sustainLevel = juce::jlimit(0.0f, 1.0f, numericValue / 100.0f);
            if (onSustainChanged) onSustainChanged(sustainLevel);
        }
        else if (labelThatHasChanged == &releaseValueLabel)
        {
            releaseTime = juce::jlimit(0.001f, 10.0f, numericValue / 1000.0f);
            if (onReleaseChanged) onReleaseChanged(releaseTime);
        }

        updateValueLabels();
        repaint();
    }

    void editorShown(juce::Label* label, juce::TextEditor& editor) override
    {
        isEditingLabel = true;
        // Select all text for easy replacement
        editor.selectAll();
        editor.setJustification(juce::Justification::centred);
        editor.setColour(juce::TextEditor::backgroundColourId, HellcatColors::panelDark);
        editor.setColour(juce::TextEditor::textColourId, HellcatColors::textPrimary);
        editor.setColour(juce::TextEditor::highlightColourId, HellcatColors::hellcatRed.withAlpha(0.4f));
        editor.setColour(juce::TextEditor::focusedOutlineColourId, HellcatColors::hellcatRed);
    }

    void editorHidden(juce::Label*, juce::TextEditor&) override
    {
        isEditingLabel = false;
    }

    // Editable value labels
    juce::Label attackValueLabel;
    juce::Label decayValueLabel;
    juce::Label sustainValueLabel;
    juce::Label releaseValueLabel;
    bool isEditingLabel = false;

    float attackTime = 0.045f;
    float decayTime = 0.28f;
    float sustainLevel = 0.65f;
    float releaseTime = 0.52f;

    // Curve parameters: negative = exponential (concave), positive = logarithmic (convex)
    // These match the ADSR DSP curve parameters
    float attackCurve = -3.0f;   // Exponential attack (fast rise, levels off)
    float decayCurve = 3.0f;     // Logarithmic decay (slow start, then drops)
    float releaseCurve = 3.0f;   // Logarithmic release (slow start, then drops)

    // Touchpoint positions (calculated in paint)
    juce::Point<float> attackPoint;
    juce::Point<float> decayPoint;
    juce::Point<float> sustainPoint;
    juce::Point<float> releasePoint;

    // Dragging state
    DragPoint currentDragPoint = DragPoint::None;
    DragPoint hoveredPoint = DragPoint::None;

    // Cached bounds
    juce::Rectangle<int> graphBounds;
    juce::Rectangle<float> pathBounds;
};
