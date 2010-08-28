APP=macbookfanspeed
JOB=init.d/$(APP)

$(APP): pid/pid.c speedcontrol.c
	gcc $^ -Wall -g -I pid `pkg-config --cflags --libs glib-2.0` -o $@

clean:
	rm -rf $(APP)

install: $(APP) $(JOB)
	cp $(APP) /usr/local/bin
	cp $(JOB) /etc/init.d/
	update-rc.d -f $(APP) defaults

uninstall:
	rm -f /usr/local/bin/$(APP)
	update-rc.d -f $(APP) remove
	rm -f /etc/init.d/$(APP)
