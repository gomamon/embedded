hw1_20161622: main.o clock.o devices.o
	arm-none-linux-gnueabi-gcc -static -o hw1_20161622 main.o clock.o devices.o
main.o: main.c main.h clock.h devices.h
	arm-none-linux-gnueabi-gcc -c main.c -o main.o
clock.o: clock.c main.h clock.h
	arm-none-linux-gnueabi-gcc -c clock.c -o clock.o
devices.o: devices.c main.h devices.h
	arm-none-linux-gnueabi-gcc -c devices.c -o devices.o
clean:
	-rm *.o
	-rm hw1_20161622
