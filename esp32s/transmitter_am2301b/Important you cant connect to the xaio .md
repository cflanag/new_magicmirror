Important you cant connect to the xaio esp32 because it is only awake 1 second the deep sleep it has no separate usb
chip so it is hard to catch it awake you need to press the boot button holde down the one click on the reset button
release the boot button then the ttyACM* will show up and you can upload

to resync the clock add some numbers or words to recompile the set_clock.cpp