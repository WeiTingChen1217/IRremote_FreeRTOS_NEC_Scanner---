# IRremote + FreeRTOS NEC Scanner

這是一個基於 Arduino UNO 的紅外線掃描器，使用 IRremote v4.5.0 與 FreeRTOS 架構。

## 功能
- 支援 NEC 指令發送與接收
- 支援序列指令解析：`sendnec`, `scannec`, `read`
- 掃描整合進接收 Task，節省記憶體

## 使用方式
- `sendnec 0x00 0x46 2`：發送 NEC 指令
- `scannec`：掃描所有 NEC 指令（Address=0x00）
