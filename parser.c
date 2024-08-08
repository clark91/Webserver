#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "httpfuncs.h"

struct request parseReq(char* reqObj){
  struct request parsed;

  //parses an HTTP request
  int p = 0, type = 0;
  for(int i = 0; i < strlen(reqObj); i++){
    if(reqObj[i] == ' ' || reqObj[i] == '\n'){

      if(strncmp(&reqObj[p], "GET", 3) == 0){
        parsed.type = malloc(i-p);
        strncpy(parsed.type, &reqObj[p], i-p);
        parsed.type[i-p]= '\0';
        //printf("Request Type: %s\n", parsed.type);
      }

      else{
        //Uses count of spaces to identify each header.
        switch (type)
        {
          case 1:
            type++;
            parsed.version = malloc(i - p + 1);
            strncpy(parsed.version, &reqObj[p], i - p);
            parsed.version[i - p] = '\0';
            //printf("Version: %s\n", parsed.version);
            break;

          case 0:
            p++;
            type++;
            parsed.resource = malloc(i-p+1);
            strncpy(parsed.resource, &reqObj[p], i - p);
            parsed.resource[i - p] = '\0';

            //printf("Webpage: %s\n", &reqObj[p]);
            //printf("Webpage: %s\n", parsed.resource);
            break;
          
          default:
            break;
        }
      }
      
      p = i;
    }
  }
  printf("\n\n");

  return parsed;
}

char* findMsgType(char* resource){

  //Finds the file extention
  int i;
  for(i = 0; i < strnlen(resource,100); i++){
    if(resource[i] == '.'){
      break;
    }
  }
  char* ext = resource + i + 1;

  //Returns the corresponding Content-Type for the extension
  if(strcmp(ext, "html") == 0){
    return "text/html";
  }else if(strcmp(ext, "css") == 0){
    return "text/css";
  }else if(strcmp(ext, "js") == 0){
    return "text/javascript";
  }else if(strcmp(ext, "ico") == 0){
    return "image/vnd.microsoft.icon";
  }else if(strcmp(ext, "png") == 0){
    return "image/png";
  }
  else{
    printf("Failed to identfiy file type\n");
    return("*/*");
  }
}