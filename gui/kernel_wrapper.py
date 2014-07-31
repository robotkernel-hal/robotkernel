import os

import links_and_nodes as ln

states = ["unknown", "error", "boot", "init", "preop", "safeop", "op"]

class kernel_wrapper(ln.ln_wrappers.services_wrapper):
    def __init__(self, clnt, robotkernel_name):
        self.name = robotkernel_name
        super(kernel_wrapper, self).__init__(clnt, robotkernel_name)

        self.config_services = dict()

        self.wrap_service(
            "get_states", "robotkernel/get_states",
            postprocessor=self._zip_states)

        self.wrap_service(
            "get_state", "robotkernel/get_state")

        self.wrap_service(
            "set_state", "robotkernel/set_state")

        self.wrap_service(
            "module_list", "robotkernel/module_list",
            postprocessor=self._evaluate_string_list)

        self.wrap_service(
                "remove_module", "robotkernel/remove_module")

    def _zip_states(self, ret):
        return zip(ret["mod_names"].split(", "), ret["states"].split(", "))

    def _evaluate_string_list(self, ret):
        return eval(ret["modules"])

    def get_config(self, module):
        svc_name = "%s.%s.get_config" %(self.name, module)
        if svc_name not in self.config_services:
            svc = self.clnt.get_service(svc_name, "module/get_config")
            self.config_services[svc_name] = svc
        svc = self.config_services[svc_name]
        svc.call()
        return svc.resp.config
