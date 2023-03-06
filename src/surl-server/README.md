== surl-server - a serial to network proxy ==

This tool is based on CURL. It receives requests on a serial port,
executes them, and sends the result back to the serial port.  It is
quite adapted to very limited computers, sending the response in chunks
of whatever size the client requests, and waiting for the client's
green light to continue.

The client "lib" lives in ../lib/surl.[ch], and an example CLI tool in
../surl/

=== Building ===
Install dependancies, for example on Debian-like systems:

```
sudo apt-get install libcurl4-gnutls-dev libgumbo-dev libjpeg-dev libpng-dev libjq-dev
```

Then build and install:
```
make
sudo make install
```

Start the service once to generate a config file to /etc/surl-server/tty.conf, and edit it if necessary:

```
sudo /usr/local/bin/surl-server
```

Enable and start the service:

```
sudo systemctl enable surl-server.service
sudo systemctl restart surl-server.service
```

On the raspios-buster-armhf-surl-server.img image, ssh is enabled so you can log on the Raspberry to change any setting you like, with the default 'pi' login, password 'raspberry'.
