## Apple 2 MAME tracer

This tool reads an execution trace as written by MAME's debugger,
and annotates it with debug symbols. 

### Building the tool
```
git clone https://github.com/colinleroy/a2tools.git
cd a2tools/src/a2trace
make && sudo make install
```

### Using the tool

Build your program with debug information and a VICE label file:
```
cl65 --debug-info --dbgfile,program.dbg -o program.as [C files] 
```

Transfer your program to a disk, start MAME with -debug.

Activate MAME's execution tracer with `trace program.run,maincpu,noloop,{tracelog "A=%02X,X=%02X,Y=%02X ",a,x,y}` in the debugger window.

Run your program.

To annotate the run, use:
```
a2trace -d program.dbg -d program.dbg -t program.run
```

You can also trace your Apple II program while it runs, using the `-f` flag.
