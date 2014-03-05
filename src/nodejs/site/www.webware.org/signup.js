module.exports.http_handler = function(request,response,utils){
 
try{
  
  utils.processPost(request,response,utils,function(post){
    function  sendResult(result){
      if(!result) result = { errorcode : 0, message : ''};
      if(!result.message) result.message = '';
      if(!result.errorcode) result.errorcode = 0;
      response.writeHead(200, {"Content-Type": "text/json"});
      response.end(JSON.stringify(result));
    }

    try{
      //console.log(post);
      if(!post['email'] || !post['pass'])
      {
        sendResult({errorcode : 1, message : 'insufficient parameters'});
      }else{
        var userpath = utils.getPath(post['email'],"user");
        var userfile = userpath + '/user.json';
        utils.fs.exists(userfile,function(exists){
          if(exists){//File is there. return errorcode 2.
            sendResult({errorcode : 2, message : 'email address already register.'});
          }else{
            utils.mkdirp(userpath,function(err){
              if(err){
                sendResult({errorcode : 3, message : 'can not create user directory:' + JSON.stringify(err)});
              }else{
                var email_verify = utils.generatePassword(24,false);
                var user = {
                  name : post['email'],
                  pass : post['pass'],
                  verify : {
                    email : email_verify,
                    phone : '',
                    reset : ''
                  },
                  reg_time : utils.getUTC()
                };
                utils.fs.writeFile(userfile,JSON.stringify(user),function(err){
                  if(err){ // can not write data to file.
                    sendResult({errorcode : 4, message : 'can not save user information:' + JSON.stringify(err)});
                  }else{
                    sendResult();
                    
                    var verify_string = "http://www.webware.org/d/activate.js?code=" + email_verify;
                    //send email to activate.
                    var message = {
                      // sender info
                      from: utils.config.mail['from'],
                      to: post['email'],
                      subject: "activate your webware account",
                      headers: {
                        'X-Laziness-level': 1000
                      },
                      // plaintext body
                      text: 'Hello follow is your active code ,please visit : ' + verify_string,
                      // HTML body
                      html:'<p><b>Hello</b> Please link this <a href="' + verify_string + '"/>link</a> to active your webware account.</p>'+
                      '<p>or your can copy follow text and paste it in browser to active your account:</p>' +
                      '<p>' + verify_string + '</p>'
                    };
                    //console.log(utils.smtp);
                    utils.smtp.sendMail(message,function(err){
                      if(err){
                        console.log('failed send mail to ' + post['email']);
                      }else{
                        console.log('sucessfully send mail to ' + post['email']);
                      }
                      //utils.smtp.close(); // close the connection pool
                    });
                  }
                });
              }
            });
          }
        });
      }
    }catch(e)
    {
      sendResult({errorcode : -1, message : JSON.stringify(e)});
    }
  });
  //step1: get user path.
  //step2: check the existance of user path.
  //step3: if exist, load user json and check activate.
  //step4: if not exist or not activated, reset user info and return.

  //console.log(aaautils);
  //console.log("/en/index.html");
  //console.log(utils.getPath("en/index.html","public"));
}catch(e)
{
  console.log(e);
}

}


//*/