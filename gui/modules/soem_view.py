#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback
import gtk  
import gobject
import yaml

from numpy import *

import links_and_nodes as ln

import gui_utils
from modules import *
import soem_canopen_subview

def match_func(row, data):
    for i in data:
        column, key = i # data is a tuple containing column number, key
        if row[column] != key:
            return False
    return True


def search(rows, func, data):
    if not rows: 
        return None 
    for row in rows:
        if func(row, data):
            return row
        result = search(row.iterchildren(), func, data)
        if result: 
            return result
    return None

class soem_view(base_view):
    def __init__(self, name, app, view_id):   
        base_view.__init__(self, name, app, view_id)
        self.active_color = self.app.window.get_style().text[0].to_string()
        self.parent = name.split(".")[0]
        self.clnt = self.app.clnt
        self.init_gui()
        self.app.notebook.insert_page(self.tab_main, self.tab_header, self.view_id)
        self.tab_main.show_all()
        

    def __getattr__(self, name):
        widget = self.builder.get_object(name)
        if widget is None:
            raise AttributeError(name)
        setattr(self, name, widget)
        return widget


    def init_gui(self):
        print "loading soem page %s" %self.name      
        filename = os.path.join(self.app.base_dir, 'resources/base.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")

        #invisible notebook       
        self.notebook = self.builder.get_object("notebook")

        #fill the notebook with desired pages
        self.canopen_view = soem_canopen_subview.soem_canopen_subview(self.name, self.app)
        self.notebook.append_page(self.canopen_view.tab_main, gtk.Label("canopen ids"))
        
        self.sercos_view = sercos_id_subview.sercos_id_subview(self.name, self.app)
        if len(self.sercos_view.device_store):
            self.notebook.append_page(self.sercos_view.tab_main, gtk.Label("sercos ids"))       

        #combostore to controll invisible notebook
        self.combo_store = gtk.ListStore(str, int)
        self.combo_nb = self.builder.get_object("select_mode_cb")
        self.combo_nb.set_model(self.combo_store)
        for n in xrange(self.notebook.get_n_pages()):
            page = self.notebook.get_nth_page(n)
            text = self.notebook.get_tab_label_text(page)
            self.combo_store.insert(-1, [text, n])  
            if n == 0:
                gui_utils.set_active_cb_text(self.combo_nb, text)
        cr = gtk.CellRendererText()
        self.combo_nb.pack_start(cr, True)
        self.combo_nb.add_attribute(cr, "text", 0)
        self.combo_nb.connect("changed", self.change_nb_page)
        gui_utils.set_active_cb_text(self.combo_nb, "canopen ids")
        
        sensitive = False
        self.button_backup_all.set_sensitive(sensitive)
        self.button_backup.set_sensitive(sensitive)    
        self.button_load.set_sensitive(sensitive)  
        self.button_refresh.set_sensitive(sensitive)

        self.togglebutton_edit.set_sensitive(sensitive)
        

    #HELPER
    def get_selected_device(self, widget):
        model, iter = widget.get_selection().get_selected()
        return model[iter][2] #device_id, device_name, pyobject
     

    def change_nb_page(self, widget, *args):
        #when mode selected from combobox
        model = widget.get_model()
        active = widget.get_active()
        if active < 0:
            return True
        page = model[active][1] 
        self.notebook.set_current_page(page)
        return True








