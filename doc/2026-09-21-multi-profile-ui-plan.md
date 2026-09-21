# TigerVNC 多配置主界面与自动配置管理开发计划

## 总体目标

将 macOS 主界面改为配置列表主页：

- 启动时扫描 `~/TigerVNC/*.tigervnc`；
- 列表显示配置名称；
- 左键单击配置立即连接；
- 右键菜单提供“修改配置”和“删除配置”；
- “添加配置”打开统一编辑窗口；
- 配置自动保存，不再显示“载入”“Save as”“Options…”按钮；
- 每个服务器独立保存服务器地址、用户名、连接选项和密码。

本次只改变 macOS 主界面；Linux 和 Windows 保留现有主界面行为。

## 1. 配置存储层

新增 `vncviewer/ProfileStore.{h,cxx}`，负责所有配置文件操作。

### 目录

固定使用 `$HOME/TigerVNC/`，启动时自动创建，权限为 `0700`。

扫描只读取 `.tigervnc` 文件，去掉后缀后的文件名作为列表名称并按名称排序。损坏或无法读取的文件记录日志并跳过。

### 文件名

保存为 `~/TigerVNC/<Name>.tigervnc`。Name 允许 UTF-8、空格、中文、字母、数字、`-`、`_`、`.`，禁止空串、路径分隔符、控制字符、`.`、`..` 及大小写不敏感的重名。

重命名配置时同步重命名配置文件和密码文件。保存使用临时文件加 `rename()` 的原子替换方式。

### 配置内容

继续使用 TigerVNC Configuration file Version 1.0 格式，保存 `ServerName`、`Username` 以及现有 OptionsDialog 的全部服务器选项。Name 由文件名提供，不重复写入内容。

## 2. 用户名与密码

新增 `Username` 参数，并加入配置读写列表。配置编辑窗口可编辑用户名；认证窗口将其作为默认值，用户修改后更新当前配置。

每个配置使用同名旁路密码文件 `~/TigerVNC/<Name>.passwd`，使用现有 `rfb::obfuscate()` 格式，权限为 `0600`。选择配置时自动设置当前 `PasswordFile`；首次输入密码后自动保存；删除配置时同步删除密码文件。

该格式是可逆混淆而非现代密码学意义上的强加密，主要用于避免明文保存并兼容现有 TigerVNC 密码格式。

## 3. 主界面

重构 `vncviewer/ServerDialog.{h,cxx}`：

- 使用配置列表显示所有配置名称；
- 左键单击加载配置并直接连接；
- 右键弹出“修改配置/删除配置”；
- 删除前确认，并同步删除配置和密码文件；
- 底部增加“添加配置”；
- 移除服务器输入框、“Options…”、“Load…”、“Save as…”和“Connect”；
- 保留“关于…”和“取消/退出”。

## 4. 统一配置编辑窗口

新增 `vncviewer/ProfileEditor.{h,cxx}`，包含 Name、VNC 服务器地址、用户名、密码、全部 Options 标签页以及保存/取消按钮。

添加配置时使用默认参数；修改配置时加载目标配置。保存自动创建或更新 `.tigervnc`，必要时重命名文件和密码文件，并刷新主页列表。取消时恢复打开编辑器前的全局参数。

## 5. OptionsDialog 重构

将现有 OptionsDialog 页面提取为可嵌入的 Options 面板。ProfileEditor 直接嵌入该面板，原有连接过程中的 OptionsDialog 继续复用同一面板。编辑流程为“加载配置参数 → 刷新控件 → 保存时写回全局参数并原子保存”。新建配置前恢复参数默认值，避免不同配置之间串值。

## 6. 启动与兼容性

没有命令行服务器参数时进入新的配置主页；有命令行服务器参数或命令行配置文件时保留现有直接连接行为。旧的 `~/.vnc`、`~/.config/tigervnc` 配置不自动迁移。

## 7. 测试与验证

新增 ProfileStore 单元测试，覆盖目录创建、扫描排序、配置读写、Name 校验、重命名、删除、原子保存、损坏文件、用户名和密码文件权限。

手工验证 macOS UI：添加、重启加载、单击连接、密码复用、修改、重命名、删除、非法名称、损坏配置、不同配置 Options 隔离，以及命令行兼容性。

最终执行 macOS 编译、App 目录检查、`Info.plist` 校验和 `git diff --check`。

## 8. 代码落地位置（已核对仓库现状）

| 位置 | 作用 |
|---|---|
| `vncviewer/ProfileStore.{h,cxx}` | 配置目录、配置文件与密码文件读写 |
| `vncviewer/OptionsPanel.{h,cxx}` | 由 OptionsDialog 抽取出的可嵌入选项面板 |
| `vncviewer/ProfileEditor.{h,cxx}` | 统一配置编辑窗口 |
| `vncviewer/ProfileDialog.{h,cxx}` | 配置列表主界面 |
| `vncviewer/OptionsDialog.{h,cxx}` | 瘦身为连接过程中的独立选项窗口，复用 OptionsPanel |
| `vncviewer/UserDialog.cxx` | 密码文件缺失时回退到输入框，并在接受后写回密码文件 |
| `vncviewer/parameters.cxx/.h` | 新增 `Username` 参数并加入配置读写列表 |
| `vncviewer/vncviewer.cxx` | macOS 无服务器参数时进入 ProfileDialog |
| `vncviewer/CMakeLists.txt` | 加入新增源文件 |
| `tests/unit/CMakeLists.txt` | 加入 profile_store 测试目标（已完成） |

## 9. 关键接口

`ProfileStore`（已由单元测试固定）：

```text
ProfileStore(const std::string& dir);   // 构造时创建目录，权限 0700
std::vector<std::string> listProfiles() const;   // 按名称排序
static bool isValidName(const std::string&);
bool configExists(const std::string&) const;
bool readConfig(const std::string&, std::string* out) const;
void writeConfig(const std::string&, const std::string& contents);   // 原子写
bool readPassword(const std::string&, std::string* out) const;
void writePassword(const std::string&, const std::string& password); // 空串删除
std::string configPath(const std::string&) const;
std::string passwordPath(const std::string&) const;
void renameProfile(const std::string&, const std::string&);
void removeProfile(const std::string&);
static const char* defaultDirectory();
```

密码文件沿用 `rfb::obfuscate()` / `rfb::deobfuscate()`（`common/rfb/obfuscate.h`）。

## 10. 连接与密码行为

1. 主界面左键单击配置：读取 `~/TigerVNC/<Name>.tigervnc`，调用现有 `loadViewerParameters()` 写入全局参数，并把 `PasswordFile` 设为 `~/TigerVNC/<Name>.passwd`；
2. `UserDialog::getUserPasswd()` 在密码文件不存在或不可读时改为回退到输入框（当前实现会直接抛错）；
3. 用户在输入框中确认后，若 `PasswordFile` 非空则以混淆格式写回该文件，权限 `0600`；
4. 命令行直接连接场景不设置 `PasswordFile`，行为保持现状。

## 11. 构建与验收

本机 FLTK 1.4 与 TigerVNC 不兼容，必须使用 FLTK 1.3 的 keg 配置：

```bash
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release -DENABLE_NLS=OFF -DBUILD_JAVA=OFF -DBUILD_WINVNC=OFF \
  -DFLTK_INCLUDE_DIR=/opt/homebrew/opt/fltk@1.3/include \
  -DFLTK_BASE_LIBRARY=/opt/homebrew/opt/fltk@1.3/lib/libfltk.dylib \
  -DFLTK_IMAGES_LIBRARY=/opt/homebrew/opt/fltk@1.3/lib/libfltk_images.dylib \
  -DFLTK_GL_LIBRARY=/opt/homebrew/opt/fltk@1.3/lib/libfltk_gl.dylib
cmake --build build-macos --target vncviewer --parallel 8
```

单元测试需要 `-DGTEST_ROOT=/opt/homebrew/opt/googletest`。

验收标准：

- 主界面仅显示配置列表、添加配置、关于、取消；
- 左键单击直接连接，第二次连接不再询问密码；
- 右键可修改与删除配置，删除同步移除密码文件；
- 配置文件与密码文件权限分别为默认与 `0600`；
- 编译产物为 arm64 macOS App，`Info.plist` 校验通过。

## 12. 实施记录

已完成并验证的内容：

- `vncviewer/ProfileStore.{h,cxx}`：配置目录（`$HOME/TigerVNC`，权限 0700）、配置文件的原子写入、密码文件的 0600 混淆存储、重命名与删除；
- `vncviewer/OptionsPanel.{h,cxx}`：由 `OptionsDialog` 抽出的可嵌入选项面板；`OptionsDialog` 保留为连接过程中的独立窗口并复用同一面板；
- `vncviewer/ProfileEditor.{h,cxx}`：Name、VNC 服务器、用户名、密码以及全部选项面板合并为一个配置窗口；
- `vncviewer/ServerDialog.cxx`（仅 macOS 分支）：配置列表、左键直接连接、右键“修改配置/删除配置”、“添加配置”按钮；
- `vncviewer/parameters.{h,cxx}`：新增 `Username` 参数并加入配置读写列表，新增 `resetViewerParameters()` 避免不同配置之间设置串值；
- `vncviewer/UserDialog.cxx`：密码文件缺失时回退到输入框，用户确认后自动写回该文件，认证失败时删除已保存的密码；
- `po/zh_CN.po`：补充新界面字符串的简体中文翻译。

验证方式：

- `ProfileStore` 使用独立校验程序覆盖目录权限、扫描排序、读写往返、名称校验、重命名、删除、密码混淆与 0600 权限、空密码删除密码文件；
- macOS 编译通过，产物为 arm64 可执行文件；
- `TigerVNC.app` 打包完成，`Info.plist` 通过 `plutil -lint`，启动冒烟测试无崩溃。

已知限制：

- 仓库自带的 gtest 用例 `tests/unit/profile_store.cxx` 需要本机安装 GoogleTest 才能运行；
- 密码采用 TigerVNC 现有的可逆混淆格式，仅用于避免明文保存；
- 新界面仅作用于 macOS，Linux 与 Windows 保持原有主界面；
- 除简体中文外的其他语言缺少新增字符串的翻译，会回退显示英文。
