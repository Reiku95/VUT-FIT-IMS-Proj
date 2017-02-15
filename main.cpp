#include "simlib.h"
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <iostream>

#define NUM_CARS 1
#define NUM_DAYS 7
#define GARBAGE_MEN_PER_CAR 3
#define HOUSEHOLDS_PER_TOWER_BLOCK 12

#define MAX_WEIGHT_COMUNAL_KG 30
#define MAX_WEIGHT_COMUNAL_BIG_KG 100
#define LITTER_PER_PERSON_KG_PER_YEAR 308
#define WEEK_CONSTANT 4.34812141
#define LITTER_PER_PERSON_KG_PER_DAY LITTER_PER_PERSON_KG_PER_YEAR/12/WEEK_CONSTANT/7
#define PEOPLE_PER_HOUSEHOLD 2.3
#define RESTAURANT 4.9104
#define CONSUMPTION 85
#define AVERAGE_TRANSFER_SPEED 80

#define FUEL_PRICE_CZK 30
#define GARBAGE_MAN_SALARY_HOUR_CZK 55
#define LIQUIDATE_FEE_PER_TON_CZK 1189
#define LIQUIDATE_SORTED_FEE_PER_TON_CZK 300
#define PARKING_SPACE_RENT_PER_DAY_CZK 12000/WEEK_CONSTANT/7


using namespace std;


/**
 * The street class
 */
class Street : public Facility {
    public:
        unsigned int meters, houses, sorted;
        const char* name;
        Street(unsigned int, unsigned int, const char*, unsigned int);
};


/**
 * Shared pointer to help with memory deallocation.
 */
typedef boost::shared_ptr<Street> street_ptr;

/**
 * Simulation functions.
 */
void getStreets();
void getNumberOfHouseholds();
void cleanStreets();
void cleanStreet(unsigned int position);
street_ptr getFreeStreet();
void printResults();
double getTravelDuration(int meters, double speed);
double convertMetersToKilometers(double meters);
double convertMinutesToHours(double minutes);
double convertKilosToTons(double kilos);

/**
 * Vector of streets to go through.
 */
vector<street_ptr> streets;

/** 
 * Simulation statistics.
 */
double duration = 0.0;
unsigned int transfers = 0;
unsigned int transfers_houses = 0;
unsigned int sum_meters = 0;
unsigned int sum_households = 0;
double litter_amount_kg = 0.0;
double total_litter_amount_picked = 0.0;
double total_sorted_amount_picked = 0.0;
unsigned int proc_count = 0;

/**
 * Simlib histogram.
 */
Histogram total_duration("Time on a single street", 0, 1, 15);

/**
 * Street constructor.
 */
Street::Street(unsigned int num_houses, unsigned int num_meters, const char* street_name = "", unsigned int num_sorted = 0) {
    houses = num_houses;
    meters = num_meters;
    name = street_name;
    sorted = num_sorted;
}

/**
 * The garbage car process.
 */
class Car : public Process {

public:
    string name;
    Car(string proc_name) {
        name = string("Car ").append(proc_name);
    }

    double serviceHouses(unsigned int numberOfHouses) {
        double sum = 0.0;
        for(unsigned int i = 0; i < numberOfHouses; i++) {
            sum += Uniform(0.10, 0.35);
        }
        return sum;
    }

    double serviceSorted(unsigned int numberOfSorted) {
        double sum = 0.0;
        for(unsigned int i = 0; i < numberOfSorted; i++) {
            sum += Uniform(0.20, 0.45);
        }
        return sum;
    }

    double transferBetweenHouses(unsigned int numberOfHouses) {
        double sum = 0.0;
        for(unsigned int i = 0; i < numberOfHouses; i++) {
            sum += Uniform(0.1, 0.15);
        }
        return sum;
    }

    void Behavior() {
        double start_time = 0.0;
        double travel_duration = 0.0;
        double work_duration = 0.0;
        double transfer_duration = 0.0;
        double sorted_duration = 0.0;
        double carrying_weight = 0.0;
        double carrying_sorted = 0.0;
        street_ptr current_street;

        //Print("%s running...\n", name.c_str());
        while((current_street = getFreeStreet())) {
            start_time = Time;
            current_street->Seize(this);
            
            if(current_street->houses != 0) {
                carrying_weight += Uniform(MAX_WEIGHT_COMUNAL_KG-(MAX_WEIGHT_COMUNAL_KG/5), MAX_WEIGHT_COMUNAL_KG);
                work_duration = serviceHouses(current_street->houses);
                duration += work_duration;
                Wait(work_duration);
                transfers_houses++;
            } else {
                travel_duration = getTravelDuration(current_street->meters, Exponential(AVERAGE_TRANSFER_SPEED));
                duration += travel_duration;
                Wait(travel_duration);
                transfers++;
            }

            /** STATISTICS */
            sum_meters += current_street->meters;
            transfer_duration = transferBetweenHouses(transfers_houses);
            sorted_duration = serviceSorted(current_street->sorted);
            carrying_sorted += current_street->sorted*Uniform(MAX_WEIGHT_COMUNAL_BIG_KG-50, MAX_WEIGHT_COMUNAL_BIG_KG);
            duration += transfer_duration + sorted_duration;

            Wait(transfer_duration + sorted_duration);
            total_duration(Time - start_time);

            current_street->Release(this);
        }
        total_litter_amount_picked += carrying_weight;
        total_sorted_amount_picked += carrying_sorted;
        //Print("%s is exiting with weight: %f kg\n", name.c_str(), carrying_weight);
    }
};

/**
 * The garbage car generator.
 */
class Generator : public Event {
    void Behavior() {
        proc_count++;
        (new Car(to_string(proc_count)))->Activate();
        if(proc_count < NUM_CARS) Activate(Time+1);
    }
};

/**
 * Initializes the basic simlib entities and runs
 * the simulation.
 * @param  argc
 * @param  argv
 * @return
 */
int main(int argc, char const *argv[])
{
    Init(0);
    RandomSeed(time(NULL)); // TODO change
    getStreets();
    getNumberOfHouseholds();

    (new Generator)->Activate();
    Run();
    printResults();
    return 0;
}

/**
 * Prints the final results of the simulation.
 */
void printResults() {
    double kilometers = convertMetersToKilometers(sum_meters);
    double consumpted = (kilometers*Uniform(CONSUMPTION-10, CONSUMPTION+10))/100;
    double final_duration = convertMinutesToHours(duration);
    double fuel_price = consumpted*Uniform(FUEL_PRICE_CZK-0.5, FUEL_PRICE_CZK+0.5)*NUM_CARS;
    double salary = final_duration*GARBAGE_MEN_PER_CAR*NUM_CARS*GARBAGE_MAN_SALARY_HOUR_CZK;
    double parking_rent = NUM_CARS*PARKING_SPACE_RENT_PER_DAY_CZK*NUM_DAYS;
    double liquidation_price = convertKilosToTons(total_litter_amount_picked)*LIQUIDATE_FEE_PER_TON_CZK;
    double liquidation_sorted_price = convertKilosToTons(total_sorted_amount_picked)*LIQUIDATE_SORTED_FEE_PER_TON_CZK;

    
    // STATIC DATA
    Print("Cars: %u\n", NUM_CARS);
    Print("Households: %u\n", sum_households);
    Print("Distance: %g km\n", convertMetersToKilometers(sum_meters));
    
    // DYNAMIC COLLECTION DATA
    Print("\nTotal duration: %g h\n", final_duration/NUM_CARS);
    Print("Total litter amount picked: %g kg\n", total_litter_amount_picked);
    Print("Total sorted amount picked: %g kg\n", total_sorted_amount_picked);
    Print("Total fuel consumption: %gl\n", consumpted*NUM_CARS);

    // DYNAMIC PRICE DATA
    Print("\nTotal fuel price: %g CZK\n", fuel_price);
    Print("Total garbage men salary: %g CZK\n", salary);
    Print("Total parking space rent: %g CZK\n", parking_rent);
    Print("Total communal liquidation price: %g CZK\n", liquidation_price);
    Print("Total sorted liquidation price: %g CZK\n", liquidation_sorted_price);
    Print("\nTotal price: %g CZK\n", fuel_price+salary+parking_rent+liquidation_price+liquidation_sorted_price);

    total_duration.Output();
}

/**
 * Goes through the whole street vector and sums the number
 * of households.
 */
void getNumberOfHouseholds() {
    for(unsigned int i = 0; i < streets.size(); i++) {
        sum_households += streets[i]->houses;
    }
}

/**
 * Returns the duration it takes to travel a certain distance
 * at certain speed.
 * @param  meters Number of meters to travel.
 * @param  speed  The speed to travel at (in kmh-1)
 * @return        The duration it takes to travel the distance.
 */
double getTravelDuration(int meters, double speed) {
    return ((convertMetersToKilometers(meters)/speed)*60);  
}

/**
 * Converts meters to kilometers.
 * @param  meters The number of meters to convert.
 * @return        The converted number.
 */
double convertMetersToKilometers(double meters) {
    return meters/1000.0;
}

/**
 * Converts minutes to hours.
 * @param  minutes The number of minutes to convert.
 * @return         The converted number.
 */
double convertMinutesToHours(double minutes) {
    return minutes/60.0;
}

/**
 * Converts kilos to tons.
 * @param  kilos The number of kilos to convert.
 * @return       The converted number.
 */
double convertKilosToTons(double kilos) {
    return kilos/100.0;
}

/**
 * Returns the closest street which is not being serviced.
 * @return The closest free street.
 */
street_ptr getFreeStreet() {
    for(unsigned int i = 0; i < streets.size(); i++) {
        if(streets[i]->Busy()) continue;
        else {
            street_ptr result = streets[i];
            streets.erase(streets.begin() + i);
            return result;
        }
    }
    return NULL;
}

/**
 * Fills the streets vector with street data.
 */
void getStreets() {
    streets.push_back(street_ptr (new Street(12, 750, "Dlouha")));
    streets.push_back(street_ptr (new Street(2, 55, "Horni", 5)));
    streets.push_back(street_ptr (new Street(5, 190, "Horni")));
    streets.push_back(street_ptr (new Street(0, 190, "Horni zpet")));
    streets.push_back(street_ptr (new Street(20, 400, "Horni")));
    streets.push_back(street_ptr (new Street(5, 71, "4473")));
    streets.push_back(street_ptr (new Street(0, 71, "4473 zpet")));
    streets.push_back(street_ptr (new Street(19, 290, "Horni")));
    streets.push_back(street_ptr (new Street(12, 180, "U Splavu", 5))); // 1 coop
    streets.push_back(street_ptr (new Street(0, 180, "U Splavu zpet")));
    streets.push_back(street_ptr (new Street(6, 150, "Horni")));
    streets.push_back(street_ptr (new Street(6, 72, "Zahradni")));
    streets.push_back(street_ptr (new Street(0, 72, "Zahradni zpet")));
    streets.push_back(street_ptr (new Street(22, 550, "Horni")));
    streets.push_back(street_ptr (new Street(3, 74, "Dlouha")));
    streets.push_back(street_ptr (new Street(0, 74, "Dlouha zpet")));
    streets.push_back(street_ptr (new Street(11, 140, "Horni")));
    streets.push_back(street_ptr (new Street(10, 160, "44613")));
    streets.push_back(street_ptr (new Street(4, 28, "Oskava")));
    streets.push_back(street_ptr (new Street(10, 130, "Horni")));
    streets.push_back(street_ptr (new Street(3, 54, "Horni")));
    streets.push_back(street_ptr (new Street(7, 89, "Sokolska")));
    streets.push_back(street_ptr (new Street(0, 89, "Sokolska zpet")));
    streets.push_back(street_ptr (new Street(1, 60, "Horni", 5)));
    streets.push_back(street_ptr (new Street(6, 140, "Oskava"))); // ceska posta + kino
    streets.push_back(street_ptr (new Street(0, 140, "Oskava zpet")));
    streets.push_back(street_ptr (new Street(9, 120, "Pravoslavna")));
    streets.push_back(street_ptr (new Street(0, 120, "Pravoslavna zpet")));
    streets.push_back(street_ptr (new Street(6, 300, "Dolni"))); // kostel, skola, hrbitov
    streets.push_back(street_ptr (new Street(2, 36, "Dolni"))); // supermarket, sokolovna
    streets.push_back(street_ptr (new Street(0, 36, "Dolni zpet")));
    streets.push_back(street_ptr (new Street(17, 290, "Dolni")));
    streets.push_back(street_ptr (new Street(4, 36, "Dolni")));
    streets.push_back(street_ptr (new Street(0, 36, "Dolni zpet")));
    streets.push_back(street_ptr (new Street(8, 68, "Dolni")));
    streets.push_back(street_ptr (new Street(21, 260, "Dolni", 8))); // maly obchod
    streets.push_back(street_ptr (new Street(18, 300, "Polni"))); // autolakovna
    streets.push_back(street_ptr (new Street(0, 250, "Dolni zpet")));
    streets.push_back(street_ptr (new Street(3, 77, "Dolni")));
    streets.push_back(street_ptr (new Street(3, 77, "Dolni")));
    streets.push_back(street_ptr (new Street(6, 79, "Dolni"))); // vinoteka
    streets.push_back(street_ptr (new Street(0, 79, "Dolni zpet")));
    streets.push_back(street_ptr (new Street(3, 39, "Na Travniku")));
    streets.push_back(street_ptr (new Street(4, 54, "Na Travniku")));
    streets.push_back(street_ptr (new Street(0, 54, "Na Travniku zpet")));
    streets.push_back(street_ptr (new Street(2, 26, "Dolni")));
    streets.push_back(street_ptr (new Street(6, 110, "Delnicka")));
    streets.push_back(street_ptr (new Street(7, 280, "Delnicka"))); // kvetinarstvi
    streets.push_back(street_ptr (new Street(0, 110, "Delnicka zpet")));
    streets.push_back(street_ptr (new Street(4, 110, "Delnicka"))); // prumyslova budova
    streets.push_back(street_ptr (new Street(11, 290, "Nadrazni", 7)));
    streets.push_back(street_ptr (new Street(3, 81, "Tovarni"))); // zahradnictvi + prumyslova budova
    streets.push_back(street_ptr (new Street(0, 81, "Tovarni zpet")));
    streets.push_back(street_ptr (new Street(4, 130, "Nadrazni")));
    streets.push_back(street_ptr (new Street(9, 150, "Tovarni")));
    streets.push_back(street_ptr (new Street(4, 110, "Nadrazni", 5))); // restaurace
    streets.push_back(street_ptr (new Street(0, 170, "Nadrazni zpet")));
    streets.push_back(street_ptr (new Street(5, 200, "Stepana Krejciho")));
    streets.push_back(street_ptr (new Street(0, 24, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(8, 210, "Hybesova")));
    streets.push_back(street_ptr (new Street(2, 50, "Sidliste")));
    streets.push_back(street_ptr (new Street(11, 210, "Nadjezdova", 7)));
    streets.push_back(street_ptr (new Street(0, 18, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(6, 210, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(2*HOUSEHOLDS_PER_TOWER_BLOCK, 90, "Sidliste")));
    streets.push_back(street_ptr (new Street(2*HOUSEHOLDS_PER_TOWER_BLOCK, 99, "Nadrazni")));
    streets.push_back(street_ptr (new Street(0, 99, "Nadrazni zpet")));
    streets.push_back(street_ptr (new Street(HOUSEHOLDS_PER_TOWER_BLOCK, 150, "Sidliste", 8)));
    streets.push_back(street_ptr (new Street(3, 57, "Brezecka", 7))); // coop
    streets.push_back(street_ptr (new Street(22, 210, "Brezecka")));
    streets.push_back(street_ptr (new Street(5, 75, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(7+HOUSEHOLDS_PER_TOWER_BLOCK, 88, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(9, 140, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(0, 88, "Nadjezdova")));
    streets.push_back(street_ptr (new Street(0, 83, "Pod nadjezdem")));
    streets.push_back(street_ptr (new Street(2, 47, "Pod nadjezdem")));
    streets.push_back(street_ptr (new Street(10, 130, "Nova")));
    streets.push_back(street_ptr (new Street(25, 350, "Nova")));
    streets.push_back(street_ptr (new Street(8, 100, "Brezecka")));
}
