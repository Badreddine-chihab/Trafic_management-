#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "libraries/queue.h"

#define SIMULATION_DURATION 30  // seconds DHSHDSJJS

void generateRandomVehicle(Queue* queue, FILE* logFile, time_t simulationStart) {
    static int vehicleId = 1;
    VehiculeType types[] = {CAR, BUS, BIKE, Emergency};
    int elapsed = (int)(time(NULL) - simulationStart);
    
    // Generate vehicle with arrival time relative to simulation start
    VehiculeType type = types[rand() % (rand() % 10 < 1 ? 4 : 3)];
    Vehicule* v = createVehicule(vehicleId++, type, simulationStart + elapsed);
    enqueue(queue, v, logFile);
}

// In simulateTrafficSystem(), modify the random generation call:

void simulateTrafficSystem(Queue* queue, FILE* logFile) {
    time_t startTime = time(NULL);
    time_t currentTime;
    int simulationActive = 1;
    printf("Simulating Traffic en cours pour %d seconds\n", SIMULATION_DURATION);
    printf("Voir le log.txt\n");
    while (simulationActive) {
        currentTime = time(NULL);
        int elapsed = (int)difftime(currentTime, startTime);

        // Check simulation duration
        if (elapsed >= SIMULATION_DURATION) {
            logWithTimestamp(logFile, "Simulation ended");
            simulationActive = 0;
            break;
        }

        // Random vehicle generation proba d apparition lors du feu rouge plus eleve
        if (queue->lightState == RED && (rand() % 100) < 40) {
        generateRandomVehicle(queue, logFile, startTime);
}

        // Traffic light management
        LightDurations durations = adjustLightDurations(queue);
        
        if (queue->lightState == GREEN) {
            // Process vehicles during green light
            if (!isEmpty(queue)) {
                Vehicule* v = dequeue(queue, logFile);
                if (v) {
                    fprintf(logFile, "Processed Vehicle %d (Wait: %lds)\n",
                          v->id, currentTime - v->arrivalTime);
                    free(v);
                }
                sleep(3);  // Process 1 vehicle every 3 seconds pour un réalisme (ne pas faire passer les vehicules en meme temps) (a ajuster aussi )
            }
            else {
                // Switch to red if queue is empty (ne pas garder feu vert si la file est vide )
                queue->lightState = RED;
                logWithTimestamp(logFile, "GREEN -> RED (Empty queue)");
            }
        }
        else { // RED light
            // Check duration and switch to green
            if (elapsed % (durations.redDuration + durations.greenDuration) >= durations.redDuration) {
                queue->lightState = GREEN;
                logWithTimestamp(logFile, "RED -> GREEN");
                fprintf(logFile, "Green duration: %ds (Jam: %s)\n",
                      durations.greenDuration,
                      detectTrafficJam(queue) ? "Yes" : "No");
            }
        }

        // Jam detection logging
        if (detectTrafficJam(queue)) {
            logWithTimestamp(logFile, "TRAFFIC JAM DETECTED!");
        }

        logQueueState(queue, logFile, "Current State");
        
        sleep(1);  // Update every second
    }
}



int main() {
    srand(time(NULL));  // Seed random number generator

    // Initialize log file
    FILE* logFile = initializeLogFile();
    if (!logFile) {
        printf("Error opening log file\n");
        return EXIT_FAILURE;
    }

    // Create queue with capacity 8 vehicles
    Queue* queue = createQueue(8, 1);
    if (!queue) {
        fclose(logFile);
        return EXIT_FAILURE;
    }

    // Run simulation
    simulateTrafficSystem(queue, logFile);

    // Cleanup
    fclose(logFile);
    free(queue);
    return EXIT_SUCCESS;
}
