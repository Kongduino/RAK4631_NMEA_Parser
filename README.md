# RAK4631_NMEA_Parser

This is a tokenizer and parser for raw NMEA sentences. This is not intended (yet anyway) for production, but as an exercice in re-inventing the wheel. There are other reasons for me to do this, including a smaller footprint (I need that code to be as small as possible). Suffice to say, I had good enough reasons to do this, and am happy to put it up there for others to enjoy. Or not.

So far it recognizes 8 NMEA verbs, the most common the [ublox module](https://store.rakwireless.com/collections/wisblock-sensor/products/rak1910-max-7q-gnss-location-sensor) spits out:

* $GPRMC
* $GPGSV
* $GPGGA
* $GPZDA (if available)
* $GPGLL
* $GPGSA
* $GPVTG
* $GPTXT

Incoming sentences are stored into a stack, then the code reads every sentence, and tries to fail, if it has to, as fast as possible, in order to move on to the next sentence, and empty the stack.

Data that is identical to a previous fix (SIV, lat/long) is not repeated in the Serial terminal, in order to make it a little more readable, unless `fixInterval` milliseconds have passed.

Ideally, all this data shouldn't be dumped to Serial, but displayed on an OLED or TFT at fixed positions.

## BLE

I added BLE UART output. You get data for:

* $GPRMC (UTC time, coordinates)
* $GPGGA (UTC time, coordinates)
* $GPZDA (UTC date & time, if available)
* $GPGLL (coordinates)
* $GPGSV (SIV)
* $GPTXT (Text messages)

### More information

For more information about the Wisblock GNSS module, see the [Quick start Guide](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK1910/Quickstart/) on RAK's Documentation Center website.