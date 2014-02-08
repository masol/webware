
/**
 * Module dependencies.
 */
 
var server = require('http').createServer(handler)
    ;//, io = require('socket.io').listen(server)

function handler(req,res){
  res.writeHead(200, {"Content-Type": "text/plain"});
  //response.end(JSON.stringify(request));
  res.end("Hello World\n"); 
}


var WebSocketServer = require('ws').Server;
var wss = new WebSocketServer({server: server});

wss.on('connection', function(ws) {
  console.log(ws.upgradeReq.url);
  for(var i in ws.upgradeReq.headers)
  {
    console.log(i + '=' + ws.upgradeReq.headers[i]);
  }
  console.log(ws.path);
  console.log(this.clients);
  console.log(this.path);
  var id = setInterval(function() {
    ws.send(JSON.stringify(process.memoryUsage()), function() { /* ignore errors */ });
  }, 1000);
  console.log('started client interval');
  ws.on('close', function() {
    console.log('stopping client interval');
    clearInterval(id);
  })
});

/*
io.configure(function () {
  io.enable('browser client etag');
  io.disable('browser client cache');
  io.set('log level', 1);

  io.set('transports', [
    'websocket'
    ,'flashsocket'
    ,'htmlfile'
//  ,'xhr-polling'
//  ,'jsonp-polling'
  ]);
});

io.sockets.on('connection', function (socket) {
  console.log('connection');
  socket.emit('news', { hello: 'world' });
  socket.on('my other event', function (data) {
    console.log(data);
  });
  
  socket.on('disconnect', function () {
    console.log('disconnect');
  });  
});//*/


server.listen(process.argv[2],function(){
 console.log('server listening on "' + process.argv[2] + '"');
});

/** @brief simple quit process. Daemon manager will handle other thing.(restart server,for example).
**/
server.on('error', function (e) {
  process.exit();
});


process.on('SIGINT',function(){
  console.log( "\nGracefully shutting down from SIGINT (Ctrl-C)" );
  // some other closing procedures go here
  server.close(function(){
    process.exit();
  });
  setTimeout(function(){ process.exit(); }, 500);
});
