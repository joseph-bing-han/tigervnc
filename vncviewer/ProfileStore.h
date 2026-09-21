/* Copyright (C) 2026 TigerVNC contributors. */

#ifndef __PROFILE_STORE_H__
#define __PROFILE_STORE_H__

#include <string>
#include <vector>

/*
 * 管理 macOS vncviewer 的服务器配置文件和密码文件。
 * 配置内容本身仍由 parameters.cxx 使用 TigerVNC 格式读写。
 */
class ProfileStore
{
public:
  // 空目录参数使用 $HOME/TigerVNC，测试和调用方可以传入指定目录。
  explicit ProfileStore(const std::string& directory = std::string());

  const std::string& directory() const { return profileDirectory; }

  void ensureDirectory() const;

  std::vector<std::string> listProfiles() const;

  bool configExists(const std::string& name) const;

  std::string configPath(const std::string& name) const;
  std::string passwordPath(const std::string& name) const;

  // 配置文件使用临时文件写入后原子替换。
  void writeConfig(const std::string& name, const std::string& contents) const;
  bool readConfig(const std::string& name, std::string* contents) const;

  // 密码文件保存 TigerVNC 现有的 8 字节可逆混淆格式。
  void writePassword(const std::string& name, const std::string& password) const;
  bool readPassword(const std::string& name, std::string* password) const;

  void removeProfile(const std::string& name) const;
  void renameProfile(const std::string& oldName,
                     const std::string& newName) const;

  static bool isValidName(const std::string& name);

private:
  std::string profileDirectory;

  std::string makePath(const std::string& name,
                       const char* suffix) const;
  bool nameConflicts(const std::string& name,
                     const std::string* ignoredName = nullptr) const;
  static bool isRegularFile(const std::string& path);
  static void atomicWrite(const std::string& path,
                          const std::string& contents,
                          unsigned mode);
};

#endif
