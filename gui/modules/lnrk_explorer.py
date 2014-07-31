import gtk
import yaml
import traceback

import pango

from string import maketrans 
from numpy import *

import links_and_nodes as ln

import gui_utils
from modules import *
from lnrk_wrapper import lnrk_wrapper

def load(fn):
    import cPickle
    fp = file(fn, "rb")
    data = cPickle.load(fp)
    fp.close()
    return data
    
def save(fn, data):
    import cPickle
    fp = file(fn, "wb")
    cPickle.dump(data, fp)
    fp.close()

def concat(tuples):
    for t in tuples:
        for field in t:
            yield field

data_types = set([
    "int8_t", "uint8_t", "char",
    "int16_t", "uint16_t",
    "int32_t", "uint32_t",
    "int64_t", "uint64_t",
    "float", "double"])


unit_format = dict(
    rad=lambda v: "%+.3f[deg]" % (v / pi * 180),
    Nm="%+.2f[Nm]",
)

class state_interface(object):
    def __init__(self, lnrk_intf, lnrk_dev, name, states, active_overwrites=[], **kwargs):
        self.lnrk_intf = lnrk_intf,
        self.lnrk_dev = lnrk_dev
        self.measure = None
        self.command = None
        self.__dict__.update(kwargs)
        self.name = name
        self.states = states
        self.active_states = set()
        #print name
        #pprint.pprint(kwargs)
        if self.measure:
            if self.measure["type"] != "bitmask":
                raise Exception("don't know how to evaluate state interface %r of type %r" % (
                        self.name, self.measure["type"]))
            self.measure_item = self.measure["item"]
            self.measure_mask = self.measure["mask"]
            self.measure_values = self.measure["values"].items()

        if self.command:
            self.command_model = gtk.ListStore(gobject.TYPE_STRING)
            values = list(self.command["values"].items())
            values.sort(key=lambda v: v[1])
            self.command_iters = {}
            for name, value in values:
                self.command_iters[name] = self.command_model.append((name, ))

        self.overwrite = None
        self._check_active_overwrites(active_overwrites)

    def _check_active_overwrites(self, active_overwrites):
        self.overwrite_active = False
        if not (active_overwrites and self.command):
            return False
        item = self.command["item"]
        mask = self.command["mask"]
        # which overwrites are enabled?
        for ov in active_overwrites:
            print ov
            if ov.type != "bitmask":
                continue
            if ov.item != item:
                continue
            if ov.bitmask_mask != mask:
                continue
            print "have overwrite for cmd item %r, mask %r, value %r" % (ov.item, ov.bitmask_mask, ov.bitmask_value)
            # try to find command checkbox and combobox and set matching item!
            for state, value in self.command["values"].iteritems():
                if value == ov.bitmask_value:
                    print "matched state", state
                    self.overwrite = state
                    self.overwrite_active = True
        return self.overwrite_active

    def add_overwrite(self, state):
        item = self.command["item"]
        mask = self.command["mask"]
        value = self.command["values"][state]
        print (item, mask, value)
        self.lnrk_add_overwrite(self.lnrk_dev, item, "bitmask", bitmask_mask=mask, bitmask_value=value)

    def del_overwrite(self, state):
        item = self.command["item"]
        mask = self.command["mask"]
        self.lnrk_del_overwrite(self.lnrk_dev, item, "bitmask", bitmask_mask=mask)

    def do_measure(self, p):
        if not self.measure:
            return False
        changed = False
        #value = getattr(p.msr, self.measure_item)
        masked_value = getattr(p.msr, self.measure_item) & self.measure_mask
        #print "value: %x, mask: %x, %d" % (value, self.measure_mask, masked_value)
        had_state = False
        for state, state_value in self.measure_values:
            if masked_value != state_value:
                if state in self.active_states:
                    self.active_states.remove(state)
                    changed = True
                continue
            had_state = True
            if state not in self.active_states:
                self.active_states.add(state)
                changed = True
        if "default" in self.measure:
            default = self.measure["default"]
            if not had_state and not default in self.active_states:
                self.active_states.add(default)
                changed = True
            if had_state and default in self.active_states:
                self.active_states.remove(default)
                changed = True
        return changed

    def __repr__(self):
        return "<state %r with states: %r>" % (self.name, self.states)

    def __str__(self):
        return ", ".join(map(str, self.active_states))

    @staticmethod
    def data_func(col, cel, m, iter):
        cel.set_property("text", str(m[iter][1]))
        return True

    @staticmethod
    def overwrite_check_data_func(col, cel, m, iter):
        self = m[iter][1]
        if not self.command:
            cel.set_property("visible", False)
            return True
        cel.set_property("visible", True)
        cel.set_property("active", self.overwrite_active)
        cel.set_property("sensitive", True)
        return True
    @staticmethod
    def overwrite_active_toggled(cel, path, m):
        self = m[path][1]
        if not self.command:
            return False
        if not cel.get_active():
            # activate!
            if self.overwrite is None:
                # ignore!
                self.overwrite_active = False
                return True
            self.add_overwrite(self.overwrite)
            self.overwrite_active = True
            return True
        # deactivate!
        self.del_overwrite(self.overwrite)
        self.overwrite_active = False
        return True
        
    @staticmethod
    def overwrite_combo_data_func(col, cel, m, iter):
        self = m[iter][1]
        if not self.command:
            cel.set_property("visible", False)
            #cel.set_property("model", None)
            return True
        cel.set_property("visible", True)
        cel.set_property("model", self.command_model)
        cel.set_property("text-column", 0)
        cel.set_property("text", self.overwrite)
        return True
    @staticmethod
    def overwrite_combo_changed(combo, path, new_iter, m):
        self = m[path][1]
        if self.overwrite_active:
            if self.overwrite == self.command_model[new_iter][0]:
                return True
            old = self.overwrite
        else:
            old = None
        self.overwrite = self.command_model[new_iter][0]
        print "change self.overwrite to %r" % self.overwrite
        if self.overwrite_active:
            self.add_overwrite(self.overwrite) # same bitmask -> will replace old overwrite!
            #if old is not None:
            #    self.del_overwrite(old)
        return True

class lnrk_explorer(base_view, lnrk_wrapper):
    def __init__(self, name, app, view_id):   
        base_view.__init__(self, name, app, view_id)
        lnrk_wrapper.__init__(self, self.app.clnt, name)
        self.parent = name.split(".")[0]
        self.clnt = self.app.clnt
        self.init_gui()
        self.app.notebook.insert_page(self.tab_main, self.tab_header, self.view_id)
        self.tab_main.show_all()
        self.connect()        

        
    def __getattr__(self, name):
        widget = self.builder.get_object(name)
        if widget is not None:
            setattr(self, name, widget)
            return widget
        raise AttributeError(name)

    def init_gui(self):
        print "loading lnrk page %s" % self.name
        filename = os.path.join(self.app.base_dir, 'resources/lnrk_explorer.ui')
        self.builder = gtk.Builder() 
        self.builder.add_from_file(filename)
        self.tab_main = self.builder.get_object("main")

        #self.fn = "bitstream vera sans mono 9"
        #self.fn = "monospace 9"
        #self.fn = "Misc Fixed medium 10"
        #self.fd = pango.FontDescription(self.fn)

        m = self.manipulators_model = gtk.TreeStore(
            gobject.TYPE_STRING)
        tv = self.manipulators_tv
        tv.set_model(m)
        tv.insert_column_with_attributes(-1, "device", gtk.CellRendererText(), text=0)

        m = self.msr_model = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_STRING)
        tv = self.msr_tv
        #tv.modify_font(self.fd)
        tv.set_model(m)
        tv.insert_column_with_attributes(-1, "name", gtk.CellRendererText(), text=0)
        value_renderer = gtk.CellRendererText()
        #value_renderer.set_property("font", self.fd)
        tv.insert_column_with_attributes(-1, "value", value_renderer, text=1)

        m = self.states_model = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_PYOBJECT)
        tv = self.states_tv
        #tv.modify_font(self.fd)
        tv.set_model(m)
        tv.insert_column_with_attributes(-1, "name", gtk.CellRendererText(), text=0)
        tv.insert_column_with_data_func(-1, "value", gtk.CellRendererText(), state_interface.data_func)

        tvc = gtk.TreeViewColumn("overwrite")
        tcr = gtk.CellRendererToggle()
        tcr.connect("toggled", state_interface.overwrite_active_toggled, self.states_model)
        ccr = gtk.CellRendererCombo()
        ccr.set_property("has_entry", False)
        ccr.set_property('editable', True)
        ccr.set_property("text-column", 0)
        ccr.connect("changed", state_interface.overwrite_combo_changed, self.states_model)
        tvc.pack_start(tcr, False)
        tvc.set_cell_data_func(tcr, state_interface.overwrite_check_data_func)
        tvc.pack_start(ccr, True)
        tvc.set_cell_data_func(ccr, state_interface.overwrite_combo_data_func)
        tv.insert_column(tvc, -1)

        self.current_manipulator = None
        self.current_device = None

        self.subscriber_port = None
        self.subscribed_device = None
        self.subscribed_manip = None
        self.hide_subscriber = None
        
        self.builder.connect_signals(self)

    def show(self):
        if self.hide_subscriber is not None:
            self.start_subscriber(self.hide_subscriber)
        return super(lnrk_explorer, self).show()

    def hide(self):
        self.hide_subscriber = self.subscribed_device
        self.stop_subscriber()
        return super(lnrk_explorer, self).hide()

    def stop(self):
        self.stop_subscriber()
        return super(lnrk_explorer, self).stop()

    def connect(self):
        print "connect to %s" %self.name
        # retrieve devices
        m = self.manipulators_model
        m.clear()
        manips, devices = self.lnrk_list()
        manip_keys = manips.keys()
        manip_keys.sort()
        all_devs = set(devices.keys())
        for ma in manip_keys:
            parent = m.append(None, (ma, ))
            for dev in manips[ma]:
                m.append(parent, (dev, ))
                if dev in all_devs:
                    all_devs.remove(dev)
        if all_devs:
            others = []
            manips["other devices"] = others
            parent = m.append(None, ("other devices", ))
            all_devs = list(all_devs)
            all_devs.sort()
            for dev in all_devs:
                others.append(dev)
                m.append(parent, (dev, ))
        self.manipulators = manips
        self.devices = devices
        self.hpaned.set_property("sensitive", True)

    def disconnect(self):
        self.stop_subscriber()
        self.hpaned.set_property("sensitive", False)
        
    def on_manip_telemetry(self, port):
        p = port.read(False)
        m = self.msr_all_model
        for col, (dev, fields, overwrites) in enumerate(self.packet_fields):
            d = getattr(p, dev)
            for field_name, short, row_iter, format in fields:
                if short == "state":
                    state = format
                    state.do_measure(d)
                    if field_name in overwrites:
                        m[row_iter][col + 1] = '<span color="red">%s</span>' % state
                    else:
                        m[row_iter][col + 1] = str(state)
                else:
                    value = getattr(getattr(d, short), field_name)
                    if type(format) in (str, unicode):
                        m[row_iter][col + 1] = format % (value,)
                    else:
                        m[row_iter][col + 1] = format(value)
        return True

    def start_manip_subscriber(self, name):
        if self.subscribed_manip == name:
            return
        self.stop_subscriber()
        
        devs = self.manipulators[self.current_manipulator]
        self._devs = devs
        tv = self.msr_all_tv
        tv.set_model(None)
        for col in tv.get_columns():
            tv.remove_column(col)
        m = self.msr_all_model = gtk.ListStore(gobject.TYPE_STRING, *([gobject.TYPE_STRING]*len(devs)))
        #tv.modify_font(self.fd)
        tv.insert_column_with_attributes(-1, "name", gtk.CellRendererText(), text=0)
        value_renderer = gtk.CellRendererText()
        #value_renderer.set_property("font", self.fd)
        defaults = [""] * (1 + len(devs))
        device_classes = []
        field_names = {}
        has_states = {}
        packet_fields = []
        for i, dev in enumerate(devs):
            dev_name = dev
            dev = dev.replace("_", "__")
            tv.insert_column_with_attributes(-1, dev, value_renderer, markup=1 + i)

            dev_data = self.devices[dev_name]
            if "pyconfig" not in dev_data:
                dev_data["pyconfig"] = yaml.load(dev_data["config"])
                dev_data["pyclass_config"] = yaml.load(dev_data["class_config"])
            config = dev_data["pyconfig"]["config"]
            class_config = dev_data["pyclass_config"]
            device_class = config["device class"]
            if device_class not in device_classes:
                device_classes.append(device_class)
                fns = field_names[device_class] = []
                for msrcmd, short in ("measurements", "msr"), ("commands", "cmd"):
                    for field in class_config.get(msrcmd, []):
                        data_type = set(field.keys()).intersection(data_types).pop()
                        field_name = field[data_type]
                        defaults[0] = "%s.%s.%s" % (device_class, short, field_name)
                        iter = m.append(defaults)

                        if "string_format" in field:
                            format = field["string_format"]
                        elif "unit" in field:
                            format = unit_format.get(field["unit"], "%s")
                        else:
                            format = "%s"
                        fns.append((field_name, short, iter, format))
                if "interfaces" in dev_data["pyconfig"] and "interfaces" in class_config:
                    # states
                    short = "state"
                    interface_classes = dev_data["pyconfig"]["interfaces"]
                    interfaces = class_config["interfaces"]
                    has_states[device_class] = []
                    for intf in interfaces:
                        interface_name, params = intf.items()[0]
                        interface = interface_classes.get(interface_name)
                        if interface is None:
                            continue
                        interface_type = interface.get("type")
                        if interface_type is None:
                            continue
                        if interface_type != "state":
                            # not a state
                            continue
                        states = interface.get("states")
                        if not states:
                            #print "no states defined in interface class %r" % interface_name
                            continue
                        try:
                            i = state_interface(self.interface_name, dev, interface_name, states, active_overwrites=[], **params)
                        except:
                            print traceback.format_exc()
                            continue
                        defaults[0] = "%s.%s.%s" % (device_class, short, interface_name)
                        iter = m.append(defaults)
                        fns.append((defaults[0], short, iter, i))
                        has_states[device_class].append((defaults[0], i))
            overwrites_active = set()
            if device_class in has_states and has_states[device_class]:
                # is there an overwrite active?
                overwrites = self.lnrk_list_overwrites(dev_name)
                for field_name, state in has_states[device_class]:
                    if state._check_active_overwrites(overwrites):
                        overwrites_active.add(field_name)
            packet_fields.append((dev_name, field_names[device_class], overwrites_active))
        self.packet_fields = packet_fields
        self.subscriber_port = self.lnrk_request_telemetry(
            devs, 
            "lnrk explorer", 
            rate=10, with_commands=1,
            consumer_cb=self.on_manip_telemetry)
        
        tv.set_model(m)
        
    def on_manipulators_tv_cursor_changed(self, tv):
        path, col = tv.get_cursor()
        m = self.manipulators_model
        name = m[path][0]
        b = self.config_tv.get_buffer()
        if len(path) == 1:
            # manipulator
            self.stop_subscriber()
            self.current_manipulator = name
            self.current_device = None

            devs = self.manipulators[self.current_manipulator]
            b.set_text("devices:\n  %s" % 
                       "\n  ".join(devs))
            #self.dev_notebook.set_current_page(0)
            if self.dev_notebook.get_current_page() == 1:
                self.start_manip_subscriber(name)

            self.msr_hpaned.hide()
            self.msr_all_sw.show()
        else:
            # device
            dev = self.devices[name]

            self.current_manipulator = None
            self.current_device = dev

            self.msr_hpaned.show()
            self.msr_all_sw.hide()

            b.set_text("device: %r\n\nconfig:\n  %s\n\nclass_config:\n  %s" % (
                    name,
                    dev["config"].replace("\n", "\n    "),
                    dev["class_config"].replace("\n", "\n    ")))
            
            if "pyconfig" not in dev:
                dev["pyconfig"] = yaml.load(dev["config"])
                dev["pyclass_config"] = yaml.load(dev["class_config"])
            
            if self.dev_notebook.get_current_page() == 1:
                self.start_subscriber(name)
        return True

    def on_dev_notebook_switch_page(self, nb, ptr, page):
        if page == 0:
            self.stop_subscriber()
        elif page == 1 and self.current_device:
            self.start_subscriber(self.current_device["name"])
        elif page == 1 and self.current_manipulator:
            self.start_manip_subscriber(self.current_manipulator)
        return True

    def stop_subscriber(self):
        if self.subscriber_port is not None:
            clnt = self.clnt
            self.unsubscribe(self.subscriber_port)
            self.subscriber_port = None
            self.subscribed_device = None
            self.subscribed_manip = None

    def on_telemetry(self, port):
        p = port.read(False)
        if p is None:
            print "on_telemetry: no new packet!?"
            return True
        m = self.states_model
        iter = m.get_iter_root()
        while iter:
            state = m[iter][1]
            if state.do_measure(self._part):
                m.row_changed(m.get_path(iter), iter)
            iter = m.iter_next(iter)
        m = self.msr_model
        o = 0
        for msrcmd in self._part._fields:
            part = getattr(self._part, msrcmd)
            for i, field in enumerate(part._fields):
                v = getattr(part, field)
                iter = self.msr_iters[o + i]
                format = self.msr_formats[o + i]
                if type(format) in (str, unicode):
                    m[iter][1] = format % (v,)
                else:
                    m[iter][1] = format(v)
                #m[iter][1] = "%s %s" % (field, m[iter][1])
            if len(part._fields):
                o += i + 1
        return True
    def start_subscriber(self, dev):
        try:
            if self.subscribed_device == dev:
                return
            self.stop_subscriber()
            m = self.msr_model
            m.clear()
            self.subscriber_port = self.lnrk_request_telemetry(
                [dev], 
                "lnrk explorer", 
                rate=10, with_commands=1,
                consumer_cb=self.on_telemetry)
            self.subscribed_device = dev
            p = self.subscriber_port.packet.dict()[dev]
            self._part = p
            self.msr_iters = []
            self.msr_formats = []
            for msrcmd in p._fields:
                N = len(p._fields)
                
                if "msr" in msrcmd:
                    msrs = self.devices[dev]["pyclass_config"].get("measurements", [])
                else:
                    if "commands" not in self.devices[dev]["pyclass_config"]:
                        # no commands for this device
                        continue
                    msrs = self.devices[dev]["pyclass_config"]["commands"]
                for i, name in enumerate(getattr(p, msrcmd)._fields):
                    self.msr_iters.append(m.append((name, "")))
                    msr = msrs[i]
                    if "string_format" in msr:
                        self.msr_formats.append(msr["string_format"])
                    elif "unit" in msr:
                        self.msr_formats.append(unit_format.get(msr["unit"], "%s"))
                    else:
                        self.msr_formats.append("%s")

            m = self.states_model
            m.clear()
            dev_data = self.devices[dev]
            if "interfaces" not in dev_data["pyconfig"] or "interfaces" not in dev_data["pyclass_config"]:
                return
            interface_classes = dev_data["pyconfig"]["interfaces"]
            interfaces = dev_data["pyclass_config"]["interfaces"]
            # collect state interfaces!
            overwrites = self.lnrk_list_overwrites(dev)
            for intf in interfaces:
                interface_name, params = intf.items()[0]
                interface = interface_classes.get(interface_name)
                if interface is None:
                    print "unknown interface class %r" % interface_name
                    continue
                interface_type = interface.get("type")
                if interface_type is None:
                    print "unknown interface type %r" % interface_type
                    continue
                if interface_type != "state":
                    # not a state
                    continue
                states = interface.get("states")
                if not states:
                    print "no states defined in interface class %r" % interface_name
                    continue
                try:
                    i = state_interface(self.interface_name, dev, interface_name, states, active_overwrites=overwrites, **params)
                except:
                    print traceback.format_exc()
                    continue
                m.append((interface_name, i))
                    
        except:
            print traceback.format_exc()



