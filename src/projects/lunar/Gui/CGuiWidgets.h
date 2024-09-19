#pragma once

typedef int PanelFlags;
typedef int Condition;

enum PanelFlags_
{
    PanelFlags_None = 0,
    PanelFlags_NoTitleBar = 1 << 0,
    PanelFlags_NoResize = 1 << 1,
    PanelFlags_NoMove = 1 << 2,
    PanelFlags_NoScrollbar = 1 << 3,
    PanelFlags_NoScrollWithMouse = 1 << 4,
    PanelFlags_NoCollapse = 1 << 5,
    PanelFlags_AlwaysAutoResize = 1 << 6,
    PanelFlags_NoBackground = 1 << 7,
    PanelFlags_NoMouseInputs = 1 << 9,
    PanelFlags_MenuBar = 1 << 10,
    PanelFlags_HorizontalScrollbar = 1 << 11,
    PanelFlags_NoFocusOnAppearing = 1 << 12,
    PanelFlags_NoBringToFrontOnFocus = 1 << 13,
    PanelFlags_AlwaysVerticalScrollbar = 1 << 14,
    PanelFlags_AlwaysHorizontalScrollbar = 1 << 15,
};

enum Condition_
{
    Condition_None = 0,
    Condition_Always = 1 << 0,
    Condition_Once = 1 << 1,
    Condition_FirstUseEver = 1 << 2,
    Condition_Appearing = 1 << 3,
};

namespace LunarGui {
    struct Vector2 // to avoid mixing with game's vector defs
    {
        float x, y;
        constexpr Vector2() : x(0.0f), y(0.0f) {}
        constexpr Vector2(float _x, float _y) : x(_x), y(_y) {}
    };
}

class CGuiWidgets
{
public:
    // Window
    bool Begin(const char* name, bool* p_open = 0, PanelFlags flags = 0);
    void End();

    // Labels
    void Label(const char* fmt, ...);

    // Inputs & Buttons
    bool Button(const char* label, const LunarGui::Vector2& size = LunarGui::Vector2(0, 0));

    // Helpers
    LunarGui::Vector2 GetCurrentWindowPos();
    LunarGui::Vector2 GetCurrentWindowSize();

    void SetCurrentWindowPos(const LunarGui::Vector2& pos, Condition cond = 0);
    void SetCurrentWindowSize(const LunarGui::Vector2& size, Condition cond = 0);
};

// Operators

static inline LunarGui::Vector2  operator*(const LunarGui::Vector2& lhs, const float rhs) { return LunarGui::Vector2(lhs.x * rhs, lhs.y * rhs); }
static inline LunarGui::Vector2  operator/(const LunarGui::Vector2& lhs, const float rhs) { return LunarGui::Vector2(lhs.x / rhs, lhs.y / rhs); }
static inline LunarGui::Vector2  operator+(const LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { return LunarGui::Vector2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline LunarGui::Vector2  operator-(const LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { return LunarGui::Vector2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline LunarGui::Vector2  operator*(const LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { return LunarGui::Vector2(lhs.x * rhs.x, lhs.y * rhs.y); }
static inline LunarGui::Vector2  operator/(const LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { return LunarGui::Vector2(lhs.x / rhs.x, lhs.y / rhs.y); }
static inline LunarGui::Vector2  operator-(const LunarGui::Vector2& lhs) { return LunarGui::Vector2(-lhs.x, -lhs.y); }
static inline LunarGui::Vector2& operator*=(LunarGui::Vector2& lhs, const float rhs) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline LunarGui::Vector2& operator/=(LunarGui::Vector2& lhs, const float rhs) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline LunarGui::Vector2& operator+=(LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline LunarGui::Vector2& operator-=(LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline LunarGui::Vector2& operator*=(LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
static inline LunarGui::Vector2& operator/=(LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }
static inline bool    operator==(const LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
static inline bool    operator!=(const LunarGui::Vector2& lhs, const LunarGui::Vector2& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }