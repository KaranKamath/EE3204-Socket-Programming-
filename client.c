/*******************************
tcp_client.c: the source file of the client in tcp transmission 
********************************/

#include "headsock.h"

float str_cli(FILE *fp, int sockfd, long *len, int *num_errors);                       //transmission function
void tv_sub(struct  timeval *out, struct timeval *in);	    //calcu the time interval between out and in

struct CSVData { 
  int packet_size; 
  float transmission_time;
  float data_rate;
  int err_prob;
}; 

int write_to_file(int count, struct CSVData *data, char const *fileName) 
{ 
  FILE *f = fopen(fileName, "a+"); 
  if (f == NULL) return -1; 
  while (count-- > 0) {
    fprintf(f, "%d,%d,%.3f, %.3f\n", data->packet_size, data->err_prob, data->transmission_time, data->data_rate); 
    ++data; 
  } 
  fclose(f); 
  return 0; 
} 

int main(int argc, char **argv)
{
	int k=0;
	float avg_ti = 0;
	float avg_rt = 0;
	int total_runs = 10;
	int err_prob = 0;
	
	for (k=0; k<total_runs; k++)
	{		
		int sockfd, ret;
		float ti, rt;
		long len;
		struct sockaddr_in ser_addr;
		char ** pptr;
		struct hostent *sh;
		struct in_addr **addrs;
		FILE *fp;
		int num_errors = 0; //keeps track of errors

		if (argc != 3) {
			printf("parameters not match");
		}
		
		err_prob = atoi(argv[2]);
		
		sh = gethostbyname(argv[1]);	                                       //get host's information
		if (sh == NULL) {
			printf("error when gethostby name");
			exit(0);
		}

		printf("canonical name: %s\n", sh->h_name);					//print the remote host's information
		for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
			printf("the aliases name is: %s\n", *pptr);
		switch(sh->h_addrtype)
		{
			case AF_INET:
				printf("AF_INET\n");
			break;
			default:
				printf("unknown addrtype\n");
			break;
		}
		
		addrs = (struct in_addr **)sh->h_addr_list;
		sockfd = socket(AF_INET, SOCK_STREAM, 0);                           //create the socket
		if (sockfd <0)
		{
			printf("error in socket");
			exit(1);
		}
		ser_addr.sin_family = AF_INET;                                                      
		ser_addr.sin_port = htons(MYTCP_PORT);
		memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
		bzero(&(ser_addr.sin_zero), 8);
		ret = connect(sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));         //connect the socket with the host
		if (ret != 0) {
			printf ("connection failed\n"); 
			close(sockfd); 
			exit(1);
		}
	
		if((fp = fopen ("myfile.txt","r+t")) == NULL)
		{
			printf("File doesn't exit\n");
			exit(0);
		}

		ti = str_cli(fp, sockfd, &len, &num_errors);                       //perform the transmission and receiving
		rt = (len/(float)ti);                                         //caculate the average transmission rate
		printf("Time(ms) : %.3f, Data sent(byte): %d\nData rate: %f (Kbytes/s)\nErrors: %d\n", ti, (int)len, rt, num_errors);
		
		avg_ti += ti;
		avg_rt += rt;
		
		close(sockfd);
		fclose(fp);
	//}	
	}
	
	avg_ti /= total_runs;
	avg_rt /= total_runs;
	
	struct CSVData datum = { .packet_size=DATALEN, .transmission_time=avg_ti, .data_rate=avg_rt, .err_prob=err_prob};
	
		//struct CSVData* data = malloc(1 * sizeof(*datum));
	
	write_to_file(1, &datum, "Data.ods");
		
	exit(0);
}

float str_cli(FILE *fp, int sockfd, long *len, int *num_errors)
{
	char *buf;
	long lsize, ci;
	char sends[DATALEN];
	struct ack_so ack;
	int n, slen;
	float time_inv = 0.0;
	struct timeval sendt, recvt;
	ci = 0;

	fseek (fp , 0 , SEEK_END);
	lsize = ftell (fp);
	rewind (fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n",DATALEN);

// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

  // copy the file into the buffer.
	fread (buf,1,lsize,fp);

  /*** the whole file is loaded in the buffer. ***/
	buf[lsize] ='\0';									//append the end byte
	gettimeofday(&sendt, NULL);							//get the current time
	while(ci<= lsize)
	{
		if ((lsize+1-ci) <= DATALEN)
			slen = lsize+1-ci;
		else 
			slen = DATALEN;
		memcpy(sends, (buf+ci), slen);
		n = send(sockfd, &sends, slen, 0);
		if(n == -1) {
			printf("send error!");								//send the data
			exit(1);
		}
		
		if ((n= recv(sockfd, &ack, 2, 0))==-1)                                   //receive the ack
		{
			printf("error when receiving\n");
			exit(1);
		}
		if (ack.num != 1|| ack.len != 0) 
		{
			*num_errors = *num_errors + 1;
		}
		else if (ack.num == 1)
		{
			ci += slen;
		}
			//printf("error in transmission\n");
	}
	gettimeofday(&recvt, NULL);
	*len= ci;                                                         //get current time
	tv_sub(&recvt, &sendt);                                                                 // get the whole trans time
	time_inv += (recvt.tv_sec)*1000.0 + (recvt.tv_usec)/1000.0;
	return(time_inv);
}

void tv_sub(struct  timeval *out, struct timeval *in)
{
	if ((out->tv_usec -= in->tv_usec) <0)
	{
		--out ->tv_sec;
		out ->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}
