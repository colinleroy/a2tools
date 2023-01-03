== surl-server - a serial to network proxy ==

This tool is based on CURL. It receives requests on a serial port,
executes them, and sends the result back to the serial port.  It is
quite adapted to very limited computers, sending the response in chunks
of whatever size the client requests, and waiting for the client's
green light to continue.

The client "lib" lives in ../lib/surl.[ch], and an example CLI tool in
../surl/
