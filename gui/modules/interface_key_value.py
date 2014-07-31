#!/usr/bin/python

import os
import sys
import gtk
import gobject
import pprint
import math
import traceback
import gui_utils
import yaml

from rk_interface import *

class interface_key_value(rk_interface, gui_utils.builder_ui):
    def __init__(self, parent, container, show_group):        
        gui_utils.builder_ui.__init__(self, filename="interface_key_value.ui")
        container.pack_start(self.key_value_box, True, True)
        self.key_value_box.hide()
        show_group.add(self.key_value_box)
        rk_interface.__init__(self, parent, "list", show_group, self.key_value_box)

        self.tv = tv = self.key_value_tv
        
        # key_value treeview
        m = self.model = gtk.ListStore(
            gobject.TYPE_STRING, # key display
            gobject.TYPE_STRING, # name
            gobject.TYPE_STRING, # value / repr
            gobject.TYPE_PYOBJECT # real key
        )
        col = tv.insert_column_with_attributes(-1, "key", gtk.CellRendererText(), text=0)
        col.set_property("resizable", True)
        col = tv.insert_column_with_attributes(-1, "name", gtk.CellRendererText(), text=1)
        col.set_property("resizable", True)
        cr = gtk.CellRendererText()
        cr.set_property("editable", True)
        cr.connect("edited", self.on_edited)
        n = tv.insert_column_with_data_func(-1, "value", cr, self.on_key_value_data)
        #col = tv.get_column(n - 1)
        
        tv.set_model(m)
        tv.connect("row-activated", self.on_key_value_tv_row_activated)
        tv.set_search_equal_func(func=self.search_func)

        self.kv_refresh_btn.connect("clicked", self.on_refresh)

        self.current_device = None

        self._display_formats = ["decimal", "hex_02", "hex_04", "hex_08", "repr"]
        self._display_byte_order = ["native", "16", "32"]
        self.default_display_options = "decimal", "native"
        self._display_options_fn = os.path.expanduser("~/.robotkernel/gui/key_value_display_options")
        self._format_strings = {}
        self._load_display_options()
        self.key_names = {}

        self._read_queue = {}
        self._read_queue_id = None
    
       
    def show(self, module, device):
        rk_interface.show(self)
        if self.current_device != (module, device):
            self.current_device = module, device
            if module not in self.display_options:
                self.display_options[module] = {}
            if device not in self.display_options[module]:
                self.display_options[module][device] = {}
            self.current_display_options = self.display_options[module][device]
            self._show_names()

    def _show_names(self):
        if self.current_device not in self.key_names:
            self._update_key_names()
        key_format = "0x%%0%dx" % self.n_hex_digits
        self.model.clear()
        for key, name in self.key_names[self.current_device]:
            self.model.append((key_format % key, name, None, key))

    def _update_key_names(self):
        method_name = "%s_%s_list" % self.current_device
        if not hasattr(self, method_name):
            self.wrap_service(
                "%s.%s.key_value.list" % self.current_device,
                "robotkernel/key_value/list",
                method_name=method_name, call_method="call_gobject")
        ret = getattr(self, method_name)()
        key_names = self.key_names[self.current_device] = []
        self.n_hex_digits = 2
        for key, name in zip(ret["keys"], ret["names"]):
            key_names.append(
                (int(key), name.value))
            #print "got key %x: %s" % (key, name.value)
            key_hex = "%x" % key
            if len(key_hex) > self.n_hex_digits:
                self.n_hex_digits = len(key_hex)
        self.n_hex_digits = int(math.ceil(self.n_hex_digits / 2.)) * 2
        print "have %d key_names" % len(key_names)

    def on_key_value_data(self, col, cr, m, iter):
        row = m[iter]
        display = row[2]
        if display is None:
            ret = self.tv.get_visible_range()
            if ret is not None:
                start_path, end_path = ret
                path = m.get_path(iter)
                if path >= start_path and path <= end_path:
                    self._queue_read(m[iter][3], path)
            display = ""
        elif display != "":
            key = row[3]
            format, byte_order = self.current_display_options.get(key, self.default_display_options)
            if format == "repr":
                display = display
            else:
                # optionally iterate over sequences
                format_string = self._format_strings.get(format)
                if format_string is None:
                    if format[0].startswith("hex"):
                        if format[1]:
                            f = "0x"
                        else:
                            f = ""
                        f += "%" + format[0].split("_", 1)[1] + "x"
                    else:
                        f = "%d"
                    format_string = self._format_strings[format] = f
                value = eval(display)
                if type(value) == str:
                    display = repr(value)
                else:
                    if value in (None, ):
                        display = str(value)
                    elif type(value) in (list, tuple):
                        display = " ".join([format_string % v for v in value])
                    else:
                        try:
                            display = format_string % value
                        except:
                            print (format_string, value)
                            raise
            # todo byte_order
        cr.set_property("text", display)
        return True

    def _queue_read(self, key, key_iter, timeout=250):
        self._read_queue[key] = key_iter
        if self._read_queue_id is None:
            self._read_queue_id = gobject.timeout_add(timeout, self._process_read_queue)
        
    def _process_read_queue(self):
        to_read = self._read_queue
        self._read_queue = {}

        if to_read:
            self._update_key_values(to_read)

        if not self._read_queue:
            gobject.source_remove(self._read_queue_id)
            self._read_queue_id = None
            return False
        return True
        
    def _update_key_values(self, to_read):
        #print "update values for %2d keys: %s" % (len(to_read), to_read.keys())
        method_name = "%s_%s_read" % self.current_device
        if not hasattr(self, method_name):
            self.wrap_service(
                "%s.%s.key_value.read" % self.current_device,
                "robotkernel/key_value/read",
                method_name=method_name, call_method="call_gobject")
        keys = list(to_read.keys())
        values = getattr(self, method_name)(keys=keys)
        for key, value in zip(keys, values):
            path = to_read[key]
            iter = self.model.get_iter(path)
            self.model[iter][2] = value.value
            self.model.row_changed(path, iter)
        
    def search_func(self, m, col, key, iter):
        if key.lower() in m[iter][1].lower():
            return False
        if m[iter][0].startswith(key):
            return False
        if ("0x%x" % m[iter][3]).startswith(key):
            return False
        try:
            key_int = int(key)
            if key_int == m[iter][3]:
                return False
        except:
            pass
        if ("%x" % m[iter][3]).startswith(key):
            return False
        return True
            
    def on_key_value_tv_row_activated(self, tv, path, col):
        self.model[path][2] = ""
        self.model.row_changed(path, self.model.get_iter(path))        
        self._queue_read(self.model[path][3], path, 50)
        return True
        
    def on_refresh(self, btn):
        if self.current_device is None:
            return
        if self.current_device in self.key_names:
            del self.key_names[self.current_device]
        self._show_names()
        
    def on_edited(self, cr, path, value):
        row = self.model[path]
        key = row[3]
        
        format, byte_order = self.current_display_options.get(key, self.default_display_options)

        try:
            if format[0].startswith("hex"):
                if not format[1]:
                    value = eval("0x%s" % value)
                else:
                    value = eval(value)
            else:
                value = eval(value)
        except:
            print "warning: invalid input format: %r" % value
            pass
        
        row[2] = None
        self.model.row_changed(path, self.model.get_iter(path))        
        # now write, then read!
        print "write 0x%x to %r" % (key, value)
        #try:
        self._write(key, value)
        #except:
        #    pass
        self._queue_read(self.model[path][3], path, 50)
        return True

    def _write(self, key, value):
        method_name = "%s_%s_write" % self.current_device
        if not hasattr(self, method_name):
            svc = self.wrap_service(
                "%s.%s.key_value.write" % self.current_device,
                "robotkernel/key_value/write",
                method_name=method_name, call_method="call_gobject")
            self.write_new_string_packet = svc.svc.req.new_string_packet
        p = self.write_new_string_packet()
        p.value = repr(value)
        try:
            getattr(self, method_name)(keys=[key], values=[p])
        except:
            self.show_exception(text='error writing value %r to key 0x%x:\n<span weight="bold" font_family="monospace">%s</span>' % (value, key, sys.exc_value))
        
    def show_exception(self, text=None, reraise=True):
        traceback.print_exc()
        
        dlg = gtk.MessageDialog(
            self.app.window,
            gtk.DIALOG_DESTROY_WITH_PARENT,
            gtk.MESSAGE_ERROR,
            buttons=gtk.BUTTONS_OK)
        
        if text is None:
            stack = traceback.format_exc().split("\n")
            stack = "\n".join(stack[2:])
            stack = stack.replace("<", "&lt;").replace(">", "&gt;")
            text = 'exception in\n<span weight="bold" font_family="monospace">%s</span>' % (stack, )

        dlg.label.set_line_wrap(False)
        dlg.set_markup(text)
        dlg.connect("delete-event", lambda w: w.destroy())
        dlg.connect("response", lambda w, rid: w.destroy())
        dlg.show()
        raise

    def on_key_value_tv_button_press_event(self, tv, ev):
        if ev.button != 3:
            return False
        ret = tv.get_path_at_pos(int(ev.x), int(ev.y))
        if ret is None:
            print "no path"
            return False
        path, tvc, x, y = ret
        tv.set_cursor(path, tvc)
        key = self.model[path][3]
        format, byte_order = self.current_display_options.get(key, self.default_display_options)
        def set_active_group_item(prefix, value, items):
            for item in items:
                i = getattr(self, "%s_%s" % (prefix, item))
                i.set_active(item == value)
        if format[0].startswith("hex"):
            self.format_hex_with_0x.set_active(format[1])
            format = format[0]
        set_active_group_item("format", format, self._display_formats)
        set_active_group_item("byteorder", byte_order, self._display_byte_order)
        self.kv_popup.popup(None, None, None, ev.button, ev.time)
        return True
    
    def kv_popup_done(self, *args):
        def get_active_group_item(prefix, items):
            for item in items:
                i = getattr(self, "%s_%s" % (prefix, item))
                if i.get_active():
                    return item
        format = get_active_group_item("format", self._display_formats)
        if format.startswith("hex"):
            format = format, self.format_hex_with_0x.get_active()
        byte_order = get_active_group_item("byteorder", self._display_byte_order)
        path, col = self.tv.get_cursor()
        key = self.model[path][3]
        #print (key, self.current_device, format, byte_order)
        if self.current_display_options.get(key, self.default_display_options) != (format, byte_order):
            self.current_display_options[key] = format, byte_order
            self._schedule_save_display_options()
            self.model.row_changed(path, self.model.get_iter(path))        

    def _load_display_options(self):
        if not os.path.isfile(self._display_options_fn):
            self.display_options = {}
            return
        fp = file(self._display_options_fn, "rb")
        self.yaml_loader = yaml.loader.Loader
        def tuple_constructor(loader, node):
            return tuple(loader.construct_sequence(node))
        self.yaml_loader.add_constructor(u'tag:yaml.org,2002:seq', tuple_constructor)
        self.display_options = yaml.load(fp, self.yaml_loader)
        if type(self.display_options) != dict:
            self.display_options = {}
        fp.close()
            
    def _schedule_save_display_options(self):
        if hasattr(self, "_schedule_save_display_options_id"):
            gobject.source_remove(self._schedule_save_display_options_id)
            del self._schedule_save_display_options_id
        def save_options():
            del self._schedule_save_display_options_id
            dn = os.path.dirname(self._display_options_fn)
            if not os.path.isdir(dn):
                os.makedirs(dn)
            fp = file(self._display_options_fn, "wb")
            self.yaml_dumper = yaml.dumper.Dumper
            def tuple_representer(dumper, data):
                return dumper.represent_list(data)
            self.yaml_dumper.add_representer(tuple, tuple_representer)
            try:
                yaml.dump(self.display_options, fp, self.yaml_dumper)
            except:
                pprint.pprint(self.display_options)
                raise
            fp.close()
            return False

        self._schedule_save_display_options_id = gobject.timeout_add(1000, save_options)
