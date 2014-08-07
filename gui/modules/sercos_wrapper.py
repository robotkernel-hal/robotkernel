#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import copy
import time
import links_and_nodes as ln
import math

import gobject
import traceback

import yaml
import gui_utils


# 0x02 = structure
# 0x04 = name
# 0x08 = attr
# 0x10 = unit
# 0x20 = min_value
# 0x40 = max_value
# 0x80 = value
# 0xFE = alles

def decode(string):
    if len(string):
        string = string.decode("cp437").strip()
    return string

def decode_eval(string):
    if len(string):
        string = eval(string.decode("cp437"))
    return string

def idattr_get_datalength(attr):
    dl = (attr & 0x70000) >> 16
    if dl == 1:
        return 2
    return 4

def idattr_get_decimalpoint(attr):
    return (attr & 0xF000000) >> 24
    

class sercos_id(object):
    def __init__(self, device, idn, tree_view=None):
        self.tree_view = tree_view
        self.sercos_device = device
        self.idn = idn
        self.name = "---"
        self.value = 0.0
        self.attr = None
        self.decimalpoint = None
        self.datalength = None
        self.offset = None
        self.fd_get_data = None
        self.valid = False

    def yaml(self):
        return dict(idn=self.idn, name=self.name.encode("utf-8"), value=self.value)

    def update_callback(self, force_update=False):
        self.get_data(force_update)
        return False
    
    
    def get_data(self, force_update=False):
        # get data if nesecary from sercos bus,
        if not self.valid or self.name is "---" or self.value is 0.0 or self.attr is None or force_update:
            if not self.sercos_device.svc_call_pending:
                self.read_id(0x8C)
            else: 
                if self.fd_get_data:
                    gobject.source_remove(self.fd_get_data)
                self.fd_get_data = gobject.timeout_add(10, self.update_callback, (force_update))                
        
        return (self.idn, self.name, self.value, self.valid)                 
      
      
    def read_id(self, elements):
        #non-blocking read on data, with callback (see get_data)
        self.sercos_device.svc_call_pending = True
        self.sercos_device.async_read_id.req.idn = self.idn
        self.sercos_device.async_read_id.req.elements = elements
        self.sercos_device.async_read_id.call_async()
        self.sercos_device.async_read_id.gobject_on_async_finish(self.on_response, time.time())
               
                                             
    def on_response(self, starttime): 
        #callback after sercos returned with data     
        try:                          
            elements = self.sercos_device.async_read_id.req.elements
            data = self.sercos_device.async_read_id.resp
            self.attr = data.attr
            self.name = decode(data.name)
            self.value = decode_eval(data.value)
            self.valid = True
            self.sercos_device.svc_call_pending = False
            #print "got name/value of idn %s after %.2f seconds" %(self.idn, time.time() - starttime)                
            #print "idn %d, name %s, value %r" % (self.idn, self.name, self.value)  

            #force redraw, but this will only do so if still visible
            #self.sercos_device.view.id_store[self.tree_iter] = (self.idn, self.name, self.value)
            if self.tree_view is not None:
                self.tree_view.queue_draw()
        except:
            print traceback.format_exc()
        return False


class sercos_parameterset(object):
    def __init__(self, device, set_number, set_name, tree_view):
        self.tree_view = tree_view
        self.sercos_device = device
        self.number = set_number
        self.name = set_name
        self.parameters = []
        
        self.valid_set = [False] * 10
        self.fd_get_data = None
    
        self.m_base = 33034
        self.e_base = 33053

        if not hasattr(self.sercos_device, str(self.m_base)) or not hasattr(self.sercos_device, str(self.e_base)):
            try:
                self.sercos_device.list_sercos_ids(self.sercos_device.view.id_view)
            except:
                raise Exception("self.sercos_device.view.id_view not found for selected parameterset")
    
    def yaml(self):  
        data = dict(
                    set=self.name, 
                    parameters=map(lambda p: {p[0]:p[1]}, self.parameters)
        )
        return data
         
    
    def update_callback(self, force_update):
        self.get_parameters(force_update)
        return False
    
    def call_callback(self, force_update):
        if self.fd_get_data:
            gobject.source_remove(self.fd_get_data)
        self.fd_get_data = gobject.timeout_add(10, self.update_callback, force_update)
        return False
    
    def select_parameterset(self):
        self.sercos_device.write_id(idn=217, elements=0x80, value=self.number)
        self.sercos_device.set_command(216)
        for i in xrange(10):
            self.sercos_device.sercos_ids[self.m_base + i].valid = False
            self.sercos_device.sercos_ids[self.e_base + i].valid = False
        self.valid_set = [False] * 10
    
    def get_parameters(self, force_update=False):
        #everything already loaded
        if all(self.valid_set) and not force_update:
            return True
        
        #another parameter is currently updating
        if self.sercos_device.parameter_update_pending not in [-1, self.number]:
            return self.call_callback(force_update)
           
        #update this parameterset
        if self.sercos_device.parameter_update_pending != self.number:
            self.select_parameterset()
        self.sercos_device.parameter_update_pending = self.number
        
        #iterate over all parameters in the set and check if already valid, if not get it from pyobject
        for i in xrange(10):
            if self.valid_set[i]:
                continue
            # get mantisse and exponent from sercos bus, or from sercos_id pyobj if already available
            sercos_id_mantisse = self.sercos_device.sercos_ids[self.m_base + i]
            sercos_id_exponent = self.sercos_device.sercos_ids[self.e_base + i]
            idn, name, mantisse, valid_mantisse = sercos_id_mantisse.get_data() 
            idn, name, exponent, valid_exponent = sercos_id_exponent.get_data()   
            
            #if finally valid data for mantisse and exponent, print it in the tree view
            if valid_mantisse and valid_exponent:
                self.valid_set[i] = True
                self.parameters[i][1] = mantisse * pow(10, exponent)
                self.tree_view.queue_draw()
                #iter = self.sercos_device.view.param_store.get_iter((self.number, i))
                #self.sercos_device.view.param_store[iter][2] = self.parameters[i][1]

        # not yet all data valid, call again in 10ms
        if not all(self.valid_set):
            return self.call_callback(force_update)
        #finished, make update availyble for other parametersets
        self.sercos_device.parameter_update_pending = -1
        return True 

    def set_parameter(self, parameter_number, newvalue, select_set=True, enable_set=True):  
        #sets a single parameter, but updates the complete set afterwards                      
        if select_set:
            self.sercos_device.write_id(idn=217, elements=0x80, value=self.number)
            self.sercos_device.set_command(216)
        mantisse, exponent = self.frexp10(newvalue)
        self.sercos_device.write_id(self.m_base + parameter_number, elements=0x80, value=mantisse)
        self.sercos_device.write_id(self.e_base + parameter_number, elements=0x80, value=exponent)
        if enable_set:
            self.sercos_device.set_command(264)
        return True
            
            
    def frexp10(self, value):
        #rational: from robotkernel lbrserver.cpp by robert burger
        man = float(value)
        exp = 0
        res = 1.0
        frac, integer = math.modf(man)
        
        while frac != 0.0 and exp > -6:
            exp -= 1
            man *= 10.0
            frac, integer = math.modf(man)

        while man > sys.maxint:
            exp += 1
            man /= 10.0;
        return int(man), exp
      
        
class sercos_device(ln.ln_wrappers.services_wrapper):
    def __init__(self, clnt, module_name, view):
        super(sercos_device, self).__init__(clnt, module_name)
        self.view = view
        self.name = module_name
        self.sercos_ids = dict()
        self.idn_n = 0
        self.sercos_parametersets = dict()
        self.svc_call_pending = False
        self.parameter_update_pending = -1
        self.diagnosis = -1
        self.pdin = None
        self.pdout = None
        self.status_word = -1
        self.commands = []
        self.mdt_config = []
        self.at_config = []
        self.pdin_mapping = False
        self.pdout_mapping = False
        
        self.wrap_service(
            "sercos_protocol.read_id", "robotkernel/sercos_protocol/read_id",
            postprocessors=dict(
                name=decode,
                value=decode_eval
            ),
            method_name="read_id")

        self.wrap_service(
            "sercos_protocol.write_id", "robotkernel/sercos_protocol/write_id",
            default_arguments=dict(state=0, structure=0, name="", unit="", attr=0, min_value="", max_value=""),
            method_name="write_id")

        self.wrap_service(
            "sercos_protocol.set_command", "robotkernel/sercos_protocol/set_command",
            method_name="set_command")

        self.wrap_service(
            "get_config", "robotkernel/module/get_config")
            
        self.svc_get_pdin = self.clnt.get_service("%s.process_data_inspection.in" %self.name)
        self.svc_get_pdout = self.clnt.get_service("%s.process_data_inspection.out" %self.name)
        self.async_read_id = self.clnt.get_service("%s.sercos_protocol.read_id" %self.name)
        
        self.device_name = self.read_id(30, 0x80)["value"]

        def pd_update():
            self.pdout = self.get_pdout()
            self.pdin = self.get_pdin()
            return True
                        
        gobject.timeout_add(1000, pd_update)
        

    def create_pd_mapping(self, config_list):  
        #this function is used both to create a mapping for pdin and pdout
        #the config_list parameter given is either equal to self.mdt_config or self.at_config
        offset = 2
        for id in config_list:
            if not self.sercos_ids[id].valid:
                return False
            attr = self.sercos_ids[id].attr
            #print id, attr, self.sercos_ids[id].valid
            datalength = idattr_get_datalength(attr)
            decimalpoint = idattr_get_decimalpoint(attr)
            setattr(self.sercos_ids[id], "decimalpoint", decimalpoint)
            setattr(self.sercos_ids[id], "datalength", datalength)
            setattr(self.sercos_ids[id], "offset", offset)
            #print id, offset, datalength
            offset = offset + datalength
        return True
        
    def get_pdin(self):
        try:
            self.svc_get_pdin.call_gobject()
        except:
            pass
        return self.svc_get_pdin.resp.data
        
    def get_pdout(self):
        try: 
            self.svc_get_pdout.call_gobject()
        except:
            pass
        return self.svc_get_pdout.resp.data


    def list_sercos_ids(self, associated_tree_view):
        sercos_ids = None
        if self.idn_n is 0:    
            sercos_ids = self.read_id(17, 0x8C)["value"] 
            self.idn_n = len(sercos_ids)
        if len(self.sercos_ids) != self.idn_n: #only uppon first call
            if sercos_ids is None:
                sercos_ids = self.read_id(17, 0x8C)["value"]
            for tree_iter, id in enumerate(sercos_ids):
                if not id in self.sercos_ids: #if not already loaded by processdata subview
                    self.sercos_ids[id] = sercos_id(self, id, associated_tree_view)
        return self.sercos_ids.keys()
            

    def list_sercos_parametersets(self, associated_tree_view):
        if not len(self.sercos_parametersets):
            filename = os.path.join(gui_utils.app_base_dir, 'resources/parameter.yaml')
            fd = open(filename, "r") 
            data = yaml.load(fd)
            fd.close()
            for i, parameter_set in enumerate(data):
                name = parameter_set["set"]
                self.sercos_parametersets[i] = sercos_parameterset(self, i, name, associated_tree_view)
                for j, param in enumerate(parameter_set["parameters"]):
                    self.sercos_parametersets[i].parameters.append([param.keys()[0], 0.0])
        return self.sercos_parametersets.keys()
          

    #dump/save/load functions

    def backup_ids(self, fn):
        #force data to update
        for idn, sercos_id in self.sercos_ids.items():
            sercos_id.get_data(force_update=True)

        #save, but only when all data is up to date
        def save_data():      
            for idn, sercos_id in self.sercos_ids.items():
                if not sercos_id.valid:
                    print ".", #not yet all ids red
                    sys.stdout.flush()
                    return True


            data = sorted( map(lambda x: x.yaml(), self.sercos_ids.values()) )
            yaml_str = yaml.dump(data=data, allow_unicode=True, default_flow_style=False)#, width=32000)
            #yaml_str = yaml_str.replace(', {', '\n-  {').strip("[]") #add line brakes, and strip brackets
            #yaml_str = "-  %s" %yaml_str #insert first listing
            fd = open(fn, "w")
            fd.write(yaml_str)
            fd.close()
            print "\ndumped ids to %s" %fn
            return False
        gobject.timeout_add(500, save_data)
        

    def load_ids(self, fn):
        #get data from yamlfile
        fd = open(fn, "r")
        data = yaml.load(fd)
        fd.close()
        for d in data:
            #do not write empty strings to sercos
            if type(d["value"]) is str and len(d["value"].strip()) == 0:
                continue
            #write to sercos and invalidate for update
            self.write_id(idn=d["idn"], elements=0x80, value=d["value"])
            print "writing id %d = %s" %(d["idn"], d["value"])
            self.sercos_ids[d["idn"]].valid = False
        

    def backup_parametersets(self, fn):
        for set_number, sercos_parameterset in self.sercos_parametersets.items():
            sercos_parameterset.get_parameters(force_update=True)

        def save_data():
            for set_number, sercos_parameterset in self.sercos_parametersets.items():
                if not all(sercos_parameterset.valid_set):
                    print ".",
                    sys.stdout.flush()
                    return True

            fd = open(fn, "w")
            data = map(lambda x: x.yaml(), self.sercos_parametersets.values())
            yaml.dump(data, fd, default_flow_style=False)
            fd.close()
            print "\ndumped parameters to %s" %fn
            return False
        gobject.timeout_add(500, save_data)


    def load_parametersets(self, fn):
        #get data from yamlfile
        fd = open(fn, "r")
        data = yaml.load(fd)
        fd.close()
        for i, d in enumerate(data):
            #preselcet parameterset and invalidate current data in gui
            parameter_set = self.sercos_parametersets[i]
            parameter_set.select_parameterset() 
            for j, p in enumerate(d["parameters"]):
                name = p.keys()[0]
                print "writing paramter %d %d = %s" %(i, j, p[name])
                parameter_set.set_parameter(j, p[name], False, False)
            self.set_command(264) #load set as writen in single sercos_ids at bus
        
        
