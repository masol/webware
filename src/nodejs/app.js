
/**
 * Module dependencies.
 */

var blockfuncs = {
  require : function(){}
};

function sandbox(script,parameters){
  with(blockfuncs){
    script.runInThisContext();
    //if script no return statement.
    return returnobject;
  }
}

function main(){
  
  var urlparser = require('url');
  var fs = require('fs');
  var us = require('underscore');
  var vm = require('vm');
  var path = require('path');

  var config = {
    "site" : __dirname + "/site/",
    "rewriter" : "/rewriter.js"
  };

  var config_file = __dirname + '/config.json';
  fs.readFile(config_file, 'utf8', function (err, data) {
    if (err) {
      return;
    }
    try{
      config_obj = JSON.parse(data);
      console.dir(config_obj);
      config = us.extend(config,config_obj);
      if(config.site.slice(-1) != '/')
        config.site += '/';
    }catch(e)
    {
    }
  });

  var server = require('http').createServer(handler)
      ;//, io = require('socket.io').listen(server)

  var rewrite_handler = {};
  var path_handler = {};

  //@FIXME: shall we use a sandbox of spawn-child to run external script?
  function handler(req,res){
    //Whether host header is defined?
    if(!req.headers['host'])
    {
      //return 400 bad request.
      res.writeHead(400);
      res.end();
      //Do not need close the socket, this is the duty of nginx.
      //req.socket.destroy();
    }
    
    try{
      //Is host wide rewrite function exist?
      var rewriter = rewrite_handler[req.headers['host']];
      var handler_fullpath;
      
      function after_handler_process(){
        
        var handler = path_handler[handler_fullpath];
        if(handler && !us.isBoolean(rewriter))
        {
          res.writeHead(200, {"Content-Type": "text/plain"});
          //response.end(JSON.stringify(request));
          //
          var body = "";
          for( i in req.headers)
          {
            body += i;
            body += '=';
            body += req.headers[i] + "<br/>";
          }
          res.end(body);
        }else{
          //return 404 not found.
          res.writeHead(404);
          res.end();
        }
      }
      
      function handler_loaded(err,content){
        if(err){
          path_handler[handler_fullpath] = true;
        }else{
          //compile content.
          var script = vm.createScript(content,config.rewriter);
          path_handler[handler_fullpath] = script ? script : true;
        }
        after_handler_process();
      }
      
      function after_rewrite_process(){
        var url = (us.isBoolean(rewriter)) ? req.url : sandbox(rewriter,{'url':req.url});
        var urlobj = urlparser.parse(url);
        if(urlobj && urlobj.pathname)
        {
          handler_fullpath = config.site + req.headers['host'] + path.normalize(urlobj.pathname);
          if(!path_handler[handler_fullpath])
          {//load handler from the fullpath.
            fs.readFile(handler_fullpath, 'utf8',handler_loaded);
          }else{
            after_handler_process();
          }
        }
      }
      
      if(!rewriter)
      {//rewriter not loading, load it.
        var filename = config.site + req.headers['host'] + config.rewriter;
        fs.readFile(filename, 'utf8',function(err, content){
            if(err){
              rewriter = rewrite_handler[req.headers['host']] = true;
            }else{
              //compile content.
              var script = vm.createScript(content,config.rewriter);
              rewriter = rewrite_handler[req.headers['host']] = script ? script : true;
            }
            after_rewrite_process();
          }
        );
      }else{
        after_rewrite_process();
      }
    }catch(e)
    {
      //return 500 internal error.
      res.writeHead(500);
      res.end();
      //Do not need close the socket, this is the duty of nginx.
      //req.socket.destroy();
      console.log(e);
    }
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

}

main();