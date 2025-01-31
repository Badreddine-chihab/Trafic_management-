#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Structure to represent a lane
typedef struct Lane {
    Queue* queue;           // The queue for this lane
    int laneID;            // Lane identifier
    int isActive;          // Flag to indicate if this lane has green light
    char direction;        // Direction of the lane (N,S,E,W)
}Lane;

// Structure to manage multiple lanes
typedef struct MultiLaneSystem {
    Lane** lanes;          // Array of lane pointers
    int laneCount;         // Number of lanes
    int currentActiveLane; // Index of the currently active lane
} MultiLaneSystem;

// Fonction pour créer un véhicule
Vehicule *createVehicule(int id, VehiculeType type, int arrivalTime) {
    Vehicule* v = (Vehicule*)malloc(sizeof(Vehicule));
    v->id = id;
    v->type = type;
    v->arrivalTime = arrivalTime;
    v->next = NULL;
    return v;
}

// Function to create a new lane
Lane* createLane(int id, int maxCapacity, char direction) {
    Lane* lane = (Lane*)malloc(sizeof(Lane));
    lane->queue = createQueue(maxCapacity);
    lane->laneID = id;
    lane->isActive = 0;
    lane->direction = direction;
    return lane;
}

// Function to create the multi-lane system
MultiLaneSystem* createMultiLaneSystem(int laneCount) {
    MultiLaneSystem* system = (MultiLaneSystem*)malloc(sizeof(MultiLaneSystem));
    system->lanes = (Lane**)malloc(sizeof(Lane*) * laneCount);
    system->laneCount = laneCount;
    system->currentActiveLane = 0;
    
    // Initialize each lane with different directions
    char directions[] = {'N', 'S', 'E', 'W'};
    for(int i = 0; i < laneCount; i++) {
        system->lanes[i] = createLane(i, 5, directions[i % 4]);
    }
    
    return system;
}

// Ajoutons des fonctions de logging spécifiques pour le système multi-voies
void logLaneState(FILE* logFile, Lane* lane) {
    fprintf(logFile, "État de la voie %d (Direction: %c):\n", lane->laneID, lane->direction);
    fprintf(logFile, "Feu de circulation: [%s]\n", 
            lane->isActive ? "VERT" : "ROUGE");
    
    fprintf(logFile, "Véhicules en attente: ");
    Vehicule* current = lane->queue->first;
    while (current != NULL) {
        switch(current->type) {
            case CAR: 
                fprintf(logFile, "[C]"); 
                break;
            case BUS: 
                fprintf(logFile, "[B]"); 
                break;
            case BIKE: 
                fprintf(logFile, "[V]"); 
                break;
            case Emergency: 
                fprintf(logFile, "[U]"); 
                break;
        }
        fprintf(logFile, " -> ");
        current = current->next;
    }
    fprintf(logFile, "NULL\n\n");
}
void logMultiLaneState(FILE* logFile, MultiLaneSystem* system) {
    fprintf(logFile, "\nÉtat du système multi-voies:\n");
    fprintf(logFile, "-------------------------\n");
    for(int i = 0; i < system->laneCount; i++) {
        logLaneState(logFile, system->lanes[i]);
    }
}

// Function to display all lanes
void displayMultiLanes(MultiLaneSystem* system) {
    printf("\nÉtat du système de circulation multi-voies:\n");
    printf("==========================================\n");
    
    for(int i = 0; i < system->laneCount; i++) {
        Lane* lane = system->lanes[i];
        printf("\nVoie %d (Direction: %c):\n", lane->laneID, lane->direction);
        printf("Feu de circulation: ");
        
        if(lane->isActive) {
            printf("[VERT]\n");
        } else {
            printf("[ROUGE]\n");
        }
        
        printf("File d'attente: ");
        Vehicule* current = lane->queue->first;
        while(current != NULL) {
            switch(current->type) {
                case CAR: printf("[C] -> "); break;
                case BUS: printf("[B] -> "); break;
                case BIKE: printf("[V] -> "); break;
                case Emergency: printf("[U] -> "); break;
            }
            current = current->next;
        }
        printf("NULL\n");
    }
}

// Function to switch active lane
void switchActiveLane(MultiLaneSystem* system) {
    // Deactivate current lane
    system->lanes[system->currentActiveLane]->isActive = 0;
    system->lanes[system->currentActiveLane]->queue->lightState = RED;
    
    // Move to next lane
    system->currentActiveLane = (system->currentActiveLane + 1) % system->laneCount;
    
    // Activate new lane
    system->lanes[system->currentActiveLane]->isActive = 1;
    system->lanes[system->currentActiveLane]->queue->lightState = GREEN;
    
    printf("\nChangement de voie active: Voie %d maintenant active\n", 
            system->currentActiveLane);
}

// Function to add vehicle to appropriate lane
void addVehicleToLane(MultiLaneSystem* system, Vehicule* v, char direction, FILE* logFile) {
    for(int i = 0; i < system->laneCount; i++) {
        if(system->lanes[i]->direction == direction) {
            enqueue(system->lanes[i]->queue, v, logFile);
            fprintf(logFile, "Véhicule ID: %d (Type: %d) ajouté à la voie %d (Direction: %c)\n", 
                    v->id, v->type, i, direction);
            fprintf(logFile, "Temps d'arrivée: %d\n", v->arrivalTime);
            return;
        }
    }
}

// Function to simulate multi-lane traffic
void simulateMultiLaneTraffic(MultiLaneSystem* system, int cycles, FILE* logFile) {
    printf("\nDébut de la simulation du trafic multi-voies\n");
    fprintf(logFile, "\nDébut de la simulation du trafic multi-voies\n");
    fprintf(logFile, "=========================================\n");
    
    for(int i = 0; i < cycles; i++) {
        // Affichage à l'écran
        printf("\nCycle %d:\n", i + 1);
        displayMultiLanes(system);  // Affichage en temps réel
        
        // Enregistrement dans le fichier log
        fprintf(logFile, "\nCycle %d:\n", i + 1);
        logMultiLaneState(logFile, system);
        
        // Traitement des véhicules dans la voie active
        Lane* activeLane = system->lanes[system->currentActiveLane];
        if(!isEmpty(activeLane->queue)) {
            int vehiclesToProcess = 2;
            while(vehiclesToProcess > 0 && !isEmpty(activeLane->queue)) {
                Vehicule* v = dequeue(activeLane->queue, NULL);
                if(v != NULL) {
                    // Affichage à l'écran
                    printf("Véhicule %d retiré de la voie %d\n", 
                            v->id, activeLane->laneID);
                    
                    // Enregistrement dans le log
                    fprintf(logFile, "Véhicule %d retiré de la voie %d (Direction: %c)\n", 
                            v->id, activeLane->laneID, activeLane->direction);
                    free(v);
                }
                vehiclesToProcess--;
            }
        }
        
        // Changement de voie active
        switchActiveLane(system);
        
        // Affichage et logging du changement
        printf("Changement de voie active: Voie %d maintenant active\n", 
                system->currentActiveLane);
        fprintf(logFile, "Changement de voie: Voie %d maintenant active\n", 
                system->currentActiveLane);
        
        Sleep(2000); // Pause pour permettre de voir les changements
    }
    
    printf("\nFin de la simulation\n");
    fprintf(logFile, "\nFin de la simulation\n");
}
