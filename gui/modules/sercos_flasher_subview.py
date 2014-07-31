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
from ihex import ihex

class sercos_flasher_subview(ln.ln_wrappers.services_wrapper):
    def __init__(self, parent_window, name, app):   
        super(sercos_flasher_subview, self).__init__(app.clnt, name.split('.', 1)[0])
        self.parent_window = parent_window
        self.name = name
        self.parent, self.module = name.split('.', 1)
        self.app = app
        self.clnt = self.app.clnt

        self.active_color = self.app.window.get_style().text[0].to_string()
        self.init_gui()
        
        self.wrap_service('get_state', 'robotkernel/get_state', 
                method_name = 'get_state')
        self.wrap_service('set_state', 'robotkernel/set_state', 
                method_name = 'set_state')
        self.wrap_service('%s.get_phase' % self.module, 'sercos/get_phase',
                method_name = 'get_phase')
        self.wrap_service('%s.set_phase' % self.module, 'sercos/set_phase',
                method_name = 'set_phase')
        self.wrap_service('%s.phase5_write' % self.module, 'sercos/phase5_write',
                method_name = 'phase5_write')
        self.wrap_service('%s.phase6_read' % self.module, 'sercos/phase6_read',
                method_name = 'phase6_read')

    def __getattr__(self, name):
        if self.have_xml:
            widget = self.xml.get_object(name)
            if widget is not None:
                setattr(self, name, widget)
                return widget
        raise AttributeError(name)
        
    def init_gui(self):
        filename = os.path.join(self.app.base_dir, 'resources/sercos_flasher.ui')
        self.have_xml = False
        self.xml = gtk.Builder() 
        self.xml.add_from_file(filename)
        self.have_xml = True
        self.tab_main = self.xml.get_object("main")

        self.flash_at_address = 1
        self.verify_flash = True
        self.checkbutton_verify_flash.set_active(self.verify_flash)
        self.spinbutton_at_address.set_adjustment(gtk.Adjustment(1, 1, 127, self.flash_at_address))
        self.xml.connect_signals(self)
        return self.tab_main

    def on_checkbutton_verify_flash_toggled(self, b):
        self.verify_flash = b.get_active()

    def on_spinbutton_at_address_value_changed(self, b):
        self.flash_at_address = int(self.spinbutton_at_address.get_value())

    def on_button_filechooser_read_flash_clicked(self, b):
        chooser = gtk.FileChooserDialog(
            'Save Flash as .. ', None,
            action=gtk.FILE_CHOOSER_ACTION_SAVE,
            buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,  gtk.STOCK_SAVE, gtk.RESPONSE_OK))
        chooser.set_filename(self.label_filechooser_read_flash.get_label())
        chooser.set_default_response(gtk.RESPONSE_OK)

        # add file filter for intel hex files
        filter = gtk.FileFilter()
        filter.set_name('Intel HEX files')
        filter.add_pattern('*.hex*')
        chooser.add_filter(filter)
        
        if chooser.run() == gtk.RESPONSE_OK:
            self.label_filechooser_read_flash.set_label(chooser.get_filename())
            
        chooser.destroy()
        
    def on_button_read_flash_clicked(self, b):
        filename = self.label_filechooser_read_flash.get_label()
        if filename == '(keine)':
            message = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
            message.set_markup("No filename given. Please select file to write first!")
            message.run()
            message.destroy()
            return

        #print self.get_state(mod_name = self.module)
        if self.get_state(mod_name = self.module) != 5:
            self.log('Switching module to boot (state = 5) ...')
            ret = self.set_state(mod_name=self.module, state=5)
            if ret == -1:
                self.log('Switching module to boot failed! See robotkernel output for more details.')
                return

        self.log('Switching sercos to phase 0 ...')
        ret = self.set_phase(phase=0, at_address=0)
        if ret == -1:
            self.log('Switching sercos to phase 0 failed! See robotkernel output for more details.')
            return

        self.log('Sercos phase 0 reached. Switching sercos to phase 6 drive %d' % self.flash_at_address)
        ret = self.set_phase(phase=6, at_address=self.flash_at_address)
        if ret == -1:
            self.log('Switching sercos to phase 6 for at address %d failed!' % self.flash_at_address)
            self.log('See robotkernel output for more details.')
            self.log('Maybe you have to switch the power off and on to reset the drive.')
            return

        self.log('Start reading Flash ...')

        intelhex = ihex()
        lower, upper = ( 0x410000, 0x430000 )
        diff = upper - lower
        adr = lower
        while adr < upper:
            fraction = (float(adr) - lower) / diff
            self.progress('Reading Flash ...', fraction)

            data = self.phase6_read(offset=adr, length=0)
            
            intelhex.put_data(adr, data)
            adr += len(data) * 2
        self.progress('done', 1.0)

        self.log('Flash reading done')
        intelhex.write_file(filename)
        self.log('HEX File written')
    
    def on_button_verify_flash_clicked(self, b):
        filename = self.filechooserbutton_flash.get_filename()
        self.do_verify_flash(filename)

    def do_verify_flash(self, filename):
        intelhex = ihex()
        try:
            intelhex.load_file(filename)    
        except:
            message = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
            message.set_markup("Opening HEX file failed!. Please select an other file to write/verify!")
            message.run()
            message.destroy()
            return
        
        self.log('Switching module to boot (state = 5) ...')
        ret = self.set_state(mod_name=self.module, state=5)
        if ret == -1:
            self.log('Switching module to boot failed! See robotkernel output for more details.')
            return

        self.log('Switching sercos to phase 0 ...')
        ret = self.set_phase(phase=0, at_address=0)
        if ret == -1:
            self.log('Switching sercos to phase 0 failed! See robotkernel output for more details.')
            return

        self.log('Sercos phase 0 reached. Switching sercos to phase 6 drive %d' % self.flash_at_address)
        ret = self.set_phase(phase=6, at_address = self.flash_at_address)
        if ret == -1:
            self.log('Switching sercos to phase 6 for at address %d failed!' % self.flash_at_address)
            self.log('See robotkernel output for more details.')
            self.log('Maybe you have to switch the power off and on to reset the drive.')
            return

        self.log('Start verifying Flash ...')

        lower, upper = intelhex.get_lower_upper()
        diff = upper - lower
        adr = lower
        while adr < upper:
            fraction = (float(adr) - lower) / diff
            self.progress('Verifying Flash ...', fraction)

            data = self.phase6_read(offset=adr, length=0)
            for i in range(0, len(data)):
                try: 
                    if intelhex.values[adr + (i * 2)] != data[i]:
                        self.log('mismatch adr 0x%X, set %04Xh, should be %04Xh' % (adr, data[i], intelhex.values[adr + (i * 2)]))
                except:
                    pass

            adr += len(data) * 2

        self.progress('done', 1.0)

        self.log('Verifying Flash done')

    def on_button_start_flash_clicked(self, b):
        filename = self.filechooserbutton_flash.get_filename()
        
        intelhex = ihex()
        try:
            intelhex.load_file(filename)    
        except:
            message = gtk.MessageDialog(type=gtk.MESSAGE_ERROR, buttons=gtk.BUTTONS_OK)
            message.set_markup("Opening HEX file failed!. Please select file an other file to write!")
            message.run()
            message.destroy()
            return

        self.log('Switching module to boot (state = 5) ...')
        ret = self.set_state(mod_name=self.module, state=5)
        if ret == -1:
            self.log('Switching module to boot failed! See robotkernel output for more details.')
            return

        self.log('Switching sercos to phase 0 ...')
        ret = self.set_phase(phase=0, at_address=0)
        if ret == -1:
            self.log('Switching sercos to phase 0 failed! See robotkernel output for more details.')
            return
        
        self.log('Sercos phase 0 reached. Switching sercos to phase 5 drive %d' % self.flash_at_address)
        ret = self.set_phase(phase=5, at_address=self.flash_at_address)
        if ret == -1:
            self.log('Switching sercos to phase 5 for at address %d failed!' % self.flash_at_address)
            self.log('See robotkernel output for more details.')
            self.log('Maybe you have to switch the power off and on to reset the drive.')
            return

        buflen = 256
        adrs = sorted(intelhex.values)

        self.log('Writing flash (at address %d, hexfile %s) ' % (self.flash_at_address, filename))

        lower, upper = intelhex.get_lower_upper()
        diff = upper - lower
        oldadr = adrs[0]-2
        while len(adrs) > 0:
            data = []
            
            for adr in adrs[:buflen/2]:
                if oldadr + 2 != adr:
                    self.log('warning %X %X' % (oldadr+2, adr))
                oldadr = adr
                data.append(intelhex.values[adr])
            while len(data) < buflen/2:
                data.append(0xFFFF)

            fraction = (float(adr) - lower) / diff
            self.progress('Writing Flash ...', fraction)

            adr = adrs[0]
            adrs = adrs[buflen/2:]
        
            self.phase5_write(offset=adr, length=len(data)*2, data=data, final=(len(adrs) <= 0)) 

        self.log('HEX file written!')
        
        if self.verify_flash:
            self.do_verify_flash(filename)

    def log(self, t):
        b = self.textview_log.get_buffer()
        i = b.get_end_iter()
        b.insert(i, '%s\n' % (t))
        i = b.get_end_iter()
        self.textview_log.scroll_to_iter(i, 0)
        self.textview_log.queue_draw()
        gtk.main_iteration(True)
    
    def progress(self, text, progress):
        self.parent_window.progressbar.set_fraction(progress)
        self.parent_window.label_action.set_text(text)
        gtk.main_iteration()

