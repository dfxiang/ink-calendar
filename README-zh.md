# ESP32-C3 四色墨水屏电子台历

将 Good Display GDEM042F86 4.2 寸四色(黑白红黄)墨水屏改装成桌面电子台历:

- 顶部:公历日期 + 星期 + 天气,农历(干支纪年)+ 湿度 / 最高最低温
- 中部:当月台历,周日开头,每格同时显示公历数字和农历
- 周一至周五的日期用红色字体,周六、周日用黑色字体
- 今天用红色方框标出(带浅黄底色)
- 公历/农历节日自动显示(春节、端午、中秋、国庆、除夕等)
- 不显示时分秒,每 60 秒检测日期与天气变化,按需刷新墨水屏

## 硬件

同原项目:AirM2M CORE ESP32C3 + GDEM042F86 4.2 寸 400x300 四色墨水屏,SPI 接口,
接线不变:

| 墨水屏 | ESP32-C3 GPIO |
| --- | ---: |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SCK` | `GPIO4` |
| `SDA` | `GPIO6` |
| `RST` | `GPIO2` |
| `DC` | `GPIO3` |
| `CS1` | `GPIO7` |
| `BUSY` | `GPIO10` |

## 手机配置 WiFi(无需重刷固件)

设备通过"配置门户"接收 WiFi 设置,保存到 ESP32 的 NVS 闪存,以后换路由器
只需要重新配置,不需要重新烧录固件。

进入配置模式有三种方式:

1. **首次上电 / 未保存任何配置**:开机直接进入配置模式;
2. **WiFi 连不上**:尝试连接 60 秒失败后自动进入配置模式;
3. **RST 按键双击**:开机后 3 秒内快速按两次 RST 按键,清除已保存的
   配置并进入配置模式(相当于"恢复出厂设置")。

配置模式下墨水屏会显示操作提示,手机操作步骤:

1. 手机连接热点 `InkScreen-Calendar`(无密码);
2. 手机浏览器会自动弹出配置页(如果没有,手动打开 `http://192.168.4.1`);
3. 选择或输入 WiFi 名称 / 密码,可顺便修改天气位置的经纬度(默认上海);
4. 点"保存并重启",设备自动连接新 WiFi 并开始显示台历。

页面里也有"清除配置并重启"按钮,效果同 RST 双击。

配置保存在 NVS(命名空间 `cfg`),天气经纬度也可以从手机端修改,不再需要
改代码。`src/config.h` 里的 `WEATHER_LATITUDE / WEATHER_LONGITUDE` 仅作为
未保存配置时的默认值。

> 说明:配置方式用的是 ESP32 自带的 WiFi 模块(SoftAP + 网页),任何手机
> 都能配,不需要装 App。蓝牙(BLE)配置需要在手机安装乐鑫的配网 App,如需
> 也可以再加。

### 可选:外接按键进入配置模式

把 [src/config.h](src/config.h) 里的 `CONFIG_BUTTON_PIN` 改成任意空闲 GPIO
(默认 -1 表示不使用),把按键一端接该引脚、另一端接 GND,开机后长按 3 秒即
可进入配置模式。

> 注意:不要用板上 BOOT 键(GPIO9)长按跨过复位,否则芯片会进入下载模式;
> 开机后(固件已运行)再长按则没有问题。

## 编译与烧录

```powershell
pio run
pio run --target upload
```

烧录后接上串口可看日志(`pio device monitor -b 115200`):

```
ESP32-C3 电子台历 starting...
[portal] AP SSID: InkScreen-Calendar      <- 首次开机,等待手机配置
[portal] AP IP: 192.168.4.1
...(保存配置并重启后)...
[wifi] connecting to MyHomeWiFi
[weather] code=2 34C hum=44% max=36 min=27
[panel] refresh done
```

## 刷新策略

- 每 60 秒检测一次:日期(公历/农历/星期)变了 → 整屏刷新;
- 每 60 秒拉取一次天气:天气描述、温度、湿度或高低温度变化了 → 快速刷新
  (该面板不支持真正局部刷新,驱动里的 fast 模式用于减轻闪烁);
- 天气变化触发刷新的最小间隔为 2 分钟(可在 config.h 调
  `WEATHER_MIN_REFRESH_SEC`),避免墨水屏频繁刷新、缩短寿命;
- 每次刷新后面板自动进入休眠,耗电很低。

## 故障处理

- **忘记 WiFi 密码 / 换路由器**:RST 快速按两次,重新配网;
- **配置后连不上**:等 60 秒会自动回到配置模式,重新检查密码;
- **屏幕停在配置画面**:说明当前处于配置模式,按上面步骤配网即可。
- **扫描不到家里的 WiFi**:ESP32-C3 只支持 2.4GHz,若路由器 5GHz 单独开了
  一个名称会看不到,可在路由器开启"双频合一"或改用 2.4GHz 的网络名;
- **手机连不上配置热点**:部分手机会自动切回原来的 WiFi,请到系统 WiFi
  设置里手动选择 `InkScreen-Calendar`。

## 天气与字体

- 天气接口:`api.open-meteo.com`(HTTP),仅需城市经纬度;
- 中文字体:由 [tools/gen_font.py](tools/gen_font.py) 用 Windows 自带
  SimHei 生成 16x16 / 12x12 点阵字库,已提交到 [src/font_data.h](src/font_data.h)。
  如需重新生成:`python tools/gen_font.py`;
- 农历转换:查表法,覆盖 1900-2100 年,已与权威农历库全区间比对(73384 天 0 差异),
  代码在 [src/lunar.cpp](src/lunar.cpp)。

## 开发辅助

- 布局预览:`python tools/preview.py 2026-08-03` 生成台历预览;
  再加一个 `config` 参数(`python tools/preview.py 2026-08-03 preview.png config`)
  可生成配置模式的提示画面;
- 农历校验:`python tools/lunar_check.py`。

## 文件结构

```
src/config.h        用户配置(SSID / 坐标 / 刷新间隔)
src/main.cpp        主循环:WiFi、NTP、轮询、刷屏
src/settings.cpp    配置存储(NVS):WiFi / 天气坐标
src/portal.cpp      配置门户:SoftAP + 网页配网 + 扫描 WiFi
src/portal_page.h   配置网页
src/render.cpp      台历版面绘制
src/frame.cpp       400x300 2bit 帧缓冲 + 点阵文本绘制
src/font.h/data.h   16x16 / 12x12 中文字库
src/lunar.cpp       公历转农历 + 节日
src/weather.cpp     Open-Meteo 天气获取与解析
src/gdem042f86.*    墨水屏驱动(原样保留)
tools/              字体生成 / 预览 / 农历校验脚本
```
