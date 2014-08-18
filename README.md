distri.lua
======
distri.lua is a lua concurrency networking framework aid to help the developer to fast
build online game server ,web application,light distributed system and so on.

Features include:

* Fast event loop
* Cooperative socket library
* Rpc support used json
* MutilThread
* Supported only Linux
* Cooperative dns query
* Lighted HttpServer
* SSL Connection
* WebSocket

distri.lua is licensed under GPL license.


get distri.lua
-----------

Install lua 5.2 or newer.

Clone [the repository](https://github.com/sniperHW/KendyNet) in deps.

Clone [the repository](https://github.com/sniperHW/distri.lua).

Post feedback and issues on the [bug tracker](https://github.com/sniperHW/distri.lua/issues),


build
------
```
cd deps/jemalloc
./configure
make
cd ../

cd deps/lua-cjson-2.1.0
make
mv cjson.lua ../../

cd ../../
make distrilua
```

running tests
-------------
```
./distrilua hello.lua
./distrilua pingclient.lua
```

to-do list
----------
* Rpc support used json
* Cooperative dns query
* MutilThread
* Lighted HttpServer
* SSL Connection
* WebSocket