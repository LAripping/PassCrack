#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <list>
#include <unordered_map>
#include <functional>

extern bool		v;

using namespace std;
typedef list< pair<char[7],char[7]>* >  Table_t;
typedef unordered_map< string,string >  HashTable_t;                    //Hash-map with {endpoint,startpoint} pairs


HashTable_t* build_hashmap(Table_t* rainbow, float load_factor){
    
    long m = rainbow->size();
    
	HashTable_t*        rainbow_map = new HashTable_t();                //Allocate map structure
    
    rainbow_map->max_load_factor( load_factor );                        //Pre-Configure map to avoid all rehashes
    rainbow_map->rehash( m/load_factor );
/*  if(v) cout <<"Before loop:"                                         
               <<"\n\tMax size of hash-map with these contents: "<<rainbow_map->max_size()
               <<"\n\tMax # of buckets in hash-map with these contents: "<<rainbow_map->max_bucket_count()               
               <<"\n\tHash-map has a load factor of "<<rainbow_map->load_factor()
               <<", max at "<<rainbow_map->max_load_factor()
               <<"\n\tHash-map has "<<rainbow_map->bucket_count()<<" buckets\n"<<endl;
*/     
                      
	float i=1, progress; 
	for( pair<char[7],char[7]>* pp : *rainbow ){
	    progress = i / m;
	    
	    if(v){
	        cout << setw(6) << setprecision(2) << fixed
			     << progress*100 << "% complete..." << '\r' << flush; 
			     
/*  		    if(i==1000000)
			        cout <<"Loop #1M:           "                       
                         <<"\n\tMax size of hash-map with these contents: "<<rainbow_map->max_size()
                         <<"\n\tMax # of buckets in hash-map with these contents: "<<rainbow_map->max_bucket_count()               
                         <<"\n\tHash-map has a load factor of "<<rainbow_map->load_factor()
                         <<", max at "<<rainbow_map->max_load_factor()
                         <<"\n\tHash-map has "<<rainbow_map->bucket_count()<<" buckets\n"<<endl;   
*/        }                         
			     
        string      startpoint(pp->first);
	    string      endpoint(pp->second);                               //Convert C-strings to std::strings 
	    
	                                                                    //Create a pair to map the startpoint by the endpoint
	    pair<string,string>* tuple = new pair<string,string>(endpoint,startpoint);          
	    rainbow_map->insert(*tuple);                                    //And insert it in the map
	    
        delete pp;	
        i += 1.0;
	}

	return rainbow_map;	    
}



/*  Overloaded for hash-map lookups
 *  Looking for pwd in endpoints.
 *      if found, the startpoint is returned through preallocated "startpoint" C-string 
 *
 */
bool lookup(char* pwd, HashTable_t* rainbow_map, char* startpoint ){
    string end_requested(pwd);
    
    HashTable_t::const_iterator got = rainbow_map->find(end_requested);
    
    if( got==rainbow_map->end() ){
       return false;
    }else{
        string      start_found        = got->second;
           
        strcpy( startpoint,start_found.c_str() );
        return true;
    }
}








