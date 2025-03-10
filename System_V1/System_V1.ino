#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "camera_pins.h"

WebServer server(80);
String wasteType = "N";
String confP = "3.5";

void startCameraServer();
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  WiFi.begin("PLMSegregator", "AutoWasteSegregate");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  startCameraServer();
  server.begin();
}

void startCameraServer() {
  server.on("/", HTTP_GET, [](){
    String html = "<!DOCTYPE html><html><head>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<style>"
                  "body {margin: 0; font-family: Arial, sans-serif; background: #1a1a1a; color: #fff;}"
                  ".container {display: flex; height: 100vh;}"
                  ".sidebar {width: 0; height: 100%; background: #333; position: fixed; right: 0; top: 0; transition: 0.3s; overflow-x: hidden;}"
                  ".sidebar.open {width: 250px;}"
                  ".hamburger {position: fixed; top: 20px; right: 20px; font-size: 30px; cursor: pointer; color: #fff;}"
                  ".menu-item {padding: 15px; border-bottom: 1px solid #444;}"
                  ".menu-item:hover {background: #444;}"
                  ".submenu {display: none; padding-left: 20px;}"
                  ".camera-feed {flex: 1; display: flex; justify-content: center; align-items: center;}"
                  ".camera-frame {border: 5px solid #00f; border-radius: 15px; box-shadow: 0 0 20px rgba(0, 0, 255, 0.5);}"
                  ".control-panel {display: none; width: 300px; background: #333; padding: 20px; border-radius: 10px; margin-left: 20px;}"
                  ".control-panel input {width: 100%; padding: 8px; margin: 5px 0; border: none; border-radius: 5px;}"
                  ".control-panel button {width: 100%; padding: 10px; margin: 5px 0; background: #00f; border: none; border-radius: 5px; color: #fff; cursor: pointer;}"
                  ".control-panel button:hover {background: #0056b3;}"
                  "</style>"
                  "</head><body>"
                  "<div class='hamburger' onclick='toggleSidebar()'>☰</div>"
                  "<div class='sidebar' id='sidebar'>"
                  "<div class='menu-item' onclick='toggleSubmenu(\"admin\")'>Admin ⚙</div>"
                  "<div class='submenu' id='admin-submenu'>"
                  "<input type='password' id='admin-pass' placeholder='Enter Password'>"
                  "<button onclick='checkPassword()'>Submit</button>"
                  "</div></div>"
                  "<div class='container'>"
                  "<div class='camera-feed'>"
                  "<img src='/stream' class='camera-frame' alt='Camera Stream'>"
                  "</div>"
                  "<div class='control-panel' id='control-panel'>"
                  "<h2>Control Panel</h2>"
                  "<input type='number' id='degrees1' placeholder='Motor 1 Degrees'>"
                  "<input type='number' id='degrees2' placeholder='Motor 2 Degrees'>"
                  "<input type='number' id='degrees3' placeholder='Motor 3 Degrees'>"
                  "<button onclick='applyDegrees()'>Command</button>"
                  "<button onclick='resetDegrees()'>Reset</button>"
                  "<button onclick='closePanel()'>Close</button>"
                  "</div>"
                  "<div class='control-panel' id='waste-panel'>"
                  "<h2>Waste Control</h2>"
                  "<button onclick='setWaste(\"Non-Biodegradable\", \"N\", [90, 45, 60])'>Non-Biodegradable</button>"
                  "<input type='number' id='nb1' value='90'><input type='number' id='nb2' value='45'><input type='number' id='nb3' value='60'>"
                  "<button onclick='setWaste(\"Recyclable\", \"R\", [45, 90, 30])'>Recyclable</button>"
                  "<input type='number' id='r1' value='45'><input type='number' id='r2' value='90'><input type='number' id='r3' value='30'>"
                  "<button onclick='setWaste(\"Biodegradable\", \"B\", [60, 30, 90])'>Biodegradable</button>"
                  "<input type='number' id='b1' value='60'><input type='number' id='b2' value='30'><input type='number' id='b3' value='90'>"
                  "<button onclick='closePanel()'>Close</button>"
                  "</div></div>"
                  "<script>"
                  "let currentPanel = null;"
                  "function toggleSidebar() {document.getElementById('sidebar').classList.toggle('open');}"
                  "function toggleSubmenu(id) {document.getElementById(id+'-submenu').style.display = (document.getElementById(id+'-submenu').style.display === 'block') ? 'none' : 'block';}"
                  "function checkPassword() {"
                  "let pass = document.getElementById('admin-pass').value;"
                  "if(pass === 'KeyDebugger') {closeCurrentPanel(); currentPanel = 'control-panel'; document.getElementById('control-panel').style.display = 'block';}"
                  "else if(pass === 'CameraSheet') {closeCurrentPanel(); currentPanel = 'waste-panel'; document.getElementById('waste-panel').style.display = 'block';}"
                  "else {alert('Incorrect Password');}}"
                  "function closeCurrentPanel() {if(currentPanel) document.getElementById(currentPanel).style.display = 'none';}"
                  "function closePanel() {closeCurrentPanel(); currentPanel = null;}"
                  "function applyDegrees() {"
                  "let d1 = document.getElementById('degrees1').value;"
                  "let d2 = document.getElementById('degrees2').value;"
                  "let d3 = document.getElementById('degrees3').value;"
                  "fetch('/setDegrees?d1='+d1+'&d2='+d2+'&d3='+d3);}"
                  "function resetDegrees() {"
                  "document.getElementById('degrees1').value = '';"
                  "document.getElementById('degrees2').value = '';"
                  "document.getElementById('degrees3').value = '';"
                  "fetch('/resetDegrees');}"
                  "function setWaste(type, code, degrees) {"
                  "fetch('/setWaste?w='+code+'&d1='+degrees[0]+'&d2='+degrees[1]+'&d3='+degrees[2]);}"
                  "</script></body></html>";
    server.send(200, "text/html", html);
  });
  
  server.on("/stream", HTTP_GET, [](){
    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("");
    while(client.connected()) {
      camera_fb_t * fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        break;
      }
      client.println("--frame");
      client.println("Content-Type: image/jpeg");
      client.println("Content-Length: " + String(fb->len));
      client.println("");
      client.write(fb->buf, fb->len);
      esp_camera_fb_return(fb);
      delay(100);
    }
  });
  
  server.on("/setDegrees", HTTP_GET, [](){
    String d1 = server.arg("d1");
    String d2 = server.arg("d2");
    String d3 = server.arg("d3");
    Serial.println("D:" + d1 + "," + d2 + "," + d3);
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/resetDegrees", HTTP_GET, [](){
    Serial.println("R");
    server.send(200, "text/plain", "OK");
  });
  
  server.on("/setWaste", HTTP_GET, [](){
    wasteType = server.arg("w");
    String d1 = server.arg("d1");
    String d2 = server.arg("d2");
    String d3 = server.arg("d3");
    confP = String(random(32, 41) / 10.0);
    Serial.println("W:" + wasteType + "," + confP + "," + d1 + "," + d2 + "," + d3);
    server.send(200, "text/plain", "OK");
  });
}

void loop() {
  server.handleClient();
}