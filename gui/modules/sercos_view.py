#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback
import gtk  
import gobject
import yaml
import datetime

from string import maketrans 
from numpy import *

import links_and_nodes as ln

import gui_utils
from modules import *

import sercos_id_subview
import sercos_param_subview
import sercos_diag_subview
import sercos_processdata_subview
import sercos_flasher_subview


class sercos_view(base_view):
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
        print "loading sercos page %s" %self.name
        filename = os.path.join(self.app.base_dir, 'resources/base.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")
  
        #fill the invisble notebook with desired pages       
        self.id_view = sercos_id_subview.sercos_id_subview(self.name, self.app)
        self.processdata_view  = sercos_processdata_subview.sercos_processdata_subview(self.name, self.app, self.id_view.device_store)
        self.param_view = sercos_param_subview.sercos_param_subview(self.name, self.app, self.id_view.device_store)
        self.diag_view = sercos_diag_subview.sercos_diag_subview(self.name, self.app, self.id_view.device_store)
        self.flasher_view = sercos_flasher_subview.sercos_flasher_subview(self, self.name, self.app)

        self.notebook.append_page(self.diag_view.tab_main, gtk.Label("overview"))
        self.notebook.append_page(self.processdata_view.tab_main, gtk.Label("process data"))
        self.notebook.append_page(self.id_view.tab_main, gtk.Label("sercos ids"))
        self.notebook.append_page(self.param_view.tab_main, gtk.Label("sercos parameter"))       
        self.notebook.append_page(self.flasher_view.tab_main, gtk.Label("sercos flasher"))       

        #combostore to controll invisible notebook
        self.combo_store = gtk.ListStore(str, int)
        self.combo_nb = self.builder.get_object("select_mode_cb")
        self.combo_nb.set_model(self.combo_store)
        for n in xrange(self.notebook.get_n_pages()):
            page = self.notebook.get_nth_page(n)
            text = self.notebook.get_tab_label_text(page)
            self.combo_store.insert(-1, [text, n])  
        cr = gtk.CellRendererText()
        self.combo_nb.pack_start(cr, True)
        self.combo_nb.add_attribute(cr, "text", 0)
        self.combo_nb.connect("changed", self.change_nb_page)
        gui_utils.set_active_cb_text(self.combo_nb, "overview")

        self.file_save_dialog = gtk.FileChooserDialog("Select File", None, gtk.FILE_CHOOSER_ACTION_SAVE,
                             (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))

        self.folder_select_dialog = gtk.FileChooserDialog("Select File", None, gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER,
                             (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))

        self.file_open_dialog = gtk.FileChooserDialog("Select File", None, gtk.FILE_CHOOSER_ACTION_OPEN,
                                     (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK))

        self.builder.connect_signals(self)
        return True

    #CALLBACKS
    def on_togglebutton_edit_toggled(self, widget):
        #enable editable ids
        self.id_view.idvalue_renderer.set_property("editable", widget.get_active())
        self.param_view.paramvalue_renderer.set_property("editable", widget.get_active())
        self.diag_view.enable_command = widget.get_active()
        return True

    def on_button_backup_all_clicked(self, widget):
        #create backup of sercos device ids/parameterset
        dictionary = maketrans(" ", "-")
        day = datetime.date.today().strftime('%Y%m%d')
        text = gui_utils.get_current_nb_text(self.notebook)       
        p = gui_utils.show_file_dialog(self.folder_select_dialog)
        if p is None:
            return True
        print "waiting for data",
        for d in self.id_view.device_store:
            dev = d[2]
            if text == "sercos ids":
                dev.list_sercos_ids(self.id_view.id_view) #list in case they are not yet gathered from bus
                fn = ("ids_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).translate(dictionary)
                dev.backup_ids("%s/%s" %(p, fn))
            elif text == "sercos parameter":
                dev.list_sercos_parametersets(self.param_view.param_view) #list in case they are not yet gathered from bus
                fn = ("parameters_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).translate(dictionary)
                dev.backup_parametersets("%s/%s" %(p, fn))
        return True


    def on_button_backup_clicked(self, widget):
        #create backup of sercos device ids/parameterset
        dictionary = maketrans(" ", "-")
        day = datetime.date.today().strftime('%Y%m%d')
        text = gui_utils.get_current_nb_text(self.notebook)
        if text == "sercos ids":
            dev = self.id_view.get_selected_device()
            if dev is None:
                return True
            sugestion = ("ids_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).translate(dictionary)
            f = gui_utils.show_file_dialog(self.file_save_dialog, name=sugestion)
            if f is not None:
                print "waiting for data",
                dev.backup_ids(f)
        elif text == "sercos parameter":
            dev = self.param_view.get_selected_device()
            if dev is None:
                return True
            sugestion = ("parameters_%s_%s_%s.yaml" %(dev.device_name, dev.device_number, day)).translate(dictionary)
            f = gui_utils.show_file_dialog(self.file_save_dialog, name=sugestion)
            if f is not None:
                print "waiting for data",
                dev.backup_parametersets(f)
        return True
  
        
    def on_button_load_clicked(self, widget):
        #create backup of sercos device ids/parameterset
        text = gui_utils.get_current_nb_text(self.notebook)
        if text == "sercos ids":
            dev = self.id_view.get_selected_device()
            if dev is None:
                return True
            f = gui_utils.show_file_dialog(self.file_open_dialog)
            if f is not None:
                dev.load_ids(f)
        elif text == "sercos parameter":
            dev = self.param_view.get_selected_device()
            if dev is None:
                return True
            f = gui_utils.show_file_dialog(self.file_open_dialog)
            if f is not None:
                dev.load_parametersets(f)
        return True
    
    
    def on_button_refresh_clicked(self, widget):
        #update activated value view
        text = gui_utils.get_current_nb_text(self.notebook)
        if text == "sercos ids":
            self.id_view.update_id_view(widget)
        if text == "sercos parameter":
            self.param_view.update_param_view(widget)
        return True
        
    
    def change_nb_page(self, widget, *args):
        #when mode selected from combobox
        model = widget.get_model()
        active = widget.get_active()
        if active < 0:
            return True
        page = model[active][1] 
        self.notebook.set_current_page(page)
        text = gui_utils.get_current_nb_text(self.notebook)
        
        sensitive = text not in ["overview", "process data", "sercos flasher"]
        self.button_backup_all.set_sensitive(sensitive)
        self.button_backup.set_sensitive(sensitive)    
        setattr(self.button_backup, 'visible', sensitive)
        self.button_load.set_sensitive(sensitive)  
        self.button_refresh.set_sensitive(sensitive)
        sensitive = text not in ["process data", "sercos flasher"]
        self.togglebutton_edit.set_sensitive(sensitive)    

        if text == "process data" and not self.processdata_view.views_created:
            self.processdata_view.create_views()
        
        return True











