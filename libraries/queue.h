#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// Définir le type de véhicule
typedef enum {
    CAR,        // Voiture
    BUS,        // Bus
    BIKE,       // Vélo
    Emergency   // Urgence
} VehiculeType;

typedef enum {
    RED,
    GREEN
} TrafficLightState;

// Structure représentant un véhicule
typedef struct Vehicule {
    int id;
    VehiculeType type;
    time_t arrivalTime;  // Changed from int to time_t for format correct and logic arrival times 
    struct Vehicule* next;
} Vehicule;

// Durées des feux
typedef struct {
    int greenDuration;
    int redDuration;
} LightDurations;

// Structure de la file d'attente des véhicules
typedef struct Queue {
    int id;
    Vehicule* first;
    Vehicule* last;
    int size;
    int Maxcapacity;
    TrafficLightState lightState;
    int baseGreenDuration; // par default (cas normal)
    int baseRedDuration;
} Queue;

// Fonction pour créer un véhicule
Vehicule *createVehicule(int id, VehiculeType type, int arrivalTime) {
    Vehicule* v = (Vehicule*)malloc(sizeof(Vehicule));
    if (!v) {
        printf("Erreur d'allocation mémoire\n");
        exit(EXIT_FAILURE);
    }
    v->id = id;
    v->type = type;
    v->arrivalTime = arrivalTime; // n.seconde apres debut de simulation 
    v->next = NULL;
    return v;
}

// Fonction pour créer une file d'attente
Queue* createQueue(int max, int id) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) {
        printf("Erreur d'allocation mémoire\n");
        exit(EXIT_FAILURE);
    }
    q->first = NULL;
    q->last = NULL;
    q->id = id;
    q->size = 0; // vide par default
    q->Maxcapacity = max;
    q->lightState = RED;
    q->baseGreenDuration = 3; // a ajuster 
    q->baseRedDuration = 6;
    return q;
}

int isFull(Queue* q) {
    return q->size >= q->Maxcapacity; 
}

int isEmpty(Queue* q) {
    return q->size == 0;
}

int detectTrafficJam(Queue* q) {
    return q->size >= q->Maxcapacity * 0.7; //a ajuster pour determiner les bouchons a un certain pourcentage
}

LightDurations adjustLightDurations(Queue* q) {
    LightDurations durations;
    if (detectTrafficJam(q)) {
        durations.greenDuration = q->baseGreenDuration + 2; // rajouter (attention ne pas avoir de nombre negatif )
        durations.redDuration = q->baseRedDuration - 2;
    } else {
        durations.greenDuration = q->baseGreenDuration;
        durations.redDuration = q->baseRedDuration;
    }
    return durations;
}

// Fonction de logging avec timestamp
void logWithTimestamp(FILE* logFile, const char* message) {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);     // real time-info
    fprintf(logFile, "[%s] %s\n", timestamp, message);
    fflush(logFile);
}

// Fonction pour afficher l'état de la file
void logQueueState(Queue* q, FILE* logFile, const char* phase) {
    fprintf(logFile, "\n=== Queue %d State (%s) ===\n", q->id, phase);
    fprintf(logFile, "Capacity: %d/%d | Light: %s\n",
           q->size, q->Maxcapacity,
           q->lightState == RED ? "RED" : "GREEN");

    fprintf(logFile, "Vehicles:");
    Vehicule* current = q->first;
    while (current) { // parcourir les elements de la file 
        fprintf(logFile, "\n- ID: %d | Type: %s | Arrived at: %ds",
               current->id,
               (current->type == CAR) ? "Car" :
               (current->type == BUS) ? "Bus" :
               (current->type == BIKE) ? "Bike" : "Emergency",
               current->arrivalTime);
        current = current->next;
    }
    if (q->size == 0) fprintf(logFile, "\n(empty)");
    fprintf(logFile, "\n=======================\n");
    fflush(logFile);
}


void enqueue(Queue* q, Vehicule* v, FILE* logFile) {
    // Gestion prioritaire des véhicules d'urgence (toujours les urgences en tete de la file )
    if (v->type == Emergency) {
        // Contournement des limites de capacité
        v->next = q->first;
        q->first = v;
        if (q->last == NULL) {
            q->last = v;
        }
        q->size++;

        char msg[100];
        snprintf(msg, 100, "Queue %d | VEHICULE URGENCE %d INSERE (Accès prioritaire)", 
                q->id, v->id);
        logWithTimestamp(logFile, msg);
        logQueueState(q, logFile, "Insertion Urgence");
        return;
    }

    if (isFull(q)) { // si file pleine , aucun vehicule ne peut rentrer sauf les urgences , ces derniers se rendent au debut de la files et on supprime le dernier
        char msg[100];
        snprintf(msg, 100, "Queue %d PLEINE!", 
                q->id);
        logWithTimestamp(logFile, msg);
        free(v);
        return;
    }

    // Ajout standard dans la file
    if (isEmpty(q)) {
        q->first = q->last = v;
    } else {
        q->last->next = v;
        q->last = v;
    }
    q->size++;

    char msg[100];
    snprintf(msg, 100, "Queue %d | Véhicule %d ajouté (%s)", 
            q->id, v->id,
            (v->type == CAR) ? "Voiture" :
            (v->type == BUS) ? "Bus" : "Vélo");
    logWithTimestamp(logFile, msg);
    logQueueState(q, logFile, "Après ajout");
}

// Fonction pour retirer un véhicule de la file
Vehicule* dequeue(Queue* q, FILE* logFile) {
    if (isEmpty(q)) {
        char msg[100];
        snprintf(msg, 100, "Queue %d | LA route est déjà vide , rien à faire passer ou retirer !", q->id);
        logWithTimestamp(logFile, msg);
        return NULL;
    }

    Vehicule* v = q->first;
    q->first = q->first->next;
    if (q->first == NULL) {
        q->last = NULL;
    }
    q->size--;

    char msg[100];
    snprintf(msg, 100, "Queue %d |  Vehicle passé : %d", q->id, v->id);
    logWithTimestamp(logFile, msg);
    return v;
}



// Fonction pour initialiser le fichier de log
FILE* initializeLogFile() {
    FILE* logFile = fopen("traffic_log.txt", "w");
    if (!logFile) {
        printf("Erreur : Impossible de créer le fichier de log.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(logFile, "Log de la simulation du trafic\n============================\n\n");
    return logFile;
}
