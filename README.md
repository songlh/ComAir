## ComAir


Profiling C/C++ program complexity in production runs.

#### 1. Prepare Your Runtime Env

- Ubuntu 16.04 and run upgrade. 

- Install llvm 5.0, you can follow [install-llvm](install-llvm.txt).

- Create a python [virtual env](https://virtualenv.pypa.io/en/stable/) and install [requirement](requirements.txt) libs.

#### 2. How To Use ComAir

<!--- (TODO: update detail document) --->

There are some examples scripts in stubs fold. Take <code>stubs/apache34464</code> as an example. 

- You should change the local env variables in <code>run_apache34464.sh</code>.

- If you want to run sampling of profiling, you should set <code>sampling="1"</code> in <code>$ sh run_apache34464.sh</code>.
 Or you can run <code>$ sh run_apache34464.sh</code> directly. The script will generate two files. One is <code>xxx_func_name_id.txt</code>, and the other is <code>aprof_logger.txt</code>. The first file is using to map function name to its' id. The second file saves all runtime log info. The <code>py</code> script in same fold can parse the log.

- Active your python virtual env, run <code>$ python run_apache34464.py</code>.
 This script will automatically fit the function curve.  

<!--- (TODO: update some examples display) --->
