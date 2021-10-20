# RAK4631_NMEA_Parser

This is a tokenizer and parser for raw NMEA sentences. This is not intended (yet anyway) for production, but as an exercice in re-inventing the wheel. There are other reasons for me to do this, including a smaller footprint (I need that code to be as small as possible). Suffice to say, I had good enough reasons to do this, and am happy to put it up there for others to enjoy. Or not.

So far it recognizes 7 NMEA verbs, the most common my ublox module spits out:

* $GPRMC
* $GPGSV
* $GPGGA
* $GPGLL
* $GPGSA
* $GPVTG
* $GPTXT

Incoming sentences are stored into a stack, then the code reads every sentence, and tries to fail, if it has to, as fast as possible, in order to move on to the next sentence, and empty the stack.

Data that is identical to a previous fix (SIV, lat/long) is not repeated in the Serial terminal, in order to make it a little more readable. Ideally, all this data shouldn't be dumped to Serial, but displayed on an OLED or TFT at fixed positions.