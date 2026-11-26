#include "gui/Theme.h"

#include <QColor>

namespace thinkfan {

namespace {

QPalette makeDarkPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#1e1f22"));
    palette.setColor(QPalette::WindowText, QColor("#e6e6e6"));
    palette.setColor(QPalette::Base, QColor("#2b2d31"));
    palette.setColor(QPalette::AlternateBase, QColor("#232529"));
    palette.setColor(QPalette::ToolTipBase, QColor("#2b2d31"));
    palette.setColor(QPalette::ToolTipText, QColor("#e6e6e6"));
    palette.setColor(QPalette::Text, QColor("#e6e6e6"));
    palette.setColor(QPalette::Button, QColor("#2b2d31"));
    palette.setColor(QPalette::ButtonText, QColor("#e6e6e6"));
    palette.setColor(QPalette::BrightText, QColor("#ff6b6b"));
    palette.setColor(QPalette::Highlight, QColor("#3d6fb4"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::PlaceholderText, QColor("#9a9a9a"));
    return palette;
}

QPalette makeLightPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#f5f5f5"));
    palette.setColor(QPalette::WindowText, QColor("#1f1f1f"));
    palette.setColor(QPalette::Base, QColor("#ffffff"));
    palette.setColor(QPalette::AlternateBase, QColor("#ededed"));
    palette.setColor(QPalette::ToolTipBase, QColor("#ffffff"));
    palette.setColor(QPalette::ToolTipText, QColor("#1f1f1f"));
    palette.setColor(QPalette::Text, QColor("#1f1f1f"));
    palette.setColor(QPalette::Button, QColor("#f0f0f0"));
    palette.setColor(QPalette::ButtonText, QColor("#1f1f1f"));
    palette.setColor(QPalette::BrightText, QColor("#b00020"));
    palette.setColor(QPalette::Highlight, QColor("#2f6feb"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    palette.setColor(QPalette::PlaceholderText, QColor("#6b6b6b"));
    return palette;
}

const ThemeTokens& darkTokens() {
    static const ThemeTokens tokens{
        QStringLiteral(
            "QLabel {"
            "  background-color: #3d3418;"
            "  border: 1px solid #a67c00;"
            "  border-radius: 4px;"
            "  padding: 8px;"
            "  color: #ffd966;"
            "  font-weight: bold;"
            "}"
        ),
        QStringLiteral(
            "QPushButton {"
            "  background-color: #5a1d24;"
            "  border: 1px solid #8b2c36;"
            "  color: #ffb3b8;"
            "  padding: 6px 12px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #6e2430;"
            "}"
            "QPushButton:disabled {"
            "  background-color: #3a3a3a;"
            "  border: 1px solid #555555;"
            "  color: #777777;"
            "}"
        ),
        QStringLiteral("color: #ff6b6b; font-weight: bold;"),
        QStringLiteral("color: #ffd966; font-weight: bold;"),
        QStringLiteral("color: #5fd38d;"),
        QStringLiteral("color: #ff6b6b;"),
        QStringLiteral("color: #9a9a9a;"),
        makeDarkPalette(),
    };
    return tokens;
}

const ThemeTokens& lightTokens() {
    static const ThemeTokens tokens{
        QStringLiteral(
            "QLabel {"
            "  background-color: #fff4cc;"
            "  border: 1px solid #c99700;"
            "  border-radius: 4px;"
            "  padding: 8px;"
            "  color: #664d00;"
            "  font-weight: bold;"
            "}"
        ),
        QStringLiteral(
            "QPushButton {"
            "  background-color: #f8d7da;"
            "  border: 1px solid #dc3545;"
            "  color: #842029;"
            "  padding: 6px 12px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #f1aeb5;"
            "}"
            "QPushButton:disabled {"
            "  background-color: #e9ecef;"
            "  border: 1px solid #ced4da;"
            "  color: #6c757d;"
            "}"
        ),
        QStringLiteral("color: #c62828; font-weight: bold;"),
        QStringLiteral("color: #8a6d00; font-weight: bold;"),
        QStringLiteral("color: #1b7f3a;"),
        QStringLiteral("color: #c62828;"),
        QStringLiteral("color: #6c757d;"),
        makeLightPalette(),
    };
    return tokens;
}

} // namespace

ThemeMode themeModeFromPortalValue(uint32_t value) {
    return value == 1 ? ThemeMode::Dark : ThemeMode::Light;
}

ThemeMode themeModeFromColorScheme(Qt::ColorScheme scheme) {
    return scheme == Qt::ColorScheme::Dark ? ThemeMode::Dark : ThemeMode::Light;
}

const ThemeTokens& tokensFor(ThemeMode mode) {
    return mode == ThemeMode::Dark ? darkTokens() : lightTokens();
}

} // namespace thinkfan
