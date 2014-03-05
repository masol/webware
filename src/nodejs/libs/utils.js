module.exports.processPost = function(request,response,utils,callback){
  
  if(typeof callback !== 'function')
    return null;

  if(request.method == 'POST') {
    var body = '';
    request.on('data', function (data) {
      body += data;
      // 1e6 === 1 * Math.pow(10, 6) === 1 * 1000000 ~~~ 1MB
      if (body.length > 1e6) { 
        // FLOOD ATTACK OR FAULTY CLIENT, NUKE REQUEST
        body = "";
        response.writeHead(413, {'Content-Type': 'text/plain'}).end();
        request.connection.destroy();
      }
    });
    request.on('end', function () {
        //use POST
        var POST = utils.qs.parse(body);
        callback(POST);
    });
  }else {
    response.writeHead(405, {'Content-Type': 'text/plain'});
    response.end();
  }
};

module.exports.getUTC = function(){
  var now = new Date();
  return new Date(now.getTime() + now.getTimezoneOffset() * 60000);
};