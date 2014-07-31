#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback
import gtk  
import gobject

import links_and_nodes as ln

import gui_utils
from modules import *

class jitter_view(base_view, ln.ln_wrappers.services_wrapper):
    def __init__(self, name, app, view_id):   
        base_view.__init__(self, name, app, view_id)
        ln.ln_wrappers.services_wrapper.__init__(self, self.app.clnt, self.name)
        self.parent = name.split(".")[0]

        self.wrap_service("reset_max_ever", "jitter_measurement/reset_max_ever")

        self.init_gui()

    def init_gui(self):
        print "loading jitter view"
        filename = os.path.join(self.app.base_dir, 'resources/jitter.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")
        
        self.app.notebook.insert_page(self.tab_main, self.tab_header, self.view_id)

        self.sourceview = gui_utils.setup_sourceview("python", "Monospace 9")
        self.sourceview.set_editable(False)
        sw = self.builder.get_object("log") #scrolledwindow
        sw.add(self.sourceview)

        button = self.builder.get_object("button_reset")
        button.connect("clicked", self.on_button_reset_clicked)

        self.tab_main.show_all()

    def on_button_reset_clicked(self, widget):
        max_ever = self.reset_max_ever()
        self.sourceview.get_buffer().set_text(str(max_ever))
        return True


