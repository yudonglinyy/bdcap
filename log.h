#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define LOGPATH "/tmp/bdcap.log"


#define LOG(err, ...) do{																				\
	FILE *fplog = fopen(LOGPATH,"a");																	\
	if (!fplog) perror("LOGPATH Error");																\
	time_t logtimep = time(NULL);																		\
	printf("[%s: %d]%s\t", __FILE__, __LINE__, __func__); 												\
	printf(err, ## __VA_ARGS__);																		\
	printf("\t(%s)\t%s", strerror(errno), ctime(&logtimep));											\
	fprintf(fplog, "[%s: %d]%s\t", __FILE__, __LINE__, __func__);										\
	fprintf(fplog, err, ## __VA_ARGS__);																\
	fprintf(fplog, "\t(%s)\t%s", strerror(errno), ctime(&logtimep));									\
	fclose(fplog);																						\
	exit(1);																							\
}while (0)



#define LOGNUM(msg_num, err, ...) do{																	\
	FILE *fplog = fopen(LOGPATH,"a");																	\
	if (!fplog) perror("LOGPATH Error");																\
	time_t logtimep = time(NULL);																		\
	printf("[%s: %d]%s\t", __FILE__, __LINE__, __func__); 												\
	printf(err, ## __VA_ARGS__);																		\
	printf("\t(%s)\t%s", strerror(errno), ctime(&logtimep));											\
	fprintf(fplog, "[%s: %d]%s\t", __FILE__, __LINE__, __func__);										\
	fprintf(fplog, err, ## __VA_ARGS__);																\
	fprintf(fplog, "\t(%s)\t%s", strerror(errno), ctime(&logtimep));									\
	fclose(fplog);																						\
	exit(1);																							\
}while (0)


#define LOGMSG(err, ...) do{																			\
	FILE *fplog = fopen(LOGPATH,"a");																	\
	if (!fplog) perror("LOGPATH Error");																\
	time_t logtimep = time(NULL);																		\
	printf("[%s: %d]%s\t", __FILE__, __LINE__, __func__); 												\
	printf(err, ## __VA_ARGS__);																		\
	printf("\t(%s)\t%s", strerror(errno), ctime(&logtimep));											\
	fprintf(fplog, "[%s: %d]%s\t", __FILE__, __LINE__, __func__);										\
	fprintf(fplog, err, ## __VA_ARGS__);																\
	fprintf(fplog, "\t(%s)\t%s", strerror(errno), ctime(&logtimep));									\
	fclose(fplog);																						\
	errno = 0;																							\
}while (0)

#endif