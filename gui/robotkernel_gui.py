#!/usr/bin/env python
# -*- encoding: utf-8 -*-
import os
import sys
import traceback

import gtk  

import links_and_nodes as ln

import gui_utils
import kernel_view 
from modules import *

class robotkernel_gui(object):
    def __init__(self, show, hide, argv):
        self.base_dir = gui_utils.app_base_dir
        self.version = self.get_version()
        self.clnt = ln.client("robotkernel_gui", argv)

        self.show = show.split(", ")
        self.hide = hide.split(", ")

        self.views = dict()
        filename = os.path.join(self.base_dir, 'resources/main.ui')

        self.main = gtk.Builder() 
        self.main.add_from_file(filename)
        
        self.main.connect_signals(self)

        self.notebook = self.main.get_object("notebook")
        self.window = self.main.get_object("robotkernel_gui")
        self.window.show_all()
        
        self.init_gui()


    def get_version(self):
        pt_file_path = "../"
        if self.base_dir.startswith("/volume/software"):
            pt_file_path = "../../"
        path = os.path.join(self.base_dir, pt_file_path, 'robotkernel.pt')
        try:
            f = open(path, "r")
            version = f.read().split("\n")[2].split("=")[-1]
            f.close()
        except:
            version = "X.X.X"
        return version 


    def init_gui(self):
        kernels = self.clnt.find_services_with_interface("robotkernel/module_list").split("\n")
        for k in kernels:
            if not len(k.strip()) and len(kernels) == 1:
                print "no kernels avialable, please refresh view when you are ready"
                break
            name = k.split(".")[0]
            self.views[name] = kernel_view.kernel_view(name, self, self.show)
            if name in self.hide:
                self.close_tab(None, None, name)
                
                
    def load_tab(self, module_name, module_type, parent_name):
        name = "%s.%s" %(parent_name, module_name)
        if name not in self.views:  #create by clicking in the robotkernel_gui
            view_id = self.notebook.get_current_page() + 1
            if module_type.endswith("sercos.so"):
                module = sercos_view(name, self, view_id)
            elif module_type.endswith("soem.so"):
                module = soem_view(name, self, view_id)
            elif module_type.endswith("lnrkinterface.so"):
                module = lnrk_explorer(name, self, view_id)
            elif module_type.endswith("jitter_measurement.so"):
                module = jitter_view(name, self, view_id)
            else:
                module = base_view(name, self, view_id)
                module.init_gui()
            self.views[name] = module
        view_id = self.views[name].view_id
        self.notebook.set_current_page(view_id)


    #CALLBACKS
    def close_tab(self, widget, event, name):
        tab = self.views[name].tab_main
        view_id = self.notebook.page_num(tab)
        self.notebook.remove_page(view_id)
        del self.views[name]
        return True


    def on_refresh(self, widget):
        for name, tab in self.views.items():
            tab.close_tab(widget, None)
        self.views = dict()    
        self.init_gui()
        return True
     
        
    def on_about(self, widget):
        d = gtk.AboutDialog()
        d.set_program_name("Robotkernel GUI")
        d.set_comments("With Great Power Comes Great Responsibility")
        d.set_copyright("DLR RM")
        d.set_website("https://rmintra01.robotic.dlr.de/wiki/Robotkernel")
        d.set_website_label("Robotkernel %s" %self.version)
        d.set_authors(["Robert Burger: Robotkernel & Modules", "Florian Schmidt: Modules", "Daniel Leidner: GUI"])
        d.set_logo(gtk.gdk.pixbuf_new_from_file(os.path.join(self.base_dir, "resources/icon.png")))
        d.run()
        d.destroy()
        return True


    def on_robotkernel_gui_destroy(self, object, data=None):
        print "stoping Robotkernel GUI"
        gtk.main_quit()


usage = """robotkernel_gui.py 
-show 'kernel1.sercos_ring1, kernel1.sercos_ring2, kernel2.ethercat'
-ln_manager host:port
"""

if __name__ == "__main__":
    show = ""
    hide = ""
    if len(sys.argv) >= 2:
        if sys.argv[1] == "-h" or sys.argv[1] == "--help":
            print usage
            exit()
    if len(sys.argv) >= 3:   
        for i, arg in enumerate(sys.argv):
            if arg.startswith("-show"):
                show = sys.argv[i+1]
            if arg.startswith("-hide"):
                hide = sys.argv[i+1]
    
    gui = robotkernel_gui(show, hide, sys.argv)
    print "at your service!"
    gtk.main()






