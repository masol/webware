// ============================================================================
// This file is part of webware.
// Published under the Apache License, Version 2.0.
// Copyright (C) by masol.li@gmail.com.
//
// See https://github.com/masol/webware for more information.
// ============================================================================

// uncomment to get symbols/line numbers in crash reports
// #pragma debug

#include "gwan.h"   // G-WAN exported functions
#include "webware.h"

#include <stdio.h>   // printf()
#include <string.h>  // strcmp()


//@fixme: is there some memory leak in g-wan? I can not use stack memory. how g-wan implement the script-machine code transimision? If they using llvm or gcc wrap. It's seem this isn't a problem.
//Follow is a simplest local thread storage alternative stack memory.SEE precede description.
static const size_t host_buf_len = 64;

#ifdef USE_CUSTOM_TLS
//We initiate gv_hostbuf in init. //To prevent gwan compiler not initilize static memory,assign it to 0l.
static char**  gv_hostbuf = 0l;
#endif //USE_CUSTOM_TLS

// init() will initialize server-wide data structures, load your files, etc.
// ----------------------------------------------------------------------------
// init() should return -1 if failure (to allocate memory for example)
int init(int argc, char *argv[])
{
   // get the Handler persistent pointer to attach anything you need
 //data_t **data = (data_t**)get_env(argv, US_HANDLER_DATA);
 //data_t **data = (data_t**)get_env(argv, US_VHOST_DATA);
 //  ww_server_t **data = (ww_server_t**)get_env(argv, US_SERVER_DATA);
   
   // initialize the persistent pointer by allocating our structure
   // attach structures, lists, sockets with a back-end/database server, 
   // file descriptiors for custom log files, etc.
 //  *data = (data_t*)calloc(1, sizeof(ww_server_t));
 //  if(!*data)
 //     return -1; // out of memory
 
#ifdef USE_CUSTOM_TLS
  //@FIXME: shall we need thread lock in here? dunno with g-wan....
  if(!gv_hostbuf)
  {
    int work_num = (int)get_env(argv,NBR_WORKERS);
    int i = 0;
    gv_hostbuf = (char**)calloc(work_num,sizeof(char*));
    for(;i < work_num; i++)
    {
      gv_hostbuf[i] = calloc(work_num,host_buf_len);
    }
    //printf("work_num is : %d",work_num);
  }
#endif //USE_CUSTOM_TLS

  

//  u8 *query_char = (u8*)get_env(argv, QUERY_CHAR);
//  *query_char = '!'; // we use "/!helloc.c" instead of "/?helloc.c"




   // define which handler states we want to be notified in main():
   // enum HANDLER_ACT { 
   //  HDL_INIT = 0, 
   //  HDL_AFTER_ACCEPT, // just after accept (only client IP address setup)
   //  HDL_AFTER_READ,   // each time a read was done until HTTP request OK
   //  HDL_BEFORE_PARSE, // HTTP verb/URI validated but HTTP headers are not 
   //  HDL_AFTER_PARSE,  // HTTP headers validated, ready to build reply
   //  HDL_BEFORE_WRITE, // after a reply was built, but before it is sent
   //  HDL_HTTP_ERRORS,  // when G-WAN is going to reply with an HTTP error
   //  HDL_CLEANUP };
   u32 *states = (u32*)get_env(argv, US_HANDLER_STATES);
   *states = (1L << HDL_HTTP_ERRORS)
         | (1L << HDL_AFTER_READ)
	 | (1L << HDL_BEFORE_WRITE);
/*		   | (1L << HDL_BEFORE_PARSE)
		   | (1L << HDL_AFTER_PARSE);//*/
   
   return 0;
}
// ----------------------------------------------------------------------------
// clean() will free any allocated memory and possibly log summarized stats
// ----------------------------------------------------------------------------
void clean(int argc, char *argv[])
{
  // free any data attached to your persistence pointer
  ww_server_t **data = (ww_server_t**)get_env(argv, US_SERVER_DATA);

  // we could close a data->log custom file 
  // if the structure had a FILE *log; field
  // fclose(data->log);

  if(data)
    free(*data);

#ifdef USE_CUSTOM_TLS
  if(gv_hostbuf)
  {
    int work_num = (int)get_env(argv,NBR_WORKERS);
    int i = 0;
    for(;i < work_num; i++)
    {
      free(gv_hostbuf[i]);
    }
    free(gv_hostbuf);
    gv_hostbuf = 0l;
    //printf("free: work_num is : %d",work_num);
  }
#endif //USE_CUSTOM_TLS
}
// ----------------------------------------------------------------------------
// main() does the job for all the connection states below:
// (see 'HTTP_Env' in gwan.h for all the values you can fetch with get_env())
// ----------------------------------------------------------------------------

// those defines summarise the return codes for each state
#define RET_CLOSE_CONN   0 // all states
#define RET_BUILD_REPLY  1 // HDL_AFTER_ACCEPT only
#define RET_READ_MORE    1 // HDL_AFTER_READ only
#define RET_SEND_REPLY   2 // all states
#define RET_CONTINUE   255 // all states


#define  MAX_LANG_LEN    8
typedef  char accept_language_t[MAX_LANG_LEN];

static inline char* getNextLine(char* s)
{
  while(*s && *s != '\r' &&  *(s+1) != '\n') s++;
  if(*s) s += 2;
  return s;
}

#define	 _DEBUG		1
//#undef _DEBUG

#ifdef	_DEBUG
static   void  debug_count(int pos)
{
  printf("%d ",pos);
}
#else
#define	debug_count(p)
#endif

/**
 * strnstr - Find the first substring in a length-limited string
 * @s1: The string to be searched
 * @s2: The string to search for
 * @len: the maximum number of characters to search
 * original from lib/string.c
*/
static char *strnstr(const char *s1, const char *s2, size_t len)
{
  size_t l2;

  l2 = strlen(s2);
  if (!l2)
    return (char *)s1;
  while (len >= l2) {
    len--;
    if (!memcmp(s1, s2, l2))
      return (char *)s1;
    s1++;
  }
  return NULL;
}


/** @brief whether s start with msg(len is msg length).
 * @return if s start with msg, return string start after msg in s and remove blank. otherwise return 0l(NULL).IE. 
**/
static char* startwith(char *s, char *msg,int len)
{
   int i = 0;
   for(;i < len;i++)
   {
     if( *s++ != *msg++) break;
   }
   return (i == len) ? s : (char*)0l;
}

static char* endwith(char *s,char *msg,int len)
{
   int i = 0;
   msg += len;
   for(;i < len;i++)
   {
     if( *s-- != *msg--) break;
   }
   return (i == len) ? s : (char*)0l;
}


int main(int argc, char *argv[])
{
   long state = (long)argv[0];
   switch(state)
   {
      // ----------------------------------------------------------------------
      // AFTER_ACCEPT return values:
      //   0: Close the client connection
      //   1: Build a reply based on a custom request buffer/HTTP status code
      //   2: Send a server reply based on a reply buffer/HTTP status code
      // 255: Continue (wait for read() to fetch data sent by client)
     /* case HDL_AFTER_ACCEPT:
      {
         char *szIP = (char*)get_env(argv, REMOTE_ADDR);
         
         // we don't want this user to touch our server
         if(!szIP || !strcmp(szIP, "1.2.3.4"))
            return 0; // 0: Close the client connection

         // we want this other user to be redirected to another server
         if(!strcmp(szIP, "5.6.7.8"))
         {
            char szURI[] = "http://another-place.org";
            xbuf_t *reply = get_reply(argv);
            xbuf_xcat(reply,
                      "<html><head><title>Redirect</title></head>"
                      "<body>Click <a href=\"%s\">here</a>.</body></html>",
     					    szURI);

            // set the HTTP reply code accordingly
            int *pHTTP_status = (int*)get_env(argv, HTTP_CODE);
            if(pHTTP_status)
               *pHTTP_status = 301; // 301:'moved permanently'
               
            // 2: Send a server reply based on a reply buffer/HTTP status code
            return 2;
         }
      }
      break; //*/
      // ----------------------------------------------------------------------
      // AFTER_READ return values:
      //   0: Close the client connection
      //   1: Read more data from client
      //   2: Send a server reply based on a reply buffer/HTTP status code
      // 255: Continue (read more if request not complete or build reply based
      //                on the client request -or your altered version)
      //  First, we check first line is a valid HTTP request.
      case HDL_AFTER_READ:
      {
        xbuf_t *read_xbuf = (xbuf_t*)get_env(argv, READ_XBUF);
        if(!read_xbuf || !read_xbuf->ptr || read_xbuf->len < 20) break; //20 is min length requirement that include Host: header.
        //if we not start from POST|GET /g/  : this mean gloabl asset.
        char *url = read_xbuf->ptr;
        if(url[0] == 'P' && url[1] == 'O' && url[2] == 'S' && url[3] == 'T' && url[4] == ' ')
          url += 5;
        if(url[0] == 'G' && url[1] == 'E' && url[2] == 'T' && url[3] == ' ')
          url += 4;
        if(url != read_xbuf->ptr && !startwith(url,"/g/",sizeof("/g/") -1 ) )
        {
          static const char msg[] = "Host: ";
          char *host = xbuf_findstr(read_xbuf,msg);
          if(host)
          {
            //int cur_worker = (int)get_env(argv,CUR_WORKER);
            //char *hostbuf = gv_hostbuf[cur_worker-1];
            char hostbuf[host_buf_len];
            //printf("curret worker = %d",cur_worker);
            char *s = hostbuf;
            host += sizeof(msg) - 1;
            strcpy(s,"/www.");
            s+=5;

            size_t left = read_xbuf->len - (host - read_xbuf->ptr); //the left valid content.
            int dotcount = 0; //the count of '.'
            if(left > (host_buf_len - 6)) left = host_buf_len - 6; //reduce left to safe length. 6 include ternimal space.

            //printf("left =  %d\r\n",left);
            while(left-- > 0 && *host != '\r')
            {
              if(*host == '.') dotcount++;
              *s++ = tolower(*host++);
            }
            //printf("left =  %d\r\n",left);
            
            //we are in error situation. insufficent space?
            if(*host != '\r')
              break;

            //len store the length that insert to xbuf_read.
            size_t len = s - hostbuf;
            *s++ = 0;
            
            //printf("len =  %d,hostbuf=%s\r\n",len,hostbuf);
            //assign valid host prefix to s pointer.
            if(dotcount == 1)
            {//one dot mean main-domain.
              s = hostbuf;
            }else{
              s = hostbuf+4;
              *s = '/';
              len -= 4;
            }
            
            //printf("len =  %d,s=%s\r\n",len,s);
            
            //printf("before rewrite: %s\r\n",url);
            xbuf_insert(read_xbuf,url,len,s);
            //printf("after rewrite: %s\r\n",read_xbuf->ptr);

          }
        }
#if 0
         //The server rewrite solution is discard. Because xbuf_t of G-Wan is heavily thread-aware.
	 //I can not operate it just like follow code. read_xbuf is MUTABLE.
	
         //char *szRequest = (char*)get_env(argv, REQUEST);
         // do something with the request (re-write URL? do it in-place)
         xbuf_t *read_xbuf = (xbuf_t*)get_env(argv, READ_XBUF);
	 
#define	 BUFFER_LEN  1024	 
	 char buffer[BUFFER_LEN];
	 s32  len = xbuf_getln(read_xbuf,buffer,BUFFER_LEN-1);
	 if(len < BUFFER_LEN)
	 {
	   buffer[len] = 0;
	   printf("%s\r\n",buffer);
	   char *s = buffer;

	   //GET /?served_from.c HTTP/1.1
	   char *url_begin; //url_begin point to url in header buffer.
	   size_t url_length; //url length in read_xbuf.
	   int  i; //temp cycle. use it with initialization.
	  
	   while(*s && *s != ' ') s++;
	   while(*s && *s == ' ') s++;
	   //not a valid http head.
	   if(!*s) break;
	  
	  
	  url_begin = s;
	  while(*s && *s != ' ') s++;
	  url_length = s - url_begin - 1;
	  
	  //not a valid http head.
	  if(!*s) break;
	  {//Check first line end with ' HTTP/1.1\r\n'
	    char msg[] = " HTTP/1.1\r\n";
	    s = startwith(s,msg,sizeof(msg)-1);
	    //Not a valid HTTP/1.1 head.
	    if(!s) break;
	  }
	  
	  //Run to here. 's' pointer to next line.
	  if(url_length == 1 && *url_begin == '/')
	  {//rewrite root directory.
	  }else if(url_length > 3 && url_begin[0] == '/' && url_begin[1] == 'd' && url_begin[2] == '/')
	  {//rewrite chandler.
	    //we replace first char with '?'. So we can redirect it to csp.
	    url_begin[3] = '?';
	    for(i = 4; i < url_length; i++)
	    {
	      //Change old '?' to '&' if exist.
	      if(url_begin[i] == '?')
	      {
		url_begin[i]='&';
		break;
	      }
	    }
	  }else if(url_length > 5 && strnstr(url_begin,".html",url_length))
	  {//rewrite locale. if this rewrite isn't expect, please use .htm suffix.
	  }
	 }
         
         while(*s && *s != ' ') s++;
         while(*s && *s == ' ') s++;
         //not a valid http head.
         if(!*s) break;
         
         url_begin = s;
	 while(*s && *s != ' ') s++;
	 url_length = s - url_begin - 1;
	 
	 //not a valid http head.
	 if(!*s) break;
	 {//Check first line end with ' HTTP/1.1\r\n'
	   char msg[] = " HTTP/1.1\r\n";
	   s = startwith(s,msg,sizeof(msg)-1);
	   //Not a valid HTTP/1.1 head.
	   if(!s) break;
	 }
	 
	 debug_count(1 + url_length);
	 
	 //Run to here. 's' pointer to next line.
	 if(url_length == 1 && *url_begin == '/')
	 {//rewrite root directory.
	 }

	 debug_count(2);
	 
	 if(url_length > 3 && url_begin[0] == '/' && url_begin[1] == 'd' && url_begin[2] == '/')
	 {//rewrite chandler.
	   //we replace first char with '?'. So we can redirect it to csp.
	   url_begin[3] = '?';
	   for(i = 4; i < url_length; i++)
	   {
	     //Change old '?' to '&' if exist.
	     if(url_begin[i] == '?')
	     {
	       url_begin[i]='&';
	       break;
	     }
	   }
	 }
	 
	 debug_count(3);

	 if(url_length > 5 )//&& strnstr(url_begin,".html",url_length))
	 {//rewrite locale. if this rewrite isn't expect, please use .htm suffix.
	 }
	 
	 printf("\r\n");
	 
         if(*s++ == '/' && *s++ == 'd' && *s ++ == '/')
         {
           
           *s ++ = '?';
           //xbuf_replfrto(read_xbuf, read_xbuf->ptr, read_xbuf->ptr + 16, "/blog", "/?blog");
           //printf("after rewrite:%s",read_xbuf->ptr);
         }else{
           //char *host=0l,*agent=0l,*cookie=0l,*accept_lang=0l;
           accept_language_t langset[8];
           accept_language_t preferred_lang;
           int langset_number = 0;
           size_t url_length;
           preferred_lang[0] = 0;
           do{
             s = getNextLine(s);
	     debug_count(20);
             switch(*s)
             {
             case 'A': //Accept-Language:
               {
                 char msg[] = "ccept-Language:";
                 char *a = startwith(++s,msg,sizeof(msg) - 1);
                 if(a)
                 {
		   debug_count(30);
                   char *nline = getNextLine(a) - 2;
                   char *lang = langset[0];
                   //@FIXME: check MAX_LANG_LEN to avoid memory overflow attack.
                   while(a < nline){//parsing accept-language. assign result to langset and langset_number.
                     if(*a == ','){ //get next language.
                       *lang = 0;
                       lang = langset[++langset_number];
                       a++;
                       continue;
                     }else if(*a == ';'){ //skip q statement.
                       *lang = 0;
                       lang = langset[++langset_number];
                       a++;
                       while(a < nline && *a != ',') a++;
                       if(*a == ',') a++;
                       else break;
                     }
                     *lang++ = *a++;
                   }
                   #if 0
                   if(langset_number)
                   {///TESTING. print current langset gatherd.
                     int i = 0;
                     for(; i < langset_number; i++)
                     {
                       printf("%dth of %d accepted language is : '%s'\r\n",i+1,langset_number,langset[i]);
                     }
                   }
                   #endif
                 }
               }
               break;
             case 'H': //Host
               {
                 char msg[] = "ost:";
                 char *host = startwith(++s,msg,sizeof(msg) - 1);
                 if(host)
                 {//Lookup hostname from database/kv table?. then rewrite to user-directory.
                 }
                 debug_count(40);
               }
               break;
             case 'U': //User-Agent
               {
                 char msg[] = "ser-Agent:";
                 char *agent = startwith(++s,msg,sizeof(msg) - 1);
                 if(agent){
                 #if 0
                   //TESTING: output the agent.
                   char *nextline = getNextLine(agent) - 2;
                   char buffer[1024];
                   strncpy(buffer,agent,nextline-agent);
                   printf("222 user agent:%s\r\n",buffer);
                 #endif
                 }
                 debug_count(50);
               }
               break;
             case 'C': //Cookie
               {
                 char msg[] = "ookie:";
                 char *cookie = startwith(++s,msg,sizeof(msg) - 1);
                 if(cookie){
                   char *nextline = getNextLine(cookie) - 2;
                   //We search uls-languages= to find the preferred language. 
                   char msg[] = "uls-languages=";
                   const char* lang = strnstr(cookie,msg,nextline - cookie);
                   if(lang){
                     char *d = preferred_lang;
                     int c = 0;
                     while( *lang && *lang != ';' && *lang != '\r' && c++ < MAX_LANG_LEN )
                       *d++ = *lang;
                   }
                 }
                 debug_count(60);
               }
               break;
             }
           }while(*s);
           
           debug_count(100);

	   url_length = 0;
           s = url_begin;
	   debug_count(101);
           while(*s++ != ' ') url_length++;
           
      	   debug_count(100 + url_length);
	   if(url_length != 15)
	   {
	     printf("\r\ncontent=%s",url_begin);
	   }
           if(url_begin[1] == ' ' || strnstr(url_begin,".html",url_length+1) != 0l)
           {
             char prefix_path[32];
             char *p = prefix_path;
             char *l = preferred_lang;
             *p++ = '/';
             //add language prefix:
             if(*l){
               while(*l) *p++ = *l++;
             }else{
               //@FIXME: fetch the implemented langset.
               char *lang = langset[0];
               while(*lang) *p++ = *lang++;
             }
             debug_count(70);
             xbuf_insert(read_xbuf,url_begin,p-prefix_path,prefix_path);
           }
         }
         printf("\r\n");
	 #endif
         #if 0
         printf("after rewrite:%s",read_xbuf->ptr);
         #endif
      }
      break;
      // ----------------------------------------------------------------------
      // BEFORE_PARSE return values:
      //   0: Close the client connection
      //   2: Send a server reply based on a reply buffer/HTTP status code
      // 255: Continue (parse the request and its HTTP Headers)
      case HDL_BEFORE_PARSE:
      {
         // here we could bypass HTTP and do something else
      }
      break;
      // ----------------------------------------------------------------------
      // AFTER_PARSE return values:
      //   0: Close the client connection
      //   2: Send a server reply based on a reply buffer/HTTP status code
      // 255: Continue (build a reply)
      case HDL_AFTER_PARSE:
      {
         // here we can take action based on HTTP Headers, to alter the 
         // request or bypass G-WAN's default behavior
      }
      break;
      // ----------------------------------------------------------------------
      // HTTP_ERRORS return values:
      //   0: Close the client connection
      //   2: Send a server reply based on a custom reply buffer
      // 255: Continue (send a reply based on the request HTTP code)
      case HDL_HTTP_ERRORS:
      {
        // here we could use our data->log file to log custom events
        // char string[256];
        // s_snprintf(string, sizeof(string)-1, "whatever", x,y,z);
        // fputs(string, data->log);
        int *pHTTP_status;
        pHTTP_status = (int*)get_env(argv, HTTP_CODE);
        if(pHTTP_status && *pHTTP_status == 404) // is it a 404 error?
        {
          u32 mod = 0, len = 0;
          http_t *http = (http_t*)get_env(argv, HTTP_HEADERS);
          char *request = (char*)get_env(argv, REQUEST);
          char *s = request;
          size_t url_length = 0;
          while(*s++ != ' '); //skip GET
          while(*s++ == ' '); //skip ' '
          while(*s && *s != ' ' && *s != '\r') s++,url_length++; //skip URL
          
          //printf("len:%d,s:%s,request:%s",url_length,s,request);
          
          if(url_length > 5 && endwith(s,".html",5))
          {
            char *c = cacheget(argv, "index.html", &len, 0, pHTTP_status, &mod, 0);
            if(!c)
            {
              // since we fetch index.html from the cache, make sure that it's there
              char str[1024] = {0};
              char *wwwpath = (char*)get_env(argv, WWW_ROOT);// get "/www" path
              xbuf_t f;      // create a dynamic buffer
              xbuf_init(&f); // initialize buffer

              s_snprintf(str, 1023, "%s/index.html", wwwpath);   // build full path
              
              xbuf_frfile(&f, str);
              if(f.len)
              {
                cacheadd(argv, "index.html", f.ptr, f.len, ".html", 200, 0); // never expire
                c = cacheget(argv, "index.html", &len, 0, pHTTP_status, &mod, 0);
                xbuf_free(&f);
              }
            }
            if(c)
            {
              *pHTTP_status = 200; // setup 200.
              char *date = (char*)get_env(argv, SERVER_DATE);
              char szmodified[64];
              static char buf[] =
                "HTTP/1.1 %s\r\n"
                "Date: %s\r\n"
                "Last-Modified: %s\r\n"
                "Content-type: text/html\r\n"
                "Content-Length: %u\r\n" // HTML body length
                "Connection: keep-alive\r\n\r\n";
                  build_headers(argv, buf,
                http_status(*pHTTP_status), // "200 OK" here
                date,                       // current HTTP time
                time2rfc(mod, szmodified)  // file HTTP time
                ,len);                       // file length
      /**	      static const char buf[] =
            "HTTP/1.1 %s\r\n"
            "Date: %s\r\n"
            "Last-Modified: %s\r\n"
            "Content-type: text/html\r\n"
            "Not-Found: %s\r\n" //Cutome header for cliend process.(We avoid using Referer here).
            "Content-Length: %u\r\n" // HTML body length
            "Connection: keep-alive\r\n\r\n";
            
              xbuf_xcat(get_reply(argv), buf,
            http_status(*pHTTP_status), //"200 OK" here
            date,time2rfc(mod, szmodified),
            urlbuf,len);*/
              set_reply(argv, c, len, *pHTTP_status); // no copy
              return 2; // 2: Send a server reply          */
            }
          }
        }
      }
      break;
      // ----------------------------------------------------------------------
      // BEFORE_WRITE return values:
      //   0: Close the client connection
      // 255: Continue (send a server reply based on a reply buffer/HTTP code)
      case HDL_BEFORE_WRITE:
      {
         // here we could use our data->log file to log custom events
         // char string[256];
         // s_snprintf(string, sizeof(string)-1, "whatever", x,y,z);
         // fputs(string, data->log);

// example:
static const char header[] = "Powered-by: ANSI C scripts\r\n";
http_header(HEAD_ADD, header, sizeof(header) - 1, argv);
	 
      }
      break;
      // ----------------------------------------------------------------------
      // AFTER_WRITE return values:
      //   0: Close the client connection
      // 255: Continue (close or keep-alive or continue sending a long reply)
      case HDL_AFTER_WRITE:
      {
         // added per the request of someone
      }
      break;
   }
   return(255); // continue G-WAN's default execution path
}
// ============================================================================
// End of Source Code
// ============================================================================
