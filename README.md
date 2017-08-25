# gst-context-share
initial

This is a demonstation of how to share an OpenGL context and X11 display that originates in a GLX program (glxgears) with gstreamer for texture sharing.

The key is to use a glfilterapp callback to get the texture and setting the bus up to hand off the display and context when requested for them.

I put my rendering engine in the gstreamer idle loop and let it sync to my refresh rate (60hz), while the gstreamer video rate was less (20hz).

I may clean it up at some point in the future, but given the lack of example code online I figured it was best to share this sooner rather than later.
