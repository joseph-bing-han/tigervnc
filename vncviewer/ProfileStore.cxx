/* Copyright (C) 2026 TigerVNC contributors. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ProfileStore.h"

#include <core/xdgdirs.h>
#include <rfb/obfuscate.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <direct.h>
#include <sys/stat.h>
#define mkdir(path, mode) _mkdir(path)
#endif

namespace {

const char* CONFIG_SUFFIX = ".tigervnc";
const char* PASSWORD_SUFFIX = ".passwd";
const char* CONFIG_HEADER = "TigerVNC Configuration file Version 1.0";

std::string joinPath(const std::string& directory, const std::string& name)
{
  if (directory.empty())
    return name;
  if (directory[directory.size() - 1] == '/')
    return directory + name;
  return directory + "/" + name;
}

std::string lowerAscii(const std::string& value)
{
  std::string result(value);
  for (size_t i = 0; i < result.size(); i++)
    result[i] = static_cast<char>(std::tolower(
      static_cast<unsigned char>(result[i])));
  return result;
}

bool sameName(const std::string& left, const std::string& right)
{
  return lowerAscii(left) == lowerAscii(right);
}

std::runtime_error fileError(const char* operation,
                             const std::string& path)
{
  return std::runtime_error(std::string(operation) + " \"" + path +
                            "\": " + std::strerror(errno));
}

void removeIfPresent(const std::string& path)
{
  if (std::remove(path.c_str()) == -1 && errno != ENOENT)
    throw fileError("Could not remove", path);
}

} // namespace

ProfileStore::ProfileStore(const std::string& directory)
{
  if (!directory.empty()) {
    profileDirectory = directory;
  } else {
    const char* home = core::getuserhomedir();
    if (home == nullptr || home[0] == '\0')
      throw std::runtime_error("Could not determine user home directory");
    profileDirectory = joinPath(home, "TigerVNC");
  }

  ensureDirectory();
}

void ProfileStore::ensureDirectory() const
{
  struct stat st;

  if (stat(profileDirectory.c_str(), &st) == 0) {
    if (!S_ISDIR(st.st_mode))
      throw std::runtime_error("Profile path is not a directory: " +
                               profileDirectory);
#ifndef _WIN32
    if (chmod(profileDirectory.c_str(), 0700) == -1)
      throw fileError("Could not protect profile directory", profileDirectory);
#endif
    return;
  }

  if (errno != ENOENT)
    throw fileError("Could not inspect profile directory", profileDirectory);

  if (mkdir(profileDirectory.c_str(), 0700) == -1 && errno != EEXIST)
    throw fileError("Could not create profile directory", profileDirectory);

  if (stat(profileDirectory.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
    throw std::runtime_error("Profile path is not a directory: " +
                             profileDirectory);
#ifndef _WIN32
  if (chmod(profileDirectory.c_str(), 0700) == -1)
    throw fileError("Could not protect profile directory", profileDirectory);
#endif
}

bool ProfileStore::isValidName(const std::string& name)
{
  if (name.empty() || name == "." || name == "..")
    return false;

  /* A name has to be visible in the configuration list */
  if (name.find_first_not_of(" \t") == std::string::npos)
    return false;

  for (size_t i = 0; i < name.size(); i++) {
    const unsigned char character = static_cast<unsigned char>(name[i]);
    if (character < 0x20 || character == 0x7f || name[i] == '/' ||
        name[i] == '\\')
      return false;
#ifdef _WIN32
    if (name[i] == ':')
      return false;
#endif
  }

  return true;
}

std::string ProfileStore::makePath(const std::string& name,
                                   const char* suffix) const
{
  if (!isValidName(name))
    throw std::invalid_argument("Invalid profile name");
  return joinPath(profileDirectory, name + suffix);
}

std::string ProfileStore::configPath(const std::string& name) const
{
  return makePath(name, CONFIG_SUFFIX);
}

std::string ProfileStore::passwordPath(const std::string& name) const
{
  return makePath(name, PASSWORD_SUFFIX);
}

bool ProfileStore::isRegularFile(const std::string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::vector<std::string> ProfileStore::listProfiles() const
{
  ensureDirectory();

  std::vector<std::string> profiles;
#ifdef _WIN32
  // The macOS viewer is the supported caller. Windows uses the existing
  // registry/configuration flow until its profile UI is implemented.
  return profiles;
#else
  DIR* directory = opendir(profileDirectory.c_str());
  if (directory == nullptr)
    throw fileError("Could not open profile directory", profileDirectory);

  const size_t suffixLength = std::strlen(CONFIG_SUFFIX);
  for (;;) {
    errno = 0;
    struct dirent* entry = readdir(directory);
    if (entry == nullptr) {
      if (errno != 0) {
        const int savedErrno = errno;
        closedir(directory);
        errno = savedErrno;
        throw fileError("Could not read profile directory", profileDirectory);
      }
      break;
    }

    const std::string filename(entry->d_name);
    if (filename.size() <= suffixLength ||
        filename.compare(filename.size() - suffixLength,
                         suffixLength, CONFIG_SUFFIX) != 0)
      continue;

    const std::string name = filename.substr(0, filename.size() - suffixLength);
    if (!isValidName(name) || !isRegularFile(joinPath(profileDirectory, filename)))
      continue;

    // 损坏的配置不进入首页列表，避免点击后才发现文件无效。
    std::string contents;
    try {
      if (!readConfig(name, &contents) ||
          contents.compare(0, std::strlen(CONFIG_HEADER), CONFIG_HEADER) != 0)
        continue;
    } catch (const std::exception&) {
      continue;
    }
    profiles.push_back(name);
  }

  closedir(directory);
  std::sort(profiles.begin(), profiles.end());
  return profiles;
#endif
}

bool ProfileStore::configExists(const std::string& name) const
{
  return isRegularFile(configPath(name));
}

bool ProfileStore::nameConflicts(const std::string& name,
                                 const std::string* ignoredName) const
{
  const std::vector<std::string> profiles = listProfiles();
  for (size_t i = 0; i < profiles.size(); i++) {
    if (ignoredName != nullptr && sameName(profiles[i], *ignoredName))
      continue;
    if (sameName(profiles[i], name))
      return true;
  }
  return false;
}

void ProfileStore::atomicWrite(const std::string& path,
                               const std::string& contents,
                               unsigned mode)
{
#ifndef _WIN32
  std::string temporaryPath = path + ".tmp.XXXXXX";
  std::vector<char> templatePath(temporaryPath.begin(), temporaryPath.end());
  templatePath.push_back('\0');
  const int fd = mkstemp(templatePath.data());
  if (fd == -1)
    throw fileError("Could not create temporary file", path);

  bool success = false;
  FILE* file = fdopen(fd, "wb");
  if (file != nullptr) {
    (void)fchmod(fd, mode);
    size_t written = 0;
    while (written < contents.size()) {
      const size_t count = fwrite(contents.data() + written, 1,
                                  contents.size() - written, file);
      if (count == 0)
        break;
      written += count;
    }
    if (written == contents.size() && fflush(file) == 0 &&
        fsync(fileno(file)) == 0 && fclose(file) == 0)
      success = true;
    else
      fclose(file);
  }

  if (!success) {
    const int savedErrno = errno;
    std::remove(templatePath.data());
    errno = savedErrno;
    throw fileError("Could not write", path);
  }

  if (rename(templatePath.data(), path.c_str()) == -1) {
    const int savedErrno = errno;
    std::remove(templatePath.data());
    errno = savedErrno;
    throw fileError("Could not replace", path);
  }
#else
  const std::string temporaryPath = path + ".tmp";
  {
    std::ofstream file(temporaryPath.c_str(), std::ios::binary | std::ios::trunc);
    if (!file)
      throw fileError("Could not create temporary file", path);
    file.write(contents.data(), contents.size());
    if (!file)
      throw fileError("Could not write", path);
  }
  if (std::rename(temporaryPath.c_str(), path.c_str()) != 0)
    throw fileError("Could not replace", path);
  (void)mode;
#endif
}

void ProfileStore::writeConfig(const std::string& name,
                               const std::string& contents) const
{
  ensureDirectory();
  if (configExists(name) || nameConflicts(name, nullptr))
    throw std::runtime_error("A profile with this name already exists");
  atomicWrite(configPath(name), contents, 0600);
}

bool ProfileStore::readConfig(const std::string& name,
                              std::string* contents) const
{
  const std::string path = configPath(name);
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file) {
    if (errno == ENOENT)
      return false;
    throw fileError("Could not open", path);
  }

  if (contents == nullptr)
    return true;

  contents->assign((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
  if (file.bad())
    throw fileError("Could not read", path);
  return true;
}

void ProfileStore::writePassword(const std::string& name,
                                 const std::string& password) const
{
  ensureDirectory();
  const std::string path = passwordPath(name);
  if (password.empty()) {
    removeIfPresent(path);
    return;
  }

  const std::vector<uint8_t> obfuscated = rfb::obfuscate(password.c_str());
  const std::string contents(reinterpret_cast<const char*>(obfuscated.data()),
                             obfuscated.size());
  atomicWrite(path, contents, 0600);
}

bool ProfileStore::readPassword(const std::string& name,
                                std::string* password) const
{
  const std::string path = passwordPath(name);
  std::ifstream file(path.c_str(), std::ios::binary);
  if (!file) {
    if (errno == ENOENT)
      return false;
    throw fileError("Could not open", path);
  }

  std::vector<uint8_t> contents((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
  if (file.bad())
    throw fileError("Could not read", path);
  if (contents.size() != 8)
    throw std::runtime_error("Invalid TigerVNC password file: " + path);
  if (password != nullptr)
    *password = rfb::deobfuscate(contents.data(), contents.size());
  return true;
}

void ProfileStore::removeProfile(const std::string& name) const
{
  removeIfPresent(configPath(name));
  removeIfPresent(passwordPath(name));
}

void ProfileStore::renameProfile(const std::string& oldName,
                                 const std::string& newName) const
{
  if (!isValidName(oldName) || !isValidName(newName))
    throw std::invalid_argument("Invalid profile name");
  if (sameName(oldName, newName)) {
    if (oldName == newName)
      return;

    /* A name that only differs in case is the same file on a case
       insensitive file system, so only the directory entry needs updating */
    const std::string oldConfig = configPath(oldName);
    const std::string newConfig = configPath(newName);
    const std::string oldPassword = passwordPath(oldName);
    const std::string newPassword = passwordPath(newName);
    const bool hasPassword = isRegularFile(oldPassword);

    if (std::rename(oldConfig.c_str(), newConfig.c_str()) != 0)
      throw fileError("Could not rename", oldConfig);

    if (hasPassword &&
        std::rename(oldPassword.c_str(), newPassword.c_str()) != 0) {
      const int savedErrno = errno;
      (void)std::rename(newConfig.c_str(), oldConfig.c_str());
      errno = savedErrno;
      throw fileError("Could not rename", oldPassword);
    }

    return;
  }
  if (!configExists(oldName))
    throw std::runtime_error("Profile does not exist: " + oldName);
  if (nameConflicts(newName, &oldName) ||
      isRegularFile(passwordPath(newName)))
    throw std::runtime_error("A profile with this name already exists");

  const std::string oldConfig = configPath(oldName);
  const std::string newConfig = configPath(newName);
  const std::string oldPassword = passwordPath(oldName);
  const std::string newPassword = passwordPath(newName);
  const bool hasPassword = isRegularFile(oldPassword);

  if (std::rename(oldConfig.c_str(), newConfig.c_str()) != 0)
    throw fileError("Could not rename", oldConfig);

  if (hasPassword && std::rename(oldPassword.c_str(), newPassword.c_str()) != 0) {
    const int savedErrno = errno;
    (void)std::rename(newConfig.c_str(), oldConfig.c_str());
    errno = savedErrno;
    throw fileError("Could not rename", oldPassword);
  }
}
