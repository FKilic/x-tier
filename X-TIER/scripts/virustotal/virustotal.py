#!/usr/bin/python

import simplejson
import urllib
import urllib2
import sys

url = "https://www.virustotal.com/vtapi/v2/file/report"
parameters = {"resource": sys.argv[1], "apikey": "91e0f8190363c5eb1ae2efa72675e3b0e185b9c1276cee4b789fce3eed553bc3"}
data = urllib.urlencode(parameters)
req = urllib2.Request(url, data)
response = urllib2.urlopen(req)
json = response.read()
response_dict = simplejson.loads(json)

if response_dict.get("response_code") == 1:
	sys.exit(response_dict.get("positives"))

sys.exit(0)
