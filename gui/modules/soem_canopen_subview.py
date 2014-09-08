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
import soem_wrapper

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


class soem_canopen_subview(object):
    def __init__(self, name, app):   
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = self.app.window.get_style().text[0].to_string()
        self.init_gui()


    def __getattr__(self, name):
        widget = self.builder.get_object(name)
        if widget is None:
            raise AttributeError(name)
        setattr(self, name, widget)
        return widget


    def init_gui(self):
        filename = os.path.join(self.app.base_dir, 'resources/dual_list.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")

        # treeviews
        self.create_device_treeview()
        self.sw_devices = self.builder.get_object("devices") #scrolledwindow
        self.sw_devices.add(self.treeview_devices)
        self.treeview_devices.connect("cursor-changed", self.on_treeview_devices_cursor_changed)
        
        self.create_dictionary_treeview()
        self.sw_values = self.builder.get_object("values") #scrolledwindow
        self.sw_values.add(self.treeview_dictionary)

        self.builder.connect_signals(self)
        return self.tab_main


    #CREATORS
    def create_device_treeview(self):
        self.liststore_devices = store = gtk.ListStore(int, str, gobject.TYPE_PYOBJECT) # name, data
        self.treeview_devices = view = gtk.TreeView(store) #set_model(store)
        view.set_model(store)
        view.insert_column(gtk.TreeViewColumn("slave", gtk.CellRendererText(), text=0), -1)
        view.insert_column(gtk.TreeViewColumn("name", gtk.CellRendererText(), text=1), -1)
        view.connect("cursor-changed", self.on_treeview_devices_cursor_changed)
        store.set_sort_column_id(0, 0)

        #initially fill all devices found by ln
        for s in self.clnt.find_services_with_interface("robotkernel/canopen_protocol/object_dictionary_list").split("\n"):
            if not s.startswith(self.name):
                continue
            number = s.split(".")[-3]
            soem_device = soem_wrapper.soem_device(self.app.clnt, "%s.%s" %(self.name, number), self)
            name = soem_device.read_element(index=4104, sub_index=0).value
            self.liststore_devices.append( (int(number.split("_")[-1]), name, soem_device) )
  
        
    
    def create_dictionary_treeview(self):
        self.treestore_dictionary = store = gtk.TreeStore(int, str, gobject.TYPE_PYOBJECT) # soem id type 
        self.treeview_dictionary = view = gtk.TreeView(store)
        view.set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)

        cell_colors = { True: self.active_color, False: "grey" }

        # ------------------ index column ------------------
        def cb_idn(column, cell, store, iter):
            cell.set_property("xalign", 1.0)
            if store.iter_parent(iter):
                cell.set_property('text', '%d'  % store[iter][0])
            else:
                cell.set_property('text', '0x%04X'  % store[iter][0])
            return True

        col_cnt = view.insert_column_with_data_func(-1, "idn" , gtk.CellRendererText(), cb_idn)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)

        # ------------------ name column -------------------
        def cb_name(column, cell, store, iter):
            parent_iter = store.iter_parent(iter)
            if not gui_utils.is_row_visible(store, iter, self.treeview_dictionary):
                return

            row = store[iter]
            soem_device  = self.get_selected_device(self.treeview_devices)

            if parent_iter:
                index        = store[parent_iter][0]
                sub_index    = row[0]
                soem_object  = soem_device.soem_dictionary[index]
                soem_element = soem_object.subindices[sub_index]
                soem_element.get_data()
                cell.set_property("text", soem_element.name)
                cell.set_property("foreground", cell_colors[soem_element.valid])
                pass
            else:
                index        = row[0]
                soem_object  = soem_device.soem_dictionary[row[0]]
                soem_object.get_data()
            
                for sub in range(0, soem_object.max_subindices + 1):
                    sub_iter = search([row, ], match_func, ([0, sub], ))
                    if sub_iter is None:
                        self.treestore_dictionary.insert(iter, -1, [sub, "", soem_device])
                    
                cell.set_property("text", soem_object.name)
                cell.set_property("foreground", cell_colors[soem_object.valid])
            return True
            
        col_cnt = view.insert_column_with_data_func(-1, "name", gtk.CellRendererText(), cb_name)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
        column.set_expand(True)
        column.set_resizable(True)


        # ------------------ data column -------------------
        def cb_data(column, cell, store, iter):
            parent_iter = store.iter_parent(iter)
            if not gui_utils.is_row_visible(store, iter, self.treeview_dictionary):
                return

            row = store[iter]
            soem_device = self.get_selected_device(self.treeview_devices)

            if parent_iter:
                index        = store[parent_iter][0]
                sub_index    = row[0]
                soem_object  = soem_device.soem_dictionary[index]
                soem_element = soem_object.subindices[sub_index]
                soem_element.get_data()
                cell.set_property("text", soem_element.value)
                cell.set_property("foreground", cell_colors[soem_element.valid])
                pass
            else:
                cell.set_property("text", "")

            return True

        col_cnt = view.insert_column_with_data_func(-1, "value", gtk.CellRendererText(), cb_data)
        column  = view.get_column(col_cnt - 1)
        column.set_sizing(gtk.TREE_VIEW_COLUMN_GROW_ONLY)
        column.set_expand(True)
        column.set_resizable(True)
        store.set_sort_column_id(0, 0)


    #HELPER
    def get_selected_device(self, widget):
        model, iter = widget.get_selection().get_selected()
        return model[iter][2] #device_id, device_name, pyobject


    #CALLBACKS
    def on_treeview_devices_cursor_changed(self, widget):
        dev = self.get_selected_device(widget)
        self.treestore_dictionary.clear()
        ids = dev.list_dictionary()
        map(lambda x: self.treestore_dictionary.insert(None, -1, [x, "", dev]), ids)        









