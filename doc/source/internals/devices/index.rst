Devices
==========

.. meta::
   :description: Overview of device and service abstractions in robotkernel for modular, decoupled system design.

.. _devices_overview:

In `robotkernel`, every device is registered in a **global device registry**, providing a centralized, hierarchical namespace for system components.

Key Properties
______________

- **Unique hierarchical name**:  
  Devices are addressable via a unique, structured name (e.g., ``ethercat.slave_0.inputs.pd``).
  
- **Dynamic registration**:  
  Providers can register devices at runtime; consumers can query and retrieve them dynamically.

- **Decoupling**:  
  The device abstraction separates *data ownership* from *data consumption*, enabling clean, reusable, and modular component design.

Device Categories
_________________

`robotkernel` supports several device types, each serving a distinct purpose:

- **Trigger Devices**  
  Event- or time-based synchronization primitives (e.g., control loop ticks, threshold crossings).  
  See :ref:`trigger_device`.

- **Process Data Devices (PD)**  
  Cyclic, deterministic I/O exchange and buffering (e.g., EtherCAT PDOs, CAN frames).  
  See :ref:`process_data_device`.

- **Stream Devices**  
  Unbounded, asynchronous byte streams (e.g., UART, serial logging, telemetry).  
  See :ref:`data_stream_device`.

.. note::
   **Design Principle**: Devices enable **plug-and-play architecture** — components interact via well-defined interfaces without knowledge of underlying implementation.

.. toctree::
   :maxdepth: 2

   triggers
   pds
   streams
