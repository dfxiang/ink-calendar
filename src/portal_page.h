#pragma once

// WiFi 配置网页。@@SSID@@ / @@LAT@@ / @@LON@@ / @@PASS@@ 由 portal.cpp 替换。
// 注意:这是 C 字符串字面量,行尾不需要 \n;请勿把中文放进注释之外的单引号里。
static const char PORTAL_HTML[] = R"HTML(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>电子台历 WiFi 配置</title>
<style>
  body { font-family: -apple-system, "Microsoft YaHei", sans-serif; background:#f3f4f6;
         margin:0; padding:16px; }
  .card { background:#fff; max-width:420px; margin:24px auto; padding:20px 22px;
          border-radius:12px; box-shadow:0 2px 8px rgba(0,0,0,.12); }
  h1 { font-size:20px; margin:0 0 6px; color:#222; }
  p.tip { color:#666; font-size:13px; margin:0 0 16px; }
  label { display:block; font-size:13px; color:#444; margin:12px 0 4px; }
  input, select { width:100%; box-sizing:border-box; padding:10px; font-size:15px;
                  border:1px solid #ccc; border-radius:8px; background:#fff; }
  input:focus, select:focus { outline:2px solid #4a90d9; border-color:#4a90d9; }
  .row { display:flex; gap:8px; }
  .row select { flex:1; }
  .row button { flex:0 0 auto; padding:10px 12px; border:1px solid #ccc; background:#fff;
                border-radius:8px; font-size:13px; }
  button.save { width:100%; margin-top:18px; padding:12px; font-size:16px; color:#fff;
                background:#2f7bff; border:0; border-radius:8px; }
  button.danger { width:100%; margin-top:10px; padding:10px; font-size:13px; color:#c0392b;
                  background:#fff; border:1px solid #e0b4b0; border-radius:8px; }
  #msg { display:none; margin-top:12px; padding:10px; border-radius:8px; font-size:14px;
         background:#e8f5e9; color:#1b5e20; text-align:center; }
</style>
</head>
<body>
<div class="card">
  <h1>电子台历 · WiFi 配置</h1>
  <p class="tip">保存后设备会自动重启并连接新 WiFi。</p>
  <form id="cfg" method="post" action="/save">
    <label>WiFi 名称(SSID)</label>
    <div class="row">
      <select id="scanlist" onchange="pick()">
        <option value="">— 选择或手动输入 —</option>
      </select>
      <button type="button" onclick="scan()">扫描</button>
    </div>
    <label>SSID</label>
    <input type="text" name="ssid" id="ssid" value="@@SSID@@" required
           maxlength="32" placeholder="WiFi 名称">
    <label>密码</label>
    <input type="password" name="pass" id="pass" value="@@PASS@@"
           maxlength="63" placeholder="WiFi 密码(无密码可留空)">
    <label>天气位置 · 纬度(可在 open-meteo.com 或地图查询)</label>
    <input type="number" name="lat" id="lat" step="0.0001" value="@@LAT@@" required>
    <label>天气位置 · 经度</label>
    <input type="number" name="lon" id="lon" step="0.0001" value="@@LON@@" required>
    <button class="save" type="submit">保存并重启</button>
    <div id="msg">已保存,设备正在重启…</div>
  </form>
  <form method="post" action="/clear" onsubmit="return confirm('确定清除配置并重启?');">
    <button class="danger" type="submit">清除配置并重启</button>
  </form>
</div>
<script>
  function esc(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
            .replace(/"/g,'&quot;').replace(/'/g,'&#39;');
  }
  function pick() {
    var sel = document.getElementById('scanlist');
    if (sel.value) { document.getElementById('ssid').value = sel.value; }
  }
  function scan() {
    fetch('/scan').then(function(r){ return r.json(); }).then(function(j){
      var sel = document.getElementById('scanlist');
      sel.innerHTML = '<option value="">— 选择或手动输入 —</option>';
      (j.networks || []).forEach(function(n){
        var o = document.createElement('option');
        o.value = n.ssid;
        o.textContent = n.ssid + ' (' + n.rssi + 'dBm)';
        sel.appendChild(o);
      });
    }).catch(function(){});
  }
  document.getElementById('cfg').addEventListener('submit', function(){
    document.getElementById('msg').style.display = 'block';
  });
  scan();
</script>
</body>
</html>
)HTML";
