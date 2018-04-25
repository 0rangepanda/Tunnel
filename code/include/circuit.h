#ifndef _CIRCUIT_H_
#define _CIRCUIT_H_

#include "common.h"
#include "extern.h"

/**************************************************************************
* circuit DS
* hops: r1 -> r2 -> r3
*       |     |     |
* key:  k1    k2    k3
**************************************************************************/
struct circuit {
        int id;
        int seq;
        int len;
        int* hops;
        char** key;

        //stage 8
        //NOTE: increase from 0 to len for each extdone recieving
        //when ready==len, circuit building is done
        int ready;

        //stage 9
        int pkt_counter;
};

/**************************************************************************
* define a flow as TCP traffic with the standard 5-tuple: a unique
* source and destination IP address and port, plus the proto number.
*
* This is a example of using unordered_map with custom key
* Usage:
*   struct circuit c1,c2;
    unordered_map<struct flow, struct circuit> flowMap = {
            { {1, 1, 1, 1, 1}, c1},
            { {1, 1, 1, 1, 2}, c2}
    };

    printf("TEST: %d\n", flowMap.find({1, 1, 1, 1, 1}) == flowMap.end() );
    printf("TEST: %d\n", flowMap.find({1, 1, 1, 1, 2}) == flowMap.end() );
    printf("TEST: %d\n", flowMap.find({1, 1, 1, 1, 3}) == flowMap.end() );
**************************************************************************/
struct flow
{
        int src;
        int dst;
        int srcport;
        int dstport;
        int proto;

        // constructor1
        flow()
        {
                this->src = 0;
                this->dst = 0;
                this->srcport = 0;
                this->dstport = 0;
                this->proto = 0;
        };
        // constructor2
        flow(int src, int dst, int srcport, int dstport, int proto)
        {
                this->src = src;
                this->dst = dst;
                this->srcport = srcport;
                this->dstport = dstport;
                this->proto = proto;
        };

        //equal function
        bool operator==(const flow &other) const
        {
                return src  == other.src &&
                       dst == other.dst &&
                       srcport  == other.srcport &&
                       dstport  == other.dstport &&
                       proto  == other.proto;
        };

};

//hash function
namespace std
{
        template <>
        struct hash<flow>
        {
                size_t operator()( const flow &f ) const
                {
                        size_t res = 17;
                        //printf("Hash: %d, %d, %d, %d, %d\n", f.src, f.dst, f.srcport, f.dstport, f.proto);
                        res = res * 31 + hash<int>()( f.src );
                        res = res * 31 + hash<int>()( f.dst );
                        res = res * 31 + hash<int>()( f.srcport );
                        res = res * 31 + hash<int>()( f.dstport );
                        res = res * 31 + hash<int>()( f.proto );
                        return res;
                }
        };
};


/*---------------------------- Circuit Functions ------------------------------*/



#endif
