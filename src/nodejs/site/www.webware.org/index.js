module.exports.http_handler = function(request,response,utils){
 
try{
  //console.log(aaautils);
  //console.log("/en/index.html");
  //console.log(utils.getPath("en/index.html","public"));
  utils.fs.readFile(utils.getPath("/en/index.html","public"),function(err,content){
    if(err){
      //console.log('test');
      //console.log(request);
      response.writeHead(200, {"Content-Type": "text/plain"});
      //response.end(JSON.stringify(request));
      //
      var body = "";
      for( i in request.headers)
      {
        body += i;
        body += '=';
        body += request.headers[i] + "<br/>";
      }

      //returnobject = "this is a test";

      response.end(body);

      /*
      for(var i in this)
      {
        console.log(i + '=' + this[i]);
      }

      ///*
      for(var i in process)
      {
        console.log(i + '=' + process[i]);
      }

      for(var i in process)
      {
        console.log(i + '=' + process[i]);
      }
      //*/
    }else{
      response.writeHead(200, {"Content-Type": "text/html"});
      response.end(content);
    }
  });
}catch(e)
{
  console.log(e);
}

}


//*/