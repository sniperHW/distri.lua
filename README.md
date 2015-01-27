distri.lua
======
distri.lua is a lua concurrency networking framework aid to help the developer to fast
build online game server ,web application,light distributed system and so on.

Features include:

* Fast event loop
* Cooperative socket library
* Rpc support used json
* Supported only Linux
* Cooperative dns query
* Lighted HttpServer
* SSL Connection
* WebSocket
* local and remote debug of lua

distri.lua is licensed under GPL license.


get distri.lua
-----------

Install libcurl

Install lua 5.2.

Clone [the repository](https://github.com/sniperHW/distri.lua).

Post feedback and issues on the [bug tracker](https://github.com/sniperHW/distri.lua/issues),


build
------
```
make distrilua
```

running tests
-------------
```
pingpong test

./distrilua samples/hello.lua
./distrilua samples/pingclient.lua

rpc test

./distrilua samples/rpcserver.lua
./distrilua samples/rpcclient.lua

```


to-do list
----------
* Cooperative dns query
* WebSocket
* debuger

test httpserver
----------

    local Http = require "lua/http"
    Http.CreateServer(function (req, res) 
        res:WriteHead(200,"OK", {"Content-Type: text/plain"})
        res:End("Hello World\n");
        end):Listen("127.0.0.1",8001)

    print("create server on 127.0.0.1 8001")

