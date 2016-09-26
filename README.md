hid-retrolink is a Linux driver for the retrolink dual controller port adapter.

This driver is based on https://github.com/retuxx/hid-retrobit

Installation
=====

##### Raspberry Pi (RetroPie/Raspbian)

1. Download the latest deb package from: https://github.com/ryden/hid-retrolink/releases.
2. Install linux headers and dkms:  
   ```
   sudo apt-get install raspberrypi-kernel-headers dkms
   ```
3. Install the driver:  
   ```
   sudo dpkg -i hid-retrolink.deb
   ```
4. Plugin your device.

##### Debian, Ubuntu, SteamOS, ...

1. Download the latest deb package from: https://github.com/ryden/hid-retrolink/releases.
2. Install dkms:  
   ```
   sudo apt-get install dkms
   ```
3. Install the driver:  
   ```
   sudo dpkg -i hid-retrolink.deb
   ```
4. Plugin your device.

##### Fedora, CentOS, ...

1. Download the latest rpm package from: https://github.com/ryden/hid-retrolink/releases.
2. Make sure you have installed the latest kernel. Otherwise, dkms is unable to build the
   module because the linux headers are missing:  
   ```
   sudo yum update
   ```
3. Reboot your system.
4. Install linux headers:  
   ```
   sudo yum install kernel-devel
   ```
5. Install the driver:  
   ```
   sudo yum localinstall hid-retrolink-1.0.0-1.noarch.rpm
   ```
6. Plugin your device.

##### Manual

Install the driver with the following commands:
```bash
make              # build the driver
sudo make install # install the driver
sudo depmod -a    # update the module dependency graph
```

Now you should be able to load the driver with:  
(the dirver was successfully loaded if there is no terminal output)
```bash
sudo modprobe hid-retrolink
```

Unfortunately the driver *hid-generic* binds all HID devices with the result that 
*hid-retrolink* is unable to do its job. The file **99-hid-retrolink.rules** contains
some udev rules which unbinds all supported devices from *hid-generic* and rebinds
them to *hid-retrolink* on the fly. Copy this file to **/etc/udev/rules.d**.

To ensure that the udev rules are able to rebind the supported devices, you finally 
need to copy the file **hid-retrolink.conf** to **/etc/modules-load.d**.

Now your devices are ready to use. Restart your system and have fun playing games 
retro style :).
