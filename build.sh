#!/usr/bin/env bash
set -euo pipefail

dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="$dir/build"

case "${1:-}" in
  --clean)
    rm -rf "$build_dir"
    exit 0
    ;;
  --uninstall)
    sudo rm -f /usr/local/bin/thinkfancpp /usr/local/bin/thinkfancpp-helper \
      /usr/local/share/polkit-1/actions/com.thinkfancpp.helper.policy \
      /usr/local/share/applications/thinkfancpp.desktop \
      /usr/local/share/icons/hicolor/1280x1280/apps/thinkfancpp.png \
      /usr/local/share/pixmaps/thinkfancpp.png
    exit 0
    ;;
  --install)
    cmake -S "$dir" -B "$build_dir" -DBUILD_TESTING=ON
    cmake --build "$build_dir"
    sudo cmake --install "$build_dir" --prefix /usr/local
    exit 0
    ;;
esac

cmake -S "$dir" -B "$build_dir" -DBUILD_TESTING=ON
cmake --build "$build_dir"
exec "$build_dir/thinkfancpp" "$@"
