APP=macbookfanspeed
JOB=macbook-fan-speed.conf

$(APP): pid/pid.c speedcontrol.c
	gcc $? -Wall -g -I pid `pkg-config --cflags --libs glib-2.0` -o $@

clean:
	rm -rf $(APP)

install: $(APP)
	cp $(APP) /usr/bin
	cp $(JOB) /etc/init
