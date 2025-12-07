/*
 srms_fixed_portable.c
 Portable single-file SRMS (fixed & cleaned)
 Compiles on Linux/macOS (gcc/clang) and Windows (MinGW).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <conio.h>
  #define CLEAR_CMD "cls"
  #define OS_WINDOWS 1
#else
  #include <termios.h>
  #include <unistd.h>
  #define CLEAR_CMD "clear"
  #define OS_WINDOWS 0
#endif

/* ---- Config ---- */
#define STUDENT_FILE "students.txt"
#define CREDENTIAL_FILE "credentials.txt"
#define BACKUP_FILE "students_backup.txt"
#define REPORT_FILE "report.txt"
#define CSV_FILE "students.csv"

#define MAX_NAME 100
#define MAX_USER 50
#define MAX_ROLE 16
#define SUBJECTS 3
const char *subjectNames[SUBJECTS] = {"Math", "Science", "English"};

/* ---- Types ---- */
typedef struct {
    int roll;
    char name[MAX_NAME];
    float marks[SUBJECTS];
    float total;
    float percentage;
    char grade[4];
} Student;

/* ---- Globals ---- */
char currentUser[MAX_USER] = {0};
char currentRole[MAX_ROLE] = {0};

/* ---- Prototypes (all functions declared for clarity) ---- */
/* utilities */
void clear_input_line(void);
void pause_and_wait(void);
void clear_screen(void);
int yesno(const char *prompt);
void safe_gets(char *buf, int n);
int contains_case_insensitive(const char *hay, const char *needle);
void get_password(char *out, int maxlen);
void xor_file(const char *filename, const char key);

/* validation & student helpers */
void calculate_student(Student *s);
int valid_name(const char *name);
int valid_marks(float mark);

/* file helpers */
int write_student_to_file(FILE *fp, const Student *s);
int parse_line_to_student(const char *line, Student *s);
Student *read_all_students(int *outCount);
int roll_exists(int roll);
int append_student(const Student *s);
int overwrite_students(Student *arr, int count);

/* credentials */
int check_credentials(const char *username, const char *password, char *outRole);
int add_credential(const char *user, const char *pass, const char *role);
int reset_password(const char *user, const char *newpass);
int remove_credential(const char *user);

/* features */
void show_banner(void);
void feature_add_student(void);
int display_students_table(Student *arr, int count);
void feature_display_all(void);
void feature_search(void);
void feature_update_student(void);
void feature_delete_student(void);
void feature_delete_all(void);
void feature_sorting(void);
void feature_statistics(void);
void feature_export(void);
void feature_backup(void);
void feature_restore(void);
void feature_manage_credentials(void);
void feature_toggle_encryption(void);

/* menus */
void admin_menu(void);
void staff_menu(void);
void guest_menu(void);
void principal_menu(void);
void student_menu(void);
void main_menu_dispatch(void);

/* login & bootstrap */
int login_system(void);
void ensure_default_credentials(void);

/* portable strcasecmp fallback */
int portable_strcasecmp(const char *a, const char *b);

/* ---- Implementations ---- */

void clear_input_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void pause_and_wait(void) {
    printf("\nPress ENTER to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {} /* consume rest of line if any */
    /* now wait for the ENTER */
    /* If input buffer already cleared, read one char and ignore; simple approach: */
    /* just return because previous scanf/fgets flow ensures prompt will behave correctly */
}

void clear_screen(void) {
    system(CLEAR_CMD);
}

int yesno(const char *prompt) {
    char buf[8];
    printf("%s (y/n): ", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    return (buf[0] == 'y' || buf[0] == 'Y');
}

void safe_gets(char *buf, int n) {
    if (!fgets(buf, n, stdin)) { buf[0] = '\0'; return; }
    buf[strcspn(buf, "\n")] = '\0';
}

/* case-insensitive substring check (portable) */
int contains_case_insensitive(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    size_t hn = strlen(hay), nn = strlen(needle);
    if (nn == 0) return 1;
    for (size_t i = 0; i + nn <= hn; ++i) {
        size_t j;
        for (j = 0; j < nn; ++j) {
            char a = tolower((unsigned char)hay[i + j]);
            char b = tolower((unsigned char)needle[j]);
            if (a != b) break;
        }
        if (j == nn) return 1;
    }
    return 0;
}

/* portable strcasecmp fallback */
int portable_strcasecmp(const char *a, const char *b) {
    for (;; a++, b++) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return (ca - cb);
        if (ca == 0) return 0;
    }
}

/* password input: Windows uses _getch, POSIX uses termios to disable echo */
void get_password(char *out, int maxlen) {
#if OS_WINDOWS
    int idx = 0;
    int ch;
    while ((ch = _getch()) != '\r' && ch != '\n' && idx + 1 < maxlen) {
        if (ch == '\b') {
            if (idx > 0) { idx--; printf("\b \b"); }
        } else {
            out[idx++] = (char)ch;
            printf("*");
        }
    }
    out[idx] = '\0';
    printf("\n");
#else
    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        /* fallback to fgets */
        if (!fgets(out, maxlen, stdin)) out[0] = '\0';
        out[strcspn(out, "\n")] = '\0';
        return;
    }
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    if (fgets(out, maxlen, stdin) == NULL) out[0] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    out[strcspn(out, "\n")] = '\0';
    printf("\n");
#endif
}

/* XOR transform for a file (simple demo encryption) */
void xor_file(const char *filename, const char key) {
    FILE *f = fopen(filename, "rb+");
    if (!f) return;
    unsigned char buf[4096];
    size_t n;
    long pos = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < n; ++i) buf[i] ^= (unsigned char)key;
        fseek(f, pos, SEEK_SET);
        fwrite(buf, 1, n, f);
        pos = ftell(f);
        fseek(f, pos, SEEK_SET);
    }
    fclose(f);
}

/* ---- Student scoring & validation ---- */
void calculate_student(Student *s) {
    s->total = 0.0f;
    for (int i = 0; i < SUBJECTS; ++i) s->total += s->marks[i];
    s->percentage = (s->total / (100.0f * SUBJECTS)) * 100.0f;
    if (s->percentage >= 90.0f) strcpy(s->grade, "A+");
    else if (s->percentage >= 80.0f) strcpy(s->grade, "A");
    else if (s->percentage >= 70.0f) strcpy(s->grade, "B");
    else if (s->percentage >= 60.0f) strcpy(s->grade, "C");
    else if (s->percentage >= 50.0f) strcpy(s->grade, "D");
    else strcpy(s->grade, "F");
}

int valid_name(const char *name) {
    return name && name[0] != '\0';
}

int valid_marks(float mark) {
    return (mark >= 0.0f && mark <= 100.0f);
}

/* ---- File helpers ---- */
/* Line format: roll|name|m1|m2|m3\n */
int write_student_to_file(FILE *fp, const Student *s) {
    if (!fp || !s) return 0;
    fprintf(fp, "%d|%s", s->roll, s->name);
    for (int i = 0; i < SUBJECTS; ++i) fprintf(fp, "|%.2f", s->marks[i]);
    fprintf(fp, "\n");
    return 1;
}

int parse_line_to_student(const char *line, Student *s) {
    if (!line || !s) return 0;
    char *copy = malloc(strlen(line) + 1);
    if (!copy) return 0;
    strcpy(copy, line);
    size_t len = strlen(copy);
    if (len && copy[len - 1] == '\n') copy[len - 1] = '\0';
    char *tok = strtok(copy, "|");
    if (!tok) { free(copy); return 0; }
    s->roll = atoi(tok);
    tok = strtok(NULL, "|");
    if (!tok) { free(copy); return 0; }
    strncpy(s->name, tok, MAX_NAME - 1);
    s->name[MAX_NAME - 1] = '\0';
    for (int i = 0; i < SUBJECTS; ++i) {
        tok = strtok(NULL, "|");
        s->marks[i] = tok ? (float)atof(tok) : 0.0f;
    }
    calculate_student(s);
    free(copy);
    return 1;
}

Student *read_all_students(int *outCount) {
    *outCount = 0;
    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) return NULL;
    Student *arr = NULL;
    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        Student s;
        if (parse_line_to_student(line, &s)) {
            Student *tmp = realloc(arr, (count + 1) * sizeof(Student));
            if (!tmp) { free(arr); fclose(fp); *outCount = 0; return NULL; }
            arr = tmp;
            arr[count++] = s;
        }
    }
    fclose(fp);
    *outCount = count;
    return arr;
}

int roll_exists(int roll) {
    FILE *fp = fopen(STUDENT_FILE, "r");
    if (!fp) return 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        Student s;
        if (parse_line_to_student(line, &s)) {
            if (s.roll == roll) { fclose(fp); return 1; }
        }
    }
    fclose(fp);
    return 0;
}

int append_student(const Student *s) {
    FILE *fp = fopen(STUDENT_FILE, "a");
    if (!fp) return 0;
    int ok = write_student_to_file(fp, s);
    fclose(fp);
    return ok;
}

int overwrite_students(Student *arr, int count) {
    FILE *fp = fopen(STUDENT_FILE, "w");
    if (!fp) return 0;
    for (int i = 0; i < count; ++i) write_student_to_file(fp, &arr[i]);
    fclose(fp);
    return 1;
}

/* ---- Credentials helpers ---- */
int check_credentials(const char *username, const char *password, char *outRole) {
    FILE *fp = fopen(CREDENTIAL_FILE, "r");
    if (!fp) return 0;
    char user[128], pass[128], role[64];
    while (fscanf(fp, "%127s %127s %63s", user, pass, role) == 3) {
        if (strcmp(user, username) == 0 && strcmp(pass, password) == 0) {
            if (outRole) strncpy(outRole, role, MAX_ROLE - 1);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int add_credential(const char *user, const char *pass, const char *role) {
    FILE *fp = fopen(CREDENTIAL_FILE, "a");
    if (!fp) return 0;
    fprintf(fp, "%s %s %s\n", user, pass, role);
    fclose(fp);
    return 1;
}

int reset_password(const char *user, const char *newpass) {
    FILE *fp = fopen(CREDENTIAL_FILE, "r");
    if (!fp) return 0;
    char lines[1024][256];
    int n = 0;
    char u[128], p[128], r[64];
    int found = 0;
    while (fscanf(fp, "%127s %127s %63s", u, p, r) == 3) {
        if (strcmp(u, user) == 0) {
            snprintf(lines[n++], sizeof(lines[n-1]), "%s %s %s\n", u, newpass, r);
            found = 1;
        } else {
            snprintf(lines[n++], sizeof(lines[n-1]), "%s %s %s\n", u, p, r);
        }
    }
    fclose(fp);
    if (!found) return 0;
    fp = fopen(CREDENTIAL_FILE, "w");
    if (!fp) return 0;
    for (int i = 0; i < n; ++i) fputs(lines[i], fp);
    fclose(fp);
    return 1;
}

int remove_credential(const char *user) {
    FILE *fp = fopen(CREDENTIAL_FILE, "r");
    if (!fp) return 0;
    char lines[1024][256];
    int n = 0;
    char u[128], p[128], r[64];
    int found = 0;
    while (fscanf(fp, "%127s %127s %63s", u, p, r) == 3) {
        if (strcmp(u, user) == 0) { found = 1; continue; }
        snprintf(lines[n++], sizeof(lines[n-1]), "%s %s %s\n", u, p, r);
    }
    fclose(fp);
    if (!found) return 0;
    fp = fopen(CREDENTIAL_FILE, "w");
    if (!fp) return 0;
    for (int i = 0; i < n; ++i) fputs(lines[i], fp);
    fclose(fp);
    return 1;
}

/* ---- Features ---- */
void show_banner(void) {
    printf("============================================\n");
    printf("     STUDENT RECORD MANAGEMENT SYSTEM       \n");
    printf("============================================\n");
    if (currentUser[0]) printf("Logged in as: %s [%s]\n", currentUser, currentRole);
    printf("--------------------------------------------\n");
}

void feature_add_student(void) {
    if (strcmp(currentRole, "ADMIN") != 0 && strcmp(currentRole, "STAFF") != 0) {
        printf("Permission denied: Only ADMIN/STAFF can add students.\n");
        return;
    }
    Student s;
    printf("Enter Roll Number: ");
    if (scanf("%d", &s.roll) != 1) { clear_input_line(); printf("Invalid roll.\n"); return; }
    clear_input_line();
    if (roll_exists(s.roll)) { printf("Roll number already exists!\n"); return; }
    printf("Enter Name: ");
    safe_gets(s.name, MAX_NAME);
    if (!valid_name(s.name)) { printf("Invalid name.\n"); return; }
    for (int i = 0; i < SUBJECTS; ++i) {
        printf("Enter marks for %s (0-100): ", subjectNames[i]);
        if (scanf("%f", &s.marks[i]) != 1) { clear_input_line(); printf("Invalid marks input.\n"); return; }
        if (!valid_marks(s.marks[i])) { printf("Marks must be 0-100.\n"); return; }
    }
    clear_input_line();
    calculate_student(&s);
    if (append_student(&s)) printf("Student added successfully!\n");
    else printf("Error: could not append to file.\n");
}

void print_students_header(void) {
    printf("\n%-6s %-20s", "Roll", "Name");
    for (int i = 0; i < SUBJECTS; ++i) printf(" %-8s", subjectNames[i]);
    printf(" %-8s %-10s %-6s\n", "Total", "Percent", "Grade");
    printf("-------------------------------------------------------------------------------\n");
}

int display_students_table(Student *arr, int count) {
    if (count == 0) { printf("No student records.\n"); return 0; }
    print_students_header();
    for (int i = 0; i < count; ++i) {
        printf("%-6d %-20s", arr[i].roll, arr[i].name);
        for (int j = 0; j < SUBJECTS; ++j) printf(" %-8.2f", arr[i].marks[j]);
        printf(" %-8.2f %-10.2f %-6s\n", arr[i].total, arr[i].percentage, arr[i].grade);
    }
    return 1;
}

void feature_display_all(void) {
    int n;
    Student *arr = read_all_students(&n);
    if (!arr) { printf("No records to display.\n"); return; }
    display_students_table(arr, n);
    free(arr);
}

void feature_search(void) {
    printf("\nSearch by:\n1) Name (partial)\n2) Roll No\n3) Marks Range\n4) Grade\nEnter choice: ");
    int ch;
    if (scanf("%d", &ch) != 1) { clear_input_line(); printf("Invalid.\n"); return; }
    clear_input_line();
    int n;
    Student *arr = read_all_students(&n);
    if (!arr || n == 0) { printf("No records.\n"); free(arr); return; }
    int found = 0;
    if (ch == 1) {
        char q[128];
        printf("Enter name or partial: ");
        safe_gets(q, sizeof(q));
        for (int i = 0; i < n; ++i) {
            if (contains_case_insensitive(arr[i].name, q)) {
                if (!found) print_students_header();
                printf("%-6d %-20s", arr[i].roll, arr[i].name);
                for (int j = 0; j < SUBJECTS; ++j) printf(" %-8.2f", arr[i].marks[j]);
                printf(" %-8.2f %-10.2f %-6s\n", arr[i].total, arr[i].percentage, arr[i].grade);
                found = 1;
            }
        }
    } else if (ch == 2) {
        int r;
        printf("Enter roll: ");
        if (scanf("%d", &r) != 1) { clear_input_line(); printf("Invalid.\n"); free(arr); return; }
        clear_input_line();
        for (int i = 0; i < n; ++i) if (arr[i].roll == r) { display_students_table(&arr[i], 1); found = 1; break; }
    } else if (ch == 3) {
        float lo, hi;
        printf("Enter lower bound of percentage: ");
        if (scanf("%f", &lo) != 1) { clear_input_line(); printf("Invalid.\n"); free(arr); return; }
        printf("Enter upper bound of percentage: ");
        if (scanf("%f", &hi) != 1) { clear_input_line(); printf("Invalid.\n"); free(arr); return; }
        clear_input_line();
        for (int i = 0; i < n; ++i) if (arr[i].percentage >= lo && arr[i].percentage <= hi) {
            if (!found) print_students_header();
            printf("%-6d %-20s %-8.2f\n", arr[i].roll, arr[i].name, arr[i].percentage);
            found = 1;
        }
    } else if (ch == 4) {
        char gradeQuery[8];
        printf("Enter grade to search (A+, A, B, C, D, F): ");
        safe_gets(gradeQuery, sizeof(gradeQuery));
        for (int i = 0; i < n; ++i) {
            if (portable_strcasecmp(arr[i].grade, gradeQuery) == 0) {
                if (!found) print_students_header();
                printf("%-6d %-20s %-6s %-8.2f\n", arr[i].roll, arr[i].name, arr[i].grade, arr[i].percentage);
                found = 1;
            }
        }
    } else {
        printf("Invalid option.\n");
    }
    if (!found) printf("No matching records found.\n");
    free(arr);
}

void feature_update_student(void) {
    if (strcmp(currentRole, "ADMIN") != 0 && strcmp(currentRole, "STAFF") != 0) {
        printf("Permission denied: Only ADMIN/STAFF can update students.\n"); return;
    }
    int roll;
    printf("Enter roll to update: ");
    if (scanf("%d", &roll) != 1) { clear_input_line(); printf("Invalid.\n"); return; }
    clear_input_line();
    int n;
    Student *arr = read_all_students(&n);
    if (!arr) { printf("No records.\n"); return; }
    int found = 0;
    for (int i = 0; i < n; ++i) {
        if (arr[i].roll == roll) {
            found = 1;
            printf("Current name: %s\nNew name (blank to keep): ", arr[i].name);
            char tmp[MAX_NAME]; safe_gets(tmp, sizeof(tmp));
            if (strlen(tmp) > 0) strncpy(arr[i].name, tmp, MAX_NAME - 1);
            for (int j = 0; j < SUBJECTS; ++j) {
                printf("Current %s: %.2f\nNew %s (-1 to keep): ", subjectNames[j], arr[i].marks[j], subjectNames[j]);
                float m;
                if (scanf("%f", &m) != 1) { clear_input_line(); printf("Invalid input. Skipping.\n"); continue; }
                if (m >= 0.0f && m <= 100.0f) arr[i].marks[j] = m;
            }
            clear_input_line();
            calculate_student(&arr[i]);
            break;
        }
    }
    if (!found) { printf("Roll not found.\n"); free(arr); return; }
    if (!overwrite_students(arr, n)) printf("Error saving updates.\n"); else printf("Record updated.\n");
    free(arr);
}

void feature_delete_student(void) {
    if (strcmp(currentRole, "ADMIN") != 0 && strcmp(currentRole, "STAFF") != 0) { printf("Permission denied.\n"); return; }
    int roll;
    printf("Enter roll to delete: ");
    if (scanf("%d", &roll) != 1) { clear_input_line(); printf("Invalid.\n"); return; }
    clear_input_line();
    int n;
    Student *arr = read_all_students(&n);
    if (!arr) { printf("No records.\n"); return; }
    int idx = -1;
    for (int i = 0; i < n; ++i) if (arr[i].roll == roll) { idx = i; break; }
    if (idx == -1) { printf("Roll not found.\n"); free(arr); return; }
    for (int i = idx; i < n - 1; ++i) arr[i] = arr[i + 1];
    n--;
    if (!overwrite_students(arr, n)) printf("Error deleting.\n"); else printf("Deleted successfully.\n");
    free(arr);
}

void feature_delete_all(void) {
    if (strcmp(currentRole, "ADMIN") != 0) { printf("Only ADMIN can delete all records.\n"); return; }
    if (!yesno("Are you sure you want to DELETE ALL STUDENT RECORDS?")) { printf("Operation cancelled.\n"); return; }
    FILE *fp = fopen(STUDENT_FILE, "w");
    if (!fp) { printf("Error clearing file.\n"); return; }
    fclose(fp);
    printf("All records deleted.\n");
}

/* Sorting helpers */
int cmp_roll_asc(const void *a, const void *b) { return ((Student*)a)->roll - ((Student*)b)->roll; }
int cmp_roll_desc(const void *a, const void *b) { return ((Student*)b)->roll - ((Student*)a)->roll; }
int cmp_name(const void *a, const void *b) { return portable_strcasecmp(((Student*)a)->name, ((Student*)b)->name); }
int cmp_marks_desc(const void *a, const void *b) {
    float diff = ((Student*)b)->total - ((Student*)a)->total;
    if (diff > 0) return 1;
    if (diff < 0) return -1;
    return 0;
}

void feature_sorting(void) {
    int n;
    Student *arr = read_all_students(&n);
    if (!arr || n == 0) { printf("No records to sort.\n"); free(arr); return; }
    printf("Sort by:\n1) Roll Asc\n2) Roll Desc\n3) Name\n4) Total Marks Desc\nEnter choice: ");
    int ch;
    if (scanf("%d", &ch) != 1) { clear_input_line(); printf("Invalid.\n"); free(arr); return; }
    clear_input_line();
    if (ch == 1) qsort(arr, n, sizeof(Student), cmp_roll_asc);
    else if (ch == 2) qsort(arr, n, sizeof(Student), cmp_roll_desc);
    else if (ch == 3) qsort(arr, n, sizeof(Student), cmp_name);
    else if (ch == 4) qsort(arr, n, sizeof(Student), cmp_marks_desc);
    else { printf("Invalid choice.\n"); free(arr); return; }
    display_students_table(arr, n);
    if (yesno("Save sorted order to file?")) {
        if (overwrite_students(arr, n)) printf("Saved.\n"); else printf("Error saving.\n");
    }
    free(arr);
}

void feature_statistics(void) {
    int n;
    Student *arr = read_all_students(&n);
    if (!arr || n == 0) { printf("No records.\n"); free(arr); return; }
    float maxPerc = -1.0f, minPerc = 101.0f, sum = 0.0f;
    int pass = 0;
    int maxIdx = 0, minIdx = 0;
    for (int i = 0; i < n; ++i) {
        sum += arr[i].percentage;
        if (arr[i].percentage > maxPerc) { maxPerc = arr[i].percentage; maxIdx = i; }
        if (arr[i].percentage < minPerc) { minPerc = arr[i].percentage; minIdx = i; }
        if (arr[i].percentage >= 50.0f) pass++;
    }
    printf("\nTotal Students: %d\nAverage Percentage: %.2f\nHighest: %.2f (%s, Roll %d)\nLowest: %.2f (%s, Roll %d)\nPass Count: %d\nFail Count: %d\n",
           n, sum / n, arr[maxIdx].percentage, arr[maxIdx].name, arr[maxIdx].roll, arr[minIdx].percentage, arr[minIdx].name, arr[minIdx].roll, pass, n - pass);
    free(arr);
}

void feature_export(void) {
    int n;
    Student *arr = read_all_students(&n);
    if (!arr || n == 0) { printf("No records to export.\n"); free(arr); return; }
    FILE *fcsv = fopen(CSV_FILE, "w");
    FILE *fr = fopen(REPORT_FILE, "w");
    if (!fcsv || !fr) { printf("Error creating export files.\n"); if (fcsv) fclose(fcsv); if (fr) fclose(fr); free(arr); return; }
    fprintf(fcsv, "Roll,Name");
    for (int i = 0; i < SUBJECTS; ++i) fprintf(fcsv, ",%s", subjectNames[i]);
    fprintf(fcsv, ",Total,Percentage,Grade\n");
    for (int i = 0; i < n; ++i) {
        fprintf(fcsv, "%d,\"%s\"", arr[i].roll, arr[i].name);
        for (int j = 0; j < SUBJECTS; ++j) fprintf(fcsv, ",%.2f", arr[i].marks[j]);
        fprintf(fcsv, ",%.2f,%.2f,%s\n", arr[i].total, arr[i].percentage, arr[i].grade);
    }
    time_t now = time(NULL);
    char *ts = ctime(&now);
    if (!ts) ts = "unknown time\n";
    fprintf(fr, "Student Report Generated on %s\n\n", ts);
    for (int i = 0; i < n; ++i) {
        fprintf(fr, "Roll: %d\nName: %s\n", arr[i].roll, arr[i].name);
        for (int j = 0; j < SUBJECTS; ++j) fprintf(fr, "%s: %.2f\n", subjectNames[j], arr[i].marks[j]);
        fprintf(fr, "Total: %.2f\nPercentage: %.2f\nGrade: %s\n-----------------\n", arr[i].total, arr[i].percentage, arr[i].grade);
    }
    fclose(fcsv);
    fclose(fr);
    printf("Exported to %s and %s\n", CSV_FILE, REPORT_FILE);
    free(arr);
}

void feature_backup(void) {
    FILE *src = fopen(STUDENT_FILE, "r");
    if (!src) { printf("No data to backup.\n"); return; }
    FILE *dst = fopen(BACKUP_FILE, "w");
    if (!dst) { printf("Error creating backup.\n"); fclose(src); return; }
    char buf[1024];
    while (fgets(buf, sizeof(buf), src)) fputs(buf, dst);
    fclose(src); fclose(dst);
    printf("Backup saved to %s\n", BACKUP_FILE);
}

void feature_restore(void) {
    if (!yesno("Restore from backup? This will overwrite current records.")) { printf("Restore cancelled.\n"); return; }
    FILE *src = fopen(BACKUP_FILE, "r");
    if (!src) { printf("Backup file not found.\n"); return; }
    FILE *dst = fopen(STUDENT_FILE, "w");
    if (!dst) { printf("Error restoring.\n"); fclose(src); return; }
    char buf[1024];
    while (fgets(buf, sizeof(buf), src)) fputs(buf, dst);
    fclose(src); fclose(dst);
    printf("Restore complete.\n");
}

void feature_manage_credentials(void) {
    if (strcmp(currentRole, "ADMIN") != 0) { printf("Only ADMIN can manage users.\n"); return; }
    printf("\nCredentials Manager:\n1) Add User\n2) Reset Password\n3) Remove User\nEnter choice: ");
    int ch;
    if (scanf("%d", &ch) != 1) { clear_input_line(); printf("Invalid.\n"); return; }
    clear_input_line();
    if (ch == 1) {
        char user[128], pass[128], role[64];
        printf("Username: "); safe_gets(user, sizeof(user));
        printf("Password: "); get_password(pass, sizeof(pass));
        printf("Role (ADMIN/STAFF/PRINCIPAL/STUDENT/GUEST): "); safe_gets(role, sizeof(role));
        for (char *p = role; *p; ++p) *p = toupper((unsigned char)*p);
        if (add_credential(user, pass, role)) printf("User added.\n"); else printf("Error.\n");
    } else if (ch == 2) {
        char user[128], pass[128];
        printf("Username to reset: "); safe_gets(user, sizeof(user));
        printf("New password: "); get_password(pass, sizeof(pass));
        if (reset_password(user, pass)) printf("Password reset.\n"); else printf("User not found.\n");
    } else if (ch == 3) {
        char user[128];
        printf("Username to remove: "); safe_gets(user, sizeof(user));
        if (remove_credential(user)) printf("User removed.\n"); else printf("User not found.\n");
    } else printf("Invalid.\n");
}

void feature_toggle_encryption(void) {
    if (strcmp(currentRole, "ADMIN") != 0) { printf("Only ADMIN can toggle encryption.\n"); return; }
    if (!yesno("Toggle XOR encryption for student file? (this will apply XOR to current file)")) { printf("Cancelled.\n"); return; }
    char keych;
    printf("Enter single character key: ");
    keych = (char)getchar();
    clear_input_line();
    xor_file(STUDENT_FILE, keych);
    printf("XOR applied with key '%c'. (Run again with same key to decrypt)\n", keych);
}

/* ---- Menus & dispatch ---- */
void main_menu_dispatch(void) {
    if (strcmp(currentRole, "ADMIN") == 0) admin_menu();
    else if (strcmp(currentRole, "STAFF") == 0) staff_menu();
    else if (strcmp(currentRole, "PRINCIPAL") == 0) principal_menu();
    else if (strcmp(currentRole, "STUDENT") == 0) student_menu();
    else guest_menu();
}

void common_reports_menu(void) {
    printf("\n1) Export (CSV & Report)\n2) Backup\n3) Restore\n4) Toggle Encryption (ADMIN only)\n5) Back\nEnter choice: ");
    int c;
    if (scanf("%d", &c) != 1) { clear_input_line(); return; }
    clear_input_line();
    switch (c) {
        case 1: feature_export(); break;
        case 2: feature_backup(); break;
        case 3: feature_restore(); break;
        case 4: feature_toggle_encryption(); break;
        default: return;
    }
}

void admin_menu(void) {
    int ch;
    do {
        clear_screen(); show_banner();
        printf("ADMIN MENU\n1) Add Student\n2) Display All\n3) Search\n4) Update\n5) Delete\n6) Delete All (Reset)\n7) Sorting\n8) Statistics\n9) Manage Credentials\n10) Reports/Backup\n11) Logout\nChoose: ");
        if (scanf("%d", &ch) != 1) { clear_input_line(); ch = -1; }
        clear_input_line();
        switch (ch) {
            case 1: feature_add_student(); break;
            case 2: feature_display_all(); break;
            case 3: feature_search(); break;
            case 4: feature_update_student(); break;
            case 5: feature_delete_student(); break;
            case 6: feature_delete_all(); break;
            case 7: feature_sorting(); break;
            case 8: feature_statistics(); break;
            case 9: feature_manage_credentials(); break;
            case 10: common_reports_menu(); break;
            case 11: printf("Logging out...\n"); return;
            default: printf("Invalid choice.\n");
        }
        pause_and_wait();
    } while (1);
}

void staff_menu(void) {
    int ch;
    do {
        clear_screen(); show_banner();
        printf("STAFF MENU\n1) Display All\n2) Search\n3) Add Student\n4) Update Student\n5) Delete Student\n6) Sorting\n7) Statistics\n8) Reports/Backup\n9) Logout\nChoose: ");
        if (scanf("%d", &ch) != 1) { clear_input_line(); ch = -1; }
        clear_input_line();
        switch (ch) {
            case 1: feature_display_all(); break;
            case 2: feature_search(); break;
            case 3: feature_add_student(); break;
            case 4: feature_update_student(); break;
            case 5: feature_delete_student(); break;
            case 6: feature_sorting(); break;
            case 7: feature_statistics(); break;
            case 8: common_reports_menu(); break;
            case 9: printf("Logging out...\n"); return;
            default: printf("Invalid choice.\n");
        }
        pause_and_wait();
    } while (1);
}

void guest_menu(void) {
    int ch;
    do {
        clear_screen(); show_banner();
        printf("GUEST MENU\n1) Display All\n2) Search\n3) Reports/Backup\n4) Logout\nChoose: ");
        if (scanf("%d", &ch) != 1) { clear_input_line(); ch = -1; }
        clear_input_line();
        switch (ch) {
            case 1: feature_display_all(); break;
            case 2: feature_search(); break;
            case 3: common_reports_menu(); break;
            case 4: printf("Logging out...\n"); return;
            default: printf("Invalid choice.\n");
        }
        pause_and_wait();
    } while (1);
}

void principal_menu(void) {
    int ch;
    do {
        clear_screen(); show_banner();
        printf("PRINCIPAL MENU\n1) Display All\n2) Search\n3) Statistics\n4) Reports/Backup\n5) Logout\nChoose: ");
        if (scanf("%d", &ch) != 1) { clear_input_line(); ch = -1; }
        clear_input_line();
        switch (ch) {
            case 1: feature_display_all(); break;
            case 2: feature_search(); break;
            case 3: feature_statistics(); break;
            case 4: common_reports_menu(); break;
            case 5: printf("Logging out...\n"); return;
            default: printf("Invalid choice.\n");
        }
        pause_and_wait();
    } while (1);
}

void student_menu(void) {
    int ch;
    do {
        clear_screen(); show_banner();
        printf("STUDENT MENU\n1) View My Record\n2) Logout\nChoose: ");
        if (scanf("%d", &ch) != 1) { clear_input_line(); ch = -1; }
        clear_input_line();
        if (ch == 1) {
            int n;
            Student *arr = read_all_students(&n);
            if (!arr || n == 0) { printf("No records.\n"); free(arr); }
            else {
                int found = 0;
                int isnum = 1;
                for (size_t i = 0; i < strlen(currentUser); ++i)
                    if (!isdigit((unsigned char)currentUser[i])) { isnum = 0; break; }
                if (isnum) {
                    int r = atoi(currentUser);
                    for (int i = 0; i < n; ++i) if (arr[i].roll == r) { display_students_table(&arr[i], 1); found = 1; break; }
                } else {
                    for (int i = 0; i < n; ++i) if (portable_strcasecmp(arr[i].name, currentUser) == 0) { display_students_table(&arr[i], 1); found = 1; break; }
                }
                if (!found) printf("No record found for you.\n");
                free(arr);
            }
        } else if (ch == 2) { printf("Logging out...\n"); return; }
        else printf("Invalid.\n");
        pause_and_wait();
    } while (1);
}

/* ---- Login & bootstrap ---- */
int login_system(void) {
    int attempts = 0;
    while (attempts < 3) {
        clear_screen();
        show_banner();
        char user[128], pass[128], rolebuf[64];
        printf("Username: ");
        if (scanf("%127s", user) != 1) { clear_input_line(); continue; }
        clear_input_line();
        printf("Password: ");
        get_password(pass, sizeof(pass));
        if (check_credentials(user, pass, rolebuf)) {
            strncpy(currentUser, user, MAX_USER - 1);
            currentUser[MAX_USER - 1] = '\0';
            strncpy(currentRole, rolebuf, MAX_ROLE - 1);
            currentRole[MAX_ROLE - 1] = '\0';
            printf("Login successful. Welcome %s [%s]\n", currentUser, currentRole);
            return 1;
        } else {
            attempts++;
            printf("Invalid credentials. Attempts left: %d\n", 3 - attempts);
            pause_and_wait();
        }
    }
    printf("Maximum attempts reached.\n");
    return 0;
}

void ensure_default_credentials(void) {
    FILE *f = fopen(CREDENTIAL_FILE, "r");
    if (f) { fclose(f); return; }
    f = fopen(CREDENTIAL_FILE, "w");
    if (!f) return;
    fprintf(f, "admin admin ADMIN\n");
    fprintf(f, "staff staff STAFF\n");
    fprintf(f, "guest guest GUEST\n");
    fprintf(f, "principal principal PRINCIPAL\n");
    fprintf(f, "student student STUDENT\n");
    fclose(f);
}

/* ---- main ---- */
int main(void) {
    ensure_default_credentials();
    clear_screen();
    printf("Advanced SRMS - Fixed portable version\n");

    

    if (!login_system()) { printf("Exiting...\n"); return 0; }
    main_menu_dispatch();

    printf("Goodbye.\n");
    return 0;
}