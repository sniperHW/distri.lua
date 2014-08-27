local Http = require "lua/http"
Http.CreateServer(function (req, res) 
  res:WriteHead(200,"OK", {"Content-Type: text/plain"})
  res:End("Hello World\n");
end):Listen("127.0.0.1",8001)

print("create server on 127.0.0.1 8001")


