A project for Computer Science studies.
Clock consist of round PCB, ATmega328p with MiniCore bootloader driven by 12MHz crystal.
Clock IC is DS3231, time is displayed using 60 LEDs and 4 digit 7 segment display in the middle.
LEDs show seconds, 4digit can display hours, minutes, date, month and year.
Under the display 4 pushbuttons are used to change displayed values and program time.
DS3231 has CR2032 lithium battery power backup for keeping time without clock main power.
LEDs are driven by 8 HC595 shift registers, 4 digit display uses 2 HC595s.
Power is supplied by Liion battery through small 3.7-5V booster alongside with TP4056 charger.
