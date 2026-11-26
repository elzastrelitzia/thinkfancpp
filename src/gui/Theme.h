#pragma once

#include <QPalette>
#include <QString>
#include <Qt>
#include <cstdint>

namespace thinkfan {

enum class ThemeMode {
    Light,
    Dark,
};

struct ThemeTokens {
    QString warningStyle;
    QString dangerButtonStyle;
    QString statusErrorStyle;
    QString statusWarnStyle;
    QString enabledStyle;
    QString disabledStyle;
    QString unavailableStyle;
    QPalette palette;
};

ThemeMode themeModeFromPortalValue(uint32_t value);
ThemeMode themeModeFromColorScheme(Qt::ColorScheme scheme);
const ThemeTokens& tokensFor(ThemeMode mode);

} // namespace thinkfan
