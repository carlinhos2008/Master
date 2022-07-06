
Debian
====================
This directory contains files used to package masterstaked/masterstake-qt
for Debian-based Linux systems. If you compile masterstaked/masterstake-qt yourself, there are some useful files here.

## masterstake: URI support ##


masterstake-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install masterstake-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your masterstakeqt binary to `/usr/bin`
and the `../../share/pixmaps/masterstake128.png` to `/usr/share/pixmaps`

masterstake-qt.protocol (KDE)

