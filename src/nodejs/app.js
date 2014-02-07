
/**
 * Module dependencies.
 */
 
var server = require('http').createServer(handler)
    , io = require('socket.io').listen(server)

var http = require('http');

server.listen(process.argv[2],function(){
 console.log('server listening on "' + process.argv[2] + '"');
});

function handler(req,res){
  res.writeHead(200, {"Content-Type": "text/plain"});
  //response.end(JSON.stringify(request));
  res.end("Hello World\n"); 
}


io.configure(function () {
  io.enable('browser client etag');
  io.disable('browser client cache');
  io.set('log level', 100);
//  io.set('resource', "d/socket.io");

  io.set('transports', [
    'websocket'
    ,'flashsocket'
    ,'htmlfile'
//  ,'xhr-polling'
//  ,'jsonp-polling'
  ]);
}); //*/

io.sockets.on('connection', function (socket) {
  console.log('connection');
  socket.emit('news', { hello: 'world' });
  socket.on('my other event', function (data) {
    console.log(data);
  });
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
