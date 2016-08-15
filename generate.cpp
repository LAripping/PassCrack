#include <iostream>
#include <fstream>
#include <iomanip>

#include <list>
#include <vector>

#include <ctime>
#include <cmath>
#include <cstring>

#include <unistd.h>
#include <pthread.h>

#include <random>

#include "blake.hpp"



using namespace std;
typedef list<pair<char[7],char[7]>*> Table_t;


extern char alphabet[64];

extern void (*hashfun)( uint8_t *out, const uint8_t *in, uint64_t inlen );
extern void (*redfun) ( char* out, const uint8_t *in, int red_by );
bool		lookup(char* pwd, Table_t* rainbow, char* startpoint );

extern bool		v, savechain, prng, inplace;
extern ofstream	outchain;





/*
 * Comparator function to be passed in list sorter.
 *
 * With an iput of "first" and "second" start-end-pair pointers,
 * returns whether (first->end) < (second->end) 
 * based on alphabet's indexing. ( see matrix above )
 *
 * That means first compare key is endpoint, 
 * Second is startpoint, then select arbitrarily. 
 *
 */
bool pass_comparator(const pair<char[7],char[7]>* first, const pair<char[7],char[7]>* second){	
	unsigned int i=0;
  
	while( i<7 ){
		if( first->second[i] < second->second[i] )
			return true;
		else if ( first->second[i] > second->second[i] )
			return false;
		else
			++i;
	}
	
	// If endpoints identical, compare based on startpoints!
	
	i=0;
	while( i<7 ){
		if( first->first[i] < second->first[i] )
			return true;
		else if ( first->first[i] > second->first[i] )
			return false;
		else
			++i;
	}
	
	// If even startpoints identical (<1 in a million) leave them be.
	return true;	
}	



/*
 * Binary predicate function to be passed in list duplicate-remover.
 *
 * With an iput of "first" and "second" start-end-pair pointers,
 * returns whether (first->end) = (second->end)  
 *
 */
bool pass_predicate(const pair<char[7],char[7]>* pair1, const pair<char[7],char[7]>* pair2){	
	//replace sole "return" statement with if-clause to deallocate the non-unique pairs found as well
	return( !strcmp(pair1->second, pair2->second) );	
}	




/*
 * Struct used to pass data to the worker threads.
 *
 * id       : Specifies the threads's id assigned by the spawner thread.
 * 
 * my_m     : Specifies the # of chains the subtable created must have
 *
 * t        : Specifies the # of links/chain the subtable created must have
 *
 * prog_v   : Refereence to a vector where the thread will report it's progress.
 *            The element of the vector is indexed by this thread's id.
 *
 * rainbow  : Reference to an allocated subtable the thread has to fill.
 *
 */
struct ThreadData{
    int  my_id;
    long my_m;
    int  my_t;                               
    vector<long>* my_prog_v;
    Table_t*      my_rainbow;
};

void* worker_thread( void* args );




pthread_mutex_t  vector_mutex = PTHREAD_MUTEX_INITIALIZER;

Table_t* generate_tables(long m, int t, int threads, bool post){
	long discarded = 0;
	float progress = 0;
	char	cur_pwd[7];													// Both hash and reduction functions take
	uint8_t	cur_h[32];													//  preallocated buffers in arguments and fill them.
	char	passchar;
	char	startpoint[7];
	startpoint[6] = '\0';												// Startpoint string is a plain old C-string
	
	
	if(!prng)                                                           // Initialize whatever PRNG is used 
	    srand( time(NULL) );	
	// else   
	    int range_from  = 0;
	    int range_to    = 63;
	    default_random_engine           generator( (unsigned int)time(0) );
	    uniform_int_distribution<int>   distr(range_from, range_to);   
	
	        
	
																		// Allocate rainbow table structure 
	Table_t* rainbow = new Table_t();	
	
	
    cout << "Generating tables (m="<<m<<", t="<<t<<")"<< endl; 
	                                                ////////////////////// SINGLE-THREADED ALGORITHM
	if(threads==1){	                                
	    for(long i=0; i<m ; i++){										// For every chain		
		    progress = (float)i / (float)m;
		    pair<char[7],char[7]>* start_end;
		    
		    do{ 
		        for(int j=0; j<6; j++){
			        if(!prng)                                           // Start from a random password
			            passchar = alphabet[ rand() % 64 ];
			        else
			            passchar = alphabet[ distr(generator) ];    						
			        startpoint[j] = passchar;
		        }	
		
		        if(savechain && i<10)	outchain << "CHAIN #" << i+1 << endl;
		        for(int j=1; j<=t-1 ; j++){								// For every link
			        if(j==1){
				        if(savechain && i<10)	outchain << startpoint;
				        hashfun( cur_h,(uint8_t*)startpoint,6 );		// (1) HASH link's password
			        }													//  or chain's starting point if it's the first
			        else{
				        if(savechain && i<10)	outchain << cur_pwd;
				        hashfun( cur_h,(uint8_t*)cur_pwd,6 );															
			        }
			
			        if(savechain && i<10){  							// Update output file if specified to do so 
				        outchain << " -> ";
				        for(int k=0; k<32; k++)
					        outchain << hex << setw(2) << setfill('0') << (int)cur_h[k] << dec ;
				        outchain << " -> " << endl;
			        }	
				
				
			        redfun( cur_pwd,cur_h,j );							// (2) REDUCE previously computed hash using the correct red-fun
		        }														// and start all over again
		        if(savechain && i<10)	outchain << cur_pwd << '\n' << endl;
		
		        start_end = new pair<char[7],char[7]>();	            // Finally, create start-end pair,
		        strcpy( start_end -> first, startpoint);
		        strcpy( start_end -> second,   cur_pwd);
		        
		        char startpoint_f[7];                                   // dummy arg- won't be used
		        
		        
		        if(i<5) break;                                          // don't bother looking if table is still too small
		        bool found = lookup(start_end->second, rainbow, startpoint_f);
		        if(!found){
		            break;
		        }else{
		            discarded++;
		            delete start_end;																	
		        }		        
		    }while(inplace);
		    
		    rainbow -> push_back( start_end );							//  allocate pair dynamically and store pointer in container
		
		    if(v) cout << setw(6) << setprecision(2) << fixed
			     << progress*100 << "% complete..." << '\r' << flush; 
	    }
	    if(v) cout<<"100.00% complete... Done!" << endl;
        if(v && inplace){
            cout<<discarded<<" chains were discarded in the process" << endl;
        }	    
	}else{                                               ////////////////////// MULTI-THREADED ALGORITHM
	    long i, sub_m;
	    int id;
	    
	    Table_t** subtables = new Table_t*[threads];                    // Subtables allocated here, filled in threads
	    for(id=0 ; id<threads ; id++)   subtables[id] = new Table_t();
	                                                                        
	    pthread_t* workers = new pthread_t[threads];
	    vector<long> progress_bars(threads, 0);                         // Create a vector where all workers threads
                                                                        // will report their progress	    
	    long m_div = m / threads;                                             
	    int m_rem = int(m - long(m_div)*long(threads) );                // Calculate std. load per worker
	     
	    
	    for(id=0 ; id<threads ; id++){                                  // Workers identified by their index in the array 
	        sub_m = long((id ? m_div : m_div+m_rem));                   // First thread takes the rem. rows plus std. load
	        ThreadData* worker_args = new ThreadData();   			    // Allocate the struct to pass arguments for worker
            worker_args->my_id      = id;
            worker_args->my_m       = sub_m;
            worker_args->my_t       = t;
            worker_args->my_prog_v  = &progress_bars;
            worker_args->my_rainbow = subtables[id];
	        if( pthread_create( &(workers[id]), NULL, worker_thread, (void*)worker_args)){
	            char msg[64];
	            sprintf(msg, "Pthread_create failed for thread #%d.\nError descriptiion:",id);
	            perror(msg);
	            exit(2);
	        }    
	    }     
	    if(v) cout<<"Spawned thread  #0 with "<<m_div+m_rem<<" rows workload"<<endl   
                  <<"Spawned threads #1...#"<<threads-1<<" with "<<m_div<<" rows workload"<<endl;                                                              
    	                                                                
    	                                                                // After all workers are spawned...
	    do{                                                             // ...the main thread every once in a while...
	        sleep(10);
	         
	        pthread_mutex_lock(&vector_mutex);
	            long overall_progress=0;
	            for(id=0; id<threads ; id++)                            // ...estimates the overall progress (in absolute values)...
	                overall_progress += long(progress_bars[id]);
            pthread_mutex_unlock(&vector_mutex);                            
	         
		    float overall_progress_pct = (float)overall_progress / float(m) * 100;
		    cout << "Overall progress: "<<setw(6) << setprecision(2) << fixed
			     <<overall_progress_pct<< "% complete..." << '\r' << flush; 
			        
	        if(overall_progress==m) break;                              // ...and sleeps until it's over.
	    } while(true);
	    if(v) cout<<"Overall progress: 100.00% complete... Done!" << endl;

	    
	    
	    pair<char[7],char[7]>* temp;
	    if(v)
	        cout<<"Assembling table from subtables...";
	    for(id=0 ; id<threads ; id++){                                  // Then eventually, wait for all workers to exit 
	        pthread_join( workers[id],NULL );
	   //     if(v) cout<<"Thread #"<<id<<" exited"<<endl;
	        
	        sub_m = (id ? m_div : m_div+m_rem);
	        for(i=0; i<sub_m; i++){                                     // And concatenate their subtale to the final table	            
	            temp = subtables[id]->front();
	            subtables[id]->pop_front();
	            rainbow->push_back( temp );
	        } 
	    //    if(v) cout<<"\tTable now has "<<rainbow->size()<<" chains in total"<<endl;  
	    }
	    if(v) cout << " Done!" << endl;
	    delete[] workers;
	    delete[] subtables;
	}


	if(v) cout << "Sorting table... " << flush;			
	rainbow -> sort( pass_comparator );									// And return a sorted table (ASCENDING) , to prepare for binary searches.
	if(v) cout << "Done!" << endl;
	
	if(post && !inplace){
	    if(v) cout << "Post processing table... " << flush;			
	    rainbow -> unique( pass_predicate );							// Remove any chains having non-unique endpoints
	    long perfect = rainbow->size();
	    if(v){
	        cout << "Done!\n\t" << ( (float)m-(float)perfect )/(float)m * 100
	        <<"% of chains had same endpoints and were removed\n\tNew m="
	        <<perfect<<endl;
	    }
	} 
	
		
	return rainbow;
}	




void* worker_thread( void* args ){
    ThreadData* _args = (ThreadData*) args;
    
    int id       = _args->my_id;
    long my_subm = _args->my_m;                                         // Upon creation, worker thread gets 
    int t        = _args->my_t;                                         // the parameters passed to it
    vector<long>* progress_bars = _args->my_prog_v;
    Table_t*      my_rainbow    = _args->my_rainbow;
    delete _args;                                                       // Then frees the struct
    
    
    float my_prog =0.0, my_last_prog = 0.0;                                  
    char	cur_pwd[7];													
	uint8_t	cur_h[32];													
	char	passchar;
	char	startpoint[7];
	startpoint[6] = '\0';												// Startpoint string is a plain old C-string
	
	bool savechain_m = savechain && (id==0);                            // Chain printing statements will only 
	                                                                    // be executed for the first thread
	
	
                                                           
	pthread_t rseed = pthread_self(); 	                                // Initialize whatever PRNG is used
	int range_from  = 0;
	int range_to    = 63;
	default_random_engine           generator( (unsigned int)time(0)+pthread_self() );
	uniform_int_distribution<int>   distr(range_from, range_to); 
	
	for(long i=0 ; i<my_subm ; i++){
	    my_prog = (float)i / (float)my_subm;
		
		for(int j=0; j<6; j++){
		    int rand_idx;
		    if(!prng){
		        rand_idx = rand_r( (unsigned int*)&rseed );
		        passchar = alphabet[ rand_idx % 64 ];
		    }else{
		        passchar = alphabet[ distr(generator) ];    
		    }						
		    startpoint[j] = passchar;
		}	
		
		if(savechain_m && i<10)	outchain << "CHAIN #" << i+1 << endl;
		for(int j=1; j<=t-1 ; j++){										// For every link
		    if(j==1){
		   	    if(savechain_m && i<10)	outchain << startpoint;
		        hashfun( cur_h,(uint8_t*)startpoint,6 );			    // (1) HASH link's password
		    }else{
		         if(savechain_m && i<10)	outchain << cur_pwd;
		         hashfun( cur_h,(uint8_t*)cur_pwd,6 );	                //  or chain's starting point if it's the first	
		    } 
		    
		    if(savechain_m && i<10){									// Update output file if specified to do so 
			    outchain << " -> ";
			    for(int k=0; k<32; k++)
				    outchain << hex << setw(2) << setfill('0') << (int)cur_h[k] << dec ;
			    outchain << " -> " << endl;
			}
		        													
            redfun( cur_pwd,cur_h,j );									// (2) REDUCE previously computed hash using the correct red-fun
		}																// and start all over again
	    if(savechain_m && i<10)	outchain << cur_pwd << '\n' << endl;
		
		
		pair<char[7],char[7]>* start_end = new pair<char[7],char[7]>();	// Finally, create start-end pair,
	    strcpy( start_end -> first, startpoint);
	    strcpy( start_end -> second,   cur_pwd);																	
	    my_rainbow -> push_back( start_end );							//  allocate pair dynamically and store pointer in container
	
	    
	    
	    
	    if(my_prog >= my_last_prog + 0.1){                              // After every 10% of work is carried out 
	        pthread_mutex_lock(&vector_mutex);
	            progress_bars->at(id) = i;                              // Report the abs. value of work carried out so far
	        pthread_mutex_unlock(&vector_mutex);	        
            if(v) cout<<"Worker thread #"<<id<<": Now "<<int(my_prog*100)<<"% complete!  "<<endl;	        
            my_last_prog = my_prog;                                    // And update the watch
	    }
	}
	
	pthread_mutex_lock(&vector_mutex);                                  // Make a last report before exiting
        progress_bars->at(id) = my_subm;                             
    pthread_mutex_unlock(&vector_mutex);	
	
	if(v) cout<<"Worker thread #"<<id<<": Finished!           "<<endl;
	pthread_exit(NULL);           
}	
