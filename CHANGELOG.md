# 6.0.0

Robotkernel release 6 is a major internal cleanup and redesign.

## Process-runner and bubblewrap-runner classes

The process-runner adds the ability to run external program from within an robotkernel module with execlp. All of the process output is printed with robotkernels logging framework. All **stdout** message are printed with loglevel *info*, **stderr** is printed with *error*. In addition there's also a bubblewrap-runner class which allows to run programs with a virtual root environment with the help of *bwrap*.

## Triggers now allow configurable cycle shift in combination with trigger divisor.

This adds the possibilitie to configure multiple triggerables to be executed in different cycles of the main trigger.

## Trigger now also provide a system clock time with configurable system time offset and last trigger time.

Enables synchronisation mechanisms for different clock sources. 

## Trigger now uses a priority scheduling when triggering its childs.

This let's you configure in which order the trigger clients are executed.

## Process data devices now always provide a trigger device 

Enables a deterministic data workflow.

## Process data devices now support injection of values.

Done via robotkernel service.

## robotkernel-idl

New robotkernel-idl to provide a yaml-based description of service an process data.

** built-in datatypes **

*bool*: Boolean values
*uint8_t*: 8-bit unsigned type 
*uint16_t*: 16-bit unsigned type
*uint32_t*: 32-bit unsigned type
*uint64_t*: 
*int8_t*: 
*int16_t*: 
*int32_t*: 
*int64_t*: 
*float*: 
*double*: 
*string*: 

**datatypes example**

```yaml
datatypes:
  my_special_type:
    fields:
      control: 
        type: uint8_t
        bitfield: 
          start: { bitsize: 1 }
          reset: { bitsize: 1 }
          reserved: { bitsize: 6 }
      target_position: { type: double }
```

**service example**

```yaml
  services:
    my_new_rk_service:
      request:
        my_req_field_1: { type: uint8_t, is_array: true }
      response:
        my_answer_field_1: { type: string } 
```

** process data example**

```yaml
  pds:
    my_new_rk_pd:
      entry_1: { type: uint8_t, is_array: true, size: 7}
      entry_2: { type: double }
      entry_3: { type: my_special_type }
```

Uses new robotkernel_generator to generate base classes for services and process data from robotkernel-idl files

## Splitted robotkernel public API from internal implementation.

Make it more robust and stable. Only expose functionality which is needed by modules/service-providers/bridges.

## Reworked robotkernel-conan-template

Now doing no longer in-source builds of robotkernel (modules, sp, bridges). These are done in seperate build folders. This also allows us to use *conan editables* when developing.

## Robotkernel module dependecies now support target states of dependencies.

Users can now specify the target state of a dependent module. Default is OP.

## Robotkernel module excludes 

Addinionally excludes can be specified. The excluded modules are forced to INIT if module is switched to a state != INIT.

## And a lot of internal changes and improvements i forgot to mention here...

