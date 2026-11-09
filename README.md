## Prototype code for a wireless eBell (to be paired with WemosEBell-server)

The client runs in one of two modes:

### Maintenance mode

Entered if a recognised configuration is not detected in EEPROM, or if the button is held down for 2s at powerup.  

The bell starts a local SSID, default password 12341234.  It runs a local web server, allowing:
- the bell to be calibrated
- the name of the eBell server to be entered

### Run mode

If a recognised configuration is detected, it will boot straight into operate mode.  It will:

- attempt to connect to the configured SSID
- send a registration message to the server
- once registration is acknowledged, send strike messages every time strike thresholds are crossed
