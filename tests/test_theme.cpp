#include <cassert>
#include "gui/Theme.h"

int main() {
    using thinkfan::ThemeMode;
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
    return 0;
}
