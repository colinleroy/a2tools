## Apple 2 MAME tracer

This tool reads an execution trace as written by MAME's debugger,
and annotates it with debug symbols. 

### Building the tool
```
cd src/trace-run
make && sudo make install
```

### Using the tool

Build your program with debug information and a VICE label file:
```
cl65 --debug-info --dbgfile,program.dbg -Ln program.lbl -o program.as [C files] 
```

Transfer your program to a disk, start MAME with -debug.

Activate MAME's execution tracer with `trace program.run` in the debugger window.

Run your program.

To annotate the run, use:
```
trace-run -d program.dbg -l program.lbl -t program.run
```

You can also trace your Apple II program while it runs, using the `-f` flag.
