#include <iostream>
#include <fstream>
#include <sstream>
#include <tr1/functional>
#include <string>
#include <map>
#include <assert.h>
#include <stdlib.h> 

using namespace std;
using namespace std::tr1;

//apache-ant-1.6.1/apache-ant-1.6.1/src/main/org/apache/tools/ant/types/selectors/modifiedselector/DigestAlgorithm.java
class DigestAlgorithm
{
public:
  static string getValue(string & str)
  {
     hash<string> str_hash;
     stringstream stream;
     stream << hex << str_hash(str);
     return stream.str();
  }
};

//jdk/src/share/classes/java/util/Properties.java
class Properties
{
  map<string, string> content;

public:
  void store(ofstream & of)
  {
     map<string, string>::iterator itMapBegin = content.begin();
     map<string, string>::iterator itMapEnd   = content.end();

     for(; itMapBegin != itMapEnd; itMapBegin ++)
     {
	of << itMapBegin->first << "=" << itMapBegin->second << "\n";
     }

     of.flush();
  }

  void load(ifstream & ifile )
  {
    content.clear();
    string line;

    while ( getline (ifile,line) )
    {
      for(size_t i = 0 ; i < line.size(); i ++ )
      {
        if(line[i] == '=')
        {
          string key = line.substr(0, i);
          string value = line.substr(i+1, line.size()-i-1);
          assert(key.size()>0);
          assert(value.size()>0);
          assert(content.find(key)==content.end());
          content[key] = value;
          break;
        }
      }
    }
  }

  void dump()
  {
    map<string, string>::iterator itMapBegin = content.begin();
    map<string, string>::iterator itMapEnd = content.end();

    for(; itMapBegin != itMapEnd; itMapBegin ++)
    {
      cout << itMapBegin->first << " " << itMapBegin->second << "\n";
    }
  }

  void put(string & key, string & value)
  {
    content[key] = value;
  }

  string getProperty(string & key)
  {
    if(content.find(key) == content.end())
    {
      return string("-1");
    }
   
    return content[key];
  }

};

//jdk/src/share/classes/java/util/PropertiesfileCache.java
class PropertiesfileCache
{
  string cachefile;
  Properties cache;
  bool cacheLoaded;
  bool cacheDirty;
  
public:
  PropertiesfileCache()
  {}

  PropertiesfileCache(string cachefile) 
  {
    this->cachefile = cachefile;
  }

  void setCacheFile(char * cachefile)
  {
    
    this->cachefile = string(cachefile);
  }

  string getCachefile() { return cachefile; }

  void dump() { cache.dump(); }

  void load()
  {
    
    if(this->cachefile.size() > 0)
    {
      ifstream ifile(cachefile.c_str());
      cache.load(ifile);
      ifile.close();   
    }
    cacheLoaded = true;
    cacheDirty  = false;
  }

  void save()
  {
    if (!cacheDirty) {
      return;
    }
  
    if(this->cachefile.size() > 0)
    {
      ofstream ofile(cachefile.c_str());
      cache.store(ofile);
      ofile.flush();
      ofile.close();
    }

    cacheDirty = false;
  }

  string get(string & key)
  {
    if (!cacheLoaded) {
      load();
    }

    return cache.getProperty(key);
  }

  void put(string & key, string & value) {
    cache.put(key, value);
    cacheDirty = true;
  }
  
};


//apache-ant-1.6.1/apache-ant-1.6.1/src/main/org/apache/tools/ant/types/selectors/modifiedselector/ModifiedSelector.java
class ModifiedSelector
{
  PropertiesfileCache cache;
  bool update;
  bool isConfigured;
  
  
public:

  ModifiedSelector()
  {
     update = false;
     isConfigured = false;
  }

  void configure()
  {
    if (isConfigured) {
      return;
    }
    isConfigured = true;
    this->cache.setCacheFile((char *)("./cache.result"));
    update = true;
  }
  
  void dump()
  {
    cache.load();
    cache.dump();
  }

  bool isSelected(string & filename)
  {
    string cachedValue = cache.get(filename);
    string newValue = DigestAlgorithm::getValue(filename);
    bool rv = (cachedValue == newValue);
    if (update && !(cachedValue == newValue))
    {
      cache.put(filename, newValue);
      cache.save();
    }
    return rv;
  }

}; 

void Test(string & test)
{
  ifstream ifile(test.c_str());
  ModifiedSelector selector;
  selector.configure();
  
  string line;
  while ( getline (ifile,line) )
  {
     selector.isSelected(line);
  }


  ifile.close();
}

void Test2()
{
  ModifiedSelector selector;
  selector.configure();
  selector.dump();
  

}




int main(int argc, char ** argv)
{
  if(argc != 2 )
  {
     cout << "wrong input parameter" << endl;
     exit(-1);
  }
  
  string sTest = string(argv[1]);
  //Test(sTest);  
  Test2();
  return 0;
}
