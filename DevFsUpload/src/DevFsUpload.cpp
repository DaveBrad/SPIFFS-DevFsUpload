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
// See "DevFsUpload.h" for usage information

#include "DevFsUpload.h"
#include "Arduino.h"
#include "FS.h"
#include "ESP8266WebServer.h"
#include "WiFiClient.h"

// the server object to operate through

ESP8266WebServer* DevFsUpload::serverPabc;

boolean DevFsUpload::uploadStarted = false;
boolean DevFsUpload::uploadAction = false;
boolean DevFsUpload::listFilesAction = false;

// SPIFFS file names may not exceed 32 chars [31 + \0] of a char string
String DevFsUpload::viewFileName;

String DevFsUpload::upldFileList = "";
char* DevFsUpload::errUpl = "";
String DevFsUpload::errUplAdd = "";

const char* DevFsUpload::lastFileTable1  = "<style>.tab td:nth-child(2){text-align: right;}</style>";
const char* DevFsUpload::lastFileTable2  = "<table class='tab'><tr><th>File name</th><th>Size</th></tr>";

File DevFsUpload::fsUploadFile;

// This code is based on original from ESP8266 documentation and webpage
// "A Beginner's Guide to the ESP8266" Chapter 11 SPIFFS, Uploading files to SPIFFS
//
// the code has been modified to provide only upload file to
// do development of applications SPIFFS http files more rapidly

DevFsUpload::DevFsUpload() {

}

/**
 setup the server with the appropriate handle function and onFileUpload
 capability
 */
void DevFsUpload::setupUpLoad(ESP8266WebServer* serverP) {
	//  record the server object for use within this class
	serverPabc = serverP;
	
	viewFileName = "";
  
	// set handles for the various needs
	// 1) GET->initial load, 2a) POST->onFileUpLoad 2b) POST->List,..
	//
	//      2a implies POST action/form and once the upload is
	//      complete the regular POST action happens with no parameters
	//
	serverPabc->on("/upload", HTTP_GET, DevFsUpload::handleUploadPage); // 1a
	serverPabc->onFileUpload(DevFsUpload::handleFileUpload);            // 2a
	serverPabc->on("/upload", HTTP_POST, DevFsUpload::handleOther);  // 2b
}

/**
   Completion page presented after upload or abort conditions happen.
   Along with any file-list or error information.
*/
void DevFsUpload::handleUploadPage() {
  // has dual purpose as 'initial page' and 'on upload completed'
  // for an upload selection

  // all self contained (making the "strings" const char* PROGMEM increases
  // the program store significantly vs the code below)
  //
  // - on initial page, only the form is presented
  // - after an upload is completed or abort, upldFileList information is
  //   added/injected into HTML after the <form>
  // -- the HTTP response provides a completed page with upldFileList info
  //
  WiFiClient client = serverPabc->client();
  DevFsUpload::respondHttp200(client, true);
  DevFsUpload::doctypeBody(client);
   
  client.println("<p>1-20 files ideal to prevent memory issues in upload execution on device.</p>");
  
  client.println("<form method='post' enctype='multipart/form-data'><input type='file' name='name' multiple onchange='upldbt(\"upldid\");'><button class='btt' id='upldid' disabled>Upload file(s)</button> to SPIFFS root</form>");
 
  client.println("<form method='post' enctype='multipart/form-data'><input type='file' name='name' multiple webkitdirectory mozdirectory onchange='upldbt(\"upldiddir\");' ><button class='btt' id='upldiddir'  disabled>Upload Directory</button> all files in: selected-dir and sub-dir(s)</form>");
  
  if(uploadAction){
	if (upldFileList != "") {
		client.println("<br>Last upload:");
		client.println(upldFileList);

		// reported once only
		upldFileList = "";
		uploadStarted = false;
	}
	if (errUpl != "") {
		String s = "<div>";
		s += errUpl;
		s += errUplAdd;
		s += "</div>";
		
		Serial.println("error in file xxxxxxx");
		Serial.println(s);
		
		client.println(s);

		// reported once
		errUpl = "";
		errUplAdd = "";
		uploadStarted = false;
	}
	uploadAction = false;
  }
  client.println("<br><form method='post'><input class='btt' type='submit' name='list' value='List  SPIFFS'></form>");
  
  if(listFilesAction){
	// the string is placed as one string rather than individual lines
	// as it saves ~20 bytes of prog mem per client.print invoke
	//
	// magic number 155 will be 6 lines and a bit at 22px per line displayed
	// alter display to have scroll on left of page
	//	  
	client.print("<div class='scrl' tabindex='-1'><div id='lsttab' class='bof dvtb'>");

	Dir dir = SPIFFS.openDir("/");
	while (dir.next()) {
		String str = "<div>" + dir.fileName() + " " + dir.fileSize() + "</div>";	
		//
		// The above information will be changed to the following via a client javascript 'onload'
		//
		// String str = "<form class='frmRow' method='post'><div class='tdi'><input type='text' name='fname' readonly value='";
		// str += dir.fileName();
		// str += "' size='";
		// str += len;
		// str += "'></div><div class='tdi siz'>";
		// str += dir.fileSize();
		// str += "</div><div class='tdi'><input class='btt' type='submit' name='delete' value='Delete'></div>";
		// str += "</form>";
		
		client.println(str);
	}	  
	client.print("</div></div>");
	listFilesAction = false; 
	
	if(viewFileName != ""){
		processViewFile();
		// reset the file being viewed
		viewFileName = "";
	}
	// a lot of files (not typical) could cause ESP Watch Dog Timer issue
	// thus a yield
	yield();
  }
  client.println("</body></html>");
  client.stop();
}

void DevFsUpload::respondHttp200(WiFiClient client, boolean htmlText) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  if(htmlText){
    client.println("text/html");
  }else{
	client.println("text/plain");
  }
  client.println("");
}
void DevFsUpload::doctypeBody(WiFiClient client) {
  client.println("<!DOCTYPE HTML><html lang=\"en\"><head><meta charset=\"UTF-8\" />");
  
    client.println("<style>");
    client.println(".btt{border-radius: 15px;}.btt:focus{border: 2px solid blue;}");
  
  	client.println(".bof{direction:ltr;float:left;}.scrl {margin-left:24px;height:155px;overflow-y:scroll; direction:rtl;}");
	//
	client.println(".dvtb{display:table;}.frmRow{display:table-row;}.frmRow div:nth-child(1) input{border: 0px;}.tdi{display:table-cell;padding-right: 10px;}.siz{text-align: right;}");
	
	// each 3rd line is hi-lite to ease alignment for readability
	client.println(".dvtb form:nth-child(3n) div:nth-child(1) input:nth-child(1),.dvtb form:nth-child(3n) div:nth-child(n-2){background:#ffffcc;}");
  
    client.println("</style>");
	
	
	client.println("<script >");
	
	// function that sets the upload buttons enabled when files are selected
	// and set by an associated browse button
	client.println("function upldbt(idstr){");
	client.println(" var itemEle = document.getElementById(idstr);");
	client.println(" itemEle.disabled = this.value === '';");
	client.println("}");
		
	// function to sort the file names for the 'list SPIFFS' request as 
	// SPIFFS gathers files in a none alphabetic order
	client.println("function sortTab(){");
	client.println("var tab = document.getElementById('lsttab');");

	client.println("if(tab === null){");
	client.println("  return;");
	client.println("}");
	
	client.println("var tabLen = tab.childNodes.length;");
	
	client.println("if(tabLen === 0){");
	client.println("  return;");
	client.println("}");
	
	client.println("var arr = [];");

	client.println("var i;");

	client.println("for(i = 0; i < tabLen; i++){");
	client.println(" var itemOfI = tab.childNodes[i].innerHTML;");
	client.println(" if(itemOfI !== undefined){");
	client.println("  arr.push(itemOfI);");
	client.println(" }");
	client.println("}");
	client.println("arr.sort();");
	
	client.println("while (tab.firstChild) {");
	client.println("    tab.removeChild(tab.firstChild);");
	client.println("}");
	
	client.println("for(i = 0; i < arr.length; i++){");
	client.println(" var str = arr[i];");
	client.println(" var idx = str.lastIndexOf(' ');");
	
	client.println(" var fn = str.substring(0, idx);");
	client.println(" var len = fn.length + 2;");
	
	client.println(" var size = str.substring(idx +1);");
	
	client.println(" var s = \"<form class='frmRow' method='post'><div class='tdi'><input type='text' name='fname' tabindex='-1' readonly value='\";");
	client.println(" s += fn;");
	client.println(" s += \"' size='\";");
	client.println(" s += len;");
	client.println(" s += \"'></div><div class='tdi siz'>\";");
	client.println(" s += size;");
	client.println(" s += \"</div><div class='tdi'><input class='btt' type='submit' name='delete' value='Delete'></div>\";");
	
	client.println(" s += \"<div class='tdi'><input class='btt' type='submit' name='view' value='View'></div>\";");
	
	client.println(" s += \"</form>\";");
	
	client.println(" tab.insertAdjacentHTML('beforeend', s);");
	
	client.println("}");
	client.println("}");
	
	client.println("</script>");
  
  client.println("</head><body onload='sortTab();'>");
}

/**
   Completion page presented after upload or abort conditions happen.
   Along with any file-list or error information.
*/
void DevFsUpload::sendComplete() {
	DevFsUpload::handleUploadPage();
}

/**
 handler for processing list, delete or view request
 */
void DevFsUpload::handleOther() {	
	uploadAction = (serverPabc->args() == 0);
	
	if(uploadAction){
		// assume on-file-upload has completed its request and
		// a regular POST happens to close of the upload action
		// which has 0 (zero) arguments
		upldFileList += "</table>";

	}else if(serverPabc->hasArg("list")){
		// action is to list the files
		listFilesAction = true;
		
	}else if(serverPabc->hasArg("delete")){
		if(SPIFFS.exists(serverPabc->arg("fname"))){
			SPIFFS.remove(serverPabc->arg("fname"));
		}
		listFilesAction = true;
		
	}else if(serverPabc->hasArg("view")){
		listFilesAction = true;
		
		if(SPIFFS.exists(serverPabc->arg("fname"))){
			viewFileName = serverPabc->arg("fname");
		}
	}
	DevFsUpload::sendComplete();
}

/**
 process the file and return the content as text for viewing
 */
void DevFsUpload::processViewFile(){
	if(SPIFFS.exists(viewFileName)){
		WiFiClient client = serverPabc->client();
		File f = SPIFFS.open(viewFileName, "r");
		
		if(!f){
			// no file should not be the case
		}else{
			client.print("<br><p>Viewing: ");
			client.print(viewFileName);
			client.println("</p>");
		
			client.print("<textarea style=\"width: 95%;\" spellcheck=\"false\"  rows=\"15\">");
			int yieldPt = 100;
			while(f.available()){
				client.print(f.readStringUntil('\r'));
				yieldPt--;
				if(yield == 0){
					// a large file (not typical) could cause ESP Watch Dog Timer 
					// issue thus a yield
					yield();
					yieldPt = 100;
				}
			}
			f.close();
			client.println("</textarea>");
			
			if(viewFileName.endsWith(".png") || viewFileName.endsWith(".jpg")
				|| viewFileName.endsWith(".bmp") || viewFileName.endsWith(".gif")){
				// if the file is an imagecthen process into an img tag
				client.print("<img");
				client.print(" src='..");
				client.print(viewFileName);
				client.println("' alt='missing image'>");
			}
		}
	}
}

/**
 upload a new file to the SPIFFS protocol handler
*/
void DevFsUpload::handleFileUpload() {
  HTTPUpload& upload = serverPabc->upload();
  uploadAction = true;

  // the HTTP upload protocol operates as a state machine
  // 1) UPLOAD_FILE_START: requires the file to be opened for writing
  // 2) UPLOAD_FILE_WRITE: a block of data to write
  // 3) UPLOAD_FILE_END: close of the file
  // 4) UPLOAD_FILE_ABORTED: upload is aborted
  //
  // The code below is optimized to check for UPLOAD_FILE_WRITE 1st
  // as the most often task
  //
  // file is open and should be processing
  // ASSUMPTION: protocol will work correctly
  //
  if (uploadStarted) {
    if (upload.status == UPLOAD_FILE_WRITE) {
		if (errUpl == "") {
			fsUploadFile.write(upload.buf, upload.currentSize);
			Serial.print(".");
		}
    } else if (upload.status == UPLOAD_FILE_END) {
		if (errUpl != "") {
			uploadStarted = true;
			DevFsUpload::handleUploadPage();			
		}else{
			// If the file was successfully created
			fsUploadFile.close();
			uploadStarted = false;

			Serial.print(" Size: ");
			Serial.println(upload.totalSize);

			upldFileList += "<td>" + String(upload.totalSize) + "</td></tr>";
		}

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      Serial.print("Aborted: ");
      Serial.println(upload.filename);
    }
  } else {
    // file not open so expect an upload protocol error

    if (upload.status != UPLOAD_FILE_START) {
      if (errUpl == "") {
        errUpl =  "500: protocol order issue";
      }
    } else {
      String fn = upload.filename;
	  
	  boolean failDirFormat = false;
	  int fnLen = fn.length();
	  
	  if(fnLen > 12){
		// this could be a directory upload item
		int remaining = fnLen - fn.lastIndexOf("/") - 1;
		failDirFormat = remaining > 12;
		
		Serial.print("remaining ");
		Serial.println(remaining);
	  }
      if (failDirFormat || fn == "/" || fn == "" ) {
        // 8.3 is 12 char format
        errUpl =  "500: file name issue (8.3 ?): ";
		errUplAdd = String(fn);
      } else {
        // ensure the filename starts with the root directory
        if (!fn.startsWith("/")) {
          fn = "/" + fn;
        }
        if (upldFileList == "") {
          upldFileList = lastFileTable1;
          upldFileList += lastFileTable2;
        }
        upldFileList += "<tr><td>" + fn + "</td>";

        Serial.print("Upload of: ");
        Serial.print(fn);
		Serial.print(" ");

        // Open the file for writing in SPIFFS and overwrite
        // (create if it doesn't exist)
        //
        fsUploadFile = SPIFFS.open(fn, "w");
        if (!fsUploadFile) {
          errUpl = "500: fail create file: ";
		  errUplAdd = String(fn);
        } else {
          uploadStarted = true;
          errUpl = "";
        }
      }
    }
  }
}

