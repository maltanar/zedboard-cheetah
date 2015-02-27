/*
 * Copyright (c) 2009-2013 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/tcp.h"
#ifdef __arm__
#include "xil_printf.h"
#endif

extern unsigned int finished;

int transfer_data() {
	return 0;
}

void print_app_header() {
	xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
	xil_printf("TCP packets sent to port 6001 will be echoed back\n\r");
}

typedef enum {
	IDLE, READ, WRITE, ERROR
} cheetahState;

cheetahState currentState = IDLE;
unsigned int * targetAddr = 0;
unsigned int bytesLeft = 0;
char responseBuffer[1024];
int sendResponse = 0;

void xmit(struct tcp_pcb * tpcb) {
	if (currentState != READ) {
		xil_printf("Cannot do sendData except in READ state!\n");
		currentState = ERROR;
		return;
	}

	while(bytesLeft > 0) {
		unsigned int tcpMaxSend = tcp_sndbuf(tpcb) / 8;
		unsigned int len = bytesLeft < tcpMaxSend ? bytesLeft : tcpMaxSend;
		err_t err = tcp_write(tpcb, (void *) targetAddr, len, 1);
		if (err != ERR_OK) {
			xil_printf("Error in sendData!\n");
			currentState = ERROR;
			return;
		}
		bytesLeft = bytesLeft - len;
		targetAddr += len / 4;
		xil_printf("xmit %x %d \n", targetAddr, len);
		tcp_output(tpcb);
	}

	currentState = IDLE;
	xil_printf("READ operation finished\n");
}

err_t sendCallback(void * arg, struct tcp_pcb * tpcb, u16_t len){

	bytesLeft = bytesLeft - len;
	targetAddr += len / 4;

	if (bytesLeft == 0) {
		currentState = IDLE;
		xil_printf("READ operation finished\n");
	} else
		xmit(tpcb);

	return ERR_OK;
}


void parseAndExecute(struct pbuf *p) {
	char * content = p->payload;

	if (p->len > 100) {
		xil_printf("Invalid command -- too long! (%d bytes)\n", p->len);
		return;
	}

	// otherwise, parse this command
	content[p->len] = 0;	// zero terminate, just in case

	char cmd = ' ';
	unsigned int cmdAddr = 0, cmdSize = 0;

	sscanf(content, "%c %x %x\n", &cmd, &cmdAddr, &cmdSize);

	switch (cmd) {
	case 'x':
		// exit
		finished = 1;
	case 'r':
		// read from ZedBoard memory
		if (cmdSize % 4 != 0)
			currentState = ERROR;
		else {
			currentState = READ;
			targetAddr = (unsigned int *) cmdAddr;
			bytesLeft = cmdSize;
			sprintf(responseBuffer,
					"Starting READ operation: %d bytes from 0x%x\n", bytesLeft,
					cmdAddr);
			sendResponse = 1;
		}
		break;
	case 'w':
		// write to ZedBoard memory
		if (cmdSize % 4 != 0)
			currentState = ERROR;
		else {
			currentState = WRITE;
			targetAddr = (unsigned int *) cmdAddr;
			bytesLeft = cmdSize;
			sprintf(responseBuffer,
					"Starting WRITE operation: %d bytes to 0x%x\n", bytesLeft,
					cmdAddr);
			sendResponse = 1;
		}

		break;
	default:
		sprintf(responseBuffer, "Unrecognized command, ignoring...\n");
	}
}

void handleWriteOp(struct pbuf *p) {
	if (p->len % 4 != 0 || p->len == 0) {
		xil_printf("Cannot handle write operation of size %d \n", p->len);
		currentState = ERROR;
		return;
	}
	bytesLeft = bytesLeft - p->len;

	// word-wise copy
	unsigned int * buf = (unsigned int *) p->payload;
	int i;
	for (i = 0; i < (p->len / 4); i++) {
		targetAddr[0] = buf[i];
		targetAddr++;
	}

	if (bytesLeft == 0) {
		currentState = IDLE;
		sprintf(responseBuffer, "WRITE operation finished\n");
		sendResponse = 1;
	}
}

void handleData(struct pbuf *p) {
	switch (currentState) {
	case IDLE:
		parseAndExecute(p);
		break;
	case WRITE:
		handleWriteOp(p);
		break;
	case ERROR:
		xil_printf(
				"Something is wrong! Cheetah is in the ERROR state. Do something!\n");
		break;
	}
}

err_t recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	/* do not read the packet if we are not in ESTABLISHED state */
	if (!p) {
		tcp_close(tpcb);
		tcp_recv(tpcb, NULL);
		return ERR_OK;
	}
	/* indicate that the packet has been received */

	handleData(p);

	if (sendResponse) {
		xil_printf(responseBuffer);
		sendResponse = 0;
		//tcp_write(tpcb, (void *) responseBuffer, strlen(responseBuffer), 1);
	}

	if (currentState == READ)
		xmit(tpcb);

	tcp_recved(tpcb, p->len);

	/* free the received pbuf */
	pbuf_free(p);

	return ERR_OK;
}

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
	static int connection = 1;

	/* set the receive callback for this connection */
	tcp_recv(newpcb, recv_callback);

	/* just use an integer number indicating the connection id as the
	 callback argument */
	tcp_arg(newpcb, (void*) connection);

	/* increment for subsequent accepted connections */
	connection++;

	return ERR_OK;
}

int start_application() {
	struct tcp_pcb *pcb;
	err_t err;
	unsigned port = 80;

	/* create new TCP PCB structure */
	pcb = tcp_new();
	if (!pcb) {
		xil_printf("Error creating PCB. Out of Memory\n\r");
		return -1;
	}

	/* bind to specified @port */
	err = tcp_bind(pcb, IP_ADDR_ANY, port);
	if (err != ERR_OK) {
		xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
		return -2;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(pcb, NULL);

	/* listen for connections */
	pcb = tcp_listen(pcb);
	if (!pcb) {
		xil_printf("Out of memory while tcp_listen\n\r");
		return -3;
	}

	/* specify callbacks to use  */
	tcp_accept(pcb, &accept_callback);
	//tcp_sent(pcb, &sendCallback);

	xil_printf("TCP echo server started @ port %d\n\r", port);

	return 0;
}
