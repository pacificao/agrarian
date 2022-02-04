
Debian
====================
This directory contains files used to package agrariand/agrarian-qt
for Debian-based Linux systems. If you compile agrariand/agrarian-qt yourself, there are some useful files here.

## agrarian: URI support ##


agrarian-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install agrarian-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your agrarian-qt binary to `/usr/bin`
and the `../../share/pixmaps/agrarian128.png` to `/usr/share/pixmaps`

agrarian-qt.protocol (KDE)

