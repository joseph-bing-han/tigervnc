/* Copyright 2026 TigerVNC Team
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __PROFILEEDITOR_H__
#define __PROFILEEDITOR_H__

#include <string>

#include <FL/Fl_Window.H>

class Fl_Input;
class Fl_Secret_Input;
class Fl_Widget;
class OptionsPanel;

/* The editable values shared by the profile editor and its owner. */
struct ProfileEditorData {
  std::string name;
  std::string serverName;
  std::string username;
  std::string password;
};

class ProfileEditor : public Fl_Window {
public:
  /*
   * Display a modal editor, including all connection options, and update
   * the given values only when the user accepts them. Returns true when the
   * values were accepted.
   *
   * The parameters of a new configuration start from the defaults, and the
   * parameter state of the caller is restored before returning.
   */
  static bool edit(ProfileEditorData *data, bool creating);

protected:
  ProfileEditor(bool creating);
  ~ProfileEditor() override;

  static void handleCancel(Fl_Widget *widget, void *data);
  static void handleOK(Fl_Widget *widget, void *data);

  bool validate();
  void copyTo(ProfileEditorData *data) const;
  void copyFrom(const ProfileEditorData& data);

private:
  bool accepted;
  bool creating;
  Fl_Input *nameInput;
  Fl_Input *serverInput;
  Fl_Input *usernameInput;
  Fl_Secret_Input *passwordInput;
  OptionsPanel *options;
};

#endif
