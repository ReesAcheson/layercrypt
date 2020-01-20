
#-m32 for 32 bit
#-m64 for 64 bit

#OPTIONS = -O2 -g -Wall # -g for debug, -O2 for optimise; -Wall additional; -fstack-protector-all
#OPTIONS = -O2 -Wall -fstack-protector-all
#OPTIONS = -O2 -g -fstack-protector-all -Wall -D_FORTIFY_SOURCE

#On cmdln use: "opts=-O2 -Wall"
OPTIONS = $(opts)
#OPTIONS +=  -O2 -Wall
#-O2 is ptimization level 2
OPTIONS += -O2
#OPTIONS += -Iinclude
OPTIONS += -D For_osLinux -D Linux -D For_Linux
#OPTIONS += -D For_OSx
#OPTIONS += -D For_osWin

#VPATH = include



SeeOptions:
	@echo $(OPTIONS) 
	@echo e.g. bits="-m64" for 64 bit

layercrypt: layercrypt.o lc_random.o lc_vars.o lc_util.o lc_debug.o lc_layers.o lc_files.o lc_crypt.o lc_init.o
	${CC} -o layercrypt $^

layercrypt.o: layercrypt.c
	${CC} ${OPTIONS} -c $<



clean:
	rm *.o
