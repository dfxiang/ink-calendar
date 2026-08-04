# 第一次bug修复, 标点变成黑色方块的问题和天气行重影加模糊

两个问题都修好了，固件编译通过，预览图已重新生成。

  问题根源都在字体生成工具 tools/gen_font.py 里，是两个独立 bug：

  1. 标点变成黑方块（preview_config.png）
  render_glyph() 会把每个字符的墨迹包围盒裁剪后强行放大填满整个 16×16 单元格。句号原始墨迹只有 5×4 像素，放大后就是实心方块；° 也被放大成占满单元格的大圆环，所以温度显示成了 “34OC”。

  2. 天气行重影模糊（preview.png）
  12px 字体每行 12 位数据拆两个字节时，低字节用了 row & 0xFF，导致 bit 7..4 在高低字节里重复。预览和固件按 16 位还原时，每个 12px 字形都被水平拉伸、笔画重叠——湿度/高低温那行正好用的
  12px 字体，看起来就是重影加模糊。

  修复内容（esp32c3-inkscreen-calendar/tools/gen_font.py）：

  - 按字符类别缩放：汉字/全角保持占格 90%，ASCII 约 72%，句号/逗号约 28%，°/℃/冒号约 52%，不再无脑填满整格。
  - 增加基线定位：句号、冒号靠底部，° 按上标位置放在行顶，排版正常。
  - 12px 改用 8 倍渲染源（96px），小符号的细笔画缩小时不再断裂。
  - 修正 pack_header() 对 12 位行数据的打包，消除 bit 重叠。

  重新生成了 esp32c3-inkscreen-calendar/src/font_data.h，并更新两张预览图 esp32c3-inkscreen-calendar/preview.png 和 esp32c3-inkscreen-calendar/preview_config.png。

  验证结果：

  - IP 里的 . 和 “1.” “2.” 后的点是底部 3~4px 的小圆点，不再是方块；
  - 34°C 的 ° 是小圆环、靠行顶，不再像大写 O；
  - 12px 的湿度/高低温行笔画清晰单线，重影消失；
  - 模拟固件逐字节读取方式与预览渲染逐像素一致；
  - pio run 编译通过（RAM 21.6%，Flash 75.6%）。

  以后想重新调字形，改完 gen_font.py 后运行 python tools/gen_font.py 再 python tools/preview.py 即可。