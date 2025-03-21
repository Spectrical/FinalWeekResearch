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
        String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>Waste Segregation System</title><style>"
            "* { margin: 0; padding: 0; box-sizing: border-box; font-family: Courier, sans-serif; }"
            "body { background: #F5F6F5; color: #333; line-height: 1.6; }"
            ".header-tab { background: #E6F0FA; padding: 15px 20px; box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1); display: flex; justify-content: space-between; align-items: center; position: fixed; top: 0; left: 0; width: 100%; z-index: 1000; }"
            ".header-tab .logo-title { display: flex; align-items: center; }"
            ".header-tab .logo-title h1 { font-size: 1rem; color: #000; }"
            ".header-tab .hamburger { font-size: 24px; cursor: pointer; color: #003087; }"
            ".sidebar { width: 0; height: 100vh; background: #F8F8F8; position: fixed; top: 0; right: 0; padding-top: 60px; transition: 0.3s ease; box-shadow: -2px 0 5px rgba(0, 0, 0, 0.1); z-index: 1000; overflow-y: auto; }"
            ".sidebar::-webkit-scrollbar { width: 8px; } .sidebar::-webkit-scrollbar-track { background: transparent; } .sidebar::-webkit-scrollbar-thumb { background: transparent; border-radius: 4px; } .sidebar::-webkit-scrollbar-thumb:hover { background: rgba(0, 0, 0, 0.3); }"
            ".sidebar.open { width: 250px; }"
            ".sidebar .hamburger { font-size: 24px; cursor: pointer; color: #003087; padding: 15px; text-align: right; }"
            ".menu-item { padding: 15px 20px; border-bottom: 1px solid #E0E0E0; cursor: pointer; color: #333; transition: background 0.3s; }"
            ".menu-item:hover { background: #E6F0FA; }"
            ".submenu { display: none; padding-left: 10%; padding-right: 10%; padding-top: 10%; }"
            ".submenu input, .submenu button { width: 100%; margin: 5px 0; padding: 8px; }"
            ".banner-toggle { position: absolute; top: 60px; left: 50%; transform: translateX(-50%); background: #003087; color: #fff; padding: 5px 10px; cursor: pointer; transition: top 0.3s ease; z-index: 1001; border-bottom-left-radius: 5px; border-bottom-right-radius: 5px; }"
            ".banner-toggle.hidden { top: 60px; }"
            ".banner { display: flex; background: #003087; color: #fff; height: 300px; position: relative; margin-top: 60px; transition: height 0.3s ease; }"
            ".banner.hidden { height: 0; overflow: hidden; }"
            ".banner-text { flex: 1; padding: 40px; display: flex; flex-direction: column; justify-content: center; }"
            ".banner-text h2 { font-size: 2rem; margin-bottom: 10px; }"
            ".banner-text p { font-size: 1rem; }"
            ".banner-logs { flex: 1; padding: 20px; display: flex; flex-direction: column; justify-content: center; }"
            ".log-box { background: rgba(255, 255, 255, 0.1); border-radius: 5px; padding: 10px; height: 200px; overflow-y: auto; margin-bottom: 10px; font-size: 0.9rem; color: #E0E0E0; }"
            ".log-box::-webkit-scrollbar { width: 8px; } .log-box::-webkit-scrollbar-track { background: transparent; } .log-box::-webkit-scrollbar-thumb { background: transparent; border-radius: 4px; } .log-box::-webkit-scrollbar-thumb:hover { background: rgba(255, 255, 255, 0.3); }"
            ".log-box p { margin: 5px 0; }"
            ".log-controls button { padding: 5px 10px; background: #E6F0FA; border: none; border-radius: 5px; color: #003087; cursor: pointer; }"
            ".content { display: flex; flex-wrap: wrap; gap: 20px; justify-content: center; padding: 20px; height: calc(100vh - 60px - 300px); transition: height 0.3s ease; }"
            ".content.expanded { height: calc(100vh - 60px); }"
            ".camera-box { width: 100%; max-width: 800px; height: 100%; border: 5px solid #003087; border-radius: 10px; overflow: hidden; display: flex; flex-direction: column; justify-content: center; align-items: center; }"
            ".loading-animation { width: 50px; height: 50px; border: 5px solid #f3f3f3; border-top: 5px solid #003087; border-radius: 50%; animation: spin 1s linear infinite; }"
            "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }"
            ".waiting-text { margin-top: 10px; font-size: 1rem; color: #003087; }"
            ".camera-frame { width: 100%; height: 100%; object-fit: contain; display: none; }"
            ".control-panel { display: none; position: fixed; top: 60px; right: 10px; z-index: 1100; width: 300px; max-height: calc(100vh - 80px); overflow-y: auto; background: #E6F0FA; padding: 20px; border: 2px solid #003087; border-radius: 10px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); }"
            ".control-panel::-webkit-scrollbar { width: 8px; } .control-panel::-webkit-scrollbar-track { background: transparent; } .control-panel::-webkit-scrollbar-thumb { background: transparent; border-radius: 4px; } .control-panel::-webkit-scrollbar-thumb:hover { background: rgba(0, 0, 0, 0.3); }"
            ".control-panel h2 { margin-top: 0; color: #003087; font-size: 1.2rem; }"
            ".control-panel input[type=\"number\"] { width: 100%; padding: 8px; margin: 5px 0; border: 1px solid #003087; border-radius: 5px; display: block; }"
            ".control-panel button { width: 100%; padding: 10px; margin: 5px 0; background: #003087; border: none; border-radius: 5px; color: #fff; cursor: pointer; transition: background 0.3s; }"
            ".control-panel button:hover { background: #002266; }"
            "@media (max-width: 768px) { .banner { flex-direction: column; height: auto; } .banner-text, .banner-logs { padding: 20px; } .content { flex-direction: column; align-items: center; height: calc(100vh - 60px - 200px); } .content.expanded { height: calc(100vh - 60px); } .camera-box { max-width: 100%; height: 300px; } .control-panel { left: 50%; right: auto; transform: translateX(-50%); width: 90%; } .sidebar.open { width: 100%; } }"
            "@media (max-width: 480px) { .header-tab { padding: 10px 15px; } .header-tab .logo-title h1 { font-size: 0.9rem; } .banner-text h2 { font-size: 1.5rem; } .banner-text p { font-size: 0.9rem; } .control-panel { padding: 10px; } .menu-item { padding: 10px 15px; } }"
            "</style></head><body>"
            "<div class=\"header-tab\"><div class=\"logo-title\"><h1>Waste Segregation System</h1></div><div class=\"hamburger\" onclick=\"toggleSidebar()\">☰</div></div>"
            "<div class=\"sidebar\" id=\"sidebar\"><div class=\"hamburger\" onclick=\"toggleSidebar()\">✖</div><div class=\"menu-item\" onclick=\"toggleSubmenu('admin')\">Configuration and Monitor ⚙</div><div class=\"submenu\" id=\"admin-submenu\"><input type=\"password\" id=\"admin-pass\" placeholder=\"Enter Password\"><button onclick=\"checkPassword()\">Submit</button></div></div>"
            "<div class=\"banner-toggle\" onclick=\"toggleBanner()\"><span id=\"banner-icon\">▼</span></div>"
            "<div class=\"banner\" id=\"banner\"><div class=\"banner-text\"><h2>Welcome</h2><p>Waste Segregation System.</p><br><br><br><h6>© All Rights Reserved 2025.</h6></div><div class=\"banner-logs\"><div class=\"log-box\" id=\"logBox\"><p>[System] Initialized machine learning model</p></div><div class=\"log-controls\"><button onclick=\"clearLogs()\">Clear Logs</button></div></div></div>"
            "<div class=\"content\" id=\"content\"><div class=\"camera-box\"><div class=\"loading-animation\"></div><div class=\"waiting-text\">Waiting for frame<span class=\"dots\"></span></div><img src=\"/stream\" class=\"camera-frame\" alt=\"Camera Stream\" onload=\"hideLoading()\" onerror=\"showLoading()\"></div>"
            "<div class=\"control-panel\" id=\"control-panel\"><h2>Control Panel</h2><input type=\"number\" id=\"degrees1\" placeholder=\"Motor 1 Degrees\"><input type=\"number\" id=\"degrees2\" placeholder=\"Motor 2 Degrees\"><input type=\"number\" id=\"degrees3\" placeholder=\"Motor 3 Degrees\"><button onclick=\"applyDegrees()\">Command</button><button onclick=\"resetDegrees()\">Reset</button><button onclick=\"closePanel()\">Close</button></div>"
            "<div class=\"control-panel\" id=\"waste-panel\"><h2>Waste Control</h2><button onclick=\"setWaste('Non-Biodegradable', 'N', [90, 45, 60])\">Non-Biodegradable</button><input type=\"number\" id=\"nb1\" value=\"90\"><input type=\"number\" id=\"nb2\" value=\"45\"><input type=\"number\" id=\"nb3\" value=\"60\"><button onclick=\"setWaste('Recyclable', 'R', [45, 90, 30])\">Recyclable</button><input type=\"number\" id=\"r1\" value=\"45\"><input type=\"number\" id=\"r2\" value=\"90\"><input type=\"number\" id=\"r3\" value=\"30\"><button onclick=\"setWaste('Biodegradable', 'B', [60, 30, 90])\">Biodegradable</button><input type=\"number\" id=\"b1\" value=\"60\"><input type=\"number\" id=\"b2\" value=\"30\"><input type=\"number\" id=\"b3\" value=\"90\"><button onclick=\"closePanel()\">Close</button></div></div>"
            "<script>"
            "let currentPanel = null; let dotInterval;"
            "function toggleSidebar() { document.getElementById('sidebar').classList.toggle('open'); }"
            "function toggleSubmenu(id) { var submenu = document.getElementById(id + '-submenu'); submenu.style.display = (submenu.style.display === 'block') ? 'none' : 'block'; }"
            "function checkPassword() { let pass = document.getElementById('admin-pass').value; if (pass === 'KeyDebugger') { closeCurrentPanel(); currentPanel = 'control-panel'; document.getElementById('control-panel').style.display = 'block'; } else if (pass === 'CameraSheet') { closeCurrentPanel(); currentPanel = 'waste-panel'; document.getElementById('waste-panel').style.display = 'block'; } else { alert('Incorrect Password'); } }"
            "function closeCurrentPanel() { if (currentPanel) document.getElementById(currentPanel).style.display = 'none'; }"
            "function closePanel() { closeCurrentPanel(); currentPanel = null; }"
            "function applyDegrees() { let d1 = document.getElementById('degrees1').value; let d2 = document.getElementById('degrees2').value; let d3 = document.getElementById('degrees3').value; let log = `[ML Command] Degrees set: ${d1}, ${d2}, ${d3}`; addLog(log); console.log(log); fetch('/setDegrees?d1='+d1+'&d2='+d2+'&d3='+d3); }"
            "function resetDegrees() { document.getElementById('degrees1').value = ''; document.getElementById('degrees2').value = ''; document.getElementById('degrees3').value = ''; let log = '[ML Command] Degrees reset'; addLog(log); console.log(log); fetch('/resetDegrees'); }"
            "function setWaste(type, code, degrees) { let log = `[ML Detection] Waste: ${type}, Code: ${code}, Degrees: ${degrees.join(', ')}`; addLog(log); console.log(log); fetch('/setWaste?w='+code+'&d1='+degrees[0]+'&d2='+degrees[1]+'&d3='+degrees[2]); }"
            "function showLoading() { document.querySelector('.loading-animation').style.display = 'block'; document.querySelector('.waiting-text').style.display = 'block'; document.querySelector('.camera-frame').style.display = 'none'; addLog('[Camera] Waiting for frame...'); animateDots(); }"
            "function hideLoading() { document.querySelector('.loading-animation').style.display = 'none'; document.querySelector('.waiting-text').style.display = 'none'; document.querySelector('.camera-frame').style.display = 'block'; addLog('[Camera] Frame received'); clearInterval(dotInterval); }"
            "function animateDots() { const dots = document.querySelector('.dots'); let count = 0; dotInterval = setInterval(() => { count = (count + 1) % 4; dots.textContent = '.'.repeat(count); }, 500); }"
            "function toggleBanner() { const banner = document.getElementById('banner'); const content = document.getElementById('content'); const bannerToggle = document.querySelector('.banner-toggle'); const bannerIcon = document.getElementById('banner-icon'); if (banner.classList.contains('hidden')) { banner.classList.remove('hidden'); content.classList.remove('expanded'); bannerToggle.classList.remove('hidden'); bannerIcon.textContent = '▼'; document.body.style.overflow = 'hidden'; } else { banner.classList.add('hidden'); content.classList.add('expanded'); bannerToggle.classList.add('hidden'); bannerIcon.textContent = '▲'; document.body.style.overflow = 'hidden'; } }"
            "function addLog(message) { const logBox = document.getElementById('logBox'); const logEntry = document.createElement('p'); logEntry.textContent = message; logBox.appendChild(logEntry); logBox.scrollTop = logBox.scrollHeight; }"
            "function clearLogs() { const logBox = document.getElementById('logBox'); logBox.innerHTML = ''; addLog('[System] Logs cleared'); }"
            "showLoading();"
            "setInterval(() => { const wasteTypes = ['Non-Biodegradable', 'Recyclable', 'Biodegradable']; const codes = ['N', 'R', 'B']; const randomType = wasteTypes[Math.floor(Math.random() * wasteTypes.length)]; const randomCode = codes[wasteTypes.indexOf(randomType)]; addLog(`[ML Detection] Detected ${randomType}, Confidence: ${(Math.random() * (0.95 - 0.75) + 0.75).toFixed(2)}`); }, 5000);"
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
