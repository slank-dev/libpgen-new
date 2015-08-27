

#include "pgen.h"
#include "packet.h"
#include "address.h"
#include "netutil.h"

#include <map>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>


static char* get_dns_name(const char* iurl){
	int result_len = 64;
	char* result = (char*)malloc(result_len);
	char url[result_len];
	std::vector<std::string> vec;
	
	memcpy(url, iurl, sizeof(url));
	memset(result, 0, result_len);
	
	for(char *tp=strtok(url, "."); tp!=NULL; tp=strtok(NULL, ".")){
		if(tp != NULL)
			vec.push_back(tp);
	}

	for(int i=0; i<vec.size(); i++){
		char buf[100];
		snprintf(buf, sizeof(buf), ".%s", vec[i].c_str());
		buf[0] = (int)strlen(vec[i].c_str());
		buf[strlen(vec[i].c_str())+1] = '\0';
		
		strncat(result, buf, result_len-strlen(result));
	}
	return result;
}


pgen_dns::pgen_dns(){
	clear();
}



pgen_dns::pgen_dns(const u_char* packet, int len){
	clear();
	cast(packet, len);
}




void pgen_dns::clear(){
	pgen_udp::clear();

	DNS.id	   = 0x0000;
	DNS.flags.qr = 0;
	DNS.flags.opcode = 0;
	DNS.flags.aa = 0;
	DNS.flags.tc = 0;
	DNS.flags.rd = 1;
	DNS.flags.ra = 0;
	DNS.flags.nouse = 0;
	DNS.flags.rcode = 0;
	DNS.qdcnt = 0x0001;
	DNS.ancnt = 0x0000;
	DNS.nscnt = 0x0000;
	DNS.arcnt = 0x0000;
	
	clear_query();
	clear_answer();
}	


void pgen_dns::clear_query(){
	for(int i=0; i<MAX_QUERY; i++){
		DNS.query[i].name  = "example.com";
		DNS.query[i].type  = 0x0001;
		DNS.query[i].cls   = 0x0001;
	}
}

void pgen_dns::clear_answer(){
	for(int i=0; i<MAX_ANSWER; i++){
		DNS.answer[i].name = 0xc00c;
		DNS.answer[i].type = 0x0001;
		DNS.answer[i].cls  = 0x0001;
		DNS.answer[i].ttl  = 0x00000b44;
		DNS.answer[i].len  = 0x0004;
		DNS.answer[i].addr = "127.0.0.1";
	}
}



void pgen_dns::compile_query(){
	memset(query_data, 0, sizeof(query_data));
	query_data_len = 0;
	
	char* name;
	struct{
		bit16 type;
		bit16 cls;
	}q_data;

	bit8* p = query_data;
	for(int j=0; j<DNS.qdcnt; j++){
		name = get_dns_name(DNS.query[j].name.c_str());
		q_data.type = htons(DNS.query[j].type);
		q_data.cls  = htons(DNS.query[j].cls);
		
		memcpy(p, name, strlen(name)+1);
		p += strlen(name)+1;
		memcpy(p, &q_data, sizeof(q_data));
		p += sizeof(q_data);
	}

	query_data_len = p - query_data;
}



void pgen_dns::compile_answer(){
	memset(answer_data, 0, sizeof(answer_data));
	answer_data_len = 0;
	
	if(DNS.flags.qr != 1)
		return;
	
	struct{
		bit16 name;
		bit16 type;
		bit16 cls;
	}a_data1;
	bit32 ttl;
	struct{
		bit16 len;
		bit8 addr[4];
			
	}a_data2;


	bit8* p = answer_data;
	for(int j=0; j<DNS.ancnt; j++){

		a_data1.name = htons(DNS.answer[j].name);
		a_data1.type = htons(DNS.answer[j].type);
		a_data1.cls  = htons(DNS.answer[j].cls);
		ttl  = htonl(DNS.answer[j].ttl);
		a_data2.len  = htons(DNS.answer[j].len);
		for(int i=0; i<4; i++)
			a_data2.addr[i] = DNS.answer[j].addr.getOctet(i);

		memcpy(p, &a_data1, sizeof(a_data1));
		p += sizeof(a_data1);
		memcpy(p, &ttl, sizeof(ttl));
		p += sizeof(ttl);
		memcpy(p, &a_data2, sizeof(a_data2));
		p += sizeof(a_data2);
	}
	answer_data_len = p - answer_data;
}




void pgen_dns::compile(){
	compile_query();
	compile_answer();

	UDP.dst = 53;
	UDP.len = UDP_HDR_LEN + DNS_HDR_LEN + query_data_len + answer_data_len;
	pgen_udp::compile();

	memset(&dns, 0, sizeof dns);
	dns.id = htons(DNS.id);
	dns.qr = DNS.flags.qr;
	dns.opcode = DNS.flags.opcode;
	dns.aa = DNS.flags.aa;
	dns.tc = DNS.flags.tc;
	dns.rd = DNS.flags.rd;
	dns.ra = DNS.flags.ra;
	dns.nouse = DNS.flags.nouse;
	dns.rcode = DNS.flags.rcode;

	dns.qdcnt = htons(DNS.qdcnt);
	dns.ancnt = htons(DNS.ancnt);
	dns.nscnt = htons(DNS.nscnt);
	dns.arcnt = htons(DNS.arcnt);

	u_char* p = data;
	memcpy(p, &eth, sizeof eth);
	p += sizeof(eth);
	memcpy(p, &ip, sizeof ip);
	p += sizeof(struct MYIP);
	memcpy(p, &udp, sizeof udp);
	p += sizeof(struct MYUDP);
	memcpy(p, &dns, sizeof dns);
	p += sizeof(struct MYDNS);
	
	memcpy(p, &query_data, query_data_len);
	p += query_data_len;
	memcpy(p, &answer_data, answer_data_len);
	p += answer_data_len;
	
	len = p - data;
}




int pgen_dns::cast_query(const u_char* packet, int len){
	
	
	return 0;
}

int pgen_dns::cast_answer(const u_char* packet, int len){
	return 0;	
}



void pgen_dns::cast(const u_char* packet, int len){
	if(!(this->minLen<=len && len<=this->maxLen)){
		fprintf(stderr, "pgen_tcp::cast(): packet len isn`t support (%d)\n", len);
		return;
	}

	pgen_udp::cast(packet, len);	
	const u_char* p = packet;

	p += ETH_HDR_LEN;
	p += IP_HDR_LEN;
	p += UDP_HDR_LEN;
	
	struct MYDNS* buf = (struct MYDNS*)p;
	p += DNS_HDR_LEN;

	this->DNS.id = ntohs(buf->id);
	this->DNS.flags.qr = buf->qr;
	this->DNS.flags.opcode = buf->opcode;
	this->DNS.flags.aa = buf->aa;
	this->DNS.flags.tc = buf->tc;
	this->DNS.flags.rd = buf->rd;
	this->DNS.flags.ra = buf->ra;
	this->DNS.flags.nouse = buf->nouse;
	this->DNS.flags.rcode = buf->rcode;
	this->DNS.qdcnt = ntohs(buf->qdcnt);
	this->DNS.ancnt = ntohs(buf->ancnt);
	this->DNS.nscnt = ntohs(buf->nscnt);
	this->DNS.arcnt = ntohs(buf->arcnt);
	
	query_data_len = 0;
	query_data_len += cast_query(packet, len);	
	p += query_data_len;

	answer_data_len = 0;
	answer_data_len += cast_answer(packet, len);
	p += answer_data_len;

	this->len = p - packet;
	
		

	/*
	// query infomation
	bit8* queryPoint;
	char url[256];
	struct query_typecls{
		bit16 type;
		bit16 cls;
	};
	struct query_typecls* tc;
	bit32 queryLen;

	// answer infomation
	bit8* answerPoint;
	struct answer{
		bit16 type:16;
		bit16 cls:16;
		bit32 ttl:32;
		bit16 len:16;
		bit8  addr[4];
	};
	struct answer ans;
	bit32 answerLen;

	queryPoint = (bit8*)(dnsPoint+sizeof(struct MYDNS));


	for(queryLen=0; *(queryPoint+queryLen)!=0; queryLen++){
		if(queryLen==0)	continue;
		if(queryLen>= 50){
			printf("stack abunai!!!\n")	;
			printf("queryLen is too large(%d)\n", queryLen);
			return;

		}


		if(('a' <=  *(queryPoint+queryLen) && *(queryPoint+queryLen) <= 'z') ||
				('A' <=  *(queryPoint+queryLen) && *(queryPoint+queryLen) <= 'Z') || 
				('0' <=  *(queryPoint+queryLen) && *(queryPoint+queryLen) <= '9')){
			url[queryLen-1] = *(queryPoint+queryLen);
		}else{
			url[queryLen-1] = '.';
		}
	}queryLen += 4 + 1;




	tc = (struct query_typecls*)(queryPoint + queryLen - 4);
	

	DNS.query.name = url;
	DNS.query.type = ntohs(tc->type);
	DNS.query.cls  = ntohs(tc->cls);


	answerPoint = queryPoint + queryLen;
	memcpy(&ans, answerPoint+2, 16);

	char ansaddrstr[16];
	snprintf(ansaddrstr, sizeof(ansaddrstr), "%d.%d.%d.%d", 
			ans.addr[0], ans.addr[1], ans.addr[2], ans.addr[3]);

	//DNS.answer.name = htons(answerPoint);
	DNS.answer.name = 0;
	DNS.answer.type = htons(ans.type);
	DNS.answer.cls  = htons(ans.cls );
	DNS.answer.ttl  = htonl(ans.ttl );
	DNS.answer.len  = htons(ans.len );
	DNS.answer.addr = ansaddrstr;

*/
}





// not coding now
void pgen_dns::summary(){
	compile();
	if(dns.qr == 1){
  		printf("Query response 0x%04x %s %s\n", ntohs(dns.id), DNS.query[1].name.c_str(),
				DNS.answer[1].addr.c_str());
	}else{
		printf("Query 0x%04x %s\n", ntohs(dns.id), DNS.query[1].name.c_str());	
	}
}







void pgen_dns::info(){
	compile();
	pgen_udp::info();

	printf(" * Domain Name System \n");
	printf("    - Identification  : 0x%04x\n", ntohs(dns.id));
	printf("    - Flags           : 0x%04x\n", ntohs(dns.flags));
	printf("         - qr         : %d", dns.qr);
	if(dns.qr == 0)				printf("   (query) \n");
	else						printf("   (response)\n");
	printf("         - opcode     : %d", dns.opcode);
	if(dns.opcode == 0)			printf("   (standard query) \n");
	else if(dns.opcode == 1)	printf("   (inverse query) \n");
	else if(dns.opcode == 2)	printf("   (server status request) \n");
	else						printf("   (malformed)\n");
	printf("         - aa         : %d", dns.aa);
	if(dns.aa == 1)				printf("   (have authority) \n");
	else						printf("   (no authority)\n");
	printf("         - tc         : %d", dns.tc);
	if(dns.tc == 1)				printf("   (caption) \n");
	else						printf("   (no caption)\n");
	printf("         - rd         : %d", dns.rd);
	if(dns.rd == 1)				printf("   (recursion desired) \n");
	else						printf("   (no recursion)\n");
	printf("         - ra         : %d", dns.ra);
	if(dns.ra == 1)				printf("   (recursion available) \n");
	else						printf("   (recursion unavailable)\n");

	printf("         - nouse      : %d\n", dns.nouse);
	printf("         - rcode      : %d\n", dns.rcode);
	
	printf("    - Question        : 0x%04x\n", ntohs(dns.qdcnt));
	printf("    - Answer RRs      : 0x%04x\n", ntohs(dns.ancnt));
	printf("    - Authority RRs   : 0x%04x\n", ntohs(dns.nscnt));
	printf("    - Additional RRs  : 0x%04x\n", ntohs(dns.arcnt));

	for(int i=0; i<DNS.qdcnt; i++){
		printf("    - Queries[%d] \n", i);
		printf("         - name       : %s \n", DNS.query[i].name.c_str());
		printf("         - type       : 0x%04x \n", DNS.query[i].type);
		printf("         - class      : 0x%04x \n", DNS.query[i].cls);
	}
	
	for(int i=0; i<DNS.ancnt; i++){
		printf("    - Answer[%d]  \n", i);
		printf("         - name       : 0x%04x (kaihatutyu)\n", DNS.answer[i].name);
		printf("         - type       : 0x%04x \n", DNS.answer[i].type);
		printf("         - class      : 0x%04x \n", DNS.answer[i].cls);
		printf("         - ttl        : 0x%08x \n", DNS.answer[i].ttl);
		printf("         - data len   : 0x%04x \n", DNS.answer[i].len);
		printf("         - address    : %s \n", DNS.answer[i].addr.c_str());
	}
}
