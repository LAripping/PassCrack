/****************************** 	 A FEW TESTERS	 	 ********************************/

/*	// Reduction Function Tester  
int main(int argc, char **argv){												
	char 	*h_hexchar = argv[1];							// Initial (64-char) Hash given as single argument
	uint8_t	h[32];
	
	char byte[3];											// Per-byte parsing to uint8_t format
	for(int i=0 ; i<64; i=i+2){
		byte[0] = h_hexchar[i];
		byte[1] = h_hexchar[i+1];
		byte[2] = '\0';
		h[i/2] = (uint8_t)strtol(byte, NULL, 16);
	}	

	char pwd[7];
	my_red_functs_set( pwd,h,1 );
}
*/


/*	// Lookup/ReadTable/WriteTable Function Tester  
	
list< pair<char[7],char[7]>* >*	read_table( ifstream& intable );
bool							lookup(char* pwd, list<pair<char[7],char[7]>*>* rainbow, char* startpoint );
void 							my_red_functs_set( char* out, const uint8_t *in, int red_by );
void (*hashfun)( uint8_t *out, const uint8_t *in, uint64_t inlen )	= blake256_hash;
void (*redfun) ( char* out, const uint8_t *in, int red_by ) 		= my_red_functs_set;


bool savechain=false, v=true;
ofstream outchain;
	
	
int main(int argc, char **argv){							
	
	char *pwd = argv[1];									// Target password given as first argument		
	ifstream intable( argv[2] );							// File to import test-table from given as second parameter 
	if( intable.fail() ){
		cerr <<"Fuck!"<< endl;
		exit(1);
	}	
	
	list<pair<char[7],char[7]>*>* rainbow = read_table( intable );
	
	
	char* startpoint;
	bool res = lookup(pwd,rainbow,startpoint);
	if(res) cout << startpoint << endl;
	else	cout << "not found" << endl;
	
} */

/****************************** 	 THAT WAS ALL!	 	 ********************************/

