### ComAir

Profiling C/C++ program complexity in production runs.

#### Prepare Runtime Env

- Ubuntu 16.04 and run upgrade. 

- Install llvm 5.0, you can follow [install-llvm](install-llvm.txt).

- Create a python [virtual env](https://virtualenv.pypa.io/en/stable/) and install [requirement](requirements.txt)libs.

#### How To Use ComAir?

[comment]: <> (TODO: update detail document)

There are some examples scripts in stubs fold. Take <code>stubs/apache34464</code> as an example. 

- You should change the local env variables in <code>run_apache34464.sh</code>.

- If You want to run sampling of profiling, you should set <code>sampling="1"</code> in <code>run_apache34464.sh</code>.
 Or you can run <code>run_apache34464.sh</code> directly.

- Active your python virtual env, run <code>$ python run_apache34464.py</code>.
 This script will automatically fit the function curve.  

[comment]: <> (TODO: update some examples display)
