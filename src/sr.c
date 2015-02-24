#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };



/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Statistics 
 * Do NOT change the name/declaration of these variables
 * You need to set the value of these variables appropriately within your code.
 * */
int A_application = 0;
int A_transport = 0;
int B_application = 0;
int B_transport = 0;

/* Globals 
 * Do NOT change the name/declaration of these variables
 * They are set to zero here. You will need to set them (except WINSIZE) to some proper values.
 * */
float TIMEOUT = 30.0;
int WINSIZE;         //This is supplied as cmd-line parameter; You will need to read this value but do NOT modify it; 
int SND_BUFSIZE = 0; //Sender's Buffer size
int RCV_BUFSIZE = 0; //Receiver's Buffer size

int send_seq_num ,send_base;
int rcvr_expseqnum;
int index = 1;
struct pkt packt_buffer[1000]; 
struct pkt rcv_buffer[1000];
int sent_pkts[1000];
int rcvr_base;
int rcv_index;
int rcv_sub_buffer[300];
int s_index = 0;
int c_index = 0;
int rcvr_winsize;
float time;
float oldest_timestamp;
int pckt_seqarray[1000];
int s_index;
struct pkt_timer *headq ;
struct pkt rcvr_ack_packet; 
float time;
void tolayer3(int AorB,struct pkt packet);
void tolayer5(int AorB,char datasent[20]);
void starttimer(int AorB,float increment);
void stoptimer(int AorB);

/* Code to Enque the packets */


/* The structure that holds the time and the packet details */
typedef struct pkt_timer{
           float s_time ;
           int send_base_seq;
           struct pkt  st_packet;
           struct pkt_timer *next;
        }pkt_timer;

/* The header of the Queue maintained for the packet timer*/
typedef struct pkt_timerQS{
  struct pkt_timer *front,*rear;
  }pkt_timerQS;


typedef struct pkt_timerQS *pkt_timerQ;
pkt_timerQ queuehead;

pkt_timerQ pq1,pq2;

/* Referred -https://www.cs.bu.edu/teaching/c/queue/linked-list/funcs.html for queue implementation */
/* The method creates a empty queue to which the pkt_timer elements can be added */

pkt_timerQ createpkttimerQ(void)
{
  pkt_timerQ queue;

  queue = (pkt_timerQ)malloc(sizeof(pkt_timerQS));
  if (queue == NULL) {
    fprintf(stderr, "Insufficient memory for new queue.\n");
    exit(1);  /* Exit program, returning error code. */
  }
  queue->front = queue->rear = NULL;
  return queue;
}

int isqempty(pkt_timerQ queue){

  if(queue->front == NULL){
    return 1;

  }else{
    return 0;
  }

}


/* The method removes the earliest element inserted */
pkt_timer*  deque(){

  pkt_timer *delNode = NULL;
  float time_nxtNode;

  if(!isqempty(queuehead)){

  delNode = (pkt_timer *)malloc(sizeof(pkt_timer));
  //delNode1 = queue->front;

    delNode = queuehead->front;
    if(delNode->next != NULL){
    time_nxtNode = delNode->next->s_time ;
    delNode->s_time = time_nxtNode - time + TIMEOUT ;
    delNode->send_base_seq = delNode->next->st_packet.seqnum;
    

   }else{
    delNode->s_time = TIMEOUT ;
    
   }
    if(delNode->next == NULL){
      queuehead->rear = NULL;
      queuehead->front = NULL;
    }else{
      queuehead->front = delNode->next;
    }
  }else{
    // delNode = (pkt_timer *)malloc(sizeof(pkt_timer));
    // delNode->s_time = TIMEOUT ;
  }
   return delNode;

}

int getnewtimeout(){

pkt_timer* deleteNode;
float time_out_val ;
int new_send_base;
deleteNode = deque();

if(deleteNode->next != NULL ){

   time_out_val = deleteNode->s_time ;   
   new_send_base = deleteNode->send_base_seq ;
   
}else{

  time_out_val = TIMEOUT ;   
  new_send_base = 1;
  
}

  stoptimer(0);
  starttimer(0,time_out_val);
  return new_send_base ;

}

void calculatetimeout(pkt_timerQ queuehead,int option){

 float timoutval;
 pkt_timer *deletedNode ;
 deletedNode = deque();
 //timoutval = deletedNode->s_time;
 if(option == 0){  
    if(deletedNode != NULL){
      timoutval = deletedNode->s_time;
      if(!(deletedNode->st_packet.seqnum == 0)){
         enque(queuehead,&deletedNode->st_packet);
         starttimer(0,timoutval);
         A_transport++;
        printdebug1("\nA_INTERRUPT:",&deletedNode->st_packet,0);
        // if(deletedNode->st_packet.seqnum == 0)
        // printf("Time :%f",deletedNode->s_time);
        tolayer3(0,deletedNode->st_packet); 
      } else{
        starttimer(0,timoutval);
      }
      
    }else{
      starttimer(0,TIMEOUT); 
     
    }
  
 }else{

  if(deletedNode != NULL){
    timoutval = deletedNode->s_time;
  }else{
     timoutval = TIMEOUT ; 
  }
  stoptimer(0);
  starttimer(0,timoutval);

 }
  
}

void killq(pkt_timerQ queue)
{
  while (!isqempty(queue))
    deque();

  queue->front = queue->rear = NULL;

  free(queue);
}

/* The method removes the packet based on the seq number value */
void removeNode(int acknum){

  pkt_timer *currNode,*prev;

  prev = NULL ;
  currNode = queuehead->front ;

  if(currNode->next == NULL){
    
       queuehead->front = NULL ;
       queuehead->rear = NULL ;
       free(currNode);
       return ;

  }

  while(currNode != NULL){

    if(currNode->st_packet.seqnum == acknum){

      if(prev == NULL){

        queuehead->front = currNode->next ;

      }else{

          prev->next = currNode->next;
          if(currNode->next == NULL){
          queuehead->rear = prev ;
          queuehead->rear->next = prev->next ;
        }
          
      }
        
      free(currNode);
      return;
    }

    prev = currNode ;
    currNode = currNode->next ;

  }

}


void printdebug1(char *func,struct pkt *packet,int option){

    int i;
    printf("\nCalled %s with:",func);
    if(option == 0)
    printf("\tseq:%d",packet->seqnum);
    else
    printf("\tack:%d\n",packet->acknum);  
}



// The method to remove the node
void enque(pkt_timerQ queue, struct pkt *packet)
{
  pkt_timer *newNodeP;

   newNodeP = (pkt_timer *)malloc(sizeof(pkt_timer));

  if (newNodeP == NULL) {
    fprintf(stderr, "Insufficient memory for new queue element.\n");
    exit(1);  /* Exit program, returning error code. */
  }

  newNodeP->st_packet.acknum = packet->acknum;
  newNodeP->st_packet.seqnum = packet->seqnum;
  newNodeP->st_packet.checksum = packet->checksum;
  strncpy(&newNodeP->st_packet.payload,packet->payload,20);
  newNodeP->s_time = time ;
  newNodeP->next = NULL;

  if (queue->front == NULL) {  /* Queue is empty */
    queue->front = queue->rear = newNodeP;
  } else {
    queue->rear->next = newNodeP;
    queue->rear = newNodeP;
  }
}



/* End of the Code to enqque the packets */         
/* THe method enque the packet into the queue structure with the current timestamp*/
void enquetimer(struct pkt *packet){

struct pkt_timer t_pkt_timer;
struct pkt_timer *tpkt;

/*copy the packet to the pkt_timer and also the content*/
t_pkt_timer.st_packet.checksum = packet->checksum;
strncpy(&t_pkt_timer.st_packet.payload,packet->payload,20);
t_pkt_timer.s_time = time ;

/* adding the packet_timer to the queue */                                              
  
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
}

/* THE method used for the debugging purpose*/
void traverseNodes(pkt_timerQ queue){
  
  pkt_timer *currNode;
  
  currNode = queue->front;
  
  while(currNode != NULL){

    printf("\nTHe values are :seqnum: %d   :time%f",currNode->st_packet.seqnum,currNode->s_time);

    currNode = currNode->next;
  }

}


/* The method calculate the checksum of the packet */
/* @arg1 - struct pkt packet
   @return - int                             */
int checksum(struct pkt *packet){

  int checksum_val = 0;
  int index ;

  checksum_val = packet->seqnum + packet->acknum ;

  for(index = 0;index < 20;index++){
    checksum_val += packet->payload[index];
  }

  return checksum_val;

}
/* The method generates a packet */
/* @arg1 - int seqnum
   @arg2 - struct msg message                           */

struct pkt make_spacket(int seqnum,struct msg message){

   struct pkt sender_pkt;
   sender_pkt.seqnum = seqnum ;
   sender_pkt.acknum = 0;
   strncpy(sender_pkt.payload,message.data,20);
   sender_pkt.checksum = checksum(&sender_pkt);
   return sender_pkt;

}


struct pkt make_rpacket(int seqnum){

   struct pkt rcvr_pkt ;
   rcvr_pkt.seqnum = 0 ;
   rcvr_pkt.acknum = seqnum;
   rcvr_pkt.checksum = checksum(&rcvr_pkt);
   return rcvr_pkt;

}

void data(){

  printf("Rc_base:%d Send_base %d",rcvr_base,send_base);

}

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
  
  struct pkt send_pkt,buff_pkt;
  struct pkt test_pkt,test_pkt1;
  pkt_timer *delNode ;
  int ipacket_index;
  A_application++;

  if(A_application == 1)
  queuehead = createpkttimerQ();
  //printf("Window Size %d",WINSIZE);

  if(send_seq_num  < (send_base + WINSIZE)){

    A_transport++;
    send_pkt = make_spacket(send_seq_num,message);
    packt_buffer[index++] = send_pkt ;
    c_index = index ;
    // Enque the packet to capture the timestamp
    if(A_application == 1 || queuehead->front == NULL){
      stoptimer(0);
      starttimer(0,TIMEOUT);
    }
    enque(queuehead,&send_pkt);
    
    printdebug1("\nA_OUTPUT:",&send_pkt,0);
    tolayer3(0,send_pkt);
    send_seq_num ++;
  }
  else{

     buff_pkt = make_spacket(A_application,message);
     packt_buffer[index++] = buff_pkt ;
     
  }
}

void B_output(message)  /* need be completed only for extra credit */
  struct msg message;
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;

{
   int iPcktIndex ;
   int new_send_base;
   int t;
   struct pkt currPacket;
   float newtimeoutval= 0.0f ;
   int index_rc;
   int incr;
   if(packet.checksum == checksum(&packet)){

    printdebug1("A_INPUT:",&packet,1);
    if(packet.acknum == 7){
        t= 0;
    }
    if(packet.acknum == send_base){
    
     //remove from the timer queue and calculate the new timeout
     //newtimeoutval =calculatetimeout(queuehead);
     if(queuehead->front != NULL){
     if(send_base == queuehead->front->st_packet.seqnum){
         new_send_base = getnewtimeout();
        //  if(new_send_base != 1){
        //  send_base = new_send_base ;
        // }else{
        //  send_base = send_base + 1 ;
        // }
      }else{
      removeNode(packet.acknum);
      }
     }else{
       stoptimer(0);
       starttimer(0,TIMEOUT);
     }
     // get the new send base values by iterating over the window
     incr = 1;
     for(index_rc = send_base + 1 ;index_rc <= send_base + WINSIZE ; index_rc++){

         if(sent_pkts[index_rc]!= 0){
            incr++;
         }else{
          break ;
         }

     }
     if(incr == 1){
     
      send_base = send_base + incr ;
     
     }else{

       send_base = sent_pkts[index_rc - 1] + 1;//send_base + incr + 1 ;
     }
     
     printf("del seq:%d",send_base);
     

    }else if(packet.acknum > send_base){
      
      if(!isqempty(queuehead))
      removeNode(packet.acknum);
      //sent_pkts[packet.acknum] = packet.acknum ;
      //remove from the timer queue
    }
      sent_pkts[packet.acknum] = packet.acknum ;  
   }
   
   int winsize_d;
   
   if(c_index != index){

   winsize_d = index < (send_base + WINSIZE ) ? index : (send_base + WINSIZE ) ;
   //Loop through the buffer and send the pending packets
   for(iPcktIndex = c_index ; iPcktIndex < winsize_d; iPcktIndex++){
    
    A_transport++ ;
    currPacket = packt_buffer[iPcktIndex];
    if(queuehead->front == NULL){
      stoptimer(0);
      starttimer(0,TIMEOUT);
    }
    enque(queuehead,&currPacket);
    tolayer3(0,currPacket);
    c_index++;
    send_seq_num++ ; 
   }
  }

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
   float newtimeoutval;
   calculatetimeout(queuehead,0);
   
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{

   send_seq_num = 1;
   send_base = 1;
   s_index = 1;
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
struct pkt packet;
{
 
 struct pkt ack_pkt ; 
 B_transport++;
 int ibuffindx = 0;
 int min_window = 0;
 struct pkt nxt_pkt;
 int inorder_flg = 0;
 int pcktindx;
 int incr1 = 0;

if(packet.seqnum == 967){

              inorder_flg = 0;
              inorder_flg = 0;


            }

 if(packet.checksum == checksum(&packet)){
   

   if(packet.seqnum == 68){

              inorder_flg = 0;
              inorder_flg = 0;


            }
    /* send an acknowledgement for the packet if it falls within the receivers window*/ 
   if(packet.seqnum >= rcvr_base  && packet.seqnum < rcvr_base + rcvr_winsize ){
      printdebug1("B_INPUT:",&packet,0);
      ack_pkt = make_rpacket(packet.seqnum);
      tolayer3(1,ack_pkt);

      if(packet.seqnum == rcvr_base){

            tolayer5(1,packet.payload);
            B_application++;

            rcvr_base = packet.seqnum + 1 ;
            
            min_window = s_index < rcvr_base + rcvr_winsize ? s_index : rcvr_base + rcvr_winsize;
            
          

            /* Code to check inorder received packet */
            incr1 = 1;
            for(ibuffindx = rcvr_base ;ibuffindx <=rcvr_base + rcvr_winsize + 1 ;ibuffindx++ ){

              if(pckt_seqarray[ibuffindx] != 0){
              B_application++;  
              tolayer5(1,rcv_buffer[ibuffindx].payload);  
                incr1++;  
              }else{

                break;
              }
            }
            
            if(incr1 == 1){
              
               //rcvr_base = rcvr_base ;

            }else{

               rcvr_base =  pckt_seqarray[ibuffindx - 1] + 1;

            } 
            /* Code to check inorder received packet */

            printf("recv seq:%d",rcvr_base);
       }else{ // buffer the data

             rcv_buffer[ack_pkt.acknum] = packet;
             pckt_seqarray[ack_pkt.acknum] = packet.seqnum;
             s_index = ack_pkt.acknum ;
             //sort(rcvr_base,rcv_index-1);

      }
      // if((packet.seqnum >= rcvr_base - rcvr_winsize) || (packet.seqnum <= rcvr_base -1)){
      // tolayer3(1,ack_pkt);
      //}
  }else{
         
        if(packet.seqnum < rcvr_base){ 
	      ack_pkt = make_rpacket(packet.seqnum);
          tolayer3(1,ack_pkt);
        }
  }

      /* Check if the packet falls within the receiver window
         if the seqnum is expected sequence# then update/increment the sendbase 
            -- check if there are any more in the buffer
         Else  buffer the packets 
      */ 

       }

     }

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
   rcvr_expseqnum = 1;
   rcvr_winsize = WINSIZE ;
   rcvr_base = 1 ;
   rcvr_ack_packet = make_rpacket(0);
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

//forward declarations
void init();
void generate_next_arrival();
void insertevent(struct event*);

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob = 0.0;	   /* probability that a packet is dropped */
float corruptprob = 0.0;   /* probability that one bit is packet is flipped */
float lambda = 0.0; 	   /* arrival rate of messages from layer 5 */
int ntolayer3 = 0; 	   /* number sent into layer 3 */
int nlost = 0; 	  	   /* number lost in media */
int ncorrupt = 0; 	   /* number corrupted by media*/

/**
 * Checks if the array pointed to by input holds a valid number.
 *
 * @param  input char* to the array holding the value.
 * @return TRUE or FALSE
 */
int isNumber(char *input)
{
    while (*input){
        if (!isdigit(*input))
            return 0;
        else
            input += 1;
    }

    return 1;
}

int main(int argc, char **argv)
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c;

   int opt;
   int seed;

   //Check for number of arguments
   if(argc != 5){
   	fprintf(stderr, "Missing arguments\n");
	printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
   	return -1;
   }

   /* 
    * Parse the arguments 
    * http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html 
    */
   while((opt = getopt(argc, argv,"s:w:")) != -1){
       switch (opt){
           case 's':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -s\n");
                           return -1;
                       }
                       seed = atoi(optarg);
                       break;

           case 'w':   if(!isNumber(optarg)){
                           fprintf(stderr, "Invalid value for -w\n");
                           return -1;
                       }
                       WINSIZE = atoi(optarg);
                       break;

           case '?':   break;

           default:    printf("Usage: %s -s SEED -w WINDOWSIZE\n", argv[0]);
                       return -1;
       }
   }
  
   init(seed);
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);  
             else
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
	//Do NOT change any of the following printfs
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
	
	printf("\n");
	printf("Protocol: SR\n");
	printf("[PA2]%d packets sent from the Application Layer of Sender A[/PA2]\n", A_application);
	printf("[PA2]%d packets sent from the Transport Layer of Sender A[/PA2]\n", A_transport);
	printf("[PA2]%d packets received at the Transport layer of Receiver B[/PA2]\n", B_transport);
	printf("[PA2]%d packets received at the Application layer of Receiver B[/PA2]\n", B_application);
	printf("[PA2]Total time: %f time units[/PA2]\n", time);
	printf("[PA2]Throughput: %f packets/time units[/PA2]\n", B_application/time);
	return 0;
}



void init(int seed)                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   srand(seed);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
}  

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


void insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;     /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
             evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
             q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(AorB,increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 //char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
 lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}
