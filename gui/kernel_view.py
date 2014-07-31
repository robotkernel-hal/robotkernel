#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback
import yaml

import gtk  
import gobject

from numpy import *
from numpy.linalg import *

import links_and_nodes as ln

import gui_utils
from modules import *
from kernel_wrapper import * 

class kernel_view(base_view, kernel_wrapper):
    def __init__(self, name, app, to_open):   
        base_view.__init__(self, name, app, 0)
        kernel_wrapper.__init__(self, self.app.clnt, self.name)

        filename = os.path.join(app.base_dir, 'resources/dual_list_kernel.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)        
        self.builder.connect_signals(self)
        self.ui = gui_utils.builder_ui(self.builder)
        self.tab_main = self.builder.get_object("main")

        view_id = self.app.notebook.append_page(self.tab_main, self.tab_header)
        self.app.notebook.set_current_page(view_id) #show tab imidiatly, otherwiese to_open tabs appear before main tab
        self.init_gui(to_open)
        self.tab_main.show_all()
        self.ui.dual_list_contents.update()

    def init_gui(self, to_open):
        name = self.name
        self.sourceview = gui_utils.setup_sourceview("python", "Monospace 9")
        self.sourceview.set_editable(True)
        self.sourceview.set_highlight_current_line(True)
        self.sourceview.set_show_line_marks(False)
        self.sourceview.set_show_line_numbers(True)
        self.sourceview.set_show_right_margin(False)
        sw = self.builder.get_object("values") #scrolledwindow
        sw.add(self.sourceview)

        self.module_store, self.module_view = self.create_module_treeview() 
        sw = self.builder.get_object("devices") #scrolledwindow
        sw.add(self.module_view)       

        for module, state in self.get_states("*"):
            self.module_store.insert(-1, (module, state))
            if "%s.%s" % (self.name, module) in to_open:
                self.show_view(module)

        #module popup menu
        self.module_popup = gtk.Menu()    # Don't need to show menus
        for s in states[2:]:
            menu = gtk.MenuItem(s)
            self.module_popup.append(menu)
            menu.connect("activate", self.on_popup_clicked, s)
            menu.show()

        #self.module_view.connect("key-press-event", self.on_keypress_module_view)
        self.module_view.connect("button-press-event", self.on_module_view_clicked)
        self.module_view.connect("cursor-changed", self.on_module_activated)

        # interfaces treeview
        self.first_interfaces = True
        tv = self.ui.interfaces_tv
        m = self.interfaces_model = gtk.TreeStore(gobject.TYPE_STRING)
        tv.insert_column_with_attributes(-1, "interface", gtk.CellRendererText(), text=0)
        tv.set_model(m)

        self.ui.new_show_group("dual_list_contents")
        self.ui.dual_list_contents.add("values")

        self.interface_guis = dict(
            key_value=interface_key_value(self, self.ui.hbox3, self.ui.dual_list_contents),
        )        

        self.timer = gobject.timeout_add(1000, self.update_states)
        self.update_states()

    #CREATORS
    def create_module_treeview(self):
        store = gtk.ListStore(str, gobject.TYPE_PYOBJECT) # name, object struct
        view = gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)
        
        # populate store
        def module_view_str(column, cell, store, iter, i):
            module = store.get_value(iter,i)    
            cell.set_property('text', module)
            column.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
            return
        
        cr = gtk.CellRendererText()   
        view.insert_column_with_data_func(-1, "module" , cr,  module_view_str, 0)
        view.insert_column_with_data_func(-1, "state" , cr,  module_view_str, 1)
        store.set_sort_column_id(0, 0)        
        
        return store, view   
        
    #CALLBACKS   
    def update_states(self):
        try:
            found_states = self.get_states("*")
        except:
            print "can not get states from rk %r: %s" % (self.name, sys.exc_value)
            return True
        for i, (module, state) in enumerate(found_states):
            self.module_store[i] = module, state

        self.update_interfaces()
        return True

    def update_interfaces(self):
        # todo: do this globally for all rk's only once!
        found_interfaces = []

        # search for key_value interfaces
        for interface_name, gui in self.interface_guis.iteritems():
            interfaces = self.app.clnt.find_services_with_interface("robotkernel/%s/%s" % (interface_name, gui.detect_service))
            if interfaces:
                for intf in interfaces.split("\n"):
                    parts = intf.rsplit(".")
                    rk_name, module_name, device_name = parts[:3]
                    if rk_name != self.name:
                        continue
                    found_interfaces.append(
                        (interface_name, module_name, device_name))
        
        # ...

        # now update tree model
        found_interfaces.sort()
        m = self.interfaces_model
        def sync_tree(parent, levels_left, level=0, found_iter=0):
            if levels_left == 0:
                found_iter += 1
                return found_iter
            iter = m.iter_children(parent)
            #print "  " * level, "sync_tree"
            # append / insert new interfaces
            if level == 0:
                this_parent = None
            else:
                this_parent = found_interfaces[found_iter][level - 1]
            while True:
                if found_iter >= len(found_interfaces):
                    break # delete rest of tree-iter interfaces
                if level != 0 and this_parent != found_interfaces[found_iter][level - 1]:
                    break # parent changed
                this_data = found_interfaces[found_iter][level]
                #print "  " * level, "data: %r" % this_data
                if not iter or not m.iter_is_valid(iter):
                    # append new interface!
                    intf_iter = m.append(parent, (this_data, ))
                elif m[iter][0] > this_data:
                    # insert before!
                    intf_iter = m.insert_before(parent, iter, (this_data, ))
                    iter = m.iter_next(iter)
                elif m[iter][0] == this_data:
                    # already exists!
                    intf_iter = iter
                    iter = m.iter_next(iter)
                else:
                    # delete old iter!
                    m.remove(iter)
                    continue
                found_iter = sync_tree(intf_iter, levels_left - 1, level+1, found_iter)
            # delete old unseen interfaces
            while iter and m.iter_is_valid(iter):
                m.remove(iter)
            return found_iter
        sync_tree(None, 3)
        if self.first_interfaces:
            self.ui.interfaces_tv.expand_all()
            self.first_interfaces = False

    def show_view(self, module_name):
        cfg = yaml.load(self.get_config(module_name))
        module_type = cfg["module_file"]
        self.app.load_tab(module_name, module_type, self.name)

    def on_button_add_module_clicked(self, widget):
        print widget

    def on_button_remove_module_clicked(self, widget):
        model, iter = self.module_view.get_selection().get_selected()
        if iter is None:
            return False      
        module = model[iter][0] 
        self.remove_module(name=module)

    def on_module_view_clicked(self, widget, event):  
        x = int(event.x)
        y = int(event.y)
        t0 = event.time
        pthinfo = widget.get_path_at_pos(x, y)
        if pthinfo is None:
            return True
        clicked_module = pthinfo[0]
        col = pthinfo[1]          
  
        widget.grab_focus()
        widget.set_cursor( clicked_module, col, 0)
        self.on_module_activated(widget)
        if event.button == 1 and event.type == gtk.gdk._2BUTTON_PRESS:  
            module_name = self.module_store[clicked_module][0]
            self.show_view(module_name)
        elif event.button == 3:
            self.module_popup.popup(None, None, None, event.button, t0)
        return True
    
    def on_module_activated(self, widget):
        model, iter = widget.get_selection().get_selected()
        if iter is None:
            return False 
        module = model[iter][0]
        config = self.get_config(module)
        self.sourceview.get_buffer().set_text(config)
        self.ui.dual_list_contents.show("values")
        return True
 
    def on_keypress_module_view_todo(self, widget, event):
        model, iter = widget.get_selection().get_selected()
        if iter is None:
            return False      
        module = model[iter][0] 
        state = states.index(model[iter][1])

        key = gtk.gdk.keyval_name(event.keyval)
        if key in ["1", "2", "3", "4", "5", "KP_1", "KP_2", "KP_3", "KP_4", "KP_5"]:
            ret_state = self.set_state(module, int(key[-1]))
        elif key in ["Left", "minus", "KP_Subtract"] and state != 1:
            ret_state = self.set_state(module, state - 1)
        elif key in ["Right", "plus", "KP_Add"] and state != 5:
            ret_state = self.set_state(module, state + 1)
        elif key == "F5":
            ret_state = self.reconfigure(module)
        else:
            return False        
        model[iter][1] = states[ret_state]
        return True

    def on_popup_clicked(self, widget, state):
        model, iter = self.module_view.get_selection().get_selected()
        if iter is None:
            return False 
        module = model[iter][0]
        ret_state = self.set_state(module, state)
        model[iter][1] = ret_state
        self.update_states()
        return True

    def close_tab(self, widget, event):
        #cleanup timers, callbacks, fds, ...
        gobject.source_remove(self.timer)
        self.timer = None
        self.app.close_tab(widget, event, self.name)
        return True
        
    def on_interfaces_tv_cursor_changed(self, tv):
        path, col = tv.get_cursor()
        if len(path) < 3:
            return True
        row = [self.interfaces_model[path[:i]][0] for i in xrange(1, 4)]
        interface, module, device = row
        if interface in self.interface_guis:
            self.interface_guis[interface].show(module, device)
        return True
