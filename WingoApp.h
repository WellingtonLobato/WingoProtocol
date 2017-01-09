//*******************************************************************************
//*	Copyright (c) 2013. Federal University of Para (UFPA), Brazil and 	*
//*			    University of Bern (UBern), Switzerland		*
//*	Developed by Research Group on Computer Network and Multimedia		*
//*	Communication (GERCOM) of UFPA in collaboration to Communication and 	*
//*	Distributed Systems (CDS) research group of UBern.			*
//*	All rights reserved							*
//*										*
//*	Permission to use, copy, modify, and distribute this protocol and its	*
//*	documentation for any purpose, without fee, and without written		*
//*	agreement is hereby granted, provided that the above copyright notice,	*
//*	and the author appear in all copies of this protocol.			*
//*										*
//*  	Module: Application to camera nodes with node mobility and QoE-aware FEC*
//*										*
//*	Ref.:	Z. Zhao, T. Braun, D. Rosário, E. Cerqueira, R. Immich, and 	*
//*		M. Curado, “QoE-aware FEC mechanism for intrusion detection in 	*
//*		multi-tier wireless multimedia sensor networks,” 		*
//*		in Proceedings of the 1st International Workshop on Wireless 	*
//*		Multimedia Sensor Networks (WiMob’12 WSWMSN), Barcelona, Spain,	*
//*		oct. 2012, pp. 697–704.						*
//*										*
//*  	Version: 2.0								*
//*  	Authors: Denis do Rosário <denis@ufpa.br>				*
//*										*
//******************************************************************************/ 


#ifndef PLATOONAPP_H_
#define PLATOONAPP_H_

#include "WiseBaseApplication.h"
#include "VirtualMobilityManager.h"
#include "VirtualMac.h"
#include <vector>

enum ValueReportingTimers {
	REQUEST_MULTIMEDIA 	= 1,
	TRACE_MOVEMENT		= 2,
	GROUP               = 3,
	SELECTION           = 4,
	REPEAT              = 5,
};



class WingoApp: public WiseBaseApplication {
public:
    WingoApp();
	virtual ~WingoApp();

protected:
	struct video{
		int ch;
		int sn;
		int id;
		FILE *pFile;
	};
	struct fec_frames {
		u_char** payload;
		int* idx;
		std::vector < uint64_t > packet_uid;
		int k;		// number of packets without redundancy
		int n;		// number of redundant pkts
		int eid;	// evalvid id
		int fhs;	// FEC header size
		int fds;	// FEC data size
		int count;	// number of received packets
		int nodeId;	// source node Id
	};

	struct fec_parameters {
		unsigned int discard_eid_packets;
		int nodeId;
	};

	struct CarMovements{
	    int idCar;
	    double xCar;
	    double yCar;
	    double vCar;
	    double ufCar;
	};
    //PSO
    int numeroPopulacao;
    int geracao;


	virtual void initialize();
	virtual void startup() {}
	virtual void finish();
	virtual void finishSpecific() {};
	virtual void fromNetworkLayer(cPacket* pkt, const char* src, double rssi, double lqi);
	virtual void handleMobilityControlMessage(MobilityManagerMessage *);
	virtual void handleSensorReading(WiseSensorMessage* msg);
	virtual void handleDirectApplicationMessage(WiseApplicationPacket* pkt);

	void processAppPacket(WiseApplicationPacket* pkt);
	void timerFiredCallback(int);
	void writeRdTrace(int id,int seqNum);
	void AddReceivedTrace(double, bool, TraceInfo tParam);
	bool FillPacketPool(int, int, const u_char *, int);
	bool rebuildFECBlock(int, int, int);
	bool ClearPacketPool(int);
	bool enoughPacketsReceived(int, int);

	bool isSink;
	int idVideo;
	CarMovements auxMovementsCar;
	std::list<video> set;
	std::list<video>::iterator crtlIterator;
	vector <fec_frames> pktPool;
	vector <fec_parameters> discardEidPkts;
	VirtualMobilityManager* mobilityModule;
	VirtualMobilityManager* OraculoSrc;
	VirtualMac *macModule;
	NodeLocation location;
	NodeLocation OraculoSrclocation;
	NodeLocation tempLocation;

    std::list<CarMovements> carList; //Criar lista de id's que responderam com CNNC
    std::list<CarMovements>::iterator carEsc; //Carro escolhido
    bool in_coligate; //Verificar se o veiculo está fazendo alguma coligação
    int type;
    int destino_final;
    double timeToSend;
    bool isSource;
    bool headOn;
    int SourceHead;
    //Informações da Origem
    int idOrig;
    double xOrig;
    double yOrig;
    double vOrig;
    //Informações do Destino
    int idDst;
    double xDst;
    double yDst;
    //GT
    double uf;
    double lucro;
    double gama;
    double distanciaPontos;
    //Variaveis para a ordenação
    int cS; //Contador do SELECTION
    int menorS; //Menor valor inicial
    int posMenor; //Posição do menor valor da UF

};

#endif /* CRITAPP_H_ */
