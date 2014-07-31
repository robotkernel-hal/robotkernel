#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback
import gtk  
import gobject
import yaml

from numpy import *
from pyutils.binary_packet import binary_packet 

import links_and_nodes as ln

import gui_utils
from modules import *
import sercos_wrapper

"""
typedef struct sercos_service_attribute {
    uint16_t conversionfactor;  
    unsigned datalength   : 3;  
    unsigned function     : 1;
    unsigned datatype     : 3;
    unsigned reserved1    : 1;
    unsigned decimalpoint : 4;
    unsigned wp_cp2       : 1;
    unsigned wp_cp3       : 1;
    unsigned wp_cp4       : 1;
    unsigned reserved2    : 1;
} sercos_service_attribute_t;
"""
		
		
packet_idattr = binary_packet((
	("conversionfactor", "H"),
	("reserved1", 1),
	("datatype", 3),
	("function", 1),
	("datalength", 3),
	("reserved2", 1),
	("wp_cp4", 1),
	("wp_cp3", 1),
	("wp_cp2", 1),
	("decimalpoint", 4),
	))
	
idattr_datatype = { 
	0: "Number", 
	1: "Unsigned Decimal", 
	2: "Signed Decimal", 
	3: "Unsigned Hex", 
	4: "Extcharset", 
	5: "Unsigned", 
	6: "Float", 
	7: "Reserved"
}

idattr_datalength = {
	0: "not available",
	1: "2-Byte-fix",
	2: "4-Byte-fix",
	4: "1-Byte-var",
	5: "2-Byte-var",
	6: "4-Byte-var" 
}

def idattr_get_datalength(attr):
    dl = (attr & 0x70000) >> 16
    if dl == 1:
        return 2
    return 4

def idattr_get_decimalpoint(attr):
    return (attr & 0xF000000) >> 24
   

class sercos_processdata_subview(object):
    def __init__(self, name, app, device_store):  
        self.name = name
        self.app = app
        self.clnt = self.app.clnt
        self.active_color = self.app.window.get_style().text[0].to_string()
        self.device_store = device_store
        self.views_created = False
        self.init_gui()

    def init_gui(self):
        filename = os.path.join(self.app.base_dir, 'resources/dual_list_vertical.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")
        return self.tab_main

        
    def create_views(self):
        self.liststore_act, self.treeview_act, self.act_columns = self.create_view(16) 
        self.sw_act = self.builder.get_object("top") #scrolledwindow
        self.sw_act.add(self.treeview_act)
            
        for d in self.device_store:
            self.liststore_act.append([d[2]]) #get from initial view, created by id_subview device_pyobject
            
        self.liststore_des, self.treeview_des, self.des_columns = self.create_view(24) 
        self.sw_des = self.builder.get_object("bottom") #scrolledwindow
        self.sw_des.add(self.treeview_des)    
        
        for d in self.device_store:
            self.liststore_des.append([d[2]]) #get from initial view, created by id_subview device_pyobject
        
        def update_view():
            self.treeview_act.queue_draw()
            self.treeview_des.queue_draw()
            return True
            
        gobject.timeout_add(1000, update_view)
        self.tab_main.show_all()
        self.views_created = True
        

    #CREATORS
    def create_view(self, columns_idn):
        store = gtk.ListStore(gobject.TYPE_PYOBJECT) # name, data
        view = gtk.TreeView(store) #set_model(store)
        view.set_border_width(4)
        view.set_headers_visible(True)
     
        cr = gtk.CellRendererText()   
        view.insert_column_with_data_func(-1, "Device", cr,  self.insert_id)
        view.insert_column_with_data_func(-1, "Name", cr,  self.insert_name)
        if columns_idn == 16:
            callback = self.parse_pdin
            view.insert_column_with_data_func(-1, "Status", cr, self.insert_statusword)
        if columns_idn == 24:
            callback = self.parse_pdout
            view.insert_column_with_data_func(-1, "Control", cr, self.insert_controlword)
        
        columns = dict()
        for d in self.device_store:
            dev = d[2]
            #dev.list_sercos_ids(self.app.id_view.id_view)
            dev_columns = dev.read_id(columns_idn, 0x8C)["value"]
            #dev.create_mapping()            
            if columns_idn == 16:
                dev.at_config = dev_columns
            if columns_idn == 24:
                dev.mdt_config = dev_columns
            for c in dev_columns:
                if c not in dev.sercos_ids:
                    dev.sercos_ids[c] = sercos_wrapper.sercos_id(dev, c, dev.view.id_view)
                if c in columns:
                    continue
                data = dev.read_id(c,0x04)
                dev.sercos_ids[c].name = data["name"].decode("cp437")
                columns[c] = dev.sercos_ids[c].name

        from string import maketrans   
        dictionary = maketrans("_", "-") # replace "_" by "-" symbols
        for idn in sorted(map(lambda x: int(x), columns.keys())):
            name = columns[idn].split("Parameter ")[-1]#.translate(dictionary)
            view.insert_column_with_data_func(idn, name, cr, callback)
            
        for i, col in enumerate(view.get_columns()):
            col.set_resizable(True)
            col.set_min_width(100) 
            col.set_sizing(gtk.TREE_VIEW_COLUMN_AUTOSIZE)
        return store, view, columns
            
        
    def insert_id(self, column, cell, store, iter):
        number = store[iter][0].name.rsplit("_", 1)[-1]
        cell.set_property('text', number)
        cell.set_property("foreground", "black")


    def insert_name(self, column, cell, store, iter):
        name = store[iter][0].device_name
        cell.set_property('text', name)
        cell.set_property("foreground", "black")

    def insert_statusword(self, column, cell, store, iter):
        value = ""
        dev = store[iter][0]  
        if dev.pdin is not None: # pdin not yet received from sercos (t < 1sec)
            value = "0x%04X" %(dev.pdin[1] << 8 | dev.pdin[0])
        cell.set_property('text', value)
        
    def insert_controlword(self, column, cell, store, iter):
        value = ""
        dev = store[iter][0]  
        if dev.pdout is not None: # pdout not yet received from sercos (t < 1sec)
            value = "0x%04X" %(dev.pdout[1] << 8 | dev.pdout[0])
        cell.set_property('text', value)

    def parse_pdin(self, column, cell, store, iter):
        value = "0.0"
        cell.set_property("foreground", "grey")
        dev = store[iter][0]       
        if dev.pdin is None: # pdin not yet received from sercos (t < 1sec)
            cell.set_property('text', value)
            return

        for idn, name in self.act_columns.items(): #get the correct index, as ordered in joints sercos data
            if column.get_title() == name:
                pd_index = dev.at_config.index(idn)
                break
        else:
            raise Exception("column title '%s' not matching any valid column" %column.get_title()) 

            
        if not dev.sercos_ids[idn].valid:  # data for idn not yet valid
            dev.sercos_ids[idn].get_data()
            cell.set_property('text', value)
            return
            
        if not dev.pdin_mapping:    
            if not dev.create_pd_mapping(dev.at_config):
                cell.set_property('text', value)
                return
            #print "%s pdin mapping complete" %dev.device_name
            dev.pdin_mapping = True  
            
        offset = dev.sercos_ids[idn].offset
        datalength = dev.sercos_ids[idn].datalength
        decimalpoint = dev.sercos_ids[idn].decimalpoint
        data = dev.pdin[offset:offset + datalength]
        if datalength == 4:
            value = int32(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * pow(10, decimalpoint * -1)
        else:
            value = int16(data[0] | (data[1] << 8)) * pow(10, decimalpoint * -1)
        cell.set_property('text', value)
        cell.set_property("foreground", "black")


    def parse_pdout(self, column, cell, store, iter):
        value = "0.0"
        cell.set_property("foreground", "grey")
        dev = store[iter][0]       
        if dev.pdout is None: # pdout not yet received from sercos (t < 1sec)
            cell.set_property('text', value)
            return

        for idn, name in self.des_columns.items(): #get the correct index, as ordered in joints sercos data
            if column.get_title() == name.split("Parameter ")[-1]:
                pd_index = dev.mdt_config.index(idn)
                break
        else:
            raise Exception("column title '%s' not matching any valid column" %column.get_title()) 

            
        if not dev.sercos_ids[idn].valid:  # data for idn not yet valid
            dev.sercos_ids[idn].get_data()
            cell.set_property('text', value)
            return
        
        if not dev.pdout_mapping:    
            if not dev.create_pd_mapping(dev.mdt_config):
                cell.set_property('text', value)
                return
            #print "%s pdout mapping complete" %dev.device_name
            dev.pdout_mapping = True
                       
        offset = dev.sercos_ids[idn].offset
        datalength = dev.sercos_ids[idn].datalength
        decimalpoint = dev.sercos_ids[idn].decimalpoint
        data = dev.pdout[offset:offset + datalength]
        if datalength == 4:
            value = int32(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * pow(10, decimalpoint * -1)
        else:
            value = int16(data[0] | (data[1] << 8)) * pow(10, decimalpoint * -1)
        cell.set_property('text', value)
        cell.set_property("foreground", "black")

        
        
