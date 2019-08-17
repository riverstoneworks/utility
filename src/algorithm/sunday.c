/*
 * sunday.c
 *
 *  Created on: Aug 2, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <utility/algorithm/sunday.h>

int sunday(const char* dest, const char* pattern ){

	if(!(dest&&pattern&&*dest&&*pattern))
		return -1;

	size_t ld=strlen(dest),lp=0;

	//build index table for character map
	unsigned int map_ind[256][2];
	memset(map_ind,0,sizeof(map_ind));

	while(pattern[lp]){
		map_ind[(unsigned char)pattern[lp++]][1]++;
	}
	for(int i=0,n=0;i<256;++i)
		if(map_ind[i][1]){
			map_ind[i][0]=n;
			n+=map_ind[i][1];
			map_ind[i][1]=0;
		}
	//build character map
	int*map= malloc(sizeof(int)*lp);
	for(int i=0;i<lp;i++)
		map[map_ind[(unsigned char)pattern[i]][0]+map_ind[(unsigned char)pattern[i]][1]++]=i;


	//match
	for(int i=lp-1,h,j,ii,mb;i<ld;i+=lp){
		//validate if dest[i] in pattern with map index table
		ii=map_ind[(unsigned char)dest[i]][1]-1;
		if(ii<0)
			continue;

		mb=map_ind[(unsigned char)dest[i]][0];

		while(ii>-1){
			//update the start position of comparison in dest
			h=j=i-map[mb+ii];
			int x=0;
			while(x<lp&&j<ld&&*(int64_t*)(pattern+x)==*(int64_t*)(dest+j)){
				x+=8;
				j+=8;
			}
			if (x > lp){
				j -= x - lp;
				x=lp;
			}
			if (j > ld){
				x -= j - ld;
				j=ld;
			}
			while(x<lp&&j<ld&&*(pattern+x)==*(dest+j)){
				++x;
				++j;
			}
			if(x>=lp){
				free(map);
				return h;
			}else if(j>=ld){
				free(map);
				return -1;
			}else{
				//Mismatched priority, move i to last mismatch j if j>i
				if(i<j){
					i^=j;
					j^=i;
					i^=j;
					ii=map_ind[(unsigned char)dest[i]][1]-1,mb=map_ind[(unsigned char)dest[i]][0];
				}

				//validate if dest[j] in pattern and the index is ji
				int iii=map_ind[(unsigned char)dest[j]][1]-1,mbb=map_ind[(unsigned char)dest[j]][0];
				while(ii>-1){
					if(map[mb+ii]<i-h){
						//the dest[j] index in pattern should be
						int ji=map[mb+ii]-(i-j);
						if(ji<0)
							break;

						while(iii>-1&&map[mbb+iii]>ji)
							iii--;

						if(iii<0){
							ii=-1;
							i=ji;
							break;
						}else if(iii>-1&&map[mbb+iii]==ji)
						//if success, do compare from dest[i-map[mb+ii]];
							break;
					}
					ii--;
				}
			}
		}
	}
	free(map);
	return -1;
}
