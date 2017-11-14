# README.md for robotkernel

**robotkernel-5** is an easily configurable hardware abstraction
framework. In most robotic system assembly of robotic hardware
components is a challenging task. With
[robotkernel-5](robotkernel-5 "wikilink") the control engineer just 
has to write a bunch of simple and small configuration in
[YAML](wp:YAML "wikilink").

# Components

Robotkernel-5 is a program which dynamically loads other objects called 
components. There are different types of components, which are distinguished
by it's functionality.

## Modules

The main components are the *modules*. A *modules* usually is an implementation 
of a hardware component/device driver. Everything what's needed to communicate 
with a specific peace of hardware needs to be included here. 

### State Machine

Each robotkernel-5 module has to implement the robotkernel-5 state machine.

![Image of robotkernel-5 state machine](https://rmc-github.robotic.dlr.de/robotkernel/robotkernel/blob/robotkernel-5/doc/images/rk_state_machine.png?style=centerme)

### Exported C-Api interface

A module provides a bunch of simple C entry points for the robotkernel-5. These are:

#### mod_configure

*mod_configure* will be Called after the *module* has been loaded by the robotkernel-5. 
The *module* will get it's configuration usually written in YAML. It should now do a state 
transition to the init state and do all it's initialization stuff.

#### mod_unconfigure

On shutdown the *mod_unconfigure* will be called. The *module* should now switch to the init
state and should do all needed cleanup afterwards.

#### mod_set_state

When a state transition is requested, *mod_set_state* will be called. The *module* has to 
perform all necessary operations to reach the requested state or signal an error.

#### mod_get_state

*mod_get_state* will be called by the robotkernel-5 to determine the *module*'s state

#### mod_tick

If using direct module-to-module synchronization, *mod_tick* will be attached to an 
other modules trigger device.

provide process data, services, ....

## Bridges

act as inter-process communication bridge. usefull to provide
communication with other applications e.g. links-and-nodes.

## Service Providers

provide a set of common services to the user ot gain the same expiriance
for different attached hardware.


# devices 

Each **component** may use robotkernel-5 internal devices to provide data to 
other components or to make inter-process-communication.

## process data

A process data device is used to provide cyclic-realtime data to other **components**.

## streams

Streams are usually used to supply a byte stream to other **components**

## triggers

A trigger is very useful, if the synchonization of different **components** is needed.

# services

For some kind of Remote-Procedure-Calls there are the acyclic services.
