==============
 Introduction
==============

.. contents::

This chapter has the purpose to give an introduction what robotkrnel
is in general, and what specific problems it does address.

.. index::   
   pair: robotkernel; overview on central components

It also gives an outline which are the central components of robotkernel, 
explaining important terms, and finally gives an overview which
information can be found in the following chapters, and where one
might to proceed reading depending on their level of prior knowledge
and need for detail.
  
What is "robotkernel"?
==========================

robotkernel is a modular real-time robotics framework for Linux-based 
control systems. It provides the infrastructure for loading, executing, 
and managing dynamically linked modules that communicate through shared 
memory and trigger-based scheduling. Designed for hard-real-time industrial 
use cases, robotkernel allows composition of complex automation pipelines 
from reusable components.

Why is it used at the institute?
================================

In the development of robotic systems, integrating hardware components from 
different suppliers is a central challenge. The typical workflow used in 
practice follows this pattern:

- Hardware vendors provide plug-and-play devices.
- Protocol specifications are provided.
- APIs are available in C/C++ or Python.
- Example applications are included.
- Developers write custom code for data acquisition and control.

While this approach works well for testing or small-scale projects, it 
reveals serious limitations when applied to larger, long-term, or collaborative 
development efforts:

- Reusability of software components is severely restricted.
- Collaboration between team members becomes difficult.
- Integration of multiple devices from different vendors is complex.
- Long-term maintenance of software becomes increasingly costly.

How do our Robotic Systems look like? 
_____________________________________

The DLR Robotics and Mechatronics Center (DLR-RMC) operates a wide range of 
robotic platforms, each with distinct technical characteristics. Key systems 
include:

- Justin / upcoming NeoJustin
- LWR (Lightweight Robot) III
- Toro
- HaSy / David / NeoDavid
- MiroSurge
- Hand-II
- SARA
- upcoming TRP

These systems rely on a broad spectrum of communication technologies and 
hardware standards:

+----------------------+----------------------------------------+
| Communication Medium | Use Case                               |
+======================+========================================+
| Ethernet             | Real-time and non-real-time            |
|                      | communication, cameras                 |
+----------------------+----------------------------------------+
| SERCOS-II            | LWR joint control                      |
+----------------------+----------------------------------------+
| SpaceWire            | Hand-II, HaSy/David, MiroSurge         |
+----------------------+----------------------------------------+
| EtherCAT             | Beckhoff terminals, ELMO drives,       |
|                      | Digi-I/O, LWR joints, Miniservo drives,|
|                      | Synapticon nodes                       |
+----------------------+----------------------------------------+
| PCIe                 | SARA joints                            |
+----------------------+----------------------------------------+
| CAN                  | Heinzmann wheels, Schunk grippers/pan- |
|                      | tilt units                             |
+----------------------+----------------------------------------+
| SSI                  | Position encoders                      |
+----------------------+----------------------------------------+
| USB                  | Asus Xtion, XSense IMUs, various       |
|                      | microcontrollers                       |
+----------------------+----------------------------------------+
| Serial (RS232/RS485) | Medical hands, Dynamixel servos,       |
|                      | DLR FTS-78                             |
+----------------------+----------------------------------------+

Additionally, a wide variety of operating systems and hardware architectures are used:

- Operating Systems: Linux (standard, preempt-rt), QNX, VxWorks, Windows, Android
- Architectures: x86, x86_64, PowerPC (PPC), ARM

This diversity reflects the complexity of modern robotic systems but also introduces 
significant integration challenges.

Problems and Drawbacks of the previouly used approach
_____________________________________________________

The current development model leads to several systemic issues:

- Multiple independent **master programs** collect data from hardware without centralized coordination.
- System-specific logic and parameters are **hard-coded**, making adaptation to new setups difficult.
- Additional **communication middleware** is required between master programs.
- Master programs are **tightly coupled** to specific hardware configurations.
- Control applications must handle **non-uniform, inconsistent interfaces**.
- There is **no unified mechanism** for synchronizing or orchestrating distributed programs.

This results in a **decentralized, ad-hoc architecture** that negatively impacts
reliability, maintainability, and real-time performance.

Why This Matters
________________

The consequences of this fragmented approach are significant:

- High engineering effort for every new robotic system.
- Low reusability of existing software components.
- Increased integration complexity and maintenance overhead.
- Reduced system reliability due to decentralized synchronization.
- Difficulty in guaranteeing hard real-time behavior across multiple programs.
- Slow adaptation to hardware changes or new use cases.

Conclusion and Future Direction
_______________________________

The diversity of hardware, communication protocols, operating systems, and 
architectures clearly shows a critical need:

> **We required a simple, unified abstraction layer for hardware.**

*Goals:*

- Develop simple, reusable software components.
- Reduce development and maintenance effort.
- Enable collaborative development across distributed teams.
- Support centralized control and orchestration of components.

*Proposed Solution:*

A **hardware abstraction layer (HAL)** called **robotkernel** that enables 
seamless integration of components regardless of underlying protocol, platform, 
or operating system.

This abstraction allow developers to:

- Write code once and reuse it across multiple systems.
- Focus on application logic rather than low-level hardware details.
- Easily integrate new devices without rewriting core logic.
- Ensure consistent behavior, synchronization, and real-time guarantees.

Note for Developers:

By centralizing component development and standardizing interfaces, we can 
significantly improve development efficiency, software quality, and long-term 
sustainability of robotic systems at DLR-RMC.
