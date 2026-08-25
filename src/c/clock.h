#pragma once

typedef struct {
  int hour;
  int minute;
} ClockTime;

void clock_time_set(ClockTime *clock_time, int hour, int minute);
int clock_time_in_minutes(const ClockTime *clock_time);
