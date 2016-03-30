## CIP-SERVER: Cloudware Interacting Protocol Server

cip-server is a part of the
[CloudwareHub](http://www.cloudwarehub.com) project.



### Building CIP Server

* Install the build dependencies. On Ubuntu use this command:

`sudo aptitude install libuv-dev libx264-dev`

* Install autotools

`sudo aptitude install autotools automake autoconf`

* Get source code and build

`git clone https://git.coding.net/guod/cip-server.git`

`cd cip-server`

`./autogen.sh`

`make`

`make install`


### Running CIP Server

* Export xserver display env

`export DISPLAY=[your xserver sock]`

* Run CIP Server

`./cip-server`
