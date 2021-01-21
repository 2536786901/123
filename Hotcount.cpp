#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <vector>

#include "Hotcount.hpp"
using namespace std;

#define START_FILE_NO 1
#define END_FILE_NO 10


struct FIVE_TUPLE{	char key[13];	};
typedef vector<FIVE_TUPLE> TRACE;
TRACE traces[END_FILE_NO - START_FILE_NO + 1];

void ReadInTraces(const char *trace_prefix)
{
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		char datafileName[100];
		sprintf(datafileName, "%s%d.dat", trace_prefix, datafileCnt - 1);
		FILE *fin = fopen(datafileName, "rb");

		FIVE_TUPLE tmp_five_tuple;
		traces[datafileCnt - 1].clear();
		while(fread(&tmp_five_tuple, 1, 13, fin) == 13)
		{
			traces[datafileCnt - 1].push_back(tmp_five_tuple);
		}
		fclose(fin);

		printf("Successfully read in %s, %ld packets\n", datafileName, traces[datafileCnt - 1].size());
	}
	printf("\n");
}

int main()
{
	ReadInTraces("data/");

	Hotcount hotcount;

    float are=0,are1=0,f1=0;
	for(int datafileCnt = START_FILE_NO; datafileCnt <= END_FILE_NO; ++datafileCnt)
	{
		hotcount.initialize();
		unordered_map<string, int> Real_Freq;
		int packet_cnt = (int)traces[datafileCnt - 1].size();
		for(int i = 0; i< packet_cnt; ++i)
		{

			hotcount.Hotcount_update((uint8_t*)(traces[datafileCnt - 1][i].key),1);
			string str((const char*)(traces[datafileCnt - 1][i].key),4);
			Real_Freq[str]++;
		}
		double ARE = 0,Recall=0,a=0;
		double tot=0,tot1=0,ARE1=0;
		int t=0;
		for(unordered_map<string,int>::iterator it =Real_Freq.begin();it !=Real_Freq.end(); ++it)
		{
			uint8_t key[4];
			memcpy(key, (it->first).c_str(),4);
			int est_val =hotcount.Hotcount_query(key);
			int dist =std::abs(it->second - est_val);
			ARE += dist * 1.0 / (it->second);

			if(it->second>(packet_cnt * 1 / 1000))
			{
				//printf("%d,%d\n",est_val,it->second);
				int dist1=std::abs(it->second - est_val);
				ARE1+= dist1 * 1.0 / (it->second);
				if(it->second>est_val)
				{
					a+=est_val;
				}
				else
				{
					a+=it->second;
				}
				tot+=est_val;
				tot1+=it->second;
				Recall+=dist1;
				t=t+1;
			}
		}
		Recall=a/tot1;
		ARE =ARE / (int)Real_Freq.size();
		ARE1=ARE1 / t;

		float pre=a/tot;
		
		are+=ARE;
		are1+=ARE1;
		printf("ARE of per-flow size estimation:%.3lf\n",ARE);
		printf("ARE of heavy hitter detection:%f\n",ARE1);
		
		printf("F1 score:%f\n",2*Recall*pre/(pre+Recall));
		f1+=2*Recall*pre/(pre+Recall);


  #define HEAVY_HITTER_THRESHOLD(total_packet) (total_packet * 1 / 1000)
		vector< pair<string, int> > heavy_hitters;
		hotcount.get_heavy_hitters(HEAVY_HITTER_THRESHOLD(packet_cnt),heavy_hitters);

		printf("heavy hitters: <srcIP, count>, threshold=%d\n",HEAVY_HITTER_THRESHOLD(packet_cnt));
		for(int i = 0,j = 0;i < (int)heavy_hitters.size(); ++i)
		{
			uint32_t srcIP;
			memcpy(&srcIP, heavy_hitters[i].first.c_str(),4);
			printf("<%.8x,%d\t>",srcIP,heavy_hitters[i].second);
			if(++j % 5 == 0)
				printf("\n");
			else printf("\t");
		}
		printf("\n");

	}

}
