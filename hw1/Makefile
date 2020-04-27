default: main.o clock.o devices.o counter.o textEditor.o drawBoard.o
	arm-none-linux-gnueabi-gcc -static -o hw1_20161622 main.o clock.o devices.o counter.o textEditor.o drawBoard.o
push: default
	adb push hw1_20161622 /data/local/tmp
main.o: main.c main.h clock.h devices.h counter.h
	arm-none-linux-gnueabi-gcc -c main.c -o main.o
clock.o: clock.c main.h clock.h
	arm-none-linux-gnueabi-gcc -c clock.c -o clock.o
counter.o: counter.c main.h counter.h
	arm-none-linux-gnueabi-gcc -c counter.c -o counter.o
textEditor.o: textEditor.c main.h textEditor.h
	arm-none-linux-gnueabi-gcc -c textEditor.c -o textEditor.o
drawBoard.o: drawBoard.c main.h drawBoard.h
	arm-none-linux-gnueabi-gcc -c drawBoard.c -o drawBoard.o
devices.o: devices.c main.h devices.h
	arm-none-linux-gnueabi-gcc -c devices.c -o devices.o
clean:
	-rm *.o
	-rm hw1_20161622
