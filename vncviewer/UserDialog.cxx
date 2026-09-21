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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Pixmap.H>

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rfb/CConnection.h>
#include <rfb/Exception.h>
#include <rfb/obfuscate.h>

#include "fltk/layout.h"
#include "fltk/util.h"
#include "i18n.h"
#include "parameters.h"
#include "UserDialog.h"

/* xpm:s predate const, so they have invalid definitions */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include "../media/secure.xpm"
#include "../media/insecure.xpm"
#pragma GCC diagnostic pop

static Fl_Pixmap secure_icon(secure);
static Fl_Pixmap insecure_icon(insecure);

static core::LogWriter vlog("UserDialog");

/* Stores a password in the same obfuscated format as vncpasswd */
static void savePasswordFile(const char *path, const std::string& password)
{
  const std::vector<uint8_t> obfuscated = rfb::obfuscate(password.c_str());

  FILE *fp = fopen(path, "wb");
  if (fp == nullptr) {
    vlog.error(_("Could not store the password in \"%s\": %s"),
               path, strerror(errno));
    return;
  }

#ifndef WIN32
  if (fchmod(fileno(fp), S_IRUSR | S_IWUSR) == -1)
    vlog.error(_("Could not protect \"%s\": %s"), path, strerror(errno));
#endif

  if (fwrite(obfuscated.data(), 1, obfuscated.size(), fp) !=
      obfuscated.size())
    vlog.error(_("Could not store the password in \"%s\": %s"),
               path, strerror(errno));

  fclose(fp);
}

std::string UserDialog::savedUsername;
std::string UserDialog::savedPassword;
static bool managedPasswordFile = false;

static long ret_val = 0;

static void button_cb(Fl_Widget *widget, long val) {
  Fl_Window* win;

  ret_val = val;

  assert(widget != nullptr);
  win = dynamic_cast<Fl_Window*>(widget);
  if (win == nullptr)
    win = widget->window();
  assert(win != nullptr);
  win->hide();
}

UserDialog::UserDialog()
{
}

UserDialog::~UserDialog()
{
}

void UserDialog::resetPassword()
{
  savedUsername.clear();
  savedPassword.clear();

  /* A failed authentication means the stored password is no longer valid,
     so drop it and let the user enter a new one */
  const char *passwordFileName(passwordFile);

  if (managedPasswordFile && passwordFileName[0]) {
    if (remove(passwordFileName) == -1 && errno != ENOENT)
      vlog.error(_("Could not remove \"%s\": %s"),
                 passwordFileName, strerror(errno));
  }
}

void UserDialog::resetSavedCredentials()
{
  savedUsername.clear();
  savedPassword.clear();
}

void UserDialog::setManagedPasswordFile(bool managed)
{
  managedPasswordFile = managed;
}

void UserDialog::getUserPasswd(bool secure_, std::string* user,
                               std::string* password)
{
  const char *passwordFileName(passwordFile);

  assert(password);
  char *envUsername = getenv("VNC_USERNAME");
  char *envPassword = getenv("VNC_PASSWORD");

  if(user && envUsername && envPassword) {
    *user = envUsername;
    *password = envPassword;
    return;
  }

  if (!user && envPassword) {
    *password = envPassword;
    return;
  }

  if (user && !savedUsername.empty() && !savedPassword.empty()) {
    *user = savedUsername;
    *password = savedPassword;
    return;
  }

  if (!user && !savedPassword.empty()) {
    *password = savedPassword;
    return;
  }

  if (passwordFileName[0]) {
    FILE* fp = fopen(passwordFileName, "rb");

    if (fp != nullptr) {
      std::vector<uint8_t> obfPwd(8);

      obfPwd.resize(fread(obfPwd.data(), 1, obfPwd.size(), fp));
      fclose(fp);

      if (obfPwd.size() == 8) {
        if (user)
          *user = ::username;
        *password = rfb::deobfuscate(obfPwd.data(), obfPwd.size());
        return;
      }
    }

    /* Nothing stored yet, so ask the user and store it afterwards */
    vlog.info(_("No usable password in \"%s\""), passwordFileName);
  }

  Fl_Window *win;
  Fl_Box *banner;
  Fl_Input *usernameInput;
  Fl_Secret_Input *passwd;
  Fl_Box *icon;
  Fl_Button *button;
  Fl_Check_Button *keepPasswdCheckbox;

  int x, y;

  win = new Fl_Window(410, 0, _("VNC authentication"));
  win->callback(button_cb, 1);

  banner = new Fl_Box(0, 0, win->w(), 20);
  banner->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE|FL_ALIGN_IMAGE_NEXT_TO_TEXT);
  banner->box(FL_FLAT_BOX);
  if (secure_) {
    banner->label(_("This connection is secure"));
    banner->color(FL_GREEN);
    banner->image(secure_icon);
  } else {
    banner->label(_("This connection is not secure"));
    banner->color(FL_RED);
    banner->image(insecure_icon);
  }

  x = OUTER_MARGIN;
  y = banner->h() + OUTER_MARGIN;

  /* Mimic a fl_ask() box */
  icon = new Fl_Box(x, y, 50, 50, "?");
  icon->box(FL_UP_BOX);
  icon->labelfont(FL_TIMES_BOLD);
  icon->labelsize(34);
  icon->color(FL_WHITE);
  icon->labelcolor(FL_BLUE);

  x += icon->w() + INNER_MARGIN;
  y += INNER_MARGIN;

  if (user) {
    y += INPUT_LABEL_OFFSET;
    usernameInput = new Fl_Input(x, y, win->w()- x - OUTER_MARGIN,
                                 INPUT_HEIGHT, _("Username:"));
    usernameInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    usernameInput->value(::username);
    y += INPUT_HEIGHT + INNER_MARGIN;
  } else {
    /*
     * Compiler is not bright enough to understand that
     * username won't be used further down...
     */
    usernameInput = nullptr;
  }

  y += INPUT_LABEL_OFFSET;
  passwd = new Fl_Secret_Input(x, y, win->w()- x - OUTER_MARGIN,
                               INPUT_HEIGHT, _("Password:"));
  passwd->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
  y += INPUT_HEIGHT + INNER_MARGIN;

  if (reconnectOnError) {
    keepPasswdCheckbox = new Fl_Check_Button(LBLRIGHT(x, y,
                                                      CHECK_MIN_WIDTH,
                                                      CHECK_HEIGHT,
                                                      _("Keep password for reconnect")));
    y += CHECK_HEIGHT + INNER_MARGIN;
  } else {
    keepPasswdCheckbox = nullptr;
  }

  x = win->w() - OUTER_MARGIN;
  y += OUTER_MARGIN - INNER_MARGIN;

  x -= BUTTON_WIDTH;
  button = new Fl_Return_Button(x, y, BUTTON_WIDTH,
                                BUTTON_HEIGHT, fl_ok);
  button->callback(button_cb, 0);
  x -= INNER_MARGIN;

  x -= BUTTON_WIDTH;
  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, fl_cancel);
  button->callback(button_cb, 1);
  button->shortcut(FL_Escape);
  x -= INNER_MARGIN;

  y += BUTTON_HEIGHT;

  y += OUTER_MARGIN;

  win->end();

  win->size(win->w(), y);

  win->set_modal();

  ret_val = -1;

  win->show();
  while (win->shown()) Fl::wait();

  if (ret_val == 0) {
    bool keepPasswd;

    if (reconnectOnError)
      keepPasswd = keepPasswdCheckbox->value();
    else
      keepPasswd = false;

    if (user) {
      *user = usernameInput->value();
      ::username.setParam(usernameInput->value());
      if (keepPasswd)
        savedUsername = usernameInput->value();
    }
    *password = passwd->value();
    if (keepPasswd)
      savedPassword = passwd->value();

    /* Remember the password so that the next connection can be automatic */
    if (managedPasswordFile && passwordFileName[0])
      savePasswordFile(passwordFileName, *password);
  }

  delete win;

  if (ret_val != 0)
    throw rfb::auth_cancelled();
}

bool UserDialog::showMsgBox(rfb::MsgBoxFlags flags,
                            const char* title, const char* text)
{
  char buffer[1024];

  if (fltk_escape(text, buffer, sizeof(buffer)) >= sizeof(buffer))
    return 0;

  // FLTK doesn't give us a flexible choice of the icon, so we ignore those
  // bits for now.

  fl_message_title(title);

  switch (flags & 0xf) {
  case rfb::M_OKCANCEL:
    return fl_choice("%s", nullptr, fl_ok, fl_cancel, buffer) == 1;
  case rfb::M_YESNO:
    return fl_choice("%s", nullptr, fl_yes, fl_no, buffer) == 1;
  case rfb::M_OK:
  default:
    if (((flags & 0xf0) == rfb::M_ICONERROR) ||
        ((flags & 0xf0) == rfb::M_ICONWARNING))
      fl_alert("%s", buffer);
    else
      fl_message("%s", buffer);
    return true;
  }

  return false;
}
