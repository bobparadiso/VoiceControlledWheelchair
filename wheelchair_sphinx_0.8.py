#!/usr/bin/python

import time
import datetime
import math
import sys, select, subprocess
import urllib2
import os

languageModel =  sys.argv[1]
adcdev =  sys.argv[2]

def speak(text):
	os.system('espeak "' + text + '" --stdout | aplay -D plughw:1,0');	
	#print text

def getUrl(url):
	urllib2.urlopen(url)


print "starting"
speak("espeak functional")
   
def main():
	while(True):
		time.sleep(0.05)
   
proc = subprocess.Popen(['sh', '-c', 'pocketsphinx_continuous -inmic yes -adcdev ' + adcdev + ' -samprate 16000/8000/48000 -lm ~/languageModel/' + languageModel + '.lm -dict ~/languageModel/' + languageModel + '.dic 2>/dev/null'],stdout=subprocess.PIPE)

while True:
	line = proc.stdout.readline()
	if line != '':
		#the real code does filtering here
		output = line.rstrip()
		print output
		if (len(output.split("READY"))>1):
			print "Ready" + '\r\n'
		#if (len(output.split("please wait"))>1):
		#	speak("Please wait")
		if (len(output.split(":"))>1):
			detected = str(output.split(":")[1])
			speak(detected)
			
			if (detected == " JAZZY LEFT ONE"):
				getUrl("http://192.168.0.2/wc/lt1")
			elif (detected == " JAZZY LEFT TWO"):
				getUrl("http://192.168.0.2/wc/lt2")
			elif (detected == " JAZZY LEFT THREE"):
				getUrl("http://192.168.0.2/wc/lt3")
			elif (detected == " JAZZY LEFT FOUR"):
				getUrl("http://192.168.0.2/wc/lt4")
			elif (detected == " JAZZY LEFT FIVE"):
				getUrl("http://192.168.0.2/wc/lt5")
			
			elif (detected == " JAZZY RIGHT ONE"):
				getUrl("http://192.168.0.2/wc/rt1")
			elif (detected == " JAZZY RIGHT TWO"):
				getUrl("http://192.168.0.2/wc/rt2")
			elif (detected == " JAZZY RIGHT THREE"):
				getUrl("http://192.168.0.2/wc/rt3")
			elif (detected == " JAZZY RIGHT FOUR"):
				getUrl("http://192.168.0.2/wc/rt4")
			elif (detected == " JAZZY RIGHT FIVE"):
				getUrl("http://192.168.0.2/wc/rt5")
			
			elif (detected == " JAZZY FORWARD ONE"):
				getUrl("http://192.168.0.2/wc/fwd1")
			elif (detected == " JAZZY FORWARD TWO"):
				getUrl("http://192.168.0.2/wc/fwd2")
			elif (detected == " JAZZY FORWARD THREE"):
				getUrl("http://192.168.0.2/wc/fwd3")
			elif (detected == " JAZZY FORWARD FOUR"):
				getUrl("http://192.168.0.2/wc/fwd4")
			elif (detected == " JAZZY FORWARD FIVE"):
				getUrl("http://192.168.0.2/wc/fwd5")
			
			elif (detected == " JAZZY BACKWARD ONE"):
				getUrl("http://192.168.0.2/wc/rev1")
			elif (detected == " JAZZY BACKWARD TWO"):
				getUrl("http://192.168.0.2/wc/rev2")
			elif (detected == " JAZZY BACKWARD THREE"):
				getUrl("http://192.168.0.2/wc/rev3")
			elif (detected == " JAZZY BACKWARD FOUR"):
				getUrl("http://192.168.0.2/wc/rev4")
			elif (detected == " JAZZY BACKWARD FIVE"):
				getUrl("http://192.168.0.2/wc/rev5")
			
			elif (detected == " JAZZY REVERSE ONE"):
				getUrl("http://192.168.0.2/wc/rev1")
			elif (detected == " JAZZY REVERSE TWO"):
				getUrl("http://192.168.0.2/wc/rev2")
			elif (detected == " JAZZY REVERSE THREE"):
				getUrl("http://192.168.0.2/wc/rev3")
			elif (detected == " JAZZY REVERSE FOUR"):
				getUrl("http://192.168.0.2/wc/rev4")
			elif (detected == " JAZZY REVERSE FIVE"):
				getUrl("http://192.168.0.2/wc/rev5")
			
			elif (detected == " JAZZY STOP"):
				getUrl("http://192.168.0.2/wc/brake")
			
			elif (detected == " JAZZY BRAKE"):
				getUrl("http://192.168.0.2/wc/brake")
			
			elif (detected == " JAZZY HALT"):
				getUrl("http://192.168.0.2/wc/brake")

			elif (detected == " JAZZY FAN ON"):
				getUrl("http://192.168.0.2/fan/on")

			elif (detected == " JAZZY FAN OFF"):
				getUrl("http://192.168.0.2/fan/off")

	else:
		print "SPHINX ERROR: aborting"
		break

