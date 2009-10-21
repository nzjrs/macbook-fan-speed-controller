all: pid/pid.c speedcontrol.c
	gcc speedcontrol.c pid/pid.c -Wall -g -I pid `pkg-config --cflags --libs glib-2.0` -o speed
