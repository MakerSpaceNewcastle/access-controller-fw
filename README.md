# RFID_access_controller
This is a PCB design for an access controller based on an ESP-12 ESP8266 based board.

Supports:   

	Onboard DC regulator (5VDC->3v3 to supply board
	Sainsmart MiFare RC522 RFID card reader
	RGB LED (WS2812) for status
	Relay output to activate/deactivate the protected device
	I2C pins broken out for use with i2c LCD or I/O expanders
	Standard FTDI pin out for programming, and jumper to select program/run (pulls GPIO0 low)
