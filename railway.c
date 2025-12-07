#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------- Structures ----------------
typedef struct Passenger {
    int ticket_id;
    struct Passenger *next;
} Passenger;

typedef struct Station {
    char name[32];
    struct Station *next;
} Station;

typedef struct Stop {
    Station *station;
    struct Stop *next;
} Stop;

typedef struct Train {
    char name[32];
    Stop *stops;
    Passenger *passengers;
    struct Train *next;
} Train;

// ---------------- Global Lists ----------------
Train *train_head = NULL;
Station *station_head = NULL;

// ---------------- Station Functions ----------------
Station* create_station(const char *name){
    Station *s = (Station*)malloc(sizeof(Station));
    strcpy(s->name,name);
    s->next = NULL;
    return s;
}

void add_station(){
    char name[32];
    printf("Enter Station Name: ");
    scanf("%s",name);
    Station *s = create_station(name);
    s->next = station_head;
    station_head = s;
    printf("Station '%s' added.\n",name);
}

void delete_station(){
    char name[32];
    printf("Enter Station Name to delete: ");
    scanf("%s",name);
    Station *cur = station_head, *prev=NULL;
    while(cur){
        if(strcmp(cur->name,name)==0){
            if(prev) prev->next = cur->next;
            else station_head = cur->next;
            free(cur);
            printf("Station '%s' deleted.\n",name);
            return;
        }
        prev=cur;
        cur=cur->next;
    }
    printf("Station '%s' not found.\n",name);
}

void list_stations(){
    printf("Stations: ");
    Station *cur = station_head;
    while(cur){
        printf("%s ",cur->name);
        cur=cur->next;
    }
    printf("\n");
}

// ---------------- Train Functions ----------------
Train* create_train(const char *name){
    Train *t = (Train*)malloc(sizeof(Train));
    strcpy(t->name,name);
    t->stops = NULL;
    t->passengers = NULL;
    t->next = NULL;
    return t;
}

void add_train(){
    char name[32];
    printf("Enter Train Name: ");
    scanf("%s",name);
    Train *t = create_train(name);
    t->next = train_head;
    train_head = t;
    printf("Train '%s' added.\n",name);
}

void delete_train(){
    char name[32];
    printf("Enter Train Name to delete: ");
    scanf("%s",name);
    Train *cur = train_head, *prev=NULL;
    while(cur){
        if(strcmp(cur->name,name)==0){
            if(prev) prev->next = cur->next;
            else train_head = cur->next;
            // Free passengers
            Passenger *p = cur->passengers;
            while(p){ Passenger *tmp = p; p=p->next; free(tmp);}
            // Free stops
            Stop *s = cur->stops;
            while(s){ Stop *tmp = s; s=s->next; free(tmp);}
            free(cur);
            printf("Train '%s' deleted.\n",name);
            return;
        }
        prev=cur;
        cur=cur->next;
    }
    printf("Train '%s' not found.\n",name);
}

void list_trains(){
    printf("Trains: ");
    Train *cur = train_head;
    while(cur){
        printf("%s ",cur->name);
        cur=cur->next;
    }
    printf("\n");
}

// ---------------- Stops Functions ----------------
void add_stop_to_train(){
    char train_name[32], station_name[32];
    printf("Enter Train Name: ");
    scanf("%s",train_name);
    Train *t = train_head;
    while(t && strcmp(t->name,train_name)!=0) t=t->next;
    if(!t){ printf("Train not found.\n"); return; }

    printf("Enter Station Name to add as stop: ");
    scanf("%s",station_name);
    Station *s = station_head;
    while(s && strcmp(s->name,station_name)!=0) s=s->next;
    if(!s){ printf("Station not found.\n"); return; }

    Stop *new_stop = (Stop*)malloc(sizeof(Stop));
    new_stop->station = s;
    new_stop->next = NULL;

    if(!t->stops) t->stops = new_stop;
    else {
        Stop *cur = t->stops;
        while(cur->next) cur=cur->next;
        cur->next = new_stop;
    }
    printf("Stop '%s' added to Train '%s'.\n",station_name,train_name);
}

// ---------------- Passenger Functions ----------------
void add_passenger(){
    char train_name[32], station_name[32];
    int ticket_id;
    printf("Enter Train Name: ");
    scanf("%s",train_name);
    Train *t = train_head;
    while(t && strcmp(t->name,train_name)!=0) t=t->next;
    if(!t){ printf("Train not found.\n"); return; }

    printf("Enter Station Name where passenger boards: ");
    scanf("%s",station_name);
    Stop *stop = t->stops;
    int found=0;
    while(stop){ if(strcmp(stop->station->name,station_name)==0){ found=1; break; } stop=stop->next; }
    if(!found){ printf("Train does not stop at'%s'.\n",station_name); return; }

    printf("Enter Ticket ID: ");
    scanf("%d",&ticket_id);
    Passenger *p = (Passenger*)malloc(sizeof(Passenger));
    p->ticket_id = ticket_id;
    p->next = t->passengers;
    t->passengers = p;
    printf("Passenger with Ticket ID %d added to Train '%s' at Station '%s'.\n",ticket_id,train_name,station_name);
}

void remove_passenger(){
    char train_name[32];
    int ticket_id;
    printf("Enter Train Name: ");
    scanf("%s",train_name);
    Train *t = train_head;
    while(t && strcmp(t->name,train_name)!=0) t=t->next;
    if(!t){ printf("Train not found.\n"); return; }

    printf("Enter Ticket ID to remove: ");
    scanf("%d",&ticket_id);
    Passenger *cur = t->passengers, *prev=NULL;
    while(cur){
        if(cur->ticket_id == ticket_id){
            if(prev) prev->next = cur->next;
            else t->passengers = cur->next;
            free(cur);
            printf("Passenger with Ticket ID %d removed from Train '%s'.\n",ticket_id,train_name);
            return;
        }
        prev=cur;
        cur=cur->next;
    }
    printf("Passenger with Ticket ID %d not found in Train '%s'.\n",ticket_id,train_name);
}

// ---------------- Run Train ----------------
void run_train(){
    char train_name[32];
    printf("Enter Train Name to run: ");
    scanf("%s",train_name);
    Train *t = train_head;
    while(t && strcmp(t->name,train_name)!=0) t=t->next;
    if(!t){ printf("Train not found.\n"); return; }

    Stop *stop = t->stops;
    if(!stop){ printf("Train has no stops.\n"); return; }

    printf("Train '%s' starting journey...\n",t->name);
    while(stop){
        printf("Arrived at Station '%s'.\n",stop->station->name);
        Passenger *p = t->passengers;
        if(p){
            printf("Passengers on board: ");
            while(p){ printf("%d ",p->ticket_id); p=p->next; }
            printf("\n");
        } else printf("No passengers on board.\n");
        stop = stop->next;
    }
    printf("Train '%s' journey ended.\n",t->name);
}

// ---------------- Main Menu ----------------
void menu(){
    int choice;
    while(1){
        printf("\n---- Railway Management Menu ----\n");
        printf("1. Add Train\n2. Delete Train\n3. Add Station\n4. Delete Station\n5. Add Stop to Train\n6. Run Train\n7. Add Passenger\n8. Remove Passenger\n9. List Trains\n10. List Stations\n11. Exit\n");
        printf("Enter choice: ");
        scanf("%d",&choice);
        switch(choice){
            case 1: add_train(); break;
            case 2: delete_train(); break;
            case 3: add_station(); break;
            case 4: delete_station(); break;
            case 5: add_stop_to_train(); break;
            case 6: run_train(); break;
            case 7: add_passenger(); break;
            case 8: remove_passenger(); break;
            case 9: list_trains(); break;
            case 10: list_stations(); break;
            case 11: exit(0);
            default: printf("Invalid choice.\n");
        }
    }
}

// ---------------- Main ----------------
int main(){
    menu();
    return 0;
}