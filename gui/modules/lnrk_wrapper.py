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

class lnrk_wrapper(object):
    def __init__(self, clnt, module_name):
        self.clnt = clnt
        self.interface_name = module_name.split(".")[-1]
        
        self.subscribed_ports = dict()
        self.multi_waiter = ln.gtk_multi_waiter(clnt)
        
        self.svc_list                    = self.clnt.get_service("%s.list" % self.interface_name, "lnrkinterface/list")
        self.svc_request_telemetry_topic = self.clnt.get_service("%s.request_telemetry_topic" % self.interface_name, "lnrkinterface/request_telemetry_topic")
        self.svc_request_command_topic   = self.clnt.get_service("%s.request_command_topic" % self.interface_name, "lnrkinterface/request_command_topic")
        self.svc_add_overwrite           = self.clnt.get_service("%s.add_overwrite" % self.interface_name, "lnrkinterface/add_overwrite")
        self.svc_del_overwrite           = self.clnt.get_service("%s.del_overwrite" % self.interface_name, "lnrkinterface/del_overwrite")
        self.svc_list_overwrites         = self.clnt.get_service("%s.list_overwrites" % self.interface_name, "lnrkinterface/list_overwrites")
        
    def subscribe(self, topic_name, msg_def_name, rate, consumer_cb, consumer_args):
        if topic_name in self.subscribed_ports:
            port = self.subscribed_ports[topic_name]
        else:
            port = self.clnt.subscribe(topic_name, msg_def_name, rate)
            self.subscribed_ports[topic_name] = port
        if rate != ln.RATE_ON_REQUEST_LAST:
            self.multi_waiter.add_port(port, consumer_cb, *consumer_args)
        return port
        
    def unsubscribe(self, port):
        if not port.topic_name in self.subscribed_ports:
            return
        del self.subscribed_ports[port.topic_name]
        self.clnt.unsubscribe(port)

    def lnrk_list(self):
        svc = self.svc_list
        svc.call()
        manips = {}
        for m in svc.resp.manipulator_list:
            manips[m.name] = m.device_names.split(", ")
        devices = {}
        for d in svc.resp.device_list:
            devices[d.name] = d.dict()
        return manips, devices
        
    def lnrk_request_telemetry(self, devices, consumer, trigger_device=None, rate=-1, with_commands=0, consumer_cb=None, consumer_args=None):
        svc = self.svc_request_telemetry_topic
        svc.req.consumer = consumer
        svc.req.device_list = ", ".join(devices)
        if trigger_device is None:
            svc.req.trigger_device = ""
        else:
            svc.req.trigger_device = trigger_device
        svc.req.with_commands = with_commands
        svc.call()
        if svc.resp.error_message_len:
            raise Exception("error from interface: %s" % svc.resp.error_message)
        tr = svc.resp.topic_name, svc.resp.msg_def_name
        #print "topic, md", tr
        if consumer_args is None:
            consumer_args = tuple()
        return self.subscribe(svc.resp.topic_name, svc.resp.msg_def_name, rate, consumer_cb, consumer_args)    

    def lnrk_request_command(self, devices, commander):
        svc = self.svc_request_command_topic
        svc.req.commander = commander
        svc.req.device_list = ", ".join(devices)
        svc.call()
        if svc.resp.error_message_len:
            raise Exception(svc.resp.error_message)
        tr = svc.resp.topic_name, svc.resp.msg_def_name
        #print "topic, md", tr
        port = self.clnt.publish(svc.resp.topic_name, svc.resp.msg_def_name)
        return port

    def lnrk_add_overwrite(self, device, item, ovtype, **kwargs):
        svc = self.svc_add_overwrite
        svc.req.device = device
        svc.req.overwrites.append(
            svc.req.new_overwrite_packet(
                item=item,
                type=ovtype))
        svc.req.overwrites[-1].__dict__.update(kwargs)
        svc.call()
        if svc.resp.error_message_len:
            raise Exception(svc.resp.error_message)

    def lnrk_del_overwrite(self, device, item, ovtype, **kwargs):
        svc = self.svc_del_overwrite
        svc.req.device = device
        svc.req.overwrites.append(
            svc.req.new_overwrite_packet(
                item=item,
                type=ovtype))
        svc.req.overwrites[-1].__dict__.update(kwargs)
        svc.call()
        if svc.resp.error_message_len:
            raise Exception(svc.resp.error_message)

    def lnrk_list_overwrites(self, device):
        svc = self.svc_list_overwrites
        svc.req.device = device
        svc.call()
        if svc.resp.error_message_len:
            raise Exception(svc.resp.error_message)
        return svc.resp.overwrites

        
        
