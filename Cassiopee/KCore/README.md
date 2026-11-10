# KCore
Cassiopee Core functions

ONERA.
Licenced under LGPL3.

For support, go to https://github.com/onera/Cassiopee

## installing KCore:

- before installing, edit etc/installBaseUser.json. Set the correct 
compiler names and libraries.

Then,

For standard install:
- set CASSIOPEE variable to Cassiopee directory, for instance:
  in sh/bash: export CASSIOPEE="/home/john/Cassiopee"
  in csh/tcsh: setenv CASSIOPEE "/home/john/Cassiopee"
- set ELSAPROD variable to your processor type, for instance:
  in sh/bash: export ELSAPROD="x86"
  in csh/tcsh: setenv ELSAPROD "x86"
- type: ./install
- binaries will be installed to $CASSIOPEE/Dist/bin/$ELSAPROD
- follow instructions

Build variants (suffixes appended to ELSAPROD, can be combined):
- _DBG: debug build (no optimization, sanitizers, bound checks)
- _i8: all integers (E_Int) compiled in 64 bits (default: 32 bits)
- _r4: all reals (E_Float) compiled in single precision, 32 bits
  (default: double precision, 64 bits). Numpy fields exchanged with
  the compiled modules must then be numpy.float32: use
  Converter.Internal.E_NpyFloat as dtype in Python code.
  Example: export ELSAPROD="x86_r4"

For custom install:
- set CASSIOPEE variable to Cassiopee source directory, for instance:
  in sh/bash: export CASSIOPEE="/home/john/Cassiopee"
  in csh/tcsh: setenv CASSIOPEE "/home/john/Cassiopee"
- set ELSAPROD variable to your processor type, for instance:
  in sh/bash: export ELSAPROD="x86"
  in csh/tcsh: setenv ELSAPROD "x86"
- set CASSIOPEE_INSTALL_PREFIX to the target directory for install.
  in sh/bash: export CASSIOPEE_INSTALL_PREFIX="/home/john/<YOUR_DIRECTORY>"
  in csh/tcsh: setenv CASSIOPEE_INSTALL_PREFIX "/home/john/<YOUR_DIRECTORY>"

- type: ./install
- binaries will be installed to <YOUR_DIRECTORY>
- follow instructions

## Using Cassiope modules:
For standard install
- in sh/bash: source $CASSIOPEE/Dist/env_Cassiopee.sh
  in csh/tcsh: source $CASSIOPEE/Dist/env_Cassiopee.sh
- run "python" or "cassiopee" (if you have CPlot installed)

For custom install:
- in sh/bash: source <YOUR_DIRECTORY>/env_Cassiopee.sh
  in csh/tcsh: source <YOUR_DIRECTORY>/env_Cassiopee.csh
- run "python" or "cassiopee" (if you have CPlot installed)
