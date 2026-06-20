# Smart Water Tap

A hand-sensing automatic tap built on ESP32. A Time-of-Flight sensor detects a hand under the spout, opens a solenoid valve for a fixed duration, gives a visual warning before closing, and then locks out for a short cooldown. The system is designed to scale from a single standalone unit to a network of tap nodes reporting to a central master node over ESP-NOW, with the master bridging that data to a server and Telegram bot.

## Status

Currently in a single-unit testing phase (6-month trial). The local sensing/valve/LED logic is complete. The Master/Daughter networking layer is implemented but unpaired — MAC addresses and WiFi channel still need to be configured per the **Pairing** section below before ESP-NOW will work.

## BoM (Bill of Materterials)

| PART NAME | PART NUMBER | PART PRICE | URL | Quantity |
| --------- | ----------- | ---------- | --- | -------- |
| Microcontroller | ESP32 | Rs. 300 | OFFLINE | 2 |
| MOSFET | IRLFZ44 |  Rs. 100 | OFFLINE | 2 |
| ToF   | VL53L0X | Rs. 200 | OFFLINE | 1 |
| Valve | ------ | Rs. 300 | Robu.in | 1 |
| Voltage Booster | MT3608 | Rs. 30 | OFFLINE | 1 |
| PCB/Perfboard | zero/Professional | < Rs. 700/50 | JLC/OFFLINE | 3 |
| PSU 12v | 12V 2A | Rs. 150 | OFFLINE | 1 |
| 5v Step Down | LM7805 | Rs. 30 | OFFLINE | 4 |
| 3.3v Step Down | AMS1117 3.3 | Rs. 47 | Robu.in | 2 |
| Diode | 1N4007 | Rs. 1 | OFFLINE | 10 |
| Capacitor | vlaues given below| Rs. 10 | OFFLINE | 4 |
| **TOTAL** | | **Rs.2450** |    |  |

Capacitors: 10μF, 0.1μF (x2), 0.22μF
