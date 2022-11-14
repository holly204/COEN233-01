/*
COEN 233 Computer Networks
Program assignment 1
Name: Li Huang
Student ID: W1641460
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "packet.h"

//define prot
#define PORT 8800

//define a function for show datapacket
void show(struct DataPacket dtp);

ACKPacket generate_recv(struct DataPacket dp);
RejectPacket generate_rej(struct DataPacket dp, int Rej_sub_code);
void show_ack(struct ACKPacket ap);
void show_rej(struct RejectPacket rp);
int receive_packet(int sockfd,struct sockaddr_in*client_addr,socklen_t addr_size);

int main()
{

        char *ip = "127.0.0.1";

        int sockfd, b;
        struct sockaddr_in server_addr, client_addr;
        
	//char buffer[1024];	
	

        socklen_t addr_size;

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0){
                perror("socket error!");
                exit(1);
        }

        memset(&server_addr, '\0', sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = inet_addr(ip);
	
	b = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if(b < 0){
                perror("bind error!");
                exit(1);
        
	}


        receive_packet(sockfd, &client_addr, addr_size);

/*
	bzero(buffer, sizeof(DataPacket));
	addr_size = sizeof(client_addr);
	recvfrom(sockfd, buffer, sizeof(DataPacket), 0,(struct sockaddr*)&client_addr, &addr_size);
	printf("Data recv: %s \n", buffer);

	DataPacket *dp = (DataPacket *)buffer;
	receive_packet(*dp);
        
	bzero(buffer, sizeof(DataPacket));
	strcpy(buffer, "Welcome to the UDP server!");
	sendto(sockfd, buffer, sizeof(DataPacket), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
	printf("Data send: %s\n", buffer);
*/
	return 0;
}

ACKPacket generate_recv(struct DataPacket dp){
	struct ACKPacket rp;
	rp.StartPacketId = START_IDENTIFIER;
	rp.ClientId = dp.ClientId;
	rp.Ack = ACK;
	rp.ReceivedSegmentNo = dp.SegmentNo;
	rp.EndPacketId = END_IDENTIFIER;
	//printf("Generate ACK");
	return rp;
}
RejectPacket generate_rej(struct DataPacket dp, int Rej_sub_code){
        struct RejectPacket jp;
        jp.StartPacketId = START_IDENTIFIER;
        jp.ClientId = dp.ClientId;
        jp.Reject = REJECT;
	if (Rej_sub_code == 1){
		jp.Reject_sub_code = REJECT_OUT_SEQUENCE;
	}else if(Rej_sub_code == 2){
		jp.Reject_sub_code = REJECT_lENGTH_MISMATCH;
	}else if(Rej_sub_code == 3){
		jp.Reject_sub_code = REJECT_DATA_MISSING;
	}else if(Rej_sub_code == 4){
		jp.Reject_sub_code = REJECT_DUPLICATE;
	}else{
		jp.Reject_sub_code = 0;
	}
        jp.ReceivedSegmentNo = dp.SegmentNo;
        jp.EndPacketId = END_IDENTIFIER;
        //printf("Generate ACK");
        return jp;
}


int receive_packet(int sockfd,struct sockaddr_in *client_addr, socklen_t addr_size) {
        // 1. call recvfrom to receive packet from client
        // 2. check packet, if packet is wrong, print error
        // 3. if packet is correct, send ack to client, otherwise, send reject.
        // 4. replace last packet segno to be current packet seqno, if segno = last seg + 1
        int rev = 1;
	int last_seg = 0;
	int response_type = 0;//0:ack, 1:out of sequence, 2:lenth mismatching, 3:End of packet missing, 4: duplicate
	DataPacket *dp = malloc(sizeof(DataPacket));
	while(rev>0){
		addr_size = sizeof(client_addr);
		uint8_t *buffer = (uint8_t *)dp;
        	rev = recvfrom(sockfd, buffer, sizeof(DataPacket), 0,(struct sockaddr*)&client_addr, &addr_size);
		printf("received %d bytes\n", rev);
		//show(*dp);
		//chek segmentNO  seg = last_seg + 1 or seg == last_seg
		if (dp->SegmentNo != last_seg + 1){
			if(dp->SegmentNo == last_seg){
				response_type = 4; //duplicate
			}
			else{
				 response_type = 1; //out of sequence
			}
			printf("\nlast_seg:%d\n",last_seg);
			printf("\nSegment No:%d\n",dp->SegmentNo);
		}
		last_seg = dp->SegmentNo;

		//check missing EndPacketId
		if (dp->EndPacketId = 0){
			response_type = 3;
		}

		//check payload length
		if(dp->Length != strlen(dp->Payload)){
			response_type = 2;
		}
		if (response_type == 0){
			ACKPacket *ap = malloc(sizeof(ACKPacket));
			*ap = generate_recv(*dp);
			//show_ack(*ap);
			buffer = (uint8_t *)ap;

	        	//for(int i = 0; i < sizeof(*ap); i++) {
        		//        printf("%x ", buffer[i]);
        		//}
        		//printf("\n");

			// Must use the addr_size from the previous recvfrom to specify addr length
			int send_ack = sendto(sockfd, buffer, sizeof(ACKPacket), 0, (struct sockaddr*)&client_addr, addr_size);
			printf("ACK send ack %d bytes, errno=%d\n", send_ack, errno);
		}
		else{
			RejectPacket *rjp = malloc(sizeof(RejectPacket));
			*rjp = generate_rej(*dp, response_type);
			//show_ack(*ap);
			buffer = (uint8_t *)rjp;

	        	//for(int i = 0; i < sizeof(*rjp); i++) {
        		//        printf("%x ", buffer[i]);
        		//}
        		//printf("\n");

			// Must use the addr_size from the previous recvfrom to specify addr length
			int send_rjk = sendto(sockfd, buffer, sizeof(RejectPacket), 0, (struct sockaddr*)&client_addr, addr_size);
			printf("Rej send  %d bytes, errno=%d\n", send_rjk, errno);

		}
       	return rev;

	}
}

void show(struct DataPacket dtp){
        printf("\nStart of Packet id:%x\n", dtp.StartPacketId);
        printf("\nClient ID:%x\n", dtp.ClientId);
        printf("\nDATA:%x\n",dtp.Data);
        printf("\nSegment No:%x\n", dtp.SegmentNo);
        printf("\nLength:%x\n", dtp.SegmentNo);
        printf("\nPayload:%x\n", dtp.Payload);
        printf("\nEnd of Packet id:%x\n", dtp.EndPacketId);

}
void show_ack(struct ACKPacket ap){

        printf("\nStart of Packet id:%x\n", ap.StartPacketId);
	printf("\nClient id:%x\n", ap.ClientId);
        printf("\nAck:%x\n", ap.Ack);
        printf("\nreceived no:%x\n", ap.ReceivedSegmentNo);
	printf("\nEnd of Packet id:%x\n", ap.EndPacketId);
}

void show_rej(struct RejectPacket rp){

        printf("\nStart of Packet id:%x\n", rp.StartPacketId);
        printf("\nClient id:%x\n", rp.ClientId);
        printf("\nRej:%x\n", rp.Reject);
        printf("\nreject sub no:%x\n", rp.Reject_sub_code);
	printf("\nreceived no:%x\n", rp.ReceivedSegmentNo);
        printf("\nEnd of Packet id:%x\n", rp.EndPacketId);
}

