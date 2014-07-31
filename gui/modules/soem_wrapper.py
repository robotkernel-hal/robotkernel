import os
import copy
import time
import links_and_nodes as ln

import gobject
import traceback

import yaml

class soem_element(object):
    def __init__(self, device, index, sub_index):
        self.soem_device = device
        self.index = index
        self.sub_index = sub_index
        self.name = None
        self.value = None
        self.fd_get_data = None
        self.valid = False

    def update_callback(self, force_update=False):
        self.get_data(force_update)
        return False
    
    def get_data(self, force_update=False):
        # get data if nesecary from soem bus,
        # this will be called twice by the treeview to get name and value seperated
        if not self.valid or self.name is None or self.value is None:
            if not self.soem_device.svc_call_pending:
                self.read()
            else: 
                if self.fd_get_data:
                    gobject.source_remove(self.fd_get_data)
                self.fd_get_data = gobject.timeout_add(10, self.update_callback, (force_update))
        
        return (self.name, self.value, self.valid)
      
    def read(self):
        def cb_read(starttime): 
            #callback after soem returned with data     
            try:                           
                data = self.soem_device.async_read_element.resp
                self.name = data.name.decode('latin1')
                self.value = data.value
                self.valid = True
                self.soem_device.svc_call_pending = False
                self.soem_device.view.treeview_dictionary.queue_draw()
            except:
                print traceback.format_exc()
            return False

        #non-blocking read on data, with callback (see get_data)
        self.soem_device.svc_call_pending = True
        self.soem_device.async_read_element.req.index = self.index
        self.soem_device.async_read_element.req.sub_index = self.sub_index
        self.soem_device.async_read_element.call_async()
        self.soem_device.async_read_element.gobject_on_async_finish(cb_read, time.time())

class soem_object(object):
    def __init__(self, device, idn):
        self.soem_device = device
        self.idn = idn
        self.name = None
        self.value = None
        self.max_subindices = 0
        self.fd_get_data = None
        self.valid = False
        self.subindices = { }

    def update_callback(self, force_update=False):
        self.get_data(force_update)
        return False
    
    def get_data(self, force_update=False):
        # get data if nesecary from soem bus,
        # this will be called twice by the treeview to get name and value seperated
        if not self.valid or self.name is None or self.value is None:
            if not self.soem_device.svc_call_pending:
                self.read()
            else: 
                if self.fd_get_data:
                    gobject.source_remove(self.fd_get_data)
                self.fd_get_data = gobject.timeout_add(10, self.update_callback, (force_update))
        
        return (self.idn, self.name, self.value, self.valid)
      
    def read(self):
        def cb_read(starttime): 
            #callback after soem returned with data     
            try:                           
                data = self.soem_device.async_read_object.resp
                self.name = data.name.decode('latin1')
                self.max_subindices = data.max_subindices
                self.subindices.clear()
                for i in range(0, self.max_subindices+1):
                    self.subindices[i] = soem_element(self.soem_device, self.idn, i)
                self.value = 0
                self.valid = True
                self.soem_device.svc_call_pending = False
                self.soem_device.view.treeview_dictionary.queue_draw()
            except:
                print traceback.format_exc()
            return False

        #non-blocking read on data, with callback (see get_data)
        self.soem_device.svc_call_pending = True
        self.soem_device.async_read_object.req.index = self.idn
        self.soem_device.async_read_object.call_async()
        self.soem_device.async_read_object.gobject_on_async_finish(cb_read, time.time())

class soem_device(object):
    def __init__(self, clnt, module_name, view):
        self.clnt = clnt
        self.view = view
        self.name = module_name
        self.soem_dictionary = dict()
        self.svc_call_pending = False

        self.async_object_dictionary_list = self.clnt.get_service(
                "%s.canopen_protocol.object_dictionary_list" % self.name)
        self.async_read_object = self.clnt.get_service(
                "%s.canopen_protocol.read_object" % self.name)
        self.async_read_element = self.clnt.get_service(
                "%s.canopen_protocol.read_element" % self.name)

    def read_object(self, index):
        self.async_read_object.req.index = index
        self.async_read_object.call()
        return self.async_read_object.resp

    def read_element(self, index, sub_index):
        self.async_read_element.req.index = index
        self.async_read_element.req.sub_index = sub_index
        self.async_read_element.call()
        return self.async_read_element.resp

    def _evaluate_string_list(self, ret):
        return eval(ret["modules"])

    def list_dictionary(self):
        if not len(self.soem_dictionary): #only uppon first call
            self.async_object_dictionary_list.call()
            soem_dictionary = self.async_object_dictionary_list.resp.indices
            for iter, idn in enumerate(soem_dictionary):
                self.soem_dictionary[idn] = soem_object(self, idn)
        return self.soem_dictionary.keys()

    def list_processdata(self, ids):
        for id in ids:
            yield self.soem_ids[id].get_data() 
        
