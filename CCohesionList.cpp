#include "CCohesionList.h"

#include <vector>
#include <algorithm>

using namespace std;

const string CCohesionList::JOIN_STRING = "~_~_~";

void CCohesionList::addCohesion(const string & key1,
                                const string &key2,
                                double cohesion)
{
    vector<string> keys;  // Save words here.
    keys.push_back(key1);
    keys.push_back(key2);

    //Use STL sort to put them in alphabetical order.
    sort(keys.begin(), keys.end());

    string nKey = keys[0] + JOIN_STRING + keys[1];
    insert( pair<string, double>(nKey, cohesion) );
}

double CCohesionList::getCohesion(const string & key1,
                                  const string &key2)
{
    vector<string> keys;  // Save words here.
    keys.push_back(key1);
    keys.push_back(key2);

    //Use STL sort to put them in alphabetical order.
    sort(keys.begin(), keys.end());

    string nKey = keys[0] + JOIN_STRING + keys[1];

    map<string, double>::iterator it;
    if((it = find(nKey)) != end() )
        return it->second;
    else
        return 0.0;
}
