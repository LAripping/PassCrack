#include <iostream>
#include <fstream>
#include <cstring>

#include <list>

using namespace std;
typedef list<pair<char[7],char[7]>*> Table_t;


bool v = true;





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
		cout << "Table imported succesfully\nIt contains: " 
			<< rainbow->size() << " chains" << endl;
	
//		ofstream out("whatwasread.txt", ofstream::trunc);
//		write_table(rainbow,out);
	}	
	
	return rainbow;
}



void write_table( Table_t* rainbow,  ofstream& outtable ){
	
	outtable << "TABLE 1" << endl;
	
	for( pair<char[7],char[7]>* pp : *rainbow )
		outtable << pp->first << " -> " << pp->second << endl;
	
	outtable.put('\n');
}





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



bool pass_predicate(const pair<char[7],char[7]>* pair1, const pair<char[7],char[7]>* pair2){	
	//replace sole "return" statement with if-clause to deallocate the non-unique pairs found as well
	return( !strcmp(pair1->second, pair2->second) );	
}





// arg1 is the file which contains all files to be read
//   create it with "ls > contents.txt"
// arg2 is the file to output the merged tables


int main( int argc, char** argv ){
   
    if(argc!=3){
        cout<<"Usage: "<< argv[0]<<" <contents_file> <output_file>"<<endl;
        exit(1);
    }     
   
    Table_t* full_rainbow = new Table_t();
    long m;
    char line[128];
    
    ifstream contents, intable;
        
    contents.open(argv[1]);
    if(v) cout << "Opened file " << argv[1] << " to read filenames" << endl;
    while(! contents.eof() ){
        contents.getline(line,128);
        if( strlen(line)<6 ) break;
        
        
        intable.open(line);
        if(v) cout << "Opened file " << line << " to import a table" << endl;

        Table_t* rainbow = read_table(intable);
        full_rainbow->merge(*rainbow,pass_comparator);
        
        m = full_rainbow->size();
        cout<<"Full table now contains: "<< m << " chains" << endl;
        
        intable.close();
    }
    contents.close();
        
    if(v) cout << "Sorting table... " << flush;			
	full_rainbow -> sort( pass_comparator );			// Sort full table
	if(v) cout << "Done!" << endl;
    
    
    if(v) cout << "Post processing table... " << flush;			
	full_rainbow -> unique( pass_predicate );			// Full table has Unique endpoints
	long perfect = full_rainbow->size();
	if(v){
	    cout << "Done!\n\t" << ( (float)m-(float)perfect )/(float)m * 100
	    <<"% of chains had same endpoints and were removed\n\tNew m="
	    <<perfect<<endl;
	}
    
    ofstream outtable;
    
    if( ifstream(argv[argc-1]) ){
	    cout << "Output file \" " << argv[argc-1] << "\" already exists! Overwrite? (y/n): ";
	    char ans;
	    cin >> ans;
	    if(ans != 'y') exit(0);
	}    

    outtable.open(argv[argc-1], ofstream::trunc);
    cout << "Opened file " << argv[argc-1] << " to export the full table." << endl;
    
    write_table( full_rainbow, outtable );
    outtable.close();
}
