from datetime import datetime
import hashlib
import urllib.request
import time
ip = "192.168.43.187"
while 1:
    now = datetime.now()
    hr = int(now.strftime("%H"))
    mn = int(now.strftime("%M"))
    htime = str(hr)+":"+str(mn)
    #print("Current Time =", htime)
    g2on="2"+htime+"on"
    g2off="2"+htime+"off"
    g16on="16"+htime+"on"
    g16off="16"+htime+"off"
    urllib.request.urlopen('http://'+ip+'/'+hashlib.md5(g2on.encode('utf-8')).hexdigest())
    urllib.request.urlopen('http://'+ip+'/'+hashlib.md5(g16off.encode('utf-8')).hexdigest())
    time.sleep(1)
    urllib.request.urlopen('http://'+ip+'/'+hashlib.md5(g2off.encode('utf-8')).hexdigest())
    urllib.request.urlopen('http://'+ip+'/'+hashlib.md5(g16on.encode('utf-8')).hexdigest())
    time.sleep(1)
