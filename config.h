#ifndef CONFIG_H
#define CONFIG_H

#define SIMULATION_DURATION 20  // Simulation duration in seconds 
#define QUEUE_CAPACITY 4           // Maximum capacity of each queue
#define EMERGENCY_CHANCE 20        // 20% chance for an emergency vehicle
#define VEHICLE_GEN_PROB 80     // 80% chance to generate a vehicle when light is red
#define BASE_GREEN_DURATION 2    // Base green light duration in seconds
#define BASE_RED_DURATION 4      // Base red light duration in seconds
#define GREEN_BOOST 1            // Extra green time during traffic jam
#define RED_REDUCTION 1           // Reduced red time during traffic jam
#define TRAFFIC_JAM_THRESHOLD 0.75 // 75% of capacity considered as a traffic jam
#define TIME_INCREMENT 1           // Time increment for simulation (1 second)
#define DURATION_FOR_VEHICULE_PASSATION 1  //durée que prend un véhicule pour passer au feu vert 

#endif // CONFIG_H