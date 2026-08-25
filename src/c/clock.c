#include "clock.h"

void clock_time_set(ClockTime *clock_time, int hour, int minute) {
  clock_time->hour = hour;
  clock_time->minute = minute;
}

int clock_time_in_minutes(const ClockTime *clock_time) {
  return clock_time->hour * 60 + clock_time->minute;
}
