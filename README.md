# MiniHdiffPatch
具备原地升级的HDiffPatch精简版本，Visual Studio 项目
HDiff项目用于生成差分文件
HPatch项目用于合成测试
项目开发环境：
Microsoft Visual Studio Community 2022 (64 位) - Preview
版本 17.8.0 Preview 1.0

差分文件生成：
设置HDiff为启动项目，项目属性中的调试参数设置为 旧文件路径 新文件路径 待生成差分文件路径
如：D:\CODE\minihdiff\old2.bin D:\CODE\minihdiff\new2.bin D:\CODE\minihdiff\patch2.bin
差分文件测试：
设置HPatch为启动项目，项目属性中的调试参数设置为 旧文件路径 差分文件路径 待生成新文件路径
如：D:\CODE\minihdiff\old2.bin D:\CODE\minihdiff\patch2.bin D:\CODE\minihdiff\result_new2.bin
