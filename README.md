# Smart Water Tap

A hand-sensing automatic tap built on ESP32. A Time-of-Flight sensor detects a hand under the spout, opens a solenoid valve for a fixed duration, gives a visual warning before closing, and then locks out for a short cooldown. The system is designed to scale from a single standalone unit to a network of tap nodes reporting to a central master node over ESP-NOW, with the master bridging that data to a server and Telegram bot.

## Status

Currently in a single-unit testing phase (6-month trial). The local sensing/valve/LED logic is complete. The Master/Daughter networking layer is implemented but unpaired — MAC addresses and WiFi channel still need to be configured per the **Pairing** section below before ESP-NOW will work.
