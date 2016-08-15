#include <iomanip>
#include <iostream>
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <ctime>

#include <string>
#include <vector>
#include <list>
#include <utility>
#include <unistd.h>

#include "blake.hpp"


using namespace std;

																		// Forward declarations to resolve dependancies
list< pair<char[7],char[7]>* >*	generate_tables(int m, int t);
list< pair<char[7],char[7]>* >*	read_table( ifstream& intable );
void							write_table( list<pair<char[7],char[7]>*>* rainbow,  ofstream& outtable );
void 							my_red_functs_set( char* out, const uint8_t *in, int red_by );
bool 							follow_chain( char* startpoint, char* password, int pos);
bool 							pass_comparator(const pair<char[7],char[7]>* first, const pair<char[7],char[7]>* second);	
bool							lookup(char* pwd, list<pair<char[7],char[7]>*>* rainbow, char* startpoint );


/*********** GLOBAL VARIABLES *************************************************/																	
vector<char*>	functs(1, "blake");										// Update array and pointers on future expansion 
void (*hashfun)( uint8_t *out, const uint8_t *in, uint64_t inlen )	= blake256_hash;
void (*redfun) ( char* out, const uint8_t *in, int red_by ) 		= my_red_functs_set;

bool			v=false , password_changed=false, savechain=false, savetable=false, readtable=false;
char*			f=functs[0];
ifstream		intable;
ofstream		outchain, outtable;






void	update_hash(int signo){ password_changed=true; }				// Signal Handler for SIGTSTOP to change the hash in real time
													

void usage(){
	std::cout << "Usage: PassCrack [Options]\n"
		<<"Options:\n"
		<<"\t-u            : Show usage\n"
		<<"\t-v            : Verbose, print extra messages during execution\n"
		<<"\t                  (used in debugging)\n"
		<<"\t-f <funct>    : Explicit definition of hash function to be used\n"
		<<"\t                  only \"blake\" currently supported.\n"
		<<"\t-i <intable>  : Pre-computed rainbow table to be imported from file <intable>.\n"
		<<"\t-o <outtable> : Export rainbow table for future use, in file <outtable>\n"
		<<"\t-s <outchain> : Save chains in their full-length in file <outchain>.\n"
		<<"\t-x            : Explicitly define chain variables M and T\n"
		<<"\t                for chains-per-table and links-per-chain respectively.\n"
		<<"\t                  Defaults: m=18, t=1000\n"
		<<"\t-t <tables>   : Number of tables to be generated\n"
		<<"\t                  only 1 currently supported." << endl ;
	return;	
}



int main(int argc, char** argv){
	
/*********** RAINBOW TABLE VARIABLES (same names as in countprob.py) **********/
int		tables=1;														// # of tables
int		m=18;															// # of chains per table
int		t=1000;															// # of links per chain
	
	
	
	
/********** OFFLINE PART **********/	
	
	char		c;
	bool		exp=false;

	
	while ((c = getopt(argc, argv, ":uvf:t:i:o:xs:")) != -1) {					// Parsing command line arguments 
   		switch(c) {
			case 'u':
				usage();
				exit(0);
			case 'v':
				v=true;
				cout << "Verbose mode enabled." << endl;
        		break;
       		case 'f':
   		 	   	f = optarg;
   		 	   	if( strcmp(f,"blake") ){
   		 	   		usage();
   		 	   		exit(0);
   		 	   	}	
   		// 	   	hashfun = & otherhashfun;									   In future expansion
   		 	    cout << "Using hash function "<< optarg << endl ;
    		    break;
    		case 'i':
    			readtable = true;
    			intable.open(optarg);
    			if( intable.fail() )
    				cerr << "Failed to open table input file!" << endl;   
    			if(v) cout << "Rainbow will be imported from file " << optarg << '.' << endl;	
    			break;	
    		case 'o':
				savetable = true;
				outtable.open(optarg, ofstream::trunc);						// If outtable already exists, discard old contents
    			if( outtable.fail() )
    				cerr << "Failed to open table output file!" << endl;  
    			cout << "Opened file " << optarg << " to export the rainbow table." << endl;			
    			break;	
    		case 's':
				savechain = true;
				outchain.open(optarg, ofstream::trunc);						// If outchain already exists, discard old contents 
    			if( outchain.fail() )
    				cerr << "Failed to open chain output file!" << endl;  
    			cout << "Opened file " << optarg << " to export the full chains." << endl;				
    			break;
    		case 'x':
    			exp = true;
				break;			
    		case 't':
    			tables = atoi(optarg);
    			if(tables!=1){
    				usage();
    				exit(0);
    			}		
    			cout << tables << "tables will be generated" << endl;	
    			break;
    			
    		case ':':
        		cerr << '-' << optopt << "without argument" << endl;
        		usage();
        		exit(1);
    		case '?':
        		cerr << '-' << optopt << "option not supported" << endl;
        		usage();
        		exit(1);
   			}
	}	
	if(v) cout << "Done parsing command line arguments." << endl;
	
	
	
	list<pair<char[7],char[7]>*>*		rainbow;							// Make array in future expansion								
	
	if( readtable ){
		rainbow = read_table( intable );
	}
	else	
	{	
		if(exp){
			cout << "How many chains per table?" << endl;
			cin  >> m;
			cout << "How many links per chain?" << endl;
			cin  >> t;
		}	
		time_t 		rawstart	= time(NULL);
		struct tm*	start 		= localtime( &rawstart );
		int startday  = start->tm_yday;
		int starthour = start->tm_hour;
		int startmin  = start->tm_min;
		int startsec  = start->tm_sec;
	
		rainbow = generate_tables(m, t);									// Create the tables, and time the process
	
		time_t 		rawend 		= time(NULL);
		struct tm*	end			= localtime( &rawend );
		int endday  = end->tm_yday;
		int endhour = end->tm_hour;
		int endmin  = end->tm_min;
		int endsec  = end->tm_sec;	
			
		if(v) cout << "Tables generated succesfully in\n\t " 
				<< endday-startday << " days " 	  << endhour-starthour << " hours "
				<< endmin-startmin << " minutes " << endsec-startsec   << " seconds." << endl;
		
		if(savetable)	write_table( rainbow, outtable );
	}		
	
	

	uint8_t	h[32], cur_h[32]; 											// |Hash| = 256 bits = 32 bytes = 64 hex chars 
	char 	cur_pwd[7];													// |Valid Password| = 6 chars long + '\0'
	static	struct sigaction act;
	
	act.sa_handler = update_hash;										// Install signal handler for SIGTSTP 	
	sigaction( SIGTSTP,&act,NULL );
	
	
	/********** ONLINE PART **********/	
	
	
	while( true ){														// if something goes wrong, restart with next initial hash don't quit.
		cout << "Enter password hash:" << endl;
		char h_hexchar[65];												// Initial hash entered as NULL-terminated c-string
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
		while(1){														// "Reduce-Lookup-Hash" cycle 
			
			if(v) cout << "Iteration # " << reps << '\r' << flush;		// Re-write on the previous line in screen (to keep only the # changing)
			
			if(cur_h==NULL)												// Reduction Function is different for every "rep"
				 redfun( cur_pwd,h,reps );								//   In the first loop reduce initial password hash,
			else redfun( cur_pwd,cur_h,reps );							//   In later ones reduce previously-computed hash

			
		//	if(v) cout << "Performing lookup in rainbow tables... ";
			char startpoint[7];
			bool found = lookup( cur_pwd, rainbow, startpoint);			// Search for the reduced output in chain-Endpoints			
			
			if( found ){												// But if we made a hit, unfold the chain from the beginning...
				char candidate_pwd[7];
				if(v) cout << endl << "Found current password as endpoint after "
							<< reps << " RLH cycles!" << endl << "The startpoint is " << startpoint << endl;
				
				follow_chain( startpoint, candidate_pwd, reps );		// ...until t-reps link to get candidate password! 
				cout << "CANDIDATE PASSWORD:\n" << candidate_pwd << endl;
				if(v) cout << "Hashing again..." << endl;				// In spite of the result, continue the cycle
			}
			
			hashfun( cur_h,(uint8_t*)cur_pwd,6 );
			reps++;		

			if( password_changed ){										// After every iteration check if dbus::GeneratePassSignall was caught
				cout << endl << "New password generated... " << endl; 
				password_changed = false;
				break;
			}	
		}
		
	}
	
}	
	

	
	
	
	
