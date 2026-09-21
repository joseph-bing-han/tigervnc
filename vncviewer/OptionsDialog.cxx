/* Copyright 2011-2025 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <map>

#include "OptionsDialog.h"
#include "OptionsPanel.h"
#include "i18n.h"

#include "fltk/layout.h"

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>

std::map<OptionsCallback*, void*> OptionsDialog::callbacks;

OptionsDialog::OptionsDialog()
  : Fl_Window(580, 420, _("TigerVNC options"))
{
  int x, y;
  Fl_Button *button;

  panel = new OptionsPanel(0, 0, w(),
                           h() - OUTER_MARGIN - BUTTON_HEIGHT - OUTER_MARGIN);

  x = w() - BUTTON_WIDTH * 2 - INNER_MARGIN - OUTER_MARGIN;
  y = h() - BUTTON_HEIGHT - OUTER_MARGIN;

  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Cancel"));
  button->callback(this->handleCancel, this);

  x += BUTTON_WIDTH + INNER_MARGIN;

  button = new Fl_Return_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("OK"));
  button->callback(this->handleOK, this);

  callback(this->handleCancel, this);

  set_modal();
}


OptionsDialog::~OptionsDialog()
{
}


void OptionsDialog::showDialog(void)
{
  static OptionsDialog *dialog = nullptr;

  if (!dialog)
    dialog = new OptionsDialog();

  if (dialog->shown())
    return;

  dialog->show();
}


void OptionsDialog::addCallback(OptionsCallback *cb, void *data)
{
  callbacks[cb] = data;
}


void OptionsDialog::removeCallback(OptionsCallback *cb)
{
  callbacks.erase(cb);
}


void OptionsDialog::show(void)
{
  /* show() gets called for raise events as well */
  if (!shown())
    panel->loadOptions();

  Fl_Window::show();
}


void OptionsDialog::handleCancel(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();
}


void OptionsDialog::handleOK(Fl_Widget* /*widget*/, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();

  dialog->panel->storeOptions();

  /* Let the running sessions pick up the new settings */
  std::map<OptionsCallback*, void*>::const_iterator iter;

  for (iter = callbacks.begin();iter != callbacks.end();++iter)
    iter->first(iter->second);
}
