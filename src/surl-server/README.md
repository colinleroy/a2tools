== surl-server - a serial to network proxy ==

This tool is based on CURL. It receives requests on a serial port,
executes them, and sends the result back to the serial port.  It is
quite adapted to very limited computers, sending the response in chunks
of whatever size the client requests, and waiting for the client's
green light to continue.

The client "lib" lives in ../lib/surl/, and examples of its usage are
available in the various programs I wrote.

=== Building ===
Install dependancies, for example on Debian-like systems:

```
sudo apt-get install libcurl4-gnutls-dev libgumbo-dev libpng-dev \
  libjq-dev libsdl-image1.2-dev libavcodec-dev libavformat-dev \
  libavfilter-dev libavutil-dev libswresample-dev libmagic-dev \
  libcups2-dev libfreetype-dev
```

Then build and install:
```
make
sudo make install
```

Start the service as root once to generate config files to /usr/local/etc/a2tools/tty.conf, /usr/local/etc/a2tools/printer.conf, and /usr/local/etc/a2tools/vsdrive.conf, and edit them if necessary. To generate the files, start the server:

```
sudo /usr/local/bin/surl-server
```

Enable and start the service:

```
sudo systemctl enable surl-server.service
sudo systemctl restart surl-server.service
```

On the provided Raspberry Pi image, ssh is enabled so you can log on the Raspberry to change any setting you like, with the default 'pi' login, password 'raspberry'.
On the provided image, the configuration files are in /etc/a2tools.
