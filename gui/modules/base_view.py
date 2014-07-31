#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback
import gtk  
import gobject

import links_and_nodes as ln
import gui_utils

class base_view(object):
    def __init__(self, name, app, view_id):   
        # self.name and self.app are persistent.
        # self.view_id is only valid after calling __init__ due to dynamic tab layout
        self.app = app
        self.name = name
        self.view_id = view_id
        bold = False
        if not self.view_id:
            bold = True
        self.tab_header = gui_utils.set_tab_header(self.name, self.close_tab, bold)

    def init_gui(self):
        # self.tab_header is gerated in the constructor of the base_view
        # every inherited gui needs self.builder and self.tab_main,
        # every glade file root widget has to be named 'main'
        filename = os.path.join(self.app.base_dir, 'resources/default.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")
        self.app.notebook.insert_page(self.tab_main, self.tab_header, self.view_id)
        self.tab_main.show_all()

    def close_tab(self, widget, event):
        # cleanup timers, callbacks, fds, ...
        self.app.close_tab(widget, event, self.name)
        return True

