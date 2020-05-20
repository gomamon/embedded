default: app.o
	arm-none-linux-gnueabi-gcc -static -o app app.o
app.o: app.c app.h 
	arm-none-linux-gnueabi-gcc -c app.c -o app.o
clean:
	-rm *.o
	-rm app
