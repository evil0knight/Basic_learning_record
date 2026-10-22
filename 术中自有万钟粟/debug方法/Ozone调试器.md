# Ozone 调试器

[← 返回 配置学习](../配置学习/MOC.md) | [← 返回 debug方法](./MOC.md) | [← 主页](../../index.md)

---

**下载地址：** [https://www.segger.com/downloads/jlink/#Ozone](https://www.segger.com/downloads/jlink/#Ozone)

### 配置:

1. ![1785383872546](image/Ozone调试器/1785383872546.png)
2. ![1785384396679](image/Ozone调试器/1785384396679.png)
3. ![1785385016917](image/Ozone调试器/1785385016917.png)

### 使用:

1. ![1785391224443](image/Ozone调试器/1785391224443.png)
2. ![1785391844859](image/Ozone调试器/1785391844859.png) 这个是找文件,输入文件名
3. ![1785392222175](image/Ozone调试器/1785392222175.png) 右键+watch查看变量![1785392630257](image/Ozone调试器/1785392630257.png)
4. ![1785392740954](image/Ozone调试器/1785392740954.png)右键graph可以查看到变量的最大最小值
5. ![1785392825102](image/Ozone调试器/1785392825102.png)![1785393078240](image/Ozone调试器/1785393078240.png)
6. call stack,可以查看断点处函数的调用![1785395137872](image/Ozone调试器/1785395137872.png)![1785395145380](image/Ozone调试器/1785395145380.png)
7. ![1785396087223](image/Ozone调试器/1785396087223.png)![1785396120262](image/Ozone调试器/1785396120262.png)
8. 保存线程快照![1785418203441](image/Ozone调试器/1785418203441.png)
9. 不对,下面这些是找不到具体寄存器才要的操作,我ozone好像出问题了
   以文本编辑器的方式打开.jedebug，添加pack包的SVD文件，如图
   在view-register里可以查看具体寄存器
   注意name和vlaue是可以拉开的对比2图和3图
   ![1792564449666](image/Ozone调试器/1792564449666.png)
   ![2图](image/Ozone调试器/1792564813781.png)![1792565284189](image/Ozone调试器/1792565284189.png)
