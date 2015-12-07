var ws = require('websocket.io')
  , server = ws.listen(3000)

server.on('connection', function (socket) {
	console.log("we have connected\n");
    socket.send("Hello");
    socket.on('message', function (data) {
        console.log(data);
    });
    socket.on('close', function () {
		console.log("client end")
	});
});
