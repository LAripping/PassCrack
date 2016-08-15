#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <utility>
#include <unistd.h>
#include <list>

#include "blake.hpp"


using namespace std;



char alphabet[64] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '@', 		\
		   			 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',	\
					 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',	\
					 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',	\
					 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', };


extern void (*hashfun)( uint8_t *out, const uint8_t *in, uint64_t inlen );
extern void (*redfun) ( char* out, const uint8_t *in, int red_by );
extern bool		v, savechain;
extern ofstream	outchain;



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




void my_red_functs_set( char* out, const uint8_t *in, int red_by ){
	
	switch( red_by ){										// Select reduction function by #
		case 1:												// Currently only #1 implemented
		{													
			int 	j=0;
			bool 	repeat=false;
			uint8_t byte, mask;
			for(int i=0 ; i<6 ; i++){						// For each byte of the first 6 in the hash (32 in total)									
				byte = in[i];
				mask = 0x7f;								// Create byte 01111111 to perform bitwise AND with hash's byte.
				byte &= mask;								// Do not consider hash-byte's 8th lefmost (msb) bit, unset it!
				
								
				if( ( 0x2f< byte && byte<=0x39 )			// Map them to a specific character in password's alphabet
				||  ( 0x41<=byte && byte <0x5b ) 
				||  ( 0x61<=byte && byte <0x7b ) )						
					out[j] = (char) byte;					// This character is usually the one whose ASCII code equals the byte's value
				
				else if((byte>=0x3a && byte<=0x41)  
					||  (byte>=0x7b && byte<=0x7f) )
					out[j] = '@';
		
				else if((byte>=0x00 && byte<=0x2f)
					||	(byte>=0x5b && byte<=0x60) )
					out[j] = '!';
				
				else										// In case i forgot a case!
					cerr << "Case not supported!" << (int)byte << endl;	// Control should never come here!
						
				j++;
				
			}
			out[j] = '\0';									// NULL terminate it to process as C-string
			break;
		}
		default :
			cout << "Reduction Function #" << red_by << " not yet implemented!";
			exit(2);	
	}			
	
//	if(v) cout<< "Hash reduced to \"" << out << "\" in password's domain." << endl;		That's way too verbose!!
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




list< pair<char[7],char[7]>* >*	generate_tables(int m, int t){
	char	cur_pwd[7];													// Both hash and reduction functions take
	uint8_t	cur_h[32];													//  preallocated buffers in arguments and fill them.
	char	passchar;
	char	startpoint[7];
	startpoint[6] = '\0';												// Startpoint string is a plain old C-string
	srand( time(NULL) );	
																		// Allocate rainbow table structure 
	list<pair<char[7],char[7]>*>* rainbow = new list<pair<char[7],char[7]>*>();	
		
	for(int i=0; i<m ; i++){											// For every chain
		for(int j=0; j<6; j++){
			passchar = alphabet[ rand() % 64 ];							// Start from a random password
			startpoint[j] = passchar;
		}	
		
		if(savechain)	outchain << "CHAIN #" << i+1 << endl;
		for(int j=1; j<=t ; j++){											// For every link
			if(j==1){
				if(savechain)	outchain << startpoint;
				hashfun( cur_h,(uint8_t*)startpoint,6 );					// (1) HASH link's password
			}																//  or chain's starting point if it's the first
			else{
				if(savechain)	outchain << cur_pwd;
				hashfun( cur_h,(uint8_t*)cur_pwd,6 );															
			}
			
			if(savechain){													// Update output file if specified to do so 
				outchain << " -> ";
				for(int k=0; k<32; k++)
					outchain << hex << setw(2) << setfill('0') << (int)cur_h[k] << dec ;
				outchain << " -> " << endl;
			}	
				
				
			redfun( cur_pwd,cur_h,1 );										// (2) REDUCE previously computed hash
		}																	// and start all over again
		if(savechain)	outchain << cur_pwd << '\n' << endl;
		
		pair<char[7],char[7]>* start_end = new pair<char[7],char[7]>();		// Finally, create start-end pair,
		strcpy( start_end -> first, startpoint);
		strcpy( start_end -> second,   cur_pwd);																	
		rainbow -> push_back( start_end );									//  allocate pair dynamically and store pointer in container
	}
		
	rainbow -> sort( pass_comparator );										// And return a sorted table (ASCENDING) , to prepare for binary searches.
		
	return rainbow;
}	
	
		


void write_table( list<pair<char[7],char[7]>*>* rainbow,  ofstream& outtable ){
	
	outtable << "TABLE 1" << endl;
	
	for( pair<char[7],char[7]>* pp : *rainbow )
		outtable << pp->first << " -> " << pp->second << endl;
	
	outtable.put('\n');
}	




list< pair<char[7],char[7]>*>*	read_table( ifstream& intable ){
	
	list<pair<char[7],char[7]>*>*	rainbow = new list<pair<char[7],char[7]>*>();	
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
bool lookup(char* pwd, list<pair<char[7],char[7]>*>* rainbow, char* startpoint){	
																		// Implicitly assigning ceil[ size/2 ] 
	int										i;
	int										base_index		= 1;
	int										middle_index	= rainbow->size() / 2;
	int										top_index		= rainbow->size();		
	list<pair<char[7],char[7]>*>::iterator	it 				= rainbow->begin();	

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
	
	if(! strcmp(middle_pair->second,pwd) ){								// Make one last comparison to decide.
		strcpy( startpoint, middle_pair->first);
		return true;
	}
	else{
		return false;
	}		
}





/*
 * "Hash-Compare-Reduce cycle" function to reproduce chain.
 *
 * When a hit is found in the rainbow table for the given hash,
 * it repeatedly switches the password back ann forth in the hash-domain
 * untill a hash identical to the original is found OR "t" steps are made.
 *
 * In that case, a candidate password is the one immediately preceding 
 * in the password-domain, and is thus returned in the preallocated 
 * character buffer "password" for the user to try, along with a 
 * "true" boolean return value.
 *
 * If no such match occured after looping "t" times 
 * -that is, the full length of the chain-
 * the hash was a result of a collision.
 * Consinsetly, no candidate password is returned and the caller is notified
 * via the "false" boolean return value.
 * 
 *
 */
bool follow_chain( char* startpoint, char* password, uint8_t* hash, int t){

	uint8_t	cur_h[32];
	char	cur_pwd[7];

	

	for(int i=1; i<=t ; i++){												
		if(i==1)	hashfun( cur_h,(uint8_t*)startpoint,6 );
		else		hashfun( cur_h,(uint8_t*)cur_pwd,6 );				
		
		bool match = true;												
		for(int j=0; j<32; j++)
			if( cur_h[j]!=hash[j] ){
				match=false;
				break;
			}
		
		if( match ){
			if(v) cout << "Match found " << i << " steps along the way!" << endl;
			strcpy(password, cur_pwd);
			return true;
		}
		else{	
			redfun( cur_pwd,cur_h,1 );
		}		
	}	
	
	return false;	
}









