# VoiceControlledWheelchair
A mixture of resources I used to test making a voice controlled wheelchair.
More info at: http://bobparadiso.com/2015/07/17/amazon-echo-controlled-wheelchair/

corpus.txt is used to generate the language model for PocketSphinx, you can use it with this tool:
http://www.speech.cs.cmu.edu/tools/lmtool-new.html

wheelchairControl_Arduino.ino runs on an Arduino Uno and directly drives the power wheelchair base and other hardware like the RF transmitter.

esp8266_wheelchair.ino runs on an ESP8266 WiFi breakout and receives HTTP requests and passes the relevant control info to the Uno.

wheelchair_sphinx_*.py are versions of a python script that run on a Raspberry Pi, wrap PocketSphinx, grab the decoded speech, and send the commands over WiFi to the wheelchair.
