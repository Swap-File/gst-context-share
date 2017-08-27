#!/bin/bash
echo Xset...
xset s 0 0
xset s off # don't activate screensaver
xset s noblank # don't blank the video device
xset s noexpose
xset -dpms
./glxgears

