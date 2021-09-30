# Mouse multiplexer

This is Arduino code that makes it possible to have two mouse cursors when connecting two USB mice to the same machine. It works on Windows, Linux and Mac with no additional software on the computer.

[Demo video.](https://www.youtube.com/watch?v=XYER6yjNt-I)

It runs on an Arduino Leonardo (or similar) with a USB Host Shield. The two mice should be connected to the Arduino (through the shield) using a USB hub.

The trick is that it internally keeps separate screen positions for both mice and alternates between them when sending a report to the computer. This results in two (rapidly blinking) mouse cursors. To achieve this, we use absolute cursor positioning instead of reporting relative movement that's normally done by mice.

To make clicks work, we temporarily give exclusive ownership of the cursor to the mouse that's currently pressing buttons.
