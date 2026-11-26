#include <cassert>
#include "core/FanTypes.h"

int main() {
  thinkfan::ProcFanState state;
  assert(!state.enabled);
  assert(state.speed_rpm == -1);
  assert(state.level == "unknown");
  assert(thinkfan::FanMode::Auto != thinkfan::FanMode::Manual);
  return 0;
}