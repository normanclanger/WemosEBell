#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

constexpr uint8_t STROKE_HAND = 1;
constexpr uint8_t STROKE_BACK = 2;

void networkLoop();
void sendStrike(uint8_t stroke);

bool isRegistered();
uint8_t getBellNumber();

#endif