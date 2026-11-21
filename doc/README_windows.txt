Agrarian Core
=====================

Intro
-----
Agrarian is a free open source peer-to-peer electronic cash system that is
completely decentralized, without the need for a central server or trusted
parties.  Users hold the crypto keys to their own money and transact directly
with each other, with the help of a P2P network to check for double-spending.


Setup
-----
Run the Windows installer and launch Agrarian Core from the Start menu.

Updating an existing installation
---------------------------------
Before updating, open Agrarian Core and choose File > Backup Wallet. Save
the backup somewhere outside the installation folder. Exit Agrarian Core
normally and wait for shutdown to finish. If you run agrariand.exe, stop
it normally as well.

Run the new installer. It defaults to your existing installation folder for
the same Windows user. It replaces program files, not wallet data, and
doesn't reset wallet settings. If Agrarian is still running, close it and
choose Retry. Silent installers stop with exit code 2 if Agrarian is running
or the process check fails; stop it normally before retrying.

Agrarian Core is the original Agrarian client and it builds the backbone of the network.
However, it downloads and stores the entire history of Agrarian transactions;
depending on the speed of your computer and network connection, the synchronization
process can take anywhere from a few hours to a day or more.
