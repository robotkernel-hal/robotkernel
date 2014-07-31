#!/usr/bin/python

import links_and_nodes as ln

class rk_interface(ln.ln_wrappers.services_wrapper):
    def __init__(self, parent, detect_service, show_group, show_group_member):
        self.parent = parent
        self.app = self.parent.app
        self.detect_service = detect_service
        self.show_group = show_group, show_group_member
        ln.ln_wrappers.services_wrapper.__init__(self, self.app.clnt, self.parent.name)
        
    def show(self):
        self.show_group[0].show(self.show_group[1])
