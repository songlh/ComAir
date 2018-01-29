if (!this.dump) this.dump = print;

function test(n)
{
 var funtext = "";
 
 for (i=0; i<n; ++i)
   funtext += "alert(" + i + "); ";

 fun = new Function(funtext);

 start = new Date();
 
 fun+"";
 
 end = new Date();
 dump("Size: " + n + ", Time: " + (end - start) + " ms" + "\n");
}

test(10000);
