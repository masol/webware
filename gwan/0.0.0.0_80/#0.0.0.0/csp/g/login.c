// ============================================================================
// This file is part of webware.
// Published under the Apache License, Version 2.0.
// Copyright (C) by masol.li@gmail.com.
//
// See https://github.com/masol/webhome for more information.
// ============================================================================

#include "gwan.h"

int main(int argc, char *argv[])
{
  char *username = "", *password = "", *remberme = "";
  get_arg("username=",   &username,   argc, argv);
  get_arg("password=",    &password,    argc, argv);
  get_arg("remberme=", &remberme, argc, argv); 

  xbuf_t *reply = get_reply(argv);
  static char un[] = "USERNAME=";
  xbuf_ncat(reply,un,sizeof(un)-1);
  xbuf_ncat(reply,username,strlen(username));

  static char pw[] = "<BR/>PASSWORD=";
  xbuf_ncat(reply,pw,sizeof(pw)-1);
  xbuf_ncat(reply,password,strlen(password));

  xbuf_ncat(reply,remberme,strlen(remberme));

  return 200;
}

