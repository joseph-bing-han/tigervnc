/* Copyright 2026 TigerVNC Team
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <algorithm>
#include <string.h>

#include <list>
#include <string>
#include <vector>

#include <core/Configuration.h>

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/fl_ask.H>

#include "fltk/layout.h"
#include "i18n.h"
#include "OptionsPanel.h"
#include "ProfileEditor.h"
#include "ProfileStore.h"
#include "parameters.h"

namespace {

struct ParameterState {
  core::VoidParameter *parameter;
  std::string value;
};

std::vector<ParameterState> snapshotParameters()
{
  core::Configuration *configuration = core::Configuration::global();
  std::vector<ParameterState> state;

  for (std::list<core::VoidParameter*>::iterator iter = configuration->begin();
       iter != configuration->end(); ++iter)
    state.push_back({*iter, (*iter)->getValueStr()});

  return state;
}

void restoreParameters(const std::vector<ParameterState>& state)
{
  for (const ParameterState& entry : state)
    entry.parameter->setParam(entry.value.c_str());
}

} // namespace


ProfileEditor::ProfileEditor(bool creating_)
  : Fl_Window(640, 600, creating_ ? _("Add VNC configuration")
                                  : _("Edit VNC configuration")),
    accepted(false),
    creating(creating_),
    nameInput(nullptr),
    serverInput(nullptr),
    usernameInput(nullptr),
    passwordInput(nullptr),
    options(nullptr)
{
  int x = OUTER_MARGIN;
  int y = OUTER_MARGIN;
  const int inputWidth = w() - OUTER_MARGIN * 2;
  Fl_Button *button;

  nameInput = new Fl_Input(
    LBLLEFT(x, y, inputWidth, INPUT_HEIGHT, _("Name:")));
  y += INPUT_HEIGHT + INNER_MARGIN;

  serverInput = new Fl_Input(
    LBLLEFT(x, y, inputWidth, INPUT_HEIGHT, _("VNC server:")));
  y += INPUT_HEIGHT + INNER_MARGIN;

  usernameInput = new Fl_Input(
    LBLLEFT(x, y, inputWidth, INPUT_HEIGHT, _("Username:")));
  y += INPUT_HEIGHT + INNER_MARGIN;

  passwordInput = new Fl_Secret_Input(
    LBLLEFT(x, y, inputWidth, INPUT_HEIGHT, _("Password:")));
  y += INPUT_HEIGHT + INNER_MARGIN;

  options = new OptionsPanel(x, y, inputWidth,
                             h() - y - OUTER_MARGIN - BUTTON_HEIGHT -
                             INNER_MARGIN);
  y += options->h() + INNER_MARGIN;

  int buttonX = w() - OUTER_MARGIN - BUTTON_WIDTH;
  button = new Fl_Button(buttonX, y, BUTTON_WIDTH, BUTTON_HEIGHT, fl_cancel);
  button->callback(handleCancel, this);
  button->shortcut(FL_Escape);

  buttonX -= INNER_MARGIN + BUTTON_WIDTH;
  button = new Fl_Return_Button(buttonX, y, BUTTON_WIDTH, BUTTON_HEIGHT,
                                creating ? _("Add") : _("Save"));
  button->callback(handleOK, this);

  end();
  resizable(nullptr);
  set_modal();
  callback(handleCancel, this);
}


ProfileEditor::~ProfileEditor()
{
}


bool ProfileEditor::edit(ProfileEditorData *data, bool creating)
{
  assert(data != nullptr);

  const std::vector<ParameterState> state = snapshotParameters();

  /* Keep unrelated settings out of a brand new configuration */
  if (creating)
    resetViewerParameters();

  ProfileEditor editor(creating);
  editor.copyFrom(*data);
  editor.options->loadOptions();
  editor.show();

  while (editor.shown())
    Fl::wait();

  const bool accepted = editor.accepted;

  if (accepted) {
    /* The caller stores the profile while these settings are still loaded */
    editor.options->storeOptions();
  } else {
    restoreParameters(state);
  }

  if (!accepted)
    return false;

  editor.copyTo(data);

  return true;
}


void ProfileEditor::copyFrom(const ProfileEditorData& data)
{
  nameInput->value(data.name.c_str());
  serverInput->value(data.serverName.c_str());
  usernameInput->value(data.username.c_str());
  passwordInput->value(data.password.c_str());
}


void ProfileEditor::copyTo(ProfileEditorData *data) const
{
  data->name = nameInput->value() != nullptr ? nameInput->value() : "";
  data->serverName = serverInput->value() != nullptr ? serverInput->value() : "";
  data->username = usernameInput->value() != nullptr ? usernameInput->value() : "";
  data->password = passwordInput->value() != nullptr ? passwordInput->value() : "";
}


bool ProfileEditor::validate()
{
  const char *name = nameInput->value();
  const char *server = serverInput->value();

  if (server == nullptr || server[0] == '\0') {
    fl_alert(_("A VNC server is required."));
    return false;
  }

  if (name == nullptr || !ProfileStore::isValidName(name)) {
    fl_alert(_("The configuration name is not valid."));
    return false;
  }

  return true;
}


void ProfileEditor::handleCancel(Fl_Widget* /*widget*/, void *data)
{
  ProfileEditor *editor = (ProfileEditor *)data;
  editor->accepted = false;
  editor->hide();
}


void ProfileEditor::handleOK(Fl_Widget* /*widget*/, void *data)
{
  ProfileEditor *editor = (ProfileEditor *)data;
  if (!editor->validate())
    return;

  editor->accepted = true;
  editor->hide();
}
