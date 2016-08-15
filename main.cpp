#include <iomanip>
#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <ctime>
#include <cmath>

#include <string>
#include <vector>
#include <list>
#include <unordered_map>


#include <utility>
#include <unistd.h>

#include "blake.hpp"


using namespace std;
typedef list< pair<char[7],char[7]>* >  Table_t;
typedef unordered_map< string,string >  HashTable_t;

																		// Forward declarations to resolve dependancies
Table_t*	read_table( ifstream& intable );
void		write_table( Table_t* rainbow,  ofstream& outtable );

void    red_functs_set1( char* out, const uint8_t *in, int red_by );
void	red_functs_set2( char* out, const uint8_t *in, int red_by );
void	red_functs_set3( char* out, const uint8_t *in, int red_by );
void	red_functs_set4( char* out, const uint8_t *in, int red_by );
void	red_functs_set5( char* out, const uint8_t *in, int red_by );

HashTable_t*    build_hashmap(Table_t* rainbow, float load_factor);
Table_t*	    generate_tables(long m, int t, int threads, bool post);

double          success_prob(long m,    int t, int l, long  long N);
bool 		    follow_chain(uint8_t hash[32], char* startpoint, char* password, int t);
bool		    lookup(char* pwd, Table_t*     rainbow,     char* startpoint );
bool		    lookup(char* pwd, HashTable_t* rainbow_map, char* startpoint );



/*********** GLOBAL VARIABLES *************************************************/																	
void (*hashfun)( uint8_t *out, const uint8_t *in, uint64_t inlen )	= blake256_hash;
void (*redfun) ( char* out, const uint8_t *in, int red_by ) 		= red_functs_set3;

bool			v=false , password_changed=false, savechain=false, savetable=false, readtable=false, inplace=false;
bool            prng=false;
ifstream		intable;
ofstream		outchain, outtable;






void	update_hash(int signo){ password_changed=true; }				// Signal Handler for SIGTSTOP to change 
													                    // the hash in real time

void usage(){
	std::cout << "Usage: PassCrack [Options]\n"
		<<"Options:\n"
		<<"\tonline/offline     : Choose operation mode (enable/disable reduce-lookup-hash cycles)\n"
		<<"\t                     Defalut: online mode\n"
		<<"\t-u                 : Show usage.\n"
		<<"\t-v                 : Verbose, print extra messages during execution\n"
		<<"\t                       (used in debugging)\n"
		<<"\t-c                 : Calculate the success probability.\n"
		<<"\t-g                 : Use the C++11 robust PRNG in table generation\n"
		<<"\t-q                 : Only keep chains with unique endpoints.\n"
		<<"\t                      Post-process table and discard duplicate endpoints\n"
		<<"\t                      Using this option results in m'<m. Use -Q for consistency\n"
		<<"\t-Q                 : Like -q, but each chain's uniqueness is evaluated right after creation\n"
		<<"\t                      using this option guarantees m'=m with an extra time cost.\n"
		<<"\t                      Option only takes effect if generation is single threaded!\n"
		<<"\t-p <thrds>         : Parallelize table generation using <thrds> threads.\n"
		<<"\t                       If -s is specified only the first thread's chains will be printed\n"
		<<"\t-r <redfunset>     : Select the set of reduction functions to be used\n"
		<<"\t                       Default: 3\n"
		<<"\t-h <load>          : Convert (post-processed) table to hash-map before lookups\n"
		<<"\t                       Option only meaningfull in online mode\n"
		<<"\t                       Hashing will result in avg. <load> elements/bucket. \n"		
		<<"\t-i <intable>       : Pre-computed rainbow table to be imported from file <intable>.\n"
		<<"\t-o <outtable>      : Export rainbow table for future use, in file <outtable>\n"
		<<"\t-s <outchain>      : Save 10 first chains in their full-length in file <outchain>.\n"
		<<"\t                       (used for testing online mode)\n"
		<<"\n"
		<<"\t-m <chains>        : Explicitly define chain variables M and T\n"
		<<"\t-t <links>             for chains-per-table and links-per-chain respectively.\n"
		<<"\t                       Defaults: m=100, t=32\n"
		<<""           
		<<"\t-l <tables>        : Number of tables to be generated\n"
		<<"\t                       only 1 currently supported." << endl ;
	return;	
}



int main(int argc, char** argv){
	
/*********** RAINBOW TABLE VARIABLES (same names as in countprob.py) **********/
int		tables=1;														// # of tables
long	m=100;															// # of chains per table
int		t=32;															// # of links per chain/reduction functions
	
	
	
	
/********** OFFLINE PART **********/	
	
	char		c;
	bool		do_calc=false, online=true, post=false, hashmap=false;
	int         threads=1, set;
	char        outchain_file[128];
	float       hash_load_factor = 100.0;

	for(int i=1; i<argc; i++){
	    if( !strcmp(argv[i],"offline") ){    
            online=false; 
	        break;
	    }
	}
	if(!online)
	    cout<<"Offline mode selected, originated hashes won't be requested"<<endl;         
    else
	    cout<<"Online mode selected"<<endl; 
	         	        
	                
	while ((c = getopt(argc, argv, ":uvcgqQt:i:o:s:m:l:p:h:r:")) != -1) {			
   		switch(c) {
			case 'u':
				usage();
				exit(0);
			case 'v':
				v=true;
				cout << "Verbose mode enabled" << endl;
        		break;
        	case 'h':
    			hashmap=true;
        		hash_load_factor = atof(optarg);
    			if(hash_load_factor<=0){
    				cout << "Please specify an apropriate load factor!" << endl;
    				usage();
    				exit(0);
    			}	    			
        		break;	
        	case 'c':
				do_calc = true;
				break;
			case 'q':
			    post = true;
			    if(v) cout<<"Chains with common endpoints will be removed " << endl;	
				break;
			case 'Q':
			    inplace = true;
				break;										
			case 'g':
				prng=true;
				cout << "The sophisticated C++11 PRNG will be used in table generation" << endl;
        		break;
        	case 'i':
    			readtable = true;
    			intable.open(optarg);
    			if( intable.fail() ){
    				cerr << "Failed to open table input file!" << endl; 
    				exit(1);
    			}	  
    			if(v) cout << "Rainbow will be imported from file " << optarg << '.' << endl;	
    			break;	
    		case 'o':
				savetable = true;
				if( ifstream(optarg) ){
				    cout << "Output file \" " << optarg << "\" already exists! Overwrite? (y/n): ";
				    char ans;
				    cin >> ans;
				    if(ans != 'y') exit(0);
				}    
				outtable.open(optarg, ofstream::trunc);				
    			if( outtable.fail() ){
    				cerr << "Failed to open table output file!" << endl;  
    				exit(1);
    			}
    			cout << "Opened file " << optarg << " to export the rainbow table." << endl;			
    			break;	
    		case 's':
				savechain = true;
				strncpy(outchain_file, optarg, strlen(optarg));
				outchain_file[strlen(optarg)] = '\0';    
				if( ifstream(optarg) ){
				    cout << "Output file \" " << optarg << "\" already exists! Overwrite? (y/n): ";
				    char ans;
				    cin >> ans;
				    if(ans != 'y') exit(0);
				} 				
				outchain.open(optarg, ofstream::trunc);					// If outchain already exists, discard old contents 
    			if( outchain.fail() )
    				cerr << "Failed to open chain output file!" << endl;  
    			break;
    		case 'm':
    			m = atol(optarg);
    			if(m<=1){
    				cout << "Please specify an apropriate number of chains!" << endl;
    				usage();
    				exit(0);
    			}		
    			cout<<"Table will contain "<<m<<" chains in total"<<endl;	
    			break;		
    		case 't':
    			t = atoi(optarg);
    			if(m<=1){
    				cout << "Please specify an apropriate number of links!" << endl;
    				usage();
    				exit(0);
    			}		
    			cout<<"Each chain will contain "<<t<<" links"<<endl;	
    			break;
    		case 'l':
    			tables = atoi(optarg);
    			if(tables!=1){
    				usage();
    				exit(0);
    			}		
    			cout << tables << "tables will be generated" << endl;	
    			break;
    		case 'p':
    			threads = atoi(optarg);
    			if(threads<=1){
    				cout << "Please specify an apropriate thread number!" << endl;
    				usage();
    				exit(0);
    			}		
    			cout<<"Table generation will be parallelized using "<<threads<<" threads"<<endl;	
    			break;
    		case 'r':
    		    set = atoi(optarg); 	    			    
    			switch(set){
    			    case 1: redfun = red_functs_set1; break;
    			    case 2: redfun = red_functs_set2; break;
    			    case 3: redfun = red_functs_set3; break;
    			    case 4: redfun = red_functs_set4; break; 
      			    case 5: redfun = red_functs_set5; break; 
    			    default:
    			        cout << "Available reduction-function sets: 1-5!" << endl;
    				    usage();
    				    exit(0); 			    
    			}   
    			cout << "Reduction function set #"<<set<<" selected"<<endl;
    			break;
    	    case ':':
        		cerr << '-' << optopt << "without argument" << endl;
        		usage();
        		exit(1);
    /*		case '?':
        		cerr << '-' << optopt << "option not supported" << endl;
        		usage();
        		exit(1);    */
   			}   
	}	
	
	
	Table_t* rainbow;						                    
	
	if(do_calc){
	    long long N = pow(64,6);
	    double prob = success_prob(m,t,tables,N);
	    printf("Given m=%ld chains/table, t=%d links/chain, l=%d tables and a keyspace of N=%lld \n",\
	                m, t, tables, N );
	    printf("the calculated success probability is %f (%d%%)\n", prob, int(prob*100) );
	}
	if(savechain){    			
	    if(threads>1)
	        cout<<"Opened "<< outchain_file 
	            <<".\nOnly the 10 first chains generated by the first thread will be exported"<<endl;
	    else    				
	        cout << "Opened file " << outchain_file<< " to export the 10 first full chains." << endl;
	}        				
	if(threads>1 && readtable){
        cout << "No point specifying thread number if table is read!" << endl;  
	} 	
	if(inplace){        // If inplace uniqueness checking is requested
	    if(threads!=1){
	        cout << "-Q option only available in single-threaded table generation!" << endl;
	        usage();
	        exit(0);
	    }else{
	        cout << "Chain uniqueness will be evaluated right after creation!" << endl;
        }
        if(savechain){
	        cout << "-Q option can't be combined with -s!" << endl;
	        usage();
	        exit(0);
	    }
    }    	         
	if(online && hashmap)
	    cout<<"Table will be converted to a hash-map with a "<<hash_load_factor<<" load factor"<<endl;        
	
	if(!online && !savetable){
	    cout << "Table will be wasted!" << endl;
	    usage();
	    exit(0);    
	}
	    
	if( readtable ){
		rainbow = read_table( intable );
	}
	else{			
		time_t 		rawstart	= time(NULL);
		struct tm*	start 		= localtime( &rawstart );
		int startday  = start->tm_yday;
		int starthour = start->tm_hour;
		int startmin  = start->tm_min;
		int startsec  = start->tm_sec;
	
		rainbow = generate_tables(m, t, threads, post);					// Create the tables, and time the process
	
		time_t 		rawend 		= time(NULL);
		struct tm*	end			= localtime( &rawend );
		int endday  = end->tm_yday;
		int endhour = end->tm_hour;
		int endmin  = end->tm_min;
		int endsec  = end->tm_sec;	
		
		int diffday = endday-startday;									// Deal with the carries in subtractions
		int diffhour= endhour-starthour;
		if( diffhour<0 ){
			diffday--;
			diffhour = 24 + diffhour;
		}
			
		int diffmin = endmin-startmin;									// (This could be made very simple by just
		if( diffmin<0 ){												//  keeping seconds and performing DIVs and MODS..)
			diffhour--;
			diffmin = 60 + diffmin;
		}	
		
		int diffsec = endsec-startsec;
		if( diffsec<0 ){
			diffmin--;
			diffsec = 60 + diffsec;
		}	 
		
		if(v) cout << "Tables generated/sorted"
		        << (post?"/filtered":"") <<" succesfully in\n\t " 
				<< diffday << " days " 	  << diffhour << " hours "
				<< diffmin << " minutes " << diffsec   << " seconds." << endl;
				
		if(savetable)	write_table( rainbow, outtable );
	}		   

	uint8_t	h[32], cur_h[32]; 											// |Hash| = 256 bits = 32 bytes = 64 hex chars 
	char 	cur_pwd[7];													// |Valid Password| = 6 chars long + '\0'
	static	struct sigaction act;
	
	act.sa_handler = update_hash;										// Install signal handler for SIGTSTP 	
	sigaction( SIGTSTP,&act,NULL );
	
	
	/****************************************** ONLINE PART **********************************************************/	
	
	HashTable_t* rainbow_map; 
	if(hashmap){
	    if(v)   cout << "Converting to hash-map..." << endl;        
	    rainbow_map = build_hashmap(rainbow, hash_load_factor);
	    if(v){
	        cout << setw(6) << setprecision(2) << fixed
			     << 100.00 << "% complete... Done!" << endl;            // Now print some stats
	        
	        cout <<"\t"<<rainbow->size()-rainbow_map->size()
	             <<" std::hash collisions were encountered. New m="<<rainbow_map->size()<<endl;
            cout <<"\tThe resulting map has "<<rainbow_map->bucket_count()<<" buckets."<<endl;
            cout <<"\tThat's a "<< setw(6) << setprecision(2) << fixed 
                 << rainbow_map->load_factor()<<" load factor!"<< endl;
        }         
	}
	
	while( online ){													// if something goes wrong, restart with next initial hash don't quit.
		cout << "Enter password hash:" << endl;
		char h_hexchar[65];												// Initial hash entered as NULL-terminated c-string
		cin.clear();
	//	cin.ignore(9999);
		cin >> h_hexchar;
		
		char byte[3];													// Per-byte parsing to uint8_t format
		for(int i=0 ; i<64; i=i+2){
			byte[0] = h_hexchar[i];
			byte[1] = h_hexchar[i+1];
			byte[2] = '\0';
			h[i/2] = (uint8_t)strtol(byte, NULL, 16);
		}
		
	/*	if(v){
			cout << "Initial hash in uint8_t[32] format:" << endl;
			for(int i=0; i<32 ; i++) 
				cout << hex << setw(2) << setfill('0') << (int)h[i] << dec ;
			cout << endl;
		}
	*/			
		

		int reps = 1;
		char startpoint[7], candidate_pwd[7];
        bool found, retrieved;
		while(reps<=t){												    // "Reduce-Lookup-Hash" cycle 
			
			if(v) cout << "Iteration #" << reps << '\r' << flush;		// Re-write on the previous line in screen (to keep only the # changing)

            int i=t-reps;                                              
            redfun(cur_pwd, h, i);                                      // In the first loop reduce initial password hash,       
            while(i<t-1){                                               // In later ones compute the hash then reduce as many times needed
                hashfun( cur_h,(uint8_t*)cur_pwd,6 );                       // cur_h is re-calculated here after every reductipn but the first
			    i++;
                redfun(cur_pwd, cur_h, i);                                  // cur_pwd is re-calculated here in every reduction                
			}
			
		//	if(v) cout << "Performing lookup in rainbow tables... ";

			
			if(hashmap)                                                 // Search for the reduced output in chain-Endpoints
			    found = lookup( cur_pwd, rainbow_map, startpoint);	
			else
			    found = lookup( cur_pwd, rainbow,     startpoint );    					
			
			
			if( found ){												// But if we made a hit, recreate the chain from its beginning...
				if(v) cout << endl << "Found current password as endpoint after "
							<< reps << " iterations!" << endl << "The startpoint is " << startpoint << endl;
				
				retrieved = follow_chain( h, startpoint, candidate_pwd, t);	        
				if(retrieved)
				    cout << "CANDIDATE PASSWORD:\n" << candidate_pwd << endl;
				else
				    if(v) cout << "Originating hash was not encountered while recreating chain!"<<endl;    
                break;                                                  // ...until you get the original hash! (candidate retrieved) 
			}                                                           // ...or a colision/merge/error and a candidate was not found :(
			
			hashfun( cur_h,(uint8_t*)cur_pwd,6 );     
			reps++;		

			if( password_changed ){										// After every iteration check if password_changed
				cout << endl << "New password generated... " << endl; 
				password_changed = false;
				break;
			}	
		}
		
		if( reps>t ) cout << "Password for this hash was not found in the rainbow table." << endl
		                << "Attack failed after " << t << " iterations\n" << endl;
	}
	exit(0);
}	
	

	
	
	
	
