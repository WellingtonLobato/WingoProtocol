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

#include "WingoApp.h"
#include "WiseDebug.h"
#include "math.h" //Biblioteca matematica
#include "time.h" //Biblioteca para escolher aleatorio
#include "algorithm"
#include <list>
#include <cctype>

Define_Module(WingoApp);

WingoApp::WingoApp() {
}

WingoApp::~WingoApp() {
}

void WingoApp::initialize() {
	WiseBaseApplication::initialize();
	isSink = par("isSink");
    in_coligate = par("in_coligate");
    type = par("type");
    destino_final = par("destino_final");
    isSource = par("isSource");
    timeToSend = par("timeToSend");
	idVideo = -1;
	cModule* mod = getParentModule()->getModuleByRelativePath("MobilityManager");
	headOn = false;
	//cModule* OraculoSrcmod = getParentModule()->getModuleByRelativePath("MobilityManager");
	//OraculoSrc = check_and_cast <VirtualMobilityManager*>(getParentModule()->getParentModule()->getParentModule()->getSubmodule("node",rcvPackets->getSrc_final())->getSubmodule("MobilityManager"));

	mobilityModule = check_and_cast<VirtualMobilityManager*>(mod);
	location = mobilityModule->getLocation();
	tempLocation = mobilityModule->getLocation();

	macModule = check_and_cast <VirtualMac*>(getParentModule()->getSubmodule("Communication")->getSubmodule("MAC"));



	//trace() << "Tamanho do buffer: " << macModule->getBufferSize() << " pkts";
	//trace() << "taxa de envio: " << macModule->dataRate_out << " mbits/s";
	//trace() << "taxa de recepcao: " << macModule->dataRate_in << " mbits/s";

	if(isSource){
        if(type == 1){ //Se o valor da variavel type no arquivo .ini for igual a 1, então executar estas ações (Valor modificado no arquivo .ini)
            setTimer(GROUP, timeToSend);
            //ID origem e ID destino
            idOrig = self;
            idDst = destino_final;
        }else{
            setTimer(REQUEST_MULTIMEDIA, timeToSend);
        }
	}

}

void WingoApp::finish() {
	WiseBaseApplication::finish();
	crtlIterator = set.begin();
	while(crtlIterator != set.end()){
		video cls  = *crtlIterator;				
		fclose(cls.pFile);		
		crtlIterator = set.erase(crtlIterator);
	}
	cOwnedObject *Del=NULL;
	int OwnedSize = this->defaultListSize();
	for(int i=0; i<OwnedSize; i++){
		Del = this->defaultListGet(0);
		this->drop(Del);
		delete Del;
	}
}


void WingoApp::timerFiredCallback(int index){
	switch (index) {
		case REQUEST_MULTIMEDIA:{
		    trace() << "####################REQUEST_MULTIMEDIA###########################";
			idVideo ++;
			WiseApplicationPacket* m = new WiseApplicationPacket("request multimedia", MULTIMEDIA_REQUEST_MESSAGE);
			m->setIdNode(self);
			m->setIdVideo(idVideo);
			send(m, "toSensorDeviceManager");
			trace() << "sender restart round to " << idVideo << "\n";
			//if (idVideo < 0) //qtd de vídeos a serem transmitidos
			//	setTimer(REQUEST_MULTIMEDIA, 20);
			break;
		}
		case TRACE_MOVEMENT:{
			location = mobilityModule->getLocation();
			if (tempLocation.x != location.x || tempLocation.y != location.y){
				tempLocation = mobilityModule->getLocation();
				WiseApplicationPacket* m = new WiseApplicationPacket("update location", MOBILE_MESSAGE);
				m->setSource(self);
				m->setX(location.x);
				m->setY(location.y);
				send(m, "toSensorDeviceManager");
			}
			setTimer(TRACE_MOVEMENT, 5);
			break;
		}
		case GROUP:{
            trace() << "********************GROUP***************************";
            location = mobilityModule->getLocation(); //Localização do nó

            if(headOn){
                trace() << "LIDER-Enviando mensagem de novo para todos...";
                WiseApplicationPacket *beacon1 = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE);
                beacon1->setAppPacketKind(APP_LEADER);
                beacon1->setSource(self);
                beacon1->setSrc_head(SourceHead);
                beacon1->setHead(true);
                toNetworkLayer(beacon1, BROADCAST_NETWORK_ADDRESS);//envio da msg externa em broadcast para os outros nós
                headOn = false;
            }

            if(isSource){
                if(mobilityModule->getSpeed() >= 5.0 && mobilityModule->getSpeed() <= 100.0){
                    trace() << "NÓ-INICIAL: " << self;
                    trace() << "SPPED LEADER: " << mobilityModule->getSpeed();
                    vOrig = mobilityModule->getSpeed(); //Velocidade do Lider
                    xOrig = location.x;
                    yOrig = location.y;
                }
            }
            gama = mobilityModule->getSpeed()+167.0+(rand() % 5);

            WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE); //Cria um pacote que será enviado para os demais nós
            beacon->setX(location.x);
            beacon->setY(location.y);
            beacon->setSource(self);
            beacon->setSpeed(mobilityModule->getSpeed());
            beacon->setGama_s(gama);
            beacon->setDestino_final(idDst);
            beacon->setAppPacketKind(APP_NTFY);
            beacon->setSrc_final(idOrig);
            beacon->setXOrig(xOrig);
            beacon->setYOrig(yOrig);
            beacon->setVOrig(vOrig);
            toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS); //envio da msg externa em broadcast para os outros nos

            trace() << "WVL - 1° Etapa para coligação";
            trace() << "Node " << self << " is moving at " << mobilityModule->getSpeed() << "m/s";
            trace() << "Node " << self << " location x: " << location.x << " location y: " << location.y;
            trace() << "Node " << self << " sending a GROUP-NTFY message to network layer in broadcast";
            trace() << "Valor para participar da coligação, GAMA: " << gama << "\n";
            setTimer(SELECTION, 0.05);
		    break;
		}
		case SELECTION:{
		    trace() << "!!!!!!!!!!!!!!!!!!!!SELECTION!!!!!!!!!!!!!!!!!!!!!!!!!!!";
		    if(carList.size() != 0){
		        cS = 0;
		        for(list<CarMovements>::iterator it = carList.begin(); it != carList.end();it++){
                    trace() << "New_car ";
                    trace() << "ID: " << it->idCar;
                    trace() << "X: " << it->xCar;
                    trace() << "Y: " << it->yCar;
                    trace() << "Velocidade: " << it->vCar;
                    trace() << "UFI: " << it->ufCar << "\n";
                    if(cS == 0){
                        menorS = abs(it->ufCar-gama);
                        posMenor = 0;
                    }else{
                        if(abs(it->ufCar-gama) < menorS){
                            posMenor = cS;
                            menorS = abs(it->ufCar-gama);
                        }
                    }
                    cS++; //Incrementa o valor de cS
		        }
		        carEsc = carList.begin();
                advance(carEsc, posMenor);

                //Carro escolhido pelo menor valor de UFi
                trace() << "Carro escolhido: " << carEsc->idCar;
                trace() << "Velocidade de Coligação: " << vOrig;
                trace() << "Velocidade atual do nó: " << carEsc->vCar;
                trace() << "X destino: " << carEsc->xCar;
                trace() << "Y destino: " << carEsc->yCar;
                trace() << "Valor da UFi: " << carEsc->ufCar << "\n";

                WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE);
                beacon->setDestinationX(mobilityModule->getX_Final());
                beacon->setDestinationY(mobilityModule->getY_Final());
                beacon->setDestination(carEsc->idCar);
                beacon->setSpeed_final(vOrig);
                beacon->setDestino_final(destino_final);
                beacon->setSrc_final(idOrig);
                beacon->setX(location.x);
                beacon->setY(location.y);
                beacon->setAppPacketKind(APP_CRTY);
                beacon->setSource(self);
                toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS);//envio da msg externa em broadcast para os outros nós

                WiseApplicationPacket *ctlMsg = new WiseApplicationPacket("next hop", APPLICATION_PLATOON); //informar o routing do next hop
                ctlMsg->setNextHop(carEsc->idCar); //colocar qual sera o proximo salto
                ctlMsg->setSource(idOrig); //colocar o nó que originou a criacao do platoon
                ctlMsg->setDestination(destino_final); //colocar qual sera o proximo salto
                toNetworkLayer(ctlMsg, BROADCAST_NETWORK_ADDRESS); //envio da msg externa em broadcast para os outros nos
                if (self == idOrig){
                    trace() << "NADA!!!\n";
                    //setTimer(REQUEST_MULTIMEDIA, 0);
                }
                setTimer(REPEAT, 0.8);

		    }else{
		        trace() << "Lista vazia, não há candidatos para coligação\n";
		        setTimer(GROUP, 0.1);
		    }
		    break;
		}
		case REPEAT:{
		    trace() << "++++++++++++++++++++REPEAT+++++++++++++++++++++++++++";
            if(headOn == false){
                trace() << "NOVO Lider não respondeu\n";
                carList.clear();
                setTimer(GROUP, 0.1);
            }else{
                trace() << "NOVO Lider respondeu\n";
                cancelTimer(GROUP); //Cancela qualquer nova coligação que esteja acontecendo
                cancelTimer(SELECTION);
            }
		    break;
		}
	}
}

void WingoApp::AddReceivedTrace(double time, bool generateTrace, TraceInfo tParam) {
	if (generateTrace){
		if(set.empty()){
			writeRdTrace(tParam.nodeId,tParam.seqNum);
			//trace() << "Frame " << tParam.id << " added";
		}
		for (crtlIterator = set.begin(); crtlIterator != set.end(); crtlIterator++){
			video info = *crtlIterator;
			/*
		 	 * Test if the output file is already created
		 	 * If the packtes is for the current seq number save the frame on the output file
		 	 * else close the file and create the new output file with the new seq number
			*/
			if (tParam.nodeId == info.ch && tParam.seqNum == info.sn){
				fprintf(info.pFile, "%f             id %d            udp %d\n",time, tParam.idFrame, tParam.byteLength);
				WiseApplicationPacket *pktTrace = new WiseApplicationPacket("received video frame", APPLICATION_STATISTICS_MESSAGE);
				pktTrace->setIdVideo(info.sn);
				pktTrace->setIdNode(info.ch);
				pktTrace->setIdFrame(0);
				toNetworkLayer(pktTrace, BROADCAST_NETWORK_ADDRESS);
				//trace() << "Frame " << tParam.id << " added";
			} else if (tParam.nodeId != info.ch || tParam.seqNum != info.sn){
		   		fclose(info.pFile);
				crtlIterator = set.erase(crtlIterator);
				writeRdTrace(tParam.nodeId, tParam.seqNum);
			}
		}
	}
}

bool WingoApp::FillPacketPool(int packet_uid, int fec_data_size, const u_char *tmp_packet, int nodeId) {
	// carregando os campos
	fec_frames temp;
	temp.fhs	= (int) tmp_packet[0];	// fec header size 
	temp.k		= (int) tmp_packet[1];	// number of packets without redundancy
	temp.n		= (int) tmp_packet[2];	// number of redundant pkts
	temp.eid	= (int) tmp_packet[4];	// evalvid id
	temp.fds	= fec_data_size; 	// fec data size
	temp.nodeId	= nodeId;		// source node Id
	temp.count	= 0;			// TODO: verificar

	// preciso ajustar os uid dos pacotes, pq senao posso ficar com id's diferentes no sd_file e rd_file
	// TODO: melhorar a explicacao da necessidade de ajuste do uid
	int adjusted_uid = (packet_uid-tmp_packet[3])+temp.count;
	temp.packet_uid.push_back(adjusted_uid);

	// reservando espaco para o array dinamico (utilizado array pq o fec decode nao aceita vector)
	temp.idx = new int[temp.k];
	temp.idx[temp.count] = (int)tmp_packet[3];
	
	// reservando espaco para o buffer
	temp.payload = (u_char**)malloc(temp.k * sizeof(void *));
	for (int i = 0 ; i < temp.k ; i++ ) 
		temp.payload[i] = (u_char*)malloc(temp.fds);

	memmove(temp.payload[temp.count], tmp_packet+temp.fhs, temp.fds);

	temp.count++;
	pktPool.push_back(temp);
	trace() << "    pktPool size " << pktPool.size();
	return true;
}

bool WingoApp::rebuildFECBlock(int nodeId, int seqNum, int index){
	if (pktPool[index].count>0) {
		trace() << "    rebuildFECBlock::received " << pktPool[index].count << " of " << pktPool[index].n << " k: " << pktPool[index].k << " eid: " << pktPool[index].eid << " for node id " << pktPool[index].nodeId;
		int k(pktPool[index].k);
		int n(pktPool[index].n);
		int sz(pktPool[index].fds);
		
		ReedSolomonFec neoFec;
		void *newCode = neoFec.create(k,n);

		if (neoFec.decode((fec_parms *)newCode, (void**)pktPool[index].payload, pktPool[index].idx, sz)) {
			//trace() << "    rebuildFECBlock::detected singular matrix ..." << " n " << n;
		} 
		
		u_char **d_original = (u_char**)neoFec.safeMalloc(k * sizeof(void *), "d_original ptr");
		for (int i = 0 ; i < k ; i++ ) {
			d_original[i] = (u_char*)neoFec.safeMalloc(sz, "d_original data");
		}
		
		neoFec.BuildSampleData(d_original, k, sz);

		int errors(0);
		for (int i=0; i<k; i++){
			if (bcmp(d_original[i], pktPool[index].payload[i], sz )) {
				errors++;
				trace() << "    " << errors << " - rebuildFECBlock::error reconstructing block " << i << " k " << k << " n " << n;
			}
		}
		int included = 0;
		// adiciono o log separadamente da validacao pq algumas vezes dava erro em relacao aos pacotes
		// recuperados ... ex recebia 1,2,4 de 4 ... mas o fec recuperava 2,3,4 ocasionando pois nao
		// havia o ID do pacote 3 (que foi perdido)
		// entao desconsidero quais foram recuperados e adiciono os que possuem ID valido
		for (int i=0; i<k-errors; i++) {
			TraceInfo tParam;
			tParam.idFrame = pktPool[index].packet_uid[i];
			tParam.byteLength = sz + 5;
			tParam.nodeId = nodeId;
			tParam.seqNum = seqNum;
			AddReceivedTrace(SIMTIME_DBL(simTime()), true, tParam);
			included ++;
			trace() << "    whith FEC included => node id " << tParam.nodeId << " seq number " << tParam.seqNum << " frame id " << tParam.idFrame << "\n";
			if (included == pktPool[index].k){
					WiseApplicationPacket *pktTrace = new WiseApplicationPacket("received video frame", APPLICATION_STATISTICS_MESSAGE);
					pktTrace->setIdVideo(tParam.seqNum);
					pktTrace->setIdNode(tParam.nodeId);
					pktTrace->setIdFrame(1);
					toNetworkLayer(pktTrace, BROADCAST_NETWORK_ADDRESS);
			}
		}
		
		neoFec.destroy((fec_parms*)newCode);
		if (d_original != NULL) {
			for (int i = 0 ; i < k ; i++ ){ 
				free(d_original[i]);
				d_original[i] = NULL;
			}
		}
		free(d_original);
		d_original = NULL ;
		ClearPacketPool(index);
		trace() << "    pktPool size " << pktPool.size();
		return true;
	}
	return false;
}

bool WingoApp::ClearPacketPool(int index) {
	pktPool.erase(pktPool.begin()+index);
	return true;
}

bool WingoApp::enoughPacketsReceived(int index, int indexEidPkts) {
	trace() << "    enoughPacketsReceived";
	trace() << "    discard_eid_packets: " << discardEidPkts[indexEidPkts].discard_eid_packets << " count: " << pktPool[index].count << " k-1: " << pktPool[index].k-1;
	if ( (discardEidPkts[indexEidPkts].discard_eid_packets == 0) && (pktPool[index].count > (pktPool[index].k-1)) ) {
		trace() << "    true";
		return true;
	}
	trace() << "    false\n";
	return false;
}

void WingoApp::handleMobilityControlMessage(MobilityManagerMessage* pkt) {
}

void WingoApp::fromNetworkLayer(cPacket* msg, const char* src, double rssi, double lqi) {
	switch (msg->getKind()) {
		case APPLICATION_MESSAGE:{
			WiseApplicationPacket *rcvPackets = check_and_cast<WiseApplicationPacket*>(msg); //Enviar mensagem caso queira coligar ou descoligar
			switch (rcvPackets->getAppPacketKind()) {
			    case APP_NTFY:{
                    location = mobilityModule->getLocation();
                    if(location.x > rcvPackets->getX()){
                        trace() << "ZDI=IN";
                        if(in_coligate == false){
                            trace() << "IN_GROUP=FALSE";
                            trace() << "DISTÂNCIA: " << sqrt(pow((location.x-rcvPackets->getX()),2)+pow((location.y-rcvPackets->getY()),2));
                            distanciaPontos = sqrt(pow((location.x-rcvPackets->getX()),2)+pow((location.y-rcvPackets->getY()),2));
                            trace() << "VEL=" << mobilityModule->getSpeed();
                            if(mobilityModule->getSpeed() != 0.0 &&  mobilityModule->getSpeed() <= 100.0 && mobilityModule->getSpeed() >= 5.0){
                                trace() << "VEL=OK";
                                trace() << "FINAL_TIME: " << mobilityModule->getStartTime_Final();
                                if(simTime() > mobilityModule->getStartTime_Final()){
                                    trace() << "DEAD_NODE=TRUE";
                                    MobilityManagerMessage* pkt = new MobilityManagerMessage("teste", MOBILE_MESSAGE); //envio de uma msg interna para o MM
                                    pkt->setMobilePacketKind(NODE_DIE);
                                    toMobilityManager(pkt);
                                }else{
                                    trace() << "ALIVE_NODE=TRUE";
                                    lucro=10;//Valor Qualquer, apenas pra passar da condição
                                    uf = (mobilityModule->getSpeed() + distanciaPontos + (rand() % 5));
                                    xOrig = rcvPackets->getXOrig();
                                    yOrig = rcvPackets->getYOrig();
                                    idDst = rcvPackets->getDestino_final();
                                    vOrig = rcvPackets->getVOrig();

                                    if(lucro > 0){
                                        trace() << "LUCRO=TRUE";
                                        if(self == rcvPackets->getDestino_final()){
                                            trace() << "DIST_FINAL=TRUE";
                                            //Verificar a distância do nó destino, o nó não pode estar muito distante do nó que irá enviar.
                                            if((sqrt(pow((location.x-rcvPackets->getX()),2)+pow((location.y-rcvPackets->getY()),2))) <= 250.0){
                                                trace() << "DIST=IN";
                                                trace() << "WVL - 2° Etapa para coligação";
                                                trace() << "WINGO_COLIGATE-O nó: " << self << " pode participar da coligação. Sua velocidade é: " << mobilityModule->getSpeed() << ".";
                                                trace() << "VELOCIDADE DO NÓ: " <<  mobilityModule->getSpeed();
                                                trace() << "VELOCIDADE DA FONTE: " << rcvPackets->getSpeed();
                                                SourceHead = rcvPackets->getSource();

                                                WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE); //Cria um pacote que será enviado para os demais nós
                                                beacon->setX(location.x);
                                                beacon->setY(location.y);
                                                beacon->setSource(self);
                                                beacon->setDestination(rcvPackets->getSource());
                                                beacon->setSpeed(mobilityModule->getSpeed());
                                                beacon->setUf(uf);
                                                beacon->setAppPacketKind(APP_CNNC);
                                                toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS); //envio da msg externa em broadcast para os outros nos


                                                /*OraculoSrclocation = OraculoSrc->getLocation();
                                                trace() << "INFORMAÇÕES DO ORACULO";
                                                trace() << OraculoSrc->getId();
                                                trace() << OraculoSrclocation.x;
                                                trace() << OraculoSrclocation.y;
                                                trace() << OraculoSrc->getSpeed();*/


                                            }else{
                                                trace() << "DIST=OUT";
                                            }
                                        }else{
                                            trace() << "DIST_FINAL=FALSE";
                                            if((sqrt(pow((location.x-rcvPackets->getX()),2)+pow((location.y-rcvPackets->getY()),2))) <= 250.0){
                                                trace() << "DIST=IN";
                                                trace() << "WVL - 2° Etapa para coligação";
                                                trace() << "WINGO_COLIGATE-O nó: " << self << " pode participar da coligação. Sua velocidade é: " << mobilityModule->getSpeed() << ".";
                                                trace() << "VELOCIDADE DO NÓ: " <<  mobilityModule->getSpeed();
                                                trace() << "VELOCIDADE DA FONTE: " << rcvPackets->getSpeed();
                                                SourceHead = rcvPackets->getSource();

                                                WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE); //Cria um pacote que será enviado para os demais nós
                                                beacon->setX(location.x);
                                                beacon->setY(location.y);
                                                beacon->setSource(self);
                                                beacon->setDestination(rcvPackets->getSource());
                                                beacon->setSpeed(mobilityModule->getSpeed());
                                                beacon->setUf(uf);
                                                beacon->setAppPacketKind(APP_CNNC);
                                                toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS); //envio da msg externa em broadcast para os outros nos

                                                /*OraculoSrc = check_and_cast <VirtualMobilityManager*>(getParentModule()->getParentModule()->getParentModule()->getSubmodule("node",rcvPackets->getSrc_final())->getSubmodule("MobilityManager"));
                                                OraculoSrclocation = OraculoSrc->getLocation();
                                                trace() << "INFORMAÇÕES DO ORACULO";
                                                trace() << OraculoSrc->getId();
                                                trace() << OraculoSrclocation.x;
                                                trace() << OraculoSrclocation.y;
                                                trace() << OraculoSrc->getSpeed();*/

                                            }else{
                                                trace() << "DIST=OUT";
                                            }
                                        }

                                    }else{
                                        trace() << "LUCRO=FALSE";
                                    }
                                }
                            }
                        }else{
                            trace() << "IN_GROUP=TRUE";
                        }
                    }else{
                        trace() << "ZDI=OUT";
                        if(self == rcvPackets->getDestino_final()){
                            trace() << "Caso especial. Nó destino ficou para trás.";
                            trace() << "Achamos o destino final.";
                            SourceHead = rcvPackets->getSource();
                            uf = (mobilityModule->getSpeed() + distanciaPontos + (rand() % 5));
                            xOrig = rcvPackets->getXOrig();
                            yOrig = rcvPackets->getYOrig();
                            idDst = rcvPackets->getDestino_final();
                            vOrig = rcvPackets->getVOrig();
                            WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE); //Cria um pacote que será enviado para os demais nós
                            beacon->setX(location.x);
                            beacon->setY(location.y);
                            beacon->setSource(self);
                            beacon->setDestination(rcvPackets->getSource());
                            beacon->setSpeed(mobilityModule->getSpeed());
                            beacon->setUf(uf);
                            beacon->setAppPacketKind(APP_CNNC);
                            toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS); //envio da msg externa em broadcast para os outros nos

                        }
                    }
                    trace() << "\n";
                    break;
			    }
			    case APP_CNNC:{
			        trace() << "Nó " << self << " recebeu CNNC do nó " <<  rcvPackets->getSource();
			        if(self == rcvPackets->getDestination()){ //Somente o nó que enviou o COLIGATE-NTFY poderá receber o COLIGATE-CRTY
			            trace() << "WVL - 3° Etapa para coligação";
			            in_coligate = true;
                        if(rcvPackets->getSpeed() != 0.0){
                                //Informações dos carros que responderam o NTFY
                                auxMovementsCar.idCar = rcvPackets->getSource();
                                auxMovementsCar.xCar = rcvPackets->getX();
                                auxMovementsCar.yCar = rcvPackets->getY();
                                auxMovementsCar.vCar = rcvPackets->getSpeed();
                                auxMovementsCar.ufCar = rcvPackets->getUf();
                                carList.push_back(auxMovementsCar);
                        }else{
                            trace() << "O nó: " << rcvPackets->getSource() << " não pode participar pois tem velocidade igual a 0.";
                        }
			        }
			        break;
			    }
			    case APP_CRTY:{
			        if(self == rcvPackets->getDestination()){ //Somente o nó que enviou o COLIGATE-CNNC e foi escolhido poderá receber o COLIGATE-CRTY
			            destino_final = rcvPackets->getDestino_final();
			            vOrig = rcvPackets->getSpeed_final();
			            idOrig = rcvPackets->getSrc_final();
			            trace() << "WVL - 4° Etapa para coligação";
			            trace() << "velocidade antiga: " <<  mobilityModule->getSpeed();
			            trace() << "Velocidade futura: " << rcvPackets->getSpeed_final();

                        MobilityManagerMessage* pkt = new MobilityManagerMessage("teste", MOBILE_MESSAGE); //envio de uma msg interna para o MM
                        pkt->setMobilePacketKind(COLIGATE);
                        pkt->setXCoorDestination(rcvPackets->getDestinationX());
                        pkt->setYCoorDestination(rcvPackets->getDestinationY());
                        pkt->setXSrc(rcvPackets->getX());
                        pkt->setYSrc(rcvPackets->getY());
                        pkt->setSpeed_final(rcvPackets->getSpeed_final()); //A velocidade final deve ser a menor
                        toMobilityManager(pkt);

			            in_coligate = true;

                        if(destino_final != self){
                            trace() << "Novo lider platoon...";
                            headOn = true;
                            carList.clear(); //Limpa a lista de carros, para escolher novos carros
                            WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE);
                            beacon->setAppPacketKind(APP_LEADER);
                            beacon->setSource(self);
                            beacon->setSrc_head(SourceHead);
                            beacon->setHead(true);
                            toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS);//envio da msg externa em broadcast para os outros nós

                            trace() << "LIDER-Enviando mensagem de novo para todos...\n";

                            setTimer(GROUP, 0.1);
                        }else{
                            trace() << "Chegou no destino.";
                            WiseApplicationPacket *beacon = new WiseApplicationPacket("BEACON MSG TO GAME THEORY", APPLICATION_MESSAGE);
                            beacon->setAppPacketKind(APP_LEADER);
                            beacon->setSource(self);
                            beacon->setSrc_head(SourceHead);
                            beacon->setHead(true);
                            toNetworkLayer(beacon, BROADCAST_NETWORK_ADDRESS);//envio da msg externa em broadcast para os outros nós

                            trace() << "LIDER_DESTINO-Enviando mensagem de novo para todos...\n";
                        }

			        }
			        break;
			    }
			    case APP_LEADER:{
                    trace() << "Mensagem do Novo Lider " << rcvPackets->getSource() << " recebida. Antigo lider: " << rcvPackets->getSrc_head();
                    if(self == rcvPackets->getSrc_head()){
                        trace() << "Eu era o antigo lider";
                        headOn = rcvPackets->getHead();
                        if(headOn){
                            trace() << "Novo head: True\n";
                        }else{
                            trace() << "Novo head: False\n";
                        }
                    }
                    break;
			    }
			}
			break;
		}
		case MULTIMEDIA_PACKET:{
			WiseApplicationPacket *rcvPackets = check_and_cast<WiseApplicationPacket*>(msg);
			TraceInfo tParam = rcvPackets->getInfo();
			//se o tamanho do fec_header for 0, singifica que eh um pacote no fec
			if(rcvPackets->getFecPktArraySize() == 0){
				trace() << "APP- Node " << self << " received an " << rcvPackets->getInfo().frameType << "-frame with packet number " << rcvPackets->getInfo().idFrame << " from node " << rcvPackets->getInfo().nodeId << " for video id " << rcvPackets->getIdVideo() << " -- whithout FEC";
				int indexPool = -1;
				for(int i =0; i<pktPool.size(); i++){
					if(pktPool[i].nodeId == rcvPackets->getInfo().nodeId){
						indexPool = i;
						break;
					}
				}
				if (indexPool != -1){
					rebuildFECBlock(tParam.nodeId, tParam.seqNum, indexPool);
					for(int i =0; i<discardEidPkts.size(); i++){
						if(discardEidPkts[i].nodeId == rcvPackets->getInfo().nodeId){
							discardEidPkts[i].discard_eid_packets = 0;
							trace() << "    discard_eid_packets " << discardEidPkts[i].discard_eid_packets;
							break;
						}
					}
				}
				// adicionar como pacote no fec
				AddReceivedTrace(SIMTIME_DBL(simTime()), true, tParam);
				WiseApplicationPacket *pktTrace = new WiseApplicationPacket("received video frame", APPLICATION_STATISTICS_MESSAGE);
				pktTrace->setIdVideo(tParam.seqNum);
				pktTrace->setIdNode(tParam.nodeId);
				pktTrace->setIdFrame(1);
				toNetworkLayer(pktTrace, BROADCAST_NETWORK_ADDRESS);
				trace() << "whithout FEC included => node id " << tParam.nodeId << " seq number " << tParam.seqNum << " frame id " << tParam.idFrame << "\n";
				} else{
								trace() << "APP- Node " << self << " received a frame number " << rcvPackets->getInfo().idFrame << " from node " << rcvPackets->getInfo().nodeId << " for video id " << rcvPackets->getIdVideo() << " -- whith FEC";
				u_char* temp_packet = (u_char*)malloc(rcvPackets->getFecPktArraySize());
				for(int i = 0; i < rcvPackets->getFecPktArraySize(); i++)
					temp_packet[i] = rcvPackets->getFecPkt(i);

				trace() << "packet data -- fec_header_size: " << rcvPackets->getFecPkt(0) << " k: " << rcvPackets->getFecPkt(1) << " n: " << rcvPackets->getFecPkt(2) << " i: " << rcvPackets->getFecPkt(3) << " evalvid id: " << rcvPackets->getFecPkt(4);

				int indexEidPkts = -1;
				for(int i =0; i<discardEidPkts.size(); i++){
					if(discardEidPkts[i].nodeId == rcvPackets->getInfo().nodeId){
						indexEidPkts = i;
						break;
					}
				}
				if(indexEidPkts == -1){
					fec_parameters temp;
					temp.discard_eid_packets = 0;
					temp.nodeId = rcvPackets->getInfo().nodeId;
					discardEidPkts.push_back(temp);
					indexEidPkts = discardEidPkts.size() - 1;
				}

				int indexPool = -1;
				for(int i =0; i<pktPool.size(); i++){
					if(pktPool[i].nodeId == rcvPackets->getInfo().nodeId){
						indexPool = i;
						break;
					}
				}

				if (discardEidPkts[indexEidPkts].discard_eid_packets == 0 && indexPool == -1){
					trace() << "    significa que nao tenho nenhum bloco de fec na memoria para esse src, comecar um";
					FillPacketPool(rcvPackets->getIdFrame(), rcvPackets->getByteLength()-temp_packet[0], temp_packet, rcvPackets->getInfo().nodeId);
					int indexPool = pktPool.size() - 1;
					trace() << "    -- criando novo bloco para receber pacotes eid: " << pktPool[indexPool].eid << " k: " << pktPool[indexPool].k << " n: " << pktPool[indexPool].n << " nodeId: " << pktPool[indexPool].nodeId << " pktPool size " << pktPool.size();
				trace() << "end";
				} else if (discardEidPkts[indexEidPkts].discard_eid_packets == 0 && indexPool > -1){
					// se for ==0 eh sinal que ainda nao antingiu o numero suficiente de pacotes
					// porem, preciso testar se ainda estou recebendo os pacotes com o mesmo EvalvidID
					// caso discard_eid_packets for > 0 ela retem o EID do ultimo bloco de fec recebido
					// possibilitando descartar caso sejam pacotes a mais ou comecar a montar o novo
					// bloco caso seja diferente
					int differentEID = -1;
					if (pktPool[indexPool].eid != temp_packet[4])
						differentEID = indexPool;
					if(differentEID == -1){
						trace() << "    O bloco de fec que esta na memoria tenha o mesmo EID (" << pktPool[indexPool].eid << ") que o novo pacote";
						memmove(pktPool[indexPool].payload[pktPool[indexPool].count], temp_packet+pktPool[indexPool].fhs, pktPool[indexPool].fds);
						pktPool[indexPool].idx[pktPool[indexPool].count] = (int) temp_packet[3];
						//Ajustar os uid dos pacotes, pq senao posso ficar com id's diferentes no sd_file e rd_file
						// TODO: melhorar a explicacao da necessidade de ajuste do uid
						int adjusted_uid = (rcvPackets->getIdFrame()-(int)temp_packet[3])+pktPool[indexPool].count;
						pktPool[indexPool].packet_uid.push_back(adjusted_uid);
						pktPool[indexPool].count++;
						trace() << "    packet pool data -- " << "fhs: "	<< pktPool[indexPool].fhs << " k: " << pktPool[indexPool].k << " n: " << pktPool[indexPool].n << " eid: " << pktPool[indexPool].eid << " count: " << pktPool[indexPool].count << " uid: " << adjusted_uid;
						trace() << "end";
					} else if(differentEID != -1){
						trace() << "    ** Different Evalvid ID before packet pool clean, received eid: " << (int) temp_packet[4] << " packet pool eid: " << pktPool[indexPool].eid << " k: " << pktPool[indexPool].k << " n: " << pktPool[indexPool].n << " count: " << pktPool[indexPool].count << " nodeId: " << pktPool[indexPool].nodeId;
						rebuildFECBlock(tParam.nodeId, tParam.seqNum, differentEID);
						discardEidPkts[indexEidPkts].discard_eid_packets = 0;
						trace() << "    discard_eid_packets " << discardEidPkts[indexEidPkts].discard_eid_packets;
						FillPacketPool(rcvPackets->getIdFrame(), rcvPackets->getByteLength()-temp_packet[0], temp_packet, rcvPackets->getInfo().nodeId);
						trace() << "end";
					}
				} else if (discardEidPkts[indexEidPkts].discard_eid_packets != temp_packet[4]){
					trace() << "    recebi um numero suficiente de pacotes com FEC, descartar os mais com o mesmo EvalvidID";
					discardEidPkts[indexEidPkts].discard_eid_packets = 0;
					trace() << "    discard_eid_packets " << discardEidPkts[indexEidPkts].discard_eid_packets;
					FillPacketPool(rcvPackets->getIdFrame(), rcvPackets->getByteLength()-temp_packet[0], temp_packet, rcvPackets->getInfo().nodeId);
					trace() << "end";
				} else{
					trace() << "    drop " << pktPool.size();
				}

				int included = -1;
				for(int i =0; i<pktPool.size(); i++){
					if(pktPool[i].nodeId == rcvPackets->getInfo().nodeId){
						included = i;
						break;
					}
				}
				if (enoughPacketsReceived(included, indexEidPkts)){
					trace() << "    ++ criando novo bloco para receber pacotes (2) eid: " << pktPool[included].eid << " k: " << pktPool[included].k << " n: " << pktPool[included].n;
					discardEidPkts[indexEidPkts].discard_eid_packets = pktPool[included].eid;
					rebuildFECBlock(tParam.nodeId, tParam.seqNum, included);
				}
				free(temp_packet);
			}
			break;
		}
		case SCALAR_PACKET:{
			WiseApplicationPacket *rcvPackets = check_and_cast<WiseApplicationPacket*>(msg);
			//trace() << "APP- Node " << self << " received a vibration report from node " << src << " with value " << rcvPackets->getSensedValue();
			break;
		}
	}
}

/*
 * Create the output file and save it in a list
*/
void WingoApp::writeRdTrace(int id,int seqNum){
	char fileTrace[50];
	sprintf(fileTrace,"rd_sn_%i_nodeId_%i", seqNum,id);
	video temp;
	temp.ch = id;
	temp.sn = seqNum;
	temp.pFile = fopen(fileTrace, "a+");
	set.push_back(temp);
}

void WingoApp::handleSensorReading(WiseSensorMessage* msg) {
	int msgKind = msg->getKind();
	switch (msgKind) {
		case MULTIMEDIA:{
			WiseApplicationPacket *pktTrace = new WiseApplicationPacket("SENDING MULTIMEDIA DATA", MULTIMEDIA_PACKET);
			pktTrace->setByteLength(msg->getByteLength());
			pktTrace->setInfo(msg->getInfo());
			pktTrace->setIdFrame(msg->getIdFrame());
			pktTrace->setFrame(msg->getFrame());
			pktTrace->setFecPktArraySize(msg->getFecPktArraySize());
			for (int i = 0; i<msg->getFecPktArraySize(); i++)
				pktTrace->setFecPkt(i, msg->getFecPkt(i));
			pktTrace->setIdVideo(msg->getInfo().seqNum);
			pktTrace->setX(location.x);
			pktTrace->setY(location.y);
			toNetworkLayer(pktTrace, BROADCAST_NETWORK_ADDRESS);
			//trace() << "Video id " << pktTrace->getIdVideo() << " frame id " << pktTrace->getFrame() << ", " << pktTrace->getInfo().frameType << "-frame " << pktTrace->getByteLength() << " bytes";
		break;
		}
	}
}

void WingoApp::handleDirectApplicationMessage(WiseApplicationPacket* pkt) {
	 //trace() << "direct app msg";
}
