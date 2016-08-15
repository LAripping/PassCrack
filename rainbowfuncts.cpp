#include <iostream>
#include <iomanip>
#include <utility>
#include <fstream>

#include <list>
#include <vector>

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>

#include <unistd.h>
#include <pthread.h>


#include "blake.hpp"



using namespace std;
typedef list<pair<char[7],char[7]>*> Table_t;


char alphabet[64] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '@', 		\
		   			 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',	\
					 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',	\
					 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',	\
					 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', };




extern void (*hashfun)( uint8_t *out, const uint8_t *in, uint64_t inlen );
extern void (*redfun) ( char* out, const uint8_t *in, int red_by );
extern bool		v, savechain;
extern ofstream	outchain;



/*
 * My reduction-functions' set.
 * Determined by the index "red_by" of the function selected from the set
 * 6 bytes are chosen from the hash.
 *
 * First their msb is unset to match the old 7bit ASCII format.
 * Then each byte is mapped to a character from the password's alphabet 
 *
 * Redfun #1:  takes bytes 0-5 (red_by>=1)
 *  -\\-  #2:  takes bytes 1-6
 *  -\\-  #3:  takes bytes 2-7 
 * ...
 * Redfun #32  takes bytes 31-4
 *
 * After 32 modulo indexing is used so #33 chooses 0-5 and so on.
 *
 * In first rep Rt is used, in second Rt-1,H,Rt, then Rt-2,H,Rt-1,H,Rt, and so it goes
 * until all function are used, then the attack fails
 *
 */
void my_red_functs_set( char* out, const uint8_t *in, int red_by ){													
	int 	o_idx=0, i_idx;
	uint8_t byte, mask;
	
	for(int i=0 ; i<6 ; i++){																
		i_idx = (i + red_by - 1) % 32;						// Select input's byte to map
		byte  = in[ i_idx ];									
		
		mask = 0x7f;										// Create byte 01111111 to perform bitwise AND with hash's byte...
		byte &= mask;										// To unset hash-byte's 8th lefmost (msb) bit.
		
						
		if( ( 0x2f< byte && byte<=0x39 )					// Map them to a specific character in password's alphabet
		||  ( 0x41<=byte && byte <0x5b ) 
		||  ( 0x61<=byte && byte <0x7b ) )						
			out[o_idx] = (char) byte;						// This character is usually the one whose ASCII code 
		                                                    // equals the byte's value
		else if((byte>=0x3a && byte<=0x41)  
			||  (byte>=0x7b && byte<=0x7f) )
			out[o_idx] = '@';
		
		else if((byte>=0x00 && byte<=0x2f)
			||	(byte>=0x5b && byte<=0x60) )
			out[o_idx] = '!';
															// In case i forgot a case!
		else												// Control should never come here!
			cerr << "Case not supported!" << (int)byte << endl;	
				
		o_idx++;
				
	}
	out[o_idx] = '\0';										// NULL terminate it to process as C-string
				
	
//	if(v) cout<< "Hash reduced to \"" << out << "\" in password's domain." << endl;	
}




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

pthread_mutex_t  vector_mutex = PTHREAD_MUTEX_INITIALIZER;

void* worker_thread( void* args );

Table_t* generate_tables(long m, int t, int threads){
	float progress = 0;
	char	cur_pwd[7];													// Both hash and reduction functions take
	uint8_t	cur_h[32];													//  preallocated buffers in arguments and fill them.
	char	passchar;
	char	startpoint[7];
	startpoint[6] = '\0';												// Startpoint string is a plain old C-string
	srand( time(NULL) );	
	
																		// Allocate rainbow table structure 
	Table_t* rainbow = new Table_t();	
	
	
	if( v==true && (m*t>1000) ) 				
		cout << "Generating tables... (This could take a while!)" << endl; 
	                                                ////////////////////// SINGLE-THREADED ALGORITHM
	if(threads==1){	                                
	    for(long i=0; i<m ; i++){										// For every chain		
		    progress = (float)i / (float)m;
		
		    for(int j=0; j<6; j++){
			    passchar = alphabet[ rand() % 64 ];						// Start from a random password
			    startpoint[j] = passchar;
		    }	
		
		    if(savechain && i<10)	outchain << "CHAIN #" << i+1 << endl;
		    for(int j=1; j<=t-1 ; j++){									// For every link
			    if(j==1){
				    if(savechain && i<10)	outchain << startpoint;
				    hashfun( cur_h,(uint8_t*)startpoint,6 );			// (1) HASH link's password
			    }														//  or chain's starting point if it's the first
			    else{
				    if(savechain && i<10)	outchain << cur_pwd;
				    hashfun( cur_h,(uint8_t*)cur_pwd,6 );															
			    }
			
			    if(savechain && i<10){  								// Update output file if specified to do so 
				    outchain << " -> ";
				    for(int k=0; k<32; k++)
					    outchain << hex << setw(2) << setfill('0') << (int)cur_h[k] << dec ;
				    outchain << " -> " << endl;
			    }	
				
				
			    redfun( cur_pwd,cur_h,j );								// (2) REDUCE previously computed hash using the correct red-fun
		    }															// and start all over again
		    if(savechain && i<10)	outchain << cur_pwd << '\n' << endl;
		
		    pair<char[7],char[7]>* start_end = new pair<char[7],char[7]>();	// Finally, create start-end pair,
		    strcpy( start_end -> first, startpoint);
		    strcpy( start_end -> second,   cur_pwd);																	
		    rainbow -> push_back( start_end );							//  allocate pair dynamically and store pointer in container
		
		    if(v) cout << setw(6) << setprecision(2) << fixed
			     << progress*100 << "% complete..." << '\r' << flush; 
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
	        sleep(2);
	         
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
	    
	    pair<char[7],char[7]>* temp;
	    if(v) cout<<"\nAssembling table from subtables..."<<endl;
	    for(id=0 ; id<threads ; id++){                                  // Then eventually, wait for all workers to exit 
	        pthread_join( workers[id],NULL );
	        if(v) cout<<"Thread #"<<id<<" exited"<<endl;
	        
	        sub_m = (id ? m_div : m_div+m_rem);
	        for(i=0; i<sub_m; i++){                                     // And concatenate their subtale to the final table	            
	            temp = subtables[id]->front();
	            subtables[id]->pop_front();
	            rainbow->push_back( temp );
	        } 
	        if(v) cout<<"\tTable now has "<<rainbow->size()<<" chains in total"<<endl;  
	    }
	    if(v) cout << "Done!" << endl;
	    delete[] workers;
	    delete[] subtables;
	}


	if(v) cout << endl << "Sorting table... " << flush;			
	rainbow -> sort( pass_comparator );									// And return a sorted table (ASCENDING) , to prepare for binary searches.
	if(v) cout << "Done!" << endl;
	
		
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
   	pthread_t rseed = pthread_self();                                   // Thread unsafe "rand" replaced here with "rand_r"
    char	cur_pwd[7];													
	uint8_t	cur_h[32];													
	char	passchar;
	char	startpoint[7];
	startpoint[6] = '\0';												// Startpoint string is a plain old C-string
	
	bool savechain_m = savechain && (id==0);                            // Chain printing statements will only 
	                                                                    // be executed for the first thread
	for(long i=0 ; i<my_subm ; i++){
	    my_prog = (float)i / (float)my_subm;
		
		for(int j=0; j<6; j++){
		    int rand_idx = rand_r( (unsigned int*)&rseed );
		    passchar = alphabet[ rand_idx % 64 ];						// Start from a random password
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
		


void write_table( Table_t* rainbow,  ofstream& outtable ){
	
	outtable << "TABLE 1" << endl;
	
	for( pair<char[7],char[7]>* pp : *rainbow )
		outtable << pp->first << " -> " << pp->second << endl;
	
	outtable.put('\n');
}	




Table_t* read_table( ifstream& intable ){
	
	Table_t* rainbow = new Table_t();	
	char line[24];
	
	intable.getline(line, 9);									// Skip "TABLE #1\n"
	while(! intable.eof() ){
		intable.getline(line, 24);						
		if( strlen(line)<6 ) break;
		
		pair<char[7],char[7]>* pp = new pair<char[7],char[7]>();
		strncpy( pp->first, line, 6);
		pp->first[6] = '\0';
		
		strncpy( pp->second, line+10, 6);
		pp->second[6] = '\0';
		rainbow->push_back( pp );
	}			 	
	
	if(v){
		cout << "Table imported succesfully\nIt now contains: " 
			<< rainbow->size() << " chains" << endl;
	
		ofstream out("whatwasread.txt", ofstream::trunc);
		write_table(rainbow,out);
	}	
	
	return rainbow;
}	





/*
 * Seeker function to operate on sorted rainbow table.
 *
 * Given a target "pwd" password, a binary search is performed 
 * in endpoints for the first exact match.
 * 
 * If found it returns TRUE, and sets "startpoint" to point
 * at the beggining of the chain, ending in "pwd".
 *
 * If not found, FALSE is returned and "startpoint" is untouched.
 *
 */
bool lookup(char* pwd, Table_t* rainbow, char* startpoint){	
																		// Implicitly assigning ceil[ size/2 ] 
	int	i;
	int	base_index		= 1;
	int	middle_index	= rainbow->size() / 2;
	int	top_index		= rainbow->size();		
	Table_t::iterator it= rainbow->begin();	

	pair<char[7],char[7]>					*middle_pair;				// Pivot-pointer pair
	pair<char[7],char[7]>					*dummy_pair = new pair<char[7],char[7]>();
	strcpy( dummy_pair->second, pwd );									
																		// Create dummy pair, with only an endpoint
																		//  that will be passed to comparator 
																																			
	for(i=base_index ; i!=middle_index ; i++,it++) ;					// Move to the middle of the initial-list
	

	while( true ){														// Split list to halves repeatedly	
		middle_pair = *it;												// Set the pivot-pointer 
		if( middle_index==base_index ||
			middle_index==top_index ) break;	 						//  and break loop for the final comparison 
																		//  if list is reduced to a single element
																		
	/*	if(v){
			sleep(3);
			cout << "[*it,base,middle,top] = [ "
				<< (*it)->second << ' ' << base_index << ' ' 
				<< middle_index << ' ' << top_index <<']' << endl;																
		}
	*/			
																		
		 
		if(! strcmp(middle_pair->second,pwd) ){							// If pwd = middle, return results
			strcpy( startpoint, middle_pair->first);
			delete dummy_pair;
			return true;
		}
		else if ( pass_comparator(dummy_pair,middle_pair) ){			// If pwd < middle
		//	base_index = base_index;									// Update indexes
			top_index = middle_index;	
			middle_index = base_index + (top_index - base_index) / 2;							
			for( ; i!=middle_index ; i--,it-- )	;						// Drop pivot to new middle
			continue;													// Loop again
		}
		else{															// If pwd > middle, loop again for the bigger-ones half
			base_index = middle_index;									// Update indexes
		//	top_index = top_index;	
			middle_index = base_index + (top_index - base_index) / 2;			
			for(i=base_index ; i!=middle_index ; i++,it++) ;			// Raise pivot to new middle
			continue;													// Loop again
		}
	}
	
	delete dummy_pair;
	if(! strcmp(middle_pair->second,pwd) ){								// Make one last comparison to decide.
		strcpy( startpoint, middle_pair->first);
		return true;
	}
	else{
		return false;
	}		
}





/*
 * "Hash-Reduce cycle" function to reproduce chain.
 *
 * This function is called when a hit is found in the rainbow
 * table's endpoints for the given hash. 
 * 
 * It repeatedly switches the startpoint from the hash-domain
 * to the password-domain until a hash identical to the original one
 * is computed. The password that resulted to it is returned in the 
 * preallocated character buffer "password" for the user to try.
 *
 */
bool follow_chain( uint8_t hash[32], char* startpoint, char* password, int t){

	uint8_t	cur_h[32];
	char	cur_pwd[7];


	for(int i=1; i<t+10 ; i++){												
		if(i==1)	hashfun( cur_h,(uint8_t*)startpoint,6 );
		else		hashfun( cur_h,(uint8_t*)cur_pwd,6 );				
		
		if( memcmp(cur_h,hash,sizeof(cur_h))==0 ){
		    strcpy(password, cur_pwd);	
		    if(v)
		        cout<<"After applying red-fun #"<<i-1<<", the hashed"
		            <<"output matches the originating one!"<<endl;
		    return true;    
		}
		
		redfun( cur_pwd,cur_h,i );
	}
	return false;
}




/*
 * Success probability calculator,
 * based on the Oechslin formula for
 * standard rainbow tables
 *
 */
double success_prob(long m, int t, int l, long long N){
    double *mi = new double[t];
    
    mi[0] = m;
    for(int i=1 ; i<t ; i++)
        mi[i] = N * (1-exp( -mi[i-1]/N ));
        
    double prob = 1;
    for(int i=0 ; i<t ; i++)
        prob *= (1 - (mi[i]/N));  
        
    delete[] mi;      
        
    return (1 - pow(prob,(float)l)  ) ;    
}
















