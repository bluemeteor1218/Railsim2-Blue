#pragma once

// Maps the legacy fixed UI coordinate space (e.g. 640x480) onto a window of any size.

#include <algorithm>
#include <cmath>

namespace rs2 {

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
};

enum class LayoutMode {
    Fit,         // keep the virtual size, letterbox to preserve aspect ratio
    FixedHeight, // keep the virtual height, widen the virtual width to the window aspect
};

class ViewportLayout {
public:
    // virtualWidth/Height: the design resolution of the UI (legacy: 640x480).
    // windowWidth/Height: client size in physical pixels.
    ViewportLayout(float virtualWidth, float virtualHeight, float windowWidth, float windowHeight,
                   LayoutMode mode = LayoutMode::Fit)
    {
        windowW_ = std::max(windowWidth, 1.0f);
        windowH_ = std::max(windowHeight, 1.0f);
        virtualW_ = std::max(virtualWidth, 1.0f);
        virtualH_ = std::max(virtualHeight, 1.0f);

        if (mode == LayoutMode::FixedHeight) {
            scale_ = windowH_ / virtualH_;
            virtualW_ = windowW_ / scale_;
            viewport_ = {0.0f, 0.0f, windowW_, windowH_};
        } else {
            scale_ = std::min(windowW_ / virtualW_, windowH_ / virtualH_);
            const float w = virtualW_ * scale_;
            const float h = virtualH_ * scale_;
            viewport_ = {std::floor((windowW_ - w) * 0.5f), std::floor((windowH_ - h) * 0.5f), w, h};
        }
    }

    float Scale() const { return scale_; }
    float VirtualWidth() const { return virtualW_; }
    float VirtualHeight() const { return virtualH_; }
    const Rect& Viewport() const { return viewport_; }

    // Window pixels -> virtual coordinates (mouse input).
    void ToVirtual(float px, float py, float* vx, float* vy) const
    {
        *vx = (px - viewport_.x) / scale_;
        *vy = (py - viewport_.y) / scale_;
    }

    // Virtual coordinates -> window pixels.
    void ToWindow(float vx, float vy, float* px, float* py) const
    {
        *px = vx * scale_ + viewport_.x;
        *py = vy * scale_ + viewport_.y;
    }

    bool ContainsWindowPoint(float px, float py) const
    {
        return px >= viewport_.x && py >= viewport_.y && px < viewport_.x + viewport_.w &&
               py < viewport_.y + viewport_.h;
    }

private:
    float windowW_ = 1, windowH_ = 1, virtualW_ = 1, virtualH_ = 1, scale_ = 1;
    Rect viewport_;
};

// Display scale factor for a DPI value (96 DPI = 1.0), snapped to 25% steps so
// text and 1px lines stay crisp.
inline float DpiScale(unsigned dpi)
{
    const float raw = static_cast<float>(dpi) / 96.0f;
    return std::max(1.0f, std::round(raw * 4.0f) / 4.0f);
}

} // namespace rs2
