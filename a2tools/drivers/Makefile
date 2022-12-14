OBJS := a2.ssc.o a2.hi.o

all: $(OBJS)

%.o: %.s
	ca65 -t apple2 $<

a2.ssc.s:
	co65 --code-label _a2_ssc a2.ssc.ser

a2.hi.s:
	co65 --code-label _a2_ssc a2.hi.tgi

clean:
	rm *.o *.s
	
.PHONY: all clean
