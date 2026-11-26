#pragma once

#include <QPalette>
#include <QString>
#include <Qt>
#include <QVariant>
#include <cstdint>
#include <optional>

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
std::optional<uint32_t> parsePortalColorScheme(const QVariant& value);
const ThemeTokens& tokensFor(ThemeMode mode);

} // namespace thinkfan
