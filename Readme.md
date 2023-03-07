
# Arduino I2C Bridge

This project was created as part of my bachelor thesis.

It implements a simple protocol for sending and receiving I2C messages using an Arduino Uno over Serial.

## Commands

STX is the ASCII Code for Start Transmission (0x02)  
ETX is the ASCII Code for End Transmission (0x03)  
Single letters are literals.  
Lengths above or equal to 255 can be expressed as multiple bytes which are summed. (256 = 0xFF 0x01)

```
Write x bytes:
[STX] [w] [send stop] [write address] [write length] [write data] [ETX]
[STX] [r] [send stop] [read address]  [read length]               [ETX]

Replies:
[STX] [w] [write address]  [status]       [write length] [write data] [ETX]
[STX] [r] [read address]   [status]       [read length]  [read data]  [ETX]
[STX] [e] [error length]   [error text]                               [ETX]
[STX] [d] [message length] [message text]                             [ETX]
```
