all: pid.c speedcontrol.c
	gcc speedcontrol.c pid.c -Wall -g `pkg-config --cflags --libs glib-2.0` -o speed
