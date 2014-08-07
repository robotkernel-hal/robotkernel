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
import sercos_wrapper


class sercos_param_subview(object):
    def __init__(self, name, app, device_store):  
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = self.app.window.get_style().text[0].to_string()
        self.device_store = device_store
        self.init_gui()

    def init_gui(self):
        filename = os.path.join(self.app.base_dir, 'resources/dual_list.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")

        #device view at param page
        wrong_device_store, self.device_view_params = self.create_device_treeview() 
        self.sw_devices = self.builder.get_object("devices") #scrolledwindow
        self.device_view_params.set_model(self.device_store)
        self.sw_devices.add(self.device_view_params)
        self.device_view_params.connect("cursor-changed", self.on_device_view_params_cursor_changed)

        #treeview for params
        self.param_store, self.param_view = self.create_parameter_treeview() 
        self.sw_param = self.builder.get_object("values") #scrolledwindow
        self.sw_param.add(self.param_view)  
        self.param_view.connect("key-press-event", self.on_keypress_param_view)

        #force the view initially to be loaded with dev 0
        self.device_view_params.set_cursor((0, ))
        self.update_param_view(None)

        self.builder.connect_signals(self)
        return self.tab_main


    #CREATORS
    def create_device_treeview(self):
        store = gtk.ListStore(int, str, gobject.TYPE_PYOBJECT) # name, data
        view = gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)
        
        # populate store
        def view_str(column, cell, store, iter, i):
            v = store.get_value(iter,i)    
            cell.set_property('text', v)
            column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
            return
        
        cr = gtk.CellRendererText()   
        view.insert_column_with_data_func(-1, "device" , cr,  view_str, 0)
        view.insert_column_with_data_func(-1, "name" , cr,  view_str, 1)
        store.set_sort_column_id(0, 0)        
        return store, view   


    def create_parameter_treeview(self):
        store = gtk.TreeStore(str, str, gobject.TYPE_PYOBJECT) # sercos id type 
        view = gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)
       
        def insert_str(column, cell, store, iter, i):
            v = store.get_value(iter,i)    
            cell.set_property('text', v)
            column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
            return

        cr = gtk.CellRendererText()   
        view.insert_column_with_data_func(-1, "idn" , cr,  insert_str, 0)
        view.insert_column_with_data_func(-1, "name" , cr,  insert_str, 1)
        self.paramvalue_renderer = gtk.CellRendererText() 
        self.paramvalue_renderer.connect("edited", self.edit_sercos_param_value)
        view.insert_column_with_data_func(-1, "value" , self.paramvalue_renderer,  self.insert_sercos_parameter)

        store.set_sort_column_id(0, 0)
        for i, col in enumerate(view.get_columns()):
            col.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
            col.set_resizable(True)
            col.set_min_width(75 * (i*2 + 1))  
        return store, view  
	

    #HELPER   
    def insert_sercos_parameter(self, column, cell, model, iter):
        #datafunc for param
        if not gui_utils.is_row_visible(model, iter, self.param_view):
            return True
   
        set_number = model.get_path(iter)[0]   
        sercos_device = self.get_selected_device()
        sercos_parameterset = sercos_device.sercos_parametersets[set_number]   
          
        row = model[iter] 
        if row[0].startswith("Set"):
            cell.set_property("text", "")#all(sercos_parameterset.valid_set))
            return True

        n = int(row[0].split(" ")[-1])
        sercos_parameterset.get_parameters()
        cell.set_property("text", sercos_parameterset.parameters[n][1])
        if not sercos_parameterset.valid_set[n]:
            cell.set_property("foreground", "grey")
        else:
            cell.set_property("foreground", self.active_color)
        return True


    def get_selected_device(self):
        model, iter = self.device_view_params.get_selection().get_selected()
        if iter is None:
            return None
        return model[iter][2] #device_id, device_name, pyobject


    #CALLBACKS
    def on_device_view_params_cursor_changed(self, widget):
        sercos_device = self.get_selected_device()
        if not sercos_device:
            return True

        self.param_store.clear()
        for set_number in sercos_device.list_sercos_parametersets(self.param_view):
            sercos_parameterset = sercos_device.sercos_parametersets[set_number]
            iter = self.param_store.insert(None, -1, ["Set %i" %sercos_parameterset.number, "", ""])
            for j, (name, value) in enumerate(sercos_parameterset.parameters):
                 iter2 = self.param_store.insert(iter, -1, ["Parameter %i" %j, name, None]) 
        self.param_view.expand_all()
        return True
              

    def on_keypress_param_view(self, widget, event):
        #catch F5 for update treeview element
        key = gtk.gdk.keyval_name(event.keyval)
        if key in ["F5"]:
            self.update_param_view(widget)
        else:
            return False        
        return True


    def update_param_view(self, widget):
        #force update all paraemters
        sercos_device = self.get_selected_device()
        if sercos_device is None:
            return True
        for name, param_set in sercos_device.sercos_parametersets.items():
            param_set.valid_set = [False] * 10
        self.param_view.queue_draw()
        return True


    def edit_sercos_param_value(self, cr, path, newvalue):
        #write to sercos after editing (confirm with ENTER)
        path = path.split(":")
        set_number = int(path[0])
        parameter_number = int(path[1])
        sercos_device = self.get_selected_device()
        parameter_set = sercos_device.sercos_parametersets[set_number]      
        parameter_set.set_parameter(parameter_number, newvalue)
        parameter_set.get_parameters(force_update=True)
        return True










