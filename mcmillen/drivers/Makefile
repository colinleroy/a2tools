OBJS := a2.ssc.o

all: $(OBJS)

%.o: %.s
	ca65 -t apple2 $<

a2.ssc.s:
	co65 --code-label _a2_ssc a2.ssc.ser

clean:
	rm *.o *.s
	
.PHONY: all clean
