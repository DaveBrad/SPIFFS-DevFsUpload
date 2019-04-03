/* Copyright (c) 2018 dbradley. 

License: 

Use is royality free for development activities (testing, design,....) 
both commerical and non-commerical.

Modification may be made with the provision:
-- that a different class name is used. That is; not 'DevFsUpload'. 
-- An attribute to original program/author is provided in the new code/class.

The code/class is provide AS-IS and has no warranty as to fitness for 
use. Determining fitness for use is the responsibility of the user.

This code/class may not be productized, that is provided with/along/packaged-with 
a consumer product, either commerical or non-commerical.

As a program there are working limits on an embedded MCU device.

*/
// --------------  upload files to SPIFFS for rapid file development ----------- //
//
// WARNING: code is not re-entrant and only supports ONE browser request at a time
//          for uploading file(s)
//
// Purpose:
//   During development on a SPIFFS supported device (eg. ESP8266,...) using an
//   IDE to upload files into the SPIFFS is often time consuming.
//
//   This method allows files to be uploaded to the SPIFFS device independently
//   of the IDE via a browser interface.
//
// Limitations:
//   1-20 file(s) upload at a time, greater than 20 is possible but RAM memory
//   will be used processing the file-uploaded-info and if too many, may cause a crash.
//
// Browser access:
// http://<host-of-device>/upload
//
//
// Usage in code:
// -- To use edit a projects .ino file after include section, 
// -- include the DevFsUpload.h, else do not 
// ---------
//  Example 1                                Example 2
//  ------------------------------------     -------------------------------------
//  Server is defined as a pointer           Server is typically shown in examples
//  which will be 'set' in the setup         as being just after the include headers
//  function.                                section.
//
//                                           Note: &server
//  ------------------------------------     -------------------------------------
//  ::                                       ::
//  #include "DevFsUpload.h"                 #include "DevFsUpload.h"
//  ..                                       ..   
//  ESP8266WebServer* server;                ESP8266WebServer server(80);
//  ..                                       ..
//  setup(){  
//    ..
//   server = new ESP8266WebServer(80);      setup(){
//	  ..                                      ..
//   // install DevFsUpload server handle    // install DevFsUpload server handle
//   #ifdef DevFsUploadInstall               #ifdef DevFsUploadInstall
//     DevFsUploadInstall(server);             DevFsUploadInstall(&server);    
//   #endif                                  #endif
//	  ..                                      ..
//
//       DevFsUploadInstall installs the following code equivalent
//       - - - - - - - - - 
//       --- '<host>/upload'      GET/POST for upload action
//       --- 
//       ------ server->on("/upload", HTTP_GET, DevFsUpload::handleUploadPage);
//       ------ server->onFileUpload(DevFsUpload::handleFileUpload);
//       ------ server->on("/upload", HTTP_POST, DevFsUpload::handleOther);
//

#ifndef DevFSUpload_h
#define DevFSUpload_h

#define DevFsUploadInstall(...) DevFsUpload::setupUpLoad(__VA_ARGS__)

#include "Arduino.h"
#include "FS.h"
#include "ESP8266WebServer.h"

class DevFsUpload {
  public:
    DevFsUpload();
    
	/**
	 setup the server with the appropriate handle function and onFileUpload
     capability
    */
    static void setupUpLoad(ESP8266WebServer* serverP);

  private:
    static ESP8266WebServer* serverPabc;
    static boolean uploadStarted;
	static boolean uploadAction;
	static boolean listFilesAction;
	static void processViewFile();

    static String upldFileList;
    static char* errUpl;
	static String errUplAdd;
	
	static String viewFileName;

    static const char* lastFileTable1;
    static const char* lastFileTable2;

    static File fsUploadFile;

	static void handleUploadPage();
    static void handleFileUpload();
	
	static void handleOther();
	
	static void sendComplete();
	static void respondHttp200(WiFiClient client, boolean htmlText);
	static void doctypeBody(WiFiClient client);
};
#endif
