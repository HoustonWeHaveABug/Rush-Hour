RUSH_HOUR_C_FLAGS=-c -O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow -Wstrict-prototypes -Wswitch -Wwrite-strings

rush_hour.exe: rush_hour.o
	gcc -o rush_hour.exe rush_hour.o

rush_hour.o: rush_hour.c rush_hour.make
	gcc ${RUSH_HOUR_C_FLAGS} -o rush_hour.o rush_hour.c
