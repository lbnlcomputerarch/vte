Verilator Testbench Environment (VTE)
========

## Abstract ##
VTE is a library that allows fast and simple generation of C++ testers for modules written in Chisel and build an efficient interface to existing C++ based simulators. It contains Scala-based interface and C++ testbench class. Scala interface interacts with Firtl Interpreter and generates a basic set of C++ testbench files. These files contain the list of the Device Under Test (DUT) input-output ports as well as their parameters. C++ testbench provides functionalities similar to the Chisel PeekPokeTester, such as poke, expect, step functions. As a backend it uses Verilator.

---
## Copyright ##
*Verilator Testbench Environment (VTE)*  Copyright (c) 2018, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any required approvals from the U.S. Dept. of Energy). All rights reserved.

If you have questions about your rights to use or distribute this software, please contact Berkeley Lab's Intellectual Property Office at  IPO@lbl.gov.

NOTICE.  This Software was developed under funding from the U.S. Department of Energy and the U.S. Government consequently retains certain rights. As such, the U.S. Government has been granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software to reproduce, distribute copies to the public, prepare derivative works, and perform publicly and display publicly, and to permit other to do so. 

---
