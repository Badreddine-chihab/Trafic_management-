#include <stdio.h>
#include <stdlib.h>
#include "libraries/queue.h"

int main() {
    // Initialisation du fichier de log
    FILE* logFile = initializeLogFile();
    if (!logFile) {
        printf("Erreur : Impossible d'ouvrir le fichier de log.\n");
        return EXIT_FAILURE;
    }

    // Création d'une file d'attente avec une capacité de 5 véhicules
    Queue* queue = createQueue(5, 1);
    if (!queue) {
        printf("Erreur : Impossible de créer la file d'attente.\n");
        fclose(logFile);
        return EXIT_FAILURE;
    }

    // Ajout initial de véhicules à la file
    enqueue(queue, createVehicule(2, BUS, 10), logFile);
    enqueue(queue, createVehicule(3, Emergency, 5), logFile);
    enqueue(queue, createVehicule(4, BIKE, 2), logFile);

    

    // Ajout de nouveaux véhicules
    enqueue(queue, createVehicule(5, CAR, 15), logFile);
    enqueue(queue, createVehicule(6, BUS, 12), logFile);
    enqueue(queue, createVehicule(7, Emergency, 8), logFile);
    enqueue(queue, createVehicule(8, BIKE, 4), logFile);
    
    // Défilement d'un autre véhicule
    dequeue(queue, logFile);
    
    // Ajout d'un dernier véhicule
    enqueue(queue, createVehicule(1, CAR, 0), logFile);
    
    // Simulation de plusieurs cycles de feux
    for (int i = 0; i < 3; i++) {
        printf("\n=== Début du cycle %d ===\n", i + 1);
        simulateTrafficLightCycle(queue, logFile);
    }
    
    // Fermeture du fichier log et libération de la mémoire
    fclose(logFile);
    free(queue);
    return EXIT_SUCCESS;
}