default: app.o
	arm-none-linux-gnueabi-gcc -static -o app app.o
push: default
	adb push app /data/local/tmp
app.o: app.c 
	arm-none-linux-gnueabi-gcc -c app.c -o app.o
clean:
	-rm *.o
	-rm app
