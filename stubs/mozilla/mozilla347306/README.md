##### How to install this bug?

1. export ROOT=/where/is/buggy/code

2. cd ${ROOT}

3. replace Linux_All.mk under src/config

4. cd buggy/src/

5. make -f Makefile.ref

How to trigger this bug?

cd ${ROOT}

time ./buggy/src/Linux_All_DBG.OBJ/js test.js 
Size: 10000, Time: 3101 ms


real	0m3.148s
user	0m3.132s
sys	0m0.004s

time ./patched/src/Linux_All_DBG.OBJ/js test.js 
Size: 10000, Time: 24 ms


real	0m0.069s
user	0m0.052s
sys	0m0.016s

patch:


#### error solutions:

clang  -flto -o Linux_All_DBG.OBJ/jscpucfg Linux_All_DBG.OBJ/jscpucfg.o

clang -flto -use-gold-plugin -Wl,-plugin-opt=save-temps -shared  -o Linux_All_DBG.OBJ/libjs.so Linux_All_DBG.OBJ/jsapi.o Linux_All_DBG.OBJ/jsarena.o Linux_All_DBG.OBJ/jsarray.o Linux_All_DBG.OBJ/jsatom.o Linux_All_DBG.OBJ/jsbool.o Linux_All_DBG.OBJ/jscntxt.o Linux_All_DBG.OBJ/jsdate.o Linux_All_DBG.OBJ/jsdbgapi.o Linux_All_DBG.OBJ/jsdhash.o Linux_All_DBG.OBJ/jsdtoa.o Linux_All_DBG.OBJ/jsemit.o Linux_All_DBG.OBJ/jsexn.o Linux_All_DBG.OBJ/jsfun.o Linux_All_DBG.OBJ/jsgc.o Linux_All_DBG.OBJ/jshash.o Linux_All_DBG.OBJ/jsinterp.o Linux_All_DBG.OBJ/jsinvoke.o Linux_All_DBG.OBJ/jsiter.o Linux_All_DBG.OBJ/jslock.o Linux_All_DBG.OBJ/jslog2.o Linux_All_DBG.OBJ/jslong.o Linux_All_DBG.OBJ/jsmath.o Linux_All_DBG.OBJ/jsnum.o Linux_All_DBG.OBJ/jsobj.o Linux_All_DBG.OBJ/jsopcode.o Linux_All_DBG.OBJ/jsparse.o Linux_All_DBG.OBJ/jsprf.o Linux_All_DBG.OBJ/jsregexp.o Linux_All_DBG.OBJ/jsscan.o Linux_All_DBG.OBJ/jsscope.o Linux_All_DBG.OBJ/jsscript.o Linux_All_DBG.OBJ/jsstr.o Linux_All_DBG.OBJ/jsutil.o Linux_All_DBG.OBJ/jsxdrapi.o Linux_All_DBG.OBJ/jsxml.o Linux_All_DBG.OBJ/prmjtime.o

  
