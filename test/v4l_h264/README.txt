V4L-H264
========
The goal of the sample code is to demonstrate the H264 encoding and decoding capabilities of Intel GPU, Sandy Bridge and successor, using libva API under X11.

Encoder part is based on http://cgit.freedesktop.org/libva/tree/test/encode/avcenc.c
V4L-Capture part is based on http://linuxtv.org/downloads/v4l-dvb-apis/capture-example.html 


Running it locally: It's mandatory to start the `decode' first.
./decode -> will wait for a connection
./encode -> will try to open /dev/video0, configure it and stream it out

Specifying the port, ip and video input parameters

Window A (first):
./decode -p 9999

Window B (second):
./encode -p 9999 -I 192.168.1.144 -d /dev/video0 -W 1280 -H 960

For more info:
./encode -? 
./decode -?
