# robotkernel

[![Build and Publish Debian Package](https://github.com/robotkernel-hal/robotkernel/actions/workflows/build-deb.yaml/badge.svg)](https://github.com/robotkernel-hal/robotkernel/actions/workflows/build-deb.yaml)
[![License: LGPL-V3](https://img.shields.io/badge/license-LGPL--V3-green.svg)](LICENSE)
[![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)](#)
[![Debian](https://img.shields.io/badge/Debian-A81D33?logo=debian&logoColor=fff)](#)
[![Ubuntu](https://img.shields.io/badge/Ubuntu-E95420?logo=ubuntu&logoColor=white)](#)

**`robotkernel`** is a modular real-time robotics framework for Linux-based control systems. It provides the infrastructure for loading, executing, and managing dynamically linked modules that communicate through shared memory and trigger-based scheduling. Designed for hard-real-time industrial use cases, `robotkernel` allows composition of complex automation pipelines from reusable components.

---

## Components

Robotkernel is a program which dynamically loads other objects called
components. There are different types of components, which are
distinguished by it's functionality.

---

## Modules

![robotkernel module](rk_module.png "robotkernel module")

The main components are the *modules*. A *modules* usually is an
implementation of a hardware component/device driver. Everything what's
needed to communicate with a specific peace of hardware needs to be
included here.

### State Machine

Each robotkernel module has to implement the robotkernel state
machine.

```mermaid
stateDiagram-v2
    [*] --> INIT : start
    INIT --> PREOP : initialization done
    PREOP --> SAFEOP : hardware ready, start cyclic operation
    SAFEOP --> OP : enable full operation (commands allowed)
    OP --> SAFEOP : disable commands, safe operation only
    OP --> BOOT : maintenance requested
    SAFEOP --> BOOT : maintenance requested
    BOOT --> PREOP : maintenance done, restart communication
    PREOP --> INIT : reset
    SAFEOP --> INIT : reset
    OP --> INIT : reset
    BOOT --> INIT : reset

    %% Error transitions
    INIT --> ERROR : error detected
    PREOP --> ERROR : error detected
    SAFEOP --> ERROR : error detected
    OP --> ERROR : error detected
    BOOT --> ERROR : error detected

    ERROR --> INIT : error resolved
```

  State    short description                description
  -------- -------------------------------- ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  INIT     Initialization phase             A module should do all necessary initialize operations and allocate all it's internal resources.
  PREOP    before (pre) operational phase   Used to start communcating to attached hardware devices. May do a hardware discovery if neccessary and start a simple communication to enable the ability to configure the hardware. May register service providers here. Only acyclic operation phase, no cyclic realtime operation.
  SAFEOP   safe opreational phase           Start cyclic operation. Only measuements/telemetry wil be valis, no commands possible here. Registering triggers should be done here.
  OP       full operational phase           Full cyclic operation. Measurements and commands are valid, commands and sent to and accepted by the hardware.
  BOOT     maintainance phase               Used to do maintainence stuff, like file upload and download. Usually needed to update the firmware of the hardware devices.
  ERROR    error phase                      On errors, the module may stop cyclic/acyclic operation and switch to the error state to signal that something has gone wrong.

### Exported C-Api interface

A module provides a bunch of simple C entry points for the
robotkernel. These are:

#### mod\_configure

*mod\_configure* will be Called after the *module* has been loaded by
the robotkernel. The *module* will get it's configuration usually
written in YAML. It should now do a state transition to the init state
and do all it's initialization stuff.

```c++
typedef MODULE_HANDLE (*mod_configure_t)(const char* name, const char* config);
```

#### mod\_unconfigure

On shutdown the *mod\_unconfigure* will be called. The *module* should
now switch to the init state and should do all needed cleanup
afterwards.

```c++
typedef int (*mod_unconfigure_t)(MODULE_HANDLE hdl);
```

#### mod\_set\_state

When a state transition is requested, *mod\_set\_state* will be called.
The *module* has to perform all necessary operations to reach the
requested state or signal an error.

```c++
typedef int (*mod_set_state_t)(MODULE_HANDLE hdl, module_state_t state);
```

#### mod\_get\_state

*mod\_get\_state* will be called by the robotkernel to determine the
*module*'s state

```c++
typedef module_state_t (*mod_get_state_t)(MODULE_HANDLE hdl);
```

#### mod\_tick

If using direct module-to-module synchronization, *mod\_tick* will be
attached to an other modules trigger device.

```c++
typedef void (*mod_tick_t)(MODULE_HANDLE hdl);
```

provide process data, services, ....

---

## Bridges

act as inter-process communication bridge. usefull to provide
communication with other applications e.g. links-and-nodes.

### Exported C-Api interface

A bridge provides a bunch of simple C entry points for the
robotkernel. These are:

#### bridge\_configure

*bridge\_configure* will be Called after the *bridge* has been loaded by
the robotkernel. The *bridge* will get it's configuration usually
written in YAML. It should now do all needed initialization stuff.

```c++
typedef BRIDGE_HANDLE (*bridge_configure_t)(const char* name, const char* config);
```

#### bridge\_unconfigure

On shutdown the *bridge\_unconfigure* will be called. The *bridge*
should do all needed cleanup.

```c++
typedef int (*bridge_unconfigure_t)(BRIDGE_HANDLE hdl);
```

#### bridge\_add\_service

This function is called if a new service was registered to the
robotkernel. The *bridge* should now do all needed operations to export
these new services. This should include, translationi/generation of the
the service definition to the middleware data description language and
service registration to the middleware.

```c++
typedef void (*bridge_add_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);
```

#### bridge\_remove\_service

If a service is removed from the robotkernel, the bridge should also
unregister the service from the middleware.

```c++
typedef void (*bridge_remove_service_t)(BRIDGE_HANDLE hdl, const robotkernel::service_t &svc);
```

--- 

## Service Providers

They provide a set of common services to the user to gain the same
expiriance for different attached hardware. Therefor a module has to
derive from the service providers *base* class and implement the service
functions. Afterwards the derived class instances can be registered to
the robotkernel by calling *add\_device*.

---

## Devices

Robotkernel devices are the internal representation of module
resources. These devices may be *process data*, *stream* or *trigger*.
Each **component** may use *robotkernel* internal devices to provide
data to other components or to make inter-process-communication. All
devices consist of the owning modules name, the device name and an
device suffix. They are represented by a unique name concatenated from

``` {.xml}
<owner name>.<device name>.<suffix>
```

. The devices will be allocated by the owning module and then registered
to the *robotkernel* with the add\_device call.

```c++
void add_device(std::shared_ptr<device> dev);
```

Removing (Unregistering) devices from the robotkernel can either be
done by removing single device or by removing all devices owned by a
module.

```c++
void remove_device(std::shared_ptr<device> dev);
void remove_devices(const std::string& owner);
```

To access a registered device from within a module, the device name has
to be known.

```c++
std::shared_ptr<device> get_device(const std::string& name);
```

### Process data

A process data device is used to provide cyclic-realtime data to other
**components**. It is derived from the *device* base class and can be
registered to the robotkernel. For a detailed description of
robotkernel process data implementations please refer to
[robotkernel/process\_data](robotkernel/process_data "wikilink").

### Streams

Streams are usually used to supply a byte stream to other
**components**. To create a stream device a module has to derive from
the *stream* class. Because *stream* is derived from *device* it needs
an owner name and a trigger name. The two byte stream functions *read*
and *write* need to be implemented here.

```c++
size_t read(void* buf, size_t bufsize);
size_t write(void* buf, size_t bufsize);
```

Afterwards, the *stream* could be registered to the robotkernel by
calling ''add\_device'.

```c++
std::shared_ptr<derived> my_stream = std::make_shared<derived>();

kernel& k = *kernel::get_instance();
k.add_device(my_stream);

while (1) {
    ...
    int rd = read(fd, buf, 1000);
    my_stream->write(buf, rd);
    ...
    rd = my_stream->read(buf, 1000);
    read(fd, buf, rd);
    ...
}       
```

### Triggers

A trigger is very useful, if the synchonization of different
**components** is needed. To create a trigger device either make an
instance of the *trigger* class or derive from it. Because *trigger* is
derived from *device* it needs an owner name and a trigger name.
Optionally a trigger rate could be specified. The owner of the trigger
should now call *trigger\_modules* if the waiter should be notified.

```c++
sp_trigger_t my_trigger = std::make_shared<trigger>(name, "posix_timer", rate);

kernel& k = *kernel::get_instance();
k.add_device(my_trigger);

while (1) {
    ...
    my_trigger->trigger_modules()
    ...
}       
```

The waiting module should now retreave the trigger device from the
robotkernel and registers its handler.

```c++
class my_trigger_func : public trigger_base {
    public:
        ...
        void tick() { /* do something */ };
        ...
}

kernel& k = *kernel::get_instance();
sp_trigger_t my_trigger = k.get_trigger_device("timer.posix_timer.trigger");
my_trigger->add_trigger(std::make_shared<my_trigger_func>(...));
```

---

## Services

For some kind of Remote-Procedure-Calls there are the acyclic services.

### Service declaration

A service usually consists of a callback function and a service
definition.

```c++
int service_set_state(const service_arglist_t& request, service_arglist_t& response);
static const std::string service_definition_set_state;
```

### Service registration

Service need to be registered to the robotkernel. Therefore a module
has to call the *add\_service* function.

```c++
kernel& k = *kernel::get_instance();
k.add_service(name, "set_state", service_definition_set_state, std::bind(&module::service_set_state, this, _1, _2));
```

### Service definition

Service definitions are simple YAML string. They describe the arguments
passed to *request* and *response* fields of the service callback
function.

```c++
const std::string module::service_definition_set_state =
"request:\n"
"- string: state\n"
"response:\n"
"- string: error_message\n";
```

### Service callback

In the service callback the request and response fields can simply be
accessed with the array operator. The developer has to ensure, that the
datatypes in the service definition match the types used in the callback
function.

```c++
int module::service_set_state(const service_arglist_t& request, service_arglist_t& response) {
    // request data
#define SET_STATE_REQ_STATE     0
    string state = request[SET_STATE_REQ_STATE];
    kernel& k = *kernel::get_instance();

    // response data
    string error_message;

    try {
        k.set_state(name, string_to_state(state.c_str()));
    } catch (const exception& e) {
        error_message = e.what();
    }

#define SET_STATE_RESP_ERROR_MESSAGE    0
    response.resize(1);
    response[SET_STATE_RESP_ERROR_MESSAGE] = error_message;

    return 0;
}
```
