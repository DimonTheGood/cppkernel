#pragma once
#include <cstdint>

constexpr uint32_t PIT_FREQUENCY = 100;

void pit_init();
void pit_tick();
uint32_t pit_ticks();