#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "../config.h"

/* --- Déclarations des énumérations --- */

// Directions possibles pour l'origine des véhicules
typedef enum { NORTH,
				SOUTH,
				 EAST,
				  WEST } Direction;

// Types de véhicules disponibles dans la simulation
typedef enum { CAR,
			 BUS,
			 BIKE,
			 Emergency } VehiculeType;

// Directions possibles pour le virage d'un véhicule
typedef enum { LEFT,
			 RIGHT,
			 STRAIGHT } TurnDirection;

// États des feux de circulation
typedef enum { RED,
			 GREEN } TrafficLightState;

// Phases des feux de circulation pour l'intersection
typedef enum {
    NORTH_SOUTH_GREEN, // Phase où les feux Nord-Sud sont verts
    EAST_WEST_GREEN    // Phase où les feux Est-Ouest sont verts
} TrafficLightPhase;

/* --- Déclaration des structures --- */

// Structure représentant un véhicule dans la simulation
typedef struct Vehicule {
    int id;                     // Identifiant du véhicule
    VehiculeType type;          // Type de véhicule (voiture, bus, moto, urgence)
    time_t arrivalTime;         // Heure d'arrivée dans la file
    Direction origin;           // Direction d'origine du véhicule
    TurnDirection turn;         // Direction dans laquelle le véhicule va tourner
    struct Vehicule* next;      // Pointeur vers le véhicule suivant dans la file
} Vehicule;

// Structure pour stocker les durées des feux (vert et rouge)
typedef struct {
    int greenDuration;
    int redDuration;
} LightDurations;

// Structure représentant une file d'attente pour les véhicules d'une direction donnée
typedef struct Queue {
    int id;                         // Identifiant de la file
    Vehicule* first;                // Premier véhicule dans la file
    Vehicule* last;                 // Dernier véhicule dans la file
    int size;                       // Nombre de véhicules dans la file
    int Maxcapacity;                // Capacité maximale de la file
    TrafficLightState lightState;   // État actuel du feu pour cette file (VERT/ROUGE)
    Direction direction;            // Direction associée à cette file
    int baseGreenDuration;          // Durée de base pour le feu vert
    int baseRedDuration;            // Durée de base pour le feu rouge
    int currentGreenDuration;       // Durée actuelle du feu vert (peut être ajustée)
    int currentRedDuration;         // Durée actuelle du feu rouge (peut être ajustée)
} Queue;

// Structure représentant une voie composée de deux files : "aller" et "retour"
typedef struct lane {
    Queue* aller;   // File pour le trafic allant vers l'intersection
    Queue* retour;  // File pour le trafic revenant de l'intersection
} lane;

// Structure for Circular Queue Node
typedef struct TrafficPhaseNode {
    TrafficLightPhase phase;
    int greenDuration;
    int redDuration;
    struct TrafficPhaseNode* next;
} TrafficPhaseNode;

// Structure for Circular Queue
typedef struct {
    TrafficPhaseNode* front;
    TrafficPhaseNode* rear;
} CircularQueue;

/* --- Fonctions de création et de gestion des structures --- */

// Crée et initialise un véhicule avec les paramètres fournis
Vehicule* createVehicule(int id, VehiculeType type, time_t arrivalTime, Direction origin, TurnDirection turn) {
    Vehicule* v = (Vehicule*)malloc(sizeof(Vehicule));
    v->id = id;
    v->type = type;
    v->arrivalTime = arrivalTime;
    v->origin = origin;
    v->turn = turn;
    v->next = NULL;
    return v;
}

// Crée et initialise une file d'attente avec la capacité maximale, un identifiant et une direction
Queue* createQueue(int max, int id, Direction dir) {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->id = id;
    q->direction = dir;
    q->first = q->last = NULL;
    q->size = 0;
    q->Maxcapacity = max;
    q->lightState = RED;
    q->baseGreenDuration = BASE_GREEN_DURATION;
    q->baseRedDuration = BASE_RED_DURATION;
    q->currentGreenDuration = q->baseGreenDuration;
    q->currentRedDuration = q->baseRedDuration;
    return q;
}

// Crée une voie en initialisant ses deux files ("aller" et "retour")
void Createlane(lane* l, int max, int id, Direction dir) {
    l->aller = createQueue(max, id, dir);
    l->retour = createQueue(max, id, dir);
}

// Vérifie si la file est pleine
int isFull(Queue* q) { 
    return q->size >= q->Maxcapacity; 
}

// Vérifie si la file est vide
int isEmpty(Queue* q) { 
    return q->size == 0; 
}

// Détecte un embouteillage si la taille de la file dépasse un seuil (défini dans config.h)
int detectTrafficJam(Queue* q) {
    return q->size >= q->Maxcapacity * TRAFFIC_JAM_THRESHOLD;
}

/* --- Ajustement des durées de feux --- */

// Ajuste les durées des feux pour une paire de files en fonction de la congestion
LightDurations adjustLightDurationsForPair(Queue* q1, Queue* q2) {
    LightDurations durations;
    if (detectTrafficJam(q1) || detectTrafficJam(q2)) {
        durations.greenDuration = q1->baseGreenDuration + GREEN_BOOST;
        durations.redDuration = q1->baseRedDuration - RED_REDUCTION;
    } else {
        durations.greenDuration = q1->baseGreenDuration;
        durations.redDuration = q1->baseRedDuration;
    }
    return durations;
}

/* --- Fonctions de journalisation (log) --- */

// Enregistre un message dans le fichier log avec un horodatage
void logWithTimestamp(FILE* logFile, const char* message) {
    time_t now = time(NULL);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(logFile, "[%s] %s\n", timestamp, message);
    fflush(logFile);
}

// Convertit une direction en chaîne de caractères
char* dirToString(Direction dir) {
    switch(dir) {
        case NORTH: return "Nord";
        case SOUTH: return "Sud";
        case EAST:  return "Est";
        case WEST:  return "Ouest";
        default:    return "Inconnu";
    }
}

// Convertit un type de véhicule en chaîne de caractères
char* typeToString(VehiculeType type) {
    switch(type) {
        case CAR:       return "Voiture";
        case BUS:       return "Bus";
        case BIKE:      return "Moto";
        case Emergency: return "Urgence";
        default:        return "Inconnu";
    }
}

// Convertit une direction de virage en chaîne de caractères
char* turnToString(TurnDirection turn) {
    switch(turn) {
        case LEFT:     return "Gauche";
        case RIGHT:    return "Droite";
        case STRAIGHT: return "Tout droit";
        default:       return "Inconnu";
    }
}

// Journalise l'état d'une file dans le fichier log
void logQueueState(Queue* q, FILE* logFile,  char* phase) {
    char* lightState = (q->lightState == GREEN) ? "VERT" : "ROUGE";
    char* trafficJam = detectTrafficJam(q) ? "Oui" : "Non";
    fprintf(logFile, "\n=== File %s (%s) ===\n", dirToString(q->direction), phase);
    fprintf(logFile, "Taille: %d/%d | Feu: %s | Embouteillage: %s | Vert: %ds, Rouge: %ds\n",
            q->size, q->Maxcapacity, lightState, trafficJam,
            q->currentGreenDuration, q->currentRedDuration);
    Vehicule* current = q->first;
    while (current != NULL) {
        fprintf(logFile, "Véhicule %d (Type: %s, Tourne: %s)\n",
               current->id, typeToString(current->type), turnToString(current->turn));
        current = current->next;
    }
    logWithTimestamp(logFile, "------------------------");
}

/* --- Fonctions de gestion des files --- */

// Ajoute un véhicule dans la file
// Les véhicules d'urgence sont ajoutés en tête de file
void enqueue(Queue* q, Vehicule* v, FILE* logFile) {
    if (v->type == Emergency) {
        v->next = q->first;
        q->first = v;
        if (q->last == NULL) {
            q->last = v;
        }
        q->size++;
        return;
    }
    if (isFull(q)) {
        logWithTimestamp(logFile, "ERREUR: File pleine!");
        return;
    }
    if (q->first == NULL) {
        q->first = v;
    } else {
        q->last->next = v;
    }
    q->last = v;
    q->size++;
}

// Retire le premier véhicule de la file et le retourne
Vehicule* dequeue(Queue* q, FILE* logFile) {
    if (isEmpty(q)) {
        logWithTimestamp(logFile, "ERREUR: File vide!");
        return NULL;
    }
    Vehicule* v = q->first;
    q->first = q->first->next;
    q->size--;
    if (q->first == NULL) {
        q->last = NULL;
    }
    return v;
}

/* --- Fonction de génération et de traitement des véhicules --- */
int vehicleId = 1; // Declare globally

void generateRandomVehicle(Queue* queue, FILE* logFile, int simTime) {
    VehiculeType type;
    if (rand() % 100 < EMERGENCY_CHANCE) {
        type = Emergency;
    } else {
        VehiculeType types[] = {CAR, BUS, BIKE};
        type = types[rand() % 3];
    }

    TurnDirection turn = rand() % 3;

    if (!isFull(queue)) {
        Vehicule* v = createVehicule(vehicleId, type, time(NULL) + simTime, queue->direction, turn);
        enqueue(queue, v, logFile);
        printf("🚗 Vehicle Created in %s-> ID: %d | Type: %s\n",dirToString(v->origin),vehicleId, typeToString(v->type));
        vehicleId++; // Increment only after successful enqueue
    } else {
        printf(" Queue is full! Vehicle %d not added.\n", vehicleId);
    }
}


// Traite la file : si le feu est vert, le véhicule en tête passe à la file "retour" d'une voie aléatoire
void processQueue(Queue* q, FILE* logFile, int* simTime, lane** lanes, int numLanes) {
    if (q->lightState == GREEN && !isEmpty(q)) {
        Vehicule* v = dequeue(q, logFile);
        if (v != NULL) {
            int randomIndex = rand() % numLanes;
            Queue* targetRetour = lanes[randomIndex]->retour;
            if (!isFull(targetRetour)) {
                enqueue(targetRetour, v, logFile);
                fprintf(logFile, "Véhicule %d déplacé vers %s (retour)\n", v->id, dirToString(targetRetour->direction));
            } else {
                fprintf(logFile, "Véhicule %d perdu (file retour pleine)\n", v->id);
                free(v);
            }
            *simTime += DURATION_FOR_VEHICULE_PASSATION;
        }
    }
}

/* --- Initialisation du fichier journal --- */

// Ouvre (ou crée) le fichier journal et enregistre le début de la simulation
FILE* initializeLogFile() {
    FILE* logFile = fopen("traffic_simulation.log", "w");
    if (!logFile) {
        perror("Erreur création fichier log");
        exit(EXIT_FAILURE);
    }
    logWithTimestamp(logFile, "Début simulation");
    return logFile;
}

// Initialize Circular Queue
void initCircularQueue(CircularQueue* q) {
    q->front = q->rear = NULL;
}

// Enqueue Traffic Light Phase (circular queue implementation)
void enqueuePhase(CircularQueue* q, TrafficLightPhase phase, int greenDuration, int redDuration) {
    TrafficPhaseNode* newNode = (TrafficPhaseNode*)malloc(sizeof(TrafficPhaseNode));
    newNode->phase = phase;
    newNode->greenDuration = greenDuration;
    newNode->redDuration = redDuration;
    newNode->next = q->front;  // Circular link

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

// Dequeue and return the next traffic light phase
TrafficPhaseNode* dequeuePhase(CircularQueue* q) {
    if (q->front == NULL) return NULL;

    TrafficPhaseNode* temp = q->front;
    q->rear->next = q->front->next; // Move front forward
    q->front = q->front->next;
    
    return temp; // Return the old phase (but not removed to maintain cycle)
}