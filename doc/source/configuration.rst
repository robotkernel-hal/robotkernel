=============
Configuration
=============

Robotkernel configuration files are written in YAML. Usually to compose
a new robotic system we have

- Human-readable configuration files for system setup
- Multiple configuration layers
    - Main robotkernel
    - Individual components
- Module configuration typically defines
    - Hardware access parameters (e.g., baudrate, cyclic data)
    - Hardware initialization routines
    - Scheduling priorities and CPU affinity
- Module interconnections and synchronization
- Explicit specification of module dependencies

Example Config file
___________________

A simple configuration file for robotkernel may look like this:

.. code-block:: yaml

  # Configuration file for robotkernel.
  #
  # vi: set ft=yaml nowrap:
  # -*- mode: yaml -*-
  
  #########################################################
  # general settings
  
  # Robotkernel's own instance name.
  name: example_rk
  
  # On robotkernel exit modules were unloaded on destruction.
  #do_not_unload_modules: false
  
  #########################################################
  # logging settings
  
  # Standard robotkernel loglevel.
  #loglevel: verbose
  
  # Limit of module name in log message output in characters.
  #log_fix_modname_length: 40
  
  # Log all messages to ftrace linux kernel infrastructure. This
  # needs some additional permissions or root access.
  #log_to_trace_fd: false
  
  # Log all messages to lttng. This needs a running lttng session
  # daemon and maybe some additional permission.
  #log_to_lttng_ust: false

  # Don't defer prints for logging to a seperate thread. Enabling
  # this can be dangerous with hard-realtime.
  #sync_logging: false
  
  # Enable dumping logs to robotkernel service. This specifies the 
  # maximum amount of internal log message buffer in bytes which 
  # can be retreaved via service.
  #max_dump_log_len: 0

  #########################################################
  # service bridges
  #
  # These bridges export all internal registered rk-5 services
  # to external processes.
  bridges:
  - name: example_cli
    so_file: libbridge_cli.so
  
  #########################################################
  # service providers
  #
  # The service providers are creating unified rk-5 services
  # to gain access to common hardware data structures.
  service_providers:
  - name: pd_inspection
    so_file: libservice_provider_process_data_inspection.so
  
  #########################################################
  # modules
  #
  modules:
  - name: timer
    so_file: libmodule_posix_timer.so
    config: !include timer.rkc
    power_up: op
  
  - name: jitter
    so_file: libmodule_jitter_measurement.so
    config: !include jitter.rkc
    depends: [ timer, ]
    power_up: op


Robotkernel general and logging settings
________________________________________

**name**
  Specifies robotkernel's own instance name.

**do_not_unload_modules**
  Don't do cleanup on exit. Deprecated, was used on VxWorks to 
  avoid crashes on stop. 

  Default is *false*.

**loglevel**
  Specify robotkernel's global logging level. Must be one of
  *error*, *warning*, *info* or *verbose*. 
  
  Default is *info*.

**log_fix_modname_length**
  Specify how many chanracters are printed from module_name|instance_name
  to determine the log message source. 

  Default is 40 chars.

**log_to_trace_fd**
  Enable logging to ftrace tracing point. All log messages are 
  printed there and visible when using ftrace. e.g. ftrace_tools.
  
  Default is *false*.

**log_to_lttng_ust**
  Enable logging to lttng tracing point. All log messages are
  printed to a specific lttng channel which is loggable with lttng.

  Default is *false*.

  This needs a running lttng session daemon and maybe some additional
  permissions.

**sync_logging**
  Don't defer prints for logging to a seperate thread. Enabling
  this can be dangerous with hard-realtime.

  Default is *false*.

**max_dump_log_len**
  Enable dumping logs to robotkernel service. This specifies the 
  maximum amount of internal log message buffer in bytes which 
  can be retreaved via service.

  Default is *0*.


bridge configuration section
____________________________

**name**
  Bridge instance name which will be passed on module creation. 
  This name must be unique durig robotkernel life-cycle.

**so_file**
  Name of bridge library to load (shared object). Usually this 
  is something like libbridge_provider_xxx.so.

**config**
  YAML configuration passed to service provider instance

service_providers configuration section
_________________________________________

**name**
  Service provider instance name which will be passed on module creation. 
  This name must be unique durig robotkernel life-cycle.

**so_file**
  Name of service provider library to load (shared object). Usually this 
  is something like libservice_provider_xxx.so.

**config**
  YAML configuration passed to service provider instance

modules configuration section
_______________________________

**name**
  Module instance name which will be passed on module creation. This name
  must be unique durig robotkernel life-cycle.

**so_file**
  Name of module library to load (shared object). Usually this is something
  like libmodule_xxx.so.

**config**
  YAML configuration passed to module instance

**depends**
  A list or dictionary of module instance this one depends on. 

  Can either be a simple list, then all dependend modules are switched to 
  op before this one is powered up. 

  Or it can be a map where the target state of the the dependend module
  is specified (e.g. `depends: { timer: op, }`)

**excludes**
  A list of module instance names which shall be switched back to *init*
  when this one is powered up.

**power_up**
  Default target state which robotkernel tries to reach during startup.
  This must be one of [ *boot*, *init*, *preop*, *safeop*, *op*]. 

  Default value is *init*:

robotkernel_installer
_____________________

There's a simple robotkernel_installer which parses your robotkernel
config files and writes a conan include file with needed packages to
run robotkernel.

.. code-block:: yaml
   :caption: cissy_workspace.yaml

   nodes:
     localhost:
       alias:
         - rkhost
   
   packages:
     robotkernel_installer:
       requires:
         - robotkernel_installer/[~2]@robotkernel/stable
       add options:
         - robotkernel_installer/*:rk_version=6
       add generators:
         - rk_workspace
         - rk_workspace_options
   
   processes:
     project:
       requires: robotkernel_installer
       add options:
         - robotkernel_installer/*:rk_config=${CISSY_WORKDIR}/main.rkc
         - robotkernel_installer/*:allow_unstable=True
       node: rkhost
       start_command: >
         for i in $(find cissygen/ -name "*.inc.yaml"); do
             d1=$(dirname ${i})
             b1=$(basename ${i})
             b2=$(basename ${d1})
             sed -e '/links_and_nodes/s/^/#/g' "$i" > "${b2}_${b1}"
         done;
         rm -rf cissygen

Just copy this ``cissy_workspace.yaml`` to the directory with your robotkernel
config files and enter:

.. code-block:: bash

   cissy2 run -p project


This should give you the include files in the workspace directory like:

.. code-block:: bash

   project_rk_deps.inc.yaml
   project_rk_deps_options.inc.yaml

These files can now simply be included from other CISSy workspaces.

Configuration inside a CISSy workspace with ln_manager
______________________________________________________
