== Repository of stuff I write for my Apple //c ==

I'm having fun with this antiquity like it's 1989!

The Apple //c was my first computer and I loved it dearly
for years, but to be honest, it served little purpose these
last years, only decorating the office. But then my wife woke
up a monster by suggesting we do the Advent of Code, and we
started, and then I thought

"I should do it with the Apple //c!"

One thing led to another and there are now quite a few tools
here. Including an HomeAssistant frontend to control switches,
heating, and view sensors' graphs.

=== Building ===

Install cc65:

```
git clone https://github.com/cc65/cc65.git
cd cc65
make
sudo make install
```

Build my things:

```
make
```

Create floppy images:

```
make dist
```

You can then transfer the images in dist/ using ADTPro.

=== Notes ===

The Apple 2 serial port is hardcoded to be port 2. You can change that in the simple_serial_open() calls, grep for them in the src/ directory.

== Raspberry installation ==
For convenience, a pre-built surl-server is configured in the Raspberry raspios-buster-armhf-surl-server.img image file.

Copy it to a microSD card:
```
gunzip raspios-buster-armhf-surl-server.img.gz
dd if=raspios-buster-armhf-surl-server.img of=/dev/mmcblk0 bs=1M
sync
```

Install the SD card into a Raspberry, connect Ethernet, connect USB/serial adapter and boot. Everything should be up and running if you have a DHCP server. 

You can ssh into the pi with the default Raspbian login, pi/raspberry.
