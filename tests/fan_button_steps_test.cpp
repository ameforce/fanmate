#include "fan_button_steps.h"

#include <cstdlib>
#include <iostream>

namespace {

void expectEqual(int actual, int expected, const char *label) {
  if (actual != expected) {
    std::cerr << label << ": expected " << expected << ", got " << actual
              << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  expectEqual(fanButtonShortUp(0), 15, "0 UP");
  expectEqual(fanButtonShortUp(7), 15, "7 UP");
  expectEqual(fanButtonShortUp(15), 30, "15 UP");
  expectEqual(fanButtonShortUp(90), 100, "90 UP");
  expectEqual(fanButtonShortUp(100), 100, "100 UP");

  expectEqual(fanButtonShortDown(100), 90, "100 DOWN");
  expectEqual(fanButtonShortDown(90), 75, "90 DOWN");
  expectEqual(fanButtonShortDown(75), 60, "75 DOWN");
  expectEqual(fanButtonShortDown(60), 45, "60 DOWN");
  expectEqual(fanButtonShortDown(45), 30, "45 DOWN");
  expectEqual(fanButtonShortDown(30), 15, "30 DOWN");
  expectEqual(fanButtonShortDown(15), 0, "15 DOWN");
  expectEqual(fanButtonShortDown(7), 0, "7 DOWN");
  expectEqual(fanButtonShortDown(0), 0, "0 DOWN");

  expectEqual(fanButtonRepeatUp(7), 8, "hold UP repeats by one");
  expectEqual(fanButtonRepeatDown(7), 6, "hold DOWN repeats by one");
  expectEqual(fanButtonRepeatUp(100), 100, "hold UP clamps at 100");
  expectEqual(fanButtonRepeatDown(0), 0, "hold DOWN clamps at 0");

  FanButtonStepTracker upHold;
  upHold.pressed();
  int heldUpValue = upHold.repeatUp(7);
  expectEqual(heldUpValue, 8, "held UP first repeat");
  expectEqual(upHold.shortUp(heldUpValue), 8, "held UP release skips short step");
  upHold.released();

  FanButtonStepTracker downHold;
  downHold.pressed();
  int heldDownValue = downHold.repeatDown(7);
  expectEqual(heldDownValue, 6, "held DOWN first repeat");
  expectEqual(downHold.shortDown(heldDownValue),
              6,
              "held DOWN release skips short step");
  downHold.released();

  FanButtonStepTracker shortClick;
  shortClick.pressed();
  expectEqual(shortClick.shortUp(7), 15, "short UP applies coarse step");
  shortClick.released();

  std::cout << "Fan button step check passed\n";
  return 0;
}
