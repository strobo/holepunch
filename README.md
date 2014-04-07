Holepunch
----------
UDP hole punching is one of the NAT-traversal technique. So I implemented simple NAT-traversal application using UDP hole punching for my practice.
If you want to know more infomation about UDP hole punching, Please see [wikipedia](http://en.wikipedia.org/wiki/UDP_hole_punching)


Requirements
------------
holepunch depends libevent. I using libevent ver 2.0.21.

To use this application. You need to prepare three computers. One is used for "matching server". And another two computers are used for "node" which will communicate each other.

**Important:** Matching server should have a global IP address. or permission to open any port. But node should not have a global IP address.


Build
-----
1. Install libevent

	Install libevent using your environment's package manager.

2. Make it

	If you have clang.

	```
	$ make
	```

	If you doesn't have clang.

	```
	$ make CC=gcc
	```

Usage
-----
```
$ holepunch
Usage: holepunch server <port>
                 node <serverAddress[:port]> <key>
```
- Port is optional.
- Server's default listening port is 60000.
- Node's default listening port is 60002.
- Communicate with the node to node, if matches the &lt;key&gt;.


On mtching server named 'example.com':

```
$ ./holepunch server
```

On node:

```
$ ./holepunch node example.com keyword
```

And then. Try to type a keyboard. character will send to connected node.

Reference
---------
- NAT越えの技術 [http://dev.ariel-networks.com/articles/networkmagazine/nat/](http://dev.ariel-networks.com/articles/networkmagazine/nat/)
