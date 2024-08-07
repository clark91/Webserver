//#ifndef HTTPFUNCS_H_
//#define HTTPFUNCS_H_
struct request {
  char* version;
  char* resource;
  char* type;
};

struct request parseReq(char* reqObj);

char* findMsgType(char* resource);
//#endif