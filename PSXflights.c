#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define FNV 0x000001000000001B3
#define FNVOFF 0xCBF29CE484222325

#define NB 10000
#define EARTH_RAD 6371008
#define M_PI 3.14159265359

#define SPEED 252.22

typedef struct ARPT {
    char ID[4];
    double latitude;
    double longitude;
    struct ARPT *next;
} ARPT;

ARPT **airports = NULL;
char (*A)[5];

int nbairport;
int nbcollision = 0;

uint64_t hash(const char *s)
{
    uint64_t result = FNVOFF;

    if (strlen(s) != 4)
        return 0;

    for (int i = 0; i < 4; i++) {
        result ^= (unsigned long)s[i];
        result *= FNV;
    }

    return result % NB;
}

static int insert(ARPT *airport)
{
    uint64_t index;
    index = hash(airport->ID);
    if (airports[index]) {
        ARPT *tmp;
        tmp = airports[index];
        while (tmp) {

            if (strcmp(airports[index]->ID, airport->ID) == 0) {
                return 0;
            }
            tmp = tmp->next;
        }

        tmp = airports[index];
        airports[index] = airport;
        airports[index]->next = tmp;
        nbcollision++;
    } else {
        airports[index] = airport;
    }
    return 1;
}
double dist(double lat1, double lat2, double long1, double long2)
{
    return 2 * EARTH_RAD *
           (sqrt(pow(sin((lat2 - lat1) / 2), 2) +
                 cos(lat1) * cos(lat2) * pow(sin((long2 - long1) / 2), 2)));
}

void print_airports(int N)
{
    ARPT *a;
    for (int i = 0; i < N; i++) {
        printf("A[%d]: %s\n", i, A[i]);
    }

    for (int i = 0; i < NB; i++) {
        if (airports[i]) {
            printf("ID[%d] %s\t", i, airports[i]->ID);
            a = airports[i];
            while (a->next) {
                a = a->next;
                printf("%s\t", a->ID);
            }
            printf("\n");
        }
    }
}

static ARPT **create_table(size_t size)
{
    return calloc(size, sizeof(ARPT *));
}

static ARPT *create_airport(void)
{
    ARPT *tmp = malloc(sizeof(ARPT));
    return tmp;
}

static int readarpt(const char *name)
{
    FILE *fp;
    int nb = 0;
    char buffer[4096];
    char *token;
    ARPT *arpt = NULL;

    fp = fopen(name, "r");
    if (fp == NULL) {
        printf("Could not open %s. Exiting now\n", name);
        exit(EXIT_FAILURE);
    }
    while (fgets(buffer, sizeof buffer, fp) != NULL) {
        // ICAO ID
        token = strtok(buffer, ",");
        if (strlen(token) == 4) {
            arpt = create_airport();
            if (arpt == NULL) {
                printf("Could not create airport\n");
                exit(EXIT_FAILURE);
            }
            strcpy(arpt->ID, token);
            // ignore 4 next tokens
            strtok(NULL, ",");
            strtok(NULL, ",");
            strtok(NULL, ",");
            strtok(NULL, ",");
            arpt->latitude = strtod(strtok(NULL, ","), NULL) / 180 * M_PI;
            arpt->longitude = strtod(strtok(NULL, ","), NULL) / 180 * M_PI;

            if (insert(arpt) == 0) {
                free(arpt);
            } else {
                nb++;
            }
        }
    }
    fclose(fp);
    return nb;
}
void delete_table(ARPT **table)
{
    ARPT *tmp;
    for (int i = 0; i < NB; i++) {
        if (table[i] != NULL) {
            while (table[i]->next != NULL) {
                tmp = table[i]->next;
                table[i]->next = tmp->next;
                free(tmp);
            }

            free(table[i]);
        }
    }
    free(table);
}

void create_list(int nbarpt)
{
    int nb = 0;
    A = malloc(nbarpt * sizeof(*A));
    for (int i = 0; i < NB; i++) {
        ARPT *a;
        if (airports[i]) {
            strcpy(A[nb], airports[i]->ID);
            nb++;
            a = airports[i];
            while (a->next) {
                a = a->next;
                strcpy(A[nb], a->ID);
                nb++;
            }
        }
    }
}
void delete_liste(void)
{
    free(A);
}

double distance (const char *arpt1, const char *arpt2)
{
    uint64_t idx1, idx2;

    idx1 = hash(arpt1);
    idx2 = hash(arpt2);
    if (airports[idx1] == NULL || airports[idx2] == NULL) {
        return -1;
    }

    return dist(airports[idx1]->latitude, airports[idx2]->latitude, airports[idx1]->longitude, airports[idx2]->longitude);
}

int find_destination(const char *depart, double expected_duration)
{

    uint64_t arptidx;
    int nb, result = 0;
    double duration;

    arptidx = hash(depart);
    if (airports[arptidx] == NULL) {
        return -1;
    }

    srand(time(NULL));
    nb = rand() % nbairport;
    for (;;) {

        duration = distance(depart, A[nb % nbairport]) / SPEED;
        // printf("%s to %s = %.2f hours\n",airports[arptidx]->ID, A[nb%nbairport],duration/3600);
        if (fabs((duration - 3600 * expected_duration) / (3600 * expected_duration)) < 0.1) {
            result = nb;
            break;
        }
        nb++;
    }

    return hash(A[result]);
}

int main(int argc, char **argv)
{
    int dest_airport_idx;
    double flight_time;

    if (argc < 4) {
        printf("Usage %s <RWY.csv> <ICAO> duration \n", argv[0]);
        return EXIT_FAILURE;
    }
    if (strlen(argv[2]) != 4) {
        printf("Starting ICAO is invalid\n");
        return EXIT_FAILURE;
    }

    flight_time = strtof(argv[3], NULL);
    if (flight_time <= 0) {
        printf("Wrong duration of flight\n");
        return EXIT_FAILURE;
    }

    airports = create_table(NB);
    nbairport = readarpt(argv[1]);

    create_list(nbairport);

    // print_airports(nbairport);
    printf("%d airports imported\n", nbairport);
    printf("With %d collisions\n", nbcollision);

    dest_airport_idx = find_destination(argv[2], flight_time);

    if (dest_airport_idx > 0) {
        printf("Found a destination: %s->%s: %.1f hours\n", argv[2], airports[dest_airport_idx]->ID,distance(argv[2], airports[dest_airport_idx]->ID) / SPEED/3600);
    } else {
        printf("No airport is within %.1f hours +/- 10%% flying time from %s\n", flight_time, argv[2]);
    }
    delete_table(airports);
    delete_liste();
    return 0;
}
