#include <cassert>
#include <cstdint>

#include <QColor>
#include <QDBusVariant>
#include <QVariant>

#include "gui/Theme.h"

int main() {
    using thinkfan::ThemeMode;
    using thinkfan::parsePortalColorScheme;
    using thinkfan::qmlTokensFor;
    using thinkfan::themeModeFromPortalValue;
    using thinkfan::tokensFor;

    assert(themeModeFromPortalValue(1) == ThemeMode::Dark);
    assert(themeModeFromPortalValue(0) == ThemeMode::Light);
    assert(themeModeFromPortalValue(2) == ThemeMode::Light);
    assert(themeModeFromPortalValue(99) == ThemeMode::Light);

    const auto& dark = tokensFor(ThemeMode::Dark);
    const auto& light = tokensFor(ThemeMode::Light);
    assert(dark.statusErrorStyle != light.statusErrorStyle);
    assert(dark.warningStyle.contains("#"));
    assert(light.warningStyle.contains("#"));

    const auto& qdark = qmlTokensFor(ThemeMode::Dark);
    const auto& qlight = qmlTokensFor(ThemeMode::Light);
    assert(qdark.window.name(QColor::HexRgb) == QStringLiteral("#1e1f22"));
    assert(qlight.window.name(QColor::HexRgb) == QStringLiteral("#f5f5f5"));
    assert(qdark.text != qlight.text);
    assert(qdark.warningText != qlight.warningText);
    assert(qdark.error != qlight.error);
    assert(qdark.success != qlight.success);
    assert(qdark.muted.alpha() > 0);
    assert(qdark.dangerButtonText.isValid());

    const QVariant nested = QVariant::fromValue(
        QDBusVariant(QVariant::fromValue(QDBusVariant(QVariant::fromValue(uint32_t{1})))));
    const auto nestedValue = parsePortalColorScheme(nested);
    assert(nestedValue.has_value() && *nestedValue == 1);

    const auto plainValue = parsePortalColorScheme(QVariant::fromValue(uint32_t{2}));
    assert(plainValue.has_value() && *plainValue == 2);

    const auto invalidValue = parsePortalColorScheme(QVariant(QStringLiteral("nope")));
    assert(!invalidValue.has_value());

    return 0;
}
