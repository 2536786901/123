#ifndef _HOTCOUNT_HPP_
#define _HOTCOUNT_HPP_


#include "BOBHash32.h"
#include <unordered_map>
#include <string.h>


#define CONSTANT_NUMBER 2654435761u
#define CalculateBucketPos(fp) (((fp) * CONSTANT_NUMBER) >> 15)
#define GetCounterVal(val) ((uint32_t)((val) & 0x7FFFFFFF))
#define KEY_LENGTH_4 4
#define KEY_LENGTH_13 13
#define n 400
#define d 4
#define w 352
#define z 16
using namespace std;


struct Coldbucket {

    uint8_t val;

} ;

struct Coldpart
{

    Coldbucket coldbuckets[n][d][w];

};
struct Hotbucket
{
	uint32_t key[z];
	uint32_t val[z];
};

struct Hotcount
{
    Coldpart coldpart;
    alignas(64) Hotbucket hotbuckets[n];

	BOBHash32* hash[d] = {NULL};

	void Hotcount_update(uint8_t *key,int f);

	uint32_t Hotcount_query(uint8_t *key);

	void get_heavy_hitters(int threshold, vector<pair<string, int>> & results);
	
	void initialize();
};
Hotcount hotcount;



void Hotcount_destory(Hotcount Hotcount);

int CalculateFP(uint8_t *key, uint32_t &fp)
{
	fp = *((uint32_t*)key);
	return CalculateBucketPos(fp) % n;
}


void Hotcount::Hotcount_update(uint8_t *key,int f=1)
{
	uint32_t *key1;
	uint_fast32_t val1;

	//hotbucket位置
    uint32_t fp;
	int pos = CalculateFP(key, fp);

	/* find if there has matched bucket */
	int matched = -1, empty = -1, min_counter = 0;
	uint32_t min_counter_val = GetCounterVal(hotbuckets[pos].val[0]);
	for(int i = 0; i <z; i++)
	{
		if(hotbuckets[pos].key[i] == fp){
			matched = i;
			break;
		}
		if(hotbuckets[pos].key[i] == 0 && empty == -1)
			empty = i;
		if(min_counter_val > GetCounterVal(hotbuckets[pos].val[i])){
			min_counter = i;
			min_counter_val = GetCounterVal(hotbuckets[pos].val[i]);
		}
	}
	/* if matched */
	if(matched != -1){
		hotbuckets[pos].val[matched] += f;
	}
	else{
		/* if there has empty bucket */
		if(empty != -1){
			hotbuckets[pos].key[empty] = fp;
			hotbuckets[pos].val[empty] = f;
		}
    	else
    	{
			uint8_t a[d];
			uint8_t minnum=255;
			for (int i = 0; i < d; i++) {
            	int index = (hash[i]->run((const char *)key, KEY_LENGTH_4)) % w;
            	a[i]= coldpart.coldbuckets[pos][i][index].val;
        	}
			uint8_t b,c=0;
			for(int i=0;i<d;i++){
				if(a[0]==a[i])
					b=b+1;
				if(b==z)
					c=1;
			}
			if(c==1){
				for(int i=0;i<d;i++){
					int index = (hash[i]->run((const char *)key, KEY_LENGTH_4)) % w;
					coldpart.coldbuckets[pos][i][index].val+=f;
					coldpart.coldbuckets[pos][i][index].val = coldpart.coldbuckets[pos][i][index].val < 255 ? coldpart.coldbuckets[pos][i][index].val : 255;
					minnum=coldpart.coldbuckets[pos][i][index].val;
				}

			}
			else
			{
				uint8_t mina=255;
				for(int i=0;i<d;i++)
				{
					mina=min(mina,a[i]);
				}
				for(int i=0;i<d;i++){
					if(a[i]==mina){
						int index = (hash[i]->run((const char *)key, KEY_LENGTH_4)) % w;
						coldpart.coldbuckets[pos][i][index].val+=f;
						coldpart.coldbuckets[pos][i][index].val = coldpart.coldbuckets[pos][i][index].val < 255 ? coldpart.coldbuckets[pos][i][index].val : 255;
						minnum = coldpart.coldbuckets[pos][i][index].val;
					}


				}
			}


			if(minnum>min_counter_val){
				hotbuckets[pos].key[min_counter] = fp;
				val1 = min_counter_val;
                hotbuckets[pos].val[min_counter] = minnum;

				for (int i = 0; i < d; i++) {
            		int index = (hash[i]->run((const char *)key, KEY_LENGTH_4)) % w;
					coldpart.coldbuckets[pos][i][index].val -= minnum;

				}

				for (int i = 0; i < d; i++) {
            		int index = (hash[i]->run((const char *)&hotbuckets[pos].key[min_counter], KEY_LENGTH_4)) % w;
					coldpart.coldbuckets[pos][i][index].val = val1+coldpart.coldbuckets[pos][i][index].val;
					//coldpart.coldbuckets[pos][i][index].val = coldpart.coldbuckets[pos][i][index].val < 255 ? coldpart.coldbuckets[pos][i][index].val : 255;
            		/*if(coldpart.coldbuckets[pos][i][index].val< val1)
					{
						coldpart.coldbuckets[pos][i][index].val = val1;
						coldpart.coldbuckets[pos][i][index].val = coldpart.coldbuckets[pos][i][index].val < 255 ? coldpart.coldbuckets[pos][i][index].val : 255;
					}*/

					/*else
					{
						coldpart.coldbuckets[pos][i][index].val = +val1;
						coldpart.coldbuckets[pos][i][index].val = coldpart.coldbuckets[pos][i][index].val < 255 ? coldpart.coldbuckets[pos][i][index].val : 255;
					}*/

        		}
			}
		}
	}


}

uint32_t Hotcount::Hotcount_query(uint8_t *key){
	uint32_t fp;
	int pos = CalculateFP(key, fp);


	for(int i = 0; i < z; i++)
	{
		if(hotbuckets[pos].key[i] == fp)

			return hotbuckets[pos].val[i];

	}
	uint8_t minnum=255;
	for (int i = 0; i < d; i++) {
        int index = (hash[i]->run((const char *)key, KEY_LENGTH_4)) % w;
        minnum= min(minnum,coldpart.coldbuckets[pos][i][index].val);
	}
	return minnum;




}

void Hotcount::get_heavy_hitters(int threshold, vector<pair<string, int>> & results)
    {
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < z; ++j)
            {
                uint32_t key = hotbuckets[i].key[j];
       
				if (hotbuckets[i].val[j] >= threshold) {
                    results.push_back(make_pair(string((const char*)&key, 4), hotbuckets[i].val[j]));
                }
            }
    }

	void Hotcount::initialize()
	{
		for(int i=0;i<n;i++)
		{
			for(int j=0;j<z;j++)
			{
				hotbuckets[i].key[j]=0;
				hotbuckets[i].val[j]=0;
			}
			
		}
		memset(coldpart.coldbuckets,0,n*d*w*sizeof(uint8_t));
		for(int i=0;i<d;i++)
			hash[i] = new BOBHash32(i + 750);

	}
#endif
