
CamSync: Automatic Copying of Images from Camera via UPnP/DLNA
==============================================================

Many newer camera models come with integrated wifi. In my specific case, it is a Canon EOS 70D which has wifi. But it lacks a simple way of quickly uploading images to my NAS at home. There is, for example, no support for FTP or CIFS upload. I find this a terrible oversight or the wrong decision.

But the camera does support DLNA, by which the camera can be automatically detected on a network and images can be viewed -- or downloaded. With camsync we exploit that. By running camsync on my NAS it automatically detects when I enable wifi on the camera and it starts downloading the images automatically. It keeps a small state database so that it does not download an image again, even it was moved to another (more suitable) directory.

This little program a little three days project during the Christmas holidays. It is tailored specifically to my use case and you might need to adapt it should you want to use it on your end. I welcome patches that make camsync more capable and compatible with more cameras.

http://github.com/timn/camsync

Requirements
------------
- GUPnP (including GSSDP and GUPnP-AV)
- GLib2
- LibSoup
- SQLite3


Known Working Cameras
---------------------
Canon EOS 70D


Known Working Platforms
-----------------------
Fedora 20
ReadyNAS Ultra 4 (RAIDiator 4.2.26, with root access enabled)


Building
--------
To build camsync on Fedora do the following:
```
mkdir build
cd build
cmake ..
make
make install
```

Building on ReadyNAS is a little more involved. It requires compiling and installing a new set of libraries which are not available or only in a too old version. We have installed the following packages:
glib	    	2.38.2
glib-networking 2.38.2
gssdp		0.14.11
gupnp 		0.20.12
gupnp-av 	0.12.7
libsoup		2.44.2

To install, use the typical procedure, with slightly alternating configure options as listed below. Note that we install these packages to /usr/local, such that we can install them in parallel to existing libraries.

You will need to tweak pkg-config to search in the proper place for your new packages in the right place, e.g. to compile gssdp with the proper glib version:
```
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/lib/pkgconfig"
```
Then compile and install the packages in the order mentioned above like this:
```
./configure --disable-static
make
sudo make install
```
Additional specific configure options:
glib-networking: --disable-more-warnings
libsoup: --disable-more-warnings

Then you may want to modify /etc/ld.so.conf such that /usr/local/lib is preferred over /usr/lib and /lib. Alternatively you may set the environment variable LD_

Running
-------
To run simply executing bin/camsync in the project directory. Add the "-h" flag to see the available options.

In particular, if you want to use the program with a camera other than the Canon EOS 70D you need to set the model name to look for. To find the appropriate model name run camsync once without specifying one. It will show all UPnP devices including the model name.

If you find that camsync is working with another camera please drop me a note.

