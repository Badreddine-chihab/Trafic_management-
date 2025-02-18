#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

/* --- Declarations des enumerations --- */

// Directions possibles pour l'origine des vehicules
typedef enum { NORTH,
            SOUTH,
            EAST,
            WEST } Direction;

// Types de vehicules disponibles dans la simulation
typedef enum { CAR,
        BUS,
        BIKE,
        Emergency } VehiculeType;

// Directions possibles pour le virage d'un vehicule
typedef enum { LEFT,
        RIGHT,
        STRAIGHT } TurnDirection;

// etats des feux de circulation
typedef enum { RED,
        GREEN } TrafficLightState;

// Phases des feux de circulation pour l'intersection
typedef enum {
NORTH_SOUTH_GREEN, // Phase où les feux Nord-Sud sont verts
EAST_WEST_GREEN    // Phase où les feux Est-Ouest sont verts
} TrafficLightPhase;

/* --- Declaration des structures --- */

// Structure representant un vehicule dans la simulation
typedef struct Vehicule {
int id;                     // Identifiant du vehicule
VehiculeType type;          // Type de vehicule (voiture, bus, moto, urgence)
time_t arrivalTime;         // Heure d'arrivee dans la file
Direction origin;           // Direction d'origine du vehicule
TurnDirection turn;         // Direction dans laquelle le vehicule va tourner
struct Vehicule* next;      // Pointeur vers le vehicule suivant dans la file
} Vehicule;

// Structure pour stocker les durees des feux (vert et rouge)
typedef struct {
int greenDuration;
int redDuration;
} LightDurations;

// Structure representant une file d'attente pour les vehicules d'une direction donnee
typedef struct Queue {
int id;                         // Identifiant de la file
Vehicule* first;                // Premier vehicule dans la file
Vehicule* last;                 // Dernier vehicule dans la file
int size;                       // Nombre de vehicules dans la file
int Maxcapacity;                // Capacite maximale de la file
TrafficLightState lightState;   // etat actuel du feu pour cette file (VERT/ROUGE)
Direction direction;            // Direction associee à cette file
int baseGreenDuration;          // Duree de base pour le feu vert
int baseRedDuration;            // Duree de base pour le feu rouge
int currentGreenDuration;       // Duree actuelle du feu vert (peut être ajustee)
int currentRedDuration;         // Duree actuelle du feu rouge (peut être ajustee)
} Queue;

// Structure representant une voie composee de deux files : "aller" et "retour"
typedef struct lane {
Queue* aller;   // File pour le trafic allant vers l'intersection
Queue* retour;  // File pour le trafic revenant de l'intersection
} lane;

// Structure pour stocker les phases de feux pour l'intersection
typedef struct TrafficPhaseNode {
TrafficLightPhase phase;
int greenDuration;
int redDuration;
struct TrafficPhaseNode* next;
} TrafficPhaseNode;

// Structure des queues circulaires
typedef struct {
TrafficPhaseNode* front;
TrafficPhaseNode* rear;
} LLCircular;

typedef struct TrafficHistoryNode {
Vehicule* vehicule;
struct TrafficHistoryNode* next;
} TrafficHistoryNode;

typedef struct {
TrafficHistoryNode* top; // pointeur pour LIFO
} TrafficHistoryStack;

/* --- Fonctions de creation et de gestion des structures --- */

// Cree et initialise un vehicule avec les paramètres fournis
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

// Cree et initialise une file d'attente avec la capacite maximale, un identifiant et une direction
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

// Cree une voie en initialisant ses deux files ("aller" et "retour")
void Createlane(lane* l, int max, int id, Direction dir) {
l->aller = createQueue(max, id, dir);
l->retour = createQueue(max, id, dir);
}

// Verifie si la file est pleine
int isFull(Queue* q) { 
return q->size >= q->Maxcapacity; 
}

// Verifie si la file est vide
int isEmpty(Queue* q) { 
return q->size == 0; 
}

// Detecte un embouteillage si la taille de la file depasse un seuil (defini dans config.h)
int detectTrafficJam(Queue* q) {
return q->size >= q->Maxcapacity * TRAFFIC_JAM_THRESHOLD;
}

/* --- Ajustement des durees de feux --- */

// Ajuste les durees des feux pour une paire de files en fonction de la congestion
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
void logWithTimestamp(FILE* logFile, char* message) {
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

// Convertit un type de vehicule en chaîne de caractères
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

// Journalise l'etat d'une file dans le fichier log
void logQueueState(Queue* q, FILE* logFile,  char* phase) {
char* lightState = (q->lightState == GREEN) ? "VERT" : "ROUGE";
char* trafficJam = detectTrafficJam(q) ? "Oui" : "Non";
fprintf(logFile, "\n=== File %s (%s) ===\n", dirToString(q->direction), phase);
fprintf(logFile, "Taille: %d/%d | Feu: %s | Embouteillage: %s | Vert: %ds, Rouge: %ds\n",
        q->size, q->Maxcapacity, lightState, trafficJam,
        q->currentGreenDuration, q->currentRedDuration);
Vehicule* current = q->first;
while (current != NULL) {
    fprintf(logFile, "Vehicule %d (Type: %s, Tourne: %s)\n",
        current->id, typeToString(current->type), turnToString(current->turn));
    current = current->next;
}
logWithTimestamp(logFile, "------------------------");
}

/* --- Fonctions de gestion des files --- */

// Ajoute un vehicule dans la file d'historique du trafic
void pushToTrafficHistory(TrafficHistoryStack* history, Vehicule* v) { // urgence ne sont pas add 
    if (v == NULL) {
        printf("[ERREUR] Vehicule NULL passe à l'historique.\n");
        return;
    }

    TrafficHistoryNode* newNode = (TrafficHistoryNode*)malloc(sizeof(TrafficHistoryNode));
    if (newNode == NULL) {
        printf("[ERREUR] Memoire insuffisante pour le nœud d'historique.\n");
        return;
    }

    newNode->vehicule = (Vehicule*)malloc(sizeof(Vehicule));
    if (newNode->vehicule == NULL) {
        printf("[ERREUR] Memoire insuffisante pour le vehicule dans l'historique.\n");
        free(newNode);
        return;
    }

    // Deep copy the vehicle data
    memcpy(newNode->vehicule, v, sizeof(Vehicule));
    newNode->next = history->top;
    history->top = newNode;
}


// Ajoute un vehicule dans la file
// Les vehicules d'urgence sont ajoutes en tête de file
void enqueue(Queue* q, Vehicule* v, FILE* logFile,TrafficHistoryStack *history) {
    if (v == NULL) {
        logWithTimestamp(logFile, "ERREUR: Tentative d'ajouter un vehicule NULL");
        return;
    }
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
    pushToTrafficHistory(history, v); 
}

// Retire le premier vehicule de la file et le retourne
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

/* --- Fonction de generation et de traitement des vehicules --- */ 
int vehicleId = 1; // VARIABLE GLOBALE

void generateRandomVehicle(Queue* queue, FILE* logFile, unsigned int simTime,TrafficHistoryStack *history) {
    VehiculeType type;
    if (rand() % 100 < EMERGENCY_CHANCE) {
        type = Emergency;
    } else {
        VehiculeType types[] = {CAR, BUS, BIKE};
        type = types[rand() % 3];
    }

    TurnDirection turn = rand() % 3;

    if (!isFull(queue)) {
        Vehicule* v = createVehicule(vehicleId, type, simTime, queue->direction, turn);
        if (v == NULL) {
            fprintf(logFile, "Erreur: Impossible de creer le vehicule %d\n", vehicleId);
            return; // Skip enqueue on failure
        }
        enqueue(queue, v, logFile,history);
        printf("Vehicle Created in %s -> ID: %d | Type: %s\n", 
                dirToString(v->origin), vehicleId, typeToString(v->type));
        vehicleId++;
    } else {
        printf("Queue is full! Vehicle %d not added.\n", vehicleId);
    }
}


/* --- Initialisation du fichier journal --- */

// Ouvre (ou cree) le fichier journal et enregistre le debut de la simulation
FILE* initializeLogFile() {
    FILE* logFile = fopen("traffic_simulation.log", "w");
    if (!logFile) {
        perror("Erreur creation fichier log");
        exit(EXIT_FAILURE);
    }
    logWithTimestamp(logFile, "Debut simulation");
    return logFile;
}

// Initialise la queue circulaire
void initLLCircular(LLCircular* q) {
    q->front = q->rear = NULL;
}

// Enqueue la phase du Traffic Light (implementation circulaire du queue )
void enqueuePhase(LLCircular* q, TrafficLightPhase phase, int greenDuration, int redDuration) {
    TrafficPhaseNode* newNode = (TrafficPhaseNode*)malloc(sizeof(TrafficPhaseNode));
    newNode->phase = phase;
    newNode->greenDuration = greenDuration;
    newNode->redDuration = redDuration;
    newNode->next = q->front;  // liaison circulaire

    if (q->rear == NULL) {
        q->front = q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }
}

// Dequeue et retourne la phase du Trafic light 
TrafficPhaseNode* dequeuePhase(LLCircular* q) {
    if (q->front == NULL) return NULL;

    TrafficPhaseNode* temp = q->front;
    q->rear->next = q->front->next; // Move front forward
    q->front = q->front->next;

    return temp; // Return la pahse ancienne (sans suppression pour maintenir le cycle)
}

// Initialisation de l'historique du systeme
void initTrafficHistory(TrafficHistoryStack* history) {
    history->top = NULL;
}

// Fonction principale de la simulation
void processQueue(Queue* q, FILE* logFile,TrafficHistoryStack *history, unsigned int* simTime, lane** lanes, int numLanes) {
    if (q->lightState == GREEN && !isEmpty(q)) {
        Vehicule* v = dequeue(q, logFile);
        if (v == NULL) {
            logWithTimestamp(logFile, "ERREUR: Dequeue a retourne NULL dans une file non vide");
            return;
        }

        // Validation du data de la vehicule
        if (v->id <= 0 || v->arrivalTime < 0) {
            fprintf(logFile, "Donnees vehicule invalides: ID=%d, Temps=%d\n", v->id, v->arrivalTime);
            free(v);
            return;
        }

            
        // Calcul du temps de passage
        time_t passTime = *simTime + DURATION_FOR_VEHICULE_PASSATION;
            
        // Si le vehicule est en direction d'entrée, il va vers la voie de retour
        int randomIndex = rand() % numLanes;
        Queue* targetRetour = lanes[randomIndex]->retour;
            
        // Si la file de retour n'est pas pleine, on l'ajoute
        if (!isFull(targetRetour)) {
            enqueue(targetRetour, v, logFile,history);
            fprintf(logFile, "Vehicle %d moved to %s return lane\n", 
                        v->id, dirToString(targetRetour->direction));
            printf("Vehicle %d processed from %s at t=%u\n", 
                        v->id, dirToString(q->direction), passTime);
        } else {
            fprintf(logFile, "Vehicle %d lost (return lane full)\n", v->id);
            free(v);
        }
            
        // Incrementation du temps de simulation
        *simTime += DURATION_FOR_VEHICULE_PASSATION;
    }
}   



// Fonction principale du programme de simulation 

// Affichage de la simulation dans l'histoirique du systeme
void printTrafficHistory(TrafficHistoryStack* history) {
    if (history == NULL) {
        printf("\n[ERREUR] Pointeur d'historique est NULL.\n");
        return;
    }

    printf("\n=== Historique du trafic (plus recent en premier) ===\n");
    
    TrafficHistoryNode* current = history->top;
    int count = 0;
    
    while (current != NULL) {
        if (current->vehicule == NULL) {
            printf("[ERREUR] Vehicule invalide dans l'historique.\n");
        } else {
            printf("%d. ID: %d | Type: %s | Direction: %s | Arrivee: %d\n",
                ++count,
                current->vehicule->id,
                typeToString(current->vehicule->type),
                dirToString(current->vehicule->origin),
                current->vehicule->arrivalTime);
            }
        current = current->next;
    }
        
    if (count == 0) {
        printf("Aucun vehicule dans l'historique.\n");
    }
}

// Suppression de l'historique du systeme
void clearTrafficHistory(TrafficHistoryStack* history) {
    while (history->top != NULL) {
        TrafficHistoryNode* temp = history->top;
        history->top = history->top->next;
        
        if (temp->vehicule != NULL) {
            free(temp->vehicule);
        }
        free(temp);
    }
}