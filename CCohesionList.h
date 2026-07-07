#if !defined(ccohesionlist_h)
#define ccohesionlist_h

#include <string>
#include <map>


class CCohesionList: public std::map<std::string, double>
{
    public:
        static const std::string JOIN_STRING;

        void addCohesion(const std::string & key1,
                         const std::string &key2,
                         double cohesion);

        double getCohesion(const std::string & key1,
                           const std::string &key2);

};

#endif
