# VS Studio Cmake

### 构建

我只提交了源文件，需要使用VS重新构建cmake项目

- 使用VS Studio建立cmake项目，项目名改为 `src` ，解决方案名自定义即可；建立好后把 `src` 里面的东西删除即可！
- 在建立的项目的根目录使用 `git clone` 把master分支拉去下来，src会自动覆盖；
- CmakeSetting.json因为每个人的主机配置不同，需要单独设置；
  - 使用VS打开项目后，在 `项目` 中的最下面一栏cmake配置修改：
    - 主要是更改工具集，改为clang_cl_x64
    - 其余保持默认就好
- 在src目录下的cmakelists.txt文件页面使用 `Ctrl + S` ，VS会自动构建Cmake，如果出错，联系我即可。
- 在启动项选择 `compiler` 即可正常运行。



运行结果，正常来说VS会把out保存在与src同级的out里面，在最里面有一个src的目录，可以看到生成的exe和相关文件，在这里新建 `testexapmle.txt` 文件即可！

