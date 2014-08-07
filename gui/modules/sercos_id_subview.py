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

class sercos_id_subview(object):
    def __init__(self, name, app):   
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = self.app.window.get_style().text[0].to_string()
        self.init_gui()


    def init_gui(self):
        filename = os.path.join(self.app.base_dir, 'resources/dual_list.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")

        #device view at ids page
        self.device_store, self.device_view_ids = self.create_device_treeview() 
        self.sw_devices = self.builder.get_object("devices") #scrolledwindow
        self.sw_devices.add(self.device_view_ids)

        #treeview for ids
        self.id_store, self.id_view = self.create_id_treeview() 
        self.sw_ids = self.builder.get_object("values") #scrolledwindow
        self.sw_ids.add(self.id_view)
        self.sw_ids.set_policy(gtk.POLICY_ALWAYS, gtk.POLICY_AUTOMATIC)
        self.id_view.connect("key-press-event", self.on_keypress_id_view)
        self.id_view.connect("button-press-event", self.on_id_view_clicked)

        #initially fill all devices found by ln
        for s in self.clnt.find_services_with_interface("robotkernel/sercos_protocol/read_id").split("\n"):
            if not s.startswith(self.name):
                continue
            number = s.split(".")[-3]

            sercos_device = sercos_wrapper.sercos_device(self.app.clnt, "%s.%s" %(self.name, number), self)
            sercos_device.device_number = int(number.split("_")[-1])
            self.device_store.append( (sercos_device.device_number, sercos_device.device_name, sercos_device) )
            #sercos_device.list_sercos_ids(self.id_view) This takes forever...
        self.device_view_ids.connect("cursor-changed", self.on_device_view_ids_cursor_changed)

        #force the view initially to be loaded with dev 0
        if len(self.device_store):
            self.device_view_ids.set_cursor((0, ))
            self.update_id_view(None)
            
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


    def create_id_treeview(self):
        store = gtk.ListStore(int) # sercos id 
        view = gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)
        
       
        cr = gtk.CellRendererText()   
        view.insert_column_with_data_func(-1, "idn" , cr,  self.insert_sercos_id)
        view.insert_column_with_data_func(-1, "name" , cr,  self.insert_sercos_id)
        #indidual renderer for value, to catch signals etc.
        self.idvalue_renderer = gtk.CellRendererText()
        self.idvalue_renderer.connect("edited", self.edit_sercos_id_value)
        view.insert_column_with_data_func(-1, "value" , self.idvalue_renderer,  self.insert_sercos_id)
        
        store.set_sort_column_id(0, 0)
        for i, col in enumerate(view.get_columns()):
            col.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
            col.set_resizable(True)
            col.set_min_width(130 * (i + 1))   
            
        view.show_all()
        return store, view  


    #HELPER   
    def insert_sercos_id(self, column, cell, model, iter):
        #datafunc for id_view, works on all 3 collumns
        if not gui_utils.is_row_visible(model, iter, self.id_view):
            return True

        #column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
        #insert idn directly, it is known from click on device view
        row = model[iter] 
        if column.get_title() == "idn":
            idn = row[0]
            if idn & 0x8000:   
                cell.set_property("text", "P-0-%04d (%d)" %(idn - 0x8000, idn))
            else:
                cell.set_property("text", "S-0-%04d (%d)" %(idn, idn))
            return True
        
        #for name and value ask sercos_id object, it will call svc if vars are empty
        sercos_device = self.get_selected_device()
        sercos_id = sercos_device.sercos_ids[row[0]]
        idn, name, value, valid = sercos_id.get_data() #note that this call will reads from sercos bus if needed
        cell.set_property("text", locals()[column.get_title()])
        
        if not valid:
            cell.set_property("foreground", "grey")
        else:
            cell.set_property("foreground", self.active_color)
        return True


    def get_selected_device(self):
        model, iter = self.device_view_ids.get_selection().get_selected()
        if iter is None:
            return None
        return model[iter][2] #device_id, device_name, pyobject


    #CALLBACKS
    def on_device_view_ids_cursor_changed(self, widget):
        dev = self.get_selected_device()
        self.id_store.clear()
        map(lambda x: self.id_store.append((x, )), dev.list_sercos_ids(self.id_view))
        return True


    def on_id_view_clicked(self, widget, event):  
        #catch double cklick to update id view element
        sercos_device = self.get_selected_device()

        if event.button == 1 and event.type == gtk.gdk._2BUTTON_PRESS: 
            pthinfo = widget.get_path_at_pos(int(event.x), int(event.y))
            if pthinfo is None:
                return True
            clicked_id = pthinfo[0]
            idn = self.id_store[clicked_id][0] 
            idn, name, value, valid = sercos_device.sercos_ids[idn].get_data(force_update=True)
            return True
        return False
        
        
    def on_keypress_id_view(self, widget, event):
        #catch F5 for update treeview element
        key = gtk.gdk.keyval_name(event.keyval)
        if key in ["F5"]:
            self.update_id_view(widget)
        else:
            return False        
        return True

       
    def update_id_view(self, widget):
        #force update visible ids
        visible_range = self.id_view.get_visible_range()
        if visible_range == None:
            return False
        begin, end = visible_range
        
        sercos_device = self.get_selected_device()
        if sercos_device is None:
            return True        
        for row in range(begin[0], end[0] + 1):
            idn = self.id_store[row][0]
            sercos_device.sercos_ids[idn].valid = False
        self.id_view.queue_draw()
        return True

       
    def edit_sercos_id_value(self, cr, path, newvalue):
        #write to sercos after editing (confirm with ENTER)
        sercos_device = self.get_selected_device()
        idn = self.id_store[self.id_store.get_iter(path)][0]
        sercos_device.write_id(idn=idn, elements=0x80, value=newvalue)
        idn, name, value, valid = sercos_device.sercos_ids[idn].get_data(force_update=True)
        return True












