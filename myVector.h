//////////////////////////////////////////////////////////////////////
//
// Compile options needed: /GX
//
//    Opless.cpp -- Illustrates the defining the < operator to sort vectors
//
// Functions:
//
//    operator< - Vector comparison operator.
//
//    vector::begin - Returns an iterator to start traversal of the vector.
//
//    vector::end - Returns an iterator for the last element of the vector.
//
//    vector::iterator - Traverses the vector.
//
//    vector::push_back - Appends (inserts) an element to the end of a
//                        vector, allocating memory for it if necessary.
//
//    sort algorithm - Sorts the vector.
//
//////////////////////////////////////////////////////////////////////

#ifndef __MY_VECTOR_HHH__
#define __MY_VECTOR_HHH__

// The debugger can't handle symbols more than 255 characters long.
// STL often creates symbols longer than that.
// When symbols are longer than 255 characters, the warning is disabled.
#pragma warning(disable:4786)

//#include <iostream>
#include <vector>
//#include <string>
//#include <algorithm>
using namespace std;

class CSegment {
public:
    int nClassNo;	// 段的类别编号
    long lSegEnd;	// 段的结束位置（毫秒）
    CSegment() : nClassNo(-1), lSegEnd(-1) {}
    CSegment(int nNo, long lEnd) : nClassNo(nNo), lSegEnd(lEnd) {}
};
// Define a template class for a vector of CSegments.
typedef vector<CSegment> SEGMENT_VECTOR;

/*
void main()
{
    // Declare a dynamically allocated vector of IDs.
    NAMEVECTOR theVector;

    // Iterator is used to loop through the vector.
    NAMEVECTOR::iterator theIterator;

    // Create a pseudo-random vector of players and scores.
    theVector.push_back(ID("Karen Palmer", 2));
    theVector.push_back(ID("Ada Campbell", 1));
    theVector.push_back(ID("John Woloschuk", 3));
    theVector.push_back(ID("Grady Leno", 2));

    for (theIterator = theVector.begin(); theIterator != theVector.end();
         theIterator++)
        cout << theIterator->Score  << "\t"
             << theIterator->Name << endl;
    cout << endl;

    // Sort the vector of players by score.
    sort(theVector.begin(), &theVector[theVector.size()]);

    // Output the contents of the vector in its new, sorted order.
    cout << "Players ranked by score:" << endl;
    for (theIterator = theVector.begin(); theIterator != theVector.end();
         theIterator++)
        cout << theIterator->Score  << "\t"
             << theIterator->Name << endl;
    cout << endl << endl;
}
*/

#endif	// __MY_VECTOR_HHH__

