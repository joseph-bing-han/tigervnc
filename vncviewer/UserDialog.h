/* Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __USERDIALOG_H__
#define __USERDIALOG_H__

#include <string>

#include <rfb/CConnection.h>

class UserDialog
{
public:
  UserDialog();
  ~UserDialog();

  // UserPasswdGetter callbacks

  void getUserPasswd(bool secure, std::string* user,
                     std::string* password);

  // UserMsgBox callbacks
  bool showMsgBox(rfb::MsgBoxFlags flags, const char* title, const char* text);

  void resetPassword();

  /* 清除当前进程缓存的凭据，但不删除配置文件中的密码。 */
  static void resetSavedCredentials();

  /* 标记当前 PasswordFile 是否由 macOS 配置列表管理。 */
  static void setManagedPasswordFile(bool managed);

private:
  static std::string savedUsername;
  static std::string savedPassword;
};

#endif
