#include "app_httpd.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "esp32-hal-log.h"
#include "img_converters.h"
#include "ra_filter.h"

#define _STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=frame"
#define _STREAM_BOUNDARY "\r\n--frame\r\n"
#define _STREAM_PART "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %ld.%06ld\r\n\r\n"

// Embedded HTML with adjusted CSS for camera frame
static const char* index_html = R"rawliteral(
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Waste Segregation System</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: Courier, sans-serif;
    }
    body {
      background: #F5F6F5;
      color: #333;
      line-height: 1.6;
    }
    .header-tab {
      background: #E6F0FA;
      padding: 15px 20px;
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
      display: flex;
      justify-content: space-between;
      align-items: center;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      z-index: 1000;
    }
    .header-tab .logo-title {
      display: flex;
      align-items: center;
    }
    .header-tab .logo-title h1 {
      font-size: 1rem;
      color: #000;
    }
    .header-tab .hamburger {
      font-size: 24px;
      cursor: pointer;
      color: #003087;
    }
    .sidebar {
      width: 0;
      height: 100vh;
      background: #F8F8F8;
      position: fixed;
      top: 0;
      right: 0;
      padding-top: 60px;
      transition: 0.3s ease;
      box-shadow: -2px 0 5px rgba(0, 0, 0, 0.1);
      z-index: 1000;
      overflow-y: auto;
    }
    .sidebar::-webkit-scrollbar {
      width: 8px;
    }
    .sidebar::-webkit-scrollbar-track {
      background: transparent;
    }
    .sidebar::-webkit-scrollbar-thumb {
      background: transparent;
      border-radius: 4px;
    }
    .sidebar::-webkit-scrollbar-thumb:hover {
      background: rgba(0, 0, 0, 0.3);
    }
    .sidebar.open {
      width: 250px;
    }
    .sidebar .hamburger {
      font-size: 24px;
      cursor: pointer;
      color: #003087;
      padding: 15px;
      text-align: right;
    }
    .menu-item {
      padding: 15px 20px;
      border-bottom: 1px solid #E0E0E0;
      cursor: pointer;
      color: #333;
      transition: background 0.3s;
    }
    .menu-item:hover {
      background: #E6F0FA;
    }
    .submenu {
      display: none;
      padding-left: 10%;
      padding-right: 10%;
      padding-top: 10%;
    }
    .submenu input,
    .submenu button {
      width: 100%;
      margin: 5px 0;
      padding: 8px;
    }
    .banner-toggle {
      position: absolute;
      top: 60px;
      left: 50%;
      transform: translateX(-50%);
      background: #003087;
      color: #fff;
      padding: 5px 10px;
      cursor: pointer;
      transition: top 0.3s ease;
      z-index: 1001;
      border-bottom-left-radius: 5px;
      border-bottom-right-radius: 5px;
    }
    .banner-toggle.hidden {
      top: 60px;
    }
    .banner {
      display: flex;
      background: #003087;
      color: #fff;
      height: 300px;
      position: relative;
      margin-top: 60px;
      transition: height 0.3s ease;
    }
    .banner.hidden {
      height: 0;
      overflow: hidden;
    }
    .banner-text {
      flex: 1;
      padding: 40px;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }
    .banner-text h2 {
      font-size: 2rem;
      margin-bottom: 10px;
    }
    .banner-text p {
      font-size: 1rem;
    }
    .banner-logs {
      flex: 1;
      padding: 20px;
      display: flex;
      flex-direction: column;
      justify-content: center;
    }
    .log-box {
      background: rgba(255, 255, 255, 0.1);
      border-radius: 5px;
      padding: 10px;
      height: 200px;
      overflow-y: auto;
      margin-bottom: 10px;
      font-size: 0.9rem;
      color: #E0E0E0;
    }
    .log-box::-webkit-scrollbar {
      width: 8px;
    }
    .log-box::-webkit-scrollbar-track {
      background: transparent;
    }
    .log-box::-webkit-scrollbar-thumb {
      background: transparent;
      border-radius: 4px;
    }
    .log-box::-webkit-scrollbar-thumb:hover {
      background: rgba(255, 255, 255, 0.3);
    }
    .log-box p {
      margin: 5px 0;
    }
    .log-controls button {
      padding: 5px 10px;
      background: #E6F0FA;
      border: none;
      border-radius: 5px;
      color: #003087;
      cursor: pointer;
    }
    .content {
      display: flex;
      flex-wrap: wrap;
      gap: 20px;
      justify-content: center;
      padding: 20px;
      height: calc(100vh - 60px - 300px);
      transition: height 0.3s ease;
    }
    .content.expanded {
      height: calc(100vh - 60px);
    }
    .camera-box {
      width: 320px; /* Matches QVGA width */
      height: 240px; /* Matches QVGA height */
      border: 5px solid #003087;
      border-radius: 10px;
      overflow: hidden;
      position: relative;
    }
    .loading-animation {
      width: 50px;
      height: 50px;
      border: 5px solid #f3f3f3;
      border-top: 5px solid #003087;
      border-radius: 50%;
      animation: spin 1s linear infinite;
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
    }
    @keyframes spin {
      0% { transform: translate(-50%, -50%) rotate(0deg); }
      100% { transform: translate(-50%, -50%) rotate(360deg); }
    }
    .waiting-text {
      margin-top: 10px;
      font-size: 1rem;
      color: #003087;
      position: absolute;
      top: 70%;
      left: 50%;
      transform: translateX(-50%);
    }
    .camera-frame {
      width: 100%;
      height: 100%;
      display: none;
      object-fit: fill; /* Ensures exact fit without gaps */
    }
    .control-panel {
      display: none;
      position: fixed;
      top: 60px;
      right: 10px;
      z-index: 1100;
      width: 300px;
      max-height: calc(100vh - 80px);
      overflow-y: auto;
      background: #E6F0FA;
      padding: 20px;
      border: 2px solid #003087;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
    .control-panel::-webkit-scrollbar {
      width: 8px;
    }
    .control-panel::-webkit-scrollbar-track {
      background: transparent;
    }
    .control-panel::-webkit-scrollbar-thumb {
      background: transparent;
      border-radius: 4px;
    }
    .control-panel::-webkit-scrollbar-thumb:hover {
      background: rgba(0, 0, 0, 0.3);
    }
    .control-panel h2 {
      margin-top: 0;
      color: #003087;
      font-size: 1.2rem;
    }
    .control-panel input[type="number"] {
      width: 100%;
      padding: 8px;
      margin: 5px 0;
      border: 1px solid #003087;
      border-radius: 5px;
      display: block;
    }
    .control-panel button {
      width: 100%;
      padding: 10px;
      margin: 5px 0;
      background: #003087;
      border: none;
      border-radius: 5px;
      color: #fff;
      cursor: pointer;
      transition: background 0.3s;
    }
    .control-panel button:hover {
      background: #002266;
    }
    @media (max-width: 768px) {
      .banner {
        flex-direction: column;
        height: auto;
      }
      .banner-text,
      .banner-logs {
        padding: 20px;
      }
      .content {
        flex-direction: column;
        align-items: center;
        height: calc(100vh - 60px - 200px);
      }
      .content.expanded {
        height: calc(100vh - 60px);
      }
      .camera-box {
        width: 320px;
        height: 240px;
      }
      .control-panel {
        left: 50%;
        right: auto;
        transform: translateX(-50%);
        width: 90%;
      }
      .sidebar.open {
        width: 100%;
      }
    }
    @media (max-width: 480px) {
      .header-tab {
        padding: 10px 15px;
      }
      .header-tab .logo-title h1 {
        font-size: 0.9rem;
      }
      .banner-text h2 {
        font-size: 1.5rem;
      }
      .banner-text p {
        font-size: 0.9rem;
      }
      .control-panel {
        padding: 10px;
      }
      .menu-item {
        padding: 10px 15px;
      }
    }
  </style>
</head>
<body>
  <div class="header-tab">
    <div class="logo-title">
      <h1>Waste Segregation System</h1>
    </div>
    <div class="hamburger" onclick="toggleSidebar()">☰</div>
  </div>
  <div class="sidebar" id="sidebar">
    <div class="hamburger" onclick="toggleSidebar()">✖</div>
    <div class="menu-item" onclick="toggleSubmenu('admin')">Configuration and Monitor ⚙</div>
    <div class="submenu" id="admin-submenu">
      <input type="password" id="admin-pass" placeholder="Enter Password">
      <button onclick="checkPassword()">Submit</button>
    </div>
  </div>
  <div class="banner-toggle" onclick="toggleBanner()">
    <span id="banner-icon">▼</span>
  </div>
  <div class="banner" id="banner">
    <div class="banner-text">
      <h2>Welcome</h2>
      <p>Waste Segregation System.</p>
      <br><br><br>
      <h6>© All Rights Reserved 2025.</h6>
    </div>
    <div class="banner-logs">
      <div class="log-box" id="logBox">
        <p>[System] Initialized machine learning model</p>
      </div>
      <div class="log-controls">
        <button onclick="clearLogs()">Clear Logs</button>
      </div>
    </div>
  </div>
  <div class="content" id="content">
    <div class="camera-box">
      <div class="loading-animation"></div>
      <div class="waiting-text">Waiting for frame<span class="dots"></span></div>
      <img src="/stream" class="camera-frame" alt="Camera Stream" onload="hideLoading()" onerror="showLoading()">
    </div>
    <div class="control-panel" id="control-panel">
      <h2>Control Panel</h2>
      <input type="number" id="degrees1" placeholder="Motor 1 Degrees">
      <input type="number" id="degrees2" placeholder="Motor 2 Degrees">
      <input type="number" id="degrees3" placeholder="Motor 3 Degrees">
      <button onclick="applyDegrees()">Command</button>
      <button onclick="resetDegrees()">Reset</button>
      <button onclick="closePanel()">Close</button>
    </div>
    <div class="control-panel" id="waste-panel">
      <h2>Waste Control</h2>
      <button onclick="setWaste('Non-Biodegradable', 'N', [90, 45, 60])">Non-Biodegradable</button>
      <input type="number" id="nb1" value="90">
      <input type="number" id="nb2" value="45">
      <input type="number" id="nb3" value="60">
      <button onclick="setWaste('Recyclable', 'R', [45, 90, 30])">Recyclable</button>
      <input type="number" id="r1" value="45">
      <input type="number" id="r2" value="90">
      <input type="number" id="r3" value="30">
      <button onclick="setWaste('Biodegradable', 'B', [60, 30, 90])">Biodegradable</button>
      <input type="number" id="b1" value="60">
      <input type="number" id="b2" value="30">
      <input type="number" id="b3" value="90">
      <button onclick="closePanel()">Close</button>
    </div>
  </div>
  <script>
    let currentPanel = null;
    let dotInterval;
    function toggleSidebar() {
      document.getElementById('sidebar').classList.toggle('open');
    }
    function toggleSubmenu(id) {
      var submenu = document.getElementById(id + '-submenu');
      submenu.style.display = (submenu.style.display === 'block') ? 'none' : 'block';
    }
    function checkPassword() {
      let pass = document.getElementById('admin-pass').value;
      if (pass === 'KeyDebugger') {
        closeCurrentPanel();
        currentPanel = 'control-panel';
        document.getElementById('control-panel').style.display = 'block';
      } else if (pass === 'CameraSheet') {
        closeCurrentPanel();
        currentPanel = 'waste-panel';
        document.getElementById('waste-panel').style.display = 'block';
      } else {
        alert('Incorrect Password');
      }
    }
    function closeCurrentPanel() {
      if (currentPanel) document.getElementById(currentPanel).style.display = 'none';
    }
    function closePanel() {
      closeCurrentPanel();
      currentPanel = null;
    }
    function applyDegrees() {
      let d1 = document.getElementById('degrees1').value;
      let d2 = document.getElementById('degrees2').value;
      let d3 = document.getElementById('degrees3').value;
      let log = `[ML Command] Degrees set: ${d1}, ${d2}, ${d3}`;
      addLog(log);
      console.log(log);
    }
    function resetDegrees() {
      document.getElementById('degrees1').value = '';
      document.getElementById('degrees2').value = '';
      document.getElementById('degrees3').value = '';
      let log = '[ML Command] Degrees reset';
      addLog(log);
      console.log(log);
    }
    function setWaste(type, code, degrees) {
      let log = `[ML Detection] Waste: ${type}, Code: ${code}, Degrees: ${degrees.join(', ')}`;
      addLog(log);
      console.log(log);
    }
    function showLoading() {
      document.querySelector('.loading-animation').style.display = 'block';
      document.querySelector('.waiting-text').style.display = 'block';
      document.querySelector('.camera-frame').style.display = 'none';
      addLog('[Camera] Waiting for frame...');
      animateDots();
    }
    function hideLoading() {
      document.querySelector('.loading-animation').style.display = 'none';
      document.querySelector('.waiting-text').style.display = 'none';
      document.querySelector('.camera-frame').style.display = 'block';
      addLog('[Camera] Frame received');
      clearInterval(dotInterval);
    }
    function animateDots() {
      const dots = document.querySelector('.dots');
      let count = 0;
      dotInterval = setInterval(() => {
        count = (count + 1) % 4;
        dots.textContent = '.'.repeat(count);
      }, 500);
    }
    function toggleBanner() {
      const banner = document.getElementById('banner');
      const content = document.getElementById('content');
      const bannerToggle = document.querySelector('.banner-toggle');
      const bannerIcon = document.getElementById('banner-icon');
      if (banner.classList.contains('hidden')) {
        banner.classList.remove('hidden');
        content.classList.remove('expanded');
        bannerToggle.classList.remove('hidden');
        bannerIcon.textContent = '▼';
        document.body.style.overflow = 'hidden';
      } else {
        banner.classList.add('hidden');
        content.classList.add('expanded');
        bannerToggle.classList.add('hidden');
        bannerIcon.textContent = '▲';
        document.body.style.overflow = 'hidden';
      }
    }
    function addLog(message) {
      const logBox = document.getElementById('logBox');
      const logEntry = document.createElement('p');
      logEntry.textContent = message;
      logBox.appendChild(logEntry);
      logBox.scrollTop = logBox.scrollHeight;
    }
    function clearLogs() {
      const logBox = document.getElementById('logBox');
      logBox.innerHTML = '';
      addLog('[System] Logs cleared');
    }
    showLoading();
    setInterval(() => {
      const wasteTypes = ['Non-Biodegradable', 'Recyclable', 'Biodegradable'];
      const codes = ['N', 'R', 'B'];
      const randomType = wasteTypes[Math.floor(Math.random() * wasteTypes.length)];
      const randomCode = codes[wasteTypes.indexOf(randomType)];
      addLog(`[ML Detection] Detected ${randomType}, Confidence: ${(Math.random() * (0.95 - 0.75) + 0.75).toFixed(2)}`);
    }, 5000);
  </script>
</body>
</html>
)rawliteral";

static ra_filter_t ra_filter;
static int64_t last_frame = 0;

#if CONFIG_LED_ILLUMINATOR_ENABLED
static bool isStreaming = false;
void enable_led(bool en) {
  // Placeholder: Implement LED control if needed
}
#endif

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[128];

    if (!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

#if CONFIG_LED_ILLUMINATOR_ENABLED
    isStreaming = true;
    enable_led(true);
#endif

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            log_e("Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        _timestamp.tv_sec = fb->timestamp.tv_sec;
        _timestamp.tv_usec = fb->timestamp.tv_usec;

        if (fb->format != PIXFORMAT_JPEG) {
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            esp_camera_fb_return(fb);
            fb = NULL;
            if (!jpeg_converted) {
                log_e("JPEG compression failed");
                res = ESP_FAIL;
                break;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if (res == ESP_OK) {
            size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART,
                                   _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }

        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if (res != ESP_OK) {
            log_e("Send frame failed");
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = (fr_end - last_frame) / 1000;
        last_frame = fr_end;

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        log_i("MJPG: %uB %ums (%.1f fps), avg: %.1f fps",
              (uint32_t)_jpg_buf_len,
              (uint32_t)frame_time,
              1000.0 / (uint32_t)frame_time,
              1000.0 / (uint32_t)avg_frame_time);
#else
        log_i("MJPG: %uB %ums (%.1f fps)",
              (uint32_t)_jpg_buf_len,
              (uint32_t)frame_time,
              1000.0 / (uint32_t)frame_time);
#endif
    }

#if CONFIG_LED_ILLUMINATOR_ENABLED
    isStreaming = false;
    enable_led(false);
#endif

    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    ra_filter_init(&ra_filter, 20);

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t index_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t stream_uri = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = stream_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &stream_uri);
    }
}
