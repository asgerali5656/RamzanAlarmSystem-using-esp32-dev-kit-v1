#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>

class WebServerManager {
public:
    WebServerManager();
    void init();
    void handleClient(); 
    
private:
    WebServer server;
    
    // Handlers
    void handleRoot();
    void handleStatus();
    void handleSettings();      
    void handleSaveSettings();  
    void handleUpdate();       // Web-based OTA Update Page
    void handleUpdateUpload(); // Web-based OTA Binary Upload logic
    
    // New Features
    void handleDisplayJson();
    void handleMessage();
    
    void handleTest();
    void handleNotFound();
};

#endif
