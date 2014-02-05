// ============================================================================
// This file is part of webware.
// Published under the Apache License, Version 2.0.
// Copyright (C) by masol.li@gmail.com.
//
// See https://github.com/masol/webware for more information.
// ============================================================================

// uncomment to get symbols/line numbers in crash reports
#pragma debug

#include "gwan.h"   // G-WAN exported functions
#include "webware.h"

#include <stdio.h>   // printf()
#include <string.h>  // strcmp()

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
  

//  u8 *query_char = (u8*)get_env(argv, QUERY_CHAR);
//  *query_char = '~'; // we must use "/!helloc.c" instead of "/?helloc.c"
 
   
   

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
   *states = (1L << HDL_AFTER_ACCEPT)
       | (1L << HDL_AFTER_READ)
		   | (1L << HDL_BEFORE_PARSE)
		   | (1L << HDL_AFTER_PARSE)
		   | (1L << HDL_BEFORE_WRITE);
   
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

static char* getNextLine(char* s)
{
  while(*s && *s != '\r' &&  *(s+1) != '\n') s++;
  if(*s) s += 2;
  return s;
}

#define	 _DEBUG		1

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
char *strnstr(const char *s1, const char *s2, size_t len)
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
@return if s start with msg, return string start from excess of s and remove blank. otherwise return 0l(NULL).
  IE. 
**/
char* isStartWith(char *s, char *msg,int len)
{
   int i = 0;
   for(;i < len;i++)
   {
     if( *s++ != *msg++) break;
   }
   if(i == len)
   {
      while(*s == ' ') s++;
      return s;
   }
   return (char*)0l;
}


int main(int argc, char *argv[])
{
   // just helping you to know where you are:
/*   static char *states[] =
   {
      [HDL_INIT]         = "0:init()",      // not seen here: init()
      [HDL_AFTER_ACCEPT] = "1:AfterAccept",
      [HDL_AFTER_READ]   = "2:AfterRead",
      [HDL_BEFORE_PARSE] = "3:BeforeParse",
      [HDL_AFTER_PARSE]  = "4:AfterParse",
      [HDL_BEFORE_WRITE] = "5:BeforeWrite",
      [HDL_AFTER_WRITE]  = "6:AfterWrite",
      [HDL_HTTP_ERRORS]  = "7:HTTP_Errors",
      [HDL_CLEANUP]      = "8:clean()",   // not seen here: clean()
      ""
   };
//*/
   long state = (long)argv[0];
/*   if(state >= 0 && state <= HDL_CLEANUP)
    printf("Handler state:%ld:%s\n", state, states[state]);
   else
    printf("Handler state:%ld\n", state);
//*/ 
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
      case HDL_AFTER_READ:
      {
         //char *szRequest = (char*)get_env(argv, REQUEST);
         // do something with the request (re-write URL? do it in-place)
         xbuf_t *read_xbuf = (xbuf_t*)get_env(argv, READ_XBUF);
         char *s = read_xbuf->ptr;
         char *url_begin; //url_begin point to url in header buffer.
         while(*s && *s != ' ') s++;
         while(*s && *s == ' ') s++;
         debug_count(10);
	 if(!*s) debug_count(11);
         url_begin = s;
         if(*s++ == '/' && *s++ == 'd' && *s ++ == '/')
         {
           //we replace first char with '?'. So we can redirect it to csp.
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
                 char *a = isStartWith(++s,msg,sizeof(msg) - 1);
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
                 char *host = isStartWith(++s,msg,sizeof(msg) - 1);
                 if(host)
                 {//Lookup hostname from database/kv table?. then rewrite to user-directory.
                 }
                 debug_count(40);
               }
               break;
             case 'U': //User-Agent
               {
                 char msg[] = "ser-Agent:";
                 char *agent = isStartWith(++s,msg,sizeof(msg) - 1);
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
                 char *cookie = isStartWith(++s,msg,sizeof(msg) - 1);
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
