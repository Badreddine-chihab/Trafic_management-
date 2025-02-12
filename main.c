#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "libraries/queue.h"

// Affiche le menu principal de la simulation
void displayMenu() {
    printf("\n**************************************************\n");
    printf("*  MENU DE SIMULATION DE TRAFIC  *\n");
    printf("**************************************************\n");

    printf("* 1. Lancer la simulation |=>|  *\n");
    printf("* 2. Quitter              |X|   *\n");
    printf("**************************************************\n");
    printf("Votre choix: ");
}

// Affiche l'en-tête de la simulation avec le temps écoulé
void printSimulationHeader(int simTime) {
    printf("\n==========Simulation de trafic  ==========\n");
    printf("Temps: %d secondes\n", simTime);
}

// Convertit le type de véhicule en chaîne de caractères en français
char* vehicleTypeToString(VehiculeType type) {
    char* types[] = {"Voiture ", "Bus ", "Moto", "Urgence "};
    return types[type];
}

// Affiche l'état des voies (aller et retour)
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
    printf("---\n");
}

// Exécute la simulation de trafic
void runSimulation() {
    srand(time(NULL));
    printf("\n=========== Simulation démarrée ===========\n");

    // Création du fichier journal
    FILE* logFile = initializeLogFile();
    logWithTimestamp(logFile, "Début de la simulation");

    // Création des files de circulation
    lane north_lane, south_lane, east_lane, west_lane;
    Createlane(&north_lane, QUEUE_CAPACITY, 1, NORTH);
    Createlane(&south_lane, QUEUE_CAPACITY, 2, SOUTH);
    Createlane(&east_lane, QUEUE_CAPACITY, 3, EAST);
    Createlane(&west_lane, QUEUE_CAPACITY, 4, WEST);

    lane* lanes[] = {&north_lane, &south_lane, &east_lane, &west_lane};
    int numLanes = 4;

    // Initialisation de la file circulaire pour les feux de circulation
    CircularQueue trafficLightQueue;
    initCircularQueue(&trafficLightQueue);

    enqueuePhase(&trafficLightQueue, NORTH_SOUTH_GREEN, BASE_GREEN_DURATION, BASE_RED_DURATION);
    enqueuePhase(&trafficLightQueue, EAST_WEST_GREEN, BASE_GREEN_DURATION, BASE_RED_DURATION);

    // Premier état des feux
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

        // Vérification des embouteillages
        for (int i = 0; i < numLanes; i++) {
            if (detectTrafficJam(lanes[i]->aller)) {
                printf("\n Embouteillage détecté sur voie %d !\n", lanes[i]->aller->id);
                logWithTimestamp(logFile, "Embouteillage détecté !");
            }
        }

        // Génération aléatoire de véhicules
        if (rand() % 100 < VEHICLE_GEN_PROB) {
            int laneIndex = rand() % numLanes;
            generateRandomVehicle(lanes[laneIndex]->aller, logFile, simTime);
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

        // Vérification du changement de phase
        time_t currentTime = time(NULL);
        int elapsed = currentTime - phaseStartTime;
        if (elapsed >= currentPhaseNode->greenDuration) {
            TrafficPhaseNode* nextPhase = dequeuePhase(&trafficLightQueue);
            if (nextPhase != NULL) {
                currentPhaseNode = nextPhase;
                logWithTimestamp(logFile, "Changement de phase");
            }
            phaseStartTime = currentTime;
        }

        // Traitement des véhicules
        for (int i = 0; i < numLanes; i++) {
            processQueue(lanes[i]->aller, logFile, &simTime, lanes, numLanes);
        }

        for (int i = 0; i < numLanes; i++) {
            logQueueState(lanes[i]->aller, logFile, "Aller");
            logQueueState(lanes[i]->retour, logFile, "Retour");
        }

        sleep(1);
        simTime++;
    }

    printf("\n============ Simulation terminée ============\n");
    logWithTimestamp(logFile, "Fin de la simulation");
    fclose(logFile);
    getchar();
}

// Main function
int main() {
    int choice;
    do {
        displayMenu();
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1:
                runSimulation();
                break;
            case 2:
                printf("\nFermeture du programme...\n");
                exit(0);
            default:
                printf("\nChoix invalide ! Appuyez sur Entrée pour continuer...");
                getchar();
        }
    } while (1);

    return 0;
}
