#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "libraries/queue.h"

// Affiche le menu principal de la simulation 
void displayMenu() {
    printf("\n***************************************************\n");
    printf("*********  MENU DE SIMULATION DE TRAFIC  **********\n");
    printf("***************************************************\n\n");
    printf("* 1. Lancer la simulation                   |=>|  *\n");
    printf("* 2. SHOW HISTORY                           |X|   *\n");
    printf("* 3. FERMER LE PROGRAMME                    |X|   *\n\n");
    printf("***************************************************\n");
    printf("Votre choix: ");
}

// Affiche l'en-tête de la simulation avec le temps ecoule
void printSimulationHeader(int simTime) {
    printf("\n==========  Simulation de trafic  ==========\n");
    printf("Temps: %d secondes\n", simTime);
}

// Convertit le type de vehicule en chaîne de caractères en français
char* vehicleTypeToString(VehiculeType type) {
    char* types[] = {" VOITURE ", " BUS ", " MOTO ", " URGENCE "};
    return types[type];
}

// Affiche l'etat des voies (aller et retour)
void printLaneStatus(lane* lanes[]) {
    printf("\n---\nEtat des voies:\n");
    char* directions[] = {"Nord |^", "Sud v|", "Est ->", "Ouest <-"};

    for (int i = 0; i < 4; i++) {
        printf("%s : ", directions[i]);
        Vehicule* current = lanes[i]->aller->first;
        while (current != NULL) {
            printf("[%s] ", vehicleTypeToString(current->type));
            current = current->next;
        }
        printf("\n%s (Retour) : ", directions[i]);
        current = lanes[i]->retour->first;
        while (current != NULL) {
            printf("[%s] ", vehicleTypeToString(current->type));
            current = current->next;
        }
        printf("\n");
    }
    printf("------------------------------------------------\n");
}

// Fonction de simulation principale
void runSimulation(TrafficHistoryStack* trafficHistory) {
    srand(time(NULL));
    printf("\n=========== Simulation demarree ===========\n");

    // Creation du fichier journal
    FILE* logFile = initializeLogFile();
    logWithTimestamp(logFile, "Debut de la simulation");

    // Creation des files de circulation
    lane north_lane, south_lane, east_lane, west_lane;
    Createlane(&north_lane, QUEUE_CAPACITY, 1, NORTH);
    Createlane(&south_lane, QUEUE_CAPACITY, 2, SOUTH);
    Createlane(&east_lane,  QUEUE_CAPACITY, 3, EAST);
    Createlane(&west_lane,  QUEUE_CAPACITY, 4, WEST);

    lane* lanes[] = {&north_lane, &south_lane, &east_lane, &west_lane};
    int numLanes = 4;

    // Initialisation de la file circulaire pour les feux de circulation
    LLCircular trafficLightQueue;
    initLLCircular(&trafficLightQueue);

    enqueuePhase(&trafficLightQueue, NORTH_SOUTH_GREEN, BASE_GREEN_DURATION, BASE_RED_DURATION);
    enqueuePhase(&trafficLightQueue, EAST_WEST_GREEN, BASE_GREEN_DURATION, BASE_RED_DURATION);

    // Premier etat des feux
    TrafficPhaseNode* currentPhaseNode = trafficLightQueue.front;
    time_t phaseStartTime = time(NULL);
    int simTime = 0;

    // Boucle de simulation
    while (simTime < SIMULATION_DURATION) {
        printSimulationHeader(simTime);

        // Affichage des feux
        printf("\nFeu Nord-Sud: %s\n", (currentPhaseNode->phase == NORTH_SOUTH_GREEN) ? "GREEN" : "RED");
        printf("Feu Est-Ouest: %s\n", (currentPhaseNode->phase == EAST_WEST_GREEN) ? "GREEN" : "RED");

        logWithTimestamp(logFile, (currentPhaseNode->phase == NORTH_SOUTH_GREEN) ? 
            "Feu NORD-SUD -> VERT" : "Feu EST-OUEST -> VERT");

        printLaneStatus(lanes);

        // Verification des embouteillages
        for (int i = 0; i < numLanes; i++) {
            if (detectTrafficJam(lanes[i]->aller)) {
                printf("\n Embouteillage detecte sur voie %d !\n", lanes[i]->aller->id);
                logWithTimestamp(logFile, "Embouteillage detecte !");
            }
        }

        // Generation aleatoire de vehicules
        if (rand() % 100 < VEHICLE_GEN_PROB) {
            int laneIndex = rand() % numLanes;
            generateRandomVehicle(lanes[laneIndex]->aller, logFile, simTime,trafficHistory);
        
            // Add vehicle to traffic history after it's generated and added to a lane
            time_t passTime = time(NULL);  // Vehicle passing time
            
        }

        // Ajustement des feux en fonction du trafic
        LightDurations adjustedDurations;
        if (currentPhaseNode->phase == NORTH_SOUTH_GREEN) {
            adjustedDurations = adjustLightDurationsForPair(north_lane.aller, south_lane.aller);
        } else {
            adjustedDurations = adjustLightDurationsForPair(east_lane.aller, west_lane.aller);
        }
        currentPhaseNode->greenDuration = adjustedDurations.greenDuration;
        currentPhaseNode->redDuration = adjustedDurations.redDuration;

        // Verification du changement de phase
        time_t currentTime = time(NULL);
        int elapsed = (int)difftime(currentTime, phaseStartTime);
        if (elapsed >= currentPhaseNode->greenDuration) {
            TrafficPhaseNode* nextPhase = dequeuePhase(&trafficLightQueue);
            if (nextPhase != NULL) {
                currentPhaseNode = nextPhase;
                logWithTimestamp(logFile, "Changement de phase");
            }
            phaseStartTime = currentTime;
        }

        // Traitement des vehicules
        for (int i = 0; i < numLanes; i++) {
            processQueue(lanes[i]->aller, logFile,trafficHistory, &simTime, lanes, numLanes);
        }
    
        for (int i = 0; i < numLanes; i++) {
            logQueueState(lanes[i]->aller, logFile, "Aller");
            logQueueState(lanes[i]->retour, logFile, "Retour");
        }

        sleep(1);
        simTime++;
    }

    printf("\n============ Simulation terminee ============\n");
    logWithTimestamp(logFile, "Fin de la simulation");
    fclose(logFile);
    getchar();
}

// La fonction main
int main() {
    int choice;
    TrafficHistoryStack trafficHistory;
    initTrafficHistory(&trafficHistory);
    do {
        displayMenu();
        scanf("%d", &choice);
        getchar();
        switch (choice) {
            case 1:
                runSimulation(&trafficHistory);
                break;
            case 2:
                printTrafficHistory(&trafficHistory);
                break;
            case 3:
                printf("\nFermeture du programme...\n");
                exit(0);
            default:
                printf("\nChoix invalide ! Appuyez sur Entree pour continuer...");
                getchar();
        }
    } while (1);
    clearTrafficHistory(&trafficHistory);
    return 0;
}
