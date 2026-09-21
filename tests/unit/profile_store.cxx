/* Copyright (C) 2026 TigerVNC contributors. */

#include <gtest/gtest.h>

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "ProfileStore.h"

namespace {

class ProfileStoreTest : public testing::Test {
protected:
  void SetUp() override
  {
    char path[] = "/tmp/tigervnc-profile-store-XXXXXX";
    const int fd = mkstemp(path);
    ASSERT_NE(fd, -1);
    close(fd);
    ASSERT_EQ(unlink(path), 0);
    directory = path;
  }

  void TearDown() override
  {
    const std::string command = "rm -rf " + directory;
    ASSERT_EQ(std::system(command.c_str()), 0);
  }

  std::string directory;
};

TEST_F(ProfileStoreTest, CreatesPrivateDirectoryAndListsProfiles)
{
  ProfileStore store(directory);

  struct stat st;
  ASSERT_EQ(stat(directory.c_str(), &st), 0);
  EXPECT_EQ(st.st_mode & 0777, 0700);

  const std::string header =
    "TigerVNC Configuration file Version 1.0\nServerName=one:5900\n";
  store.writeConfig("Zulu", header);
  store.writeConfig("Alpha", header);

  std::ofstream ignored((directory + "/ignored.txt").c_str());
  ignored << header;
  ignored.close();

  const std::vector<std::string> profiles = store.listProfiles();
  ASSERT_EQ(profiles.size(), 2U);
  EXPECT_EQ(profiles[0], "Alpha");
  EXPECT_EQ(profiles[1], "Zulu");
}

TEST_F(ProfileStoreTest, RejectsUnsafeNames)
{
  ProfileStore store(directory);

  EXPECT_FALSE(ProfileStore::isValidName(""));
  EXPECT_FALSE(ProfileStore::isValidName("."));
  EXPECT_FALSE(ProfileStore::isValidName(".."));
  EXPECT_FALSE(ProfileStore::isValidName("../outside"));
  EXPECT_FALSE(ProfileStore::isValidName("server/name"));
  EXPECT_FALSE(ProfileStore::isValidName("server\\name"));
  EXPECT_FALSE(ProfileStore::isValidName("server\nname"));
  EXPECT_TRUE(ProfileStore::isValidName("Office Mac 1.0"));
}

TEST_F(ProfileStoreTest, ReadsAndWritesConfigurationAtomically)
{
  ProfileStore store(directory);
  const std::string contents =
    "TigerVNC Configuration file Version 1.0\n"
    "ServerName=127.0.0.1\\:5900\n"
    "Username=alice\n";

  store.writeConfig("office", contents);

  std::string loaded;
  ASSERT_TRUE(store.readConfig("office", &loaded));
  EXPECT_EQ(loaded, contents);
  EXPECT_TRUE(store.configExists("office"));
  EXPECT_FALSE(store.readConfig("missing", &loaded));
}

TEST_F(ProfileStoreTest, StoresObfuscatedPasswordWithPrivatePermissions)
{
  ProfileStore store(directory);
  store.writePassword("office", "secret");

  std::string password;
  ASSERT_TRUE(store.readPassword("office", &password));
  EXPECT_EQ(password, "secret");

  struct stat st;
  ASSERT_EQ(stat(store.passwordPath("office").c_str(), &st), 0);
  EXPECT_EQ(st.st_mode & 0777, 0600);
}

TEST_F(ProfileStoreTest, RenamesAndRemovesConfigurationAndPassword)
{
  ProfileStore store(directory);
  const std::string contents =
    "TigerVNC Configuration file Version 1.0\nServerName=office:5900\n";
  store.writeConfig("office", contents);
  store.writePassword("office", "secret");

  store.renameProfile("office", "home");
  EXPECT_FALSE(store.configExists("office"));
  EXPECT_TRUE(store.configExists("home"));
  EXPECT_FALSE(store.readPassword("office", nullptr));

  std::string password;
  ASSERT_TRUE(store.readPassword("home", &password));
  EXPECT_EQ(password, "secret");

  store.removeProfile("home");
  EXPECT_FALSE(store.configExists("home"));
  EXPECT_FALSE(store.readPassword("home", nullptr));
}

TEST_F(ProfileStoreTest, RenamesCaseOnlyChanges)
{
  ProfileStore store(directory);
  const std::string contents =
    "TigerVNC Configuration file Version 1.0\nServerName=office:5900\n";
  store.writeConfig("office", contents);
  store.writePassword("office", "secret");

  store.renameProfile("office", "Office");
  EXPECT_FALSE(store.configExists("office"));
  EXPECT_TRUE(store.configExists("Office"));

  std::string password;
  ASSERT_TRUE(store.readPassword("Office", &password));
  EXPECT_EQ(password, "secret");
}

TEST_F(ProfileStoreTest, RejectsMalformedPassword)
{
  ProfileStore store(directory);
  std::ofstream password(store.passwordPath("office").c_str(),
                         std::ios::binary);
  password.put('\0');
  password.close();

  EXPECT_THROW(store.readPassword("office", nullptr), std::runtime_error);
}

TEST_F(ProfileStoreTest, EmptyPasswordRemovesPasswordFile)
{
  ProfileStore store(directory);
  store.writePassword("office", "secret");
  store.writePassword("office", "");
  EXPECT_FALSE(store.readPassword("office", nullptr));
}

} // namespace
