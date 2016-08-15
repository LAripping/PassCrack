#include <iostream>
#include <fstream>

#include <list>

#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>

#include <random>


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
extern bool pass_comparator(const pair<char[7],char[7]>* first, const pair<char[7],char[7]>* second);	

extern bool		v, savechain;



/*
 * Reduction-functions' set #1.
 *          "Simple but terribly biased"
 *
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
void red_functs_set1( char* out, const uint8_t *in, int red_by ){													
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
}




/*
 * Reduction-functions' set #2.
 *          "High Kolmogorov Complexity"
 *
 * 1024 original red-funs
 *
 * Each one selects 6 bytes of the hash and MODs them with 64 to map it
 * to an alphabet letter.
 *
 * Discreet red-funs are divided into 31 "klasses" subsets, each klass 
 * with 32 red-funs defining the starting byte to be selected 
 *   1st starts with byte[0]
 *   2nd starts with byte[1]
 *   ...
 *
 * Then, the next byte is the previous +klass circularly around 
 * the 32 of the hash.
 * E.g. for klass=2, red-fun #28 selects bytes[28-30-0-2-4-6]
 *     
 *
 */
void red_functs_set2( char* out, const uint8_t *in, int red_by ){				
    
    int klass = (red_by/32 + 1);                        // Map every red_by to a Klass 1,2,...31,32,33,...
    int inklass_idx;                                    // idx of function inside its klass, ranging 0...31
    
    if(klass>32){
            klass = (klass-1) % 32 + 1; 
            inklass_idx = (red_by-1) % 32;   
    }else{        
        inklass_idx = red_by - ((klass-1)*32 + 1);              
    }     
     
    if(klass==32) klass=9;                              // Skip the merge-suspect 32rd klass!
     
    int o_idx, i_idx =inklass_idx, ab_idx;
    for(o_idx = 0; o_idx<6; o_idx++ ){
        ab_idx = in[i_idx]%64;
        out[o_idx] = alphabet[ ab_idx ];    
        i_idx = (i_idx + klass) % 32;
    }
   	out[o_idx] = '\0';									// NULL terminate it to process as C-string                     
}

		
		
/*
 * Reduction-functions' set #3.
 *          "Maximum output entropy"
 *
 * Resulting from some "mixing" of techniques
 *
 */
void red_functs_set3( char* out, const uint8_t *in, int red_by ){				

    int o_idx, i_idx =red_by%32, ab_idx;
    for(o_idx = 0; o_idx<6; o_idx++ ){
        ab_idx = in[i_idx]%64;
        out[o_idx] = alphabet[ ab_idx ];    
        i_idx = (i_idx + 9) % 32;
    }
   	out[o_idx] = '\0';									// NULL terminate it to process as C-string                     
}
		
		
		
		
/*
 * Reduction-functions' set #4.
 *          "PRNG - Uniformly distributed indices "
 *
 * Seeding uniform_distribution generator with red_by index
 * for the indices of the hash's bytes to be selected. 
 * Then these bytes' values are mapped with a mod64 to a 
 * character in the passwords alphabet
 *
 *  WARNING: Much slower to implement, but offers greater uniformity
 *
 */
void red_functs_set4( char* out, const uint8_t *in, int red_by ){				
    int o_idx, i_idx, ab_idx;
    int range_from  = 0;            // inclusive
    int range_to    = 31;
    default_random_engine           generator(red_by); // seeding happens here
    uniform_int_distribution<int>   distr(range_from, range_to);    
    
    for(o_idx = 0; o_idx<6; o_idx++ ){
        i_idx = distr(generator);
 
        ab_idx = in[i_idx]%64;
        out[o_idx] = alphabet[ ab_idx ];    
        i_idx = (i_idx + 9) % 32;
    }
   	out[o_idx] = '\0';									// NULL terminate it to process as C-string                     
}
		
	
	
	
/*
 * Reduction-functions' set #5.
 *          "PRNG - Uniformly distributed passchars "
 *
 * Red-by with a "twist" determines the byte of the hash
 * that will be selected to seed the PRNG.
 * This in turn generates 6 chars uniformly distributed in the alphabet
 * 
 */
void red_functs_set5( char* out, const uint8_t *in, int red_by ){				
    int o_idx, i_idx;
    int range_from  = 0;            // inclusive
    int range_to    = 63;
    
    i_idx = (red_by+9) % 32;
        
    default_random_engine           generator( in[i_idx] ); // seeding happens here
    uniform_int_distribution<int>   distr(range_from, range_to);    
    
    for(o_idx = 0; o_idx<6; o_idx++ ){
        out[o_idx] = alphabet[ distr(generator) ];    
    }
   	out[o_idx] = '\0';									// NULL terminate it to process as C-string                     
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
	
//		ofstream out("whatwasread.txt", ofstream::trunc);
//		write_table(rainbow,out);
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
	int	base_index		          = 1;
	int	middle_index	          = rainbow->size() / 2;
	int	top_index		          = rainbow->size();		
	Table_t::iterator         it  = rainbow->begin();
	Table_t::reverse_iterator tit = rainbow->rbegin();	

	pair<char[7],char[7]>					*middle_pair;				// Pivot-pointer pair
	pair<char[7],char[7]>                   *base_pair = *it, *top_pair = *tit;
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
																		
		if( pass_comparator(dummy_pair,base_pair)                       // Skip the whole process in the rare case
		 || pass_comparator(top_pair,dummy_pair)  ){                    // that pwd > top OR pwd < base
		     delete dummy_pair;
		     return false;
		}     
		 
		 
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
	if(! strcmp(middle_pair->second,pwd) ){								// Make one last comparisonfor single point case
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


	for(int i=1; i<t+2 ; i++){												
		if(i==1)	hashfun( cur_h,(uint8_t*)startpoint,6 );
		else		hashfun( cur_h,(uint8_t*)cur_pwd,6 );				
		
		if( memcmp(cur_h,hash,sizeof(cur_h))==0 ){
		    strcpy(password, cur_pwd);	
		    if(v)
		        cout<<"After applying red-fun #"<<i-1<<", the hashed "
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
















