/*
 * kmp.c
 *
 *  Created on: Jul 29, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stdlib.h>
#include <stdio.h>
#include <utility/algorithm/kmp.h>

//O(mn)
int kmp(const char* dest,const char * pattern){
	if(!*dest||!*pattern)
		return -1;

	int n=0;
	for(const char* c=pattern,*s=dest;*(++c);){
		if(!*(++s))
			return -1;
		if(*c==*pattern)
			++n;
	}

	int *a=malloc(sizeof(int)*(n*2+1));
	a[0]=n;

	//build substring position map
	for(int i=1,ai=1,n=0;pattern[i];++i){
		if(pattern[i]==*pattern){
			for(n=1;pattern[i+n]==pattern[n];++n);
			a[ai++]=i;
			a[ai++]=n;
		}
	}

	for(int pi=0,di=0,f=0,i=1;;){
		if(!pattern[pi]){
			free(a);
			return di-pi;
		}else if(!dest[di]){
			free(a);
			return -1;
		}else if(pattern[pi]==dest[di]){
			++pi;
			++di;
		}else if(pi>1){
			if(di!=f){
				i=1;
				f=di;
			}
			for(;a[0]*2+1>i;i+=2){ // find the next start point based on substring map
				if(a[i]+a[i+1]>=pi){
					pi-=a[i];
					break;
				}
			};
			if(a[0]*2+1<=i)
				pi=0;
		}else if(pi){
			pi=0;
		}else
			++di;
	}
}
