
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
To build camsync do the following:
mkdir build
cd build
cmake ..
make
make install


Running
-------
