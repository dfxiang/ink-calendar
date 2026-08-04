Arduino GxEPD2y库示例 支持ESP32 或 ESP8266

1、需要先安装Arduino IDE，官方下载地址：https://www.arduino.cc/en/software

2、在arduino库管理中安装搜索 GxEPD2库 作者：ZinggJM

3、主控建议使用ESP系列，比如 ESP32、ESP8266、ESP32C3、ESP32S3，当然其他的也是可以，如果内存很小，哪会非常麻烦，因为全刷数据是需要一次性构建好再发送出去的，虽然也可以实时生成，但就考验编程能力了。
安装方便：在开发板管理中需要搜索 ESP32 或 ESP8266（这个安装可以参考：https://blog.csdn.net/mucherry/article/details/134296259，也可以百度一下，教程比较多）

4、根据自己的型号，修改 ink_test.ino 文件中驱动部分，对应引脚已经在注释中标明
注意：SCK SDA 引脚为Arduino 默认，正常情况下不要修改，其他4个引脚为普通GPIO口，可自动修改。
修改方法：
1）先找到自己板子相关的配置，比如是ESP32C3，我们就看 ESP32C3主控接法：
// ESP32C3主控接法： 
//    SS(CS)=7 MOSI(SDA)=6  MISO(不接)=5 SCK=4 DC=3 RST=2 BUSY=10
// 2.9寸屏
// GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> display(GxEPD2_290_T94_V2(/*CS=5*/ 7, /*DC=*/ 3, /*RST=*/ 2, /*BUSY=*/ 10));
// 2.13寸屏
 GxEPD2_BW<GxEPD2_213_B72, GxEPD2_213_B72::HEIGHT> display(GxEPD2_213_B72(/*CS=5*/ 7, /*DC=*/ 3, /*RST=*/ 2, /*BUSY=*/ 10));
// 1.54寸屏
// GxEPD2_BW<GxEPD2_154_T8, GxEPD2_154_T8::HEIGHT> display(GxEPD2_154_T8(/*CS=5*/ 7, /*DC=*/ 3, /*RST=*/ 2, /*BUSY=*/ 10));

2)注意此行内容：
GxEPD2_BW<GxEPD2_213_B72, GxEPD2_213_B72::HEIGHT> display(GxEPD2_213_B72(/*CS=5*/ 7, /*DC=*/ 3, /*RST=*/ 2, /*BUSY=*/ 10));

此行为初始化代码，其他不用的需要在行首使用 两个反斜杠注释，只保留一项即可以。

其中：GxEPD2_213_B72，代表的就是墨水屏驱动类型，213 指的是2.13寸，B72 指的是型号。

如果不知道型号，哪就只能一个个试了，如果还不行，哪可能GxEPD2库中没有你的屏的型号，需要自己想办法了，这里无法保证所有屏都能驱动起来，一般屏没反映，就是因为这个，驱动程序不支持。


