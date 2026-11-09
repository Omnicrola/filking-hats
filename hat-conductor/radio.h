#pragma once

#include <Arduino.h>

struct HatInfo {
  uint8_t mac[6];
  int8_t rssi;
  uint16_t version;
  uint32_t firstMs;
  uint32_t lastMs;
};

bool radioBegin();
void broadcast(uint8_t level, uint8_t hue, uint8_t mode);
int radioReceiverCount();
int radioHats(HatInfo *out, int max);
uint8_t radioChannel();
uint8_t radioPendingChannel();
bool radioRequestChannel(uint8_t channel);
