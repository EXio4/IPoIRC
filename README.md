IPoIRC
======

A simple gateway of IP over IRC!

---

### Basic usage

From its usage:
*<netid> <irc_nick> <irc_network> <irc_pass> <irc_channel> <local_ip> <remote_ip>*

The **netid** is used for the in-irc comunication, it will identify the machine in a channel, you should **NOT** repeat it in different machines unless you want the bots to ignore each other.

The **irc nick** is the nick used for connecting to the network, it should have a placeholder ("%d") which will be replaced with a random number at runtime.

**irc network** is the address of the IRC that you want to use for IPoIRC, if it needs a password you should use **irc pass** for it, if it doesn't need one "-" means it will get ignored
note that the port is actually hardcoded to 6667

**irc channel** defines the channel used for the comunication between different bots.

**local ip** is the IP that the bot will "own" locally, it will be the local ip in the irc%d interface

**remote ip** defines the IP that the other side of the "PtP" connection will have.

---

### Protocol

PLACEHOLDER: IPoIRC's "Protocol" isn't stable

---

#### Building

IPoIRC is actually using a hand-written Makefile, you need the following deps (using debian names):

* libdumbnet-dev
* libssl-dev
* libbsd-dev
* libircclient-dev
* libpcre3-dev
* libzmq3-dev
* liblua5.2-dev

IPoIRC is using submodules, thus meaning you might need to run
```sh

git submodule init
git submodule updater

```