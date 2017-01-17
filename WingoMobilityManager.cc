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
//*  	Module: Bonnmotion: a mobility scenario generation and analysis tool	*
//*										*
//*  	Version: 1.0								*
//*  	Authors: Wellington Viana Lobato Junior				*
//*		 				*
//*										*
//******************************************************************************/

#include "WingoMobilityManager.h"

Define_Module(WingoMobilityManager);

void WingoMobilityManager::initialize(){
	VirtualMobilityManager::initialize();
	mobilityFile = par("mobilityFile");
	isMobile = par("isMobile");
	node = getParentModule();
	index = node->getIndex();
	network = node->getParentModule();
	wchannel = network->getSubmodule("wirelessChannel");
	updateInterval = par("updateInterval");
	energyToFly = par("energyToFly");
	traceMobility = new cMessage("process trace", MOVE);
	targetPosition = new cMessage("Periodic location update message", TARGET_POSITION);

	atraso = 0;
	atrasoTotal = 0;
	last_speed = -1;
	totalCO2Emission=0;
	totalFuel=0;
    if (!network)
		opp_error("Unable to obtain SN pointer for deployment parameter");
	if (!wchannel)
		opp_error("Unable to obtain wchannel pointer");
    //Redundante?
	if (isMobile) {
	    coligar = 0;
        getMovements(index, mobilityFile, nodePosition); /*--- get mobility ---*/
        scheduleAt(updateIntervalTargetPoint, new MobilityManagerMessage("Periodic location update message", TARGET_POSITION));/*--- schedule new positions ---*/ //Primeiro TARGET_POSITION

	} else{
		setLocation(node->par("xCoor"), node->par("yCoor"), 0.0);
		nodeLocation.x = node->par("xCoor");
		nodeLocation.y = node->par("yCoor");
		notifyWirelessChannel();
	}

	if (isMobile){
	    	//trace() << "initial location(x:y:z) is " << nodeLocation.x << ":" << nodeLocation.y << ":" << nodeLocation.z << "\n";
	}
	else{
	    	//trace() << "initial location(x:y:z) is " << nodeLocation.x << ":" << nodeLocation.y << ":" << nodeLocation.z << " but no mobile. \n";
	}

	string s = ev.getConfig()->getConfigValue("sim-time-limit");
	s = s.erase(s.find('s'));
	double sim_time_limit = atof(s.c_str()); //Tempo final de simulação
	scheduleAt(sim_time_limit - 0.02, new MobilityManagerMessage("WINGO_STATISTICS", WINGO_STATISTICS)); //Estatisticas Finais, executado no final da simulação
}

void WingoMobilityManager::finish() {
	cOwnedObject *Del=NULL;
	int OwnedSize = this->defaultListSize();
	for(int i=0; i<OwnedSize; i++){
		Del = this->defaultListGet(0);
		this->drop(Del);
		delete Del;
	}
}

void WingoMobilityManager::handleMessage(cMessage * msg){
	int msgKind = msg->getKind();
	trace() << "MV;" << index << ";" << nodeLocation.x << ";" << nodeLocation.y << ";" << simTime() << ";" << coligar ;
	switch (msgKind) {
		case MOBILE_MESSAGE:{
			fromApplicationLayer(msg);
			break;
		}
		case TARGET_POSITION:{
        //Só pra saber o tempo de inicio final e o tempo de fim final.
        last_it = --nodePosition.end();
        startTime_final = last_it->startTime;
        endTime_final = last_it->endTime;
        x_final = last_it->posX;
        y_final = last_it->posY;

            //Atribui os valores atuais de x, y e z para as variaveis current_*
            current_x = nodeLocation.x;
            current_y = nodeLocation.y;
            current_z = nodeLocation.z;
            //Atribui os valores de destino para x, y e z para as variaveis dest_*, it é o interador
            dest_x = it->posX;
            dest_y = it->posY;
            dest_z = it->posZ;
            //Calcula a distancia a ser percorria
            distance = sqrt(pow(current_x - dest_x, 2) + pow(current_y - dest_y, 2) + pow(current_z - dest_z, 2));
            //Tempo inicial
            startTime = it->startTime; //Atribui o valor de tempo inicial do xml
            //Tempo final
            endTime = it->endTime; //Atribui o valor de final inicial do xml
            //Calcula a diferença entre tempo final e inicial(final-inicial)
            difTime = endTime - startTime;
            //Calcula a velocidade
            speed = distance / difTime;
            //Calcula a aceleração
            acceleration = (speed-last_speed)/difTime; //Formula da aceleração (Variação da velocidade)/(Variação do tempo)m/s^2
            last_speed = speed; //Velocidade antiga recebe velocidade atual
            co2emission = calculateCO2emission(speed, acceleration); //Chama a função que calcula a emissão de CO2
            fuel = calculateFuelRate(speed, acceleration); //Chama a função que calcula o consumo de gasolina
            totalCO2Emission = totalCO2Emission+co2emission; //Guarda o valor total da emissão de CO2
            totalFuel = totalFuel+fuel; //Guarda o valor total do consumo de gasolina

            if(startTime >= 0 && startTime <= 800){

                trace() << "--------------------TARGET_POSITION---------------------------";
                trace() << "current location(x:y:z) is " << current_x << ":" << current_y << ":" << current_z;
                trace() << "destination location(x:y:z) is " << dest_x << ":" << dest_y << ":" << dest_z;
                trace() << "distance to move " << distance;
                trace() << "startTime " << startTime;
                trace() << "endTime " << endTime;
                trace() << "difTime " << difTime;
                trace() << "updateInterval " << updateInterval;
                trace() << "speed " << speed << " m/s";
                trace() << "acceleration " << acceleration << " m/s^2";
                trace() << "co2emission " << co2emission << " g/s"; //grams per seconds
                trace() << "totalCO2Emission " << totalCO2Emission << " g/s";
                trace() << "fuel " << fuel << " g/s";
                trace() << "totalFuel " << totalFuel << " g/s" << "\n";
            }


            if (speed > 0 && distance > 0) {
                double tmp = (distance / speed) / updateInterval;
                incr_x = (dest_x - current_x) / tmp;
                incr_y = (dest_y - current_y) / tmp;
                incr_z = (dest_z - current_z) / tmp;
                cancelEvent(traceMobility);
                scheduleAt(simTime() + updateInterval, traceMobility);
                if(startTime >= 0 && startTime <= 800){
                    trace() << "--------------------INCREMENTO---------------------------";
                    trace() << "tmp: " << tmp;
                    trace() << "incr_x: " << incr_x;
                    trace() << "incr_y: " << incr_y;
                    trace() << "incr_z: " << incr_z;
                }
            }

            MobilityManagerMessage *mobileMSG = new MobilityManagerMessage("routing msg", TOPOLOGY_CONTROL);
            mobileMSG->setXCoorDestination(dest_x);
            mobileMSG->setYCoorDestination(dest_y);
            mobileMSG->setZCoorDestination(dest_z);
            toApplicationLayer(mobileMSG);

            ++it;
            // condition is used in case of finish struct or it check if it haven't mobility
            if( it != nodePosition.end()) {
                updateIntervalTargetPoint = it->startTime;
                scheduleAt(updateIntervalTargetPoint, targetPosition);
            }
			break;
		}
		case MOVE:{
            temp_x = nodeLocation.x;
            temp_y = nodeLocation.y;
            temp_z = nodeLocation.z;
            nodeLocation.x += incr_x;
            nodeLocation.y += incr_y;
            nodeLocation.z += incr_z;
            double dist = sqrt(pow(nodeLocation.x - temp_x, 2) + pow(nodeLocation.y - temp_y, 2) + pow(nodeLocation.z - temp_z, 2));
            double energy = (dist * energyToFly) + uniform(0, 1) * ((dist * energyToFly) / 100);
            powerDrawn(energy);
            if(simTime() >= 0.0 && simTime() <= 800.0){
                trace() << "--------------------MOVE---------------------------";
                trace() << "current location(x:y:z) to " << temp_x << ":" << temp_y << ":" << temp_z;
                trace() << "changed location(x:y:z) to " << nodeLocation.x << ":" << nodeLocation.y << ":" << nodeLocation.z;
                trace() << "destination location(x:y:z) is " << dest_x << ":" << dest_y << ":" << dest_z;
                trace() << "updateInterval " << updateInterval;
                trace() << "Energy consumed to move (" << dist << " m)" << energy << "\n";
                trace() << "speed " << speed << " m/s" << "\n";
            }
            if(simTime() + updateInterval  < endTime){
                cancelEvent(traceMobility);
                scheduleAt(simTime() + updateInterval, traceMobility);
            }else{
                nodeLocation.x = dest_x;
                nodeLocation.y = dest_y;
                nodeLocation.z = dest_z;
                if(simTime() >= 0.0 && simTime() <= 800.0){
                    //trace() << "changed location(x:y:z) to " << nodeLocation.x << ":" << nodeLocation.y << ":" << nodeLocation.z;
                    //trace() << "destination location(x:y:z) is " << dest_x << ":" << dest_y << ":" << dest_z << "\n";
                }
            }
            notifyWirelessChannel();
			break;
		}
		case WINGO_STATISTICS:{
            difTime = endTime - startTime;
            speed = distance / difTime;
            acceleration = (speed-last_speed)/difTime; //Formula da aceleração (Variação da velocidade)/(Variação do tempo)m/s^2
            last_speed = speed; //Velocidade antiga recebe velocidade atual
            co2emission = calculateCO2emission(speed, acceleration); //Chama a função que calcula a emissão de CO2
            fuel = calculateFuelRate(speed, acceleration); //Chama a função que calcula o consumo de gasolina
            totalCO2Emission = totalCO2Emission+co2emission; //Guarda o valor total da emissão de CO2
            totalFuel = totalFuel+fuel; //Guarda o valor total do consumo de gasolina
            output() << "--------------------FINAL RESULT---------------------------";
            //Id;CO2;Gasolina;Gasolina por hora;Atraso
            output() << ";" << index << ";" << totalCO2Emission << ";" << totalFuel << ";" << totalFuel*227.1 << ";" << atraso_viajem << ";" << coligar;
            //output() << "totalFuel " << totalFuel << " g/s" << "\n";
            //output() << "totalFuel(Liter per hour) " << totalFuel*227.1 << " L/h" << "\n";
            break;
		}
		default:{
			trace() << "WARNING: Unexpected message " << msgKind;
		}
	}
}

//Leitura do XML
void WingoMobilityManager::getMovements( const int index, const char *mobilityFile, list<BonnMotionMovements> &nodePosition){
	/*--- Open File ---*/
	char _line[301]; 							// read lines of file
	ifstream fin(mobilityFile, ios::in);		// read file xml

	if(!fin){
		opp_error( "Problems in file. It didn't read. See the name or local of file.");
	}

	fin.getline(_line,300);						// head file not necessary
	fin.getline(_line,300);						// head file not necessary
	/*--- Startup words ---*/
	char nodeSetings[16] = "<node_settings>";
	char mobility[11] = "<mobility>";
	char nodeIdent[25];
	stringstream ss;
	string nodeId = "<node_id>";
	string nodeId2 = "</node_id>";

	ss << nodeId << index << nodeId2;
	ss >> nodeIdent; 							// result: <node_id>##</node_id>

	string positions;
	while (!fin.eof()){
		fin.getline(_line,300);
		fin >> skipws >> _line;

		if( strcmp(_line, nodeSetings) == 0 ) {
			fin.getline(_line,300);
			fin >> skipws >> _line;
			fin.getline(_line,300);
			fin >> skipws >> _line;
		}
		if( strcmp(_line, nodeIdent) == 0 ){
			fin.getline(_line,300);
			fin >> skipws >> _line;
			fin.getline(_line,300);
			fin >> skipws >> _line;
			fin.getline(_line,300);
			fin >> skipws >> _line;

			/*--- get positionX ---*/
			positions = _line;
			positions.erase(0,6);
			positions.erase(15,positions.size()-15);

			auxMovements.posX = atof(positions.c_str()); //atof converter string para float
			//trace() << "-" << _line << "--->" << fin.gcount() << "@" << positions << ")(" << auxMovements.posX << "!!" << ss.str() << endl;

			/*--- get positionY ---*/
			fin.getline(_line,300);
			fin >> skipws >> _line;
			positions = _line;
			positions.erase(0,6);
			positions.erase(15,positions.size()-15);

			auxMovements.posY = atof(positions.c_str());
			//trace() << "-" << _line << "--->" << fin.gcount() << "@" << positions << ")(" << auxMovements.posY << "!!" << ss.str() << endl;

			/*--- get time inicial---*/
			auxMovements.startTime = 0.0;
			auxMovements.endTime = 0.0;

			//trace() << "settings " << auxMovements.startTime << " | " << auxMovements.posX << " | " << auxMovements.posY;
			nodePosition.push_back(auxMovements);
		}
		if( strcmp(_line, mobility) == 0 ){
			while(!fin.eof()) {
				fin.getline(_line,300);
				fin >> skipws >> _line;
				if( strcmp(_line, nodeIdent) == 0 ){
					fin.getline(_line,300);
					fin >> skipws >> _line;

					/*--- get startTime ---*/
					positions = _line;
					positions.erase(0,12); 		// erase start_time

					int i=0;
					while(positions[i]!='\0'){
						if(positions[i] == '<')
							break;
						i++;
					}
					positions.erase(i,(positions.size()-i));
					auxMovements.startTime = atof(positions.c_str());

					/*--- get endTime ---*/
					fin.getline(_line,300);
					fin >> skipws >> _line;

					positions = _line;
					positions.erase(0,10); 		// erase endTime

					i=0;
					while(positions[i]!='\0'){
						if(positions[i] == '<')
							break;
						i++;
					}

					positions.erase(i,(positions.size()-i));
					auxMovements.endTime = atof(positions.c_str());

					/*--- get posX ---*/
					fin.getline(_line,300);
					fin >> skipws >> _line;
					fin.getline(_line,300);
					fin >> skipws >> _line;

					positions = _line;
					positions.erase(0,6);
					positions.erase(15,positions.size()-15);
					auxMovements.posX = atof(positions.c_str());

					/*--- get posY ---*/
					fin.getline(_line,300);
					fin >> skipws >> _line;

					positions = _line;
					positions.erase(0,6);
					positions.erase(15,positions.size()-15);
					auxMovements.posY = atof(positions.c_str());
					nodePosition.push_back(auxMovements);
				}
			}
		}
	}
	//for(it = nodePosition.begin(); it != nodePosition.end(); ++it)
	//	trace() << "settings: " << it->startTime << " | " << it->endTime << " | "<< it->posX << " | " << it->posY;
	//trace() << "Nó: " << index;
	/*for(list<BonnMotionMovements>::iterator ite = nodePosition.begin(); ite != nodePosition.end(); ite++){

	    trace() << "Tempo inicial: " << ite->startTime;
	    trace() << "Tempo final: " << ite->endTime;
	    trace() << "x: " << ite->posX;
	    trace() << "y: " << ite->posY;
	    trace() << "z: " << ite->posZ;
	}*/

	it = nodePosition.begin();
	setLocation(it->posX, it->posY, 0.0);
	//trace() << "boonmotion: " << it->posX << " | " << it->posY << " " << it->startTime;
	++it;
	updateIntervalTargetPoint = it->startTime;
}

void WingoMobilityManager::fromApplicationLayer(cMessage * msg) {
	MobilityManagerMessage *rcvPackets = check_and_cast<MobilityManagerMessage*>(msg);
	trace() << "WVL-Node received a msg";
	switch (rcvPackets->getMobilePacketKind()) {
	        case COLIGATE:{
	            coligar = 1;

	            cancelEvent(targetPosition); //Cancelar evento de TargetPosition
                cancelEvent(traceMobility); //Cancelar evento de Move

                last_it = --nodePosition.end();
                trace()<< "Final X " << last_it->posX;
                trace()<< "Final Y " << last_it->posY;
                trace()<< "Final startTime " << last_it->startTime;
                trace()<< "Final endTime " << last_it->endTime;
                //double x_final = last_it->posX;
                //double y_final = last_it->posY;

                double x_final = rcvPackets->getXCoorDestination();
                double y_final = rcvPackets->getYCoorDestination();

                double endTime_final = last_it->endTime;

                double x_src = rcvPackets->getXSrc();
                double y_src = rcvPackets->getYSrc();

                double DistC = sqrt(pow((x_src-x_final),2)+pow((y_src-y_final),2));
                double distC = DistC - (167+100);
                double k = (167.0+100)/distC;

                double new_x = (x_src+k*x_final)/(1+k);
                double new_y = (y_src+k*y_final)/(1+k);

                double tempo_atual_simulacao = SIMTIME_DBL(simTime()) + 1.0;

                nodePosition.clear(); //Limpar valor do vetor


                auxMovements.startTime = tempo_atual_simulacao;
                auxMovements.endTime = tempo_atual_simulacao + 10;
                auxMovements.posX = new_x;
                auxMovements.posY = new_y;
                auxMovements.posZ = 0.0;

                nodePosition.push_back(auxMovements); //Coloca novos movimentos

                double tempo_final = sqrt(pow((x_final-new_x),2)+pow(y_final-new_y,2))/rcvPackets->getSpeed_final();
                atraso_viajem = (tempo_atual_simulacao + tempo_final) - endTime_final;

                auxMovements.startTime = tempo_atual_simulacao + 10;
                auxMovements.endTime = tempo_atual_simulacao + tempo_final;
                auxMovements.posX = x_final;
                auxMovements.posY = y_final;
                auxMovements.posZ = 0.0;

                nodePosition.push_back(auxMovements); //Coloca novos movimentos

                it = nodePosition.begin();
                updateIntervalTargetPoint = it->startTime;
                scheduleAt(updateIntervalTargetPoint, targetPosition);
                /*
                double tempo_atual_simulacao = SIMTIME_DBL(simTime()) + 1.0;
                double tempo_final = sqrt(pow((x_final-rcvPackets->getXCoorDestination()),2)+pow(y_final-rcvPackets->getYCoorDestination(),2))/rcvPackets->getSpeed_final();
                atraso_viajem = (tempo_atual_simulacao + tempo_final) - endTime_final;


                nodePosition.clear(); //Limpar valor do vetor


                auxMovements.startTime = tempo_atual_simulacao;
                auxMovements.endTime = tempo_atual_simulacao + 120;
                auxMovements.posX = 167+rcvPackets->getXCoorDestination();
                auxMovements.posY = 0.0;
                auxMovements.posZ = 0.0;

                nodePosition.push_back(auxMovements); //Coloca novos movimentos

                auxMovements.startTime = tempo_atual_simulacao + 120.01;
                auxMovements.endTime = tempo_atual_simulacao + 120.01 + tempo_final;
                auxMovements.posX = x_final;
                auxMovements.posY = 0.0;
                auxMovements.posZ = 0.0;

                nodePosition.push_back(auxMovements); //Coloca novos movimentos

                for(list<BonnMotionMovements>::iterator ite = nodePosition.begin(); ite != nodePosition.end(); ite++){

                        trace() << "Tempo inicial: " << ite->startTime;
                        trace() << "Tempo final: " << ite->endTime;
                        trace() << "x: " << ite->posX;
                        trace() << "y: " << ite->posY;
                        trace() << "z: " << ite->posZ;
               }

                it = nodePosition.begin();
                updateIntervalTargetPoint = it->startTime;
                scheduleAt(updateIntervalTargetPoint, targetPosition); */

	            break;
	        }
	        case NODE_DIE:{
	            trace() << "NODEDIE";
	            for(int i=0;i<40001;i++){
	                powerDrawn(500000.0);
	            }
                break;
	        }
	        case NOT_COLIGATE:{
	            break;
	        }
	}

    /*if(rcvPackets->getEvent() == 2){
        scheduleAt(simTime(), new MobilityManagerMessage("Coligação dos nós", COLLIGATE)); //Já faz a coligação dos nós
    }else if(rcvPackets->getEvent() == 3){
        scheduleAt(simTime(), new MobilityManagerMessage("Coligação dos nós", NOTCOLLIGATE)); //Desfaz a coligação dos nós
    }else if(rcvPackets->getEvent() == 4){
        trace() << "WVL-Teoria dos jogos " << rcvPackets->getEvent();
    }*/
}

double WingoMobilityManager::calculateCO2emission(double speed, double acceleration) const{
    // Calculate CO2 emission parameters according to:
    //Cappiello, A. and Chabini, I. and Nam, E.K. and Lue, A. and Abou Zeid, M., "A statistical model of vehicle emissions and fuel consumption," IEEE 5th International Conference on Intelligent Transportation Systems (IEEE ITSC), pp. 801-809, 2002
    double A = 1000 * 0.1326; // W/m/s
    double B = 1000 * 2.7384e-03; // W/(m/s)^2
    double C = 1000 * 1.0843e-03; // W/(m/s)^3
    double M = 1325.0; // kg

    // power in W
    double P_tract = A*speed + B*speed*speed + C*speed*speed*speed + M*acceleration*speed; // for sloped roads: +M*g*sin_theta*v

    //Podemos modelar novos carros se for necessario
    /*
    // "Category 7 vehicle" (e.g. a '92 Suzuki Swift)
    double alpha = 1.01;
    double beta = 0.0162;
    double delta = 1.90e-06;
    double zeta = 0.252;
    double alpha1 = 0.985;
    */

    // "Category 9 vehicle" (e.g. a '94 Dodge Spirit)
    double alpha = 1.11;
    double beta = 0.0134;
    double delta = 1.98e-06;
    double zeta = 0.241;
    double alpha1 = 0.973;

    if (P_tract <= 0) return alpha1;
    return alpha + beta*speed*3.6 + delta*speed*speed*speed*(3.6*3.6*3.6) + zeta*acceleration*speed;
}

double WingoMobilityManager::calculateFuelRate(double speed, double acceleration) const{
    // Calculate CO2 emission parameters according to:
    //Cappiello, A. and Chabini, I. and Nam, E.K. and Lue, A. and Abou Zeid, M., "A statistical model of vehicle emissions and fuel consumption," IEEE 5th International Conference on Intelligent Transportation Systems (IEEE ITSC), pp. 801-809, 2002
    double A = 1000 * 0.1326; // W/m/s
    double B = 1000 * 2.7384e-03; // W/(m/s)^2
    double C = 1000 * 1.0843e-03; // W/(m/s)^3
    double M = 1325.0; // kg

    // power in W
    double P_tract = A*speed + B*speed*speed + C*speed*speed*speed + M*acceleration*speed; // for sloped roads: +M*g*sin_theta*v

    //Podemos modelar novos carros se for necessario
    /*
    // "Category 7 vehicle" (e.g. a '92 Suzuki Swift)
    double alpha = 1.01;
    double beta = 0.0162;
    double delta = 1.90e-06;
    double zeta = 0.252;
    double alpha1 = 0.985;
    */

    // "Category 9 vehicle" (e.g. a '94 Dodge Spirit)
    // Valores estao de acordo com a Tabela I do artigo "A statistical model of vehicle emissions and fuel consumption,"
    double alpha = 0.365;
    double beta = 0.00114;
    double delta = 9.65e-07;
    double zeta = 0.0943;
    double alpha1 = 0.299;

    if (P_tract <= 0) return alpha1;
    return alpha + beta*speed*3.6 + delta*speed*speed*speed*(3.6*3.6*3.6) + zeta*acceleration*speed;
}
